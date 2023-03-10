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
* Support classes for Quantum Computing in Python - Q-Sim Instruction Set
* 
* Q-Sim "assembler" instruction set handling class. 
*
* Ref: Q-Sim::Any Interface in "qSim_How_To_User_Guide_v1.1"
*
* Change History
* 
* Ver.  Date      Reason
* ------------------------------------------------------------------------
* 1.0   May-2022  Module creation.
* 1.1   Nov-2022  Moved to qSim v2 and renamed to qSim_qcln_qasm.
*                 Update message tags and function definitions for aligning
*                 to qSim v2.0 QASM.
* 1.2   Feb-2023  Added macro to identify 1-qubit parametric q-functions.
*                 Supported qureg state expectations calculation.
*                 Handled QML function blocks (feature map and q-net).
* 
* ------------------------------------------------------------------------
*
"""

# TODO 
# - check_syntax (where needed)


# --------------------------------------------------------

# control instructions
QASM_MSG_ID_NOPE       = 0
QASM_MSG_ID_REGISTER   = 1
QASM_MSG_ID_UNREGISTER = 2

# qureg handling instructions
QASM_MSG_ID_QREG_ALLOCATE     = 10
QASM_MSG_ID_QREG_RELEASE      = 11
QASM_MSG_ID_QREG_ST_RESET     = 12
QASM_MSG_ID_QREG_ST_SET       = 13
QASM_MSG_ID_QREG_ST_TRANSFORM = 14
QASM_MSG_ID_QREG_ST_PEEK      = 15
QASM_MSG_ID_QREG_MEASURE      = 16
QASM_MSG_ID_QREG_EXPECT       = 17

# responses
QASM_MSG_ID_RESPONSE = 20

# separators
QASM_MSG_FIELD_SEP  = "|"
QASM_MSG_PARAM_SEP  = ":"
QASM_MSG_PARVAL_SEP = "="

# parameter tags
QASM_MSG_PARAM_TAG_ID         = "id"
QASM_MSG_PARAM_TAG_TOKEN      = "token"

QASM_MSG_PARAM_TAG_QREG_QN      = "qr_n"
QASM_MSG_PARAM_TAG_QREG_H       = "qr_h"
QASM_MSG_PARAM_TAG_QREG_STIDX   = "qr_stIdx"
QASM_MSG_PARAM_TAG_QREG_STVALS  = "qr_stVals"
QASM_MSG_PARAM_TAG_QREG_MQIDX   = "qr_mQidx"
QASM_MSG_PARAM_TAG_QREG_MQLEN   = "qr_mQlen"
QASM_MSG_PARAM_TAG_QREG_MRAND   = "qr_mRand"
QASM_MSG_PARAM_TAG_QREG_MCOLL   = "qr_mStColl"
QASM_MSG_PARAM_TAG_QREG_MSTIDX  = "qr_mStIdx"
QASM_MSG_PARAM_TAG_QREG_MPR     = "qr_mStPr"
QASM_MSG_PARAM_TAG_QREG_MSTIDXS = "qr_mStIdxs"
QASM_MSG_PARAM_TAG_QREG_ESTIDX  = "qr_exStIdx"
QASM_MSG_PARAM_TAG_QREG_EQIDX   = "qr_exQidx"
QASM_MSG_PARAM_TAG_QREG_EQLEN   = "qr_exQlen"
QASM_MSG_PARAM_TAG_QREG_EOBSOP  = "qr_exObsOp"
QASM_MSG_PARAM_TAG_QREG_ESTVAL  = "qr_exStVal"

QASM_MSG_PARAM_TAG_F_TYPE     = "f_type"
QASM_MSG_PARAM_TAG_F_SIZE     = "f_size"
QASM_MSG_PARAM_TAG_F_REP      = "f_rep"
QASM_MSG_PARAM_TAG_F_LSQ      = "f_lsq"
QASM_MSG_PARAM_TAG_F_CRANGE   = "f_cRange"
QASM_MSG_PARAM_TAG_F_TRANGE   = "f_tRange"
QASM_MSG_PARAM_TAG_F_UTYPE    = "f_uType"
QASM_MSG_PARAM_TAG_F_ARGS     = "f_args"

QASM_MSG_PARAM_TAG_FBQML_REP      = "fqml_rep"         
QASM_MSG_PARAM_TAG_FBQML_ENTANG   = "fqml_entang_type" 
QASM_MSG_PARAM_TAG_FBQML_SUBTYPE  = "fqml_subtype"	
QASM_MSG_PARAM_TAG_FBQML_FMAPVEC  = "fqml_fmap_fvec"	

QASM_MSG_PARAM_TAG_RESULT   = "result"
QASM_MSG_PARAM_TAG_ERROR    = "error"

# parameter values
QASM_MSG_PARAM_VAL_OK  = "Ok"
QASM_MSG_PARAM_VAL_NOK = "Not-Ok"

# --------------------

# function types
QASM_F_TYPE_NULL = -1

# 1-qubit
QASM_F_TYPE_I = 0 
QASM_F_TYPE_H = 1
QASM_F_TYPE_X = 2
QASM_F_TYPE_Y = 3
QASM_F_TYPE_Z = 4
QASM_F_TYPE_SX = 5
QASM_F_TYPE_PS = 6
QASM_F_TYPE_T = 7
QASM_F_TYPE_S = 8
QASM_F_TYPE_Rx = 9
QASM_F_TYPE_Ry = 10
QASM_F_TYPE_Rz = 11

def QASM_F_IS_Q1(ft):
    return ((ft >= QASM_F_TYPE_I) and (ft <= QASM_F_TYPE_Rz))

def QASM_F_IS_Q1_PARAMETRIC(ft):
    return (ft in [QASM_F_TYPE_PS, QASM_F_TYPE_Rx, QASM_F_TYPE_Ry, QASM_F_TYPE_Rz])

# 2-qubit
QASM_F_TYPE_CU = 12
QASM_F_TYPE_CX = 13
QASM_F_TYPE_CY = 14
QASM_F_TYPE_CZ = 15

def QASM_F_IS_Q2(ft):
    return ((ft >= QASM_F_TYPE_CU) and (ft <= QASM_F_TYPE_CZ))

# n-qubit
QASM_F_TYPE_MCS_LRU = 16
QASM_F_TYPE_CCX = 17

def QASM_F_IS_Qn(ft):
    return ((ft >= QASM_F_TYPE_MCS_LRU) and (ft <= QASM_F_TYPE_CCX))

# --------------------

# function blocks
QASM_FB_TYPE_SWAP_Q1 = 100
QASM_FB_TYPE_SWAP_Qn = 101
QASM_FB_TYPE_CSWAP_Q1 = 102
QASM_FB_TYPE_CSWAP_Qn = 103

def QASM_FB_IS_BLOCK(ft):
    return ((ft >= QASM_FB_TYPE_SWAP_Q1) and (ft <= QASM_FB_TYPE_CSWAP_Qn))

QASM_FBQML_TYPE_FMAP = 200
QASM_FBQML_TYPE_QNET = 201

def QASM_FB_IS_BLOCK_QML(ft):
    return ((ft >= QASM_FBQML_TYPE_FMAP) and (ft <= QASM_FBQML_TYPE_QNET))

# --------------------

# function forms
QASM_F_FORM_NULL = -1

QASM_F_FORM_DIRECT  = 0
QASM_F_FORM_INVERSE = 1

# --------------------

# exectation observable operator types
QASM_EX_OBSOP_TYPE_NULL = -1

QASM_EX_OBSOP_TYPE_COMP   = 0 
QASM_EX_OBSOP_TYPE_PAULIZ = 1

# --------------------

# data type and supported entanglement types
QASM_QML_ENTANG_TYPE_NULL = -1

QASM_QML_ENTANG_TYPE_LINEAR   = 0 
QASM_QML_ENTANG_TYPE_CIRCULAR = 1

# --------------------

# data type and supported feature map types
QASM_QML_FMAP_TYPE_NULL = -1

QASM_QML_FMAP_TYPE_PAULI_Z  = 0 
QASM_QML_FMAP_TYPE_PAULI_ZZ = 1

# --------------------

# data type and supported qnetwork types
QASM_QML_QNET_TYPE_NULL = -1

QASM_QML_QNET_TYPE_REAL_AMPL  = 0 

# --------------------------------------------------------

# ==> qSim instruction set handling
class qSim_qcln_qasm(): 
   
    def __init__(self):
        # class constructor 
        self.m_counter = 0
        self.m_id = 0
        self.m_params_dict = {}
    
    # ------------------
    
    def add_param_tagValue(self, par_tag, par_val):
        self.m_params_dict[par_tag] = par_val

    def get_param_valueByTag(self, par_tag):
        if par_tag in self.m_params_dict.keys():
            return self.m_params_dict[par_tag]
        else:
            return None

    # -------------------------

    def check_syntax(self):
        # not implemented yet...........
        return True

    # -------------------------

    @staticmethod
    def is_ftype_q1(ftype):
        return ftype in range(QASM_F_TYPE_I, QASM_F_TYPE_Rz+1)

    @staticmethod
    def is_ftype_q2(ftype):
        return ftype in range(QASM_F_TYPE_CU, QASM_F_TYPE_CZ+1)

    @staticmethod
    def is_ftype_qn(ftype):
        return ftype in range(QASM_F_TYPE_MCS_LRU, QASM_F_TYPE_MCS_LRU)
    
    @staticmethod
    def is_ftype_block(ftype):
        return QASM_FB_IS_BLOCK(ftype)
    
    @staticmethod
    def is_ftype_block_qml(ftype):
        return QASM_FB_IS_BLOCK_QML(ftype)
    
    # -------------------------
    # encoding/decoding

    # coding format: ASCII string with "|" as field separators
    #    <encoded_instruction> =  <counter>"|"<id>"|"params
    # 
    # with ":" as param tag + value pairs separator
    #    <params> = <param_tag>=<param_value>:....

    # # separators
    # QASM_MSG_FIELD_SEP  = '|'
    # QASM_MSG_PARAM_SEP  = ':'
    # QASM_MSG_PARVAL_SEP = '='

    def from_raw_message(self, msg_str):
        # extract counter, id and params from given string

        idx = msg_str.find(QASM_MSG_FIELD_SEP) # first separator mandatory
        if idx < 1:
            print('qSim_qasm_message::from_raw_message - wrong format!')
            return
	
        self.m_counter = int(msg_str[0:idx])
        msg_str = msg_str[idx+1:]
        # print('m_counter:', self.m_counter, '...msg_str:', msg_str)

        idx = msg_str.find(QASM_MSG_FIELD_SEP) # second separator mandatory
        if idx < 1:
            print('qSim_qasm_message::from_raw_message - wrong format!')
            return
	
        self.m_id = int(msg_str[0:idx])
        msg_str = msg_str[idx+1:]
        # print('m_id:', self.m_id, '...msg_str:', msg_str)

        self.m_params_dict.clear()
        while len(msg_str) > 0:
            idx = msg_str.find(QASM_MSG_PARAM_SEP) # separator mandatory
            if idx < 1:
                print('qSim_qasm_message::from_raw_message - wrong format!')
                return

            par_pair = msg_str[0:idx]
            msg_str = msg_str[idx+1:]

            idx2 = par_pair.find(QASM_MSG_PARVAL_SEP) # separator mandatory
            if idx2 < 1:
                print('qSim_qasm_message::from_raw_message - wrong format!')
                return

            par_tag = par_pair[0:idx2]
            par_val = par_pair[idx2+1:]
            self.m_params_dict[par_tag] = par_val

    def to_raw_message(self):
        # encode counter, id and params into a string
        msg_str = str(self.m_counter)
        msg_str += QASM_MSG_FIELD_SEP
        msg_str += str(self.m_id)
        msg_str += QASM_MSG_FIELD_SEP
        for p_key in self.m_params_dict.keys():
            msg_str += p_key + QASM_MSG_PARVAL_SEP + self.m_params_dict[p_key]
            msg_str += QASM_MSG_PARAM_SEP
                
        return msg_str
    
    # -------------------------
    # diagnostics mehods
    
    def dump(self):
        # dump message attributes on stdout
        print('*** qSim_qasm_message dump ***')
        print()

        print('m_counter:', self.m_counter)
        print('m_id:     ', self.m_id)
        print('m_params_dict count:', len(self.m_params_dict.keys()))
        for i, p_key in enumerate(self.m_params_dict.keys()):
            print('#', i, 'par_tag:', p_key, 'par_val:', self.m_params_dict[p_key]);
        print()
        print('**********************************')
        print()
        
    # -------------------------

    
