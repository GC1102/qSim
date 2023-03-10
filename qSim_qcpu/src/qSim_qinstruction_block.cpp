/*
 * qSim_qinstruction_block.cpp
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
 *  Created on: Dec 9, 2022
 *      Author: gianni
 *
 * Q-Reg instruction block class, handling qCpu "block" instruction dataset
 * extracted from a QASM message for performing the following qreg function block ij
 * transformations:
 * - n-qubit qureg swap
 * - n-qubit controlled qureg swap
 *
 * Derived class from qSim_qinstruction_base.
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

#include "qSim_qinstruction_block.h"
#include "qSim_qinstruction_core.h"


/////////////////////////////////////////////////////////////////////////
// q-instruction block class


// ------------------------

#define SAFE_TRASFORMATION_PARAMS_CHECK() {\
	if (QASM_F_TYPE_IS_FUNC_BLOCK(m_ftype) || \
		QASM_F_TYPE_IS_FUNC_BLOCK_QML(m_ftype)) \
		m_valid = this->check_params();\
	else {\
		cerr << "qSim_qinstruction_block - unhandled ftype value [" << m_ftype << "]!!";\
		m_valid = false;\
	}\
	if (!m_valid)\
		return; \
}

// ------------------------

// constructor & destructor
qSim_qinstruction_block::qSim_qinstruction_block() :
		qSim_qinstruction_base(0) {
	// empty constructor
	m_type = 0;
	m_qr_h = 0;
	m_ftype = QASM_F_TYPE_NULL;
	m_fsize = 0;
	m_frep = 0;
	m_flsq = 0;
	m_fcrng = QREG_F_INDEX_RANGE_TYPE();
	m_ftrng = QREG_F_INDEX_RANGE_TYPE();
	m_fargs = QREG_F_ARGS_TYPE();
	m_valid = false;
}

qSim_qinstruction_block::qSim_qinstruction_block(qSim_qasm_message* msg) :
		qSim_qinstruction_base(msg->get_id()) {
	// initialise from given message
//	m_type = msg->get_id();
	m_qr_h = 0;
	m_ftype = QASM_F_TYPE_NULL;
	m_fsize = 0;
	m_frep = 0;
	m_flsq = 0;
	m_fcrng = QREG_F_INDEX_RANGE_TYPE();
	m_ftrng = QREG_F_INDEX_RANGE_TYPE();
	m_fargs = QREG_F_ARGS_TYPE();
	m_valid = true; // updated in the switch in case of exceptions
//	cout << "qSim_qinstruction_block...m_type: " << m_type << "  msg_id: " << msg->get_id() << endl;

	switch (m_type) {
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
			SAFE_MSG_GET_PARAM_AS_FARGS(QASM_MSG_PARAM_TAG_F_ARGS, m_fargs)

			// final semantic check
			SAFE_TRASFORMATION_PARAMS_CHECK()
		}
		break;

		// --------------------

		default: {
			// error case
			cerr << "qSim_qinstruction_block - unhandled qasm message type " << msg->get_id() << "!!" << endl;
		}
	}
}

qSim_qinstruction_block::~qSim_qinstruction_block() {
	// nothing to do...
}

// -------------------------------------

// other constructors - diagnostics
qSim_qinstruction_block::qSim_qinstruction_block(QASM_MSG_ID_TYPE type, int qr_h, QASM_F_TYPE ftype, int fsize, int frep, int flsq,
												 QREG_F_INDEX_RANGE_TYPE fcrng, QREG_F_INDEX_RANGE_TYPE ftrng,
												 QREG_F_ARGS_TYPE fargs) : qSim_qinstruction_base (type) {
	// qureg state transformation
	m_valid = true;
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
			m_fargs = fargs;

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
			m_fcrng = QREG_F_INDEX_RANGE_TYPE();
			m_ftrng = QREG_F_INDEX_RANGE_TYPE();
			m_fargs = QREG_F_ARGS_TYPE();
			m_valid = false;
		}
	}
}


// internal methods for transformation parameters semantic checks
bool qSim_qinstruction_block::check_params() {
	// semantic checks
	bool res = true;

	// general checks
	// check LSQ index >= 0
	SAFE_CHECK_PARAM_VALUE((m_flsq >= 0), res,
						   "qSim_qinstruction_block::check_params - illegal function LSQ value", m_flsq)

	// check function repetitions > 0
	SAFE_CHECK_PARAM_VALUE((m_frep >= 1), res,
						   "qSim_qinstruction_block::check_params - illegal function repetitions value", m_frep)

// ...

	// block type specific checks
	if (m_ftype == QASM_FB_TYPE_QN_CSWAP) {
		// check control & target ranges not empty
		SAFE_CHECK_PARAM_VALUE((!m_fcrng.is_empty()), res,
							   "qSim_qinstruction_block::check_params - control range cannot be empty", "")

		SAFE_CHECK_PARAM_VALUE((!m_ftrng.is_empty()), res,
							   "qSim_qinstruction_block::check_params - target range cannot be empty", "")
	}
	// ...

	return res;
}

// -------------------------------------
// -------------------------------------
// function block decomposition into core instructions

void qSim_qinstruction_block::unwrap_block_swap_q1(std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose) {
	// swap function block decomposition in sequence of alternate CX core transformations
	// 2-qubits case
	if (verbose)
		cout << "SWAP-1Q - unwrap_block_swap_q1..." << endl;

	// 2-qubits CX core instructions
	int fsize = 4;
	QREG_F_INDEX_RANGE_TYPE fcrng_d(1, 1);
	QREG_F_INDEX_RANGE_TYPE ftrng_d(0, 0);
	QREG_F_INDEX_RANGE_TYPE fcrng_i(0, 0);
	QREG_F_INDEX_RANGE_TYPE ftrng_i(1, 1);

	qinstr_list->clear();
	qSim_qinstruction_core* qr_1 = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
					m_qr_h, QASM_F_TYPE_Q2_CX, fsize, m_frep,
					m_flsq, fcrng_d, ftrng_d); // transformation
	qinstr_list->push_back(qr_1);

	qSim_qinstruction_core* qr_2 = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
					m_qr_h, QASM_F_TYPE_Q2_CX, fsize, m_frep,
					m_flsq, fcrng_i, ftrng_i); // transformation
	qinstr_list->push_back(qr_2);

	qSim_qinstruction_core* qr_3 = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
					m_qr_h, QASM_F_TYPE_Q2_CX, fsize, m_frep,
					m_flsq, fcrng_d, ftrng_d); // transformation
	qinstr_list->push_back(qr_3);
}

// -------------------------------------

void qSim_qinstruction_block::unwrap_block_swap_qn(std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose) {
	// swap function block decomposition in sequence of alternate CX core transformations
	// n-qubits case
	if (verbose)
		cout << "unwrap_block_swap_qn..." << endl;

	// get swap block size in qubits
	int qsw_n = log2(m_fsize)/2;
	int qsw_fsize = powf(2, qsw_n);
	int tot_qsw_loops = powf(qsw_n, 2);
	int qsw_frep = 1;
 	if (verbose)
		cout << "SWAP-nQ - qsw_n: " << qsw_n << "  qsw_fsize: "  << qsw_fsize << "  tot_qsw_loops: " << tot_qsw_loops << endl;

	// setup CX transformation sequence
	for (int i=0; i<tot_qsw_loops; i++) {
		// handle i-th qubit swap

		// set swap start qubit, considering i-th position and
		// applied |a> and |b> qureg start indexes

		// starting SWAP qubit (LSQ)
		int qidx_s = (i % qsw_n) + qsw_n - 1 - (i / qsw_n) + m_flsq;
		if (verbose)
			cout << "qidx_s: " << qidx_s << endl;

		// setup a 1-qubit swap block and unwrap it
		qSim_qinstruction_block qr_sw1q(QASM_MSG_ID_QREG_ST_TRANSFORM, m_qr_h, QASM_FB_TYPE_Q1_SWAP,
										qsw_fsize, qsw_frep, qidx_s);
		std::list<qSim_qinstruction_core*> qsw1q_instr_list;
		qr_sw1q.unwrap_block_swap_q1(&qsw1q_instr_list, verbose=false);
//		cout << "swap list..." << qsw1q_instr_list.size() << endl;

		// add core instructions to overall block list
		std::list<qSim_qinstruction_core*>::iterator it;
		for (it = qsw1q_instr_list.begin(); it != qsw1q_instr_list.end(); ++it){
			qinstr_list->push_back(*it);
		}
	}
}

// -------------------------------------

void qSim_qinstruction_block::unwrap_block_cswap_q1(std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose) {
	// c-swap function block decomposition in sequence of controlled alternated CX core transformations
	// 1-qubits case
	if (verbose)
		cout << "unwrap_block_cswap_q1..." << endl;

	// detect form and inner gaps (if any)
	int fb_form;
	int fb_gapn;
	if (m_fcrng.m_start > m_ftrng.m_stop) {
		 // direct form
		 fb_form = QASM_F_FORM_DIRECT;
		 fb_gapn = m_fcrng.m_start - m_ftrng.m_stop - 1;
	}
	else {
		 // inverse form
		 fb_form = QASM_F_FORM_INVERSE;
		 fb_gapn = m_ftrng.m_start - m_fcrng.m_stop - 1;
	}
	if (verbose)
		cout << "CSWAP-1Q - fb_crng:" << m_fcrng.to_string() << " qfb_trng: " << m_ftrng.to_string() <<
			    "-> fb_form: " << fb_form << " fb_gapn: " << fb_gapn << endl;

	// get swapped qureg size in qubits
	// block qubit size -> total size is 2*n + inner gaps (if any) + 1
	int fb_n = log2(m_fsize);
	int qsw_n = 1;
	if (verbose)
	  cout << "qsw_n: " << qsw_n << " fb_n: " << fb_n << endl;

	// 1-qubit swap instruction block params
	int qsw_fsize = 4;
	int qsw_frep = 1;
	int qsw_flsq;
	if (fb_form == QASM_F_FORM_DIRECT)
		qsw_flsq = m_flsq;
	else
		qsw_flsq = fb_n - fb_gapn - 1 + m_flsq;

	// setup a support 1-qubit swap block
	qSim_qinstruction_block qr_sw1q(QASM_MSG_ID_QREG_ST_TRANSFORM, m_qr_h, QASM_FB_TYPE_Q1_SWAP,
									qsw_fsize, qsw_frep, qsw_flsq);

	// controlled swap params
	int qfc_frep = 1;
	int qfc_n = fb_n;
	int qfc_stn = powf(2, qfc_n);
	if (verbose)
		cout << "qfc_n: " << qfc_n << endl;

	// unwrap to core instructions, create related controlled-U instructions and add to overall list
	std::list<qSim_qinstruction_core*> qsw_instr_list;
	qr_sw1q.unwrap_block_swap_q1(&qsw_instr_list, verbose=false);
//	cout << "swap list..." << qsw_instr_list.size() << endl;

	std::list<qSim_qinstruction_core*>::iterator it;
	for (it = qsw_instr_list.begin(); it != qsw_instr_list.end(); ++it){
		// create a controlled transformation for i-th CX item
		qSim_qinstruction_core* qf_cx = *it;

		// set i-th c-U element size in qubits and states
		QREG_F_INDEX_RANGE_TYPE qfc_crng;
		QREG_F_INDEX_RANGE_TYPE qfc_trng;
		if (fb_form == QASM_F_FORM_DIRECT) {
			qfc_crng = QREG_F_INDEX_RANGE_TYPE(qfc_n-1, qfc_n-1);
			qfc_trng = QREG_F_INDEX_RANGE_TYPE(0, 1);
		}
		else {
			qfc_crng = QREG_F_INDEX_RANGE_TYPE(0, 0);
			qfc_trng = QREG_F_INDEX_RANGE_TYPE(qfc_n-2, qfc_n-1);
		}

		int qfc_flsq = m_flsq; //qf_cx->m_flsq;
		QREG_F_INDEX_RANGE_TYPE qsw_fcrng = qf_cx->m_fcrng;
		QREG_F_INDEX_RANGE_TYPE qsw_ftrng = qf_cx->m_ftrng;

		// setup a C-U function with CX block function as controlled
		qSim_qinstruction_core* qf_ccx = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM, m_qr_h, QASM_F_TYPE_QN_MCSLRU,
																	qfc_stn, qfc_frep, qfc_flsq, qfc_crng, qfc_trng, QREG_F_ARGS_TYPE(),
																	QASM_F_TYPE_Q2_CX, qsw_fcrng, qsw_ftrng);
		if (verbose) {
			cout << "ccx prep done..." << endl;
			qf_ccx->dump();
		}
		qinstr_list->push_back(qf_ccx);
	}
	for (it = qsw_instr_list.begin(); it != qsw_instr_list.end(); ++it){
		delete *it;
	}
}

// -------------------------------------

void qSim_qinstruction_block::unwrap_block_cswap_qn(std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose) {
	// c-swap function block decomposition in sequence of controlled alternated CX core transformations
	// n-qubits case
	if (verbose)
		cout << "unwrap_block_cswap_qn..." << endl;

	// detect form and inner gaps (if any)
	int fb_form;
	int fb_gapn;
	if (m_fcrng.m_start > m_ftrng.m_stop) {
		 // direct form
		 fb_form = QASM_F_FORM_DIRECT;
		 fb_gapn = m_fcrng.m_start - m_ftrng.m_stop - 1;
	}
	else {
		 // inverse form
		 fb_form = QASM_F_FORM_INVERSE;
		 fb_gapn = m_ftrng.m_start - m_fcrng.m_stop - 1;
	}
	if (verbose)
		cout << "CSWAP-nQ - fb_crng:" << m_fcrng.to_string() << " qfb_trng: " << m_ftrng.to_string() <<
			    "-> fb_form: " << fb_form << " fb_gapn: " << fb_gapn << endl;

	// get swapped qureg size in qubits
	// qn: qubit size -> total size is 2*n + inner gaps (if any) + 1
	int fb_n = log2(m_fsize);
	int qcsw_n = (fb_n - fb_gapn - 1)/2;
	int tot_qcsw_loops = powf(qcsw_n, 2.0);
	if (verbose)
	  cout << "fb_n: " << fb_n << " qcsw_n: " << qcsw_n << " tot_qcsw_loops: " << tot_qcsw_loops << endl;

	// n-qubit swap instruction block params
	int qcsw_frep = 1;

	// setup CSWAP-1q transformation sequence
	for (int i=0; i<tot_qcsw_loops; i++) {
		// handle i-th qubit cswap

		// set cswap start qubit, considering i-th position and
		// applied |a> and |b> qureg start indexes

		// starting SWAP qubit (LSQ)
        int qidx_s = (i % qcsw_n) + qcsw_n - 1 - (i / qcsw_n);
		if (verbose)
			cout << "qidx_s: " << qidx_s << endl;

		// control-target ranges - relative values
		QREG_F_INDEX_RANGE_TYPE qcsw_fcrng;
		QREG_F_INDEX_RANGE_TYPE qcsw_ftrng;
		int qcsw_fsize;
		int qcsw_flsq;
		if (fb_form == QASM_F_FORM_DIRECT) {
			 qcsw_fcrng = QREG_F_INDEX_RANGE_TYPE(fb_n-1-qidx_s, fb_n-1-qidx_s);
			 qcsw_ftrng = QREG_F_INDEX_RANGE_TYPE(0, 1);
			 qcsw_fsize = powf(2, (fb_n-qidx_s));
			 qcsw_flsq = qidx_s + m_flsq;
		}
		else {
			 qidx_s += 1;
			 qcsw_fcrng = QREG_F_INDEX_RANGE_TYPE(0, 0);
			 qcsw_ftrng = QREG_F_INDEX_RANGE_TYPE(qidx_s, qidx_s+1);
			 qcsw_fsize = powf(2, (qidx_s+2));
			 qcsw_flsq = m_flsq;
		}
		if (verbose) {
			 cout << "CSWAP qcsw_fcrng: " << qcsw_fcrng.to_string() << " qcsw_ftrng: " << qcsw_ftrng.to_string() <<
			 	 	 " qcsw_fsize: " << qcsw_fsize << " qcsw_flsq: " << qcsw_flsq << endl;
		}

		// setup a support swap block
		qSim_qinstruction_block qr_csw(QASM_MSG_ID_QREG_ST_TRANSFORM, m_qr_h, QASM_FB_TYPE_Q1_CSWAP,
				                       qcsw_fsize, qcsw_frep, qcsw_flsq, qcsw_fcrng, qcsw_ftrng);

		if (verbose) {
			cout << "i: " << i << endl;
			qr_csw.dump();
			cout.flush();
		}

		// unwrap to core instructions and add to overall list
		std::list<qSim_qinstruction_core*> qcsw_instr_list;
		qr_csw.unwrap_block_cswap_q1(&qcsw_instr_list, verbose=false);
		if (verbose) {
			cout << "cswap-q1 unwrap - list size: " << qcsw_instr_list.size() << endl;
		}
		std::list<qSim_qinstruction_core*>::iterator it;
		for (it = qcsw_instr_list.begin(); it != qcsw_instr_list.end(); ++it){
			qinstr_list->push_back(*it);
		}
	}
//	cout << "cswap-qn unwrap done!! - list size: " << qinstr_list->size() << endl;

}

// ---------------------------------

void qSim_qinstruction_block::dump() {
	cout << "*** qSim_qinstruction_block dump ***" << endl;
	cout << "m_type: " << m_type << endl;
	switch (m_type) {
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
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction_block - unhandled qasm message type: "
				 << m_type << "!!" << endl;
		}
	}
	cout << endl;
}

