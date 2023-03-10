/*
 * qSim_qinstruction_block_qml.cpp
 *
 * --------------------------------------------------------------------------
 * Copyright (C) 2023 Gianni Casonato
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
 *  Created on: Feb 24, 2023
 *      Author: gianni
 *
 * QML specialised instruction block class, handling ad-hoc "block" instructions
 * supporting:
 * - feature map blocks
 * - QVC q-net blocks
 *
 * Derived class from qSim_qinstruction_block.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   Feb-2023   Module creation.
 *
 *  --------------------------------------------------------------------------
 */


#include <iostream>
#include <complex>
using namespace std;

#include <string.h>

#include "qSim_qinstruction_block_qml.h"
#include "qSim_qinstruction_core.h"


/////////////////////////////////////////////////////////////////////////
// QML q-instruction block class


// ------------------------

#define SAFE_TRASFORMATION_PARAMS_CHECK() {\
	if (QASM_F_TYPE_IS_FUNC_BLOCK_QML(m_ftype)) \
		m_valid = this->check_params();\
	else {\
		cerr << "qSim_qinstruction_block_qml - unhandled ftype value [" << m_ftype << "]!!";\
		m_valid = false;\
	}\
	if (!m_valid)\
		return; \
}

// ------------------------

// constructor & destructor
qSim_qinstruction_block_qml::qSim_qinstruction_block_qml(qSim_qasm_message* msg) :
		qSim_qinstruction_block() {
	// initialise all fields from given message
	m_type = msg->get_id();
	m_qr_h = 0;
	m_valid = true; // updated in the switch in case of exceptions
//	cout << "qSim_qinstruction_block...m_type: " << m_type << "  msg_id: " << msg->get_id() << endl;

	// qureg state transformation QML block message handling
	SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_QREG_H, m_qr_h)
	SAFE_MSG_GET_PARAM_AS_FTYPE(QASM_MSG_PARAM_TAG_F_TYPE, m_ftype)
	SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_FBQML_REP, m_frep)
	int fbent;
	SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_FBQML_ENTANG, fbent)
	m_fbent = (QASM_QML_ENTANG_TYPE)fbent;
	SAFE_MSG_GET_PARAM_AS_INT(QASM_MSG_PARAM_TAG_FBQML_SUBTYPE, m_fbsubtype)
	SAFE_MSG_GET_PARAM_AS_FARGS(QASM_MSG_PARAM_TAG_F_ARGS, m_fargs)

	// final semantic check
	SAFE_TRASFORMATION_PARAMS_CHECK()
}

qSim_qinstruction_block_qml::~qSim_qinstruction_block_qml() {
	// nothing to do...
}

// -------------------------------------

// other constructors - diagnostics
qSim_qinstruction_block_qml::qSim_qinstruction_block_qml(QASM_MSG_ID_TYPE type, int qr_h,
 														 QASM_F_TYPE fbtype, int fbrep,
														 QASM_QML_ENTANG_TYPE fbent, int fbsubtype,
														 QREG_F_ARGS_TYPE fargs)
 : qSim_qinstruction_block(type, qr_h, fbtype, 0, fbrep, 0) {
	// qureg qml block transformation
//	cout << "qSim_qinstruction_block_qml...m_frep: " << m_frep << endl;
	m_valid = true;

	// set attributes
	m_fbent = fbent;
	m_fbsubtype = fbsubtype;
	m_fargs = fargs;

	// semantic check
	SAFE_TRASFORMATION_PARAMS_CHECK()
}


// internal methods for transformation parameters semantic checks
bool qSim_qinstruction_block_qml::check_params() {
	// semantic checks
	bool res = true;

	// general checks
	// check function type within block range
	SAFE_CHECK_PARAM_VALUE(QASM_F_TYPE_IS_FUNC_BLOCK_QML(m_ftype), res,
						   "qSim_qinstruction_block_qml::check_params - illegal block type value", m_ftype)

	// check function repetitions > 0
	SAFE_CHECK_PARAM_VALUE((m_frep >= 1), res,
						   "qSim_qinstruction_block_qml::check_params - illegal function repetitions value", m_frep)

	// block type specific checks - feature map
	if (m_ftype == QASM_FBQML_TYPE_FMAP) {
		// check subtype type value within range
		SAFE_CHECK_PARAM_VALUE(((m_fbsubtype >= QASM_QML_FMAP_TYPE_PAULI_Z) || (m_fbsubtype <= QASM_QML_FMAP_TYPE_PAULI_ZZ)), res,
							   "qSim_qinstruction_block_qml::check_params - feature map subtype out of range", m_fbsubtype)

		if (m_fbsubtype == QASM_QML_FMAP_TYPE_PAULI_ZZ) {
			// check entanglement type value within range
			SAFE_CHECK_PARAM_VALUE(((m_fbent >= QASM_QML_ENTANG_TYPE_LINEAR) || (m_fbent <= QASM_QML_ENTANG_TYPE_CIRCULAR)), res,
							   	"qSim_qinstruction_block_qml::check_params - feature map entanglement out of range", m_fbent)
		}
	}
	// ...

	return res;
}

// -------------------------------------
// -------------------------------------

// function block decomposition into core instructions - feature map

void qSim_qinstruction_block_qml::unwrap_block_fmap(std::list<qSim_qinstruction_core*>* qinstr_list,
		                                            bool verbose) {
	// feature map QML function block decomposition in core transformations
	// based on given subtype (Pauli-Z or PAuli-ZZ)
	if (verbose)
		cout << "QML block - unwrap_fmap..." << endl;

	// perform unwrap according to sub-ftype value
	qinstr_list->clear();
	switch (m_fbsubtype) {
		case QASM_QML_FMAP_TYPE_PAULI_Z: {
			feature_map_pe_pauliZ(&m_fargs, m_frep, qinstr_list, verbose);
		}
		break;

		case QASM_QML_FMAP_TYPE_PAULI_ZZ: {
			feature_map_pe_pauliZZ(&m_fargs, m_frep, m_fbent, qinstr_list, verbose);
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction_block_qml - unhandled qasm message subtype " << m_fbsubtype << "!!" << endl;
		}
		break;
	}
}

void qSim_qinstruction_block_qml::feature_map_pe_pauliZ(QREG_F_ARGS_TYPE* f_vec, int b_rep,
		                                    std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose) {
    // 1:1 mapping between feature datapoint vector and qubits -> n = N
    // x vector to q state phases by H*PS gates blocks
    // applying given block repetition factor

    int n = f_vec->size();
    int fh_stn = 2;
    int fps_stn = 2;

    // unwrap feature map into core instructions - replicating per given total blocks
    for (int b=0; b<b_rep; b++) {
        // create <n> H gates
		qSim_qinstruction_core* qi_H = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
							m_qr_h, QASM_F_TYPE_Q1_H, fh_stn, n, 0);
		qinstr_list->push_back(qi_H);

        // create phase shift functions using <f_vec> vector elements as parameters
        for (int i=0; i<n; i++) {
        	QREG_F_ARGS_TYPE fargs;
			double phi = 2.0*(*f_vec)[i].m_d;
			fargs.push_back(qSim_qreg_function_arg(phi));
    		qSim_qinstruction_core* qi_PS = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
    						m_qr_h, QASM_F_TYPE_Q1_PS, fps_stn, 1, i,
							QREG_F_INDEX_RANGE_TYPE(), QREG_F_INDEX_RANGE_TYPE(), fargs);
    		qinstr_list->push_back(qi_PS);
       }
    }
}

void qSim_qinstruction_block_qml::feature_map_pe_pauliZZ(QREG_F_ARGS_TYPE* f_vec, int b_rep, QASM_QML_ENTANG_TYPE b_entang,
						    		std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose) {

	// 1:1 mapping between features datapoint vector and qubits -> n = N
	// x vector to q state phases by H*CX*PS gates sequence blocks
	// applying given block repetition factor and entanglement type

    int n = f_vec->size();
    int fh_stn = 2;
    int fps_stn = 2;

    // unwrap feature map into core instructions - replicating per given total blocks
    for (int b=0; b<b_rep; b++) {
        // create <n> H gates
		qSim_qinstruction_core* qi_H = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
							m_qr_h, QASM_F_TYPE_Q1_H, fh_stn, n, 0);
		qinstr_list->push_back(qi_H);

		// create control-X and phase shift functions slice as per given entanglement scheme
		// and using <f_vec> elements as parameters

        // create phase shift functions using <f_vec> vector elements as parameters
        for (int i=0; i<n; i++) {
        	QREG_F_ARGS_TYPE fargs;
			double phi = 2.0*(*f_vec)[i].m_d;
			fargs.push_back(qSim_qreg_function_arg(phi));
    		qSim_qinstruction_core* qi_PS = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
    						m_qr_h, QASM_F_TYPE_Q1_PS, fps_stn, 1, i,
							QREG_F_INDEX_RANGE_TYPE(), QREG_F_INDEX_RANGE_TYPE(), fargs);
    		qinstr_list->push_back(qi_PS);
        }

        // alternate sequence of controlled X and phase shift functions
        if (b_entang == QASM_QML_ENTANG_TYPE_LINEAR) {
        	// linear entanglement
        	// CX - PS - CX on i-th line starting from second qubit, CX controlled from i-1 line
        	int fcx_stn = 4; //2 qubits CX here...
            for (int i=1; i<n; i++) {
            	// add CX - PS - CX on i-th line, controlled from i-1 line (inverse form)
            	QREG_F_INDEX_RANGE_TYPE fcrng(i-1, i-1);
            	QREG_F_INDEX_RANGE_TYPE ftrng(i, i);
        		qSim_qinstruction_core* qi_CX1 = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
        						m_qr_h, QASM_F_TYPE_Q2_CX, fcx_stn, 1, i-1, fcrng, ftrng);
        		qinstr_list->push_back(qi_CX1);

            	QREG_F_ARGS_TYPE fargs;
    			double phi = 2.0*(*f_vec)[i].m_d;
    			fargs.push_back(qSim_qreg_function_arg(phi));
        		qSim_qinstruction_core* qi_PS = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
        						m_qr_h, QASM_F_TYPE_Q1_PS, fps_stn, 1, i,
    							QREG_F_INDEX_RANGE_TYPE(), QREG_F_INDEX_RANGE_TYPE(), fargs);
        		qinstr_list->push_back(qi_PS);

        		qSim_qinstruction_core* qi_CX2 = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
        						m_qr_h, QASM_F_TYPE_Q2_CX, fcx_stn, 1, i-1, fcrng, ftrng);
        		qinstr_list->push_back(qi_CX2);
            }
        }
        else if (b_entang == QASM_QML_ENTANG_TYPE_CIRCULAR) {
        	// circular entanglement
        	// CX - PS - CX on i-th line, CX controlled from i-1 line with wrapping for i=0
            for (int i=0; i<n; i++) {
            	// add CX - PS - CX on i-th line, controlled from i-1 line
//            # print('...adding first CX - i:', i)
            	if (i == 0) {
                	if (n > 2) {
						// CX target on first qubit
						int fcx_stn = (int)powf(2, n); // full span here
						QREG_F_INDEX_RANGE_TYPE fcrng(n-1, n-1);
						QREG_F_INDEX_RANGE_TYPE ftrng(0, 0);
						QREG_F_ARGS_TYPE fargs;
						int futype=QASM_F_TYPE_Q1_X;
						qSim_qinstruction_core* qi_CX = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
										m_qr_h, QASM_F_TYPE_QN_MCSLRU, fcx_stn, 1, 0, fcrng, ftrng, fargs, futype);
						qinstr_list->push_back(qi_CX);
                	}
            	}
            	else {
            		// CX target on other qubits
            		int fcx_stn = 4; // 2 qubits CX here...
                	QREG_F_INDEX_RANGE_TYPE fcrng(i-1, i-1);
                	QREG_F_INDEX_RANGE_TYPE ftrng(i, i);
            		qSim_qinstruction_core* qi_CX = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
            						m_qr_h, QASM_F_TYPE_Q2_CX, fcx_stn, 1, i-1, fcrng, ftrng);
            		qinstr_list->push_back(qi_CX);
            	}

//            # print('...adding PS - i:', i)
            	QREG_F_ARGS_TYPE fargs;
    			double phi = 2.0*(*f_vec)[i].m_d;
    			fargs.push_back(qSim_qreg_function_arg(phi));
        		qSim_qinstruction_core* qi_PS = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
        						m_qr_h, QASM_F_TYPE_Q1_PS, fps_stn, 1, i,
    							QREG_F_INDEX_RANGE_TYPE(), QREG_F_INDEX_RANGE_TYPE(), fargs);
        		qinstr_list->push_back(qi_PS);

//            # print('...adding second CX - i:', i)
            	if (i == 0) {
                	if (n > 2) {
						// CX target on first qubit
						int fcx_stn = (int)powf(2, n); // full span here
						QREG_F_INDEX_RANGE_TYPE fcrng(n-1, n-1);
						QREG_F_INDEX_RANGE_TYPE ftrng(0, 0);
						QREG_F_ARGS_TYPE fargs;
						int futype=QASM_F_TYPE_Q1_X;
						qSim_qinstruction_core* qi_CX = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
										m_qr_h, QASM_F_TYPE_QN_MCSLRU, fcx_stn, 1, 0, fcrng, ftrng, fargs, futype);
						qinstr_list->push_back(qi_CX);
                	}
            	}
            	else {
            		// CX target on other qubits
            		int fcx_stn = 4; // 2 qubits CX here...
                	QREG_F_INDEX_RANGE_TYPE fcrng(i-1, i-1);
                	QREG_F_INDEX_RANGE_TYPE ftrng(i, i);
            		qSim_qinstruction_core* qi_CX = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
            						m_qr_h, QASM_F_TYPE_Q2_CX, fcx_stn, 1, i-1, fcrng, ftrng);
            		qinstr_list->push_back(qi_CX);
            	}
            }
        }
//    elif entang == qvc_qfeatureMap.FMAP_ENTANG_FULL:
//        # full entanglement
//        print('ZZ Feature Map case with full entanglement not implemented yet!!!')
    }
}

// -------------------------------------
// -------------------------------------

// function block decomposition into core instructions - q-network

void qSim_qinstruction_block_qml::unwrap_block_qnet(int n, std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose) {
	// q-network QML function block decomposition in core transformations
	// based on given layout type (general or real amplitudes)
	if (verbose)
		cout << "QML block - unwrap_qnet..." << endl;

	// perform unwrap according to sub-ftype value
	qinstr_list->clear();
	switch (m_fbsubtype) {
		case QASM_QML_QNET_LAY_TYPE_REAL_AMPL: {
			qnetwork_realAmplitude(n, &m_fargs, m_frep, m_fbent, qinstr_list, verbose);
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction_block_qml - unhandled qasm message subtype " << m_fbsubtype << "!!" << endl;
		}
		break;
	}
}

void qSim_qinstruction_block_qml::qnetwork_realAmplitude(int n, QREG_F_ARGS_TYPE* param_vec, int b_rep,
					QASM_QML_ENTANG_TYPE b_entang, std::list<qSim_qinstruction_core*>* qinstr_list, bool verbose) {
	// real amplitude q-network layout building with given entanglement and repetitions

    // sequence of G as first layer (always - all layouts) - replicating it for given total blocks
    for (int b=0; b<b_rep; b++) {
	    int tot_block_params = 0;
        for (int i=0; i<n; i++) {
    	    // G1 gate params selection
    	    qnetwork_build_G_block(n, b, i, tot_block_params, param_vec,
    	    					   QASM_QML_QNET_LAY_TYPE_REAL_AMPL, qinstr_list);
		}

		// control G gates with target on all qubits based on given
		// entanglement type - no params here
		if (b_entang == QASM_QML_ENTANG_TYPE_LINEAR) {
			// --> linear entanglement form
			// CX functions with target starting from second qubit and control on previous qubit
	        for (int i=0; i<n-1; i++) {
	        	qnetwork_build_CG_block(i, i+1, NULL, i, qinstr_list);
	//        # print('gCG...c_idx:', c_idx, 't_idx:', t_idx, 'tc_rng:', tc_rng)

	//        qfCG = qvc_qnet.qf_CG(c_idx=0, t_idx=1, params=None,
	//                              qSim_cln=qSim_cln, verbose=False)
	//        csCG = qcir.myqpy_circuit_slice(self.m_q_n, 'CG'+str(i))
	//        csCG.add_elem(qfCG, idx_start=i, idx_stop=i+1, tag='CX')
	        }
		}

		else if (b_entang == QASM_QML_ENTANG_TYPE_CIRCULAR) {
			// --> circular entanglement form
			// CX functions with target starting from second qubit and control on previous qubit
			// with wrapping for i=0
	        for (int i=0; i<n-1; i++) {
	        	int t_idx, c_idx;
				if (i == 0) {
					t_idx = 0;
					c_idx = n-1; // wrapping on control qubit
				}
				else {
					t_idx = i;
					c_idx = i-1;
				}
	        	qnetwork_build_CG_block(c_idx, t_idx, NULL, i, qinstr_list);
	        }
	//    for i in range(n):
	//        if i == 0:
	//            idx_start = 0
	//            idx_stop = n-1
	//            t_idx = 0
	//            c_idx = n-1 # wrapping on control qubit
	//        else:
	//            idx_start = i-1
	//            idx_stop = i
	//            t_idx = 1
	//            c_idx = 0
	//        # print('gCG...c_idx:', c_idx, 't_idx:', t_idx, 'tc_rng:', tc_rng)
	//        qfCG = qvc_qnet.qf_CG(c_idx=c_idx, t_idx=t_idx, params=None,
	//                              qSim_cln=qSim_cln, verbose=False)
	//        csCG = qcir.myqpy_circuit_slice(self.m_q_n, 'CG'+str(i))
	//        csCG.add_elem(qfCG, idx_start=idx_start, idx_stop=idx_stop, tag='CX')
	//        self.insert(csCG)
		}
	//if lay_entang == self.LAY_ENTANG_FULL:
	//    # --> full entanglement form - NOT SUPPORTED YET
	//    print('RealAmplitude qnet case with full entanglement not implemented yet!!!')
	//    return None

		//sequence of G as last layer (last h for real-amplitude layout only)
		if (b == b_rep-1) {
			int tot_block_params = b * n;
	        for (int i=0; i<n; i++) {
	        	// G1 gate params selection
	    	    qnetwork_build_G_block(n, b+1, i, tot_block_params, param_vec,
	    	    		               QASM_QML_QNET_LAY_TYPE_REAL_AMPL, qinstr_list);
	        }
		}
    }
}

void qSim_qinstruction_block_qml::qnetwork_build_G_block(int n, int b, int i, int tot_block_params,
		QREG_F_ARGS_TYPE* param_vec, int lay_type, std::list<qSim_qinstruction_core*>* qinstr_list) {
	// define the proper parameter range for creating a G block
	// and adding it to the list
	QREG_F_INDEX_RANGE_TYPE param_idx_range;
	if (lay_type == QASM_QML_QNET_LAY_TYPE_REAL_AMPL) {
		// real-ampl G1 is single parameter (Ry based)
		param_idx_range = QREG_F_INDEX_RANGE_TYPE(i+b*n, i+b*n);
	}
    else {
    	// error - unhandled lay type!!
    	cout << "ERROR!! - unhandled layout type [" << lay_type << "]" << endl;
    	return;
    }

	// G1 instructions creation and addition to list
	qnetwork_build_G1(param_idx_range, param_vec, i, qinstr_list);
}

void qSim_qinstruction_block_qml::qnetwork_build_G1(QREG_F_INDEX_RANGE_TYPE param_idx_range,
		QREG_F_ARGS_TYPE* param_vec, int i, std::list<qSim_qinstruction_core*>* qinstr_list) {
	int range_span = param_idx_range.m_stop - param_idx_range.m_start + 1;
	if (range_span == 1) {
		// single parameter gate instruction
    	QREG_F_ARGS_TYPE fargs;
		double theta = (*param_vec)[param_idx_range.m_start].m_d;
		fargs.push_back(qSim_qreg_function_arg(theta));
		int fry_stn = 2;
		qSim_qinstruction_core* qi_Ry = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
						m_qr_h, QASM_F_TYPE_Q1_Ry, fry_stn, 1, i,
						QREG_F_INDEX_RANGE_TYPE(), QREG_F_INDEX_RANGE_TYPE(), fargs);
		qinstr_list->push_back(qi_Ry);
	}
//    if len(th_vec) == 2:
//         qfx = qpfij.myqpy_qfunction_ij(2, qf_ij_type=qpfij.MYFTYPE_Rx, qf_args=[th_vec[0]],
//                                        qf_lsq=qlsq, qSim=qSim_cln)
//         qfy = qpfij.myqpy_qfunction_ij(2, qf_ij_type=qpfij.MYFTYPE_Ry, qf_args=[th_vec[1]],
//                                        qf_lsq=qlsq, qSim=qSim_cln)
//         return [qfx, qfy]
}

void qSim_qinstruction_block_qml::qnetwork_build_CG_block(int c_idx, int t_idx,
		QREG_F_ARGS_TYPE* param_vec, int i, std::list<qSim_qinstruction_core*>* qinstr_list) {
	// n-qubit controlled G=X gate -> CX short/long range (no params)
	int fcx_stn;
	if (c_idx > t_idx)
		fcx_stn = (int)powf(2, (c_idx - t_idx + 1));
	 else
		 fcx_stn = (int)powf(2, (t_idx - c_idx + 1));
//	 # print('fcx_stn:', fcx_stn)

	QREG_F_INDEX_RANGE_TYPE fcrng(c_idx, c_idx);
	QREG_F_INDEX_RANGE_TYPE ftrng(t_idx, t_idx);
	QREG_F_ARGS_TYPE fargs;
	int futype = QASM_F_TYPE_Q1_X;
	qSim_qinstruction_core* qi_CX = new qSim_qinstruction_core(QASM_MSG_ID_QREG_ST_TRANSFORM,
					m_qr_h, QASM_F_TYPE_QN_MCSLRU, fcx_stn, 1, i, fcrng, ftrng, fargs, futype);
	qinstr_list->push_back(qi_CX);
}

// ---------------------------------

void qSim_qinstruction_block_qml::dump() {
	cout << "*** qSim_qinstruction_block_qml dump ***" << endl;
	cout << "m_type: " << m_type << endl;
	switch (m_type) {
		case QASM_MSG_ID_QREG_ST_TRANSFORM: {
			cout << "m_qr_h: " << m_qr_h << endl;
			cout << "m_ftype: " << m_ftype << endl;
			cout << "m_frep: " << m_frep << endl;
			cout << "m_fbent: " << m_fbent << endl;
			cout << "m_fbsubtype: " << m_fbsubtype << endl;
			cout << "m_fargs.size: " << m_fargs.size()
				 << " str: " << fargs_to_string(m_fargs) << endl;
		}
		break;

		default: {
			// error case
			cerr << "qSim_qinstruction_block_qml - unhandled qasm message type: "
				 << m_type << "!!" << endl;
		}
	}
	cout << endl;
}

