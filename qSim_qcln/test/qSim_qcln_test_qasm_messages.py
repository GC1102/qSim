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
* Test QASM message handling functions ---
*
"""


import qSim_qcln_qasm as qasm


# ******************************************************************
# test functions
# ******************************************************************

def test_qcln_qasm():
    qm = qasm.qSim_qcln_qasm()
    msg_str = '1|10|token=1653751880aaa:qr_n=2:' # allocate qureg
    print('=>> msg_str:', msg_str)
    print()

    qm.from_raw_message(msg_str)    
    qm.dump()
    
    msg_str2 = qm.to_raw_message()
    print('=>> msg_str2:', msg_str2)
    print()
    
    print('done.')
    
# -------------------------

