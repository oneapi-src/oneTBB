/*
    Copyright (c) 2024 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

// this pseudo-code was used in the book "Today's TBB" (2015)
// it serves no other purpose other than to be here to verify compilation,
// and provide consist code coloring for the book



// loop 1
for (int i = 0; i < N; ++i) {
    b[i] = f(a[i]);
}



;;;;;;;



// loop 2
for (int i = 0; i < M; ++i) {
    d[i] = g(c[i]);
}



;;;;;;;;;

// loop 3
for (int i = 0; i < N; ++i) {
    b[i] = f(a[i]);
}

// loop 4
for (int i = 0; i < N; ++i) {
    c[i] = f(b[i]);
}


;;;;;;;


try_to_get_a_next_task:
    t_next = try to get a critical task
             else try to pop from end of thread's local deque

if t_next is not valid
    do
        do
            t_next = try to get critical task
                     else try to get from thread's affinity mailbox
                     else try to get from arena's resumed tasks
                     else try to get from arena's fifo tasks
                     else try to steal task from random vitcim thread in same arena
        while t_next is not valid and the thread should stay in this arena
    while thread must stay in this arena
end if

return t next


;;;;;;;;;;


dispatcher:
    if is master
        t_next = starting task
    else
        t_next = try_to_get_a_next_task()
    end if

    while t_next is valid

        do // innermost execute and bypass loop
            t = t_next
            if t's task group context has been canceled
                t_next = t->cancel()
            else
                t_next = t->execute()
            end if
            t_crit = try to get critical task
            if t_crit is valid
                spawn t_next
                t_next = t_crit
            end if
        while t_next is a valid task // end bypass loop

        if I'm a master and work is done
            break
        else if I'm a worker, arena allotment has changed and I should leave
            break
        end if

        t_next = try_to_get_next_task()

    end while // couldn't find valid t_next

    if I'm a worker thread
        return myself to the global thread pool
    else I'm a master thread
        return
    end if







;;;;;;;;;;;;;;



void serial_f(int N, v_type* v) {
    std::for_each(std::execution::seq, v, v + N,
        [](v_type& e) { e = f(e); });
}

void pfor_f(int N, v_type* v) {
    tbb::parallel_for(tbb::blocked_range<v_type*>(v, v + N),
        [=](const tbb::blocked_range<v_type*>& r) {
            std::for_each(std::execution::seq, r.begin(), r.end(),
                [](v_type& e) { e = f(e); });
        }, tbb::static_partitioner());
}

void unseq_f(int N, v_type* v) {
    std::for_each(std::execution::unseq, v, v + N,
        [](v_type& v) { v = f(v); });
}

void pfor_unseq_f(int N, v_type* v) {
    tbb::parallel_for(tbb::blocked_range<v_type*>(v, v + N),
        [=](const tbb::blocked_range<v_type*>& r) {
            std::for_each(std::execution::unseq, r.begin(), r.end(),
                [](v_type& e) { e = f(e); });
        }, tbb::static_partitioner());
}


