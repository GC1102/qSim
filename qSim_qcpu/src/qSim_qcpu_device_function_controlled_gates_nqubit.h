
/*
 * qSim_qcpu_device_function_controlled_gates_nqubit.h
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
 *  Created on: Nov 20, 2022
 *      Author: gianni
 *
 * Q-CPU support module implementing device supported transformation functions:
 * - controlled n-qubit gates (long range and multi-control)
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Nov-2022   Module creation.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QCPU_DEVICE_FUNCTION_CONTROLLED_GATES_NQUBIT_H_
#define QSIM_QCPU_DEVICE_FUNCTION_CONTROLLED_GATES_NQUBIT_H_

#include <math.h>

#ifdef __QSIM_CPU__
#include "qSim_qcpu_device_CPU.h"
#define __device__ /* dummy */
#else
#include "qSim_qcpu_device_GPU_CUDA.h"
#endif

#include "qSim_qcpu_device_function_gates_1qubit.h"


// ################################################################
// n-qubit gates - handling functions and relevant static pointers
// ################################################################

// note: for 2-qubits controlled functions the function args include as first arguments
// - fargs[0]: fu_crng
// - fargs[1]: fu_trng

#define F_QDEV_SELECT_EXEC(futype, fui, fuj, fuform, fargs, f_val) {\
	if (QASM_F_TYPE_IS_GATE_1QUBIT(futype)) {\
		FunctionCallback f_dev_k;\
		f_dev_k = QDEV_F_GATE_1Q_SELECTOR((QASM_F_TYPE)futype);\
		f_val = f_dev_k(fui, fuj, fargs);\
/*		printf("...1q-f-dev f_val: (%g %g)\n", f_val.x, f_val.y); */\
	}\
	else if (QASM_F_TYPE_IS_GATE_2QUBIT((QASM_F_TYPE)futype)) {\
		FunctionCallback_2q f_dev_k;\
		f_dev_k = QDEV_F_GATE_2Q_SELECTOR((QASM_F_TYPE)futype);\
		f_val = f_dev_k(fui, fuj, fuform, 0, fargs);\
/*		printf("...2q-f-dev f_val: (%g %g)\n", f_val.x, f_val.y);*/\
	}\
}

class qSim_qcpu_device_function_controlled_gates_nqubit {
public:
	// constructor & destructor
	qSim_qcpu_device_function_controlled_gates_nqubit();
	~qSim_qcpu_device_function_controlled_gates_nqubit();

	// helpers

	// ----------------------------------------------
	// DIRECT case

	__device__
	static bool is_in_type_u_block_direct(int fdbi, int fdbj, int tot_blocks, int tot_u_blocks) {
		// check if diagonal block i,j pair is within a type-U function block
		// based on the control/gaps generated  block pattern
		//
		// -> type-U function blocks are the last in the sequence
		return ((fdbi >= tot_blocks - tot_u_blocks) && (fdbj >= tot_blocks - tot_u_blocks) && (fdbi == fdbj));
	}

	__device__
	static bool is_in_type_1_block_direct(int i, int j, int fdbi, int fdbj, int tot_blocks, int tot_u_blocks) {
		// check if block i,j if a type-1 block
		// based on the control/gaps generated  block pattern
		//
		// -> type-1 function blocks are the first in the sequence
		return ((fdbi < tot_blocks - tot_u_blocks) && (fdbj < tot_blocks - tot_u_blocks) && (i == j));
	}

	// ----------------------------------------------
	// INVERSE case

	__device__
	static bool is_in_type_u_block_inverse(int fubi, int fubj, int fugbsize) {
		// check if diagonal block i,j pair is within a type-U function block
		// based on the control/gaps generated  block pattern
		//
		// -> type-U function blocks are along all block diagonals on odd positions within blocks
		//    with replicas as per total U-blocks

        return ((fubi%fugbsize == fugbsize-1) && (fubj%fugbsize == fugbsize-1) && (fubi == fubj));
	}

	__device__
	static bool is_in_type_1_block_inverse(int i, int j, int f1bsize) {
		// check if block i,j if a type-1 block
		// based on the control/gaps generated  block pattern
		//
		// -> type-1 function pairs are along main diagonal on even positions
		//    or on odd positions in first 2^gap blocks for gates > 1-qubit

        return ((i%f1bsize < f1bsize-1) && (j%f1bsize < f1bsize-1) && (i == j));
	}

	// ----------------------------------------------
	// transformation methods
	// ----------------------------------------------

	// => Qn - Multi-Controlled Short/Long Range U function
	__device__
	static QDEV_ST_VAL_TYPE f_dev_qn_mcu_slr(int i, int j, int fn, int fform, int gapn,
			                                 int futype, int fun, int fuform, QDEV_F_ARGS_TYPE* fuargs) {
		// apply controlled form on given U-gate handling gaps and multi-controls
		//
		// => <direct> form
		// - controls: qn..q1 / i0,j0 ... ik,jk (MSQ)
		// - U function: q0 / in/jn (LSQ)
		//
		// => <inverse> form
		// - U function: qn / i0,j0 (MSQ)
		// - control: q0..qk / ik,jk ... in/jn (LSQ)

		int fsize = powf(2, fn);
		int fusize = powf(2, fun);
		int ctrln = fn - fun - gapn;
//		printf("f_dev_qn_mcu_slr...i: %d  j: %d  fn: %d  fsize: %d  fbsize: %d  gapn: %d\n",
//				i, j, fn, fsize, fbsize, gapn);

		int fdbsize = fusize; // diagonal block size

		// calculate value based on given i,j indeces
		QDEV_ST_VAL_TYPE f_val;
		if (fform == QASM_F_FORM_DIRECT) {
			// *** direct form ***
			/*
			 * => controls = 1, gaps = 1 	 => controls = 1, gaps = 0
			 *
			 *     (-----------)  			 	 (-----------)
			 *     | I 0 | 0 0 |				 | I 0 | 0 0 |
			 *     | 0 I | 0 0 | 				 | 0 I | 0 0 |
			 * U = ------------- 			 U = -------------
			 *     | 0 0 | U 0 | 				 | 0 0 | I 0 |
			 *     | 0 0 | 0 U | 				 | 0 0 | 0 U |
			 *     (-----------)			     (-----------)
			 */

			int tot_blocks = powf(2, ctrln+gapn);
			int tot_u_blocks = powf(2, gapn);

			// select diagonal block for given i,j pair
			int fdbi = i / fdbsize;
			int fdbj = j / fdbsize;
//			printf("direct f_dev_qn_mcu_slr...i: %d  j: %d  fdbsize: %d  fdbi: %d  fdbj: %d  tot_blocks: %d  tot_u_blocks: %d\n",
//					i, j, fdbsize, fdbi, fdbj, tot_blocks, tot_u_blocks);

			// check if block i,j pair is within a type-U function block
			// based on the control/gaps generated  block pattern
			if (is_in_type_u_block_direct(fdbi, fdbj, tot_blocks, tot_u_blocks)) {
				// block found - apply gate
				int fui = i % fdbsize;
				int fuj = j % fdbsize;
//				printf("...applying 1Q gate i: %d  j: %d  futype: %d  fusize: %d  fdbsize: %d  fui: %d  fuj: %d\n",
//						i, j, futype, fusize, fdbsize, fui, fuj);
				F_QDEV_SELECT_EXEC(futype, fui, fuj, fuform, fuargs, f_val)
			}

			// check if block i,j if a type-1 block
			// based on the control/gaps generated  block pattern
			else if (is_in_type_1_block_direct(i, j, fdbi, fdbj, tot_blocks, tot_u_blocks)) {
//				printf("...setting 1.0 i: %d  j: %d\n", i, j);
				f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
			}

			// last case - it is a type-0 block
			else {
				f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
			}
		}
		else {
			// *** inverse form ***
			/*
			 * => controls = 1, gaps = 1 	 							=> controls = 1, gaps = 0
			 *
			 *     (---------------------------------------) 			 	 (-----------------)
			 *     | P0+P1U00          | P0+P1U01   0      |				 | 1   0  | 0   0  |
			 *     |   0      P0+P1U00 |   0      P1U01    | 				 | 0  U00 | 0  U01 |
			 * U = ----------------------------------------- 			 U = -------------------
			 *     |  P1U10     0      | P0+P1U11   0      | 				 | 0   0  | 1   0  |
			 *     |   0      P1U01    |   0      P0+P1U11 | 				 | 0  U10 | 0  U11 |
			 *     (---------------------------------------)			     (-----------------)
			 */

			// select diagonal block for given i,j pair
			int fubsize = fsize / fusize;
			int f1bsize = powf(2, ctrln);
			int fubi = i % fubsize;
			int fubj = j % fubsize;
			int fugbsize = fubsize / powf(2, gapn);
//			printf("inverse f_dev_qn_mcu_slr...i: %d  j: %d  ctrln: %d  gapn: %d  fubsize: %d  f1bsize: %d  fugbsize: %d  fubi: %d  fubj: %d\n",
//					i, j, ctrln, gapn, fubsize, f1bsize, fugbsize, fubi, fubj);

			// check if block i,j pair is within a type-U function block
			// based on the control/gaps generated  block pattern
			if (is_in_type_u_block_inverse(fubi, fubj, fugbsize)) {
				// block found - apply gate
				int fui = i / fubsize ;
				int fuj = j / fubsize;
//				printf("...applying u-gate i: %d  j: %d  futype: %d  fusize: %d  fqbsize: %d  fui: %d  fuj: %d\n",
//						i, j, futype, fusize, fqbsize, fui, fuj);
				F_QDEV_SELECT_EXEC(futype, fui, fuj, fuform, fuargs, f_val)
//				printf("...f_val: (%g %g)\n", f_val.x, f_val.y);
			}

			// check if block i,j if a type-1 block
			// based on the control/gaps generated  block pattern
			else if (is_in_type_1_block_inverse(i, j, f1bsize)) {
//				printf("...setting 1.0 i: %d  j: %d  f1bsize: %d\n",
//						i, j, f1bsize);
				f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);
			}

			// last case - it is a type-0 block
			else {
				f_val = QDEV_ST_MAKE_VAL(0.0, 0.0);
			}
		}
		// return result
		return f_val;
	}

	// -----------------------------------

	// => Q3 - CCX function (Toffoli)
	__device__
	static QDEV_ST_VAL_TYPE f_dev_q3_ccx(int i, int j, int /*fn*/, int fform, int /*gapn*/,
                                         int /*futype*/, int /*fun*/, int fuform, QDEV_F_ARGS_TYPE*) {
		// apply multi-controlled form on given 1-qubit X-gate
		// calling controlled-U function (CX same function form as CCX!!)
		QDEV_ST_VAL_TYPE f_val = f_dev_qn_mcu_slr(i, j, 3, fform, 0, QASM_F_TYPE_Q1_X, 1, fuform, NULL);

		// return result
		return f_val;
	}

};

// ################################################################
// Function pointers definition
// ################################################################

// function type
typedef QDEV_ST_VAL_TYPE (*FunctionCallback_nq)(int, int, int, int, int, int, int, int, QDEV_F_ARGS_TYPE*);

// ---------------------------------------------

// Static array of pointers to functions - same order as for QASM function type values!!
#define QASM_F_TYPE_GATE_NQ_TOT_ENTRIES (QASM_F_TYPE_QN_MCSLRU - QASM_F_TYPE_Q3_CCX + 1)

__device__
FunctionCallback_nq pf_device_controlled_gates_nqubit_vec[] = {
	// n-qubit
	qSim_qcpu_device_function_controlled_gates_nqubit::f_dev_qn_mcu_slr,
};

__device__
FunctionCallback_nq get_functionRefByFtype_controlled_gates_nqubit(QASM_F_TYPE ftype) {
	if ((ftype < QASM_F_TYPE_QN_MCSLRU) | (ftype > QASM_F_TYPE_Q3_CCX)) {
		printf("!!!get_functionRefByFtype_controlled_gates_nqubit ERROR - device function type %d out of allowed range [%d ,%d]!!!\n",
				ftype, QASM_F_TYPE_QN_MCSLRU, QASM_F_TYPE_Q3_CCX);
		return NULL;
	}
	FunctionCallback_nq f_ptr = pf_device_controlled_gates_nqubit_vec[ftype-QASM_F_TYPE_QN_MCSLRU];
	if (f_ptr == NULL)
		printf("!!!get_functionRefByFtype_controlled_gates_nqubit ERROR - device function type %d not defined in function pointer vector!!!", ftype);
	return f_ptr;
}

#define QDEV_F_GATE_NQ_SELECTOR get_functionRefByFtype_controlled_gates_nqubit


#endif /* QSIM_QCPU_DEVICE_FUNCTION_CONTROLLED_GATES_NQUBIT_H_ */

