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
 *
 *  --------------------------------------------------------------------------
 */

#ifndef QSIM_QREG_H_
#define QSIM_QREG_H_


#include <vector>
#include <complex>

#ifndef __QSIM_CPU__
#include "qSim_qcpu_device_GPU_CUDA.h"
#else
#include <qSim_qcpu_device_CPU.h>
#endif

#include "qSim_qinstruction_core.h"
#include "qSim_qinstruction_block.h"


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
				                  QREG_ST_INDEX_TYPE* m_st, double* m_exp, QREG_ST_INDEX_ARRAY_TYPE* m_vec);

		bool applyBlockInstruction(qSim_qinstruction_block* qr_instr, std::string* result);

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

		bool measureState(int q_idx, int q_len, bool m_rand, bool m_coll,
						  QREG_ST_INDEX_TYPE* m_st, double* m_exp, QREG_ST_INDEX_ARRAY_TYPE* m_vec);

		// CUDA device interface control (used by qCpu class)
		void synchDevStates();

		// support methods for qureg state measurement handling
		bool do_state_measure(int q_idx, int q_len,  bool do_rnd, bool collapse_st,
				              QREG_ST_INDEX_TYPE* m_st, double* m_exp, QREG_ST_INDEX_ARRAY_TYPE* m_vec, bool d_vals);
		double get_state_measure_expectation(int st_idx, int q_idx, int q_len);

		int get_state_bitval(int st_idx, int q_idx, int q_len);
};

#endif /* QSIM_QREG_H_ */
