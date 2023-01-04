/*
 * qSim.h
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
 *  Created on: May 18, 2022
 *      Author: gianni
 *
 * qSim main class, handling qIo clients access and routing to qCpu for execution.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation.
 *  1.1   Nov-2022   Handled qio and qcpu handlers are dynamic attributes and
 *                   initialised passing verbose flag.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_H_
#define QSIM_H_


#include <thread>

#include "qSim_qio.h"
#include "qSim_qcpu.h"

// return codes
#define QSIM_OK    QBUS_SOCK_OK
#define QSIM_ERROR QBUS_SOCK_ERROR


class qSim {

	public:
		// constructor and destructor
		qSim(bool verbose=false);
		virtual ~qSim();

		int init(std::string ipAddr, int port);

		void startLoop();
		void stopLoop();

	private:
		// qIo handler
		qSim_qio* m_qioHandler;

		// qCpu handler
		qSim_qcpu* m_qcpuHandler;

		// loop thread control
		void doLoop();
		std::thread m_thr_id;
		std::atomic_flag m_keepRunning = ATOMIC_FLAG_INIT;

		bool m_verbose;
};

#endif /* QSIM_H_ */
