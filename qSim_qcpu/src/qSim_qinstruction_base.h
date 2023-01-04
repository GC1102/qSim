/*
 * qSim_qinstruction_base.h
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

#ifndef QSIM_QINSTRUCTION_BASE_H_
#define QSIM_QINSTRUCTION_BASE_H_


#include <string>
#include <vector>
#include <complex>
using namespace std;

#include "qSim_qasm.h"


// -------------------------------------------------
// typedef definitions
// -------------------------------------------------

typedef QASM_MSG_ID_TYPE QREG_INSTR_TYPE;
typedef QASM_F_TYPE QREG_F_TYPE;
typedef std::vector<std::complex<double>> QREG_ST_VAL_ARRAY_TYPE; // CPU convenient type using std::vector
typedef std::complex<double> QREG_ST_VAL_TYPE;
typedef unsigned int QREG_ST_INDEX_TYPE;
typedef std::vector<QREG_ST_INDEX_TYPE> QREG_ST_INDEX_ARRAY_TYPE;

// data type for instruction function control/target index ranges
#define QREG_F_INDEX_RANGE_TYPE_NULL -1  // null index value

struct qSim_qasm_index_range {
	int m_start;
	int m_stop;

	qSim_qasm_index_range();
	qSim_qasm_index_range(int start, int stop);

	bool is_empty();

	std::string to_string();
	bool from_string(std::string rng_str);
};
typedef qSim_qasm_index_range QREG_F_INDEX_RANGE_TYPE;

// data type for instruction function variable arguments
class qSim_qreg_function_arg {
public:
	qSim_qreg_function_arg();
	qSim_qreg_function_arg(int val);
	qSim_qreg_function_arg(double val);
	qSim_qreg_function_arg(QREG_F_INDEX_RANGE_TYPE rng);

	std::string to_string();
	bool from_string(std::string farg_str);

	enum { INT=0, DOUBLE=1 , RANGE=2} m_type;
	int m_i;
	double m_d;
	QREG_F_INDEX_RANGE_TYPE m_rng;
};
typedef qSim_qreg_function_arg QREG_F_ARG_TYPE;
typedef std::vector<qSim_qreg_function_arg> QREG_F_ARGS_TYPE;


// -------------------------------------------------
// base virtual instruction class
// -------------------------------------------------

class qSim_qinstruction_base {

public:
	// class attributes
	QREG_INSTR_TYPE m_type;

	// constructor and destructor
	qSim_qinstruction_base(QREG_INSTR_TYPE type);
	virtual ~qSim_qinstruction_base();

	// setup parameterized arguments - diagnostics
	void add_function_arg(int val);
	void add_function_arg(double val);

	// instruction status accessor
	bool is_valid() { return m_valid; }

	// instruction class (core or block)
	static bool is_core(qSim_qasm_message* msg);
	static bool is_block(qSim_qasm_message* msg);

	// state value::string conversion helpers
	static std::string state_value_to_string(QREG_ST_VAL_ARRAY_TYPE q_st);
	static bool state_string_to_value(std::string qr_st_str, QREG_ST_VAL_ARRAY_TYPE*);

//	// index range value::string conversion helpers
//	static std::string index_range_value_to_string(QREG_F_INDEX_RANGE_TYPE q_st);
//	static bool index_range_string_to_value(std::string qr_st_str, QREG_F_INDEX_RANGE_TYPE*);

	// function args from/to string conversions
	static std::string fargs_to_string(QREG_F_ARGS_TYPE fargs);
	static bool fargs_from_string(std::string fargs_str, QREG_F_ARGS_TYPE*);

	// measure index value::string conversion helpers
	static std::string measure_index_value_to_string(QREG_ST_INDEX_ARRAY_TYPE m_vec);
	static bool measure_index_string_to_value(std::string qr_st_str, QREG_ST_INDEX_ARRAY_TYPE*);

	// diagnostics
	virtual void dump() = 0;

	// parameters access helper methods
	static bool get_msg_param_value_as_int(qSim_qasm_message* msg, std::string par_name, int* par_val);
	static bool get_msg_param_value_as_uint(qSim_qasm_message* msg, std::string par_name, unsigned* par_val);
	static bool get_msg_param_value_as_ftype(qSim_qasm_message* msg, std::string par_name, QASM_F_TYPE* par_val);
	static bool get_msg_param_value_as_state_array(qSim_qasm_message* msg, std::string par_name, QREG_ST_VAL_ARRAY_TYPE* par_val);
	static bool get_msg_param_value_as_index_range(qSim_qasm_message* msg, std::string par_name, QREG_F_INDEX_RANGE_TYPE* par_val);
	static bool get_msg_param_value_as_bool(qSim_qasm_message* msg, std::string par_name, bool* par_val);
	static bool get_msg_param_value_as_fargs(qSim_qasm_message* msg, std::string par_name, QREG_F_ARGS_TYPE* par_val);

protected:

	// internal helper methods
	template <typename T>
	static std::string to_string_with_precision(const T a_value, const int n=6);
	static std::string trim_string(std::string str);

	// instruction content validity flag
	bool m_valid;
};


// ---------------------------------------------------------------
// helper macros

#define SAFE_MSG_GET_PARAM_AS_INT(par_name, int_val) {\
	m_valid = qSim_qinstruction_base::get_msg_param_value_as_int(msg, par_name, &int_val);\
	if (!m_valid)\
		return; \
}

#define SAFE_MSG_GET_PARAM_AS_FTYPE(par_name, ftype_val) {\
	m_valid = qSim_qinstruction_base::get_msg_param_value_as_ftype(msg, par_name, &ftype_val);\
	if (!m_valid)\
		return; \
}

#define SAFE_MSG_GET_PARAM_AS_STATE_INDEX(par_name, uint_val) {\
	m_valid = qSim_qinstruction_base::get_msg_param_value_as_uint(msg, par_name, &uint_val);\
	if (!m_valid)\
		return; \
}

#define SAFE_MSG_GET_PARAM_AS_STATE_ARRAY(par_name, arr_val) {\
	m_valid = qSim_qinstruction_base::get_msg_param_value_as_state_array(msg, par_name, &arr_val);\
	if (!m_valid)\
		return; \
}

#define SAFE_MSG_GET_PARAM_AS_INDEX_RANGE(par_name, idx_val) {\
	m_valid = qSim_qinstruction_base::get_msg_param_value_as_index_range(msg, par_name, &idx_val);\
	if (!m_valid)\
		return; \
}

#define SAFE_MSG_GET_PARAM_AS_BOOL(par_name, bool_val) {\
	m_valid = qSim_qinstruction_base::get_msg_param_value_as_bool(msg, par_name, &bool_val);\
	if (!m_valid)\
		return; \
}

#define SAFE_MSG_GET_PARAM_AS_FARGS(par_name, fargs_val) {\
	m_valid = qSim_qinstruction_base::get_msg_param_value_as_fargs(msg, par_name, &fargs_val);\
	if (!m_valid)\
		return; \
}

// ---------------------------------

#define SAFE_CHECK_PARAM_VALUE(cond, res, err_msg, err_val) {\
	if (res) { \
		res &= cond;\
		if (!res)\
			cerr << "qSim_qinstruction_base - " << err_msg << " [" << err_val << "]!!" << endl;\
	} \
}


#endif /* QSIM_QINSTRUCTION_BASE_H_ */
