/*
 * qSim_qcpu_device_CPU.cpp
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
 *  Created on: Dec 19, 2022
 *      Author: gianni
 *
 * Q-CPU support module, providing functions for CPU device based transformations handling,
 * applying a function based approach, and supporting:
 * - basic transformation functions (I, X, H, CX and their combination)
 * - extended transformation functions (SWAP, Toffoli, Fredkin, QFT)
 * - custom transformation functions (look-up-table based)
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Dec-2022   Module creation adapting qSim_qcup_GPU_CUDA and agnostic
 *                   class renaming to qSim_qcpu_device.
 *
 *  --------------------------------------------------------------------------
 */


#include <qSim_qcpu_device_CPU.h>
#include <qSim_qcpu_device_function_exec.h>


// --------------------------------
// kernel entry point functions
// --------------------------------

// --------------------------------
// sequential processing case

void sequential_prod_fxi(QDEV_ST_VAL_TYPE *x, QDEV_ST_VAL_TYPE *y, int idx, int N,
						QASM_F_TYPE* d_ftype_cuda_vec, int* d_fn_cuda_vec,
						QDEV_F_ARGS_TYPE* d_fargs_cuda_vec, int tot_f, int max_block_size, int block_inner_gap_size,
						int /*ftype*/, int fn, int fform, int gapn, int futype, int fun, int fuform) {
	// single kernel case
//	printf("sequential_prod_fxi...idx: %d\n", idx);

	// combine all i-th row with x elements for y i-th result
	if (idx < N) {
		y[idx] = QDEV_ST_MAKE_VAL(0.0, 0.0);

		// define current calculation limits considering LSQ & MSQ gap fillers generated zeroes
	    int k_step = block_inner_gap_size;
		int k_start = max(0, (idx/max_block_size)*max_block_size);
		int k_stop = min(N-1, k_start+max_block_size-1);
//		printf("fxi_sk...idx: %d  N: %d  k_step: %d  k_start: %d  k_stop: %d\n", idx, N, k_step, k_start, k_stop);

//		for (int k=0; k<N; k++) {
		for (int k=k_start; k<k_stop+1; k++) {
			// check for calculation limits considering LSQ gap fillers generated zeroes
			if (k % k_step == idx % k_step) {
//				printf("...found idx: %d  k: %d", idx, k);
				y[idx] += x[k] * f_dev_qn_exec(idx, k, d_ftype_cuda_vec, d_fn_cuda_vec,
										   	   fn, fform, gapn, futype, fun, fuform, d_fargs_cuda_vec, tot_f);
			}
//			printf("\n");
		}
//		printf("fxi_sk...%d -> %f %f\n", idx, y[idx].x, y[idx].y);
	}
}


// --------------------------------------------------------
// class methods
// --------------------------------------------------------

#define CPU_QREG_MAX_N 20 			// max qureg size supported
#define CPU_TOT_F CPU_QREG_MAX_N  // bounded by max qureg size

// constructor & destructor
qSim_qcpu_device::qSim_qcpu_device() {
    // allocate function host vectors
    m_ftype_vec = (QASM_F_TYPE*)malloc(CPU_TOT_F*sizeof(QASM_F_TYPE));
    m_fsize_vec = (int*)malloc(CPU_TOT_F*sizeof(int));
    m_fargs_vec = (QDEV_F_ARGS_TYPE*)malloc(CPU_TOT_F*sizeof(QDEV_F_ARGS_TYPE));

    // device vectors direct link to host vectors
    d_ftype_dev_vec = m_ftype_vec;
    d_fsize_dev_vec = m_fsize_vec;
    d_fargs_dev_vec = m_fargs_vec;
}

qSim_qcpu_device::~qSim_qcpu_device() {
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
		printf("d_N: %d - ftype: %d - frep: %d - flsq: %d - fargs size: %u\n",
				d_N, ftype, frep, flsq, (unsigned)fargs->size());
		printf("fargs: ");
		for (unsigned i=0; i<fargs->size(); i++)
			printf("%s, ", (*fargs)[i].to_string().c_str());
		printf("\n");
	}

	// convert fargs to device pointer array
	QDEV_F_ARGS_TYPE dev_fargs = qSim_qcpu_device::fargs_to_dev_ptr_array(*fargs);
	if (verbose)
		printf("dev_fargs...argc: %d - argv: %g\n", dev_fargs.argc, dev_fargs.argv);

	// perform function aggregation for gap filling w.r.t overall qureg size
	int fn = 1; // fixed for 1-qubit gate
	int fsize = 2; // fixed for 1-qubit gate
	int tot_f = f_dev_gap_filling(d_N, ftype, fsize, frep, flsq, dev_fargs,
								  m_ftype_vec, m_fsize_vec, m_fargs_vec, verbose);
	if (tot_f < 1) {
		printf("cpu_qreg_apply_function_gate_1qubit: 0 functions returned by gap filling - error!!\n");
		return QDEV_RES_ERROR; // return error
	}
	int max_block_size = powf(2, fn*frep + flsq);
	int block_inner_gap_size = powf(2, flsq);
	if (verbose) {
		printf("cuda_qreg_apply_function_gate_1qubit: gap filling tot_f: %d  max_block_size: %d  block_inner_gap_size: %d\n",
				tot_f, max_block_size, block_inner_gap_size);
	}

	// perform kernel function on N elements
	if (verbose)
		printf("calling kernel...SK\n\n");
	for (int idx=0; idx<d_N; idx++) {
		sequential_prod_fxi(d_x, d_y, idx, d_N, d_ftype_dev_vec, d_fsize_dev_vec,
							d_fargs_dev_vec, tot_f, max_block_size, block_inner_gap_size,
							ftype, fn, 0, 0, 0, QASM_F_TYPE_NULL, 0); // form n.a. here
	}

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
		printf("d_N: %d - ftype: %d - frep: %d - flsq: %d - fform: %d - futype: %d - fuargs size: %u\n",
				d_N, ftype, frep, flsq, fform, futype, (unsigned)fuargs->size());
		printf("fuargs: ");
		for (unsigned i=0; i<fuargs->size(); i++)
			printf("%s, ", (*fuargs)[i].to_string().c_str());
		printf("\n");
	}

	// convert fargs to device pointer array
	QDEV_F_ARGS_TYPE dev_fuargs = qSim_qcpu_device::fargs_to_dev_ptr_array(*fuargs);
	if (verbose)
		printf("dev_fuargs...argc: %d - argv: %g\n", dev_fuargs.argc, dev_fuargs.argv);

	// perform function aggregation for gap filling w.r.t overall qureg size
	int fn = 2; // fixed for 2-qubit case
	int fsize = 4; // fixed for 2-qubit gcaseate
	int fuform = 0; // fixed for 2-qubit case
	int tot_f = f_dev_gap_filling(d_N, ftype, fsize, frep, flsq, dev_fuargs,
								  m_ftype_vec, m_fsize_vec, m_fargs_vec, verbose);
	if (tot_f < 1) {
		printf("cuda_qreg_apply_function_gate_1qubit: 0 functions returned by gap filling - error!!");
		return QDEV_RES_ERROR; // return error
	}
	int max_block_size = powf(2, fn*frep + flsq);
	int block_inner_gap_size = powf(2, flsq);
	if (verbose) {
		printf("cuda_qreg_apply_function_gate_1qubit: gap filling tot_f: %d  max_block_size: %d  block_inner_gap_size: %d\n",
				tot_f, max_block_size, block_inner_gap_size);
	}

	// perform kernel function on N elements
	if (verbose)
		printf("calling kernel...SK\n\n");
	for (int idx=0; idx<d_N; idx++) {
		sequential_prod_fxi(d_x, d_y, idx, d_N, d_ftype_dev_vec, d_fsize_dev_vec,
							d_fargs_dev_vec, tot_f, max_block_size, block_inner_gap_size,
							ftype, fn, fform, 0, futype, 1, fuform);
	}

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
		printf("d_N: %d - ftype: %d - fsize: %d - frep: %d - flsq: %d - fform: %d - fgapn: %d - futype: %d - fun: %d - fuform: %d - fuargs size: %u\n",
				d_N, ftype, fsize, frep, flsq, fform, fgapn, futype, fun, fuform, (unsigned)fuargs->size());
	}

	// convert fargs to device pointer array
	QDEV_F_ARGS_TYPE dev_fuargs = qSim_qcpu_device::fargs_to_dev_ptr_array(*fuargs);

	// perform function aggregation for gap filling w.r.t overall qureg size
	int fn = log2(fsize); // function size in qubits
	int tot_f = f_dev_gap_filling(d_N, ftype, fsize, frep, flsq, dev_fuargs,
								  m_ftype_vec, m_fsize_vec, m_fargs_vec, verbose);
	if (tot_f < 1) {
		printf("cuda_qreg_apply_function_gate_1qubit: 0 functions returned by gap filling - error!!");
		return QDEV_RES_ERROR; // return error
	}
	int max_block_size = powf(2, fn*frep + flsq);
	int block_inner_gap_size = powf(2, flsq);
	if (verbose) {
		printf("cuda_qreg_apply_function_gate_1qubit: gap filling tot_f: %d  max_block_size: %d  block_inner_gap_size: %d\n",
				tot_f, max_block_size, block_inner_gap_size);
	}

	// perform kernel function on N elements
	if (verbose)
		printf("calling kernel...SK\n\n");
	for (int idx=0; idx<d_N; idx++) {
		sequential_prod_fxi(d_x, d_y, idx, d_N, d_ftype_dev_vec, d_fsize_dev_vec,
							d_fargs_dev_vec, tot_f, max_block_size, block_inner_gap_size,
							ftype, fn, fform, fgapn, futype, fun, fuform);
	}

	if (verbose)
		printf("qreg_apply_function done\n");

	return QDEV_RES_OK;
}

// ---------------------------------------------------------
// instructions execution - qureg state handling
// ---------------------------------------------------------

// => qureg state setup - kernel function
void sequential_kernel_set_state(QDEV_ST_VAL_TYPE *x, int idx, int N, int st_val) {
	//	1D vector: only x-dimension used
	if (idx < N) {
		if (idx != st_val)
			x[idx] = QDEV_ST_MAKE_VAL(0.0, 0.0);
		else
			x[idx] = QDEV_ST_MAKE_VAL(1.0, 0.0);
	}
}

// --------------------------------

// => qureg state value set (any pure state)
void qSim_qcpu_device::dev_qreg_set_state(QDEV_ST_VAL_TYPE*d_x, int N, int st_val, bool verbose) {
	// sequential kernel function call
	for (int idx=0; idx<N; idx++) {
		sequential_kernel_set_state(d_x, idx, N, st_val);
	}
}

// ---------------------------------------------------------
// static helper host <--> device conversion methods
// ---------------------------------------------------------

void qSim_qcpu_device::dev_qreg_host2device(QDEV_ST_VAL_TYPE** d_x, QDEV_ST_VAL_TYPE* x, int N) {
	// allocate and setup device memory from given host one
	*d_x = (QDEV_ST_VAL_TYPE*)malloc(N*sizeof(QDEV_ST_VAL_TYPE));
	memcpy((*d_x), x, N*sizeof(QDEV_ST_VAL_TYPE));
}

void qSim_qcpu_device::dev_qreg_device2host(QDEV_ST_VAL_TYPE* x, QDEV_ST_VAL_TYPE* d_x, int N) {
	// allocate and setup device memory with given host one
	memcpy(x, d_x, N*sizeof(QDEV_ST_VAL_TYPE));
}

void qSim_qcpu_device::dev_qreg_host2device_align(QDEV_ST_VAL_TYPE* d_x, QDEV_ST_VAL_TYPE* x, int N) {
	// device memory alignment with given host one - no allocation
	memcpy(d_x, x, N*sizeof(QDEV_ST_VAL_TYPE));
}

//void qSim_qcpu_device::dev_vec_host2device(void** d_x, void* x, int n, int size) {
//	// device memory alignment with given host one - no allocation - vector case
//	memcpy(d_x, x, n*size);
////	cudaMemcpy((*d_x), x, n*size, cudaMemcpyHostToDevice);
////	checkCUDAError("cudaMemcpy");
////	cudaDeviceSynchronize();	-> not needed after a cudaMemcpy (it is synchronous)
//}
//
//void qSim_qcpu_device::dev_args_host2device(QDEV_F_ARGS_TYPE** d_fargs, QDEV_F_ARGS_TYPE* fargs, int tot_f) {
//	// device memory alignment with given host one - no allocation - function arguments case
//	memcpy(d_fargs, fargs, tot_f*sizeof(QDEV_F_ARGS_TYPE));
////	cudaMemcpy((*d_fargs), fargs, tot_f*sizeof(QDEV_F_ARGS_TYPE), cudaMemcpyHostToDevice);
////	checkCUDAError("cudaMemcpy");
////	cudaDeviceSynchronize();	-> not needed after a cudaMemcpy (it is synchronous)
//}

// helper function for device memory release
void qSim_qcpu_device::dev_qreg_device_release(QDEV_ST_VAL_TYPE* d_x) {
	free(d_x);
}

// ---------------------------------

// QASM function arguments conversion into device structure
QDEV_F_ARGS_TYPE qSim_qcpu_device::fargs_to_dev_ptr_array(QREG_F_ARGS_TYPE fargs) {
	// convert from QASM to device format
	QDEV_F_ARGS_TYPE dev_fargs;
	if (fargs.size() > 0) {
		// some argument found
		dev_fargs.argc = fargs.size();
		for (unsigned int i=0; i<fargs.size(); i++) {
			if (fargs[i].m_type == qSim_qreg_function_arg::INT) {
				dev_fargs.argv = (double)fargs[i].m_i;
			}
			else {
				dev_fargs.argv = fargs[i].m_d;
			}
		}
	}
	else {
		// no arguments
		dev_fargs.argc = 0;
		dev_fargs.argv = 0.0;
	}
	return dev_fargs;
}

// --------------------------------

