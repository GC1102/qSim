/*
 * qSim_qasm.h
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
 *  1.1   May-2022   Rewritten to limit to supported Q-CPU supported instruction
 *                   set functions.
 *  1.2   Nov-2022   Instruction set limitation to 1 and 2 qubit gates, and to
 *                   generic n-qubit controlled gates.
 *                   Renamed qureg state get method constant.
 *                   Renamed client id and token constants.
 *                   Updated message parameter tag labels.
 *                   Supported function block - 1-qubit SWAP.
 *                   Code clean-up.
 *  1.3   Feb-2023   Supported qureg state expectation calculation and fixed
 *                   terminology for state probability measure.
 *                   Handled QML function blocks (feature map and q-net).
 *
 *  --------------------------------------------------------------------------
 */

#ifndef QSIM_QASM_H_
#define QSIM_QASM_H_

#include <map>
#include <string>


///////////////////////////////////////////////////////////////////////////
//  QASM message information handling class and constants
///////////////////////////////////////////////////////////////////////////

// control messages
#define QASM_MSG_ID_NOPE       0
#define QASM_MSG_ID_REGISTER   1
#define QASM_MSG_ID_UNREGISTER 2

// qureg handling instruction messages
#define QASM_MSG_ID_QREG_ALLOCATE     10
#define QASM_MSG_ID_QREG_RELEASE      11
#define QASM_MSG_ID_QREG_ST_RESET     12
#define QASM_MSG_ID_QREG_ST_SET       13
#define QASM_MSG_ID_QREG_ST_TRANSFORM 14
#define QASM_MSG_ID_QREG_ST_PEEK      15
#define QASM_MSG_ID_QREG_ST_MEASURE   16
#define QASM_MSG_ID_QREG_ST_EXPECT    17

// message responses
#define QASM_MSG_ID_RESPONSE 20

// message body separators
#define QASM_MSG_FIELD_SEP "|"
#define QASM_MSG_PARAM_SEP ":"
#define QASM_MSG_PARVAL_SEP "="

// message parameter tags
#define QASM_MSG_PARAM_TAG_CLIENT_ID    "id"		// client mnemonic identifier
#define QASM_MSG_PARAM_TAG_CLIENT_TOKEN "token"		// client unique token

#define QASM_MSG_PARAM_TAG_QREG_QN      "qr_n"       // qureg size in qubits
#define QASM_MSG_PARAM_TAG_QREG_H       "qr_h" 		 // qureg state handler
#define QASM_MSG_PARAM_TAG_QREG_STIDX   "qr_stIdx" 	 // qureg state index (pure state set)
#define QASM_MSG_PARAM_TAG_QREG_STVALS  "qr_stVals"  // qureg state values (arbitrary state set)
#define QASM_MSG_PARAM_TAG_QREG_MQIDX   "qr_mQidx" 	 // qureg measurement start qubit
#define QASM_MSG_PARAM_TAG_QREG_MQLEN   "qr_mQlen" 	 // qureg measurement length
#define QASM_MSG_PARAM_TAG_QREG_MRAND   "qr_mRand" 	 // qureg measurement random flag
#define QASM_MSG_PARAM_TAG_QREG_MCOLL   "qr_mStColl" // qureg measured state collapse flag
#define QASM_MSG_PARAM_TAG_QREG_MSTIDX  "qr_mStIdx"	 // qureg measurement state index
#define QASM_MSG_PARAM_TAG_QREG_MSTPR   "qr_mStPr"	 // qureg measurement state probability
#define QASM_MSG_PARAM_TAG_QREG_MSTIDXS "qr_mStIdxs" // qureg measurement state index vector
#define QASM_MSG_PARAM_TAG_QREG_EXSTIDX "qr_exStIdx" // qureg measurement state index
#define QASM_MSG_PARAM_TAG_QREG_EXQIDX  "qr_exQidx"  // qureg state expectation start qubit
#define QASM_MSG_PARAM_TAG_QREG_EXQLEN  "qr_exQlen"  // qureg state expectation length
#define QASM_MSG_PARAM_TAG_QREG_EXOBSOP "qr_exObsOp" // qureg state expectation observable operator
#define QASM_MSG_PARAM_TAG_QREG_EXSTVAL "qr_exStVal" // qureg state expectation value

#define QASM_MSG_PARAM_TAG_F_TYPE     "f_type"      // function type
#define QASM_MSG_PARAM_TAG_F_SIZE     "f_size"		// # of function states
#define QASM_MSG_PARAM_TAG_F_REP      "f_rep"		// # of function repetitions
#define QASM_MSG_PARAM_TAG_F_LSQ      "f_lsq"		// function LSQ index
#define QASM_MSG_PARAM_TAG_F_CRANGE   "f_cRange"	// function control qubit index range (controlled-U only)
#define QASM_MSG_PARAM_TAG_F_TRANGE   "f_tRange"	// function target qubit index range (controlled-U only)
#define QASM_MSG_PARAM_TAG_F_UTYPE    "f_uType"		// function-U type
#define QASM_MSG_PARAM_TAG_F_ARGS     "f_args"		// function arguments, including function-U args if any

#define QASM_MSG_PARAM_TAG_FBQML_REP      "fqml_rep"         // QML block repetitions
#define QASM_MSG_PARAM_TAG_FBQML_ENTANG   "fqml_entang_type" // QML entanglement type
#define QASM_MSG_PARAM_TAG_FBQML_SUBTYPE  "fqml_subtype"	 // QML subtype (PauliZ/PauliZZ for fmap or RealAmpl/TBD for qnet)
#define QASM_MSG_PARAM_TAG_FBQML_QNETTYPE "fqml_qnet_type"	 // QML qnet layout type

#define QASM_MSG_PARAM_TAG_RESULT     "result"		// instruction result
#define QASM_MSG_PARAM_TAG_ERROR      "error"		// instruction result error details

// parameter values
#define QASM_MSG_PARAM_VAL_OK  "Ok"
#define QASM_MSG_PARAM_VAL_NOK "Not-Ok"


// -----------------------------------------------

// datatypes definition

typedef std::string QASM_MSG_ACCESS_TOKEN_TYPE;
typedef int QASM_MSG_ID_TYPE;
typedef unsigned int QASM_MSG_COUNTER_TYPE;
typedef std::map<std::string, std::string> QASM_MSG_PARAMS_TYPE;

// -----------------------------------------------

// QASM message handling class
class qSim_qasm_message {

public:
	// constructors
	qSim_qasm_message();
	qSim_qasm_message(QASM_MSG_COUNTER_TYPE counter, QASM_MSG_ID_TYPE id,
					  QASM_MSG_PARAMS_TYPE params=QASM_MSG_PARAMS_TYPE());
	virtual ~qSim_qasm_message();

	// accessors
	QASM_MSG_COUNTER_TYPE get_counter()	{ return m_counter; }
	QASM_MSG_ID_TYPE      get_id() 		{ return m_id; }
	QASM_MSG_PARAMS_TYPE  get_params()	{ return m_params; }

	bool is_control_message()     { return ((m_id==QASM_MSG_ID_REGISTER) || (m_id==QASM_MSG_ID_UNREGISTER)); }
	bool is_instruction_message() { return ((m_id>=QASM_MSG_ID_QREG_ALLOCATE) && (m_id<=QASM_MSG_ID_QREG_ST_EXPECT)); }

	// message parameters handling
	bool check_param_valueByTag(std::string par_tag)      { return (m_params.count(par_tag) > 0); };
	std::string get_param_valueByTag(std::string par_tag) { return m_params[par_tag]; };
	void add_param_tagValue(std::string par_tag, std::string par_val);

	// content syntax checking
	bool check_syntax();

	// decoding/encoding from/to transmission raw format
	void from_char_array(unsigned int len, char*);
	void to_char_array(unsigned int* len, char**);

	// diagnostics
	void dump();

protected:
	// class attributes
	QASM_MSG_ID_TYPE m_id;
	QASM_MSG_COUNTER_TYPE m_counter;
	QASM_MSG_PARAMS_TYPE m_params;

	// error logging
 	void log_missing_param_tag(std::string par_tag);

};


///////////////////////////////////////////////////////////////////////////
//  QASM transformation functions handling constants
///////////////////////////////////////////////////////////////////////////

// data type and supported function types - must be contiguous! (used as vector indexes in CUDA)

enum QASM_F_TYPE {
	// null value
	QASM_F_TYPE_NULL = -1,

	// 1 qubit
	QASM_F_TYPE_Q1_I = 0,
	QASM_F_TYPE_Q1_H,
	QASM_F_TYPE_Q1_X,
	QASM_F_TYPE_Q1_Y,
	QASM_F_TYPE_Q1_Z,
	QASM_F_TYPE_Q1_SX,
	QASM_F_TYPE_Q1_PS,
	QASM_F_TYPE_Q1_T,
	QASM_F_TYPE_Q1_S,
	QASM_F_TYPE_Q1_Rx,
	QASM_F_TYPE_Q1_Ry,
	QASM_F_TYPE_Q1_Rz,

	// 2 qubits
	QASM_F_TYPE_Q2_CU,
	QASM_F_TYPE_Q2_CX,
	QASM_F_TYPE_Q2_CY,
	QASM_F_TYPE_Q2_CZ,

	// n qubits
	QASM_F_TYPE_QN_MCSLRU, // multi-controlled short/long range U
	QASM_F_TYPE_Q3_CCX,    // aka Toffoli

	// function blocks
	QASM_FB_TYPE_Q1_SWAP = 100, // 1-qubit swap
	QASM_FB_TYPE_QN_SWAP,       // n-qubit swap
	QASM_FB_TYPE_Q1_CSWAP,      // 1-qubit c-swap
	QASM_FB_TYPE_QN_CSWAP,      // n-qubit c-swap

	// function QML blocks
	QASM_FBQML_TYPE_FMAP = 200, // feature-map
	QASM_FBQML_TYPE_QNET,       // qvc q-network

	// others...
	// ...
};

// macros for checking for 1-qubit or 2-qubit gates
#define QASM_F_TYPE_IS_GATE_1QUBIT(ft) ((ft >= QASM_F_TYPE_Q1_I) && (ft <= QASM_F_TYPE_Q1_Rz))
#define QASM_F_TYPE_IS_GATE_2QUBIT(ft) ((ft >= QASM_F_TYPE_Q2_CU) && (ft <= QASM_F_TYPE_Q2_CZ))
#define QASM_F_TYPE_IS_GATE_NQUBIT(ft) ((ft >= QASM_F_TYPE_QN_MCSLRU) && (ft <= QASM_F_TYPE_Q3_CCX))

#define QASM_F_TYPE_IS_FUNC(ft) (QASM_F_TYPE_IS_GATE_1QUBIT(ft) || QASM_F_TYPE_IS_GATE_2QUBIT(ft) || QASM_F_TYPE_IS_GATE_NQUBIT(ft))
#define QASM_F_TYPE_IS_FUNC_BLOCK(ft) ((ft >= QASM_FB_TYPE_Q1_SWAP) && (ft <= QASM_FB_TYPE_QN_CSWAP))
#define QASM_F_TYPE_IS_FUNC_BLOCK_QML(ft) ((ft >= QASM_FBQML_TYPE_FMAP) && (ft <= QASM_FBQML_TYPE_QNET))

// ----------------------------------

// function form values
#define QASM_F_FORM_NULL    -1
#define QASM_F_FORM_DIRECT  0
#define QASM_F_FORM_INVERSE 1

// ----------------------------------

///////////////////////////////////////////////////////////////////////////
//  QASM qureg state expectation observable operators handling constants
///////////////////////////////////////////////////////////////////////////

// data type and supported observable operators

enum QASM_EX_OBSOP_TYPE {
	// null value
	QASM_EX_OBSOP_TYPE_NULL = -1,

	QASM_EX_OBSOP_TYPE_COMP = 0,
	QASM_EX_OBSOP_TYPE_PAULIZ,
	// add others here...
	// ...
};

// ----------------------------------

///////////////////////////////////////////////////////////////////////////
//  QASM qml function blocks handling constants
///////////////////////////////////////////////////////////////////////////

// data type and supported entanglement types

enum QASM_QML_ENTANG_TYPE {
	// null value
	QASM_QML_ENTANG_TYPE_NULL = -1,

	QASM_QML_ENTANG_TYPE_LINEAR = 0,
	QASM_QML_ENTANG_TYPE_CIRCULAR,

	// add others here...
	// ...
};

// data type and supported feature map types

enum QASM_QML_FMAP_TYPE {
	// null value
	QASM_QML_FMAP_TYPE_NULL = -1,

	QASM_QML_FMAP_TYPE_PAULI_Z = 0,
	QASM_QML_FMAP_TYPE_PAULI_ZZ,

	// add others here...
	// ...
};

// data type and supported qnet layout types

enum QASM_QML_QNET_LAY_TYPE {
	// null value
	QASM_QML_QNET_LAY_TYPE_NULL = -1,

	QASM_QML_QNET_LAY_TYPE_REAL_AMPL = 0,

	// add others here...
	// ...
};

// ----------------------------------

#endif /* QSIM_QASM_H_ */
