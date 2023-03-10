/*
 * qSim.cpp
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
 *  1.1   Nov-2022   Updated to align to changes in QASM module.
 *                   Handled qio and qcpu handlers are dynamic attributes and
 *                   initialised passing verbose flag.
 *  1.2   Feb-2023   Handled message reading and socket polling timeouts passage
 *                   as init arguments.
 *
 *  --------------------------------------------------------------------------
 */

#include <chrono>
#include <iostream>
using namespace std;

#include "qSim.h"


// constructor
qSim::qSim(bool verbose) {
	// init handlers
	m_qioHandler = new qSim_qio(verbose);
	m_qcpuHandler = new qSim_qcpu(verbose);

	// set message loop timeout value
	m_msgTimeout = QSIM_MSG_LOOP_TIMEOUT_MSEC;

	// set verbose level
	m_verbose = verbose;
}

// destructor
qSim::~qSim() {
	// release handlers
	delete m_qioHandler;
	delete m_qcpuHandler;
}

///////////////////////////////////////////////////////////////////

int qSim::init(std::string ipAddr, int port, int msgTimeout, int sockTimeout) {
	// Initialize qIo handler
	m_msgTimeout = msgTimeout;
	int ret = m_qioHandler->init(ipAddr, port, sockTimeout);
	if (m_verbose) {
		if (ret == QBUS_SOCK_OK)
			cout << "qSim::init done - ipAddr: " << ipAddr << " port: " << port << endl << endl;
		else
			cerr << "qSim::init error!!" << endl;
	}
	return ret;
}

///////////////////////////////////////////////////////////////////

void qSim::startLoop() {
//	if (m_verbose)
//		cout << "qSim::startLoop..." << endl;

	m_keepRunning.test_and_set();
	m_thr_id = std::thread(&qSim::doLoop, this);
	m_thr_id.detach();
}

void qSim::stopLoop() {
	if (m_verbose)
		cout << "qSim::stopLoop..." << endl;

	m_keepRunning.clear();
	if (m_thr_id.joinable())
		m_thr_id.join();
}

// *********************************************************
// support methods
// *********************************************************

// thread loop handling method
void qSim::doLoop() {
	cout << "qSim...doLoop...m_msgTimeout:" << m_msgTimeout << endl;

	// loop for routing incoming & outcoming messages - this is a performance critical part!
	while (m_keepRunning.test_and_set()) {

		// check qio input queue for instructions from client
		if (m_qioHandler->get_msgIn_queue_size() > 0) {
			// message present - extract first one
			qSim_qasm_message* msg_in = m_qioHandler->pop_msgIn_queue();
			if (m_verbose) {
				cout << "qSim::doLoop - msg found in queue-in" << endl;
				msg_in->dump();
				cout << "... sending to qcpu..." << endl;
			}

			// submit instruction message to CPU
			qSim_qasm_message* msg_out = m_qcpuHandler->dispatch_instruction(msg_in);
			if (m_verbose) {
				cout << "qSim::doLoop - qcpu processing done" << endl;
				msg_out->dump();
				cout << "... pushing to queue-out..." << endl;
			}

			// submit CPU response to qio output queue
			m_qioHandler->push_msgOut_queue(msg_out);
		}

		// sleep
		this_thread::sleep_for(chrono::microseconds(m_msgTimeout));
	}

	cout << "qSim::doLoop done." << endl;
}

