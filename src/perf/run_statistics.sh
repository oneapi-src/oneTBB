#!/bin/bash
#
# Copyright 2005-2015 Intel Corporation.  All Rights Reserved.
#
# This file is part of Threading Building Blocks. Threading Building Blocks is free software;
# you can redistribute it and/or modify it under the terms of the GNU General Public License
# version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
# distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See  the GNU General Public License for more details.   You should have received a copy of
# the  GNU General Public License along with Threading Building Blocks; if not, write to the
# Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA
#
# As a special exception,  you may use this file  as part of a free software library without
# restriction.  Specifically,  if other files instantiate templates  or use macros or inline
# functions from this file, or you compile this file and link it with other files to produce
# an executable,  this file does not by itself cause the resulting executable to be covered
# by the GNU General Public License. This exception does not however invalidate any other
# reasons why the executable file might be covered by the GNU General Public License.

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
#setting output format .csv, 'pivot' - is pivot table mode, ++ means append
export STAT_FORMAT=pivot-csv++
#check existing files because of apend mode
ls *.csv
rm -i *.csv
#setting a delimiter in txt or csv file
#export STAT_DELIMITER=,
export STAT_RUNINFO1=Host=`hostname -s`
#append a suffix after the filename
#export STAT_SUFFIX=$STAT_RUNINFO1
for ((i=1;i<=${repeat:=100};++i)); do echo $i of $repeat: && STAT_RUNINFO2=Run=$i $* || break; done
