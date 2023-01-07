
/*
 * qSim_qcpu_device_function_gates_1qubit.h
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
 *  Created on: May 9, 2022
 *      Author: gianni
 *
 * Q-CPU support module implementing device supported transformation functions:
 * - basic 1-qubit gates (I, X, H, Pauli, etc.)
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation.
 *  1.1   Nov-2022   Renamed to qSim_qcpu_device_function_gates_1qubit and
 *                   scope limited to 1-qubit gates.
 *                   Transformed to class and supported both GPU CUDA and CPU devices.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QCPU_DEVICE_FUNCTION_GATES_1QUBIT_H_
#define QSIM_QCPU_DEVICE_FUNCTION_GATES_1QUBIT_H_

#include <math.h>

#ifdef __QSIM_CPU__
#include "qSim_qcpu_device_CPU.h"
#ifdef __WIN32
#define __device__ /* dummy */
#define M_PI 2*acos(0.0) // !!!! not working directly from <cmat> in Windows!!!
#endif
#else
#include "qSim_qcpu_device_GPU_CUDA.h"
#endif


// ################################################################
// 1-qubit gates - handling functions and relevant static pointers
// ################################################################

class qSim_qcpu_device_function_gates_1qubit {
public:
	// constructor & destructor
	qSim_qcpu_device_function_gates_1qubit();
	~qSim_qcpu_device_function_gates_1qubit();

	// ----------------------------------------------
	// transformation methods
	// ----------------------------------------------

	// => Q1 - identity function --> Note: working also for I (nxn) cases and then used for gap fillers !!!!
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_i(int i, int j, QDEV_F_ARGS_TYPE* /*f_args[]*/) {
		QDEV_ST_VAL_TYPE f_val;
		if (i==j)
			f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
		else
			f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
//		printf("f_dev_q1_i...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

	// -------------------------------------

	// => Q1 - Hadamard function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_h(int i, int j, QDEV_F_ARGS_TYPE* /*f_args[]*/) {
		QDEV_ST_VAL_TYPE f_val;
		if (j<1)
			f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
		else
			f_val = QDEV_ST_MAKE_VAL(powf(-1.0, i), 0.0);
#ifdef __QSIM_CPU__
		f_val /= QDEV_ST_MAKE_VAL(sqrtf(2.0), 0.0);
//		printf("f_dev_q1_h...%d %d  -> %f %f\n", i, j, f_val.real(), f_val.imag());
#else
		f_val = cuCdiv(f_val, QDEV_ST_MAKE_VAL(sqrtf(2.0), 0.0));
//		printf("f_dev_q1_h...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
#endif
		return f_val;
	}

	// ------------------------------------

	// => Q1 - X function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_x(int i, int j, QDEV_F_ARGS_TYPE* /*f_args[]*/) {
		QDEV_ST_VAL_TYPE f_val;
		if (j==i)
			f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
		else
			f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
	//	printf("f_dev_q1_x...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

	// ------------------------------------

	// => Q1 - Y function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_y(int i, int j, QDEV_F_ARGS_TYPE* /*f_args[]*/) {
		QDEV_ST_VAL_TYPE f_val;
		if (j==i)
			f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
		else
			f_val = QDEV_ST_MAKE_VAL(0.0, powf(-1.0, i+1));
	//	printf("f_dev_q1_y...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

	// ------------------------------------

	// > Q1 - Z function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_z(int i, int j, QDEV_F_ARGS_TYPE* /*f_args[]*/) {
		QDEV_ST_VAL_TYPE f_val;
		if (j==i)
			f_val = QDEV_ST_MAKE_VAL(powf(-1.0, i), 0.0);
		else
			f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
	//	printf("f_dev_q1_z...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

	// ------------------------------------

	// => Q1 - SX function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_sx(int i, int j, QDEV_F_ARGS_TYPE* /*f_args[]*/) {
		QDEV_ST_VAL_TYPE f_val;
		if (j==i)
			f_val = QDEV_ST_MAKE_VAL(1.0, 1.0);
		else
			f_val = QDEV_ST_MAKE_VAL(1.0, -1.0);
#ifdef __QSIM_CPU__
//		printf("f_val before: %g  %g\n", f_val.real(), f_val.imag());
		f_val /= QDEV_ST_MAKE_VAL(2.0, 0.0);
//		printf("f_val after: %g  %g\n", f_val.real(), f_val.imag());
//		printf("f_val: %g  %g    sqrt(2): %g   %g %g \n", f_val.real(), f_val.imag(), sqrtf(2.0), sqrt2.real(), sqrt2.imag());
#else
		f_val = cuCdiv(f_val, QDEV_ST_MAKE_VAL(2.0, 0.0));
		//	printf("f_dev_q1_sx...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
#endif
		return f_val;
	}

	// ------------------------------------

	// => Q1 - PS function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_ps(int i, int j, QDEV_F_ARGS_TYPE* f_args) {
		// f_args[0] : phi

		QDEV_ST_VAL_TYPE f_val;
		double phi = 0.0;
		if (f_args->argc > 0)
			phi = f_args->argv;
		else
			printf("ERROR - f_dev_q1_ps - missing phi argument!! - 0.0 used");
//		printf("f_dev_q1_ps - i: %d  j: %d  phi: %g\n", i, j, phi);

		if (i == j)
			if (i == 0)
				f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
			else
				f_val = QDEV_ST_MAKE_VAL(cos(phi), sin(phi));
		else
			f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
//		printf("f_dev_q1_ps...%d %d %f -> %f %f\n", i, j, phi, f_val.x, f_val.y);
		return f_val;
	}

	// ------------------------------------

	// => Q1 - S function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_s(int i, int j, QDEV_F_ARGS_TYPE* /*f_args*/) {
		// apply Q1 PS function with PI/2 argument
		QDEV_ST_VAL_TYPE f_val;
		double phi = M_PI/2.0;
//		printf("S...phi: %g\n", phi);

		if (j==i)
			if (i==0)
				f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
			else
				f_val = QDEV_ST_MAKE_VAL(cos(phi), sin(phi));
		else
			f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
	//	printf("f_dev_q1_s...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

	// ------------------------------------

	// => Q1 - T function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_t(int i, int j, QDEV_F_ARGS_TYPE* /*f_args[]*/) {
		// apply Q1 PS function with PI/4 argument
		QDEV_ST_VAL_TYPE f_val;
		double phi = M_PI/4.0;
	//	printf("S tot_args: %d   phi: %g\n", tot_args, phi);

		if (j==i)
			if (i==0)
				f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
			else
				f_val = QDEV_ST_MAKE_VAL(cos(phi), sin(phi));
		else
			f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
	//	printf("f_dev_q1_s...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

	// ------------------------------------

	// => Q1 - Rx function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_rx(int i, int j, QDEV_F_ARGS_TYPE* f_args) {
		// f_args[0] : phi

		QDEV_ST_VAL_TYPE f_val;
		double phi = 0.0;
		if (f_args->argc > 0)
			phi = f_args->argv;
		else
			printf("ERROR - f_dev_q1_rx - missing phi argument!! - 0.0 used");

		 //    printf("Rx tot_args: %d   phi: %g\n", tot_args, phi);
		if (j==i)
			f_val = QDEV_ST_MAKE_VAL(cos(phi/2), 0.0);
		else
			f_val = QDEV_ST_MAKE_VAL(0.0, -sin(phi/2));
	//	printf("f_dev_q1_ps...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

	// ------------------------------------

	// => Q1 - Ry function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_ry(int i, int j, QDEV_F_ARGS_TYPE* f_args) {
		QDEV_ST_VAL_TYPE f_val;
		double phi = 0.0;
		if (f_args->argc > 0)
			phi = f_args->argv;
		else
			printf("ERROR - f_dev_q1_ry - missing phi argument!! - 0.0 used");

 //    printf("Ry tot_args: %d   phi: %g\n", tot_args, phi);
		if (j==i)
			f_val = QDEV_ST_MAKE_VAL(cos(phi/2), 0.0);
		else
			f_val = QDEV_ST_MAKE_VAL(powf(-1, i+1)*sin(phi/2), 0.0);
	//	printf("f_dev_q1_ry...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

	// ------------------------------------

	// => Q1 - Rz function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q1_rz(int i, int j, QDEV_F_ARGS_TYPE* f_args) {
		QDEV_ST_VAL_TYPE f_val;
		double phi = 0.0;
		if (f_args->argc > 0)
			phi = f_args->argv;
		else
			printf("ERROR - f_dev_q1_rz - missing phi argument!! - 0.0 used");

		//	printf("Rz tot_args: %d   phi: %g\n", tot_args, phi);
		if (j==i)
			f_val = QDEV_ST_MAKE_VAL((double)cos(powf(-1.0, i+1)*phi/2.0), (double)sin(powf(-1.0, i+1)*phi/2.0));
		else
			f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
	//	printf("f_dev_q1_rz...%d %d  -> %f %f\n", i, j, f_val.x, f_val.y);
		return f_val;
	}

};

// ################################################################
// Function pointers definition
// ################################################################

// function type
typedef QDEV_ST_VAL_TYPE (*FunctionCallback)(int, int, QDEV_F_ARGS_TYPE*);

// ---------------------------------------------

// Static array of pointers to functions - same order as for QASM function type values!!
#define QASM_F_TYPE_GATE_1Q_TOT_ENTRIES 12

__device__
FunctionCallback pf_device_gates_1qubit_vec[] = {
	// 1-qubit
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_i,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_h,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_x,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_y,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_z,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_sx,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_ps,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_t,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_s,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_rx,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_ry,
	qSim_qcpu_device_function_gates_1qubit::f_dev_q1_rz
};

__device__
FunctionCallback get_functionRefByFtype_gates_1qubit(QASM_F_TYPE ftype) {
	if ((ftype < QASM_F_TYPE_Q1_I) || (ftype > QASM_F_TYPE_Q1_Rz)) {
		printf("!!!get_functionRefByFtype_gates_1qubit ERROR - device function type %d out of allowed range [%d, %d]!!!\n",
				ftype, QASM_F_TYPE_Q1_I, QASM_F_TYPE_Q1_Rz);
		return NULL;
	}
	FunctionCallback f_ptr = pf_device_gates_1qubit_vec[ftype-QASM_F_TYPE_Q1_I];
	if (f_ptr == NULL)
		printf("!!!get_functionRefByFtype_gates_1qubit ERROR - device function type %d not defined in function pointer vector!!!", ftype);
	return f_ptr;
}

#define QDEV_F_GATE_1Q_SELECTOR get_functionRefByFtype_gates_1qubit


#endif /* QSIM_QCPU_DEVICE_FUNCTION_GATES_1QUBIT_H_ */

