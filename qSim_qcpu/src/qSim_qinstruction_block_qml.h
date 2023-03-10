/*
 * qSim_qinstruction_block_qml.h
 *
 * --------------------------------------------------------------------------
 * Copyright (C) 2023 Gianni Casonato
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
 *  Created on: Feb 24, 2023
 *      Author: gianni
 *
 * QML specialised instruction block class, handling ad-hoc "block" instructions
 * supporting:
 * - feature map blocks
 * - QVC q-net blocks
 *
 * Derived class from qSim_qinstruction_block.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Feb-2023   Module creation.
 *
 *  --------------------------------------------------------------------------
 */

#ifndef QSIM_QINSTRUCTION_BLOCK_QML_H_
#define QSIM_QINSTRUCTION_BLOCK_QML_H_


#include <list>
using namespace std;

#include "qSim_qinstruction_block.h"


// -------------------------------------------------
// QML q-instruction block class
// -------------------------------------------------

class qSim_qinstruction_block_qml : public qSim_qinstruction_block {

public:
	// class attributes

	// QML transformation function block related
	QASM_QML_ENTANG_TYPE m_fbent;
	int m_fbsubtype;

	// constructor and destructor
	qSim_qinstruction_block_qml(qSim_qasm_message*);
	virtual ~qSim_qinstruction_block_qml();

	// other constructors - diagnostics
	qSim_qinstruction_block_qml(QASM_MSG_ID_TYPE type, int qr_h,
								QASM_F_TYPE fbtype, int fbrep,
								QASM_QML_ENTANG_TYPE fbent, int fbsubtype,
								QREG_F_ARGS_TYPE fargs=QREG_F_ARGS_TYPE());

	// helper methods for specific block unwrapping into core instructions
	void unwrap_block_fmap(std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose=false);
	void unwrap_block_qnet(int n, std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose=false);

	// ... other blocks...

	// diagnostics
	void dump();

private:

	// internal methods for transformation parameters semantic checks
	bool check_params();

	// feature map phase encoding block unwrap implementation methods
	void feature_map_pe_pauliZ(QREG_F_ARGS_TYPE* f_vec, int b_rep,
							   std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose);
	void feature_map_pe_pauliZZ(QREG_F_ARGS_TYPE* f_vec, int b_rep, QASM_QML_ENTANG_TYPE b_entang,
							    std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose);

	// q-network block unwrap implementation methods
	void qnetwork_realAmplitude(int n, QREG_F_ARGS_TYPE* param_vec, int b_rep, QASM_QML_ENTANG_TYPE b_entang,
							    std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose);

	void qnetwork_build_G_block(int n, int b, int i, int tot_block_params, QREG_F_ARGS_TYPE* param_vec, int lay_type,
			std::list<qSim_qinstruction_core*>* qinstr_list);
	void qnetwork_build_G1(QREG_F_INDEX_RANGE_TYPE param_idx_range, QREG_F_ARGS_TYPE* param_vec, int i,
			std::list<qSim_qinstruction_core*>* qinstr_list);
	void qnetwork_build_CG_block(int c_idx, int t_idx, QREG_F_ARGS_TYPE* param_vec, int i,
			std::list<qSim_qinstruction_core*>* qinstr_list);

};


#endif /* QSIM_QINSTRUCTION_BLOCK_QML_H_ */
