/*
 * qSim_qio.cpp
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
 *  Created on: May 12, 2022
 *      Author: gianni
 *
 * Q-IO class, handling q-SIM access in terms of:
 * - control instructions and responses
 * - data input & output
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation.
 *  1.1   Nov-2022   Updated to align to changes in QASM module.
 *
 *  --------------------------------------------------------------------------
 */

#include <ctime>
#include <chrono>
#include <iostream>
using namespace std;

#include "qSim_qio.h"
#include "qSim_qasm.h"


// thread loop timeout
#define LOOP_TIMEOUT_MSEC 100


// constructor
qSim_qio::qSim_qio(bool verbose) {
	m_qsockSrv = new qSim_qio_socket_server(verbose);
	m_verbose = verbose;
}

// destructor
qSim_qio::~qSim_qio() {
	// stop qsocket
	m_qsockSrv->stopLoop();
	delete m_qsockSrv;

	// release queues content
	qSim_qasm_message* msg;

	while ((msg = m_msgIn_queue.peek()) != NULL) {
		delete msg;
		m_msgIn_queue.pop();
	}

	while ((msg = m_msgOut_queue.peek()) != NULL) {
		delete msg;
		m_msgOut_queue.pop();
	}
}

///////////////////////////////////////////////////////////////////

int qSim_qio::init(std::string ipAddr, int port) {
	// Initialize socket server
	m_qsockSrv->set_dataInOut_callback(this);
	int ret = m_qsockSrv->init(ipAddr, port);
	if (ret == QBUS_SOCK_ERROR)
		cerr << "qSim_qio::init - failed to initialize socket server - ipAddr: " << ipAddr
		     << "  port: " << port << endl;
	else
		m_qsockSrv->startLoop();

	return ret;
}

///////////////////////////////////////////////////////////////////

qSim_qasm_message* qSim_qio::pop_msgIn_queue() {
	qSim_qasm_message* qasm_msg = m_msgIn_queue.peek();
	m_msgIn_queue.pop();
	return qasm_msg;
}

void qSim_qio::push_msgOut_queue(qSim_qasm_message* qasm_msg) {
	m_msgOut_queue.push(qasm_msg);
}

// *********************************************************
// support methods
// *********************************************************

// in/out message handling callbacks for socket server

void qSim_qio::in_message_cb(qio_raw_msg* msg) {
	// create qasm object from given raw message
	qSim_qasm_message* qasm_msg = new qSim_qasm_message();
	qasm_msg->from_char_array(msg->m_len, msg->m_dataBuf);
	if (m_verbose)
		cout << "qSim_qio::in_message_cb - m_len: " << msg->m_len
		     << "  m_dataBuf: " << msg->m_dataBuf << endl;

	// add to input queue - correct
	if (qasm_msg->check_syntax()) {
		// syntax ok - check type
		if (m_verbose) {
			cout << "qSim_qio::in_message_cb - qasm syntax check ok" << endl;
//			qasm_msg->dump(); done already before...
		}
		if (qasm_msg->is_control_message()) {
			// admin message - handle it here
			handle_control_message(qasm_msg);
			if (m_verbose)
				cout << "qSim_qio::in_message_cb - qasm control msg processed" << endl;
		}
		else {
			// instruction message - check if token is ok
			QASM_MSG_ACCESS_TOKEN_TYPE token = qasm_msg->get_param_valueByTag(QASM_MSG_PARAM_TAG_CLIENT_TOKEN);
			if (check_clien_token(token)) {
				// token recognized - push message to in-queue for qcpu
				m_msgIn_queue.push(qasm_msg);
				if (m_verbose)
					cout << "qSim_qio::in_message_cb - qasm instruction msg syntax ok -> added to in-queue" << endl;
			}
			else {
				// token error - raise error response and discard message
				cerr << "qSim_qio::in_message_cb - qasm token not recognised!! -> discarded" << endl;

				// push error message in the out-queue
				QASM_MSG_COUNTER_TYPE counter = qasm_msg->get_id();
				QASM_MSG_ID_TYPE id = QASM_MSG_ID_RESPONSE;
				qSim_qasm_message* qasm_err_msg = new qSim_qasm_message(counter, id);
				qasm_err_msg->add_param_tagValue(QASM_MSG_PARAM_TAG_CLIENT_TOKEN, token);
				qasm_err_msg->add_param_tagValue(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK);
				qasm_err_msg->add_param_tagValue(QASM_MSG_PARAM_TAG_ERROR, "unrecognised token");
				m_msgOut_queue.push(qasm_err_msg);

				delete qasm_msg;
			}
		}
	}
	else {
		// syntax error - raise error response and discard message
		cerr << "qSim_qio::in_message_cb - qasm msg syntax not ok!! -> discarded" << endl;

		// push error message in the out-queue
		QASM_MSG_COUNTER_TYPE counter = qasm_msg->get_id();
		QASM_MSG_ID_TYPE id = QASM_MSG_ID_RESPONSE;
		qSim_qasm_message* qasm_err_msg = new qSim_qasm_message(counter, id);
		qasm_err_msg->add_param_tagValue(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_NOK);
		qasm_err_msg->add_param_tagValue(QASM_MSG_PARAM_TAG_ERROR, "message syntax wrong");
		m_msgOut_queue.push(qasm_err_msg);

		delete qasm_msg;
	}
}

void qSim_qio::out_message_cb(qio_raw_msg* msg) {
	// check for a message in the output queue
	qSim_qasm_message* qasm_msg = m_msgOut_queue.pop();

	// return it - if found
	if (qasm_msg != NULL) {
		// fill-in given raw message from qasm object
		qasm_msg->to_char_array(&(msg->m_len), &(msg->m_dataBuf));

		if (m_verbose)
			cout << "qSim_qio::out_message_cb - m_len: " << msg->m_len
			     << "  m_dataBuf: " << msg->m_dataBuf << endl;

		// release message from queue
		delete qasm_msg;
	}
	else {
		// reset raw message length (i.e. no message found)
		msg->m_len = 0;
	}
}

// ------------------------------------------------------------

void qSim_qio::handle_control_message(qSim_qasm_message* qasm_msg) {
	// handle given control message, i.e. one between
	// - register a client
	// - unregister a client

	switch (qasm_msg->get_id()) {
		case QASM_MSG_ID_REGISTER: {
			// new client registration - check if already registered
			// and in case store it and provide access token back
			string name = qasm_msg->get_param_valueByTag(QASM_MSG_PARAM_TAG_CLIENT_ID);
			QIO_CLIENT_ACCESS_REGISTRY::iterator it;
			for (it=m_cln_registry.begin(); it!=m_cln_registry.end(); it++) {
				if (it->second == name) {
					// client already registered - log warning and erase entry
					cout << "WARNING - user [" << name << "] is registering again - previous token disabled!!" << endl;
					m_cln_registry.erase(it->first);
					break;
				}
			}

			// proceed and return the access token
			QASM_MSG_ACCESS_TOKEN_TYPE token = build_client_token();
			m_cln_registry.insert(std::make_pair(token, name));

			QASM_MSG_COUNTER_TYPE counter = 0;
			QASM_MSG_ID_TYPE id = QASM_MSG_ID_RESPONSE;
			qSim_qasm_message* qasm_err_msg = new qSim_qasm_message(counter, id);
			qasm_err_msg->add_param_tagValue(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK);
			qasm_err_msg->add_param_tagValue(QASM_MSG_PARAM_TAG_CLIENT_TOKEN, token);
			m_msgOut_queue.push(qasm_err_msg);
		}
		break;

		case QASM_MSG_ID_UNREGISTER: {
			// client deregistration - remove credentials
			// --------- >>>> to be done on client disconnect (CB from socket??) of after <x> minutes of inactivity

			QASM_MSG_ACCESS_TOKEN_TYPE token = qasm_msg->get_param_valueByTag(QASM_MSG_PARAM_TAG_CLIENT_TOKEN);
			m_cln_registry.erase(token);

			QASM_MSG_COUNTER_TYPE counter = 0;
			QASM_MSG_ID_TYPE id = QASM_MSG_ID_RESPONSE;
			qSim_qasm_message* qasm_err_msg = new qSim_qasm_message(counter, id);
			qasm_err_msg->add_param_tagValue(QASM_MSG_PARAM_TAG_RESULT, QASM_MSG_PARAM_VAL_OK);
			m_msgOut_queue.push(qasm_err_msg);
		}
		break;

		default: {
			// error id for a control message!!!
			cerr << "ERROR - unhandled control message id " << qasm_msg->get_id() << endl;
		}
		break;
	}
}

QASM_MSG_ACCESS_TOKEN_TYPE qSim_qio::build_client_token() {
	// get seconds from 1970
	std::time_t tm1970 = std::time(0);

	// convert to string
	QASM_MSG_ACCESS_TOKEN_TYPE token = to_string(tm1970);
	return token;
}

bool qSim_qio::check_clien_token(QASM_MSG_ACCESS_TOKEN_TYPE token) {
	return (m_cln_registry.count(token) > 0);
}

// ------------------------------------------------------------

