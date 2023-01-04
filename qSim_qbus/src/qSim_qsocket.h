/*
 * qSim_qbus.h
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
 *  Created on: Apr 6, 2018
 *      Author: gianni
 *
 * Q-SOCKET class, supporting generic client and server socket handlers.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Apr-2018   Module creation.
 *  1.1   May-2022   Rewritten for simplification and c++ standard classes use.
 *                   Handled client reconnection to server in transparent way.
 *                   Included loop thread as class method.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QSOCKET_H_
#define QSIM_QSOCKET_H_

#include <thread>
#include <cstring>

#ifndef _WIN32
// linux case
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#else
// windows case
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>

#endif


// TCP-IP default IP address and port
#define QBUS_DEFAULT_IPADDR "127.0.0.1"
#define QBUS_DEFAULT_PORT 27015

// return codes
#define QBUS_SOCK_OK    0
#define QBUS_SOCK_ERROR -1

// server check type (read or write)
#define QSOCK_CK_RD 0
#define QSOCK_CK_WR 1


///////////////////////////////////////////////////////////////////
////  ABSTRACT BASE CLASS
///////////////////////////////////////////////////////////////////

class qSim_qsocket_base {
public:
	qSim_qsocket_base(bool verbose);
	virtual ~qSim_qsocket_base();

	virtual int init();
	void release();

	virtual void startLoop();
	virtual void stopLoop();

	virtual bool isRunning() { return m_sockfd != 0; }

protected:

	virtual void doLoop() = 0;

	virtual int read_raw_data(int sockFd, unsigned char* buf, int len);
	virtual int write_raw_data(int sockFd, unsigned char* buf, int len);

	bool m_verbose;

#ifndef _WIN32
	// linux case
    int m_sockfd;

#else
	// windows case
    SOCKET m_sockfd;

#endif

	// buffer for data write (and read) as needed...
	unsigned char* m_buf; // to be allocated in derived classes - as needed
	unsigned m_buflen;

//	// connection data
//	string m_server_ipAddr;
//	int m_server_port;

	// loop thread control
	std::thread m_thr_id;
	std::atomic_flag m_keepRunning = ATOMIC_FLAG_INIT;
};


///////////////////////////////////////////////////////////////////
////  ABSTRACT SERVER CLASS
///////////////////////////////////////////////////////////////////

class qSim_qsocket_server : public qSim_qsocket_base {
	public:
		qSim_qsocket_server(bool verbose=false);
		virtual ~qSim_qsocket_server();

		virtual int init(string server_ipAddr, int server_port);

		virtual int accept_client();
		virtual void release_client();

		virtual int check_client(int ck_type);

	protected:

	#ifndef _WIN32
		// linux case
		int m_cln_sockfd;
	#else
		// windows case
		SOCKET m_cln_sockfd;
	#endif

};


///////////////////////////////////////////////////////////////////
////  ABSTRACT CLIENT CLASS
///////////////////////////////////////////////////////////////////

class qSim_qsocket_client : public qSim_qsocket_base {
	public:
		qSim_qsocket_client(bool verbose=false);
		virtual ~qSim_qsocket_client();

		virtual int init(string server_ipAddr, int server_port);

//	protected:

};


#endif /* QSIM_QSOCKET_H_ */
