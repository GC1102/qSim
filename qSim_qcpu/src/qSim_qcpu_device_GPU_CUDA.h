/*
 * qSim_qcpu_device_GPU_CUDA.h
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
 * - basic transformation functions (I, X, H, CX and their combination)
 * - extended transformation functions (SWAP, Toffoli, Fredkin, QFT)
 * - custom transformation functions (look-up-table based)
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation.
 *  1.1   Nov-2022   Instruction set limitation to 1 and 2 qubit gates.
 *                   Transformed to class.
 *                   Defined and handled a function arguments structure.
 *                   Module renamed to qSim_qcup_device_GPU_CUDA.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QCPU_DEVICE_GPU_CUDA_H_
#define QSIM_QCPU_DEVICE_GPU_CUDA_H_

#include "cuComplex.h"

#include "qSim_qasm.h"
#include "qSim_qinstruction_core.h"


// data type for a q-state value as complex
typedef cuDoubleComplex QDEV_ST_VAL_TYPE;
#define QDEV_ST_MAKE_VAL make_cuDoubleComplex

// return codes
#define QDEV_RES_OK    0
#define QDEV_RES_ERROR -1

// data type for qreg state value and array
typedef QDEV_ST_VAL_TYPE QREG_ST_RAW_VAL_TYPE; // same type as in the GPU

// --------------

// data type for CUDA arguments pointer array
struct qSim_qreg_function_args_cuda {
	uint argc;
	double argv;
};
typedef qSim_qreg_function_args_cuda QDEV_F_ARGS_TYPE;

// ---------------------------------------------

class qSim_qcpu_device {
public:
	// constructor & destructor
	qSim_qcpu_device();
	~qSim_qcpu_device();

	// instructions execution

	// - 1-qubit gate functions
	int dev_qreg_apply_function_gate_1qubit(QDEV_ST_VAL_TYPE*d_x, QDEV_ST_VAL_TYPE*d_y, int d_N,
			                                 QREG_F_TYPE ftype, int frep, int flsq,
											 QREG_F_ARGS_TYPE* fargs, bool verbose);

	// - 2-qubit gate functions
	int dev_qreg_apply_function_gate_2qubit(QDEV_ST_VAL_TYPE*d_x, QDEV_ST_VAL_TYPE*d_y, int d_N,
											QREG_F_TYPE ftype, int frep, int flsq, int fform, int futype,
											QREG_F_ARGS_TYPE* fuargs, bool verbose);

	// - n-qubit gate functions
	int dev_qreg_apply_function_controlled_gate_nqubit(QDEV_ST_VAL_TYPE*d_x, QDEV_ST_VAL_TYPE*d_y, int d_N,
													   QREG_F_TYPE ftype, int fsize, int frep, int flsq, int fform, int fgapn,
													   int futype, int fun, int fuform, QREG_F_ARGS_TYPE* fuargs, bool verbose);

	// qureg state value set
	void dev_qreg_set_state(QDEV_ST_VAL_TYPE*d_x, int d_N, int st_val, bool verbose);

	// static helper host <--> device conversion methods
	static void dev_qreg_host2device(QDEV_ST_VAL_TYPE**, QDEV_ST_VAL_TYPE* x, int d_N);
	static void dev_qreg_device2host(QDEV_ST_VAL_TYPE* x, QDEV_ST_VAL_TYPE* d_x, int d_N);
	static void dev_qreg_host2device_align(QDEV_ST_VAL_TYPE* d_x, QDEV_ST_VAL_TYPE* x, int d_N);
	static void dev_qreg_device_release(QDEV_ST_VAL_TYPE* d_x);

	static void checkCUDAError(const char* cmd_msg);

	static void dev_vec_host2device(void** d_x, void* x, int n, int size);
	static void dev_args_host2device(QDEV_F_ARGS_TYPE** d_fargs, QDEV_F_ARGS_TYPE* fargs, int tot_f);

	// function args to CUDA device pointer array conversions
	static QDEV_F_ARGS_TYPE fargs_to_dev_ptr_array(QREG_F_ARGS_TYPE fargs);

	// CUDA device info diagnostics
	int dev_get_gpu_cuda_count();
	void dev_gpu_cuda_properties_dump();

private:
	// function type, size and arg host vectors
    // allocate function host vectors
	QASM_F_TYPE* m_ftype_vec; 		// overall function type sequence to use
	int* m_fsize_vec;			 	// overall function sizes, as per type sequence
	QDEV_F_ARGS_TYPE* m_fargs_vec;	// overall function arguments, as per type sequence

	// function type, size and arg CUDA vectors
	QASM_F_TYPE* d_ftype_cuda_vec;
	int* d_fsize_cuda_vec;
	QDEV_F_ARGS_TYPE* d_fargs_cuda_vec;

#ifdef __CUDA_DYNPAR__
	// DP specific CUDA vectors for transformation result accumulation
	double* d_y_real;
	double* d_y_img;
#endif
};

#endif /* QSIM_QCPU_DEVICE_GPU_CUDA_H_ */

