# -*- coding: utf-8 -*-
"""
Created on Thu Mar  9 16:16:01 2023

@author: Casonato

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
* qSim client and server data exchange performance test ---
*
"""

import numpy as np
import matplotlib.pyplot as plt
import time

import qSim_qcln_access_client as qacc
import qSim_qcln_qasm as qasm


# ******************************************************************
# test function
# ******************************************************************

# potential message ids - transformations more likely than measures -> only them used!
TST_MSG_IDS = [qasm.QASM_MSG_ID_QREG_ST_TRANSFORM,
               # qasm.QASM_MSG_ID_QREG_ST_TRANSFORM,
               # qasm.QASM_MSG_ID_QREG_ST_TRANSFORM,
               # qasm.QASM_MSG_ID_QREG_ST_TRANSFORM,
               # qasm.QASM_MSG_ID_QREG_MEASURE,
               ]

def test_qcln_access_client_socket_perfo(tot_msg=5, n=5, verbose=False):
    # qSim client-server socket data exchange performance test
    print('*** qSim Client-Server socket data exchange performance test ***')
    print()
    
    # instantiate class and connect
    qcln = qacc.qSim_qcln_access_client(verbose=verbose)
    qcln.connect()
    print('=> isConnected:', qcln.isConnected())
    print()

    # allocate a test qureg - same for all messages!
    qr_h = qcln.qreg_allocate(n)
    print('=> qureg allocated - tot_qubits:', n)
    print()
    
    msg_tm_vec = []
    msg_sz_vec = []
    tot_trf = 0
    tot_msr = 0
    i_block = 10
    for i in range(tot_msg):
        if i % i_block == 0:
            print('iteration #', i)
            
        # select a random message 
        idx = np.random.randint(0, len(TST_MSG_IDS))
        msg_id = TST_MSG_IDS[idx]

        # apply it & get response
        if msg_id == qasm.QASM_MSG_ID_QREG_ST_TRANSFORM:
            msg_tm, msg_sz = apply_transformation(qcln, qr_h, verbose)
            tot_trf += 1
        else: #elif msg_id == qasm.QASM_MSG_ID_QREG_MEASURE:
            msg_tm, msg_sz = apply_measure(qcln, qr_h, n, verbose)
            tot_msr += 1
                    
        # update elapsed time and exchanged data info
        msg_tm_vec.append(msg_tm)    
        msg_sz_vec.append(msg_sz/1024) # to KB

    # release qureg and disconnect
    qcln.qreg_release(qr_h)    
    print('=> qr-release')
    print()

    qcln.disconnect()
    print()

    # calculate other stats
    msg_tm_vec = np.array(msg_tm_vec)
    msg_sz_vec = np.array(msg_sz_vec)
    kb_sec_rate_vec = msg_sz_vec/msg_tm_vec
    msg_sec_rate_avg = np.mean(1./msg_tm_vec)
    kb_sec_rate_avg = np.mean(kb_sec_rate_vec)
    
    # plot results
    plt.figure()
    plt.plot(msg_tm_vec, label='msg-time')
    plt.plot([np.mean(msg_tm_vec)]*tot_msg, 'r', label='msg-time-avg')
    plt.legend()
    plt.grid()
    plt.xlabel('loop #')
    plt.ylabel('sec')
    plt.title('message 2-way transfer time')
    plt.show()
    print()

    plt.figure()
    plt.plot(msg_sz_vec, label='msg-size')
    plt.plot([np.mean(msg_sz_vec)]*tot_msg, 'r', label='msg-size-avg')
    plt.legend()
    plt.grid()
    plt.xlabel('loop #')
    plt.ylabel('KB')
    plt.title('message 2-way size')
    plt.show()
    print()
        
    plt.figure()
    plt.plot(kb_sec_rate_vec, label='trf-rate')
    plt.plot([np.mean(kb_sec_rate_vec)]*tot_msg, 'r', label='trf-rate-avg')
    plt.legend()
    plt.grid()
    plt.xlabel('loop #')
    plt.ylabel('KB/sec')
    plt.title('message 2-way transfer rate')
    plt.show()
    print()
    
    print('*** Stats:')
    print()
    print('...tot_trf msg:', tot_trf)
    print('...tot_msr msg:', tot_msr)
    print()
    print('...msg/sec avg:', ('%6.3f' % msg_sec_rate_avg))
    print('...kb/sec avg: ', ('%6.3f' % kb_sec_rate_avg))
    print()
    
    print('done.')
    print()
    
# ------------------------------------------------------    

def apply_transformation(qcln, qr_h, verbose=False):
    # set qureg transformation params
    f_type = np.random.randint(0, qasm.QASM_F_TYPE_CZ+1)
    f_rep = 1
    f_lsq = 0
    f_args = None
    fu_type = qasm.QASM_F_TYPE_NULL
    fu_size = 0
    if qasm.QASM_F_IS_Q1(f_type):
        # 1-qubit case
        f_size = 2
        if qasm.QASM_F_IS_Q1_PARAMETRIC(f_type):
            f_args = [.123]
    else:        
        # 2-qubit case
        f_size = 4
        if f_type == qasm.QASM_F_TYPE_CU:
            # define controlled 1-qubit function
            fu_type = qasm.QASM_F_TYPE_H
            fu_size = 2

    # apply and get result and stats        
    start_tm = time.time()
    res, diag = qcln.qreg_state_transform(qr_h, f_type, f_size, f_rep, f_lsq, 
                                    f_args=f_args, fu_type=fu_type, fu_size=fu_size, 
                                    diag=True)
    end_tm = time.time() # take timing...
    msg_tm = end_tm - start_tm
    msg_sz = diag[0] + diag[1] # cumulate request and response message sizes
    if verbose:
        print('=> qr-transformation [', f_type, '] ... res:', res, 'tm:', msg_tm, 'sz:', msg_sz)
        print()            
    return msg_tm, msg_sz

# ---------------------

def apply_measure(qcln, qr_h, n, verbose=False):
    # set qureg measurement params
    q_idx = 0
    q_len = n - 1
    m_rand = True
    st_coll = False

    # apply and get result and stats        
    start_tm = time.time()
    m_st, _, _, diag = qcln.qreg_measure(qr_h, q_idx, q_len, m_rand, st_coll, diag=True)
    end_tm = time.time() # take timing...
    msg_tm = end_tm - start_tm
    msg_sz = diag[0] + diag[1] # cumulate request and response message sizes
    if verbose:
        print('=> qr-measure... m_st:', m_st, 'tm:', msg_tm, 'sz:', msg_sz)
        print()            
    return msg_tm, msg_sz

# ---------------------

