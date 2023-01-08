
/*
 * qSim_qcpu_device_function_gates_2qubit.h
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
 *  Created on: Nov 15, 2022
 *      Author: gianni
 *
 * Q-CPU support module implementing device supported transformation functions:
 * - basic 2-qubit gates (CX, CY, CZ and generic C-U)
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Nov-2022   Module creation.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QCPU_DEVICE_FUNCTION_GATES_2QUBIT_H_
#define QSIM_QCPU_DEVICE_FUNCTION_GATES_2QUBIT_H_

#include <math.h>

#ifdef __QSIM_CPU__
#include "qSim_qcpu_device_CPU.h"
#define __device__ /* dummy */
#else
#include "qSim_qcpu_device_GPU_CUDA.h"
#endif

#include "qSim_qcpu_device_function_gates_1qubit.h"


// ################################################################
// 2-qubit gates - handling functions and relevant static pointers
// ################################################################

class qSim_qcpu_device_function_gates_2qubit {
public:
	// constructor & destructor
	qSim_qcpu_device_function_gates_2qubit();
	~qSim_qcpu_device_function_gates_2qubit();

	// ----------------------------------------------
	// transformation methods
	// ----------------------------------------------

	// => Q2 - CU function (generic controlled-U function)
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q2_cu(int i, int j, int fform, int fu_type, QDEV_F_ARGS_TYPE* fu_args) {
		// apply controlled form on given 1-qubit U-gate
		//
		// => <direct> form
		// - control: q1 / i0,j0 (MSQ)
		// - U function: q0 / i1/j1 (LSQ)
		//
		// => <inverse> form
		// - U function: q1 / i0/j0 (MSQ)
		// - control: q0 / i1,j1 (LSQ)

		// calculate value based on given i,j indeces
		QDEV_ST_VAL_TYPE f_val;
		if (fform == QASM_F_FORM_DIRECT) {
			// direct form
			if ((i > 1) && (j > 1)) {
				// apply gate
				FunctionCallback fu_dev_k = QDEV_F_GATE_1Q_SELECTOR((QASM_F_TYPE)fu_type);
				f_val = fu_dev_k(i%2, j%2, fu_args);
			}
			else if ((i == j) && (i < 2)) {
				// printf("f_dev_q2_cu...direct...1.0...i: %d  j:%d \n", i, j);
				f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
			}
			else {
				// printf("f_dev_q2_cu...direct...0.0...i: %d  j:%d \n", i, j);
				f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
			}
		}
		else {
			// inverse form
			if ((i%2 == 1) && (j%2 == 1) && ((i == j) || (i == j+2) || (j == i+2))) {
				// apply gate
				FunctionCallback fu_dev_k = QDEV_F_GATE_1Q_SELECTOR((QASM_F_TYPE)fu_type);
				f_val = fu_dev_k(i/2, j/2, fu_args);
			}
			else if ((i == j) && ((j == 0) || (j == 2)))
				f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
			else
				f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
		}
		// return result
		return f_val;
	}

	// -----------------------------------

	// => Q2 - CX function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q2_cx(int i, int j, int fform, int, QDEV_F_ARGS_TYPE*) {
		// apply controlled form on given 1-qubit X-gate
		// calling controlled-U function
		QDEV_ST_VAL_TYPE f_val = f_dev_q2_cu(i, j, fform, QASM_F_TYPE_Q1_X, NULL);

		// return result
		return f_val;
	}

	// -----------------------------------

	// => Q2 - CY function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q2_cy(int i, int j, int fform, int, QDEV_F_ARGS_TYPE*) {
		// apply controlled form on given 1-qubit Y-gate
		// calling controlled-U function
		QDEV_ST_VAL_TYPE f_val = f_dev_q2_cu(i, j, fform, QASM_F_TYPE_Q1_Y, NULL);

		// return result
		return f_val;
	}

	// -----------------------------------

	// => Q2 - CZ function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q2_cz(int i, int j, int fform, int, QDEV_F_ARGS_TYPE*) {
		// apply controlled form on given 1-qubit Z-gate
		// calling controlled-U function
		QDEV_ST_VAL_TYPE f_val = f_dev_q2_cu(i, j, fform, QASM_F_TYPE_Q1_Z, NULL);

		// return result
		return f_val;
	}

};

// ################################################################
// Function pointers definition
// ################################################################

// function type
typedef QDEV_ST_VAL_TYPE (*FunctionCallback_2q)(int, int, int, int, QDEV_F_ARGS_TYPE*);

// ---------------------------------------------

// Static array of pointers to functions - same order as for QASM function type values!!
#define QASM_F_TYPE_GATE_2Q_TOT_ENTRIES (QASM_F_TYPE_Q2_CZ - QASM_F_TYPE_Q2_CU + 1)

__device__
FunctionCallback_2q pf_device_gates_2qubit_vec[] = {
	// 2-qubit
		qSim_qcpu_device_function_gates_2qubit::f_dev_q2_cu,
		qSim_qcpu_device_function_gates_2qubit::f_dev_q2_cx,
		qSim_qcpu_device_function_gates_2qubit::f_dev_q2_cy,
		qSim_qcpu_device_function_gates_2qubit::f_dev_q2_cz,
};

__device__
FunctionCallback_2q get_functionRefByFtype_gates_2qubit(QASM_F_TYPE ftype) {
	if ((ftype < QASM_F_TYPE_Q2_CU) | (ftype > QASM_F_TYPE_Q2_CZ)) {
		printf("!!!get_functionRefByFtype_gates_2qubit ERROR - device function type %d out of allowed range [%d, %d]!!!\n",
				ftype, QASM_F_TYPE_Q2_CU, QASM_F_TYPE_Q2_CZ);
		return NULL;
	}
	FunctionCallback_2q f_ptr = pf_device_gates_2qubit_vec[ftype-QASM_F_TYPE_Q2_CU];
	if (f_ptr == NULL)
		printf("!!!get_functionRefByFtype_gates_2qubit ERROR - device function type %d not defined in function pointer vector!!!", ftype);
	return f_ptr;
}

#define QDEV_F_GATE_2Q_SELECTOR get_functionRefByFtype_gates_2qubit


#endif /* QSIM_QCPU_DEVICE_FUNCTION_GATES_2QUBIT_H_ */

