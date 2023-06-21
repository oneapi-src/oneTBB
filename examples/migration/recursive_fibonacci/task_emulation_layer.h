/*
    Copyright (c) 2023 Intel Corporation

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

#ifndef __TBB_task_emulation_layer_H
#define __TBB_task_emulation_layer_H

#include "tbb/task_group.h"
#include "tbb/task_arena.h"

#include <atomic>

namespace task_emulation {

struct task_group_pool {
    task_group_pool() : pool_size(std::thread::hardware_concurrency()), task_submitters(new tbb::task_group[pool_size]) {}

    ~task_group_pool() {
        for (std::size_t i = 0; i < pool_size; ++i) {
            task_submitters[i].wait();
        }

        delete [] task_submitters;
    }

    tbb::task_group& operator[] (std::size_t idx) { return task_submitters[idx]; }

    const std::size_t pool_size;
    tbb::task_group* task_submitters;
};

static task_group_pool tg_pool;

class base_task {
public:
    base_task() = default;

    base_task(const base_task& t) : m_parent(t.m_parent), m_child_counter(t.m_child_counter.load())
    {}

    virtual ~base_task() = default;

    void operator() () const {
        base_task* parent_snapshot = m_parent;
        const_cast<base_task*>(this)->execute();
        if (m_parent && parent_snapshot == m_parent) {
            m_parent->release();
        }
    }

    virtual void execute() = 0;

    template <typename C, typename... Args>
    C* create_continuation(std::uint64_t ref, Args&&... args) {
        C* continuation = new C{std::forward<Args>(args)...};
        continuation->reset_parent(reset_parent());
        continuation->m_child_counter = ref;
        return continuation;
    }

    template <typename F, typename... Args>
    F create_child_of_continuation(Args&&... args) {
        F obj{std::forward<Args>(args)...};
        obj.reset_parent(this);
        return obj;
    }

    template <typename C>
    void recycle_as_child_of_continuation(C& c) {
        reset_parent(&c);
    }

protected:
    void reserve() {
        ++m_child_counter;
    }

private:
    template <typename F, typename... Args>
    friend F create_root_task(tbb::task_group& tg, Args&&... args);


    void release() {
        if (--m_child_counter == 0) {
            operator()();
            // Free will be called only for continuations
            delete this;
        }
    }

    base_task* reset_parent(base_task* ptr = nullptr) {
        auto p = m_parent;
        m_parent = ptr;
        return p;
    }

    base_task* m_parent{nullptr};
    std::atomic<std::uint64_t> m_child_counter{0};
};

class root_task : public base_task {
public:
    root_task(tbb::task_group& tg) : m_tg(tg), m_callback(m_tg.defer([] { /* Create empty callback to preserve reference for wait. */})) {
        reserve();
    }

private:
    void execute() override {
        m_tg.run(std::move(m_callback));
    }

    tbb::task_group& m_tg;
    tbb::task_handle m_callback;
};

template <typename F, typename... Args>
F create_root_task(tbb::task_group& tg, Args&&... args) {
    F obj{std::forward<Args>(args)...};
    obj.reset_parent(new root_task{tg});
    return obj;
}

template <typename F>
void run_task(F&& f) {
    tg_pool[tbb::this_task_arena::current_thread_index()].run(std::forward<F>(f));
}
} // namespace task_emulation

#endif // __TBB_task_emulation_layer_H
