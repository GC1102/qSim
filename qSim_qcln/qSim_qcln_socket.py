#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat May 28 17:46:04 2022

@author: gianni

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

"""

"""
* ------------------------------------------------------------------------
*
* Support classes for Quantum Computing in Python - Q-Sim Socket Client
* 
* Q-Sim socket client handling class, for connecting to qSim server using
* a TCP/IP socket and exchanging raw messages based on qSim handshake. 
*
* Ref: Q-Sim::Any Interface in "qSim_How_To_User_Guide_v1.1"
*
* Change History
* 
* Ver.  Date      Reason
* ------------------------------------------------------------------------
* 1.0   May-2022  Module creation.
* 1.1   Nov-2022  Moved to qSim v2 and renamed to qSim_qcln_socket.
* 
* ------------------------------------------------------------------------
*
"""


import socket


# --------------------------------------------------------

# constants definition
QSIM_SERVER_IP_ADDR = '127.0.0.1'
QSIM_SERVER_PORT    = 27020

# --------------------------------------------------------

# ==> qSim socket client handling
class qSim_qcln_socket(): 
   
    def __init__(self, verbose=False):
        # class constructor 
        self.m_sock_client = None
        self.m_verbose = verbose
    
    # ------------------
    
    def connect(self, ipAddr=QSIM_SERVER_IP_ADDR, port=QSIM_SERVER_PORT):
        # setup socket and connect to qSim server
        self.m_sock_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.m_sock_client.connect((ipAddr, port))
        if self.m_verbose:
            print('qSim-socket-client - server connected @ iaAddr:', ipAddr, ' - port:', port)
            print()

    def disconnect(self):
        # disconect from server
        self.m_sock_client.close()
        if self.m_verbose:
            print('qSim-client disconnected')
            print()

    # -------------------------
    # message reception and transmission

    # handshake for raw message exchange:
    # (1) msg len transfer as binary integer - 4 bytes little-endian format
    # (2) msg data transfer as binary string stream - variable length as per (1)
    
    def receive_raw_message(self):
        # read a raw message from server
        
        # get message length
        from_server = self.m_sock_client.recv(4)
        msg_len = int.from_bytes(from_server, 'little')
        if self.m_verbose:
            print('server raw message - len:', msg_len)
        # print('raw_message...len field size:', len(from_server), ' val:', msg_len)
    
        # get message body - in a loop!
        msg_str = ''
        while len(msg_str) < msg_len:
            from_server = self.m_sock_client.recv(msg_len)
            msg_str += from_server.decode()
        if self.m_verbose:
            print('server raw message - data:', msg_str)
        # print('raw_message...data field size:', len(from_server), ' val (first 100!):', msg_str[:100])
        
        return msg_str
        
    def send_raw_message(self, msg_str):
        # send a raw message from server
        
        # send message length
        msg_len = len(msg_str)
        self.m_sock_client.send(msg_len.to_bytes(4, byteorder='little'))
        
        # send message body
        tot_sent = 0
        while (tot_sent < msg_len):
            tot_sent += self.m_sock_client.send(msg_str.encode())
        # print('raw_message...data field size:', msg_len, ' tot_sent:', tot_sent)
        
        if self.m_verbose:
            print('qSim-client - message sent to server')
    

# ******************************************************************
# test functions
# ******************************************************************

def test_qcln_socket():
    qs = qSim_qcln_socket(verbose=True)
    qs.connect()
    
    msg_req = "0|1|name=myqpy-client:"
    print('==> msg to send:', msg_req)
    qs.send_raw_message(msg_req)
    print()
    
    msg_res = qs.receive_raw_message()
    print('==> msg received:', msg_res)
    print()
    
    qs.disconnect()
    
    print('done.')
    
    
    
   
