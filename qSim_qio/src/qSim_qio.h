/*
 * qSim_qio.h
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
 *  1.1   Dec-2022   Modified pop method to return removed element reference.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QIO_H_
#define QSIM_QIO_H_


#include <map>
#include <string>

#include "qSim_qio_socket.h"
#include "qSim_qio_queue.h"
#include "qSim_qasm.h"

// return codes
#define QIO_OK    QBUS_SOCK_OK
#define QIO_ERROR QBUS_SOCK_ERROR

// Client access credentials handling
typedef std::map<QASM_MSG_ACCESS_TOKEN_TYPE, std::string> QIO_CLIENT_ACCESS_REGISTRY;

class qSim_qio : public qSim_qio_socket_server_cb {

	public:
		// constructor and destructor
		qSim_qio(bool verbose=false);
		virtual ~qSim_qio();

		int init(std::string ipAddr, int port);

		// queues access
		int get_msgIn_queue_size()  { return m_msgIn_queue.size(); }
		int get_msgOut_queue_size() { return m_msgOut_queue.size(); }
		qSim_qasm_message* pop_msgIn_queue();
		void push_msgOut_queue(qSim_qasm_message* qasm_msg);

	private:
		// in/out message handling callbacks for socket server
		virtual void in_message_cb(qio_raw_msg*);
		virtual void out_message_cb(qio_raw_msg*);

		void handle_control_message(qSim_qasm_message* qasm_msg);

		// socket server handler
		qSim_qio_socket_server* m_qsockSrv;

		// in/out message queues
		qSim_qio_queue m_msgIn_queue;
		qSim_qio_queue m_msgOut_queue;

		// client access credentials handling
		QASM_MSG_ACCESS_TOKEN_TYPE build_client_token();
		bool check_clien_token(QASM_MSG_ACCESS_TOKEN_TYPE token);

		QIO_CLIENT_ACCESS_REGISTRY m_cln_registry;

		bool m_verbose;
};

#endif /* QSIM_QIO_H_ */
