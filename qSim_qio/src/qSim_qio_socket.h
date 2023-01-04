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
 *
 *  --------------------------------------------------------------------------
 */

#ifndef QSIM_QIO_SOCKET_H_
#define QSIM_QIO_SOCKET_H_

#include <thread>
#include <functional>

#include "qSim_qsocket.h"

//#define QIO_MSG_DATA_MAX_LEN 128

// => variable length messages exchanged
//    - message len first - 4 bytes fixed length
//    - message content - <len> bytes

#define QIO_MSG_LEN_SIZE 4 // same as sizeof(unsigned int)
//#define QIO_MSG_DATA_MAX_LEN 128

struct qio_raw_msg {
	unsigned int  m_len;
//	char 		  m_data[QIO_MSG_DATA_MAX_LEN];
	char* 		  m_dataBuf; // dynamic allocation!!

	~qio_raw_msg() {if (m_len > 0) delete(m_dataBuf); }
};

class qSim_qio_socket_server_cb {
public:
	qSim_qio_socket_server_cb() {};
	virtual ~qSim_qio_socket_server_cb() {};
	virtual void in_message_cb(qio_raw_msg*) = 0;
	virtual void out_message_cb(qio_raw_msg*) = 0;
};

class qSim_qio_socket_server : public qSim_qsocket_server {
	public:
		qSim_qio_socket_server(bool verbose=false);
		virtual ~qSim_qio_socket_server();

//		void set_dataIn_callback_function(std::function<void ()> fn);
//		void set_dataOut_callback_function(std::function<void ()> fn);
		void set_dataInOut_callback(qSim_qio_socket_server_cb* cb);

	private:
//		std::function<void ()> m_dataIn_cbFunction;
//		std::function<void ()> m_dataOut_cbFunction;
		qSim_qio_socket_server_cb* m_dataInOut_cb;

		std::thread m_clnThr_id;

		virtual void doLoop();
		void doLoop_client();

		bool receive_data(struct qio_raw_msg* msg);
		bool send_data(struct qio_raw_msg* msg);

};

/////////////////////////////////////////////////////////////////////
//
//class qSim_qreg_qsocket_client : public qSim_qsocket_client {
//public:
//	qSim_qreg_qsocket_client(bool verbose=false);
//	virtual ~qSim_qreg_qsocket_client();
//
//	int connect_server(const char* server_ipaddr, int server_port=QBUS_DEFAULT_PORT);
//
//	bool receive_data(qreg_msg* msg);
//	bool send_data(qreg_msg* msg);
//
////private:
////	...
//};

#endif /* QSIM_QIO_SOCKET_H_ */
