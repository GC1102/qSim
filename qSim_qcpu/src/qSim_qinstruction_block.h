/*
 * qSim_qinstruction_block.h
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

#ifndef QSIM_QINSTRUCTION_BLOCK_H_
#define QSIM_QINSTRUCTION_BLOCK_H_


#include <list>
using namespace std;

#include "qSim_qinstruction_base.h"
#include "qSim_qinstruction_core.h"


// -------------------------------------------------
// q-instruction block class
// -------------------------------------------------

class qSim_qinstruction_block : public qSim_qinstruction_base {

public:
	// class attributes

//	// general
//	QASM_MSG_ID_TYPE m_type; --> inherited from base class

	// qureg handling related
	int m_qr_h;

	// transformation function block related
	QASM_F_TYPE m_ftype;
	int m_fsize;
	int m_frep;
	int m_flsq;
	QREG_F_INDEX_RANGE_TYPE m_fcrng;
	QREG_F_INDEX_RANGE_TYPE m_ftrng;
	QREG_F_ARGS_TYPE m_fargs;

	// constructor and destructor
	qSim_qinstruction_block();
	qSim_qinstruction_block(qSim_qasm_message*);
	virtual ~qSim_qinstruction_block();

	// other constructors - diagnostics
	qSim_qinstruction_block(QASM_MSG_ID_TYPE type, int qr_h, QASM_F_TYPE fbtype, int fsize, int frep, int flsq,
					  	    QREG_F_INDEX_RANGE_TYPE fcrng=QREG_F_INDEX_RANGE_TYPE(),
							QREG_F_INDEX_RANGE_TYPE ftrng=QREG_F_INDEX_RANGE_TYPE(),
							QREG_F_ARGS_TYPE fargs=QREG_F_ARGS_TYPE()); // transformation block

	// helper methods for specific block unwrapping into core instructions
	void unwrap_block_swap_q1(std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose=false);
	void unwrap_block_swap_qn(std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose=false);

	void unwrap_block_cswap_q1(std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose=false);
	void unwrap_block_cswap_qn(std::list<qSim_qinstruction_core*>* , bool verbose=false);

	// ... other blocks...

	// diagnostics
	void dump();

private:

	// internal methods for transformation parameters semantic checks
	bool check_params();

};


#endif /* QSIM_QINSTRUCTION_BLOCK_H_ */
