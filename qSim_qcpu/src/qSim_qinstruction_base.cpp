/*
 * qSim_qinstruction_base.cpp
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
 *  Created on: Dec 7, 2022
 *      Author: gianni
 *
 * Q-Reg instruction base virtual class, providing virtual methods for specific classes
 * for core and block instructions.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Dec-2022   Module creation.
 *
 *  --------------------------------------------------------------------------
 */


#include <iostream>
#include <complex>
using namespace std;

#include <string.h>

#include "qSim_qinstruction_base.h"


/////////////////////////////////////////////////////////////////////////
// qreg function arg class

qSim_qreg_function_arg::qSim_qreg_function_arg() {
	m_type = INT;
	m_i = 0; // dummy value
	m_d = 0.0; // dummy value
	m_rng = QREG_F_INDEX_RANGE_TYPE(); // dummy value
}

qSim_qreg_function_arg::qSim_qreg_function_arg(int val) {
	m_type = INT;
	m_i = val;
	m_d = 0.0; // dummy value
	m_rng = QREG_F_INDEX_RANGE_TYPE(); // dummy value
}

qSim_qreg_function_arg::qSim_qreg_function_arg(double val) {
	m_type = DOUBLE;
	m_i = 0; // dummy value
	m_d = val;
	m_rng = QREG_F_INDEX_RANGE_TYPE(); // dummy value
}

qSim_qreg_function_arg::qSim_qreg_function_arg(QREG_F_INDEX_RANGE_TYPE val) {
	m_type = RANGE;
	m_i = 0; // dummy value
	m_d = 0.0; // dummy value
	m_rng = val;
}

// -----------------------------------

// (1) function arg values
// coding format: ASCII string with
//    <arg-x> = <value>"|"<type> and <type> as "I" for integers or "D" for doubles or "R" for ranges

#define QREG_F_ARGS_VALTYPE_SEP std::string("|")
const std::string QREG_F_ARGS_VALTYPE_TYPE_LABELS[] = {"I", "D", "R"};

std::string qSim_qreg_function_arg::to_string() {
	std::string farg_str = "";
	if (m_type == INT)
		farg_str = std::to_string(m_i);
	else if (m_type == DOUBLE)
		farg_str = std::to_string(m_d);
	else if (m_type == RANGE)
		farg_str = m_rng.to_string();
	farg_str += "|" + QREG_F_ARGS_VALTYPE_TYPE_LABELS[m_type];
	return farg_str;
}

bool qSim_qreg_function_arg::from_string(std::string farg_str) {
	int idx = farg_str.find(QREG_F_ARGS_VALTYPE_SEP); // value-type separator mandatory
	if (idx < 1) {
		cerr << "ERROR - qSim_qreg_function_arg::from_string - wrong format in [" << farg_str << "]!!" << endl;
		return false;
	}
	std::string valType = farg_str.substr(idx+1, 1); // I or D or R argument value type
	if (valType == QREG_F_ARGS_VALTYPE_TYPE_LABELS[INT]) {
		int val = stoi(farg_str.substr(0, idx));
//		printf("...I val: %d\n", val);
		m_type = INT;
		m_i = val;
	}
	else if (valType == QREG_F_ARGS_VALTYPE_TYPE_LABELS[DOUBLE]) {
		double val = stof(farg_str.substr(0, idx));
//		printf("...D val: %g\n", val);
		m_type = DOUBLE;
		m_d = val;
	}
	else if (valType == QREG_F_ARGS_VALTYPE_TYPE_LABELS[RANGE]) {
		m_type = RANGE;
		m_rng.from_string(farg_str);
	}
	else {
		cerr << "qSim_qreg_function_arg::from_string - unhandled val type found [" << valType << "]!!!" << endl;
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////
// qreg function arg index range class

qSim_qasm_index_range::qSim_qasm_index_range() {
	m_start = m_stop = QREG_F_INDEX_RANGE_TYPE_NULL;
}

qSim_qasm_index_range::qSim_qasm_index_range(int start, int stop) {
	// perform sanity checks on passed arguments
	if ((start>=QREG_F_INDEX_RANGE_TYPE_NULL) && (stop>=QREG_F_INDEX_RANGE_TYPE_NULL)) {
		if (((start==QREG_F_INDEX_RANGE_TYPE_NULL) && (stop==QREG_F_INDEX_RANGE_TYPE_NULL)) ||
				((start!=QREG_F_INDEX_RANGE_TYPE_NULL) && (stop!=QREG_F_INDEX_RANGE_TYPE_NULL))) {
			m_start = start;
			m_stop = stop;
		}
		else {
			cerr << "qSim_qasm_index_range - inconsistent range values [start: " << start << ", stop: " << stop << "]!! - Null values applied";
			m_start = m_stop = QREG_F_INDEX_RANGE_TYPE_NULL;
		}
	}
	else {
		cerr << "qSim_qasm_index_range - illegal range values [start: " << start << ", stop: " << stop << "]!! - Null values applied";
		m_start = m_stop = QREG_F_INDEX_RANGE_TYPE_NULL;
	}
}

bool qSim_qasm_index_range::is_empty() {
	return ((m_start==QREG_F_INDEX_RANGE_TYPE_NULL) &&
		    (m_stop==QREG_F_INDEX_RANGE_TYPE_NULL));
}

// string <-> integer index range back & forth conversions
// => index range string format: (start_idx, stop_idx)

std::string qSim_qasm_index_range::to_string() {
	return "(" + std::to_string(m_start) + ", " + std::to_string(m_stop) + ")";
}

bool qSim_qasm_index_range::from_string(std::string rng_str) {
	//
	std::string strBuf = rng_str;
	if (strBuf.size() > 0) {
		int idx0 = strBuf.find("(");
		int idx1 = strBuf.find(",");
		int idx2 = strBuf.find(")");
		if ((strBuf.size() < 2) || ((idx0 < 0) || (idx2 < 0))) {
			// wrong range string
			cerr << "index_range_string_to_value error - wrong string format in [" << rng_str << "]!!" << endl;
			return false;
		}
		if (strBuf.size() == 2) {
			// empty range
			m_start = QREG_F_INDEX_RANGE_TYPE_NULL;
			m_stop = QREG_F_INDEX_RANGE_TYPE_NULL;
		}
		else {
			// defined range
			m_start = stoi(strBuf.substr(idx0+1, idx1-idx0-1));
			m_stop = stoi(strBuf.substr(idx1+2, idx2-idx1));
		}
	}
	else {
		cerr << "qSim_qasm_index_range::from_string - empty string!!" << endl;
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////
// qreg instruction base class constructor and destructor

qSim_qinstruction_base::qSim_qinstruction_base(QREG_INSTR_TYPE type)
{
	m_type = type;
	m_valid = false;
}

qSim_qinstruction_base::~qSim_qinstruction_base()
{
}

// ---------------------------------

// instruction class (core or block)
bool qSim_qinstruction_base::is_core(qSim_qasm_message* msg) {
	// core instruction message check
	// -> it must be a qureg state control or a 1Q-2Q-nQ transformation
	if (msg->is_instruction_message() && (msg->get_id() != QASM_MSG_ID_QREG_ST_TRANSFORM))
		return true;
	if (msg->is_instruction_message() && (msg->get_id() == QASM_MSG_ID_QREG_ST_TRANSFORM)) {
		QASM_F_TYPE ftype;
		if (!qSim_qinstruction_base::get_msg_param_value_as_ftype(msg, QASM_MSG_PARAM_TAG_F_TYPE, &ftype)) {
			cerr << "qSim_qcpu::dispatch_message - no function type parameter in instruction message!!" << endl;
			return false;
		}
		else
			return (QASM_F_TYPE_IS_GATE_1QUBIT(ftype) ||
					QASM_F_TYPE_IS_GATE_2QUBIT(ftype) ||
					QASM_F_TYPE_IS_GATE_NQUBIT(ftype));
	}
	else
		return false;
}

bool qSim_qinstruction_base::is_block(qSim_qasm_message* msg) {
	// block instruction message check
	// -> it must be a block transformation
	if (msg->is_instruction_message() && (msg->get_id() == QASM_MSG_ID_QREG_ST_TRANSFORM)) {
		QASM_F_TYPE ftype;
		if (!qSim_qinstruction_base::get_msg_param_value_as_ftype(msg, QASM_MSG_PARAM_TAG_F_TYPE, &ftype)) {
			cerr << "qSim_qcpu::dispatch_message - no function type parameter in instruction message!!" << endl;
			return false;
		}
		else
			return (QASM_F_TYPE_IS_FUNC_BLOCK(ftype));
	}
	else
		return false;
}

/////////////////////////////////////////////////////////////////////////
// qreg instruction base class helper methods

// ---------------------------------

// coding format: ASCII string with "[", "]" as start/stop tags and with "," as field separators
//   <fargs_string> =  "[<arg-1>,<arg-2, ... <arg-n>]"
//
// with <arg-x> = <value>"|"<type> and
// - <value> as <number> for scalar of (<number>, <number>) for range
// - <type> as "I" for integers or "D" for doubles, or "R" for ranges

#define QREG_TRASF_ARGS_TAG_FIRST   std::string("[")
#define QREG_TRASF_ARGS_TAG_LAST    std::string("]")
#define QREG_TRASF_ARGS_TAG_SEP     std::string(",")
#define QREG_TRASF_ARGS_TAG_NULL    std::string("null")
#define QREG_TRASF_ARGS_TAG_RANGE_FIRST std::string("(")
#define QREG_TRASF_ARGS_TAG_RANGE_LAST  std::string(")")

//const std::string QREG_TRASF_ARGS_ARGTYPE_LABELS[] = {"G", "C"};
const std::string QREG_TRASF_ARGS_FCFORM_LABELS[]  = {"D", "I", "R"};

template <typename T>
std::string qSim_qinstruction_base::to_string_with_precision(const T a_value, const int n) {
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

std::string qSim_qinstruction_base::trim_string(std::string str) {
	const char* typeOfWhitespaces = " \t\n\r\f\v";
	str.erase(str.find_last_not_of(typeOfWhitespaces) + 1);
	str.erase(0,str.find_first_not_of(typeOfWhitespaces));
	return str;
}

// function args from/to string conversions
std::string qSim_qinstruction_base::fargs_to_string(QREG_F_ARGS_TYPE fargs) {
	// convert given function args to string format
	std::string fargs_str = QREG_TRASF_ARGS_TAG_FIRST;
	for (unsigned int i=0; i<fargs.size(); i++) {
		cout <<"..farg #" << i << " - type: " << fargs[i].m_type << " - str: " << fargs[i].to_string() << endl;
		fargs_str += fargs[i].to_string();
		if (i < fargs.size()-1)
			fargs_str += QREG_TRASF_ARGS_TAG_SEP; // add ending comma separator not for last one!
	}
	fargs_str += QREG_TRASF_ARGS_TAG_LAST;
//			printf("general function..fargs_str:  %s\n", fargs_str.c_str());
	return fargs_str;
}

bool qSim_qinstruction_base::fargs_from_string(std::string fargs_str, QREG_F_ARGS_TYPE* fargs) {
	// setup function arg list - basic implementation

	// check data content
	if ((fargs_str.size() > 0) && (trim_string(fargs_str) != QREG_TRASF_ARGS_TAG_NULL)) {
		// decode attributes from given argument string - check if for general function or controlled function
		std::string bufStr = std::string(fargs_str); // input buffer cloned not to modify it

		// check opening string tag - mandatory
		int idx = bufStr.find(QREG_TRASF_ARGS_TAG_FIRST);
		if (idx != 0) {
			cerr << "ERROR - qSim_qreg_trasf_args::from_string - missing opening tag in [" << fargs_str << "]!!" << endl;
			return false;
		}
		bufStr.erase(0, idx+1);

		// loop over arguments
		std::string farg_str;
		while (bufStr.size() > 1) {
			// comma separator not present for last argument - handle it
			idx = bufStr.find(QREG_TRASF_ARGS_TAG_SEP);
			if (idx > 0) {
				farg_str = bufStr.substr(0, idx);
				bufStr.erase(0, idx+1);
			}
			else {
				farg_str = bufStr;
				bufStr = "";
			}
//				printf("...bufStr: %s...arg: %s\n", bufStr.c_str(), arg.c_str());
			qSim_qreg_function_arg farg;
			if (farg.from_string(farg_str))
				fargs->push_back(farg);
			else {
				cerr << "fargs_from_string - wrong parameter value found in [" << farg_str << "]!!" << endl;
				return false;
			}
		}
	}
	return true;
}

// ---------------------------------

// string <-> complex array back & forth conversions
// => state array string format: (r1, i1), (r2, i2), ..., (rn, in)

std::string qSim_qinstruction_base::state_value_to_string(QREG_ST_VAL_ARRAY_TYPE q_st) {
	std::string qr_st_str = "";
	for (unsigned int i=0; i<q_st.size(); i++) {
		qr_st_str += "(" + to_string(q_st[i].real()) + ", "
				+ to_string(q_st[i].imag()) + ")";
		if (i < q_st.size()-1)
			qr_st_str += ",";
		qr_st_str += " ";
	}
	return qr_st_str;
}

bool qSim_qinstruction_base::state_string_to_value(std::string qr_st_str, QREG_ST_VAL_ARRAY_TYPE* qr_st) {
	std::string strBuf = qr_st_str;
	qr_st->clear();
	while (strBuf.size() > 0) {
		int idx0 = strBuf.find("(");
		int idx1 = strBuf.find(",");
		int idx2 = strBuf.find(")");
		if ((strBuf.size() < 5) || ((idx0 < 0) || (idx2 < 0))) {
			cerr << "qregSt_string_to_value error - wrong string format in [" << qr_st_str << "]!!" << endl;
			return false;
		}
		else {
			double st_val_r = stof(strBuf.substr(idx0+1, idx1-idx0-1));
			double st_val_i = stof(strBuf.substr(idx1+2, idx2-idx1));
			QREG_ST_VAL_TYPE st_val(st_val_r, st_val_i);
			qr_st->push_back(st_val);
			strBuf.erase(0, idx2+2);
		}
	}
	return true;
}

// ---------------------------------

//// string <-> complex array back & forth conversions
//// => state array string format: (r1, i1), (r2, i2), ..., (rn, in)
////
//// Note: no outer parenthesis!!
//
//std::string qSim_qinstruction_base::index_range_value_to_string(QREG_F_INDEX_RANGE_TYPE idx_rng) {
//	std::string idx_rng_str = "(";
//	if (!idx_rng.is_empty())
//		idx_rng_str += to_string(idx_rng.m_start) + ", " + to_string(idx_rng.m_stop);
//	idx_rng_str += ")";
//	return idx_rng_str;
//}
//
//bool qSim_qinstruction_base::index_range_string_to_value(std::string idx_rng_str, QREG_F_INDEX_RANGE_TYPE* idx_rng) {
//	std::string strBuf = idx_rng_str;
//	if (strBuf.size() > 0) {
//		int idx0 = strBuf.find("(");
//		int idx1 = strBuf.find(",");
//		int idx2 = strBuf.find(")");
//		if ((strBuf.size() < 2) || ((idx0 < 0) || (idx2 < 0))) {
//			// wrong range string
//			cerr << "index_range_string_to_value error - wrong string format in [" << idx_rng_str << "]!!" << endl;
//			return false;
//		}
//		if (strBuf.size() == 2) {
//			// empty range
//			idx_rng->m_start = QREG_F_INDEX_RANGE_TYPE_NULL;
//			idx_rng->m_stop = QREG_F_INDEX_RANGE_TYPE_NULL;
//		}
//		else {
//			// defined range
//			idx_rng->m_start = stoi(strBuf.substr(idx0+1, idx1-idx0-1));
//			idx_rng->m_stop = stoi(strBuf.substr(idx1+2, idx2-idx1));
//		}
//	}
//	else {
//		cerr << "index_range_string_to_value error - empty string!!" << endl;
//		return false;
//	}
//	return true;
//}

// ---------------------------------

// measure index value::string conversion helpers
// => index array string format: [idx1, ... idxn]
//
// Note: no outer parenthesis are optional!

std::string qSim_qinstruction_base::measure_index_value_to_string(QREG_ST_INDEX_ARRAY_TYPE m_vec) {
	std::string m_vec_str = "[";
	for (unsigned int i=0; i<m_vec.size(); i++) {
		m_vec_str += to_string(m_vec[i]);
		if (i < m_vec.size()-1)
			m_vec_str += ", ";
	}
	m_vec_str += "]";
	return m_vec_str;
}

bool qSim_qinstruction_base::measure_index_string_to_value(std::string idx_vec_str, QREG_ST_INDEX_ARRAY_TYPE* m_vec) {
	std::string strBuf = idx_vec_str;
	// remove starting "[" and ending "]" - if present
	int idx0 = strBuf.find("[");
	if (idx0 >= 0)
		strBuf.erase(0, idx0+1);
	int idxn = strBuf.find("]");
	if (idxn >= 0)
		strBuf.erase(idxn, 1);

	// scan remaining string
	m_vec->clear();
	while (strBuf.size() > 0) {
		int idx1 = strBuf.find(",");
		if ((strBuf.size() < 2) && (idx1 > 0)) {
			// wrong range string
			cerr << "measure_index_string_to_value error - wrong string format in [" << idx_vec_str << "]!!" << endl;
			return false;
		}
		else if (idx1 > 0) {
			// first or middle index
//			cout << "... 1st/middle index read...str_val: " << strBuf.substr(0, idx1-1) << endl;
			int idx_val = stoi(strBuf.substr(0, idx1));
			m_vec->push_back(idx_val);
			strBuf.erase(0, idx1+2);
		}
		else {
			// last or single index - remove ending "]" if present
//			cout << "... last/single index read...str_val: " << strBuf << endl;
			int idx_val = stoi(strBuf);
//			cout << "... idx_val: " << idx_val << endl;
			m_vec->push_back(idx_val);
			strBuf.clear();
		}
	}
	return true;
}

// ---------------------------------

bool qSim_qinstruction_base::get_msg_param_value_as_int(qSim_qasm_message* msg, std::string par_name,
												       int* par_val) {
	// read given param string as integer and catch exceptions
	bool res = true;
	try {
		std::string str_val = msg->get_param_valueByTag(par_name);
		*par_val = stoi(str_val);
	} catch (const std::exception& e) {
		cerr <<"qSim_qinstruction - error reading param: " << par_name << " as <int>!!" << endl;
		cerr << e.what() << endl;
		res = false;
	}
	return res;
}

bool qSim_qinstruction_base::get_msg_param_value_as_uint(qSim_qasm_message* msg, std::string par_name,
												         unsigned* par_val) {
	// read given param string as integer and catch exceptions
	bool res = true;
	try {
		std::string str_val = msg->get_param_valueByTag(par_name);
		*par_val = stoul(str_val);
	} catch (const std::exception& e) {
		cerr <<"qSim_qinstruction - error reading param: " << par_name << " as <unsigned int>!!" << endl;
		cerr << e.what() << endl;
		res = false;
	}
	return res;
}

bool qSim_qinstruction_base::get_msg_param_value_as_ftype(qSim_qasm_message* msg, std::string par_name,
												         QASM_F_TYPE* par_val) {
	// read given param string as integer and catch exceptions
	bool res = true;
	try {
		std::string str_val = msg->get_param_valueByTag(par_name);
		*par_val = (QASM_F_TYPE)stoi(str_val);
	} catch (const std::exception& e) {
		cerr <<"qSim_qinstruction - error reading param: " << par_name << " as <function type>!!" << endl;
		cerr << e.what() << endl;
		res = false;
	}
	return res;
}

bool qSim_qinstruction_base::get_msg_param_value_as_state_array(qSim_qasm_message* msg, std::string par_name,
		                                                   QREG_ST_VAL_ARRAY_TYPE* par_val) {
	// read given param string as integer and catch exceptions
	bool res = true;
	try {
		std::string str_val = msg->get_param_valueByTag(par_name);
		res = qSim_qinstruction_base::state_string_to_value(str_val, par_val);
	} catch (const std::exception& e) {
		cerr <<"qSim_qinstruction - error reading param: " << par_name << " as <state array>!!" << endl;
		cerr << e.what() << endl;
		res = false;
	}
	return res;
}

bool qSim_qinstruction_base::get_msg_param_value_as_index_range(qSim_qasm_message* msg, std::string par_name,
		                                                   QREG_F_INDEX_RANGE_TYPE* par_val) {
	// read given param string as integer and catch exceptions
	bool res = true;
	try {
		std::string str_val = msg->get_param_valueByTag(par_name);
//		res = qSim_qinstruction_base::index_range_string_to_value(str_val, par_val);
		res = par_val->from_string(str_val);
	} catch (const std::exception& e) {
		cerr <<"qSim_qinstruction - error reading param: " << par_name << " as <index range>!!" << endl;
		cerr << e.what() << endl;
		res = false;
	}
	return res;
}

bool qSim_qinstruction_base::get_msg_param_value_as_bool(qSim_qasm_message* msg, std::string par_name,
												        bool* par_val) {
	// read given param string as integer and catch exceptions
	bool res = true;
	try {
		std::string str_val = msg->get_param_valueByTag(par_name);
		*par_val = (stoi(str_val) == 1);
	} catch (const std::exception& e) {
		cerr <<"qSim_qinstruction - error reading param: " << par_name << " as <bool>!!" << endl;
		cerr << e.what() << endl;
		res = false;
	}
	return res;
}

bool qSim_qinstruction_base::get_msg_param_value_as_fargs(qSim_qasm_message* msg, std::string par_name,
		                                                  QREG_F_ARGS_TYPE* par_val) {
	// read given param string as integer and catch exceptions
	bool res = true;
	try {
		std::string str_val = msg->get_param_valueByTag(par_name);
		res = qSim_qinstruction_base::fargs_from_string(str_val, par_val);
	} catch (const std::exception& e) {
		cerr <<"qSim_qinstruction - error reading param: " << par_name << " as <function args>!!" << endl;
		cerr << e.what() << endl;
		res = false;
	}
	return res;
}


