
/*
 * qSim_qcpu_GPU_CUDA_function_exec.h
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
 *  Created on: Jun 13, 2022
 *      Author: gianni
 *
 * Q-CPU support module implementing entry point functions for CUDA kernels
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Jun-2022   Module creation.
 *  1.1   Nov-2022   Instruction set limitation to 1 and 2 qubit gates.
 *                   Updated function arguments passage.
 *                   Module renamed to qSim_qcpu_GPU_CUDA_function_exec.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QCPU_DEVICE_FUNCTION_EXEC_H_
#define QSIM_QCPU_DEVICE_FUNCTION_EXEC_H_

#include <math.h>

#ifdef __QSIM_CPU__
#include "qSim_qcpu_device_CPU.h"
#define __device__ /* dummy */
#else
#include "qSim_qcpu_device_GPU_CUDA.h"
#endif

#include "qSim_qcpu_device_function_gates_1qubit.h"
#include "qSim_qcpu_device_function_gates_2qubit.h"
#include "qSim_qcpu_device_function_controlled_gates_nqubit.h"


// ############################################################

// ----------------------------------------------
// helper functions
// ----------------------------------------------

// => function LSQ & MSQ gap filling
int f_dev_gap_filling(int qsize, QASM_F_TYPE ftype, int fsize, int frep, int flsq, QDEV_F_ARGS_TYPE fargs,
					  QASM_F_TYPE* ftype_vec, int* fsize_vec, QDEV_F_ARGS_TYPE* fargs_vec,
					  bool verbose) {
	// perform function aggregation for gap filling w.r.t overall qureg size
	// => IN
	// - qsize: qureg size (tot states)
	// - ftype: function to apply
	// - fsize: function size (tot states)
	// - frep: # of function repetitions to apply
	// - flsq: function LSQ index
	// - fargs: function arguments structure
	// => OUT
	// - ftype_vec: array of function types to apply for gap filling
	// - fsize_vec: array of function sizes corresponding to function types
	// - fargs_vec: array of function arguments corresponding to function types

	// calculate supporting qubits & states info
	int qn = log2(qsize);
	int fn = log2(fsize);
	int fmsq = flsq + fn*frep - 1;
	if (verbose)
		printf("fvec... qsize: %d qn: %d fsize: %d fn: %d - frep: %d - flsq: %d - fmsq: %d\n",
				qsize, qn, fsize, fn, frep, flsq, fmsq);

	// make some sanity checks on params
	if (powf(2, fmsq) > qsize) {
		fprintf(stderr, "ERROR!! - f_dev_gap_filling - too many function repetitions - limit [%d] exceeded!!!\n", qsize);
		return 0;
	}

	if (fsize > qsize) {
		fprintf(stderr, "ERROR!! - f_dev_gap_filling - function size [%d] cannot be larger than qureg one [%d]!!!\n", fsize, qsize);
		return 0;
	}

	// setup vectors - MSQ part first!
	int tot_f = 0; // counter for actual length
	if (fmsq < qn-1) {
		// gap on MSQ part - add I (nxn) gate as filler
//		printf("device - f_dev_gap_filling...adding I on MSQ part\n");
		ftype_vec[tot_f] = QASM_F_TYPE_Q1_I;
		fsize_vec[tot_f] = pow(2, (qn-fmsq-1));
		fargs_vec[tot_f++] = QDEV_F_ARGS_TYPE(); // no args for I gate
	}
	
	// provided function part - add requested repetitions
//	printf("device - f_dev_gap_filling...adding ftype\n");
	for (int i=0; i<frep; i++) {
		ftype_vec[tot_f] = ftype;
		fsize_vec[tot_f] = fsize;
		fargs_vec[tot_f++] = fargs;
	}
	
	if (flsq > 0) {
		// gap on LSQ part - add I (nxn) gate as filler
//		printf("device - f_dev_gap_filling...adding I on LSQ part\n");
		ftype_vec[tot_f] = QASM_F_TYPE_Q1_I;
		fsize_vec[tot_f] = pow(2, flsq);
		fargs_vec[tot_f++] = QDEV_F_ARGS_TYPE(); // no args for I gate
	}

//	if (verbose) {
//		printf("device - f_dev_gap_filling...\n");
//		printf("tot_f: %d\n", tot_f);
//		for (int i=0; i<tot_f; i++) {
//			printf("#%d - ftype_vec: %d  fsize_vec: %d  fargs_vec: (%d %f)\n",
//					i, ftype_vec[i], (*fsize_vec)[i], (*fargs_vec)[i].argc, (*fargs_vec)[i].argv);
//		}

	// return total number of functions prepared
	return tot_f;
}


// --------------------------------------------

#define QDEV_F_VAL_EPS 1e-21

// gap-filled function items combining as tensor-product - n-qubits
__device__
QDEV_ST_VAL_TYPE f_dev_qn_exec(int i, int j, QASM_F_TYPE* ftype_dev_vec, int* fsize_dev_vec,
						 	   int fn, int fform, int gapn, int futype, int fun, int fuform,
							   QDEV_F_ARGS_TYPE* fargs_dev_vec, int tot_f) {
	// perform overall transformation function application to current state element (i, j)
	// by iteration over all function elements to apply and handling function form (direct/inverse)
	// and function class (general/controlled)
	//
	// => IN
	// - i, j: state element iteration indeces (i: state index, j: moving cursor for value-i calculation)
	// - ftype_dev_vec: function types to apply - device vector
	// - fsize_dev_vec: function sizes (tot states) - device vector
	// - fn: function width in qubits
	// - fform: function form
	// - gapn: gap width in qubits
	// - futype: U-gate function type
	// - fun: U-gate function width in qubits
	// - fargs_cuda_vec: function args - device vector
	// - tot_f: # of function elements to apply
	//
	// => OUT
	// - f_val: state value at position i for j contribution

	// *** N-QUBIT GATE CASE ***
	//	printf("f_cuda_qn_exec...\n");
//	printf("f_dev_qn_exec...fn: %d  fform: %d  gapn: %d  futype: %d...\n", fn, fform, gapn, futype);

	// setup result accumulator
	QDEV_ST_VAL_TYPE f_val = QDEV_ST_MAKE_VAL(1.0, 0.0);

	// setup working indexes to starting position
	int ik = i;
	int jk = j;

	// loop through function elements
	for (int k=0; k<tot_f; k++) {
		// access elements from last i.e. starting from LSQ
		int f_idx = tot_f - k - 1;
		int fN_k = fsize_dev_vec[f_idx];

		if (QASM_F_TYPE_IS_GATE_1QUBIT(ftype_dev_vec[f_idx])) {
			// 1-qubit gate case
//			printf("1-qubit case...f_idx: %d -> ftype: %d\n", f_idx, ftype_dev_vec[f_idx]);

			// multiply for i-th function block value
			FunctionCallback f_dev_k;
			f_dev_k = QDEV_F_GATE_1Q_SELECTOR((QASM_F_TYPE)ftype_dev_vec[f_idx]);
#ifdef __QSIM_CPU__
			f_val *= f_dev_k(ik%fN_k, jk%fN_k, &(fargs_dev_vec[f_idx]));
//			QDEV_ST_VAL_TYPE f_val_k = f_dev_k(ik%fN_k, jk%fN_k, &(fargs_dev_vec[f_idx]));
//			printf("f_val before 1q: %g  %g  f_val_k: %g  %g\n", f_val.real(), f_val.imag(), f_val_k.real(), f_val_k.imag());
//			f_val *= f_val_k;
//			printf("f_val after 1q: %g  %g\n", f_val.real(), f_val.imag());
#else
			f_val = cuCmul(f_val, f_dev_k(ik%fN_k, jk%fN_k, &(fargs_dev_vec[f_idx])));
#endif
		}
		else if (QASM_F_TYPE_IS_GATE_2QUBIT(ftype_dev_vec[f_idx])) {
			// 2-qubit gate case
//			printf("2-qubit case...f_idx: %d -> ftype: %d\n", f_idx, ftype_dev_vec[f_idx]);

			// multiply for i-th function block value
			FunctionCallback_2q f_dev_k;
			f_dev_k = QDEV_F_GATE_2Q_SELECTOR((QASM_F_TYPE)ftype_dev_vec[f_idx]);
#ifdef __QSIM_CPU__
			f_val *= f_dev_k(ik%fN_k, jk%fN_k, fform, futype, &(fargs_dev_vec[f_idx]));
//			printf("f_val after 2q: %g  %g\n", f_val.real(), f_val.imag());
#else
			f_val = cuCmul(f_val, f_dev_k(ik%fN_k, jk%fN_k, fform, futype, &(fargs_dev_vec[f_idx])));
#endif
		}
		else if (QASM_F_TYPE_IS_GATE_NQUBIT(ftype_dev_vec[f_idx])) {
			// n-qubit gate case
//			printf("n-qubit case...f_idx: %d -> ftype: %d\n", f_idx, ftype_dev_vec[f_idx]);

			// multiply for i-th function block value
			FunctionCallback_nq f_dev_k;
			f_dev_k = QDEV_F_GATE_NQ_SELECTOR((QASM_F_TYPE)ftype_dev_vec[f_idx]);
#ifdef __QSIM_CPU__
			f_val *= f_dev_k(ik%fN_k, jk%fN_k, fn, fform, gapn, futype, fun, fuform, &(fargs_dev_vec[f_idx]));
//			printf("f_val after nq: %g  %g\n", f_val.real(), f_val.imag());
#else
			f_val = cuCmul(f_val, f_dev_k(ik%fN_k, jk%fN_k, fn, fform, gapn, futype, fun, fuform, &(fargs_dev_vec[f_idx])));
#endif
		}

		// update working indexes by the size f the processed function
		ik = ik / fN_k;
		jk = jk / fN_k;

		// check for an early exit condition on zero value
#ifdef __QSIM_CPU__
		if (abs(f_val) < QDEV_F_VAL_EPS) // avoid to test vs. zero using floats!
#else
		if (cuCabs(f_val) < QDEV_F_VAL_EPS) // avoid to test vs. zero using floats!
#endif
			break;
	}

#ifdef __QSIM_CPU__
//	printf("f_dev_qn_exec...%d %d %d  -> %f %f\n", i, j, tot_f, f_val.real(), f_val.imag());
#else
//	printf("f_dev_qn_exec...%d %d %d  -> %f %f\n", i, j, tot_f, f_val.x, f_val.y);
#endif
    return f_val;
}


#endif /* QSIM_QCPU_DEVICE_FUNCTION_EXEC_H_ */

