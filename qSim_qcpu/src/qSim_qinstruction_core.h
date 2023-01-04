/*
 * qSim_qinstruction_core.h
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

#ifndef QSIM_QINSTRUCTION_CORE_H_
#define QSIM_QINSTRUCTION_CORE_H_


#include "qSim_qinstruction_base.h"


// -------------------------------------------------
// derived instruction core class
// -------------------------------------------------

class qSim_qinstruction_core : public qSim_qinstruction_base {

public:
	// class attributes

//	// general
//	QASM_MSG_ID_TYPE m_type; --> inherited from base class

	// qureg handling related
	int m_qn;
	int m_qr_h;
	unsigned m_st_idx;
	QREG_ST_VAL_ARRAY_TYPE m_st_array;

	// qureg state measure related
	int m_q_idx;
	int m_q_len;
	bool m_rand;
	bool m_coll;

	// transformation function related
	QASM_F_TYPE m_ftype;
	int m_fsize;
	int m_frep;
	int m_flsq;
	QREG_F_INDEX_RANGE_TYPE m_fcrng;
	QREG_F_INDEX_RANGE_TYPE m_ftrng;
	QREG_F_ARGS_TYPE m_fargs;

	// target function-U related- for controlled-U case
	int m_futype;
	QREG_F_INDEX_RANGE_TYPE m_fucrng;
	QREG_F_INDEX_RANGE_TYPE m_futrng;
	QREG_F_ARGS_TYPE m_fuargs;

	// constructor and destructor
	qSim_qinstruction_core(qSim_qasm_message*);
	virtual ~qSim_qinstruction_core();

	// other constructors
	qSim_qinstruction_core(QASM_MSG_ID_TYPE, int qr_h, unsigned st_idx=0); // allocate, reset, set (pure state), peek
	qSim_qinstruction_core(QASM_MSG_ID_TYPE, int qr_h, QREG_ST_VAL_ARRAY_TYPE); // set (arbitrary state)
	qSim_qinstruction_core(QASM_MSG_ID_TYPE, int qr_h, int, int, bool, bool); // measure
	qSim_qinstruction_core(QASM_MSG_ID_TYPE, int qr_h, QASM_F_TYPE ftype, int fsize, int frep, int flsq,
						   QREG_F_INDEX_RANGE_TYPE fcrng=QREG_F_INDEX_RANGE_TYPE(),
						   QREG_F_INDEX_RANGE_TYPE ftrng=QREG_F_INDEX_RANGE_TYPE(),
						   QREG_F_ARGS_TYPE fargs=QREG_F_ARGS_TYPE(),
						   int futype=QASM_F_TYPE_NULL,
						   QREG_F_INDEX_RANGE_TYPE fucrng=QREG_F_INDEX_RANGE_TYPE(),
						   QREG_F_INDEX_RANGE_TYPE futrng=QREG_F_INDEX_RANGE_TYPE(),
						   QREG_F_ARGS_TYPE fuargs=QREG_F_ARGS_TYPE()); // transformation

	// helper methods for getting function form from control & target ranges
	static int ctrange_2_form(QREG_F_INDEX_RANGE_TYPE fcrng, QREG_F_INDEX_RANGE_TYPE ftrng);

	// parameters access helper methods
	static bool get_msg_param_value_as_fparams(qSim_qasm_message* msg, std::string par_name, QREG_F_ARGS_TYPE *fargs,
										       QREG_F_INDEX_RANGE_TYPE* fucrng, QREG_F_INDEX_RANGE_TYPE* futrng,
											   QREG_F_ARGS_TYPE* fuargs);

	// diagnostics
	void dump();

private:

	// internal methods for transformation parameters semantic checks
	bool check_params_1q();
	bool check_params_2q();
	bool check_params_nq();

	// internal method for handling transformation instruction setup
	static bool fuparams_from_string(std::string fargs_str, QREG_F_ARGS_TYPE* fargs,
			                         QREG_F_INDEX_RANGE_TYPE* fucrng, QREG_F_INDEX_RANGE_TYPE* futrng,
							         QREG_F_ARGS_TYPE* fuargs);

};

// ---------------------------------------------------------------
// helper macros

#define SAFE_MSG_GET_PARAM_AS_FPARAMS(par_name, fargs_val, fucrng_val, futrng_val, fuargs_val) {\
	m_valid = qSim_qinstruction_core::get_msg_param_value_as_fparams(msg, par_name, &fargs_val, \
			                                                         &fucrng_val, &futrng_val, &fuargs_val);\
	if (!m_valid)\
		return; \
}


#endif /* QSIM_QINSTRUCTION_CORE_H_ */
