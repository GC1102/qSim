/*
 * qSim_qio_queue.h
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
 * Q-IO queue class, handling a generic message queue in thread safe way.
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation.
 *
 *  --------------------------------------------------------------------------
 */


#ifndef QSIM_QIO_QUEUE_H_
#define QSIM_QIO_QUEUE_H_


#include <queue>
#include <mutex>

class qSim_qasm_message;

class qSim_qio_queue {

	public:
		// constructor and destructor
		qSim_qio_queue();
		virtual ~qSim_qio_queue();

		qSim_qasm_message* peek();
		qSim_qasm_message* pop();
		void push(qSim_qasm_message*);

		int size() { return m_queue.size(); }

	private:
		std::queue<qSim_qasm_message*> m_queue;
		std::mutex m_mutex;

	};

#endif /* QSIM_QIO_QUEUE_H_ */
