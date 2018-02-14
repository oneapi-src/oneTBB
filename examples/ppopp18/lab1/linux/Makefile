#    Copyright (c) 2018 Intel Corporation
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.

# GNU Makefile that builds and runs example.

SRCDIR = ../src/

# The C++ compiler
ifneq (,$(shell which icc 2>/dev/null))
CXX=icc
endif # icc

CXXFLAGS = -O3 -std=c++11
TBBLIB = -ltbb
LIBS+= -lrt 

all:	
	@echo "Usage: make <lab_step>"
	@echo "Example: make 00"
	@echo ""
	@echo "Possible values of <lab_step>:"
	@echo ""
	@echo  "	01	: parallel_for"
	@echo  "	01s	: parallel_for_solution"
	@echo  "	02	: reduction"
	@echo  "	02s	: reduction_solution"
	@echo  "	03	: container"
	@echo  "	03s	: container_solution"
	@echo  "	04	: flow_graph"
	@echo  "	04s	: flow_graph_solution"
	@echo  "	05	: task"
	@echo  "	05s	: task_solution"

01: $(SRCDIR)/01_parallel_for.cpp
	$(CXX) $(CXXFLAGS) -o 01_parallel_for $(SRCDIR)/01_parallel_for.cpp $(TBBLIB) $(LIBS)

01s: $(SRCDIR)/01_parallel_for_solution.cpp
	$(CXX) $(CXXFLAGS) -o 01_parallel_for_solution $(SRCDIR)/01_parallel_for_solution.cpp $(TBBLIB) $(LIBS)

02: $(SRCDIR)/02_reduction.cpp
	$(CXX) $(CXXFLAGS) -o 02_reduction $(SRCDIR)/02_reduction.cpp $(TBBLIB) $(LIBS)

02s: $(SRCDIR)/02_reduction_solution.cpp
	$(CXX) $(CXXFLAGS) -o 02_reduction_solution $(SRCDIR)/02_reduction_solution.cpp $(TBBLIB) $(LIBS)

03: $(SRCDIR)/03_container.cpp
	$(CXX) $(CXXFLAGS) -o 03_container $(SRCDIR)/03_container.cpp $(TBBLIB) $(LIBS)

03s: $(SRCDIR)/03_container_solution.cpp
	$(CXX) $(CXXFLAGS) -o 03_container_solution $(SRCDIR)/03_container_solution.cpp $(TBBLIB) $(LIBS)

04: $(SRCDIR)/04_flow_graph.cpp
	$(CXX) $(CXXFLAGS) -o 04_flow_graph $(SRCDIR)/04_flow_graph.cpp $(TBBLIB) $(LIBS)

04s: $(SRCDIR)/04_flow_graph_solution.cpp
	$(CXX) $(CXXFLAGS) -o 04_flow_graph_solution $(SRCDIR)/04_flow_graph_solution.cpp $(TBBLIB) $(LIBS)

05: $(SRCDIR)/05_task.cpp
	$(CXX) $(CXXFLAGS) -o 05_task $(SRCDIR)/05_task.cpp $(TBBLIB) $(LIBS)

05s: $(SRCDIR)/05_task_solution.cpp
	$(CXX) $(CXXFLAGS) -o 05_task_solution $(SRCDIR)/05_task_solution.cpp $(TBBLIB) $(LIBS)

clean:
	$(RM) *.o 00_getting_started 01_parallel_for 01_parallel_for_solution 02_reduction 02_reduction_solution 03_container 03_container_data 03_container_solution 04_flow_graph 04_flow_graph_solution 05_task 05_task_solution

