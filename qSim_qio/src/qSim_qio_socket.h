/*
 * qSim_qio_socket.h
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
 *  Created on: May 13, 2022
 *      Author: gianni
 *
 * Q-IO socket server class, specializing qBus general class for supporting I/O messages:
 * - control instructions
 * - data input & output
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation cloning former qreg_qsocket class.
 *  1.1   Feb-2023   Handled socket client polling timeout passage as init argument.
 *                   Code clean-up.
 *
 *  --------------------------------------------------------------------------
 */

#ifndef QSIM_QIO_SOCKET_H_
#define QSIM_QIO_SOCKET_H_

#include <thread>
#include <functional>

#include "qSim_qsocket.h"

// => variable length messages exchanged
//    - message len first - 4 bytes fixed length
//    - message content - <len> bytes

#define QIO_MSG_LEN_SIZE 4 // same as sizeof(unsigned int)

struct qio_raw_msg {
	unsigned int  m_len;
	char* 		  m_dataBuf; // dynamic allocation

	~qio_raw_msg() {if (m_len > 0) delete(m_dataBuf); }
};

// ----------------

class qSim_qio_socket_server_cb {
public:
	qSim_qio_socket_server_cb() {};
	virtual ~qSim_qio_socket_server_cb() {};
	virtual void in_message_cb(qio_raw_msg*) = 0;
	virtual void out_message_cb(qio_raw_msg*) = 0;
};

#define QIO_SOCK_CLN_ACCEPT_LOOP_TIMEOUT_USEC 10000
#define QIO_SOCK_CLN_MSG_LOOP_TIMEOUT_USEC 100


class qSim_qio_socket_server : public qSim_qsocket_server {
	public:
		qSim_qio_socket_server(bool verbose=false);
		virtual ~qSim_qio_socket_server();

		void set_dataInOut_callback(qSim_qio_socket_server_cb* cb);
		void set_clientPollingTimeout(int timeout);

	private:
		qSim_qio_socket_server_cb* m_dataInOut_cb;
		int m_clnPollingTimeout;

		std::thread m_clnThr_id;

		virtual void doLoop();
		void doLoop_client();

		bool receive_data(struct qio_raw_msg* msg);
		bool send_data(struct qio_raw_msg* msg);

};

#endif /* QSIM_QIO_SOCKET_H_ */
