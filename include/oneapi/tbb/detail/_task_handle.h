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
#include "oneapi/tbb/rw_mutex.h"

#include <memory>
#include <forward_list>
#include <iostream>

namespace tbb {
namespace detail {

namespace d1 { class task_group_context; class wait_context; struct execution_data; }
namespace d2 {

class task_handle;

class continuation_vertex : public d1::reference_vertex {
public:
    continuation_vertex(d1::task* continuation_task, d1::task_group_context& ctx, d1::small_object_allocator& alloc)
        : d1::reference_vertex(nullptr, 1)
        , m_continuation_task(continuation_task)
        , m_ctx(ctx)
        , m_allocator(alloc)
    {}

    void release(std::uint32_t delta = 1) override;

private:
    d1::task* m_continuation_task;
    d1::task_group_context& m_ctx;
    d1::small_object_allocator m_allocator;
};

class task_handle_task : public d1::task {
public:
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
        release_successors();
    }

    d1::task_group_context& ctx() const { return m_ctx; }

    bool is_continuation() const { return m_continuation != nullptr; }

    void release_continuation() { m_continuation->release(); }

    void transfer_successors_to(task_handle_task* target) {
        target->m_wait_tree_vertex_successors.splice_after(target->m_wait_tree_vertex_successors.begin(), m_wait_tree_vertex_successors);
        m_wait_tree_vertex_successors.clear();
    }

    void add_predecessor(task_handle_task& predecessor) {
        this->add_dependency(predecessor);
    }

    void add_successor(task_handle_task& successor) {
        successor.add_dependency(*this);
    }

private:
    void add_dependency(task_handle_task& dependency) {
        if (m_continuation == nullptr) {
            d1::small_object_allocator alloc;
            m_continuation = alloc.new_object<continuation_vertex>(this, m_ctx, alloc);
        }
        m_continuation->reserve();
        dependency.m_wait_tree_vertex_successors.push_front(m_continuation);
    }

    void release_successors() {
        if (!m_wait_tree_vertex_successors.empty()) {
            for (auto successor : m_wait_tree_vertex_successors) {
                successor->release();
            }
        }
    }

    std::uint64_t m_version_and_traits{};
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
    // rw_mutex m_mutex;
public:
    task_handle() = default;
    task_handle(task_handle&&) = default;
    task_handle& operator=(task_handle&&) = default;

    void add_predecessor(task_handle& th) { 
        if (m_handle) {
            m_handle->add_predecessor(*th.m_handle);
        }
    }

    void add_successor(task_handle& th) {
        if (m_handle) {
            m_handle->add_successor(*th.m_handle);
        }
    }

    explicit operator bool() const noexcept { return static_cast<bool>(m_handle); }

    friend bool operator==(task_handle const& th, std::nullptr_t) noexcept;
    friend bool operator==(std::nullptr_t, task_handle const& th) noexcept;

    friend bool operator!=(task_handle const& th, std::nullptr_t) noexcept;
    friend bool operator!=(std::nullptr_t, task_handle const& th) noexcept;

private:
    friend struct task_handle_accessor;

    task_handle(task_handle_task* t) : m_handle {t}{};

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
static bool is_continuation(task_handle& th) { return th.m_handle->is_continuation(); }
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
