# Copyright (C) 2024 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.� IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
# OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
# OR OTHER DEALINGS IN THE SOFTWARE.
#
# SPDX-License-Identifier: MIT

CXX = icpx
CXXFLAGS = $(EXTRA_CXXFLAGS) -std=c++17 -O2
LIBS = -ltbb

# \
!ifndef 0   # \
all clean:  _printmsg # \
# \
_printmsg:  # \
	@echo "for nmake, please specify /f Makefile.nmake"
# \
!else

.PHONY : all clean cancel_group_execution1 cancel_group_execution2 cancel_group_execution3

all : cancel_group_execution1 cancel_group_execution2 cancel_group_execution3

cancel_group_execution1 : 
	$(CXX) $(CXXFLAGS) -o cancel_group_execution1 cancel_group_execution1.cpp $(LIBS)

cancel_group_execution2 : 
	$(CXX) $(CXXFLAGS) -o cancel_group_execution2 cancel_group_execution2.cpp $(LIBS)

cancel_group_execution3 : 
	$(CXX) $(CXXFLAGS) -o cancel_group_execution3 cancel_group_execution3.cpp $(LIBS)

clean : 
	rm -f cancel_group_execution1 cancel_group_execution2 cancel_group_execution3 cancel_group_execution1.o cancel_group_execution2.o cancel_group_execution3.o *.bmp *.txt out*.png


# \
!endif
