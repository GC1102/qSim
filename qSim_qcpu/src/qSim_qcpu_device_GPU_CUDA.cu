/*
 * qSim_qcup_device_GPU_CUDA.cu
 *
 * --------------------------------------------------------------------------
 * Copyright (C) 2022 Gianni Casonato
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * --------------------------------------------------------------------------
 *
 *  Created on: May 4, 2022
 *      Author: gianni
 *
 * Q-CPU support module, providing functions for CUDA based transformations handling,
 * applying a function based approach, and supporting:
 * - single or repeated function block
 * - function block "gap filling" before and after
 *
 * Implemented functions:
 * - basic transformations (I, X, H, CX)
 * - extended transformations (SWAP, Toffoli, Fredkin, QFT) -> TO BE DONE
 * - custom transformations (look-up-table based) -> TO BE DONE
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation.
 *  1.1   Nov-2022   Instruction set limitation to 1 and 2 qubit gates.
 *                   Transformed to class.
 *                   Defined and handled a function arguments structure.
 *                   Code clean-up.
 *                   Module renamed to qSim_qcup_device_GPU_CUDA.
 *
 *  -------------------------------------------------------------------------- 
 */


#include <stdio.h>
#include <math.h>

#include "qSim_qcpu_device_function_exec.h"
#include "qSim_qcpu_device_GPU_CUDA.h"


#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// --------------------------------------------------------------
//Device 0: "NVIDIA GeForce GT 1030"
//  CUDA Driver Version / Runtime Version          11.4 / 9.2
//  CUDA Capability Major/Minor version number:    6.1
//  Total amount of global memory:                 2001 MBytes (2098135040 bytes)
//  Total amount of constant memory:               65536 bytes
//  Total amount of shared memory per block:       49152 bytes
//  Total number of registers available per block: 65536
//  Warp size:                                     32
//  Maximum number of threads per multiprocessor:  2048
//  Maximum number of threads per block:           1024
//  Max dimension size of a thread block (x,y,z): (1024, 1024, 64)
//  Max dimension size of a grid size    (x,y,z): (2147483647, 65535, 65535)
// --------------------------------------------------------------

// --------------------------------------------------------------
//Device 0: "GRID T4-1Q"
//  CUDA Driver Version / Runtime Version          11.4 / 10.2
//  CUDA Capability Major/Minor version number:    7.5
//  Total amount of global memory:                 1024 MBytes (1073741824 bytes)
//  Total amount of constant memory:               65536 bytes
//  Total amount of shared memory per block:       49152 bytes
//  Total number of registers available per block: 65536
//  Warp size:                                     32
//  Maximum number of threads per multiprocessor:  1024
//  Maximum number of threads per block:           1024
//  Max dimension size of a thread block (x,y,z): (1024, 1024, 64)
//  Max dimension size of a grid size    (x,y,z): (2147483647, 65535, 65535)
// --------------------------------------------------------------

#ifdef __CUDA_DYNPAR__

#define THREADS_PER_BLOCK 32 // best trade-off value for DP kernel

#else

#define THREADS_PER_BLOCK 32 // best trade-off value for SK kernel (similar to 64 or 128)

#endif

// --------------------------------
// kernel entry point functions
// --------------------------------

#ifdef __CUDA_DYNPAR__

// --------------------------------
// multi kernel case

__global__
void kernel_prod_ki(QDEV_ST_VAL_TYPE *x, int idx, int k_start, int k_stop, int k_step,
		            QASM_F_TYPE* d_ftype_cuda_vec, int* d_fn_cuda_vec,
					QDEV_F_ARGS_TYPE* d_fargs_cuda_vec, int tot_f,
					int /*ftype*/, int fn, int fform, int gapn, int futype, int fun, int fuform,
					double* d_y_real, double* d_y_img) {
	// dynamic parallelism case - child kernel
	int jdx = blockIdx.x * blockDim.x + threadIdx.x + k_start; // 1D vector: only x-dimension used
//	printf("ki_dp - level 2...idx: %d  jdx:%d\n", idx, jdx);

	// check for calculation limits considering LSQ gap fillers generated zeroes
	if ((jdx % k_step == idx % k_step) && (jdx < k_stop+1)) {
		// calculate current coefficient
		QDEV_ST_VAL_TYPE k = cuCmul(x[idx], f_dev_qn_exec(idx, jdx, d_ftype_cuda_vec, d_fn_cuda_vec,
				                                          fn, fform, gapn, futype, fun, fuform, d_fargs_cuda_vec, tot_f));

		// use atomic add on real and image parts separately
		atomicAdd(&(d_y_real[idx]), (double)k.x);
		atomicAdd(&(d_y_img[idx]), (double)k.y);
//		printf("fxi_dp - k: %f %f  ---  d_y: %f %f\n", k.x, k.y, d_y_real[i], d_y_img[i]);
	}
//	printf("ki...%d %d -> %f %f\n", i, idx, k->x, k->y);
}

__global__
void kernel_prod_fxi_dp(QDEV_ST_VAL_TYPE *x, QDEV_ST_VAL_TYPE *y, int N,
		                QASM_F_TYPE* d_ftype_cuda_vec, int* d_fn_cuda_vec,
		                QDEV_F_ARGS_TYPE* d_fargs_cuda_vec, int tot_f, int max_block_size, int block_inner_gap_size,
						int ftype, int fn, int fform, int gapn, int futype, int fun, int fuform,
						double* d_y_real, double* d_y_img) {
	// dynamic parallelism case - parent kernel
	int idx = blockIdx.x * blockDim.x + threadIdx.x; // 1D vector: only x-dimension used
//	printf("fxi_dp - level 1...idx: %d\n", idx);

	// combine all i-th row with x elements for y i-th result
	if (idx < N) {
		// define current calculation limits considering LSQ & MSQ gap fillers generated zeroes
	    int k_step = block_inner_gap_size;
		int k_start = max(0, (idx/max_block_size)*max_block_size);
		int k_stop = min(N-1, k_start+max_block_size-1);
		int k_N = k_stop - k_start + 1;
		int nblocks = (k_N+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;
		int nthreads = MIN(k_N, THREADS_PER_BLOCK);
		kernel_prod_ki<<<nblocks, nthreads>>>(x, idx, k_start, k_stop, k_step,
				                              d_ftype_cuda_vec, d_fn_cuda_vec, d_fargs_cuda_vec, tot_f,
											  ftype, fn, fform, gapn, futype, fun, fuform, d_y_real, d_y_img);
		cudaDeviceSynchronize(); // to sync children kernels - NEEDED!
		y[idx] = QDEV_ST_MAKE_VAL(d_y_real[idx], d_y_img[idx]);
//		printf("fxi_dp...%d -> %f %f\n", idx, y[idx].x, y[idx].y);
	}
}

#else

// --------------------------------
// single kernel case

__global__
void kernel_prod_fxi_sk(QDEV_ST_VAL_TYPE *x, QDEV_ST_VAL_TYPE *y, int N,
						QASM_F_TYPE* d_ftype_cuda_vec, int* d_fn_cuda_vec,
						QDEV_F_ARGS_TYPE* d_fargs_cuda_vec, int tot_f, int max_block_size, int block_inner_gap_size,
						int /*ftype*/, int fn, int fform, int gapn, int futype, int fun, int fuform) {
	// single kernel case
	int idx = blockIdx.x * blockDim.x + threadIdx.x; // 1D vector: only x-dimension used
//	printf("fxi_sk...idx: %d\n", idx);

	// combine all i-th row with x elements for y i-th result
	if (idx < N) {
		y[idx] = QDEV_ST_MAKE_VAL(0.0, 0.0);

		// define current calculation limits considering LSQ & MSQ gap fillers generated zeroes
	    int k_step = block_inner_gap_size;
		int k_start = max(0, (idx/max_block_size)*max_block_size);
		int k_stop = min(N-1, k_start+max_block_size-1);
//		printf("fxi_sk...idx: %d  N: %d  k_step: %d  k_start: %d  k_stop: %d\n", idx, N, k_step, k_start, k_stop);
		
		for (int k=k_start; k<k_stop+1; k++) {
			// check for calculation limits considering LSQ gap fillers generated zeroes
			if (k % k_step == idx % k_step) {
//				printf("...found idx: %d  k: %d", idx, k);
				y[idx] = cuCadd(y[idx], cuCmul(x[k], f_dev_qn_exec(idx, k, d_ftype_cuda_vec, d_fn_cuda_vec,
										       fn, fform, gapn, futype, fun, fuform, d_fargs_cuda_vec, tot_f)));
			}
//			printf("\n");
		}
//		printf("fxi_sk...%d -> %f %f\n", idx, y[idx].x, y[idx].y);
	}
}

#endif

// --------------------------------------------------------
// class methods
// --------------------------------------------------------

#define dev_qreg_MAX_N 20 			// max qureg size supported
#define CUDA_TOT_F dev_qreg_MAX_N  // bounded by max qureg size

// constructor & destructor
qSim_qcpu_device::qSim_qcpu_device() {
    // allocate function host vectors
    m_ftype_vec = (QASM_F_TYPE*)malloc(CUDA_TOT_F*sizeof(QASM_F_TYPE));
    m_fsize_vec = (int*)malloc(CUDA_TOT_F*sizeof(int));
    m_fargs_vec = (QDEV_F_ARGS_TYPE*)malloc(CUDA_TOT_F*sizeof(QDEV_F_ARGS_TYPE));

    // allocate function param vectors
	cudaMalloc((void**)&d_ftype_cuda_vec, CUDA_TOT_F*sizeof(QASM_F_TYPE));
	qSim_qcpu_device::checkCUDAError("cudaMalloc");
	cudaMalloc((void**)&d_fsize_cuda_vec, CUDA_TOT_F*sizeof(int));
	qSim_qcpu_device::checkCUDAError("cudaMalloc");
	cudaMalloc((void**)&d_fargs_cuda_vec, CUDA_TOT_F*sizeof(QDEV_F_ARGS_TYPE));
	qSim_qcpu_device::checkCUDAError("cudaMalloc");

#ifdef __CUDA_DYNPAR__
	// DP case specific part
	int qreg_max_stn = pow(2, dev_qreg_MAX_N);
	cudaMalloc(&d_y_real, qreg_max_stn*sizeof(double));
	qSim_qcpu_device::checkCUDAError("cudaMalloc");
	cudaMalloc(&d_y_img, qreg_max_stn*sizeof(double));
	qSim_qcpu_device::checkCUDAError("cudaMalloc");

	cudaMemset(d_y_real, 0, qreg_max_stn*sizeof(double));
	qSim_qcpu_device::checkCUDAError("cudaMemset");
	cudaMemset(d_y_img, 0, qreg_max_stn*sizeof(double));
	qSim_qcpu_device::checkCUDAError("cudaMemset");
	cudaDeviceSynchronize();	
#endif
}

qSim_qcpu_device::~qSim_qcpu_device() {
	// release function CUDA vectors
	cudaFree(d_ftype_cuda_vec);
	cudaFree(d_fsize_cuda_vec);
	cudaFree(d_fargs_cuda_vec);
	qSim_qcpu_device::checkCUDAError("cudaFree");

#ifdef __CUDA_DYNPAR__
	// DP case specific part
	cudaFree(d_y_real);
	cudaFree(d_y_img);
	qSim_qcpu_device::checkCUDAError("cudaFree");
#endif

	// release function host vectors
	free(m_ftype_vec);
	free(m_fsize_vec);
	free(m_fargs_vec);
}

// ---------------------------------------------------------
// instructions execution - qureg transformations
// ---------------------------------------------------------

// => 1-qubit gate functions
int qSim_qcpu_device::dev_qreg_apply_function_gate_1qubit(QDEV_ST_VAL_TYPE*d_x, QDEV_ST_VAL_TYPE*d_y, int d_N,
														 QASM_F_TYPE ftype, int frep, int flsq,
														 QREG_F_ARGS_TYPE* fargs, bool verbose) {
	// handle 1-qubit gate application to given qureg data
	if (verbose) {
		printf("applying 1-qubit gate function...\n");
		printf("d_N: %d - ftype: %d - frep: %d - flsq: %d - fargs size: %lu\n",
				d_N, ftype, frep, flsq, fargs->size());
	}

	// convert fargs to CUDA device pointer array
	QDEV_F_ARGS_TYPE dev_fargs = qSim_qcpu_device::fargs_to_dev_ptr_array(*fargs);
	if (verbose)
		printf("dev_fargs...argc: %d - argv: %g\n", dev_fargs.argc, dev_fargs.argv);

	// perform function aggregation for gap filling w.r.t overall qureg size
	int fn = 1; // fixed for 1-qubit gate
	int fsize = 2; // fixed for 1-qubit gate
	int tot_f = f_dev_gap_filling(d_N, ftype, fsize, frep, flsq, dev_fargs,
								   m_ftype_vec, m_fsize_vec, m_fargs_vec, verbose);
	if (tot_f < 1) {
		printf("dev_qreg_apply_function_gate_1qubit: 0 functions returned by gap filling - error!!\n");
		return QDEV_RES_ERROR; // return error
	}
	int max_block_size = powf(2, fn*frep + flsq);
	int block_inner_gap_size = powf(2, flsq);
	if (verbose) {
		printf("dev_qreg_apply_function_gate_1qubit: gap filling tot_f: %d  max_block_size: %d  block_inner_gap_size: %d\n",
				tot_f, max_block_size, block_inner_gap_size);
	}

	// store function type vectors into CUDA device memory objects for use in kernels
	dev_vec_host2device((void**)&d_ftype_cuda_vec, m_ftype_vec, tot_f, sizeof(QASM_F_TYPE));
	dev_vec_host2device((void**)&d_fsize_cuda_vec, m_fsize_vec, tot_f, sizeof(int));
	if (verbose)
		printf("ftype & fsize prepared...\n");

	// convert function args vector to CUDA memory objects
	dev_args_host2device(&d_fargs_cuda_vec, m_fargs_vec, tot_f);
	if (verbose) {
		printf("fargs prepared...\n");
//		for (int jj=0; jj<tot_f; jj++) {
//			printf("#%d - d_fargs_cuda: (%d %f)\n", jj, d_fargs_cuda_vec[jj].argc, d_fargs_cuda_vec[jj].argv);
//		}
	}

	// perform kernel function on N elements
	int nblocks = (d_N+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;
	int nthreads = MIN(d_N, THREADS_PER_BLOCK);
	if (verbose)
		printf("nblocks: %d  nthreads: %d\n\n", nblocks, nthreads);

#ifdef __CUDA_DYNPAR__
	// dynamic parallelism mode
	if (verbose)
		printf("calling kernel...DP\n\n");
	kernel_prod_fxi_dp<<<nblocks, nthreads>>>(d_x, d_y, d_N, d_ftype_cuda_vec, d_fsize_cuda_vec,
			                                  d_fargs_cuda_vec, tot_f, max_block_size, block_inner_gap_size,
											  ftype, fn, 0, 0, 0, QASM_F_TYPE_NULL, 0, // form n.a. here
											  d_y_real, d_y_img);
	qSim_qcpu_device::checkCUDAError("kernel_prod_fxi_dp");

#else
	// single kernel mode
	if (verbose)
		printf("calling kernel...SK\n\n");
	kernel_prod_fxi_sk<<<nblocks, nthreads>>>(d_x, d_y, d_N, d_ftype_cuda_vec, d_fsize_cuda_vec,
											  d_fargs_cuda_vec, tot_f, max_block_size, block_inner_gap_size,
											  ftype, fn, 0, 0, 0, QASM_F_TYPE_NULL, 0); // form n.a. here
	qSim_qcpu_device::checkCUDAError("kernel_prod_fxi_sk");

#endif

	// wait for all kernel instances to complete
	cudaDeviceSynchronize();

	if (verbose)
		printf("qreg_apply_function done\n");

	return QDEV_RES_OK;
}

// --------------------------------

// => 2-qubit gate functions
int qSim_qcpu_device::dev_qreg_apply_function_gate_2qubit(QDEV_ST_VAL_TYPE*d_x, QDEV_ST_VAL_TYPE*d_y, int d_N,
														 QASM_F_TYPE ftype, int frep, int flsq, int fform, int futype,
														 QREG_F_ARGS_TYPE* fuargs, bool verbose) {
	// handle 2-qubit gate application to given qureg data
	if (verbose) {
		printf("applying 2-qubit gate function...\n");
		printf("d_N: %d - ftype: %d - frep: %d - flsq: %d - fform: %d - futype: %d - fargs size: %lu\n",
				d_N, ftype, frep, flsq, fform, futype, fuargs->size());
	}

	// convert fargs to CUDA device pointer array
	QDEV_F_ARGS_TYPE dev_fuargs = qSim_qcpu_device::fargs_to_dev_ptr_array(*fuargs);
	if (verbose)
		printf("dev_fuargs...argc: %d - argv: %g\n", dev_fuargs.argc, dev_fuargs.argv);

	// perform function aggregation for gap filling w.r.t overall qureg size
	int fn = 2; // fixed for 1-qubit gate
	int fsize = 4; // fixed for 1-qubit gate
	int fuform = 0; // fixed for 2-qubit case
	int tot_f = f_dev_gap_filling(d_N, ftype, fsize, frep, flsq, dev_fuargs,
								   m_ftype_vec, m_fsize_vec, m_fargs_vec, verbose);
	if (tot_f < 1) {
		printf("dev_qreg_apply_function_gate_1qubit: 0 functions returned by gap filling - error!!");
		return QDEV_RES_ERROR; // return error
	}
	int max_block_size = powf(2, fn*frep + flsq);
	int block_inner_gap_size = powf(2, flsq);
	if (verbose) {
		printf("dev_qreg_apply_function_gate_1qubit: gap filling tot_f: %d  max_block_size: %d  block_inner_gap_size: %d\n",
				tot_f, max_block_size, block_inner_gap_size);
	}

	// store function type vectors into CUDA device memory objects for use in kernels
	dev_vec_host2device((void**)&d_ftype_cuda_vec, m_ftype_vec, tot_f, sizeof(QASM_F_TYPE));
	dev_vec_host2device((void**)&d_fsize_cuda_vec, m_fsize_vec, tot_f, sizeof(int));
	if (verbose)
		printf("ftype & fsize prepared...\n");

	// convert function args vector to CUDA memory objects
	dev_args_host2device(&d_fargs_cuda_vec, m_fargs_vec, tot_f);
	if (verbose) {
		printf("fargs prepared...\n");
		// for (int jj=0; jj<tot_f; jj++) {
			// printf("#%d - d_fargs_cuda: (%d %f)\n", jj, d_fargs_cuda_vec[jj].argc, d_fargs_cuda_vec[jj].argv);
		// }
	}

	// perform kernel function on N elements
	int nblocks = (d_N+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;
	int nthreads = MIN(d_N, THREADS_PER_BLOCK);
	if (verbose)
		printf("nblocks: %d  nthreads: %d\n\n", nblocks, nthreads);

#ifdef __CUDA_DYNPAR__
	// dynamic parallelism mode
	if (verbose)
		printf("calling kernel...DP\n\n");
	kernel_prod_fxi_dp<<<nblocks, nthreads>>>(d_x, d_y, d_N, d_ftype_cuda_vec, d_fsize_cuda_vec,
			                                  d_fargs_cuda_vec, tot_f, max_block_size, block_inner_gap_size,
											  ftype, fn, fform, 0, futype, 1, fuform,
											  d_y_real, d_y_img);
	qSim_qcpu_device::checkCUDAError("kernel_prod_fxi_dp");

#else
	// single kernel mode
	if (verbose)
		printf("calling kernel...SK\n\n");
	kernel_prod_fxi_sk<<<nblocks, nthreads>>>(d_x, d_y, d_N, d_ftype_cuda_vec, d_fsize_cuda_vec,
											  d_fargs_cuda_vec, tot_f, max_block_size, block_inner_gap_size,
											  ftype, fn, fform, 0, futype, 1, fuform);
	qSim_qcpu_device::checkCUDAError("kernel_prod_fxi_sk");

#endif

	// wait for all kernel instances to complete
	cudaDeviceSynchronize();

	if (verbose)
		printf("qreg_apply_function done\n");

	return QDEV_RES_OK;
}

// --------------------------------

// => n-qubit gate functions
int qSim_qcpu_device::dev_qreg_apply_function_controlled_gate_nqubit(QDEV_ST_VAL_TYPE*d_x, QDEV_ST_VAL_TYPE*d_y, int d_N,
																	 QREG_F_TYPE ftype, int fsize, int frep, int flsq, int fform,
																	 int fgapn, int futype, int fun, int fuform,
																	 QREG_F_ARGS_TYPE* fuargs, bool verbose) {
	// handle 2-qubit gate application to given qureg data
	if (verbose) {
		printf("applying n-qubit gate function...\n");
		printf("d_N: %d - ftype: %d - fsize: %d - frep: %d - flsq: %d - fform: %d - futype: %d - fargs size: %lu\n",
				d_N, ftype, fsize, frep, flsq, fform, futype, fuargs->size());
	}

	// convert fargs to CUDA pointer array
	QDEV_F_ARGS_TYPE dev_fargs = qSim_qcpu_device::fargs_to_dev_ptr_array(*fuargs);

	// perform function aggregation for gap filling w.r.t overall qureg size
	int fn = log2(fsize); // function size in qubits
	int tot_f = f_dev_gap_filling(d_N, ftype, fsize, frep, flsq, dev_fargs,
								   m_ftype_vec, m_fsize_vec, m_fargs_vec, verbose);
	if (tot_f < 1) {
		printf("dev_qreg_apply_function_gate_1qubit: 0 functions returned by gap filling - error!!");
		return QDEV_RES_ERROR; // return error
	}
	int max_block_size = powf(2, fn*frep + flsq);
	int block_inner_gap_size = powf(2, flsq);
	if (verbose) {
		printf("dev_qreg_apply_function_gate_1qubit: gap filling tot_f: %d  max_block_size: %d  block_inner_gap_size: %d\n",
				tot_f, max_block_size, block_inner_gap_size);
	}

	// store function type vectors into CUDA device memory objects for use in kernels
	dev_vec_host2device((void**)&d_ftype_cuda_vec, m_ftype_vec, tot_f, sizeof(QASM_F_TYPE));
	dev_vec_host2device((void**)&d_fsize_cuda_vec, m_fsize_vec, tot_f, sizeof(int));
	if (verbose)
		printf("ftype & fsize prepared...\n");

	// convert function args vector to CUDA memory objects
	dev_args_host2device(&d_fargs_cuda_vec, m_fargs_vec, tot_f);
	if (verbose) {
		printf("fargs prepared...\n");
		// for (int jj=0; jj<tot_f; jj++) {
			// printf("#%d - d_fargs_cuda: (%d %f)\n", jj, d_fargs_cuda_vec[jj].argc, d_fargs_cuda_vec[jj].argv);
		// }
	}

	// perform kernel function on N elements
	int nblocks = (d_N+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;
	int nthreads = MIN(d_N, THREADS_PER_BLOCK);
	if (verbose)
		printf("nblocks: %d  nthreads: %d\n\n", nblocks, nthreads);

#ifdef __CUDA_DYNPAR__
	// dynamic parallelism mode
	if (verbose)
		printf("calling kernel...DP\n\n");
	kernel_prod_fxi_dp<<<nblocks, nthreads>>>(d_x, d_y, d_N, d_ftype_cuda_vec, d_fsize_cuda_vec,
			                                  d_fargs_cuda_vec, tot_f, max_block_size, block_inner_gap_size,
											  ftype, fn, fform, fgapn, futype, fun, fuform,
											  d_y_real, d_y_img);
	qSim_qcpu_device::checkCUDAError("kernel_prod_fxi_dp");

#else
	// single kernel mode
	if (verbose)
		printf("calling kernel...SK\n\n");
	kernel_prod_fxi_sk<<<nblocks, nthreads>>>(d_x, d_y, d_N, d_ftype_cuda_vec, d_fsize_cuda_vec,
											  d_fargs_cuda_vec, tot_f, max_block_size, block_inner_gap_size,
											  ftype, fn, fform, fgapn, futype, fun, fuform);
	qSim_qcpu_device::checkCUDAError("kernel_prod_fxi_sk");

#endif

	// wait for all kernel instances to complete
	cudaDeviceSynchronize();

	if (verbose)
		printf("qreg_apply_function done\n");

	return QDEV_RES_OK;
}

// ---------------------------------------------------------
// instructions execution - qureg state handling
// ---------------------------------------------------------

// => qureg state setup - kernel function
__global__
void kernel_set_state(QDEV_ST_VAL_TYPE *x, int N, int st_val) {
	int idx = blockIdx.x * blockDim.x + threadIdx.x; // 1D vector: only x-dimension used
//	printf("N: %d  idx: %d\n", rN, idx);
	if (idx < N)
		if (idx != st_val)
			x[idx] = QDEV_ST_MAKE_VAL(0.0, 0.0);
		else
			x[idx] = QDEV_ST_MAKE_VAL(1.0, 0.0);
}

// --------------------------------

// => qureg state value set
void qSim_qcpu_device::dev_qreg_set_state(QDEV_ST_VAL_TYPE*d_x, int N, int st_val, bool verbose) {
	// perform kernel function on N elements
	int nblocks = (N+THREADS_PER_BLOCK-1)/THREADS_PER_BLOCK;
	int nthreads = MIN(N, THREADS_PER_BLOCK);
	if (verbose) {
		printf("CUDA - qreg_set_state...st_val: %d\n", st_val);
		printf("nblocks: %d  nthreads: %d\n\n", nblocks, nthreads);
	}

	// call CUDA kernel functions
	kernel_set_state<<<nblocks, nthreads>>>(d_x, N, st_val);
	qSim_qcpu_device::checkCUDAError("kernel_set_state");

	// wait for all kernel instances to complete
	cudaDeviceSynchronize();
}

// ---------------------------------------------------------
// static helper host <--> device conversion methods
// ---------------------------------------------------------

void qSim_qcpu_device::dev_qreg_host2device(QDEV_ST_VAL_TYPE** d_x, QDEV_ST_VAL_TYPE* x, int N) {
	// allocate and setup device memory with given host one
	cudaMalloc((void**)d_x, N*sizeof(QDEV_ST_VAL_TYPE));
	checkCUDAError("cudaMalloc");
	cudaMemcpy((*d_x), x, N*sizeof(QDEV_ST_VAL_TYPE), cudaMemcpyHostToDevice);
	checkCUDAError("cudaMemcpy");
//	cudaDeviceSynchronize();	-> not needed after a cudaMemcpy (it is synchronous)
}

void qSim_qcpu_device::dev_qreg_device2host(QDEV_ST_VAL_TYPE* x, QDEV_ST_VAL_TYPE* d_x, int N) {
	// allocate and setup device memory with given host one
	cudaMemcpy(x, d_x, N*sizeof(QDEV_ST_VAL_TYPE), cudaMemcpyDeviceToHost);
	checkCUDAError("cudaMemcpy");
//	cudaDeviceSynchronize();	-> not needed after a cudaMemcpy (it is synchronous)
}

void qSim_qcpu_device::dev_qreg_host2device_align(QDEV_ST_VAL_TYPE* d_x, QDEV_ST_VAL_TYPE* x, int N) {
	// device memory alignment with given host one - no allocation
	cudaMemcpy(d_x, x, N*sizeof(QDEV_ST_VAL_TYPE), cudaMemcpyHostToDevice);
	checkCUDAError("cudaMemcpy");
//	cudaDeviceSynchronize();	-> not needed after a cudaMemcpy (it is synchronous)
}

void qSim_qcpu_device::dev_vec_host2device(void** d_x, void* x, int n, int size) {
	// allocate and setup unified memory with given one
	cudaMemcpy((*d_x), x, n*size, cudaMemcpyHostToDevice);
	checkCUDAError("cudaMemcpy");
//	cudaDeviceSynchronize();	-> not needed after a cudaMemcpy (it is synchronous)
}

void qSim_qcpu_device::dev_args_host2device(QDEV_F_ARGS_TYPE** d_fargs, QDEV_F_ARGS_TYPE* fargs, int tot_f) {
	// allocate and setup unified memory with given one - iterating over function arguments
	cudaMemcpy((*d_fargs), fargs, tot_f*sizeof(QDEV_F_ARGS_TYPE), cudaMemcpyHostToDevice);
	checkCUDAError("cudaMemcpy");
//	cudaDeviceSynchronize();	-> not needed after a cudaMemcpy (it is synchronous)
}

// helper function for device memory release
void qSim_qcpu_device::dev_qreg_device_release(QDEV_ST_VAL_TYPE* d_x) {
	cudaFree(d_x);
	checkCUDAError("cudaFree");
}

// helper function for kernel error codes check
void qSim_qcpu_device::checkCUDAError(const char* cmd_msg) {
    cudaError_t err = cudaGetLastError();
    if (cudaSuccess != err) {
    	fprintf(stderr, "CUDA error executing %s: err: %d  msg: %s.\n",
    			cmd_msg, err, cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
}

// ---------------------------------

// QASM function arguments conversion into CUDA device structure
QDEV_F_ARGS_TYPE qSim_qcpu_device::fargs_to_dev_ptr_array(QREG_F_ARGS_TYPE fargs) {
	// convert from QASM to CUDA device formats
	QDEV_F_ARGS_TYPE cuda_fargs;
	if (fargs.size() > 0) {
		// some argument found
		cuda_fargs.argc = fargs.size();
		for (unsigned int i=0; i<fargs.size(); i++) {
			if (fargs[i].m_type == qSim_qreg_function_arg::INT) {
				cuda_fargs.argv = (double)fargs[i].m_i;
			}
			else {
				cuda_fargs.argv = fargs[i].m_d;
			}
		}
	}
	else {
		// no arguments
		cuda_fargs.argc = 0;
		cuda_fargs.argv = 0.0;
	}
	return cuda_fargs;
}

// ---------------------------------------------------------
// CUDA device inspection methods
// ---------------------------------------------------------

// => CUDA device count
int qSim_qcpu_device::dev_get_gpu_cuda_count() {
	int nDevices = 0;
#ifndef _WIN32
	// linux case
	cudaGetDeviceCount(&nDevices);
	checkCUDAError("cudaGetDeviceCount");
#else
	// windows case
	printf("dev_get_gpu_cuda_count non supported by Windows GPU!\n\n");
	nDevices = 1;
#endif
	return nDevices;
}

// => CUDA device info
void qSim_qcpu_device::dev_gpu_cuda_properties_dump() {
#ifndef _WIN32
	// linux case
	int nDevices = dev_get_gpu_cuda_count();
	for (int i = 0; i < nDevices; i++) {
		cudaDeviceProp prop;
		cudaGetDeviceProperties(&prop, i);
		checkCUDAError("cudaGetDeviceCount");

		printf("GPU CUDA Device Number: %d\n", i);
		printf("  Device name: %s\n", prop.name);
		printf("  Total Global Memory (GB): %4.3f\n", prop.totalGlobalMem/1e9);
		printf("  Memory Clock Rate (KHz): %d\n", prop.memoryClockRate);
		printf("  Memory Bus Width (bits): %d\n", prop.memoryBusWidth);
		printf("  Peak Memory Bandwidth (GB/s): %f\n",
			   2.0*prop.memoryClockRate*(prop.memoryBusWidth/8)/1.0e6);
		printf("\n");
	}
	printf("--------------------------------\n\n");
#else
// windows case
	printf("dev_gpu_cuda_properties_dump non supported by Windows GPU!\n\n");
#endif
}

// --------------------------------

