/*
 * qSim_qcpu_device_CPU.h
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


#ifndef QSIM_QCPU_DEVICE_CPU_H_
#define QSIM_QCPU_DEVICE_CPU_H_

#include <cstdint>
#include <complex>

#include "qSim_qasm.h"
#include "qSim_qinstruction_core.h"


// data type for a q-state value as complex
typedef complex<double> QDEV_ST_VAL_TYPE;
#define QDEV_ST_MAKE_VAL complex<double>

// return codes
#define QDEV_RES_OK     0
#define QDEV_RES_ERROR -1

// data type for qreg state value and array
typedef QDEV_ST_VAL_TYPE QREG_ST_RAW_VAL_TYPE;

// --------------

// data type for device arguments pointer array
struct qSim_qreg_function_args_device {
	int argc;
	double argv;
};
typedef qSim_qreg_function_args_device QDEV_F_ARGS_TYPE;

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

//	static void dev_vec_host2device(void** d_x, void* x, int n, int size);
//	static void dev_args_host2device(QDEV_F_ARGS_TYPE** d_fargs, QDEV_F_ARGS_TYPE* fargs, int tot_f);

	// function args to device pointer array conversions
	static QDEV_F_ARGS_TYPE fargs_to_dev_ptr_array(QREG_F_ARGS_TYPE fargs);

//	// device info diagnostics
//	int cuda_get_device_count();
//	void cuda_properties_dump();

private:
	// function type, size and arg host vectors
    // allocate function host vectors
	QASM_F_TYPE* m_ftype_vec; 		// overall function type sequence to use
	int* m_fsize_vec;			 	// overall function sizes, as per type sequence
	QDEV_F_ARGS_TYPE* m_fargs_vec;	// overall function arguments, as per type sequence

	// function type, size and arg CPU vectors
	QASM_F_TYPE* d_ftype_dev_vec;
	int* d_fsize_dev_vec;
	QDEV_F_ARGS_TYPE* d_fargs_dev_vec;
};

#endif /* QSIM_QCPU_DEVICE_CPU_H_ */

