# --------------------------------------------------------------------------
# Copyright (C) 2018 Gianni Casonato
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# --------------------------------------------------------------------------

# command line makefile script for qSim_test building - GPU GT-1030 


# Define the symbols we might want to change:
CXX	 := /usr/bin/g++-7
CXXFLAGS := -std=c++1z
NVCC	 := nvcc
NVCCFLAGS := 
CUDA_ARCH := -gencode arch=compute_61,code=sm_61

CUDA_INC :=/usr/local/cuda-9.2/include
INCLUDES := -I$(CUDA_INC) -I../qSim_qcpu/src  -I../qSim_qbus/src  -I../qSim_qio/src -I../qSim/src

LIBS := 
CUDA_LIB_PATH := /usr/local/cuda-9.2/lib64
CUDA_LIBS := -L$(CUDA_LIB_PATH) -lcudart -lpthread

TARGET := qSim
TARGET_GPU := $(TARGET)_gpu
TARGET_CPU := $(TARGET)_cpu

OBJECTS	:= ./obj/qSim.o ./obj/qSim_main.o \
	   ./obj/qSim_qio.o ./obj/qSim_qio_queue.o ./obj/qSim_qio_socket.o \
	   ./obj/qSim_qasm.o ./obj/qSim_qsocket.o \
	   ./obj/qSim_qcpu.o ./obj/qSim_qreg.o \
	   ./obj/qSim_qinstruction_base.o ./obj/qSim_qinstruction_core.o ./obj/qSim_qinstruction_block.o
OBJECTS_GPU := $(OBJECTS) ./obj/qSim_qcpu_device_GPU_CUDA.o
OBJECTS_CPU := $(OBJECTS) ./obj/qSim_qcpu_device_CPU.o

# SK case (default)
NVCCFLAGS := $(NVCCFLAGS) $(CUDA_ARCH) 
NVCCLNKFLAGS := $(CUDA_ARCH)  


$(info ************  building $(TARGET) ************)
#all: $(TARGET_GPU) $(TARGET_CPU)
all: $(TARGET_GPU) 

gpu: $(TARGET_GPU)

cpu:	CXXFLAGS := $(CXXFLAGS) -D__QSIM_CPU__
cpu:	$(TARGET_CPU)

qSim_gpu: $(OBJECTS_GPU)
	  $(CXX) $(OBJECTS_GPU) -o $(TARGET_GPU) $(LIBS) $(CUDA_LIBS)

qSim_cpu: $(OBJECTS_CPU)
	  $(CXX) $(OBJECTS_CPU) -o $(TARGET_CPU) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TARGET_GPU) $(OBJECTS_GPU) $(TARGET_CPU) $(OBJECTS_CPU)


###########################################
# build rules
###########################################

# -----------  qSim  -----------------
./obj/qSim.o: ../qSim/src/qSim.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim/src/qSim.cpp -o ./obj/qSim.o

./obj/qSim_main.o: ../qSim/src/qSim_main.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim/src/qSim_main.cpp -o ./obj/qSim_main.o

# -----------  qSim test  -----------------
./obj/qSim_test.o: ../qSim_test/src/qSim_test.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_test/src/qSim_test.cpp -o ./obj/qSim_test.o

./obj/qSim_test_qio.o: ../qSim_test/src/qSim_test_qio.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_test/src/qSim_test_qio.cpp -o ./obj/qSim_test_qio.o

./obj/qSim_test_qbus.o: ../qSim_test/src/qSim_test_qbus.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_test/src/qSim_test_qbus.cpp -o ./obj/qSim_test_qbus.o

./obj/qSim_test_qcpu.o: ../qSim_test/src/qSim_test_qcpu.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_test/src/qSim_test_qcpu.cpp -o ./obj/qSim_test_qcpu.o

./obj/qSim_test_qsim.o: ../qSim_test/src/qSim_test_qsim.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_test/src/qSim_test_qsim.cpp -o ./obj/qSim_test_qsim.o

./obj/qSim_test_qcpu_bug_fixes.o: ../qSim_test/src/qSim_test_qcpu_bug_fixes.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_test/src/qSim_test_qcpu_bug_fixes.cpp -o ./obj/qSim_test_qcpu_bug_fixes.o

./obj/qSim_test_qcpu_perfo.o: ../qSim_test/src/qSim_test_qcpu_perfo.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_test/src/qSim_test_qcpu_perfo.cpp -o ./obj/qSim_test_qcpu_perfo.o

./obj/qSim_test_qcpu_ref_file_checks.o: ../qSim_test/src/qSim_test_qcpu_ref_file_checks.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_test/src/qSim_test_qcpu_ref_file_checks.cpp -o ./obj/qSim_test_qcpu_ref_file_checks.o

# -----------  qIo  -----------------
./obj/qSim_qio.o: ../qSim_qio/src/qSim_qio.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qio/src/qSim_qio.cpp -o ./obj/qSim_qio.o

./obj/qSim_qio_queue.o: ../qSim_qio/src/qSim_qio_queue.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qio/src/qSim_qio_queue.cpp -o ./obj/qSim_qio_queue.o

./obj/qSim_qio_socket.o: ../qSim_qio/src/qSim_qio_socket.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qio/src/qSim_qio_socket.cpp -o ./obj/qSim_qio_socket.o

# -----------  qBus  -----------------
./obj/qSim_qasm.o: ../qSim_qbus/src/qSim_qasm.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qbus/src/qSim_qasm.cpp -o ./obj/qSim_qasm.o

./obj/qSim_qsocket.o: ../qSim_qbus/src/qSim_qsocket.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qbus/src/qSim_qsocket.cpp -o ./obj/qSim_qsocket.o

# -----------  qCpu  -----------------
./obj/qSim_qcpu.o: ../qSim_qcpu/src/qSim_qcpu.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qcpu/src/qSim_qcpu.cpp -o ./obj/qSim_qcpu.o

./obj/qSim_qreg.o: ../qSim_qcpu/src/qSim_qreg.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qcpu/src/qSim_qreg.cpp -o ./obj/qSim_qreg.o

./obj/qSim_qinstruction_base.o: ../qSim_qcpu/src/qSim_qinstruction_base.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qcpu/src/qSim_qinstruction_base.cpp -o ./obj/qSim_qinstruction_base.o

./obj/qSim_qinstruction_core.o: ../qSim_qcpu/src/qSim_qinstruction_core.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qcpu/src/qSim_qinstruction_core.cpp -o ./obj/qSim_qinstruction_core.o

./obj/qSim_qinstruction_block.o: ../qSim_qcpu/src/qSim_qinstruction_block.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qcpu/src/qSim_qinstruction_block.cpp -o ./obj/qSim_qinstruction_block.o

./obj/qSim_qcpu_device_GPU_CUDA.o: ../qSim_qcpu/src/qSim_qcpu_device_GPU_CUDA.cu
	$(NVCC) $(INCLUDES) $(NVCCFLAGS) -c ../qSim_qcpu/src/qSim_qcpu_device_GPU_CUDA.cu -o ./obj/qSim_qcpu_device_GPU_CUDA.o
	
./obj/qSim_qcpu_device_CPU.o: ../qSim_qcpu/src/qSim_qcpu_device_CPU.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim_qcpu/src/qSim_qcpu_device_CPU.cpp -o ./obj/qSim_qcpu_device_CPU.o



