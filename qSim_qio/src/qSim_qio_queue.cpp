/*
 * qSim_qio.cpp
 *
 * --------------------------------------------------------------------------
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
 * --------------------------------------------------------------------------
 *
 *  Created on: May 18, 2022
 *      Author: gianni
 *
 * Q-IO queue class, handling a generic object queue in thread safe way.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation.
 *  1.1   Dec-2022   Modified pop method to return removed element reference.
 *
 *  --------------------------------------------------------------------------
 */

#include <chrono>
#include <iostream>
using namespace std;

#include "qSim_qio_queue.h"


// constructor
qSim_qio_queue::qSim_qio_queue() {
}

// destructor
qSim_qio_queue::~qSim_qio_queue() {
}

///////////////////////////////////////////////////////////////////

qSim_qasm_message* qSim_qio_queue::peek() {
	// handle queue check using a critical section
	qSim_qasm_message* item = NULL;
	m_mutex.lock();
	if (m_queue.size() > 0)
		item = m_queue.front();
	m_mutex.unlock();
	return item;
}

qSim_qasm_message* qSim_qio_queue::pop() {
	// handle queue top element removal using a critical section
	qSim_qasm_message* item = NULL;
	m_mutex.lock();
	if (m_queue.size() > 0) {
		item = m_queue.front();
		m_queue.pop();
	}
	m_mutex.unlock();
	return item;
}

void qSim_qio_queue::push(qSim_qasm_message* item) {
	// handle queue bottom element insert using a critical section
	m_mutex.lock();
	m_queue.push(item);
	m_mutex.unlock();
}

