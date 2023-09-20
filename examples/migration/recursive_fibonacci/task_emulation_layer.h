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

    base_task(const base_task& t) : m_type(t.m_type.load()), m_parent(t.m_parent), m_ref_counter(t.m_ref_counter.load())
    {}

    virtual ~base_task() = default;

    void operator() () const {
        base_task* parent_snapshot = m_parent;
        std::uint64_t type_snapshot = m_type;

        const_cast<base_task*>(this)->execute();

        bool is_task_recycled_as_child = parent_snapshot != m_parent;
        bool is_task_recycled_as_continuation = type_snapshot != m_type;

        if (m_parent && !is_task_recycled_as_child && !is_task_recycled_as_continuation) {
            auto child_ref = m_parent->remove_child_reference() & (m_self_ref - 1);
            if (child_ref == 0) {
                m_parent->operator()();
            }
        }

        if (type_snapshot != task_type::stack_based && const_cast<base_task*>(this)->remove_self_ref() == 0) {
            delete this;
        }
    }

    virtual void execute() = 0;

    template <typename C, typename... Args>
    C* allocate_continuation(std::uint64_t ref, Args&&... args) {
        C* continuation = new C{std::forward<Args>(args)...};
        continuation->m_type = task_type::continuation;
        continuation->reset_parent(reset_parent());
        continuation->m_ref_counter = ref;
        continuation->add_self_ref();
        return continuation;
    }

    template <typename F, typename... Args>
    F create_child(Args&&... args) {
        return create_child_impl<F>(std::forward<Args>(args)...);
    }

    template <typename F, typename... Args>
    F create_child_and_increment(Args&&... args) {
        add_child_reference();
        return create_child_impl<F>(std::forward<Args>(args)...);
    }

    template <typename F, typename... Args>
    F* allocate_child(Args&&... args) {
        return allocate_child_impl<F>(std::forward<Args>(args)...);
    }

    template <typename F, typename... Args>
    F* allocate_child_and_increment(Args&&... args) {
        add_child_reference();
        return allocate_child_impl<F>(std::forward<Args>(args)...);
    }

    template <typename C>
    void recycle_as_child_of(C& c) {
        reset_parent(&c);
    }

    void recycle_as_continuation() {
        add_self_ref();
        m_type += task_type::continuation;
    }

    void add_child_reference() {
        ++m_ref_counter;
    }

    std::uint64_t remove_child_reference() {
        return --m_ref_counter;
    }

protected:
    void add_self_ref() {
        m_ref_counter.fetch_add(m_self_ref);
    }

    std::uint64_t remove_self_ref() {
        return m_ref_counter.fetch_sub(m_self_ref) - m_self_ref;
    }

    struct task_type {
        static constexpr std::uint64_t stack_based = 1;
        static constexpr std::uint64_t allocated = 1 << 1;
        static constexpr std::uint64_t continuation = 1 << 2;
    };

    std::atomic<std::uint64_t> m_type;

private:
    template <typename F, typename... Args>
    friend F create_root_task(tbb::task_group& tg, Args&&... args);

    template <typename F, typename... Args>
    friend F* allocate_root_task(tbb::task_group& tg, Args&&... args);

    template <typename F, typename... Args>
    F create_child_impl(Args&&... args) {
        F obj{std::forward<Args>(args)...};
        obj.m_type = task_type::stack_based;
        obj.reset_parent(this);
        return obj;
    }

    template <typename F, typename... Args>
    F* allocate_child_impl(Args&&... args) {
        F* obj = new F{std::forward<Args>(args)...};
        obj->m_type = task_type::allocated;
        obj->add_self_ref();
        obj->reset_parent(this);
        return obj;
    }

    base_task* reset_parent(base_task* ptr = nullptr) {
        auto p = m_parent;
        m_parent = ptr;
        return p;
    }

    base_task* m_parent{nullptr};
    static constexpr std::uint64_t m_self_ref = std::uint64_t(1) << 48;
    std::atomic<std::uint64_t> m_ref_counter{0};
};

class root_task : public base_task {
public:
    root_task(tbb::task_group& tg) : m_tg(tg), m_callback(m_tg.defer([] { /* Create empty callback to preserve reference for wait. */})) {
        add_child_reference();
        m_type = base_task::task_type::continuation;
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
    obj.m_type = base_task::task_type::stack_based;
    obj.reset_parent(new root_task{tg});
    return obj;
}

template <typename F, typename... Args>
F* allocate_root_task(tbb::task_group& tg, Args&&... args) {
    F* obj = new F{std::forward<Args>(args)...};
    obj->m_type = base_task::task_type::allocated;
    obj->add_self_ref();
    obj->reset_parent(new root_task{tg});
    return obj;
}

template <typename F>
void run_task(F&& f) {
    tg_pool[tbb::this_task_arena::current_thread_index()].run(std::forward<F>(f));
}

template <typename F>
void run_task(F* f) {
    tg_pool[tbb::this_task_arena::current_thread_index()].run(std::ref(*f));
}

template <typename F>
void run_and_wait(tbb::task_group& tg, F* f) {
   tg.run_and_wait(std::ref(*f));
}
} // namespace task_emulation

#endif // __TBB_task_emulation_layer_H
