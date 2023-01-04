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

CUDA_LIB := /usr/local/cuda-9.2/lib64
LIBS	 := -L$(CUDA_LIB) -lcudart -lpthread

TARGET := qSim
OBJECTS	:= ./obj/qSim.o ./obj/qSim_main.o \
	   ./obj/qSim_qio.o ./obj/qSim_qio_queue.o ./obj/qSim_qio_socket.o \
	   ./obj/qSim_qasm.o ./obj/qSim_qsocket.o \
	   ./obj/qSim_qcpu.o ./obj/qSim_qreg.o \
	   ./obj/qSim_qinstruction_base.o ./obj/qSim_qinstruction_core.o ./obj/qSim_qinstruction_block.o \
	   ./obj/qSim_qcpu_device_GPU_CUDA.o 

# SK case (default)
NVCCFLAGS := $(NVCCFLAGS) $(CUDA_ARCH) 
NVCCLNKFLAGS := $(CUDA_ARCH)  
TARGET := $(TARGET)_sk


$(info ************  building $(TARGET) ************)
all: $(TARGET)


qSim_sk: $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TARGET) $(OBJECTS)


###########################################
# build rules
###########################################

# -----------  qSim  -----------------
./obj/qSim.o: ../qSim/src/qSim.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim/src/qSim.cpp -o ./obj/qSim.o

./obj/qSim_main.o: ../qSim/src/qSim_main.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c ../qSim/src/qSim_main.cpp -o ./obj/qSim_main.o

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



