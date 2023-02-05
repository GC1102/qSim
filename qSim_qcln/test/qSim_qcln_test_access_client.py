#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat May 28 21:06:16 2022

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
* Test connection and request exchange between qSim client and server ---
*
"""


import qSim_qcln_access_client as qacc
import qSim_qcln_qasm as qasm


# ******************************************************************
# test functions
# ******************************************************************

def test_qcln_access_client_simple():
    # basic test
    print('*** qSim Client Access module basic test ***')
    print()
    
    # instantiate class and connect
    qcln = qacc.qSim_qcln_access_client(verbose=True)
    qcln.connect()
    print('=> isConnected:', qcln.isConnected())
    print()
    
    # allocate a test qureg and peek states
    qr_h = qcln.qreg_allocate(3)
    qr_st = qcln.qreg_state_getValues(qr_h)
    print('=> qr-states:', qr_st)
    print()

    # apply a transformation
    f_type = qasm.QASM_F_TYPE_H
    f_size = 2
    f_rep = 1
    f_lsq = 1
    res = qcln.qreg_state_transform(qr_h, f_type, f_size, f_rep, f_lsq)
    print('=> qr-transformation res:', res)
    print()

    # peek states
    qr_st = qcln.qreg_state_getValues(qr_h)
    print('=> qr-states:', qr_st)
    print()

    # release qureg and disconnect
    qcln.qreg_release(qr_h)    
    print('=> qr-release')
    print()

    qcln.disconnect()
    print()
    
    print('done.')
    print()
    
# ------------------------------------------------------    

def test_qcln_access_client_all():
    print('*** qSim Client Access module test ***')
    print()
        
    qcln = qacc.qSim_qcln_access_client(verbose=True)
    res = qcln.connect()
    print('qClient connection - res:', res)
    print()    
    if not res:
        print('--> connection error!! cannot continue')
        return

    # test loop with all request types 
    req = 1
    while req > 0:
        # get user input request
        req = test_qcln_access_select_request()
        
        # execute request
        test_qcln_access_exec_request(qcln, req)
    print()
    
    # loop complete - close client
    qcln.disconnect()
    
    print('done.')
    print()    

# ----------

def test_qcln_access_select_request():
    # allow user selection of the access client request to execute
    print('--> qClient Access Requests <--')
    print()
    print('(1) - qreg_allocate')
    print('(2) - qreg_release')
    print('(3) - qreg_state_reset')
    print('(4) - qreg_state_set')
    print('-----------------------')
    print('(5) - qreg_state_transform')
    print('-----------------------')
    print('(6) - qreg_state_getValues')
    print('(7) - qreg_measure')
    print('-----------------------')
    print('(0) - exit test')
    print('-----------------------')
    req = input('Selection? ')
    req = int(req)
    return req

# ----------

REQ_QREG_NONE = 0
REQ_QREG_ALLOCATE = 1
REQ_QREG_RELEASE = 2
REQ_QREG_STATE_RESET = 3
REQ_QREG_STATE_SET = 4
REQ_QREG_STATE_TRANSFORM = 5
REQ_QREG_STATE_GET = 6
REQ_QREG_STATE_MEASURE = 7

def test_qcln_access_exec_request(qcln, req):
    # perform access client request execution
    if req == REQ_QREG_ALLOCATE:
        # qureg allocate - get size and execute        
        qn = input('qureg_allocate - qureg size? ')
        qn = int(qn)
        qr_h = qcln.qreg_allocate(qn)
        print('qureg_allocate done - qr:', qr_h)
        print()
        
    elif req == REQ_QREG_RELEASE:
        # qureg release - get handler and execute
        qr_h = input('qureg_release - qureg handler? ')
        qr_h = int(qr_h)
        res = qcln.qreg_release(qr_h)
        print('qureg_release done - res:', res)
        print()
    
    elif req == REQ_QREG_STATE_RESET:
        # qureg state reset - get handler and execute
        qr_h = input('qreg_state_reset - qureg handler? ')
        qr_h = int(qr_h)
        res = qcln.qreg_state_reset(qr_h)
        print('qreg_state_reset done - res:', res)
        print()
    
    elif req == REQ_QREG_STATE_SET:
        # qureg state set - get handler and other args and execute
        qr_h = input('qreg_state_set - qureg handler? ')
        qr_h = int(qr_h)
        st_idx = input('pure state index? ')
        st_idx = int(st_idx)
        res = qcln.qreg_state_set(qr_h, st_idx=st_idx)
        print('qreg_state_set done - res:', res)
        print()
    
    elif req == REQ_QREG_STATE_TRANSFORM:
        # qureg state transform - get handler and other args and execute
        # only 1-qubit and 2-qubit standalone q-functions tested for simplicity
        # - controlled and blocks direct extension using the User Guide
        qr_h = input('qreg_state_transform - qureg handler? ')
        qr_h = int(qr_h)        
        f_type = input('q-function type? ') # see values in the User Guide
        f_type = int(f_type)
        if qasm.QASM_F_IS_Q1_PARAMETRIC(f_type):
            f_args = input('q-function argument? ')
            f_args = [float(f_args)]
        else:
            f_args = None
        f_size = input('q-function size? ') # see values in the User Guide
        f_size = int(f_size)
        f_rep = input('q-function repetitions? ')
        f_rep = int(f_rep)
        f_lsq = input('q-function LSQ index? ')
        f_lsq = int(f_lsq)
    # def qreg_state_transform(self, qr_h, f_type, f_size, f_rep, f_lsq, f_crng=[], f_trng=[], f_args=None, 
    #                          fu_type=qasm.QASM_F_TYPE_NULL, fu_size=0):
        res = qcln.qreg_state_transform(qr_h, f_type, f_size, f_rep, f_lsq, f_args=f_args)
        print('qreg_state_transform done - res:', res)
        print()
   
    elif req == REQ_QREG_STATE_GET:
        # qureg state get values - get handler and execute
        qr_h = input('qreg_state_getValues - qureg handler? ')
        qr_h = int(qr_h)
        st_vals = qcln.qreg_state_getValues(qr_h)
        print('qreg_state_getValues done - st_vals:', st_vals)
        print()

    elif req == REQ_QREG_STATE_MEASURE:
        # qureg state measure - get handler and other args and execute
        qr_h = input('qreg_measure - qureg handler? ')
        qr_h = int(qr_h)
        q_idx = input('qubit start index? ')
        q_idx = int(q_idx)
        q_len = input('sub-qureg size? ')
        q_len = int(q_len)
        m_rand = input('random measure flag (y/n)? ')
        m_rand = int(m_rand=='y')
        st_coll = input('qureg collapse flag (y/n)? ')
        st_coll = int(st_coll=='y')
    # def qreg_measure(self, qr_h, q_idx, q_len, m_rand, st_coll):
        m_st, m_pr, m_vec = qcln.qreg_measure(qr_h, q_idx, q_len, m_rand, st_coll)
        print('qreg_measure done - m_st:', m_st, 'm_pr:', m_pr, 'm_vec:', m_vec)
        print()

    elif req == REQ_QREG_NONE:
        pass
    
    else:
        # wrong request!!
        print('ERROR - unhandled request ['+str(req)+']!!')
        print()

# ----------
