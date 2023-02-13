#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat May 28 21:58:57 2022

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
* Support classes for Quantum Computing in Python - Q-Sim Access Client
* 
* Q-Sim access client handling class, wrapping low level socket based 
* connection to qSim server and raw message exchange.
*
* => Entry point class for other mypqy applications using qSim.
*
* Change History
* 
* Ver.  Date      Reason
* ------------------------------------------------------------------------
* 1.0   May-2022  Module creation.
* 1.1   Nov-2022  Moved to qSim v2 and renamed to qSim_qcln_access_client.
* 1.2   Feb-2023  Fixed terminology for measure output (probability returned).
*                 Supported qureg state expectations calculation.
* 
* ------------------------------------------------------------------------
*
"""


import numpy as np

import qSim_qcln_socket as qsock
import qSim_qcln_qasm as qasm


# --------------------------------------------------------

# # constants definition
# QSIM_SERVER_IP_ADDR = '127.0.0.1'
# QSIM_SERVER_PORT    = 27020

# --------------------------------------------------------

# ==> qSim access client handling
class qSim_qcln_access_client(): 
   
    def __init__(self, id_mnem='qSim-qcln-client', verbose=False):
        # class constructor 
        self.m_qsock = None
        self.m_token = None
        self.m_id = id_mnem
        self.m_counter = 1
        
        self.m_verbose = verbose
    
    # ------------------
    
    def connect(self):
        # setup connection to qSim server
        # and perform client registration for getting the token

        # reset token
        self.m_token = None

        # socket client setup
        self.m_qsock = qsock.qSim_qcln_socket()
        self.m_qsock.connect() # use default ip-address and port....
        if self.m_verbose:
            print('qSim-access - client socket connected')
        
        # client registration

        # (1) send request
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = 0 # not used here
        msg_reg.m_id = qasm.QASM_MSG_ID_REGISTER
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_ID, self.m_id)
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        if self.m_verbose:
            print('qSim-access - registration request sent - id:', self.m_id)
            msg_reg.dump()
            print('raw_msg:', raw_msg)
            print()

        # (2) read response 
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        if self.m_verbose:
            print('qSim-access - server registration response received')
            msg_res.dump()
            print('raw_msg:', raw_msg)
            print()

        # (3) check result and extract token
        res = self.check_response_message(msg_res)
        if res:
            # request ok - get token
            self.m_token = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_TOKEN)
            if self.m_verbose:
                print('qSim-access - registration OK - token:', self.m_token)
        return res
    
    def disconnect(self):
        # deregister client from server and perform connection release 

        # client deregistration

        # (1) send request
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = 0 # not used here
        msg_reg.m_id = qasm.QASM_MSG_ID_UNREGISTER
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        if self.m_verbose:
            print('qSim-access - deregistration request sent - token:', self.m_token)

        # reset token
        self.m_token = None
        
        # (2) read response for a clean disconnection - not mandatory
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)

        # socket client release
        self.m_qsock.disconnect() 
        if self.m_verbose:
            print('qSim-access - client socket disconnected')

    def isConnected(self):
        # check is the client is connected and ready to go
        return not (self.m_token is None)
            
    # --------------------------------------------------------------
    # --------------------------------------------------------------
    # message reception and transmission - all cases handled
    
    def qreg_allocate(self, qn):
        # allocate qureg of given size
        
        # send message
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = self.m_counter
        msg_reg.m_id = qasm.QASM_MSG_ID_QREG_ALLOCATE
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_QN, str(qn))
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        self.m_counter += 1
        if self.m_verbose:
            print('qSim-access - qureg allocation request sent - qn:', qn)
        
        # receive response
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        res = self.check_response_message(msg_res)
        if res:
            # request ok - get qreg handler
            qr_h = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_QREG_H)
            if self.m_verbose:
                print('qSim-access - qreg allocation OK - qr_h:', qr_h)
        else:
            print('ERROR - qureg allocation failed!!!')
            qr_h = None
        return qr_h
 
    # -------
    
    def qreg_release(self, qr_h):
        # release given qureg handler
        
        # send message
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = self.m_counter
        msg_reg.m_id = qasm.QASM_MSG_ID_QREG_RELEASE
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_H, str(qr_h))
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        self.m_counter += 1
        if self.m_verbose:
            print('qSim-access - qureg release request sent - qr_h:', qr_h)
        
        # receive response
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        res = self.check_response_message(msg_res)
        if res:
            # request ok 
            if self.m_verbose:
                print('qSim-access - qreg release OK')
        return res

    # -------
    
    def qreg_state_reset(self, qr_h):
        # reset state for given qureg handler
        
        # send message
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = self.m_counter
        msg_reg.m_id = qasm.QASM_MSG_ID_QREG_ST_RESET
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_H, str(qr_h))
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        self.m_counter += 1
        if self.m_verbose:
            print('qSim-access - qureg state reset request sent - qr_h:', qr_h)
        
        # receive response
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        res = self.check_response_message(msg_res)
        if res:
            # request ok 
            if self.m_verbose:
                print('qSim-access - qreg state reset OK')
        return res

    # -------
    
    def qreg_state_set(self, qr_h, st_idx=None, st_vals=None):
        # set state for given qureg handler
        
        # send message
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = self.m_counter
        msg_reg.m_id = qasm.QASM_MSG_ID_QREG_ST_SET
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_H, str(qr_h))
        if not st_idx is None:
            st_idx_str = str(st_idx)
            msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_STIDX, st_idx_str)            
        elif not st_vals is None:
            st_vals_str = self.states_complex_2_string(st_vals)
            # print('st_vals:', st_vals, '-> st_vals_str:', st_vals_str)
            msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_STVALS, st_vals_str)
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        self.m_counter += 1
        if self.m_verbose:
            print('qSim-access - qureg state set request sent - qr_h:', qr_h)
        
        # receive response
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        res = self.check_response_message(msg_res)
        if res:
            # request ok 
            if self.m_verbose:
                print('qSim-access - qreg state set OK')
        return res
        
    # -------

    def qreg_state_transform(self, qr_h, f_type, f_size, f_rep, f_lsq, f_crng=[], f_trng=[], f_args=None, 
                             fu_type=qasm.QASM_F_TYPE_NULL, fu_size=0):
        # print('qreg_state_transform...f_args:', f_args)
        if qasm.QASM_F_IS_Q2(fu_type):
            # remove arg #3 (qureg size - not needed for QASM!!)
            f_args = f_args[:2]
        
        # prepare message
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = self.m_counter
        msg_reg.m_id = qasm.QASM_MSG_ID_QREG_ST_TRANSFORM
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_H, str(qr_h))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_F_TYPE, str(f_type))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_F_SIZE, str(f_size))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_F_REP, str(f_rep))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_F_LSQ, str(f_lsq))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_F_CRANGE, self.index_range_to_string(f_crng))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_F_TRANGE, self.index_range_to_string(f_trng))
        if fu_type != qasm.QASM_F_TYPE_NULL:
            msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_F_UTYPE, str(fu_type))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_F_ARGS, self.fargs_to_string(f_args, f_type, fu_type))
        
        # send message
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        self.m_counter += 1
        if self.m_verbose:
            print('qSim-access - qureg state transformation request sent - qr_h:', qr_h)
            print('raw_msg:', raw_msg)
        
        # receive response
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        res = self.check_response_message(msg_res)
        if res:
            # request ok 
            if self.m_verbose:
                print('qSim-access - qreg state transformation OK')
        return res
    
    # -------
    
    def qreg_state_getValues(self, qr_h):
        # get state values for given qureg handler
        
        # send message
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = self.m_counter
        msg_reg.m_id = qasm.QASM_MSG_ID_QREG_ST_PEEK
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_H, str(qr_h))
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        self.m_counter += 1
        if self.m_verbose:
            print('qSim-access - qureg state get values request sent - qr_h:', qr_h)
        
        # receive response
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        # msg_res.dump()
        res = self.check_response_message(msg_res)
        if res:
            # request ok - get qreg state values
            qr_st_str = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_QREG_STVALS)
            # print(qr_st_str)
            # convert to complex array
            qr_st = self.states_string_2_complex(qr_st_str)
            if self.m_verbose:
                print('qSim-access - qreg state get values OK')
        else:
            qr_st = None
        return qr_st
            
    # -------
    
    def qreg_measure(self, qr_h, q_idx, q_len, m_rand, st_coll):
        # state measurement for given qureg handler
        
        # send message
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = self.m_counter
        msg_reg.m_id = qasm.QASM_MSG_ID_QREG_MEASURE
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_H, str(qr_h))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_MQIDX, str(q_idx))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_MQLEN, str(q_len))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_MRAND, str(int(m_rand)))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_MCOLL, str(int(st_coll)))
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        self.m_counter += 1
        if self.m_verbose:
            print('qSim-access - qureg state get values request sent - qr_h:', qr_h)
        
        # receive response
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        res = self.check_response_message(msg_res)
        if res:
            # request ok - get qreg measurement values
            
            # measured state index - convert to int
            m_st_str = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_QREG_MSTIDX)
            m_st = int(m_st_str)
            
            # measurement state probability - convert to double array
            m_pr_str = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_QREG_MPR)
            # print('m_pr_str:', m_pr_str)
            if not m_pr_str is None:
                m_pr = float(m_pr_str)
            else:
                m_pr = None
            
            # measurement residual qureg states - optional - convert to double array
            m_vec_str = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_QREG_MSTIDXS)
            # print('m_exp_str:', m_vec_str)
            if not m_vec_str is None:
                m_vec = eval(m_vec_str)
            else:
                m_vec = None

            if self.m_verbose:
                print('qSim-access - qreg measurement OK - m_st:', m_st)
        else:
            m_st = None
            m_pr = None
            m_vec = None
        return m_st, m_pr, m_vec
            
    # -------
    
    def qreg_expectation(self, qr_h, st_idx, q_idx, q_len, q_obs_op):
        # state expectation for given qureg handler
        
        # send message
        msg_reg = qasm.qSim_qcln_qasm()
        msg_reg.m_counter = self.m_counter
        msg_reg.m_id = qasm.QASM_MSG_ID_QREG_EXPECT
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_TOKEN, self.m_token)
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_H, str(qr_h))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_ESTIDX, str(st_idx))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_EQIDX, str(q_idx))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_EQLEN, str(q_len))
        msg_reg.add_param_tagValue(qasm.QASM_MSG_PARAM_TAG_QREG_EOBSOP, str(q_obs_op))
        raw_msg = msg_reg.to_raw_message()
        self.m_qsock.send_raw_message(raw_msg)
        self.m_counter += 1
        if self.m_verbose:
            print('qSim-access - qureg state get values request sent - qr_h:', qr_h)
        
        # receive response
        raw_msg = self.m_qsock.receive_raw_message()
        msg_res = qasm.qSim_qcln_qasm()
        msg_res.from_raw_message(raw_msg)
        res = self.check_response_message(msg_res)
        if res:
            # request ok - get qreg expectation values
            
            # expectation value - convert to double
            m_exp_str = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_QREG_ESTVAL)
            m_exp = float(m_exp_str)
            
            if self.m_verbose:
                print('qSim-access - qreg expectation OK - m_exp:', m_exp)
        else:
            m_exp = None
        return m_exp
            
    # -------------------------
    # helper methods

    def check_response_message(self, msg_res):
        # extract and check response return code
        res = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_RESULT)
        if res != qasm.QASM_MSG_PARAM_VAL_OK:
            err = msg_res.get_param_valueByTag(qasm.QASM_MSG_PARAM_TAG_ERROR)
            print('qSim-access - ERROR!! - reason:', err)
        return (res == qasm.QASM_MSG_PARAM_VAL_OK)


    # -----------------
    
    # string format: (val1) (val2) ... (valn)
    
    @staticmethod
    def states_string_2_complex(qr_st_str):
        qr_st_tk = qr_st_str.split(',')
        qr_st = []
        for i in range(0, len(qr_st_tk), 2):            
            tk_i0 = qr_st_tk[i].strip()
            tk_i1 = qr_st_tk[i+1].strip()
            # print(i, tk_i0, tk_i1)
            st_r = eval(tk_i0[1:])
            st_i = eval(tk_i1[:-1])
            # print('...', st_r, st_i)
            qr_st.append(st_r + st_i*1.j)
        qr_st = np.array(qr_st, dtype=complex)
        return qr_st
    
    @staticmethod
    def states_complex_2_string(qr_st):
        qr_st_str = ''
        for st_i in qr_st:
            qr_st_str += '(' + str(np.real(st_i)) + ', ' + str(np.imag(st_i)) + ') '
        return qr_st_str

    @staticmethod
    def index_range_to_string(rng_val):
        if rng_val is None:
            str_val = '(-1, -1)'
        elif len(rng_val) == 0:
            str_val = '(-1, -1)'
        elif len(rng_val) == 2:
            str_val = str(tuple(rng_val))
        else:
            str_val = '(' + str(rng_val[0]) + ')'
        return str_val
            
    def fargs_to_string(self, fargs, ftype, futype):
        # coding format: ASCII string with "[", "]" as start/stop tags and with "," as field separators
        #   <fargs_string> =  "[<arg-1>,<arg-2, ... <arg-n>]"
        # 
        # with <arg-x> = <value>"|"<type> and 
        # - <value> as <number> for scalar or (<number>, <number>) for range
        # - <type> as "I" for integers or "D" for doubles or "R" for ranges
        if not fargs is None:
            # function arguments to handle
            fargs_str = '[' # starting tag
            if qasm.QASM_F_IS_Q1(ftype):
                # single function argument only expected (if any)
                if len(fargs) > 0:
                    fargs_str += qSim_qcln_access_client.farg_double_2_string(fargs[0])
                
            elif qasm.QASM_F_IS_Q2(ftype):
                if ftype == qasm.QASM_F_TYPE_CU:
                    # 1-qubit function-U case 
                    # -> function-U argument only expected (if any)
                    if len(fargs) > 0:
                        fargs_str += qSim_qcln_access_client.farg_double_2_string(fargs[0])
                else:
                    # 2-qubit function case - no arguments expected
                    pass

            elif qasm.QASM_F_IS_Qn(ftype):
                if qasm.QASM_F_IS_Q1(futype):
                    # 1-qubit function-U case 
                    # -> function-U argument only expected (if any)
                    if len(fargs) > 0:
                        fargs_str += qSim_qcln_access_client.farg_double_2_string(fargs[0])
                else:
                    # 2-qubit function case 
                    # -> control & target ranges + function-U argument expected
                    fargs_str += qSim_qcln_access_client.farg_range_2_string(fargs[0])
                    fargs_str += qSim_qcln_access_client.farg_range_2_string(fargs[1])
                    if len(fargs) > 2:
                        fargs_str += qSim_qcln_access_client.farg_double_2_string(fargs[2])
                
            # ending tag
            if fargs_str[-1] == ',':
                fargs_str = fargs_str[:-1] + ']' # skip last ","
            else:
                fargs_str += ']'
                
        else:
            # no function arguments - empty string
            fargs_str = '[]'
            
        return fargs_str        

    @staticmethod
    def farg_double_2_string(farg_val):
        return str(farg_val) + '|D,'

    @staticmethod
    def farg_range_2_string(farg_rng):
        return qSim_qcln_access_client.index_range_to_string(farg_rng)+'|R,'


#######################################################################

    
