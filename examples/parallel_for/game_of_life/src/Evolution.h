/*
    Copyright 2005-2015 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks. Threading Building Blocks is free software;
    you can redistribute it and/or modify it under the terms of the GNU General Public License
    version 2  as  published  by  the  Free Software Foundation.  Threading Building Blocks is
    distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See  the GNU General Public License for more details.   You should have received a copy of
    the  GNU General Public License along with Threading Building Blocks; if not, write to the
    Free Software Foundation, Inc.,  51 Franklin St,  Fifth Floor,  Boston,  MA 02110-1301 USA

    As a special exception,  you may use this file  as part of a free software library without
    restriction.  Specifically,  if other files instantiate templates  or use macros or inline
    functions from this file, or you compile this file and link it with other files to produce
    an executable,  this file does not by itself cause the resulting executable to be covered
    by the GNU General Public License. This exception does not however invalidate any other
    reasons why the executable file might be covered by the GNU General Public License.
*/

/** 
    Evolution.h: Header file for evolution classes; evolution classes do 
    looped evolution of patterns in a defined 2 dimensional space 
**/

#ifndef __EVOLUTION_H__
#define __EVOLUTION_H__

#include "Board.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN

#include "tbb/task_scheduler_init.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/tick_count.h"

#ifndef _CONSOLE
#include <windows.h>
using namespace System::Threading;
#else
typedef unsigned int Int32;
#endif

void UpdateState(Matrix * m_matrix, char * dest ,int begin, int end);

/**
    class Evolution - base class for SequentialEvolution and ParallelEvolution
**/
#ifndef _CONSOLE
public ref class Evolution abstract
#else
class Evolution
#endif
{
public:
    Evolution( Matrix *m,                //! beginning matrix including initial pattern
               BoardPtr board              //! the board to update
             ) : m_matrix(m), m_board(board), 
                 m_size(m_matrix->height * m_matrix->width), m_done(false)
    {
        //! allocate memory for second matrix data block
        m_dest = new char[m_size];
        is_paused = false;
#ifdef _CONSOLE
        m_serial_time = 0;
#endif
    }

    virtual ~Evolution()
    {
        delete[] m_dest;
    }

    //! Run() - begins looped evolution
#ifndef _CONSOLE
    virtual void Run() = 0;
#else
    virtual void Run(double execution_time, int nthread) = 0;
#endif

    //! Quit() - tell the thread to terminate
    virtual void Quit() { m_done = true; }
    
    //! Step() - performs a single evolutionary generation computation on the game matrix
    virtual void Step() = 0;

    //! SetPause() - change condition of variable is_paused
    virtual void SetPause(bool condition)
    {
        if ( condition == true )
            is_paused = true;
        else
            is_paused = false;
    }
    
protected:
    /** 
        UpdateMatrix() - moves the previous destination data to the source 
        data block and zeros out destination.
    **/
    void UpdateMatrix();

protected:
    Matrix*         m_matrix;       //! Pointer to initial matrix
    char*           m_dest;         //! Pointer to calculation destination data    
    BoardPtr        m_board;        //! The game board to update
    int             m_size;         //! size of the matrix data block
    volatile bool   m_done;         //! a flag used to terminate the thread
    Int32           m_nIteration;   //! current calculation cycle index
    volatile bool   is_paused;      //! is needed to perform next iteration
    
    //! Calculation time of the sequential version (since the start), seconds.
    /**
        This member is updated by the sequential version and read by parallel,
        so no synchronization is necessary.
    **/
#ifndef _CONSOLE
    static volatile double m_serial_time = 0;

    static System::Threading::AutoResetEvent    ^m_evt_start_serial = gcnew AutoResetEvent(false),
                                                ^m_evt_start_parallel = gcnew AutoResetEvent(false);
#else
    double m_serial_time;
#endif
};

/**
    class SequentialEvolution - derived from Evolution - calculate life generations serially
**/
#ifndef _CONSOLE
public ref class SequentialEvolution: public Evolution
#else
class SequentialEvolution: public Evolution
#endif
{
public:
    SequentialEvolution(Matrix *m, BoardPtr board)
                       : Evolution(m, board)
    {}
#ifndef _CONSOLE        
    virtual void Run() override;
    virtual void Step() override;
#else
    virtual void Run(double execution_time, int nthread);
    virtual void Step();
#endif

};

/**
    class ParallelEvolution - derived from Evolution - calculate life generations
    in parallel using Intel(R) TBB
**/
#ifndef _CONSOLE
public ref class ParallelEvolution: public Evolution
#else
class ParallelEvolution: public Evolution
#endif
{
public:

    ParallelEvolution(Matrix *m, BoardPtr board)
                     : Evolution(m, board),
                       m_parallel_time(0)
    {
        // instantiate a task_scheduler_init object and save a pointer to it
        m_pInit = NULL;
    }
    
    ~ParallelEvolution()
    {
        //! delete task_scheduler_init object
        if (m_pInit != NULL)
            delete m_pInit;
    }
#ifndef _CONSOLE
    virtual void Run() override;
    virtual void Step() override;
#else
    virtual void Run(double execution_time, int nthread);
    virtual void Step();
#endif
    

private:
    tbb::task_scheduler_init* m_pInit;

    double m_parallel_time;
};

#endif
