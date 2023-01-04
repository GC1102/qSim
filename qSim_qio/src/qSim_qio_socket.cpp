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
 * Q-IO socket class, specializing qBus general class for supporting I/O messages:
 * - control instructions
 * - data input & output
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation cloning former qreg_qsocket class.
 *  1.1   Dec-2022   Performed message length check vs. maximum value, to detect
 *                   and handle communication sync loss with clients.
 *
 *  --------------------------------------------------------------------------
 */


#include <chrono>
#include <thread>
#include <cstring>
#include <bitset>
#include <iostream>
using namespace std;

#include "qSim_qio_socket.h"

// ==> handshake protocol with client:
// (1) the client send a message -> instruction code + data (optional)
// (2) the server read the message and add to qIo IN queue
// (3) the server get an answer from the qIo OUT queue and send it -> result code + data (optional)
// (4) the client read the answer to the initial message

// message handshake control params
// -> polling loop timeout (msec)
// -> max message length (bytes)

#define QIO_LOOP_TIMEOUT_MSEC 100
#define QIO_MSG_MAX_LEN 65536


qSim_qio_socket_server::qSim_qio_socket_server(bool verbose) : qSim_qsocket_server(verbose) {
	// nothing to do...
	m_dataInOut_cb = NULL;
}

qSim_qio_socket_server::~qSim_qio_socket_server(){
	// nothing to do...
}

// --------------------------------

//void qSim_qio_socket_server::set_dataIn_callback_function(std::function<void ()> fn) {
//	m_dataIn_cbFunction = fn;
//}
//
//void qSim_qio_socket_server::set_dataOut_callback_function(std::function<void ()> fn) {
//	m_dataOut_cbFunction = fn;
//}

void qSim_qio_socket_server::set_dataInOut_callback(qSim_qio_socket_server_cb* cb) {
	m_dataInOut_cb = cb;
}

// --------------------------------
// Support methods

// => variable length messages exchanged
//    - message len first - 4 bytes fixed length
//    - message content - <len> bytes

bool qSim_qio_socket_server::receive_data(struct qio_raw_msg* msg) {
	// read message length first
	unsigned char buf[QIO_MSG_LEN_SIZE+1];
	bool res = (read_raw_data(m_cln_sockfd, buf, QIO_MSG_LEN_SIZE) == QIO_MSG_LEN_SIZE);
	if (res) {
		// message length preamble read - read message content now - in a loop to handle buffer limits
		int msg_len = 0;
		memcpy(&msg_len, buf, QIO_MSG_LEN_SIZE);
		// check message length to detect corrupted message data
		if (msg_len > QIO_MSG_MAX_LEN) {
			cerr << "qSim_qio_socket_server - wrong message length detected: " << msg_len << " bytes (max is " << QIO_MSG_MAX_LEN << " bytes) - "
			     << "client connection terminated!!" << endl;
			return false;
		}

		// check passed - setup message structure for reading the message body
		msg->m_len = msg_len;
		if (m_verbose)
			cout << "server_rx len: " << msg->m_len << endl;

		msg->m_dataBuf = new char[(msg->m_len+1)*sizeof(char)]; // count len+1 for adding a \0 termination
		memset(msg->m_dataBuf, 0, (msg->m_len+1)*sizeof(char));
		int tot_len = 0;
		while (tot_len < msg_len) {
			int ret = read_raw_data(m_cln_sockfd, (unsigned char*)msg->m_dataBuf+tot_len, msg->m_len-tot_len);
			tot_len += ret;
//			cout << " ...reading...tot_len:" << tot_len << endl;
			if (ret < 0) {
			    cerr << "qSim_qio_socket_server - receive_data error reading msg data!" << endl;
			    res = false;
			    break;
			}			
		}
		std::string mgs_str = msg->m_dataBuf;
//		cout << "receive_data... full message read - mgs_len: " << msg_len << " msg (20 chars): " << mgs_str.substr(0, 20) << endl;
	}
//	else {
//	    cout << "receive_data error reading msg len!" << endl;
//	}
	return res;
}

bool qSim_qio_socket_server::send_data(struct qio_raw_msg* msg) {
	// send message length first
	unsigned char buf[QIO_MSG_LEN_SIZE];
	int msg_len = msg->m_len;
	memcpy(buf, &msg_len, QIO_MSG_LEN_SIZE);
	bool res = (write_raw_data(m_cln_sockfd, (unsigned char*)buf, QIO_MSG_LEN_SIZE) == QIO_MSG_LEN_SIZE);
	if (res) {
	    // message length preamble sent - send message content now - in a loop to handle buffer limits
  	    int tot_len = 0;
	    while (tot_len < msg_len) {
	        int ret = write_raw_data(m_cln_sockfd, (unsigned char*)msg->m_dataBuf+tot_len, msg->m_len-tot_len);
		tot_len += ret;
//		cout << " ...sending...tot_len:" << tot_len << endl;
		if (ret < 0) {
		    cerr << "send_data error reading msg data!" << endl;
		    res = false;
		    break;
		}
	    }
//	    cout << "send_data... full message sent - mgs_len: " << msg_len << endl;
	}
	return res;
}

// --------------------------------

void qSim_qio_socket_server::doLoop() {
	// connect to client and start handling loop
	cout << "qio-server...doLoop..." << endl;

	// loop for accepting clients
	while (m_keepRunning.test_and_set()) {
		//accept, create a new socket descriptor to handle the new connection with client
		if (m_verbose)
			cout << "Waiting for a client to connect..." << endl;
		this->accept_client();
		if (m_verbose)
			cout << "qsocket-server client accepted..." << endl;

		// start message handling in a separate thread - exit when client disconnect or for errors

		m_clnThr_id = std::thread(&qSim_qio_socket_server::doLoop_client, this);
		m_clnThr_id.detach();

		// sleep a while
		this_thread::sleep_for(chrono::milliseconds(QIO_LOOP_TIMEOUT_MSEC));
	}
	cout << "qSim_qio_socket_server::doLoop done." << endl;
}

void qSim_qio_socket_server::doLoop_client() {
	// handle exchange with client
	cout << "qio-server...doLoop_client..." << endl;

	// setup message structure
	struct qio_raw_msg msg_in;
	struct qio_raw_msg msg_out;

	// loop for exchanging messages with a client
	bool loop = true;
	while (loop) {
		// check client activity flag - for read
		int sel = this->check_client(QSOCK_CK_RD);
//			cout << "qsocket server - check_client RD...sel:" << sel << endl;

		if (sel > 0) {
			// client has performed some activity (sent data or disconnected)

			// check for message to receive
			if (this->receive_data(&msg_in)) {
				// call CB to pass message to qIo class
				if (m_dataInOut_cb != NULL)
					m_dataInOut_cb->in_message_cb(&msg_in);
				if (m_verbose)
					cout << "qsocket server - message received ==> len: " << msg_in.m_len
						 << "  m_dataBuf: " << msg_in.m_dataBuf << endl;
			}
			else {
				 // read 0 bytes or error - client disconnected
				 cout << "errno: " << errno << endl;
				 cout << "read 0-bytes or error - client disconnected..." << endl;
//				 loop = false;
				 break;
			}
		}

		// check client activity flag - for write
		sel = this->check_client(QSOCK_CK_WR);
//			cout << "qsocket server - check_client WR...sel:" << sel << endl;
		if (sel >= 0) {
			// no errors - check out outgoing messages

			// call CB to check for message to send from qIo class
			if (m_dataInOut_cb != NULL)
				m_dataInOut_cb->out_message_cb(&msg_out);

			if (msg_out.m_len > 0) {
				// out message passed - send it
				if (this->send_data(&msg_out)) {
					if (m_verbose)
						cout << "qsocket server - response message sent..." << endl;
				}
				else {
					 // write error - client disconnected.
					 cout << "write 0-bytes or error - client disconnected..." << endl;
//					 loop = false;
					 break;
				}
			}
		}
		else {
			// select error - client disconnected.
			cerr << "select error - client disconnected..." << endl;
//			loop = false;
			break;
		}

		// sleep a while
		this_thread::sleep_for(chrono::milliseconds(QIO_LOOP_TIMEOUT_MSEC));
	}

	// client handling completed
    this->release_client();
	cout << "qSim_qio_socket_server::doLoop_client done." << endl;
}

