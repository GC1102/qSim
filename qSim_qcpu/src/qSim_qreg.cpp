/*
 * qSim_qreg.cpp
 *
 * --------------------------------------------------------------------------
 * Copyright (C) 2018 Gianni Casonato
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
 *  Created on: Mar 30, 2018
 *      Author: gianni
 *
 * Q-REG class, supporting control methods for:
 * - qreg initialisation from given number of qubits
 * - qregs state set/reset
 * - qregs state setup from values
 * - qregs state access
 *
 * Deferred device->host sync implemented, using a specific class attribute flag,
 * and done only if needed i.e. in state set/reset/peek/measure methods.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Mar-2018   Module creation.
 *  2.0   May-2022   Rewritten for implementing Q-REG control methods for supporting
 *                   function based Q-CPU, eliminating splitting in nodes and socket links.
 *  2.1   Nov-2022   Instruction set limitation to 1 and 2 qubit gates.
 *                   Used qreg_instruction class for argument data type handling.
 *                   Handled verbose flag for controlling diagnostic messages in
 *                   qureg transformation execution flow.
 *                   Handled deferred device->host sync, using a specific class attribute
 *                   flag, and done only if needed i.e. in state set/reset/peek/measure methods.
 *                   Supported q-function blocks - swap and cswap.
 *                   Handled CPU mode compiling and applied agnostic device class naming.
 *                   Code clean-up.
 *  2.2   Feb-2023   Supported qureg state expectation calculation and fixed
 *                   terminology for state probability measure.
 *                   Handled QML function blocks (feature map and q-net).
 *
 *  --------------------------------------------------------------------------
 */


#include <algorithm>
#include <iostream>
#include <complex>
#include <list>
#include <vector>
using namespace std;

#include <string.h>

#include "qSim_qasm.h"
#include "qSim_qreg.h"

#ifdef __QSIM_CPU__
#include "qSim_qcpu_device_CPU.h"
#else
#include "qSim_qcpu_device_GPU_CUDA.h"
#endif

#define QREG_ST_MAKE_VAL QDEV_ST_MAKE_VAL

// measure max index vector size allowed - due to performance reasons
#define MEASURE_MAX_INDEX_VEC_SIZE 10

/////////////////////////////////////////////////////////////////////////
// qreg class definition

qSim_qreg::qSim_qreg(int qn, qSim_qcpu_device* qcpu_dev, bool verbose) {
	// initialise state array (raw values)
	m_totQubits = qn;
	m_totStates = pow(2, qn);
	m_states_x = new QREG_ST_RAW_VAL_TYPE[m_totStates];

	// store qCpu CUDA instance
	m_qcpu_device = qcpu_dev;

	// setup device registers from class states (same initial states for both)
	m_qcpu_device->dev_qreg_host2device(&m_devStates_x, m_states_x, m_totStates);
	m_qcpu_device->dev_qreg_host2device(&m_devStates_y, m_states_x, m_totStates);
	m_syncFlag = true;

	// set observable type arrays
	m_obs_ev_map[QASM_EX_OBSOP_TYPE_COMP] = {1.0, 1.0};
	m_obs_ev_map[QASM_EX_OBSOP_TYPE_PAULIZ] = {1.0, -1.0};

	// store verbose flag setting
	m_verbose = verbose;

	// set qreg in ground state
	resetState();
}

qSim_qreg::~qSim_qreg() {
	// release memory on device first
	m_qcpu_device->dev_qreg_device_release(m_devStates_x);
	m_qcpu_device->dev_qreg_device_release(m_devStates_y);

	// release memory on class
	delete m_states_x;
}

// -------------------------------------

bool qSim_qreg::applyCoreInstruction(qSim_qinstruction_core* qr_instr, std::string* res_str) {
	// handle instruction execution based on instruction type and return response
	// for qureg state reset / set / transform instructions

	bool res;
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ST_RESET: {
			// apply to qureg
			res = resetState();
			if (!res)
				*res_str = "resetState generic error";
		}
		break;

		case QASM_MSG_ID_QREG_ST_SET: {
			// apply to qureg
			if (qr_instr->m_st_array.size() == 0) {
				// pure state setup
				res = setState(qr_instr->m_st_idx);
			}
			else {
				// arbitrary state values setup
				res = setState(&(qr_instr->m_st_array));
			}
			if (!res)
				*res_str = "setState generic error";
		}
		break;

		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
	//		cout << "...st-transform..." << endl;
//			qr_instr->dump();

			// extract arguments
			QASM_F_TYPE ftype = qr_instr->m_ftype;
			int fsize = qr_instr->m_fsize;
			int frep = qr_instr->m_frep;
			int flsq = qr_instr->m_flsq;
			QREG_F_INDEX_RANGE_TYPE fcrng = qr_instr->m_fcrng;
			QREG_F_INDEX_RANGE_TYPE ftrng = qr_instr->m_ftrng;
			QREG_F_ARGS_TYPE fargs = qr_instr->m_fargs;
			int futype = qr_instr->m_futype;
			QREG_F_INDEX_RANGE_TYPE fucrng = qr_instr->m_fucrng;
			QREG_F_INDEX_RANGE_TYPE futrng = qr_instr->m_futrng;
			QREG_F_ARGS_TYPE fuargs = qr_instr->m_fuargs;

			// apply to qureg
			res = transform(ftype, fsize, frep, flsq, fcrng, ftrng, fargs,
					        futype, fucrng, futrng, fuargs);
			if (!res)
				*res_str = "stateTransform generic error";
		}
		break;

		default: {
			res = false;
		}
	}

	return res;
}

// -------------------------------------

bool qSim_qreg::applyCoreInstruction(qSim_qinstruction_core* qr_instr, std::string* res_str,
									 QREG_ST_INDEX_TYPE* m_st, double* m_pr, QREG_ST_INDEX_ARRAY_TYPE* m_vec) {
	// handle instruction execution based on instruction type and return response
	// for qureg state measurement instruction

	bool res;
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ST_MEASURE: {
			// extract arguments
			int q_idx = qr_instr->m_q_idx;
			int q_len = qr_instr->m_q_len;
			bool m_rand = qr_instr->m_rand;
			bool m_coll = qr_instr->m_coll;

			// apply to qureg
			res = stateMeasure(q_idx, q_len, m_rand, m_coll, m_st, m_pr, m_vec);
			if (!res)
				*res_str = "stateMeasure generic error";
		}
		break;

		default: {
			res = false;
		}
	}

	return res;
}

// -------------------------------------

bool qSim_qreg::applyCoreInstruction(qSim_qinstruction_core* qr_instr, std::string* res_str,
									 double* m_exp) {
	// handle instruction execution based on instruction type and return response
	// for qureg state expectation instruction

	bool res;
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ST_EXPECT: {
			// extract arguments
			int st_idx = qr_instr->m_st_idx;
			int q_idx = qr_instr->m_q_idx;
			int q_len = qr_instr->m_q_len;
			QASM_EX_OBSOP_TYPE ex_opsOp = qr_instr->m_ex_obsOp;
//			cout << "qr_instr->m_ex_obsOp:" << qr_instr->m_ex_obsOp << "...ex_opsOp:" << ex_opsOp << endl;

			// apply to qureg
			res = stateExpectation(st_idx, q_idx, q_len, ex_opsOp, m_exp);
			if (!res)
				*res_str = "stateExpectation generic error";
		}
		break;

		default: {
			res = false;
		}
	}

	return res;
}

// -------------------------------------

bool qSim_qreg::applyCoreInstruction(qSim_qinstruction_core* qr_instr, std::string* res_str,
		                             QREG_ST_VAL_ARRAY_TYPE* st_array) {
	// handle instruction execution based on instruction type and return response
	// for qureg state peek instruction

	bool res;
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ST_PEEK: {
			// apply to qureg
			res = getStates(st_array);
			if (!res)
				*res_str = "peekState generic error";
		}
		break;

		default: {
			res = false;
		}
	}

	return res;
}

// -------------------------------------
// -------------------------------------

bool qSim_qreg::applyBlockInstruction(qSim_qinstruction_block* qr_instr, std::string* res_str) {
	// handle block instruction execution based on instruction type and return response
	// for qureg state reset / set / transform instructions

	bool res;
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
	//		cout << "...st-transform..." << endl;
//			qr_instr->dump();

			// extract arguments
			QASM_F_TYPE ftype = qr_instr->m_ftype;
//
			// translate into core instructions
			std::list<qSim_qinstruction_core*> qinstr_list;
			switch (ftype) {
				case QASM_FB_TYPE_Q1_SWAP: {
					qr_instr->unwrap_block_swap_q1(&qinstr_list, m_verbose);
				}
				break;

				case QASM_FB_TYPE_QN_SWAP: {
					qr_instr->unwrap_block_swap_qn(&qinstr_list, m_verbose);
				}
				break;

				case QASM_FB_TYPE_Q1_CSWAP: {
					qr_instr->unwrap_block_cswap_q1(&qinstr_list, m_verbose);
				}
				break;

				case QASM_FB_TYPE_QN_CSWAP: {
					qr_instr->unwrap_block_cswap_qn(&qinstr_list, m_verbose);
				}
				break;

				default: {
					// error case
					cerr << "qSim_qreg::applyBlockInstruction - unhandled function block type ["
						 << ftype << "]!!" << endl;
					res = false;
				}
			}
			if (m_verbose)
				cout << "applyBlockInstruction...qinstr_list.size: " << qinstr_list.size() << endl;

			// apply to qureg
			apply_instruction_and_release(&qinstr_list, qr_instr->m_fargs, &res, res_str);
		}
		break;

		default: {
			res = false;
		}
	}

	return res;
}

// -------------------------------------
// -------------------------------------

#define DUMP_FARGS(qinst) {\
	cout << "m_fargs.size: " << qinst->m_fargs.size()\
		 << " str: " << qinst->fargs_to_string(qinst->m_fargs) << endl;\
}

bool qSim_qreg::applyBlockInstructionQml(qSim_qinstruction_block_qml* qr_instr, std::string* res_str) {
	// handle QML block instruction execution based on instruction type and return response

//	cout << "qml qinstr..." << endl;
//	DUMP_FARGS(qr_instr);

	bool res;
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			// extract arguments
			QASM_F_TYPE ftype = qr_instr->m_ftype;

			// translate into core instructions
			std::list<qSim_qinstruction_core*>* qinstr_list;
			QREG_F_ARGS_TYPE qinstr_list_fargs;
			switch (ftype) {
				case QASM_FBQML_TYPE_FMAP: {
					qr_instr->unwrap_block_fmap(&qinstr_list, &qinstr_list_fargs, m_verbose);
				}
				break;

				case QASM_FBQML_TYPE_QNET: {
					qr_instr->unwrap_block_qnet(m_totQubits, &qinstr_list, &qinstr_list_fargs, m_verbose);
				}
				break;

				default: {
					// error case
					cerr << "qSim_qreg::applyBlockInstructionQml - unhandled function block type ["
						 << ftype << "]!!" << endl;
					res = false;
				}
			}
			if (m_verbose)
				cout << "applyBlockInstruction...qinstr_list.size: " << qinstr_list->size() << endl;

			// apply to qureg - no release (caching applied)!
			apply_instruction_and_release(qinstr_list, qinstr_list_fargs, &res, res_str, false);
		}
		break;

		default: {
			res = false;
		}
	}

	return res;
}

// -------------------------------------
// -------------------------------------

void qSim_qreg::apply_instruction_and_release(std::list<qSim_qinstruction_core*>* qinstr_list,
		QREG_F_ARGS_TYPE fargs, bool* res, std::string* res_str, bool do_release) {
	// apply to qureg using given fargs - overriding those used for q-instruction creation
	int i = 0;
	for (std::list<qSim_qinstruction_core*>::iterator it = qinstr_list->begin(); it != qinstr_list->end(); ++it) {
//		cout << "apply...f_type: " << (*it)->m_ftype << endl;
//		DUMP_FARGS((*it));
    	QREG_F_ARGS_TYPE fargs_i;
    	if ((*it)->m_fargs.size() > 0) {
    		fargs_i.push_back(fargs[i]);
    		i++;
    	}
		(*res) = transform((*it)->m_ftype, (*it)->m_fsize, (*it)->m_frep, (*it)->m_flsq,
				        (*it)->m_fcrng, (*it)->m_ftrng, fargs_i,
						(*it)->m_futype, (*it)->m_fucrng, (*it)->m_futrng, (*it)->m_fuargs);
		if (!(*res)) {
			*res_str = "block stateTransform generic error";
			break;
		}

		// release instruction object - if enabled
		if (do_release)
			delete *it;
	}

//	// release list
//	for (std::list<qSim_qinstruction_core*>::iterator it = qinstr_list->begin(); it != qinstr_list->end(); ++it) {
//		delete *it;
//	}
}

// -------------------------------------
// -------------------------------------

// state control and access
bool qSim_qreg::resetState() {
	// set qureg in ground state - call device function
	m_qcpu_device->dev_qreg_set_state(m_devStates_x, m_totStates, 0, m_verbose);

	// force an host-device sync
	m_syncFlag = false;
	synchDevStates();
	return true;
}

bool qSim_qreg::setState(unsigned st_idx) {
	// set qureg with given arbitrary pure state and align device register
	if (m_verbose)
		cout << "qreg::setState - pure state setup - st_idx: " << st_idx << endl;

	// perform sanity checks on given state array
	if (st_idx > m_totStates-1) {
		cerr << "ERROR!! qreg::set - incorrect pure state index passed - state not set!!" << endl;
		cerr << "state index: " << st_idx << " - qreg totStates: " << m_totStates << endl;
		return false;
	}

	// pure state to set - call device function
	m_qcpu_device->dev_qreg_set_state(m_devStates_x, m_totStates, st_idx, m_verbose);

	// synchronise host with device register data
	m_syncFlag = false;
	synchDevStates();
	return true;
}

bool qSim_qreg::setState(QREG_ST_VAL_ARRAY_TYPE* st_array) {
	// set qureg with given arbitrary state and align device register
	if (m_verbose)
		cout << "qreg::setState - arbitrary state setup" << endl;

	// perform sanity checks on given state array
	if (st_array == NULL) {
		cerr << "ERROR!! qreg::set - null state vector passed - state not set!!" << endl;
		return false;
	}

	if (st_array->size() != m_totStates) {
		cerr << "ERROR!! qreg::set - state vector of incorrect size passed - state not set!!" << endl;
		cerr << "st_array size: " << st_array->size() << " - qreg totStates: " << m_totStates << endl;
		return false;
	}

	// set qureg state using custom data
	// update host array first and align device afterwards
	for (unsigned int i=0; i<m_totStates; i++) {
		double st_r = (*st_array)[i].real();
		double st_i = (*st_array)[i].imag();
		m_states_x[i] = QREG_ST_MAKE_VAL(st_r, st_i);
	}
	m_qcpu_device->dev_qreg_host2device_align(m_devStates_x, m_states_x, m_totStates);

	// no sync needed - arrays aligned in this case
	m_syncFlag = true;
	return true;
}

// -------------------------------------

bool qSim_qreg::transform(QASM_F_TYPE ftype, int fsize, int frep, int flsq,
		                  QREG_F_INDEX_RANGE_TYPE fcrng, QREG_F_INDEX_RANGE_TYPE ftrng, QREG_F_ARGS_TYPE fargs,
						  int futype, QREG_F_INDEX_RANGE_TYPE fucrng, QREG_F_INDEX_RANGE_TYPE futrng, QREG_F_ARGS_TYPE fuargs) {
	// transform qureg - call proper CUDA function
	if (m_verbose)
		cout << "qSim_qreg::transform - function...ftype: " << ftype
		     << " fsize: " << fsize << " frep: " << frep << " flsq: " << flsq
			 << " fcrng: " << fcrng.to_string() << " ftrng: " << ftrng.to_string() << " fargs size: " << fargs.size()
			 << " futype: " << futype << " fucrng: " << fucrng.to_string() << " futrng: " << futrng.to_string()
			 << " fuargs size: " << fuargs.size() << endl;

	int ret = QDEV_RES_OK;

	// final check before execution: LSQ and repetitions consistent with function and qureg size
	if (powf(fsize, frep) > m_totStates) {
		cout << "!!!ERROR - function repetitions exceeds qureg size!!" << endl;
		return false;
	}
	else if (powf(fsize, frep) + powf(2, flsq) - 1 > m_totStates) {
		cout << "!!!ERROR - inconsistent LSQ value found [" << flsq << "]" << endl;
		return false;
	}

	// call CUDA function - based on function type class
	if (QASM_F_TYPE_IS_GATE_1QUBIT(ftype)) {
		// 1-qubit gate case found
		ret = m_qcpu_device->dev_qreg_apply_function_gate_1qubit(m_devStates_x, m_devStates_y, m_totStates,
												                 ftype, frep, flsq, &fargs, m_verbose);
	}
	else if (QASM_F_TYPE_IS_GATE_2QUBIT(ftype)) {
		// 2-qubit gate case found
		int fform = qSim_qinstruction_core::ctrange_2_form(fcrng, ftrng);
		ret = m_qcpu_device->dev_qreg_apply_function_gate_2qubit(m_devStates_x, m_devStates_y, m_totStates,
												                 ftype, frep, flsq, fform, futype, &fuargs,
																 m_verbose);
	}
	else if (QASM_F_TYPE_IS_GATE_NQUBIT(ftype)) {
		// n-qubit gate case found
		int fform = qSim_qinstruction_core::ctrange_2_form(fcrng, ftrng);
		int fgapn;
		if (fform == QASM_F_FORM_DIRECT)
			fgapn = fcrng.m_start - ftrng.m_stop - 1;
		else
			fgapn = ftrng.m_start - fcrng.m_stop - 1;
		int fun;
		if (QASM_F_TYPE_IS_GATE_1QUBIT(futype))
			fun = 1;
		else
			fun = 2;
		int fuform = qSim_qinstruction_core::ctrange_2_form(fucrng, futrng);
		ret = m_qcpu_device->dev_qreg_apply_function_controlled_gate_nqubit(m_devStates_x, m_devStates_y, m_totStates,
												                            ftype, fsize, frep, flsq, fform, fgapn,
																			futype, fun, fuform, &fuargs, m_verbose);
	}
	else {
		cout << "!!!ERROR - unhandled function transformation type:" << ftype << endl;
		return false;
	}

	if (m_verbose)
		cout << "qSim_qreg::transform - function applied on GPU! - result:" << ret << endl;

	if (ret == QDEV_RES_OK) {
		// swap device pointers
		QREG_ST_RAW_VAL_TYPE* app = m_devStates_x;
		m_devStates_x = m_devStates_y;
		m_devStates_y = app;

//		cout << "qureg states..." << endl;
//		for (unsigned int k=0; k<m_totStates; k++)
//	#ifndef __QSIM_CPU__
//			cout << "#" << k << " " << m_devStates_x[k].x << "  " << m_devStates_x[k].y << endl;
//	#else
//			cout << "#" << k << " " << m_devStates_x[k].real() << "  " << m_devStates_x[k].imag() << endl;
//	#endif

		// unset sync flag
		m_syncFlag = false;
	}
	return (ret == QDEV_RES_OK);
}

// -------------------------------------

bool qSim_qreg::getStates(QREG_ST_VAL_ARRAY_TYPE* stArray) {
	// check if qureg size allows a peek instruction
	if (this->m_totQubits > MEASURE_MAX_INDEX_VEC_SIZE) {
		cerr << "qSim_qreg::getStates - peek max state vector size exceeded - no values returned!!" << endl;
		return false;
	}

	// synchronise host with device
	synchDevStates();

	// convert to array and return
//	*stArray = QREG_ST_VAL_ARRAY_TYPE(m_totStates);
	stArray->clear();
	for (unsigned int i=0; i<m_totStates; i++) {
#ifndef __QSIM_CPU__
		double st_r = m_states_x[i].x;
		double st_i = m_states_x[i].y;
#else
		double st_r = m_states_x[i].real();
		double st_i = m_states_x[i].imag();
#endif
		stArray->push_back(QREG_ST_VAL_TYPE(st_r, st_i));
	}
	return true;
}

unsigned int qSim_qreg::getTotStates() {
	return m_totStates;
}

// -------------------------------------

// type of measurements
// 0 - real: using random expected value - status collapsed -> default behavior
// 1 - real: using max probability value - status collapsed
// 2 - simulated: using random probability value - status unchanged
// 3 - simulated: using probability value - status unchanged

bool qSim_qreg::stateMeasure(int q_idx, int q_len, bool m_rand, bool m_coll,
		                     QREG_ST_INDEX_TYPE* m_st, double* m_pr, QREG_ST_INDEX_ARRAY_TYPE* m_vec) {
	// perform measure on qureg, i.e. collapsing states, and applying
	// random expected selection (default) or deterministic max probability
	//
	// inputs:
	// - q_idx: measured sub-qureg start index position, in range [0...(n-1])], with LSB=0 and MSB = n-1.
	//          value -1 is for complete state measure (i.e. same as q_idx=0 and q_len=n)
	// - q_len: measured sub-qureg len, in range [1...n-q_idx-1]
	// - m_rand: use random expectation selection or deterministic max probability selection
	// - m_coll: perform state collapsing on measured qureg
	// - st_idx: measured state index in the selected qureg
	// - m_pr: measured state probability in the selected qureg
	// - m_vec: collapsed state index vector
	//

	// sanity checks in input arguments
	if (q_idx > (int)this->m_totQubits-1) {
		cerr << "qSim_qreg::stateMeasure - q_idx parameter [" << q_idx << "] outside allowed range - ERROR!!" << endl;
		return false;
	}

	if ((q_len < 0) || (q_len > (int)this->m_totQubits-q_idx)) {
		cerr << "qSim_qreg::stateMeasure - q_len parameter [" << q_len << "] outside allowed range - ERROR!!" << endl;
		return false;
	}

	bool d_vals = (this->m_totQubits-q_len <= MEASURE_MAX_INDEX_VEC_SIZE);
	if (!d_vals)
		cerr << "qSim_qreg::stateMeasure - measure max index vector size exceeded - no indeces returned!!" << endl;

	// synchronise host with device
	synchDevStates();

	// perform the requested measure
	bool res = false;
	if (m_coll) {
		// real measure to be performed - with qureg state collapse
		res = do_state_measure(q_idx, q_len, m_rand, true, m_st, m_pr, m_vec, d_vals);
	}
	else {
		// simulate measure to be performed - no qureg state collapse
		res = do_state_measure(q_idx, q_len, m_rand, false, m_st, m_pr, m_vec, d_vals);
	}
	return res;
}

bool qSim_qreg::do_state_measure(int q_idx, int q_len, bool do_rnd, bool collapse_st,
		                         QREG_ST_INDEX_TYPE* m_st, double* m_pr,
								 QREG_ST_INDEX_ARRAY_TYPE* m_vec, bool d_vals) {
	// perform measure on qureg, i.e. collapsing states, and applying
	// random expected selection (default) or deterministic max probability
	//
	// inputs:
	// - q_idx: measured sub-qureg start index position, in range [0...(n-1])], with LSB=0 and MSB = n-1.
	//          value -1 is for complete state measure (i.e. same as q_idx=0 and q_len=n)
	// - q_len: measured sub-qureg len, in range [1...n-q_idx-1]
	// - do_rnd: use random expectation selection or deterministic max probability selection
	// - collapse_st: perform state collapsing on measured qureg	
	// - st_idx: measured state index in the selected qureg
	//

	if (m_verbose)
		cout << "do_measure...q_idx: " << q_idx << " q_len: " << q_len << " do_rnd: " << do_rnd
			 << " collapse_st: " << collapse_st << endl;

	bool res = true;

    // check qubit index
    if (q_idx < 0) {
        q_idx = 0;
        q_len = this->m_totQubits;
    }

    // calculate sub-states expectations
    int q_stn = powf(2, q_len); // total number of sub-states for the measured qubits
    std::vector<double> pr_vec;

	// take all expectations for selected qureg
    for (int i=0; i<q_stn; i++) {
        double pr = get_state_probability(i, q_idx, q_len);
        pr_vec.push_back(pr);
    }

    // handle measure state index calculation
    if (do_rnd) {
		// get random index using expectations
    	// calculating a random probability and selecting
    	// the min index with probability >= calculated one

    	double pr_rnd = std::rand()/RAND_MAX;
    	*m_st = 0;
    	*m_pr = pr_vec[*m_st];
    	for (int i=1; i<q_stn; i++) {
    		if ((pr_vec[i] >= pr_rnd) && (pr_vec[i] < *m_pr)) {
    			*m_pr = pr_vec[i];
    			*m_st = i;
    		}
    	}
    }
    else {
    	// get highest probability value directly
    	*m_st = 0;
    	*m_pr = pr_vec[*m_st];
    	for (int i=1; i<q_stn; i++) {
    		if (pr_vec[i] > *m_pr) {
    			*m_pr = pr_vec[i];
    			*m_st = i;
    		}
    	}
    }

    // handle qureg state collapsing after measure
    if (collapse_st) {
    	// collapse states
    	for (int i=0; i<(int)this->m_totStates; i++) {
    		unsigned int val_i = get_state_bitval(i, q_idx, q_len);
    	    if (val_i == *m_st) {
                // state taken
#ifndef __QSIM_CPU__
    	        this->m_states_x[i] = cuCdiv(this->m_states_x[i], QREG_ST_MAKE_VAL(sqrt(*m_pr), 0.0));
#else
    	        this->m_states_x[i] /= QREG_ST_MAKE_VAL(sqrt(*m_pr), 0.0);
#endif
    	        m_vec->push_back(i);
    	    }
    	    else {
    	        // state not taken - reset
    	    	this->m_states_x[i] = QREG_ST_MAKE_VAL(0.0, 0.0);
    	    }
    	}
    	
    	// apply to device
    	m_qcpu_device->dev_qreg_host2device_align(m_devStates_x, m_states_x, m_totStates);
    }

	return res;
}

double qSim_qreg::get_state_probability(int st_idx, int q_idx, int q_len) {
	// calculate the state probability (i.e. partial or total probability) at given index
	// for the given measured sub-qureg taking values from state coefficient(s)

//	cout << "get_state_probability...st_idx: " << st_idx << " q_idx: " << q_idx << "  q_len: " << q_len << endl;

	// inputs:
	// - st_idx: probability state index (>=0!)
	// - q_idx: measured sub-qureg start index (-1 for whole qureg or [0, ..., n-1] for full qureg/partial sub-qureg)
	// - q_len: measured sub-qureg length (0 for whole qureg or [1, ..., n] for full qureg/partial sub-qureg
	//
	double m_pr;
	// check qureg specs
    if ((q_idx < 0) || ((q_idx == 0) && ((unsigned int)q_len == this->m_totQubits))) {
    	// complete qureg state probability

    	// probability of the state taken - abs() get the absolute value of a complex number
#ifndef __QSIM_CPU__
//    	m_pr = cuCmul(this->m_states_x[st_idx], cuConj(this->m_states_x[st_idx])).x;
    	m_pr = powf(cuCabs(this->m_states_x[st_idx]), 2.0);
#else
    	m_pr = std::norm(this->m_states_x[st_idx]);
#endif

//    	cout << "get_state_probability...st_idx: " << st_idx << " --> st_ampl: " << this->m_states_x[st_idx].x << "," << this->m_states_x[st_idx].y << endl;
//    	cout << "st_ampl-conj: " << cuConj(this->m_states_x[st_idx]).x << "," << cuConj(this->m_states_x[st_idx]).y << endl;
//    	cout << "pr: " << cuCmul(this->m_states_x[st_idx], cuConj(this->m_states_x[st_idx])).x << "," << cuCmul(this->m_states_x[st_idx], cuConj(this->m_states_x[st_idx])).y << endl;
//    	cout << "pr2: " << powf(cuCabs(this->m_states_x[st_idx]), 2.0) << endl;

    }
    else {
    	// give partial qureg state probability
    	// look at all partial state occurrences
    	m_pr = 0.0;
    	for (int i=0; i<(int)this->m_totStates; i++) {
    		// extract given qubit partial state (index-length) from i-th state
			int val_i = get_state_bitval(i, q_idx, q_len);

			// check it with given sub-state
	        if (st_idx == val_i) {
	        	// probability of the state taken - abs() get the absolute value of a complex number
#ifndef __QSIM_CPU__
//	        	m_pr += cuCmul(this->m_states_x[i], cuConj(this->m_states_x[i])).x;
	        	m_pr += powf(cuCabs(this->m_states_x[i]), 2.0);
#else
	        	m_pr += std::norm(this->m_states_x[i]);
#endif
			}
		}
	}
	return m_pr;
}

int qSim_qreg::get_state_bitval(int val, int b_idx, int b_len) {
    int b = 0;
    for (int i=0; i<b_len; i++) {
        b += int(powf(2, i))*((val & int((powf(2, (b_idx+i))))) >> (b_idx+i));
    }
    return b;
}

// -------------------------------------
// -------------------------------------

bool qSim_qreg::stateExpectation(int st_idx, int q_idx, int q_len, QASM_EX_OBSOP_TYPE ex_opsOp, double* m_exp) {
	// - st_idx: state index for expectation (-1 for all qureg states and a value for specific state)
	// - q_idx: qubit start index for partial qureg expectation or -1 for complete qureg (st_idx >= 0 case only)
	// - q_len: qubit length for partial qureg expectation (q_idx >= 0 case only)
	// - obs_op: observable operator

	// sanity checks in input arguments
	if (st_idx > (int)this->m_totStates-1) {
		cerr << "qSim_qreg::stateExpectation - st_idx parameter [" << st_idx << "] outside allowed range - ERROR!!" << endl;
		return false;
	}

	if (q_idx > (int)this->m_totQubits-1) {
		cerr << "qSim_qreg::stateExpectation - q_idx parameter [" << q_idx << "] outside allowed range - ERROR!!" << endl;
		return false;
	}

	if ((q_len < 0) || (q_len > (int)this->m_totQubits-q_idx)) {
		cerr << "qSim_qreg::stateExpectation - q_len parameter [" << q_len << "] outside allowed range - ERROR!!" << endl;
		return false;
	}

	bool d_vals = (this->m_totQubits-q_len <= MEASURE_MAX_INDEX_VEC_SIZE);
	if (!d_vals)
		cerr << "qSim_qreg::stateExpectation - measure max index vector size exceeded - no indeces returned!!" << endl;

	if (m_verbose)
		cout << "stateExpectation -> st_idx: " << st_idx << " q_idx: " << q_idx
			 << " q_len: " << q_len << " ex_opsOp: " << ex_opsOp << endl;

	// synchronise host with device
	synchDevStates();

	// take value from state coefficient(s)
	if (st_idx < 0) {
		// handle complete qureg states
		if (m_verbose)
			cout << "all states - complete ==> st_idx: -1" << endl;

		// get observable expectation values
		std::vector<double> ex_obsOp_vec;
		get_state_expectations(-1, 0, ex_opsOp, &ex_obsOp_vec);

		// get state probability values
		std::vector<double> pr_vec;
		get_state_probabilities(&pr_vec);

		// sum of the expectations of all qubit states
		(*m_exp) = 0.0;
		for (unsigned int i=0; i<m_totStates; i++) {
			(*m_exp) += ex_obsOp_vec[i]*pr_vec[i];
			if (m_verbose)
				cout << "i: " << i << " pr_i: " << pr_vec[i] << " exp_i: " << ex_obsOp_vec[i]*pr_vec[i] << endl;
		}
		if (m_verbose)
			cout << "tot exp: " << (*m_exp) << endl;
	}
	else {
		// specific state given
		if (m_verbose)
			cout << "specific state - complete => st_idx:" << st_idx << endl;

		// get observable expectation values
		std::vector<double> ex_obsOp_vec;
		get_state_expectations(q_idx, q_len, ex_opsOp, &ex_obsOp_vec);

		// expectation of the given qureg state
		if (q_idx < 0) {
			// full qureg
			double pr = get_state_probability(st_idx, -1, 0);
			(*m_exp) = ex_obsOp_vec[st_idx]*pr;
//			cout << "...pr: " << pr << "  st_exp: " << ex_obsOp_vec[st_idx] << "  exp: " << (*m_exp) << endl;
		}
		else {
			// specific sub-qureg - spanning over all states
			(*m_exp) = 0.0;
			std::vector<double> pr_vec;
			get_state_probabilities(&pr_vec);
			for (unsigned int i=0; i<m_totStates; i++) {
				int val_i = get_state_bitval(i, q_idx, q_len);
				if (val_i == st_idx) {
					// state taken - update expectation
					(*m_exp) += ex_obsOp_vec[i ]* pr_vec[i];
				}
				else {
					// state not taken - skip it
					;
				}
			}
		}
	}

	return true;
}

void qSim_qreg::get_state_probabilities(std::vector<double>* pr_vec) {
	// get qureg state probability array
//	QREG_ST_VAL_ARRAY_TYPE st_vec;
//	getStates(&st_vec);
	pr_vec->clear();
//	for (unsigned int i=0; i<st_vec.size(); i++) {
//		pr_vec->push_back(std::norm(st_vec[i]));
//	}
	for (unsigned int i=0; i<m_totStates; i++) {
#ifndef __QSIM_CPU__
		double st_r = m_states_x[i].x;
		double st_i = m_states_x[i].y;
#else
		double st_r = m_states_x[i].real();
		double st_i = m_states_x[i].imag();
#endif
		pr_vec->push_back(std::norm(std::complex<double>(st_r, st_i)));
	}

	if (m_verbose) {
		cout << "pr_vec: ";
		for (unsigned int i=0; i<pr_vec->size(); i++)
			cout << (*pr_vec)[i] << " ";
		cout << endl;
	}

}

void qSim_qreg::kron_product(std::vector<double>* v1, std::vector<double>* v2, std::vector<double>* v3) {
	// perform kroneker product between two vectors
	std::vector<double> v_aux;
	for (unsigned int i=0; i<v1->size(); i++) {
		for (unsigned int j=0; j<v2->size(); j++) {
			v_aux.push_back((*v1)[i]*(*v2)[j]);
		}
	}
	(*v3) = v_aux;
}

void qSim_qreg::get_state_expectations(int q_idx, int q_len, QASM_EX_OBSOP_TYPE ex_obsOp,
									   std::vector<double>* ex_obsOp_vec) {
	// get observable expectations for current qureg start index and size
	// applying ones vectors as LSQ and MSQ via tensor products

	// - q_idx: qubit start index for partial qureg expectation or -1 for complete qureg
	// - q_len: qubit length for partial qureg expectation (q_idx >= 0 case only)
	// - obs_op: observable operator

	// retrieve operator vector for given observable
	std::vector<double> obs_ev_1q_vec = m_obs_ev_map[ex_obsOp];
	if (m_verbose) {
		cout << "ex_obsOp: " << ex_obsOp << endl;
		cout << "ex_obs_ev_1q: ";
		for (unsigned int i=0; i<obs_ev_1q_vec.size(); i++)
			cout << obs_ev_1q_vec[i] << " ";
		cout << endl;
	}

	if (q_idx < 0) {
    	// whole qureg - apply 1-qubit obs_op replicas for all qubits
		(*ex_obsOp_vec) = obs_ev_1q_vec;
		for (unsigned int i=0; i<m_totQubits-1; i++) {
			kron_product(ex_obsOp_vec, &obs_ev_1q_vec, ex_obsOp_vec);
		}
	}
	else {
		// specified sub-qureg - apply 1-qubit obs_op used for given qubit range only
		// and 1-qubit identity_op LSQ and MSQ fillers for the rest
		std::vector<double> ones_vec = {1.0, 1.0};

		// handle LSQ identities + first 1-qubit obs_op replica
		if (q_idx > 0) {
			(*ex_obsOp_vec) = ones_vec;
			for (int i=0; i<q_idx-1; i++) {
				kron_product(ex_obsOp_vec, &ones_vec, ex_obsOp_vec);
			}
			kron_product(ex_obsOp_vec, &obs_ev_1q_vec, ex_obsOp_vec);
		}
		else {
			(*ex_obsOp_vec) = obs_ev_1q_vec;
		}

		// add other 1-qubit obs_op replicas - if needed
		for (int i=0; i<q_len-1; i++) {
			kron_product(ex_obsOp_vec, &obs_ev_1q_vec, ex_obsOp_vec);
		}

		// add MSQ replicas - if needed
		for (unsigned int i=q_idx+q_len; i<m_totQubits; i++) {
			kron_product(ex_obsOp_vec, &ones_vec, ex_obsOp_vec);
		}
	}

	// return overall observable expval vector - spanning the full qureg!
	if (m_verbose)
    	cout << "q_idx: " << q_idx << " --> obs_ev_vec: " << ex_obsOp_vec->size() << endl;

	if (m_verbose) {
		cout << "ex_obsOp_vec: ";
		for (unsigned int i=0; i<ex_obsOp_vec->size(); i++)
			cout << (*ex_obsOp_vec)[i] << " ";
		cout << endl;
	}

}

// -------------------------------------
// -------------------------------------

void qSim_qreg::synchDevStates() {
	// synchronise qreg content with device array, if needed
	if (m_verbose)
		cout << "qSim_qreg::synchDevStates - synch_flag: " << m_syncFlag << endl;

	if (!m_syncFlag) {
		// qureg host & device not in sync - perform alignment
		m_qcpu_device->dev_qreg_device2host(m_states_x, m_devStates_x, m_totStates);
		m_syncFlag = true;
	}
}

// -------------------------------------

void qSim_qreg::dump(unsigned max_st) {
	cout << "*** qSim_qreg dump ***" << endl << endl;
	cout << " m_devStates_x: " << m_devStates_x << endl;
	cout << " m_devStates_y: " << m_devStates_y << endl;
	cout << " m_totStates:   " << m_totStates << endl;
	unsigned int tot_st = std::min(m_totStates, max_st);
	for (unsigned int k=0; k<tot_st; k++)
#ifndef __QSIM_CPU__
		cout << "#" << k << " " << m_states_x[k].x << "  " << m_states_x[k].y << endl;
#else
		cout << "#" << k << " " << m_states_x[k].real() << "  " << m_states_x[k].imag() << endl;
#endif
	if (tot_st < m_totStates)
		cout << "...";
	cout << endl;
}

