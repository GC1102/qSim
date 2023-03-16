/*
 * qSim_qreg.h
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
 *  Created on: Mar 30, 2018
 *      Author: gianni
 *
 * Q-REG class, supporting control methods for:
 * - qreg initialisation from given number of qubits
 * - qregs state set/reset
 * - qregs state setup from values
 * - qregs state access
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Mar-2018   Module creation.
 *  2.0   May-2022   Rewritten for implementing Q-REG control methods for supporting
 *                   function based Q-CPU, eliminating splitting in nodes and
 *                   socket links.
 *  2.1   Nov-2022   Used qreg_instruction class for argument data type.*
 *                   Handled verbose flag for controlling diagnostic messages in
 *                   qureg transformation execution flow.
 *                   Handled device->host sync only if needed, based on specific
 *                   class attribute flag.
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

#ifndef QSIM_QREG_H_
#define QSIM_QREG_H_


#include <vector>
#include <complex>
#include <map>

#ifndef __QSIM_CPU__
#include "qSim_qcpu_device_GPU_CUDA.h"
#else
#include <qSim_qcpu_device_CPU.h>
#endif

#include "qSim_qinstruction_core.h"
#include "qSim_qinstruction_block.h"
#include "qSim_qinstruction_block_qml.h"


class qSim_qreg {

	// internal attribute: state vectors
	QREG_ST_RAW_VAL_TYPE* m_states_x;
	QREG_ST_RAW_VAL_TYPE* m_devStates_x;
	QREG_ST_RAW_VAL_TYPE* m_devStates_y;
	unsigned int m_totStates;
	unsigned int m_totQubits;

	// reference to qCUP CUDA instance
	qSim_qcpu_device* m_qcpu_device;

	// device->host synch flag
	bool m_syncFlag;

	// verbose flag
	bool m_verbose;

	public:
		// constructor and destructor
		qSim_qreg(int q_n, qSim_qcpu_device* qcpu_dev, bool verbose);
		virtual ~qSim_qreg();

		// qureg control & transformation
		bool applyCoreInstruction(qSim_qinstruction_core* qr_instr, std::string* result);
		bool applyCoreInstruction(qSim_qinstruction_core* qr_instr, std::string* result,
								  QREG_ST_VAL_ARRAY_TYPE* stArray);
		bool applyCoreInstruction(qSim_qinstruction_core* qr_instr, std::string* result,
				                  QREG_ST_INDEX_TYPE* m_st, double* m_pr, QREG_ST_INDEX_ARRAY_TYPE* m_vec);
		bool applyCoreInstruction(qSim_qinstruction_core* qr_instr, std::string* result,
				                  double* m_exp);

		bool applyBlockInstruction(qSim_qinstruction_block* qr_instr, std::string* result);
		bool applyBlockInstructionQml(qSim_qinstruction_block_qml* qr_instr, std::string* result);

		// accessors
		unsigned int getTotStates();

		// diagnostics
		void dump(unsigned max_st=10u);

	private:
		// state control and access
		bool resetState();
		bool setState(unsigned st_idx);
		bool setState(QREG_ST_VAL_ARRAY_TYPE* stArray);

		bool transform(QASM_F_TYPE ftype, int fsize, int frep, int flsq,
				       QREG_F_INDEX_RANGE_TYPE fcrng, QREG_F_INDEX_RANGE_TYPE ftrng, QREG_F_ARGS_TYPE fargs,
				       int futype, QREG_F_INDEX_RANGE_TYPE fucrng, QREG_F_INDEX_RANGE_TYPE futrng, QREG_F_ARGS_TYPE fuargs);

		bool getStates(QREG_ST_VAL_ARRAY_TYPE* stArray);

		bool stateMeasure(int q_idx, int q_len, bool m_rand, bool m_coll,
						  QREG_ST_INDEX_TYPE* m_st, double* m_pr, QREG_ST_INDEX_ARRAY_TYPE* m_vec);
		bool stateExpectation(int st_idx, int q_idx, int q_len, QASM_EX_OBSOP_TYPE ex_opsOp, double* m_exp);

		// CUDA device interface control (used by qCpu class)
		void synchDevStates();

		// support methods for qureg state measurement handling
		bool do_state_measure(int q_idx, int q_len,  bool do_rnd, bool collapse_st,
				              QREG_ST_INDEX_TYPE* m_st, double* m_exp, QREG_ST_INDEX_ARRAY_TYPE* m_vec, bool d_vals);
		double get_state_probability(int st_idx, int q_idx, int q_len);

		int get_state_bitval(int st_idx, int q_idx, int q_len);

		// support methods for qureg state expectation handling
		void get_state_expectations(int q_idx, int q_len, QASM_EX_OBSOP_TYPE ex_opsOp, std::vector<double>* ex_obsOp_vec);
		void get_state_probabilities(std::vector<double>* pr_vec);

		void kron_product(std::vector<double>* v1, std::vector<double>* v2, std::vector<double>* v3);

		void apply_instruction_and_release(std::list<qSim_qinstruction_core*>* qinstr_list, QREG_F_ARGS_TYPE fargs,
				                           bool* res, std::string* res_str, bool do_release=true);

		map<QASM_EX_OBSOP_TYPE, std::vector<double>> m_obs_ev_map;

};

#endif /* QSIM_QREG_H_ */
