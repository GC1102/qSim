/*
 * qSim_qasm.cpp
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
 *  Created on: 5 Apr 2018
 *      Author: Casonato
 *
 * Q-CPU support module defining supported transformation functions:
 * - basic transformation functions (I, X, H, CX and their combination)
 * - extended transformation functions (SWAP, Toffoli, Fredkin, QFT)
 * - custom transformation functions (look-up-table based)
 *
 * Definition of the instruction information handling class, providing:
 * - application level instruction attributes (IDs and parameters)
 * - coding/decoding methods from string arrays used for socket communications.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Apr-2018   Module creation.
 *  1.1   May-2022   Rewritten to limit to defining supported Q-CPU supported
 *                   instruction set functions.
 *  1.2   Nov-2022   Instruction set limitation to 1 and 2 qubit gates.
 *                   Renamed qureg state get method constant.
 *                   Renamed client id and token constants.
 *                   Supported function block - 1-qubit SWAP.
 *                   Code clean-up.
 *  1.3   Feb-2023   Supported qureg state expectation calculation and fixed
 *                   terminology for state probability measure.
 *
 *  --------------------------------------------------------------------------
 */

#include <iostream>
using namespace std;

#include "qSim_qasm.h"


///////////////////////////////////////////////////////////////////////////
//  QASM instruction information handling class
///////////////////////////////////////////////////////////////////////////

qSim_qasm_message::qSim_qasm_message() {
	// set class attributes
	m_id = QASM_MSG_ID_NOPE;
	m_counter = 0;
	m_params = QASM_MSG_PARAMS_TYPE();
}

qSim_qasm_message::qSim_qasm_message(QASM_MSG_COUNTER_TYPE counter,
				    QASM_MSG_ID_TYPE id,
				    QASM_MSG_PARAMS_TYPE params) {
	// set class attributes
	m_counter = counter;
	m_id = id;
	m_params = params;
}

qSim_qasm_message::~qSim_qasm_message() {
	// nothing to do...
}

// ------------------------------------------------------

void qSim_qasm_message::add_param_tagValue(std::string par_tag, std::string par_val) {
	m_params.insert(std::make_pair(par_tag, par_val));
}

// ------------------------------------------------------
// content syntax checking
bool qSim_qasm_message::check_syntax() {
	// check syntax correctness of class data
	bool res = true;
	switch (m_id) {
		case QASM_MSG_ID_REGISTER: {
			// params:
			// (1) identifier = <value>
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_CLIENT_ID) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_CLIENT_ID);
				res = false;
			}
		}
		break;

		case QASM_MSG_ID_UNREGISTER: {
			// params:
			// (1) token = <value>
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_CLIENT_TOKEN) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_CLIENT_TOKEN);
				res = false;
			}
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ALLOCATE: {
			// params:
			// (1) qr_n = <value>
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_QREG_QN) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_QN);
				res = false;
			}
		}
		break;

		case QASM_MSG_ID_QREG_RELEASE: {
			// params:
			// (1) qr_h = <value>
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_QREG_H) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_H);
				res = false;
			}
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ST_RESET: {
			// params:
			// (1) qr_h = <value>
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_QREG_H) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_H);
				res = false;
			}
		}
		break;

		case QASM_MSG_ID_QREG_ST_SET: {
			// params:
			// (1) qr_h = <value>
			// (2) qr_stVal = <value> (optional - full qureg excited state used as default)
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_QREG_H) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_H);
				res = false;
			}
		}
		break;

		case QASM_MSG_ID_QREG_ST_MEASURE: {
			// params:
			// (1) qr_h = <value>
			// (2) qr_mQidx = <value>
			// (3) qr_mQlen = <value>
			// (4) qr_mRand = <value> (optional - random probability used as default)
			// (5) qr_mStColl = <value> (optional - state collapse used as default)
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_QREG_H) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_H);
				res = false;
			}
			else if (m_params.count(QASM_MSG_PARAM_TAG_QREG_MQIDX) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_MQIDX);
				res = false;
			}
			else if (m_params.count(QASM_MSG_PARAM_TAG_QREG_MQLEN) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_MQLEN);
				res = false;
			}
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ST_EXPECT: {
			// params:
			// (1) qr_h = <value>
			// (2) qr_stIdx = <value> (optional - all states used as default)
			// (3) qr_exQidx = <value> (optional - start qubit used as default)
			// (4) qr_exQlen = <value> (optional - qureg length used as default)
			// (5) qr_exObsOp = <value> (optional - computational basis used as default)
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_QREG_H) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_H);
				res = false;
			}
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			// params:
			// (1) qr_h = <value>
			// (2) f_type = <value>
			// (3) f_size = <value>
			// (4) f_rep = <value>
			// (5) f_lsq = <value>
			// (6) f_args = <value> (optional - empty args used as default)
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_QREG_H) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_H);
				res = false;
			}
			else if (m_params.count(QASM_MSG_PARAM_TAG_F_TYPE) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_F_TYPE);
				res = false;
			}

// -> those are checked in the instruction classes!
//			else if (m_params.count(QASM_MSG_PARAM_TAG_F_SIZE) == 0) {
//				log_missing_param_tag(QASM_MSG_PARAM_TAG_F_SIZE);
//				res = false;
//			}
//			else if (m_params.count(QASM_MSG_PARAM_TAG_F_REP) == 0) {
//				log_missing_param_tag(QASM_MSG_PARAM_TAG_F_REP);
//				res = false;
//			}
//			else if (m_params.count(QASM_MSG_PARAM_TAG_F_LSQ) == 0) {
//				log_missing_param_tag(QASM_MSG_PARAM_TAG_F_LSQ);
//				res = false;
//			}
		}
		break;

		// --------------------

		case QASM_MSG_ID_QREG_ST_PEEK: {
			// params:
			// (1) qr_h = <value>
			//
			if (m_params.count(QASM_MSG_PARAM_TAG_QREG_H) == 0) {
				log_missing_param_tag(QASM_MSG_PARAM_TAG_QREG_H);
				res = false;
			}
		}
		break;

		// --------------------

		default: {
			cerr << "qSim_qasm_message::check_sintax - unhandled qasm message type "
				 << m_id << "!!" << endl;
			res = false;
		}
	}
	return res;
}

// ------------------------------------------------------
// encoding/decoding

// coding format: ASCII string with "|" as field separators
//   <encoded_instruction> =  <counter>"|"<id>"|"<params>
//
// with ":" as param tag + value pairs separator
//	 <params> = <par_tag_1>=<par_value_1>:<par_tag_2>=<par_value_2>: ... <par_tag_n>=<par_value_n>:

void qSim_qasm_message::from_char_array(unsigned int len, char* buf) {
	// extract counter, id and parameters from given string
	std::string bufStr = std::string(buf); // input buffer cloned not to modify it

	// get counter
	int idx = bufStr.find(QASM_MSG_FIELD_SEP); // first separator mandatory
	if (idx < 1) {
		cerr << "qSim_qasm_message::from_char_array - wrong format!" << endl;
		return;
	}
	m_counter = stoi(bufStr.substr(0, idx));
	bufStr.erase(0, idx+1);

	// get id
	idx = bufStr.find(QASM_MSG_FIELD_SEP); // second separator only if params are present
	if (idx < 1) {
		cerr << "qSim_qasm_message::from_char_array - wrong format!" << endl;
		return;
	}
	m_id = stoi(bufStr.substr(0, idx));
	bufStr.erase(0, idx+1);

	// get parameters
	m_params.clear();
	while (bufStr.size() > 0) {
		// get initial param separator
		idx = bufStr.find(QASM_MSG_PARAM_SEP);
		if (idx < 0) {
			cerr << "qSim_qasm_message::from_char_array - wrong parameter format!" << endl;
			break;
		}
		std::string par_pair = bufStr.substr(0, idx);

		// get final param separator
		int idx2 = bufStr.find(QASM_MSG_PARVAL_SEP);
		if (idx2 < 0) {
			cerr << "qSim_qasm_message::from_char_array - wrong parameter tag-value format!" << endl;
			break;
		}

		// extract param tag and value
		std::string par_tag = par_pair.substr(0, idx2);
		std::string par_val = par_pair.substr(idx2+1, par_pair.size()-idx2-1);
		m_params.insert(std::make_pair(par_tag, par_val));
		bufStr.erase(0, idx+1);
	}
}

void qSim_qasm_message::to_char_array(unsigned int* len, char** buf) {
	// compact counter, id and params into a string
	// => Note: output buffer allocated in the method

	// convert fields to string and concatenate
	std::string bufStr = to_string(m_counter);
	bufStr += QASM_MSG_FIELD_SEP;
	bufStr += to_string(m_id);
	bufStr += QASM_MSG_FIELD_SEP;

	// handle params encoding and concatenation
	QASM_MSG_PARAMS_TYPE::iterator it;
	for (it=m_params.begin(); it!=m_params.end(); it++)	{
		bufStr += (it->first + QASM_MSG_PARVAL_SEP + it->second);
		bufStr += QASM_MSG_PARAM_SEP;
	}

	// allocate output buffer and store encoded string
	*len = bufStr.size();
	*buf = new char[(*len)+1];
	bufStr.copy(*buf, bufStr.size());
	(*buf)[bufStr.size()] = '\0';
}

// ------------------------------------------------------
// error logging

void qSim_qasm_message::log_missing_param_tag(std::string par_tag) {
	cerr << "qSim_qasm_message::check_sintax - message [" << m_id <<
			"] wrong parameter [" << par_tag << "]" << endl;
}

// ------------------------------------------------------
// diagnostics

void qSim_qasm_message::dump() {
	cout << "*** qSim_qasm_message dump ***" << endl << endl;

	cout << "m_counter: " << m_counter << endl;
	cout << "m_id:      " << m_id << endl;
	cout << "m_params count:" << m_params.size() << endl;
	QASM_MSG_PARAMS_TYPE::iterator it;
	int i = 0;
	for (it=m_params.begin(); it!=m_params.end(); it++) {
	    // print parameters with 100 chars limit (for perfo reasons!!)
	    if (it->second.size() < 100)
	        cout << "  # " << i << "  par_tag: " << it->first << "  par_val: " << it->second << endl;
            else
	        cout << "  # " << i << "  par_tag: " << it->first << "  par_val: " << it->second.substr(0, 100) << "..." << endl;	        
	    i++;
	}
	cout << endl;
	cout << "**********************************" << endl << endl;
}


