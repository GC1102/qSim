/*
 * qSim_qcpu.cpp
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
 *  Created on: Apr 8, 2018
 *      Author: gianni
 *
 * Q-CPU class, supporting control methods for:
 * - CPU reset and shutdown
 * - qregs handling (creation, set/reset and state access)
 * - qregs transformation via state functions (using CUDA kernel call wrappers)
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Apr-2018   Module creation.
 *  2.0   May-2022   Updated for implementing Q-CPU control methods.
 *  2.1   Nov-2022   Renamed instruction dispatching and qureg state get methods.
 *                   Moved state value::string conversion helpers to qreg_instruction
 *                   class.
 *                   Used qreg_instruction class for instruction data handling.
 *                   Handled verbose flag for controlling diagnostic messages in
 *                   the qureg instruction execution flow.
 *                   Supported function block - 1-qubit SWAP.
 *                   Handled CPU mode compiling and applied agnostic class renaming
 *                   to qSim_qcpu_device.
 *                   Code clean-up.
 *  2.2   Feb-2023   Supported qureg state expectation calculation and fixed
 *                   terminology for state probability measure.
 *                   Handled QML function blocks (feature map and q-net).
 *
 *  --------------------------------------------------------------------------
 */


#include <sstream>
#include <string>
#include <iostream>
#include <iterator>
using namespace std;

#include "qSim_qcpu.h"
#include "qSim_qasm.h"
#include "qSim_qreg.h"

static QREG_HNDL_TYPE s_qreg_id_counter = 1;

// constructor
qSim_qcpu::qSim_qcpu(bool verbose) {
	// instantiate CUDA device handler
	m_qcpu_device = new qSim_qcpu_device();

#ifndef __QSIM_CPU__
	// check for CUDA device availability
	if (m_qcpu_device->dev_get_gpu_cuda_count() == 0) {
    	fprintf(stderr, "No GPU CUDA device found - cannot continue\n");
        exit(EXIT_FAILURE);
	}
#endif
	m_verbose = verbose;
}

// destructor
qSim_qcpu::~qSim_qcpu() {
	// release qreg map objects
	qreg_mapRelease();
	delete m_qcpu_device;
}

// *********************************************************

// qcpu data reset control method
bool qSim_qcpu::reset() {
	cout << "qSim_qcpu::reset" << endl;

	// release qreg map objects
	qreg_mapRelease();
	s_qreg_id_counter = 1;
	return true;
}

// qcpu turn off control method
bool qSim_qcpu::switchOff() {
	cout << "qSim_qcpu::switchOff" << endl;
	// ...
	return true;
}

// *********************************************************

// ------------------------

#define SAFE_INSTRUCTION_VALIDITY_CHECK(err_msg_tag) {\
	if (!qr_instr.is_valid()) {\
		cerr << "qSim_qcpu::dispatch_message - incorrect " << err_msg_tag << " received!!" << endl;\
		qr_instr.dump();\
		int counter = msg_in->get_counter();\
		int id = QASM_MSG_ID_RESPONSE;\
		params.insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));\
		params.insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, err_msg_tag+" transformation syntax error"));\
		qSim_qasm_message* msg_out = new qSim_qasm_message(counter, id, params);\
		return msg_out;\
	}\
}

// qasm message dispatching for execution entry point
qSim_qasm_message* qSim_qcpu::dispatch_instruction(qSim_qasm_message* msg_in) {
	// handle instruction execution based on instruction type and return response

	// allocate a qureg instruction object and process it
	QASM_MSG_PARAMS_TYPE params;
	if (qSim_qinstruction_base::is_core(msg_in)) {
		// perform core instruction
		qSim_qinstruction_core qr_instr(msg_in);

		// check instruction correctness
		SAFE_INSTRUCTION_VALIDITY_CHECK(std::string("core instruction"))

		// execute instruction
		exec_qureg_instruction_core(&qr_instr, &params);
	}
	else if (qSim_qinstruction_base::is_block(msg_in)) {
		// perform block instruction
		qSim_qinstruction_block qr_instr = qSim_qinstruction_block(msg_in);

		// check instruction correctness
		SAFE_INSTRUCTION_VALIDITY_CHECK(std::string("block instruction"))

		// execute instruction
		exec_qureg_instruction_block(&qr_instr, &params);
	}
	else if (qSim_qinstruction_base::is_block_qml(msg_in)) {
		// perform QML block instruction
		qSim_qinstruction_block_qml qr_instr = qSim_qinstruction_block_qml(msg_in);

		// check instruction correctness
		SAFE_INSTRUCTION_VALIDITY_CHECK(std::string("QML block instruction"))

		// execute instruction
		exec_qureg_instruction_block_qml(&qr_instr, &params);
	}
	else {
		// error case
		cerr << "qSim_qcpu::dispatch_message - unhandled qasm class type "
			 << msg_in->get_id() << "!!" << endl;
		params.insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
		params.insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, "Unhandled qasm message type"));
	}

	std::string res_val = params[QASM_MSG_PARAM_TAG_RESULT];
	cout << "qCpu message [" << msg_in->get_id() << "] executed - result: " << res_val << endl;

	// build output message
	int counter = msg_in->get_counter();
	int id = QASM_MSG_ID_RESPONSE;
	qSim_qasm_message* msg_out = new qSim_qasm_message(counter, id, params);
	return msg_out;
}

// *********************************************************
// *********************************************************

#define SAFE_QREG_OBJ(qr_h, qr_obj) { \
	if (m_qreg_map.count(qr_h)) \
		qr_obj = m_qreg_map[qr_h]; \
	else { \
		cerr << "qSim_qcpu - wrong qreg handler provided [" << qr_h << "]!!!" << endl; \
		return false; \
	} \
}

// qureg control - allocation for given number of qubits
QREG_HNDL_TYPE qSim_qcpu::qureg_allocate(qSim_qinstruction_core* qr_instr) {
	// perform qureg allocation using given instruction fields, namely:
	// - qureg size
	//
	// extract arguments
	int qn = qr_instr->m_qn;
	if (m_verbose)
		cout << "qSim_qcpu::qureg_allocate - qn: " << qn << endl;

	// create a new qreg instance of given size and store in the map
	qSim_qreg* qr_obj = new qSim_qreg(qn, m_qcpu_device, m_verbose);
//	qr_obj->dump();

	const QREG_HNDL_TYPE qr_h = s_qreg_id_counter;
	m_qreg_map.insert(std::make_pair(qr_h, qr_obj));
	s_qreg_id_counter++;
	return qr_h;
}

// qureg control - allocation for given number of qubits
bool qSim_qcpu::qureg_release(qSim_qinstruction_core* qr_instr) {
	// perform qureg release using given instruction fields, namely:
	// - qureg handler
	//
	// extract arguments
	int qr_h = qr_instr->m_qr_h;
	if (m_verbose)
		cout << "qSim_qcpu::qureg_release - qr_h: " << qr_h << endl;

	// deallocate  given qreg instance
	qSim_qreg* qr_obj;// = m_qreg_map[qr_h];
	SAFE_QREG_OBJ(qr_h, qr_obj);
	m_qreg_map.erase(qr_h);
	delete qr_obj;
	return true;
}

// -----------------------------------------------------

// qureg core instructions handling
bool qSim_qcpu::exec_qureg_instruction_core(qSim_qinstruction_core* qr_instr,
		                                    QASM_MSG_PARAMS_TYPE* params) {
	// execute qureg core instruction using given instruction fields - based on instruction type
	//
	bool res;
	std::string res_str;
	params->clear();
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ALLOCATE: {
			// allocate a new qureg of given size and return the handler
			//
			// apply to qureg
			QREG_HNDL_TYPE qr_h = qureg_allocate(qr_instr);
			res = true;

			// store result
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK));
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_QREG_H, to_string(qr_h)));
		}
		break;

		case QASM_MSG_ID_QREG_RELEASE: {
			// release a given qureg by handler
			//
			// apply to qureg
			qureg_release(qr_instr);
			res = true;

			// store result
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK));
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ST_RESET:
		case QASM_MSG_ID_QREG_ST_SET:
		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			// reset/set or transform qureg state
			//
			// extract arguments
			int qr_h = qr_instr->m_qr_h;
			if (m_verbose)
				cout << "qSim_qcpu::exec_qureg_instruction_core reset/set/transform - qr_h: " << qr_h << endl;

			// apply to qureg
			qSim_qreg* qr_obj;
			SAFE_QREG_OBJ(qr_h, qr_obj);
			res = qr_obj->applyCoreInstruction(qr_instr, &res_str);

			// store result
			if (res)
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK));
			else {
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, res_str));
			}
		}
		break;

		case QASM_MSG_ID_QREG_ST_MEASURE: {
			// measure qureg state
			//
			// extract arguments
			int qr_h = qr_instr->m_qr_h;
			if (m_verbose)
				cout << "qSim_qcpu::exec_qureg_instruction_core measure - qr_h: " << qr_h << endl;

			// apply to qureg
			qSim_qreg* qr_obj;
			SAFE_QREG_OBJ(qr_h, qr_obj);
			QREG_ST_INDEX_TYPE m_st;
			double m_pr;
			QREG_ST_INDEX_ARRAY_TYPE m_vec;
			res = qr_obj->applyCoreInstruction(qr_instr, &res_str, &m_st, &m_pr, &m_vec);

			// store result
			if (res) {
				// measure done
				if (m_verbose)
					cout << "measure ok...m_st: " << m_st << "  m_pr: " << m_pr << "  m_vec.size: " << m_vec.size() << endl;
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_QREG_MSTIDX, to_string(m_st)));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_QREG_MSTPR, qr_instr->double_value_to_string(m_pr)));
				std::string m_vec_str = qr_instr->measure_index_value_to_string(m_vec);
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_QREG_MSTIDXS, m_vec_str));
			}
			else {
				// measure error
				if (m_verbose)
					cerr << "measure error!!" << endl;
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, res_str));
			}
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ST_EXPECT: {
			// calculate qureg state expectation
			//
			// extract arguments
			int qr_h = qr_instr->m_qr_h;
			if (m_verbose)
				cout << "qSim_qcpu::exec_qureg_instruction_core expectation - qr_h: " << qr_h << endl;
//			cout << "qr_instr->m_ex_obsOp:" << qr_instr->m_ex_obsOp << endl;

			// apply to qureg
			qSim_qreg* qr_obj;
			SAFE_QREG_OBJ(qr_h, qr_obj);
			double m_exp;
			res = qr_obj->applyCoreInstruction(qr_instr, &res_str, &m_exp);

			// store result
			if (res) {
				// measure done
				if (m_verbose)
					cout << "expectation ok...m_exp: " << m_exp << endl;
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_QREG_EXSTVAL, qr_instr->double_value_to_string(m_exp)));
			}
			else {
				// measure error
				if (m_verbose)
					cerr << "measure error!!" << endl;
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, res_str));
			}
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ST_PEEK: {
			// peek qureg state values - diagnostics only
			//
			int qr_h = qr_instr->m_qr_h;
			if (m_verbose)
				cout << "qSim_qcpu::exec_qureg_instruction_core peek - qr_h: " << qr_h << endl;

			// apply to qureg
			qSim_qreg* qr_obj;
			SAFE_QREG_OBJ(qr_h, qr_obj);
			QREG_ST_VAL_ARRAY_TYPE q_st;
			res = qr_obj->applyCoreInstruction(qr_instr, &res_str, &q_st);

			// store result
			if (res) {
				std::string qr_st_str = qr_instr->state_value_to_string(q_st);
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_QREG_STVALS, qr_st_str));
			}
			else {
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, res_str));
			}
		}
		break;

		// --------------------

		default: {
			// error case
			cerr << "qSim_qcpu::exec_qureg_instruction_core - unhandled qasm message type "
				 << qr_instr->m_type << "!!" << endl;
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, "Unhandled qasm message type"));
			res = false;
		}
	}
	return res;
}

// -----------------------------------------------------

// qureg block instructions handling
bool qSim_qcpu::exec_qureg_instruction_block(qSim_qinstruction_block* qr_instr,
		                                     QASM_MSG_PARAMS_TYPE* params) {
	// execute qureg block instruction using given instruction fields - based on instruction type
	//
	bool res;
	std::string res_str;
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			// reset/set or transform qureg state
			//
			// extract arguments
			int qr_h = qr_instr->m_qr_h;
			if (m_verbose)
				cout << "qSim_qcpu::exec_qureg_instruction_block transform - qr_h: " << qr_h << endl;

			// apply to qureg
			qSim_qreg* qr_obj;
			SAFE_QREG_OBJ(qr_h, qr_obj);
			res = qr_obj->applyBlockInstruction(qr_instr, &res_str);

			// store result
			if (res)
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK));
			else {
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, res_str));
			}
		}
		break;
		// --------------------

		default: {
			// error case
			cerr << "qSim_qcpu::exec_qureg_instruction_block - unhandled qasm message type "
				 << qr_instr->m_type << "!!" << endl;
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, "Unhandled qasm message type"));
			res = false;
		}
	}
	return res;
}

// -----------------------------------------------------

// qureg QML block instructions handling
bool qSim_qcpu::exec_qureg_instruction_block_qml(qSim_qinstruction_block_qml* qr_instr,
		                                         QASM_MSG_PARAMS_TYPE* params) {
	// execute qureg QML block instruction using given instruction fields - based on instruction type
	//
	bool res;
	std::string res_str;
	switch (qr_instr->m_type) {
		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			// transform qureg state
			//
			// extract arguments
			int qr_h = qr_instr->m_qr_h;
			if (m_verbose)
				cout << "qSim_qcpu::exec_qureg_instruction_block_qml transform - qr_h: " << qr_h << endl;

			// apply to qureg
			qSim_qreg* qr_obj;
			SAFE_QREG_OBJ(qr_h, qr_obj);
			res = qr_obj->applyBlockInstructionQml(qr_instr, &res_str);

			// store result
			if (res)
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK));
			else {
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
				params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, res_str));
			}
		}
		break;
		// --------------------

		default: {
			// error case
			cerr << "qSim_qcpu::exec_qureg_instruction_block_qml - unhandled qasm message type "
				 << qr_instr->m_type << "!!" << endl;
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK));
			params->insert(std::make_pair(QASM_MSG_PARAM_TAG_ERROR, "Unhandled qasm message type"));
			res = false;
		}
	}
	return res;
}

// *********************************************************

// monitoring & diagnostics
void qSim_qcpu::dump() {
	cout << "*** qCpu content dump ***" << endl << endl;

	cout << "Tot qRegs:" << m_qreg_map.size() << endl;
	for (const auto &keyval_pair : m_qreg_map ) {
		cout << " #" << keyval_pair.first << " - q-states: " << keyval_pair.second->getTotStates() << endl;
	}
	cout << endl;
	cout << "**************************" << endl << endl;
}

#ifndef __QSIM_CPU__
void qSim_qcpu::dumpGpuInfo() {
	cout << "*** qCpu GPU CUDA dump ***" << endl << endl;
	m_qcpu_device->dev_gpu_cuda_properties_dump();
	cout << "**************************" << endl << endl;
}
#endif

int qSim_qcpu::qureg_size(int qr_h) {
	qSim_qreg* qr_obj = m_qreg_map[qr_h];
	return log2(qr_obj->getTotStates());
}

// *********************************************************
// support methods
// *********************************************************

// release qreg map objects
void qSim_qcpu::qreg_mapRelease() {
	for (const auto &keyval_pair : m_qreg_map ) {
		delete keyval_pair.second;
	}
	m_qreg_map.clear();
}

