/*
 * qSim_main.cpp
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
 *  Created on: May 12, 2022
 *      Author: gianni
 *
 * Q-SIM application - main module
 *
 *  Version History:
 *
 *  Ver   Date       Change
 *  --------------------------------------------------------------------------
 *  1.0   May-2022   Module creation.
 *  1.1   Nov-2022   Updated release version.
 *                   Handled command line arguments for help, verbose flag and
 *                   TCP/IP port.
 *
 *  --------------------------------------------------------------------------
 */


#include <string>
#include <iostream>
using namespace std;

#include "qSim.h"

#define QSIM_DEFAULT_IPADDR "127.0.0.1"
#define QSIM_DEFAULT_PORT 27020

#ifdef __QSIM_CPU__
#define QSIM_ARCH "CPU"
#else
#define QSIM_ARCH "GPU"
#endif

#define QSIM_VERSION "v2.0"


// print usage information
void show_usage(std::string cmd) {
	cout << "Usage: " << cmd << " [args...]" << endl;
	cout << "where arguments include:" << endl;
	cout << " -help, -h" << endl;
	cout << "\t to display this help" << endl;
	cout << " -verbose, -v" << endl;
	cout << "\t to enable diagnostic messages" << endl;
	cout << " -port=<number>, -p=<number>" << endl;
	cout << "\t to set a specific TCP/IP port" << endl;
	cout << endl;
}

// main function entry point
int main(int argc, char *argv[]) {

	cout << "***********************" << endl;
	cout << "*** qSim "<< QSIM_ARCH << " - " << QSIM_VERSION << " ***" << endl;
	cout << "***********************" << endl;
	cout << endl;

	// setup parameters from command line arguments - if any
	bool verbose = false;
	int port = QSIM_DEFAULT_PORT;
	for (int i=1; i<argc; i++) {
		std::string arg = std::string(argv[i]);
		if ((arg.compare("-v") == 0) || (arg.compare("-verbose") == 0)) {
			// set verbose flag
			verbose = true;
		}
		else if ((arg.find("-p=") != std::string::npos) || (arg.find("-port=") != std::string::npos)) {
			// port tag found - check for correct syntax (-port=<value>) and read port value
			int sep_index = arg.find("=");
			std::string port_str = arg.substr(sep_index+1, arg.length()-sep_index-1);
			if (port_str.length() > 0) {
				port = std::stoi(port_str);
			}
			else {
				// wrong syntax
				cerr << "ERROR!! wrong port number syntax [" << arg << "]" << endl << endl;
				show_usage(std::string(argv[0]));
				return 0;
			}
		}
		// other cases...

		else if ((arg.compare("-help") == 0) || (arg.compare("-h") == 0)) {
			// print usage and exit
			show_usage(std::string(argv[0]));
			return 0;
		}

		else {
			// wrong argument passed
			cerr << "ERROR!! wrong argument [" << arg << "] provided" << endl << endl;
			show_usage(std::string(argv[0]));
			return 0;
		}
	}

	cout << "qSim parameters:" << endl;
	cout << "-> verbose: " << verbose << endl;
	cout << endl;

	// initialise qsim component
	qSim qsim(verbose);
	int ret = qsim.init(QSIM_DEFAULT_IPADDR, port);
	if (ret == QSIM_ERROR) {
		cerr << "ERROR!! qsim initialisation failed" << endl;
		return 0;
	}

	// start handling loops
	cout << "qSim initialised - starting loop..." << endl;
	qsim.startLoop();

	while (1) {
		// ... nothing else here - all done in the thread loop

		// sleep
		this_thread::sleep_for(chrono::milliseconds(10000));
	}

	cout << "=> qSim run completed" << endl << endl;

	return 0;
}

