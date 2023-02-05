# qSim
The Quantum Simulator (qSim) is simulator able to provide an high performance universal QC
machine, running on CPU or GPU based hardware in Linux.

## Overview
The functionalities provided by qSim are:

  • qubit registers (quregs) handling
  
  • universal 1-qubit and 2-qubits quantum gate set (e.g. Pauli, Hadamard, identity, phase shift, CX, CY, Cz, etc.)
  
  • high level blocks for common n-qubits transformations (e.g. swap and controlled swap)
  
  • quregs measurement
  
In addition, some diagnostic & test helper functionalities are also supported:

  • arbitrary qureg state setup
  
  • qureg state access (max 10 qubits qureg)
  
## Build and Run
To build qSim in Linux run rhe Makefile provided in the build_make folder:

  => "make" or "make gpu" for GPU target build
  
  => "make cpu" for CPU target build
  
To run qSim simply lanuch the executable build in the build_make folder with the previous steps.

  => "qSim_gpu" or "qSim_cpu"

Use the "-verbose" flag as command line argument to enable diagnostic messages.

The qSim requires CUDA libraries installed in the target platform in case of GPU target.

Feel free to customise the related CUDA variables in Makefile setting up your own includes and library aths as well as the CUDA compute level supported by your hardware.
  
## Client Access
The access to qSim is TCP/IP socket based and then possible by any applicaion supporting it. As Python specifi chelper, a dedicated module client access module is also included in the repository (qSim_qcln), making the access from a Python script very straightforward. This module is also described in the "How-To" access guide.

## References
Further details and a complete description of the access protocol are provided in the "qSim How-To User Guide" provided in the repository.
For any questions or requests my contact is: giacas11@gmail.com


  
