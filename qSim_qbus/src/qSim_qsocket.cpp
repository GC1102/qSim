/*
 * qSim_qsocket.cpp
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
 *  1.2   Nov-2022   Disabled diagnostic client activity check (displayed in socket
 *                   loop).
 *  1.3   Mar-2023   Improved server and client socket transfer performance setting
 *                   TCP NODELAY flag (5x improvement).
 *
 *  --------------------------------------------------------------------------
 */


#include <iostream>
#include <cmath>
using namespace std;

#ifndef _WIN32
// linux case
#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/time.h>

#else
// windows case
//#include <Windows.h>

#endif

#include <fcntl.h>

#include "qSim_qsocket.h"


#define QSOCK_SELECT_TIMEOUT_SEC 0
#define QSOCK_SELECT_TIMEOUT_USEC 10000


///////////////////////////////////////////////////////////////////
////  ABSTRACT BASE CLASS
///////////////////////////////////////////////////////////////////

qSim_qsocket_base::qSim_qsocket_base(bool verbose) {
	m_verbose = verbose;
	m_sockfd = 0;

	m_buf = NULL;
	m_buflen = 0;

	m_keepRunning.clear();
}

qSim_qsocket_base::~qSim_qsocket_base() {
	// release buffer space
	if (m_buf != NULL)
		delete m_buf;

	// release socket
	release();
}

///////////////////////////////////////////////////////////////////

int qSim_qsocket_base::init() {
    // open stream oriented socket with internet address
    // also keep track of the socket descriptor

#ifndef _WIN32
	// linux case
	;
#else
	// windows case
    WSADATA wsaData;
    int res;
    res = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (res != 0) {
        cerr << "qbus server - !!Error - WSAStartup failed with error: " << res << endl;
        return QBUS_SOCK_ERROR;
    }
#endif

    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd < 0) {
        cerr << "qSim_socket creation error - errno: " << errno << endl;
        return QBUS_SOCK_ERROR;
    }

    if (m_verbose) {
    	cout << "qSim_socket creation done " << endl;
    }
    return QBUS_SOCK_OK;
}

void qSim_qsocket_base::release() {
	// release socket
	if (m_sockfd > 0) {
#ifndef _WIN32
		// linux case
		close(m_sockfd);
#else
		// windows case
		closesocket(m_sockfd);
		WSACleanup();
#endif
	}
	m_sockfd = 0;
}

///////////////////////////////////////////////////////////////////

void qSim_qsocket_base::startLoop() {
	if (m_sockfd <= 0) {
		cerr << "ERROR - socket not initialized - cannot start message loop!!" << endl << endl;
		return;
	}
	else {
		m_keepRunning.test_and_set();
		m_thr_id = std::thread(&qSim_qsocket_base::doLoop, this);
		m_thr_id.detach();
	}
}

void qSim_qsocket_base::stopLoop() {
	m_keepRunning.clear();
	if (m_thr_id.joinable())
		m_thr_id.join();
}

///////////////////////////////////////////////////////////////////

int qSim_qsocket_base::read_raw_data(int sockFd, unsigned char* buf, int len) {
    // read buffered data up to given len from given socket
    memset(buf, 0, len); //clear the buffer
    int flags = 0;//MSG_DONTWAIT;
    int ret = recv(sockFd, (char*)buf, len, flags);
    if (ret < 0) {
        // error writing data to socket
    	if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
            cerr << "qSim_socket - errno: EAGAIN or EWOULDBLOCK" << endl;
            ret = 0;
    	}
    	else {
    		cerr << "qSim_socket - error reading data from socket - errno: " << errno << endl;
    	}
    }
    return ret;
}

int qSim_qsocket_base::write_raw_data(int sockFd, unsigned char* buf, int len) {
    // write buffered data of given len to given socket
    int ret = send(sockFd, (char*)buf, len, 0);
    if (ret < 0) {
    	// error writing data to socket
    	cerr << "qSim_socket - error writing data to socket - errno: " << errno << endl;
    }
//    cout << "qSim socket... msg_len: " << len << " sent: " << ret << endl;
    return ret;
}



///////////////////////////////////////////////////////////////////
////  SERVER CLASS
///////////////////////////////////////////////////////////////////

qSim_qsocket_server::qSim_qsocket_server(bool verbose) : qSim_qsocket_base(verbose) {
    // initialise client socket handler
    m_cln_sockfd = 0;
}

qSim_qsocket_server::~qSim_qsocket_server() {
    // release client socket - rest done by parent destructor
    release_client();
}

///////////////////////////////////////////////////////////////////

int qSim_qsocket_server::init(string server_ipAddr, int server_port) {
    // call base class init method
    int ret = qSim_qsocket_base::init();
    if (ret == QBUS_SOCK_ERROR)
        return ret;

#ifndef _WIN32
    // linux case
    sockaddr_in servAddr;
#else
    // windows case
    SOCKADDR_IN servAddr;
#endif

//    bzero((char*)&servAddr, sizeof(servAddr));
	memset((char*)&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
//    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_addr.s_addr = inet_addr(server_ipAddr.c_str()); //INADDR_ANY;
	servAddr.sin_port = htons(server_port);

    // bind to the given address & port
#ifndef _WIN32
    // linux case
    int yes = 1;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        cerr << "qSim_qsocket_server setsockopt error for SO_REUSEADDR - errno: " << errno << endl;
        return QBUS_SOCK_ERROR;
    }
    if (setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int)) == -1) {
        cerr << "qSim_qsocket_server setsockopt error for TCP_NODELAY - errno: " << errno << endl;
        return QBUS_SOCK_ERROR;
    }

    if (bind(m_sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        cerr << "qSim_qsocket_server bind error - errno: " << errno << endl;
        return QBUS_SOCK_ERROR;
    }
#else
    // windows case
    bool yes = true;
    if (setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(DWORD)) == SOCKET_ERROR) {
        cerr << "qSim_qsocket_server setsockopt error for TCP_NODELAY - errno: " << errno << endl;
        return QBUS_SOCK_ERROR;
    }

    bind(m_sockfd, (SOCKADDR*)&servAddr, sizeof(servAddr));
#endif

    // listen to incoming connection
    listen(m_sockfd, 0);
    if (m_verbose)
    	cout << "qSim_qsocket_server - listening for incoming connections..." << endl;

    return QBUS_SOCK_OK;
}

///////////////////////////////////////////////////////////////////


int qSim_qsocket_server::accept_client() {
    // accepting clients - 1 client only managed
#ifndef _WIN32
    // linux case
    struct sockaddr_in client_addr;
    socklen_t client_addrSize = sizeof(client_addr);
    m_cln_sockfd = accept(m_sockfd, (struct sockaddr*)&client_addr, &client_addrSize);
#else
    // windows case
    SOCKADDR_IN client_addr;
    int client_addrSize = sizeof(client_addr);
    m_cln_sockfd = accept(m_sockfd, (SOCKADDR *)&client_addr, &client_addrSize);
#endif

    if (m_verbose)
    	cout << "qSim_qsocket_server - client connected!" << endl;

    return QBUS_SOCK_OK;
}

void qSim_qsocket_server::release_client() {
	// release client socket
	if (m_cln_sockfd > 0) {
#ifndef _WIN32
		// linux case
		close(m_cln_sockfd);
#else
		// windows case
        int retval = shutdown(m_cln_sockfd, SD_SEND);
        if (retval == SOCKET_ERROR) {
            cerr <<  "server - client shutdown failed: " << WSAGetLastError() << endl;
        }

		closesocket(m_cln_sockfd);
//		WSACleanup(); --> THIS MUST BE CALLED BY SERVER DESTRUCTOR!!
#endif
	}
	m_cln_sockfd = 0;
}

int qSim_qsocket_server::check_client(int ck_type) {
    // setup flags for select
    struct timeval tv;
    tv.tv_sec = QSOCK_SELECT_TIMEOUT_SEC;
    tv.tv_usec = QSOCK_SELECT_TIMEOUT_USEC;
    
    int sel;   
    if (ck_type == QSOCK_CK_RD) {
        // check for reading data
        fd_set read_sd;
        FD_ZERO(&read_sd);
        FD_SET(m_cln_sockfd, &read_sd);

        // check client activity flag
        fd_set rsd = read_sd;
        sel = select(m_cln_sockfd + 1, &rsd, NULL, NULL, &tv);
    }
    else {
        // check for writing data
        fd_set write_sd;
        FD_ZERO(&write_sd);
        FD_SET(m_cln_sockfd, &write_sd);

        // check client activity flag
        fd_set wsd = write_sd;
        sel = select(m_cln_sockfd + 1, NULL, &wsd, NULL, &tv);        
    }
    
//    if (m_verbose)
//        cout << "checking client activity...sel: " << sel << endl;

    return sel;
}


///////////////////////////////////////////////////////////////////
////  CLIENT CLASS
///////////////////////////////////////////////////////////////////

qSim_qsocket_client::qSim_qsocket_client(bool verbose) :
		qSim_qsocket_base(verbose) {
}

qSim_qsocket_client::~qSim_qsocket_client() {
	// disconnect from server
#ifdef _WIN32
    int retval = shutdown(m_sockfd, SD_SEND);
    if (retval == SOCKET_ERROR) {
        cerr <<  "server - client shutdown failed: " << WSAGetLastError() << endl;
    }
#endif

    release();
}

///////////////////////////////////////////////////////////////////////////

int qSim_qsocket_client::init(string server_ipAddr, int server_port) {
	// call base class init method
	int ret = qSim_qsocket_base::init();
	if (ret == QBUS_SOCK_ERROR)
		return ret;

    // connect to given address & port
#ifndef _WIN32
	// linux case
    struct sockaddr_in server_addr;
#else
    // windows case
    SOCKADDR_IN server_addr;
#endif

    // resolve the server address and port
    server_addr.sin_addr.s_addr = inet_addr(server_ipAddr.c_str()); //INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

#ifndef _WIN32
    // linux case
    int yes = 1;
    if (setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int)) == -1) {
        cerr << "qSim_qsocket_server setsockopt error for TCP_NODELAY - errno: " << errno << endl;
        return QBUS_SOCK_ERROR;
    }

    if (connect(m_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr << "qSim_qsocket_client connect failed - errno:" << errno << endl;
        return QBUS_SOCK_ERROR;
    }

#else
    // windows case
    bool yes = true;
    if (setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(DWORD)) == SOCKET_ERROR) {
        cerr << "qSim_qsocket_server setsockopt error for TCP_NODELAY - errno: " << errno << endl;
        return QBUS_SOCK_ERROR;
    }

    connect(m_sockfd, (SOCKADDR*)&server_addr, sizeof(server_addr));
#endif

    if (m_verbose)
        cout << "qSim_qsocket_client connected to server at ipAddr: " << server_ipAddr
	     << "  port: " << server_port << endl;

    return QBUS_SOCK_OK;
}

///////////////////////////////////////////////////////////////////////////


