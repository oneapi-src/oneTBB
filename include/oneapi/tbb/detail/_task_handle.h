/*
    Copyright (c) 2020-2024 Intel Corporation

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


#ifndef __TBB_task_handle_H
#define __TBB_task_handle_H

#include "_config.h"
#include "_task.h"
#include "_small_object_pool.h"
#include "_utils.h"
#include "oneapi/tbb/mutex.h"

#include <memory>
#include <forward_list>
#include <iostream>

namespace tbb {
namespace detail {

namespace d1 { class task_group_context; class wait_context; struct execution_data; }
namespace d2 {

class task_handle;
class task_handle_task;

class continuation_vertex : public d1::reference_vertex {
public:
    continuation_vertex(task_handle_task* continuation_task, d1::task_group_context& ctx, d1::small_object_allocator& alloc)
        : d1::reference_vertex(nullptr, 1)
        , m_continuation_task(continuation_task)
        , m_ctx(ctx)
        , m_allocator(alloc)
    {}

    void release(std::uint32_t delta = 1) override;

private:
    task_handle_task* m_continuation_task;
    d1::task_group_context& m_ctx;
    d1::small_object_allocator m_allocator;
};

class task_handle_task : public d1::task {
public:
    class task_state_handler {
    public:
        task_state_handler(task_handle_task* task, d1::small_object_allocator& alloc) : m_task(task), m_alloc(alloc) {}
        void release() {
            d1::mutex::scoped_lock lock(m_mutex);
            release_impl(lock);
        }

        void complete_task() {
            d1::mutex::scoped_lock lock(m_mutex);
            m_is_finished = true;
            release_impl(lock);
        }

        void add_successor(task_handle_task& successor) {
            if (successor.m_continuation == nullptr) {
                d1::small_object_allocator alloc;
                successor.m_continuation = alloc.new_object<continuation_vertex>(&successor, successor.m_ctx, alloc);
            }

            d1::mutex::scoped_lock lock(m_mutex);
            if (!m_is_finished) {
                successor.m_continuation->reserve();
                m_task->m_wait_tree_vertex_successors.push_front(successor.m_continuation);
            }
        }

    private:
        void release_impl(d1::mutex::scoped_lock& lock) {
            if (--m_num_references == 0) {
                lock.release();
                m_alloc.delete_object(this);
            }
        }

        task_handle_task* m_task;
        bool m_is_finished{false};
        int m_num_references{2};
        d1::mutex m_mutex;
        d1::small_object_allocator m_alloc;
    };

    void finalize(const d1::execution_data* ed = nullptr) {
        if (ed) {
            m_allocator.delete_object(this, *ed);
        } else {
            m_allocator.delete_object(this);
        }
    }

    task_handle_task(d1::wait_tree_vertex_interface* vertex, d1::task_group_context& ctx, d1::small_object_allocator& alloc)
        : m_wait_tree_vertex_successors{vertex}
        , m_ctx(ctx)
        , m_allocator(alloc)
    {
        suppress_unused_warning(m_version_and_traits);
        vertex->reserve();
    }

    ~task_handle_task() {
        if (m_state_holder) {
            m_state_holder->complete_task();
        }
        release_successors();
    }

    d1::task_group_context& ctx() const { return m_ctx; }

    bool has_dependency() const { return m_continuation != nullptr; }

    void release_continuation() { m_continuation->release(); }

    void transfer_successors_to(task_handle_task* target) {
        // TODO: What if we set current task as a dependency later?
        target->m_wait_tree_vertex_successors.splice_after(target->m_wait_tree_vertex_successors.begin(), m_wait_tree_vertex_successors);
        m_wait_tree_vertex_successors.clear();
    }

    void add_predecessor(task_handle_task& predecessor) {
        predecessor.m_state_holder->add_successor(*this);
    }

    void add_successor(task_handle_task& successor) {
        this->m_state_holder->add_successor(successor);
    }

    task_state_handler* get_state_holder() {
        d1::small_object_allocator alloc;
        m_state_holder = alloc.new_object<task_state_handler>(this, alloc);
        return m_state_holder;
    }

private:
    void release_successors() {
        if (!m_wait_tree_vertex_successors.empty()) {
            for (auto successor : m_wait_tree_vertex_successors) {
                successor->release();
            }
        }
    }

    std::uint64_t m_version_and_traits{};
    task_state_handler* m_state_holder{nullptr};
    std::forward_list<d1::wait_tree_vertex_interface*> m_wait_tree_vertex_successors;
    continuation_vertex* m_continuation{nullptr};
    d1::task_group_context& m_ctx;
    d1::small_object_allocator m_allocator;
};


class task_handle {
    struct task_handle_task_finalizer_t{
        void operator()(task_handle_task* p){ p->finalize(); }
    };
    using handle_impl_t = std::unique_ptr<task_handle_task, task_handle_task_finalizer_t>;

    handle_impl_t m_handle = {nullptr};
    task_handle_task::task_state_handler* m_state_holder = {nullptr};
public:
    task_handle() = default;
    task_handle(task_handle&& th) : m_handle(std::move(th.m_handle)), m_state_holder(th.m_state_holder) {
        th.m_state_holder = nullptr;
    }

    task_handle& operator=(task_handle&& th) {
        if (this != &th) {
            m_handle = std::move(th.m_handle);
            m_state_holder = th.m_state_holder;
            th.m_state_holder = nullptr;
        }
        return *this;
    }

    ~task_handle() {
        if (m_state_holder) {
            m_state_holder->release();
        }
    }

    void add_predecessor(task_handle& th) { 
        if (m_state_holder) {
            th.m_state_holder->add_successor(*m_handle);
        }
    }

    void add_successor(task_handle& th) {
        if (m_state_holder) {
            m_state_holder->add_successor(*th.m_handle);
        }
    }

    explicit operator bool() const noexcept { return static_cast<bool>(m_handle); }

    friend bool operator==(task_handle const& th, std::nullptr_t) noexcept;
    friend bool operator==(std::nullptr_t, task_handle const& th) noexcept;

    friend bool operator!=(task_handle const& th, std::nullptr_t) noexcept;
    friend bool operator!=(std::nullptr_t, task_handle const& th) noexcept;

private:
    friend struct task_handle_accessor;

    task_handle(task_handle_task* t) : m_handle{t}, m_state_holder(t->get_state_holder()) {};

    d1::task* release() {
       return m_handle.release();
    }
};

struct task_handle_accessor {
static task_handle              construct(task_handle_task* t)  { return {t}; }
static d1::task*                release(task_handle& th)        { return th.release(); }
static d1::task_group_context&  ctx_of(task_handle& th)         {
    __TBB_ASSERT(th.m_handle, "ctx_of does not expect empty task_handle.");
    return th.m_handle->ctx();
}
static bool has_dependency(task_handle& th) { return th.m_handle->has_dependency(); }
static void release_continuation(task_handle& th) {
    th.m_handle->release_continuation();
    th.release();
}
static void transfer_successors_to(task_handle& th, task_handle_task* task) {
    task->transfer_successors_to(th.m_handle.get());
}
};

inline bool operator==(task_handle const& th, std::nullptr_t) noexcept {
    return th.m_handle == nullptr;
}
inline bool operator==(std::nullptr_t, task_handle const& th) noexcept {
    return th.m_handle == nullptr;
}

inline bool operator!=(task_handle const& th, std::nullptr_t) noexcept {
    return th.m_handle != nullptr;
}

inline bool operator!=(std::nullptr_t, task_handle const& th) noexcept {
    return th.m_handle != nullptr;
}

} // namespace d2
} // namespace detail
} // namespace tbb

#endif /* __TBB_task_handle_H */
