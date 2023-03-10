#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu May 26 15:01:27 2022

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
*****  Quantum Circuit Class Testing  *****
*
* qSim socket client test
*
"""

import socket
import time
        

def qSim_qcln_socket_client():
    print('*** qSim test client ***')
    print()
    
    ipAddr = '127.0.0.1'
    port = 27020
    print('qSim server info - ipAddr:',ipAddr, 'port:', port)
    print()

    # connecting    
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((ipAddr, port))
    print('qSim server connected...')    
    print()

    time.sleep(1)
    
    # sending a registration request
    msg = "0|1|name=myqpy-client:"
    msg_len = len(msg)
    client.send(msg_len.to_bytes(4, byteorder='little'))
    client.send(msg.encode())
    print('client request - len: ', msg_len, 'msg:', msg)
    print()
    print(' ...waiting for the response')
    
    # reading the answer and extract token
    from_server = client.recv(4)
    rsp_len = int.from_bytes(from_server, 'little')  
    print('server response - len:', rsp_len)

    from_server = client.recv(rsp_len)
    rsp_msg = from_server.decode()
    print('server response - msg:', rsp_msg)
    print()
    
    idx = rsp_msg.find('token=')
    token = rsp_msg[idx+6:-1]
    print('=> token:', token)
    print()
    
    time.sleep(1)
        
    # sending a request
    # -> allocate qreg size 2
    msg = "1|10|token=" + token + ":qn=2:"
    msg_len = len(msg)
    client.send(msg_len.to_bytes(4, byteorder='little'))
    client.send(msg.encode())
    print('client request - len: ', msg_len, 'msg:', msg)
    print()
    print(' ...waiting for the response')
    
    # reading the answer
    from_server = client.recv(4)
    rsp_len = int.from_bytes(from_server, 'little')  
    print('server response - len:', rsp_len)

    from_server = client.recv(rsp_len)
    rsp_msg = from_server.decode()
    print('server response - msg:', rsp_msg)
    print()
    
    time.sleep(1)

    # sending an unregistration request
    msg = "0|2|token=" + token + ":"
    msg_len = len(msg)
    client.send(msg_len.to_bytes(4, byteorder='little'))
    client.send(msg.encode())
    print('client request - len: ', msg_len, 'msg:', msg)
    print()

    # reading the answer =>> OPTIONAL.....
    from_server = client.recv(4)
    rsp_len = int.from_bytes(from_server, 'little')  
    print('server response - len:', rsp_len)

    from_server = client.recv(rsp_len)
    rsp_msg = from_server.decode()
    print('server response - msg:', rsp_msg)
    print()

    # disconnecting
    client.close()
    print('client disconnected')
    print()
    
    print('done.')
    print()
    
    
