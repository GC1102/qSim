/*
 * qSim_qcpu.h
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
 *  Created on: Apr 8, 2018
 *      Author: gianni
 *
 * Q-CPU class, supporting control methods for:
 * - CPU reset and shutdown
 * - qregs handling (creation, set/reset and state access)
 * - qregs transformation via state functions (using CUDA kernel call wrappers)
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Apr-2018   Module creation.
 *  2.0   May-2022   Updated for implementing Q-CPU control methods.
 *  2.1   Nov-2022   Moved state value::string conversion helpers to qreg_instruction
 *                   class.
 *                   Used qreg_instruction class for instruction data handling.
 *                   Handled verbose flag for controlling diagnostic messages in
 *                   the qureg instruction execution flow.
 *                   Supported function block - 1-qubit SWAP.
 *                   Handled CPU mode compiling and applied agnostic class renaming
 *                   to qSim_qcpu_device.
 *                   Code clean-up.
 *  2.2   Feb-2023   Handled QML function blocks (feature map and q-net).
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QCPU_H_
#define QSIM_QCPU_H_

#include <map>

#include "qSim_qreg.h"
#include "qSim_qinstruction_core.h"
#include "qSim_qinstruction_block.h"
#include "qSim_qinstruction_block_qml.h"

#ifndef __QSIM_CPU__
#include <qSim_qcpu_device_GPU_CUDA.h>
#else
#include <qSim_qcpu_device_CPU.h>
#endif

// data type for qreg handlers
typedef unsigned int QREG_HNDL_TYPE;

class qSim_qcpu {
	public:
		// constructor and destructor
		qSim_qcpu(bool verbose=false);
		virtual ~qSim_qcpu();

		// QASM instruction message dispatcher
		qSim_qasm_message* dispatch_instruction(qSim_qasm_message* msg_in);

		// ----------------------------------------
		// qcpu instructions execution handlers

		// - qcpu control
		bool reset();
		bool switchOff();

		// qureg control
		QREG_HNDL_TYPE qureg_allocate(qSim_qinstruction_core*);
		bool qureg_release(qSim_qinstruction_core*);

		// qureg core instructions handling
		bool exec_qureg_instruction_core(qSim_qinstruction_core*, QASM_MSG_PARAMS_TYPE*);

		// qureg block instructions handling
		bool exec_qureg_instruction_block(qSim_qinstruction_block*, QASM_MSG_PARAMS_TYPE*);

		// qureg QML block instructions handling
		bool exec_qureg_instruction_block_qml(qSim_qinstruction_block_qml*, QASM_MSG_PARAMS_TYPE*);

		// diagnostics
		void dump();
#ifndef __QSIM_CPU__
		void dumpGpuInfo();
#endif
		int get_tot_quregs();
		int qureg_size(int qr_h);

	private:
		// internal attributes: qureg objects and CUDA qCpu instance
		map<QREG_HNDL_TYPE, qSim_qreg*> m_qreg_map;
		qSim_qcpu_device* m_qcpu_device;

		// diagnostic message control
		bool m_verbose;

		// qureg map deallocation
		void qreg_mapRelease();
	};

#endif /* QSIM_QCPU_H_ */
