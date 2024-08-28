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
#include <memory>

namespace tbb {
namespace detail {

namespace d1 { class task_group_context; class wait_context; struct execution_data; }
namespace d2 {

class task_handle;

class continuation_vertex : public d1::reference_vertex {
public:
    continuation_vertex(d1::wait_tree_vertex_interface* parent, std::uint32_t ref_count, d1::task* continuation_task, d1::small_object_allocator& alloc)
        : d1::reference_vertex(parent, ref_count)
        , m_continuation_task(continuation_task)
        , m_allocator(alloc)
    {}
private:
    d1::task* m_continuation_task;
    d1::small_object_allocator m_allocator;

};

class task_handle_task : public d1::task {
    std::uint64_t m_version_and_traits{};
    d1::wait_tree_vertex_interface* m_wait_tree_vertex;
    bool m_is_continuation{false};
    d1::task_group_context& m_ctx;
    d1::small_object_allocator m_allocator;
public:
    void finalize(const d1::execution_data* ed = nullptr) {
        if (ed) {
            m_allocator.delete_object(this, *ed);
        } else {
            // task_group::run was never called for task_handle so there is no assosiated execution_data
            __TBB_ASSERT(!m_is_continuation, "Continuation should be task_group::run before task_handle destruction");
            m_allocator.delete_object(this);
        }
    }

    task_handle_task(d1::wait_tree_vertex_interface* vertex, d1::task_group_context& ctx, d1::small_object_allocator& alloc)
        : m_wait_tree_vertex(vertex)
        , m_ctx(ctx)
        , m_allocator(alloc) {
        suppress_unused_warning(m_version_and_traits);
        m_wait_tree_vertex->reserve();
    }

    ~task_handle_task() {
        m_wait_tree_vertex->release();
    }

    d1::task_group_context& ctx() const { return m_ctx; }

    bool is_continuation() const { return m_is_continuation; }

    void add_predecessor(task_handle_task& predecessor) {
        if (!m_is_continuation) {
            d1::small_object_allocator alloc;
            auto continuation = alloc.new_object<continuation_vertex>(m_wait_tree_vertex, 1, this, alloc);
            m_wait_tree_vertex = continuation;
            m_is_continuation = true;
        }
        m_wait_tree_vertex->reserve();
        predecessor.m_wait_tree_vertex->release();
        predecessor.m_wait_tree_vertex = m_wait_tree_vertex;
    }
};


class task_handle {
    struct task_handle_task_finalizer_t{
        void operator()(task_handle_task* p){ p->finalize(); }
    };
    using handle_impl_t = std::unique_ptr<task_handle_task, task_handle_task_finalizer_t>;

    handle_impl_t m_handle = {nullptr};
public:
    task_handle() = default;
    task_handle(task_handle&&) = default;
    task_handle& operator=(task_handle&&) = default;

    void add_predecessor(task_handle& th) { 
        if (m_handle) {
            m_handle->add_predecessor(*th.m_handle);
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
