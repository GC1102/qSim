/*
 * qSim_qinstruction_core.cpp
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
 *  Created on: Nov 13, 2022
 *      Author: gianni
 *
 * Q-Reg instruction core class, handling qCpu "core" instruction dataset
 * extracted from a QASM message for performing:
 * - qreg initialisation and state handling
 * - qreg state transformations
 *
 * Derived class from qSim_qinstruction_base.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Nov-2022   Module creation.
 *
 *  --------------------------------------------------------------------------
 */


#include <iostream>
#include <complex>
using namespace std;

#include <string.h>

#include "qSim_qinstruction_core.h"


/////////////////////////////////////////////////////////////////////////
// qreg instruction core class

// ------------------------

#define SAFE_TRASFORMATION_PARAMS_CHECK() {\
	if (QASM_F_TYPE_IS_GATE_1QUBIT(m_ftype)) \
		m_valid = this->check_params_1q();\
	else if (QASM_F_TYPE_IS_GATE_2QUBIT(m_ftype)) \
		m_valid = this->check_params_2q();\
	else if (QASM_F_TYPE_IS_GATE_NQUBIT(m_ftype)) \
		m_valid = this->check_params_nq();\
	else {\
		cerr << "qSim_qinstruction_core - unhandled ftype value [" << m_ftype << "]!!";\
		m_valid = false;\
	}\
	if (!m_valid)\
		return; \
}

// ------------------------

// constructor & destructor
qSim_qinstruction_core::qSim_qinstruction_core(qSim_qasm_message* msg) :
		qSim_qinstruction_base(msg->get_id()) {
	// initialise from given message
	m_qn = 0;
	m_qr_h = 0;
	m_st_array = QREG_ST_VAL_ARRAY_TYPE();
	m_q_idx = 0;
	m_q_len = 0;
	m_rand = false;
	m_coll = false;
	m_ftype = QASM_F_TYPE_NULL;
	m_fsize = 0;
	m_frep = 0;
	m_flsq = 0;
	m_fcrng = QREG_F_INDEX_RANGE_TYPE();
	m_ftrng = QREG_F_INDEX_RANGE_TYPE();
	m_futype = QASM_F_TYPE_NULL;
	m_fargs = QREG_F_ARGS_TYPE();
	m_valid = true; // updated in the switch in case of exceptions
//	cout << "qSim_qinstruction_core...m_type: " << m_type << "  msg_id: " << msg->get_id() << endl;

	switch (m_type) {
		case QASM_MSG_ID_QREG_ALLOCATE: {
			// qureg allocation message handling
			SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_QREG_QN, m_qn)
		}
		break;

		case QASM_MSG_ID_QREG_RELEASE:
		case QASM_MSG_ID_QREG_ST_RESET:
		case QASM_MSG_ID_QREG_ST_PEEK: {
			// qureg release or state reset or state read message handling
			SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_QREG_H, m_qr_h)
		}
		break;

		case QASM_MSG_ID_QREG_ST_SET: {
			// qureg state set message handling
			SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_QREG_H, m_qr_h)

			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_QREG_STIDX)) {
				SAFE_MSG_GET_PARAM_AS_STATE_INDEX(QASM_MSG_PARAM_TAG_QREG_STIDX, m_st_idx)
			}

			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_QREG_STVALS)) {
				SAFE_MSG_GET_PARAM_AS_STATE_ARRAY(QASM_MSG_PARAM_TAG_QREG_STVALS, m_st_array)
			}
		}
		break;

		case QASM_MSG_ID_QREG_ST_MEASURE: {
			// qureg measurement message handling
			SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_QREG_H, m_qr_h)

			m_q_idx = 0;
			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_QREG_MQIDX)) {
				// measurement qureg starting qubit passed as argument (optional)
				SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_QREG_MQIDX, m_q_idx)
			}

			m_q_len = -1; // whole qureg
			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_QREG_MQLEN)) {
				// measurement qureg length passed as argument (optional)
				SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_QREG_MQLEN, m_q_len)
			}

			m_rand = true;
			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_QREG_MRAND)) {
				// measurement random flag passed as argument (optional)
				SAFE_MSG_GET_PARAM_AS_BOOL(QASM_MSG_PARAM_TAG_QREG_MRAND, m_rand)
			}

			m_coll = true;
			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_QREG_MCOLL)) {
				// measurement collapse state flag passed as argument (optional)
				SAFE_MSG_GET_PARAM_AS_BOOL(QASM_MSG_PARAM_TAG_QREG_MCOLL, m_coll)
			}
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			// qureg state transformation message handling
			SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_QREG_H, m_qr_h)
			SAFE_MSG_GET_PARAM_AS_FTYPE(QASM_MSG_PARAM_TAG_F_TYPE, m_ftype)
			SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_F_SIZE, m_fsize)
			SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_F_REP, m_frep)
			SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_F_LSQ, m_flsq)

			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_F_CRANGE)) {
				// function control index range passed as argument (optional)
				SAFE_MSG_GET_PARAM_AS_INDEX_RANGE(QASM_MSG_PARAM_TAG_F_CRANGE, m_fcrng)
			}
			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_F_TRANGE)) {
				// function target index range passed as argument (optional)
				SAFE_MSG_GET_PARAM_AS_INDEX_RANGE(QASM_MSG_PARAM_TAG_F_TRANGE, m_ftrng)
			}
			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_F_UTYPE)) {
				// function-U type passed as argument (optional)
				SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_F_UTYPE, m_futype)
			}

			if (msg->check_param_valueByTag(QASM_MSG_PARAM_TAG_F_ARGS)) {
				// function args special handling:
				// -> taken as-is for 1-qubit case
				// -> target function-U extraction for controlled case
				if (QASM_F_TYPE_IS_GATE_1QUBIT(m_ftype)) {
					// 1-qubit function - args as passed
					SAFE_MSG_GET_PARAM_AS_FARGS(QASM_MSG_PARAM_TAG_F_ARGS, m_fargs)
				}
				else if (QASM_F_TYPE_IS_GATE_2QUBIT(m_ftype)) {
					// 2-qubit function - check for controlled-U case
					if (m_ftype == QASM_F_TYPE_Q2_CU) {
						// 1-qubit target function - fu-args as passed
						SAFE_MSG_GET_PARAM_AS_FARGS(QASM_MSG_PARAM_TAG_F_ARGS, m_fuargs)
					}
					else {
						// 2-qubit function - no args
						;
					}
				}
				else if (QASM_F_TYPE_IS_GATE_NQUBIT(m_ftype)) {
					// n-qubit function - check function-U type
					if (QASM_F_TYPE_IS_GATE_1QUBIT(m_futype)) {
						// 1-qubit target function - fu-args as passed
						SAFE_MSG_GET_PARAM_AS_FARGS(QASM_MSG_PARAM_TAG_F_ARGS, m_fuargs)
					}
					else {
						// 2-qubit target function - set control & target fu-ranges and no args
						SAFE_MSG_GET_PARAM_AS_FPARAMS(QASM_MSG_PARAM_TAG_F_ARGS, m_fargs, m_fucrng, m_futrng, m_fuargs)
					}
				}
			}

			// final semantic check
			SAFE_TRASFORMATION_PARAMS_CHECK()
		}
		break;

		// --------------------

		default: {
			// error case
			cerr << "qSim_qinstruction - unhandled qasm message type " << msg->get_id() << "!!" << endl;
		}
	}
}

qSim_qinstruction_core::~qSim_qinstruction_core() {
	// nothing to do...
}

// -------------------------------------

// other constructors - diagnostics
qSim_qinstruction_core::qSim_qinstruction_core(QASM_MSG_ID_TYPE type, int qr_h, unsigned st_idx) :
		qSim_qinstruction_base (type) {
	// qureg allocate, release, reset, set (pure state), peek
	m_type = type;
	switch (m_type) {
		case QASM_MSG_ID_QREG_ALLOCATE: {
			m_qn = qr_h;
		}
		break;

		case QASM_MSG_ID_QREG_RELEASE:
		case QASM_MSG_ID_QREG_ST_RESET:
		case QASM_MSG_ID_QREG_ST_SET:
		case QASM_MSG_ID_QREG_ST_PEEK: {
			m_qr_h = qr_h;
			m_st_idx = st_idx;
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction constructor for qreg allocate/reset/set/peek - unhandled qasm message type "
				 << m_type << "!!" << endl;
			m_qn = 0;
			m_qr_h = 0;
		}
	}

	m_q_idx = 0;
	m_q_len = 0;
	m_rand = false;
	m_coll = false;
	m_ftype = QASM_F_TYPE_NULL;
	m_fsize = 0;
	m_frep = 0;
	m_flsq = 0;
	m_futype = QASM_F_TYPE_NULL;
	m_valid = true;
}

qSim_qinstruction_core::qSim_qinstruction_core(QASM_MSG_ID_TYPE type, int qr_h, QREG_ST_VAL_ARRAY_TYPE st_array) :
		qSim_qinstruction_base (type) {
	// qureg set (arbitrary state)
	m_type = type;
	switch (m_type) {
		case QASM_MSG_ID_QREG_ST_SET:
		case QASM_MSG_ID_QREG_ST_PEEK: {
			m_qr_h = qr_h;
			m_st_array = st_array;
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction constructor for qreg allocate/reset/set/peek - unhandled qasm message type "
				 << m_type << "!!" << endl;
			m_qn = 0;
			m_qr_h = 0;
		}
	}

	m_q_idx = 0;
	m_q_len = 0;
	m_st_idx = 0;
	m_rand = false;
	m_coll = false;
	m_ftype = QASM_F_TYPE_NULL;
	m_fsize = 0;
	m_frep = 0;
	m_flsq = 0;
	m_futype = QASM_F_TYPE_NULL;
	m_valid = true;
}

qSim_qinstruction_core::qSim_qinstruction_core(QASM_MSG_ID_TYPE type, int qr_h,
						 int q_idx, int q_len, bool rand, bool coll) : qSim_qinstruction_base (type) {
	// qureg measure
	m_type = type;
	switch (m_type) {
		case QASM_MSG_ID_QREG_ST_MEASURE: {
			m_qr_h = qr_h;
			m_q_idx = q_idx;
			m_q_len = q_len;
			m_rand = rand;
			m_coll = coll;
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction constructor for qreg measurement - unhandled qasm message type "
				 << m_type << "!!" << endl;
			m_qr_h = 0;
			m_q_idx = 0;
			m_q_len = 0;
			m_rand = false;
			m_coll = false;
		}
	}

	m_qn = 0;
	m_st_idx = 0;
	m_st_array = QREG_ST_VAL_ARRAY_TYPE();
	m_ftype = QASM_F_TYPE_NULL;
	m_fsize = 0;
	m_frep = 0;
	m_flsq = 0;
	m_futype = QASM_F_TYPE_NULL;
	m_valid = true;
}

qSim_qinstruction_core::qSim_qinstruction_core(QASM_MSG_ID_TYPE type, int qr_h, QASM_F_TYPE ftype,
		                                       int fsize, int frep, int flsq,
							                   QREG_F_INDEX_RANGE_TYPE fcrng, QREG_F_INDEX_RANGE_TYPE ftrng,
											   QREG_F_ARGS_TYPE fargs, int futype,
											   QREG_F_INDEX_RANGE_TYPE fucrng, QREG_F_INDEX_RANGE_TYPE futrng,
											   QREG_F_ARGS_TYPE fuargs) : qSim_qinstruction_base (type) {
	// qureg state transformation
	m_qn = 0;
	m_st_idx = 0;
	m_st_array = QREG_ST_VAL_ARRAY_TYPE();
	m_q_idx = 0;
	m_q_len = 0;
	m_rand = false;
	m_coll = false;
	m_valid = true;
	m_type = type;
	switch (m_type) {
		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			// set attributes
			m_qr_h = qr_h;
			m_ftype = ftype;
			m_fsize = fsize;
			m_frep = frep;
			m_flsq = flsq;
			m_fcrng = fcrng;
			m_ftrng = ftrng;
			m_futype = futype;

			if (QASM_F_TYPE_IS_GATE_1QUBIT(m_ftype)) {
				// 1-qubit function - args as passed
				m_fargs = fargs;
			}
			else if (QASM_F_TYPE_IS_GATE_2QUBIT(m_ftype)) {
				// 2-qubit function - check for controlled-U case
				if (ftype == QASM_F_TYPE_Q2_CU) {
					// 1-qubit target function - fu-args as passed
					m_fuargs = fuargs;
				}
				else {
					// 2-qubit function - no args
					;
				}
			}
			else if (QASM_F_TYPE_IS_GATE_NQUBIT(m_ftype)) {
				// n-qubit function - check function-U type
				if (QASM_F_TYPE_IS_GATE_1QUBIT(m_futype)) {
					// 1-qubit target function - fu-args as passed
					m_fuargs = fuargs;
				}
				else {
					// 2-qubit target function - set contrl & target fu-ranges and no args
					m_fucrng = fucrng;
					m_futrng = futrng;
				}
			}

			// semantic check
			SAFE_TRASFORMATION_PARAMS_CHECK()
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction constructor for qreg measurement - unhandled qasm message type "
				 << m_type << "!!" << endl;
			m_qr_h = 0;
			m_ftype = QASM_F_TYPE_NULL;
			m_fsize = 0;
			m_frep = 0;
			m_flsq = 0;
			m_futype = QASM_F_TYPE_NULL;
			m_valid = false;
		}
	}
}

// ---------------------------------

// internal methods for transformation parameters semantic checks
bool qSim_qinstruction_core::check_params_1q() {
	// semantic checks for 1-qubit transformation function
	bool res = true;

	// check function type within 1-qubit range
	SAFE_CHECK_PARAM_VALUE(((m_ftype >= QASM_F_TYPE_Q1_I) && (m_ftype <= QASM_F_TYPE_Q1_Rz)), res,
						   "check_params_transformation_function_1q - illegal function type value", m_ftype)

	// check LSQ index > 0
	SAFE_CHECK_PARAM_VALUE((m_flsq >= 0), res,
						   "check_params_transformation_function_1q - illegal function LSQ value", m_flsq)

	// check function repetitions > 0
	SAFE_CHECK_PARAM_VALUE((m_frep >= 1), res,
						   "check_params_transformation_function_1q - illegal function repetitions value", m_frep)

	// check function size == 2
	SAFE_CHECK_PARAM_VALUE((m_fsize == 2), res,
						   "check_params_transformation_function_1q - illegal function size value", m_fsize)

	return res;
}

bool qSim_qinstruction_core::check_params_2q() {
	// semantic checks for 2-qubit transformation function
	bool res = true;

	// check function type within 2-qubit range
	SAFE_CHECK_PARAM_VALUE(((m_ftype >= QASM_F_TYPE_Q2_CU) && (m_ftype <= QASM_F_TYPE_Q2_CZ)), res,
						   "check_params_transformation_function_2q - illegal function type value", m_ftype)

	// check LSQ index > 0
	SAFE_CHECK_PARAM_VALUE((m_flsq >= 0), res,
						   "check_params_transformation_function_2q - illegal function LSQ value", m_flsq)

	// check function repetitions > 0
	SAFE_CHECK_PARAM_VALUE((m_frep >= 1), res,
						   "check_params_transformation_function_2q - illegal function repetitions value", m_frep)

	// check function size == 4 or 2*fusize for C-U case
	SAFE_CHECK_PARAM_VALUE((m_fsize == 4), res,
						   "check_params_transformation_function_2q - illegal function size value", m_fsize)

//	// check function-U type within 1-qubit range
//    if (m_futype != QASM_F_TYPE_NULL) {
//    	SAFE_CHECK_PARAM_VALUE(((m_futype >= QASM_F_TYPE_Q1_I) && (m_futype <= QASM_F_TYPE_Q1_Rz)), res,
//    						   "check_params_transformation_function_2q - illegal function-U type value", m_futype)
//    }

	return res;
}

bool qSim_qinstruction_core::check_params_nq() {
	// semantic checks for n-qubit transformation function
	bool res = true;

	// check function-U type within 1-qubit + 2-qubit range
	SAFE_CHECK_PARAM_VALUE(((m_futype >= QASM_F_TYPE_Q1_I) && (m_futype <= QASM_F_TYPE_Q2_CZ)), res,
						   "check_params_transformation_function_nq - illegal function-U type value", m_futype)

	// check LSQ index > 0
	SAFE_CHECK_PARAM_VALUE((m_flsq >= 0), res,
						   "check_params_transformation_function_nq - illegal function LSQ value", m_flsq)

	// check function repetitions > 0
	SAFE_CHECK_PARAM_VALUE((m_frep >= 1), res,
						   "check_params_transformation_function_nq - illegal function repetitions value", m_frep)

	// check function target range - same size of function-U
	int trng_span = m_ftrng.m_stop - m_ftrng.m_start + 1;
	int fun;
	if (QASM_F_TYPE_IS_GATE_1QUBIT(m_futype))
		fun = 1;
	else
		fun = 2;
	SAFE_CHECK_PARAM_VALUE((trng_span == fun), res,
						   "check_params_transformation_function_nq - target range not consistent with function-U size", m_ftrng.to_string())

//	// check function-U type within 1-qubit range
//    if (m_futype != QASM_F_TYPE_NULL) {
//    	SAFE_CHECK_PARAM_VALUE(((m_futype >= QASM_F_TYPE_Q1_I) && (m_futype <= QASM_F_TYPE_Q1_Rz)), res,
//    						   "check_params_transformation_function_2q - illegal function-U type value", m_futype)
//    }

	return res;
}

// ---------------------------------

// helper method for getting function form from control & target ranges
int qSim_qinstruction_core::ctrange_2_form(QREG_F_INDEX_RANGE_TYPE fcrng, QREG_F_INDEX_RANGE_TYPE ftrng) {
	int fform = QASM_F_FORM_NULL;
	if (!fcrng.is_empty() && !ftrng.is_empty()) {
		if (fcrng.m_start > ftrng.m_stop)
			fform = QASM_F_FORM_DIRECT;
		else
			fform = QASM_F_FORM_INVERSE;
	}
	return fform;
}

// ---------------------------------

// parameters access helper methods
bool qSim_qinstruction_core::get_msg_param_value_as_fparams(qSim_qasm_message* msg, std::string par_name, QREG_F_ARGS_TYPE* fargs,
												            QREG_F_INDEX_RANGE_TYPE* fucrng, QREG_F_INDEX_RANGE_TYPE* futrng,
													        QREG_F_ARGS_TYPE* fuargs) {
	// read given param string as function parameters and catch exceptions
	bool res = true;
	try {
		std::string str_val = msg->get_param_valueByTag(par_name);
		res = qSim_qinstruction_core::fuparams_from_string(str_val, fargs, fucrng, futrng, fuargs);
	} catch (const std::exception& e) {
		cerr <<"qSim_qinstruction - error reading param: " << par_name << " as <function args>!!" << endl;
		cerr << e.what() << endl;
		res = false;
	}
	return res;
}

bool qSim_qinstruction_core::fuparams_from_string(std::string fargs_str, QREG_F_ARGS_TYPE* fargs,
												  QREG_F_INDEX_RANGE_TYPE* fucrng, QREG_F_INDEX_RANGE_TYPE* futrng,
												  QREG_F_ARGS_TYPE* fuargs) {
	// setup function arg list - f + fu params extraction implementation

	// format:
	// - str_arg[0]: function-U c-range
	// - str_arg[1]: function-U t-range
	// - str_arg[2]: function-U args (if needed - optional)
	//
	// note: fargs set to <empty>!!

	// extract content as argument array
	QREG_F_ARGS_TYPE fargs_aux;
	bool res = qSim_qinstruction_base::fargs_from_string(fargs_str, &fargs_aux);

	// check array content
	if (res && (fargs_aux.size() >= 2)) {
		// expected arguments found - extract values
		QREG_F_ARG_TYPE crng_val = fargs_aux[0];
		if (crng_val.m_type != crng_val.RANGE) {
			cerr << "fuparams_from_string - wrong control range argument value [" << crng_val.to_string() << "]!!!" << endl;
			return false;
		}
		*fucrng = crng_val.m_rng;

		QREG_F_ARG_TYPE trng_val = fargs_aux[1];
		if (trng_val.m_type != trng_val.RANGE) {
			cerr << "fuparams_from_string - wrong target range argument value [" << trng_val.to_string() << "]!!!" << endl;
			return false;
		}
		*futrng = trng_val.m_rng;

		if (fargs_aux.size() == 3) {
			QREG_F_ARG_TYPE fuargs_val = fargs_aux[2];
			if (fuargs_val.m_type != fuargs_val.DOUBLE) {
				cerr << "fuparams_from_string - wrong fuarg argument value [" << fuargs_val.to_string() << "]!!!" << endl;
				return false;
			}
			fuargs->clear();
			fuargs->push_back(fuargs_val);
		}

		fargs->clear();
	}
	return true;
}

// ---------------------------------

void qSim_qinstruction_core::dump() {
	cout << "*** qSim_qinstruction_core dump ***" << endl;
	cout << "m_type: " << m_type << endl;
	switch (m_type) {
		case QASM_MSG_ID_QREG_ALLOCATE: {
			cout << "m_qn: " << m_qn << endl;
		}
		break;

		case QASM_MSG_ID_QREG_RELEASE:
		case QASM_MSG_ID_QREG_ST_RESET:
		case QASM_MSG_ID_QREG_ST_PEEK: {
			cout << "m_qr_h: " << m_qr_h << endl;
		}
		break;

		case QASM_MSG_ID_QREG_ST_SET: {
			cout << "m_qr_h: " << m_qr_h << endl;
			cout << "m_st_array.size: " << m_st_array.size()
			     << " str: " << state_value_to_string(m_st_array) << endl;
		}
		break;

		case QASM_MSG_ID_QREG_ST_MEASURE: {
			cout << "m_qr_h: " << m_qr_h << endl;
			cout << "m_q_idx: " << m_q_idx << endl;
			cout << "m_q_len: " << m_q_len << endl;
			cout << "m_rand: " << m_rand << endl;
			cout << "m_coll: " << m_coll << endl;
		}
		break;

		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			cout << "m_qr_h: " << m_qr_h << endl;
			cout << "m_ftype: " << m_ftype << endl;
			cout << "m_fsize: " << m_fsize << endl;
			cout << "m_frep: " << m_frep << endl;
			cout << "m_flsq: " << m_flsq << endl;
			cout << "m_fcrng: " << m_fcrng.to_string() << endl;
			cout << "m_ftrng: " << m_ftrng.to_string() << endl;
			cout << "m_fargs.size: " << m_fargs.size()
				 << " str: " << fargs_to_string(m_fargs) << endl;
			if (QASM_F_TYPE_IS_GATE_NQUBIT(m_ftype) || (m_ftype == QASM_F_TYPE_Q2_CU)) {
				cout << "m_futype: " << m_futype << endl;
				cout << "m_fucrng: " << m_fucrng.to_string() << endl;
				cout << "m_futrng: " << m_futrng.to_string() << endl;
				cout << "m_fuargs.size: " << m_fuargs.size()
					 << " str: " << fargs_to_string(m_fuargs) << endl;
			}
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction_core - unhandled qasm message type: "
				 << m_type << "!!" << endl;
		}
	}
	cout << endl;
}

