/*
    Copyright (c) 2005-2021 Intel Corporation

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

#ifndef __TBB_detail_scoped_lock_H
#define __TBB_detail_scoped_lock_H

#include <algorithm>
#include <utility>

namespace tbb {
namespace detail {
namespace d1 {

// unique_scoped_lock supposes that Mutex operations never throw
template <typename Mutex>
class unique_scoped_lock {
    //! Points to currently held Mutex, or NULL if no lock is held.
    Mutex* m_mutex{};

    // Move constructor logic. Assumes that current scoped_lock doesn't own any mutex.
    // The method mustn't be called concurrently.
    constexpr void move_constructor_implementation(unique_scoped_lock&& other) noexcept {
        // Since there is only a pointer to a mutex in the class, it's sufficient to move that pointer.
        m_mutex = other.m_mutex;
        other.m_mutex = nullptr;
    }

    // Convenient method.
    // The method mustn't be called concurrently.
    void smart_reset() noexcept {
        // If current scoped_lock owns a mutex.
        if (m_mutex != nullptr) {
            // Then release it.
            release();
        }
    }

public:
    //! Construct without acquiring a Mutex.
    constexpr unique_scoped_lock() noexcept : m_mutex(nullptr) {}

    //! Construct and acquire lock on a Mutex.
    unique_scoped_lock(Mutex& m) {
        acquire(m);
    }

    constexpr unique_scoped_lock(unique_scoped_lock&& other) noexcept {
        // The whole logic contains in the method below.
        move_constructor_implementation(std::move(other));
    }

    // Move assignment works similar as move constructor.
    unique_scoped_lock& operator=(unique_scoped_lock&& other) noexcept {
        // If this differ from other.
        if (this != &other) {
            // If current scoped_lock owns a mutex, then release it.
            smart_reset();
            // We now have the scoped_lock instance that doesn't own any mutex.
            move_constructor_implementation(std::move(other));
        }
        return *this;
    }

    //! No Copy
    unique_scoped_lock(const unique_scoped_lock&) = delete;
    unique_scoped_lock& operator=(const unique_scoped_lock&) = delete;

    //! Acquire lock.
    void acquire(Mutex& m) {
        __TBB_ASSERT(m_mutex == nullptr, "The mutex is already acquired");
        m_mutex = &m;
        m.lock();
    }

    //! Try acquiring lock (non-blocking)
    /** Return true if lock acquired; false otherwise. */
    bool try_acquire(Mutex& m) {
        __TBB_ASSERT(m_mutex == nullptr, "The mutex is already acquired");
        bool succeed = m.try_lock();
        if (succeed) {
            m_mutex = &m;
        }
        return succeed;
    }

    //! Release lock
    void release() {
        __TBB_ASSERT(m_mutex, "release on Mutex::unique_scoped_lock that is not holding a lock");
        m_mutex->unlock();
        m_mutex = nullptr;
    }

    //! Destroy lock. If holding a lock, releases the lock first.
    ~unique_scoped_lock() {
        // If current scoped_lock owns a mutex, then release it.
        smart_reset();
    }

    // Swap semantics, constexpr since C++ 20.
    // The method mustn't be called concurrently.
    _CONSTEXPR20 void swap(unique_scoped_lock& other) noexcept {
        std::swap(m_mutex, other.m_mutex);
    }
};

// rw_scoped_lock supposes that Mutex operations never throw
template <typename Mutex>
class rw_scoped_lock {
public:
    //! Construct lock that has not acquired a mutex.
    /** Equivalent to zero-initialization of *this. */
    constexpr rw_scoped_lock() noexcept {}

    //! Acquire lock on given mutex.
    rw_scoped_lock(Mutex& m, bool write = true) {
        acquire(m, write);
    }

    //! Release lock (if lock is held).
    ~rw_scoped_lock() {
        // If current scoped_lock owns a mutex, then release it.
        smart_reset();
    }
    
    constexpr rw_scoped_lock(rw_scoped_lock&& other) noexcept {
        // The whole logic contains in the method below.
        move_constructor_implementation(std::move(other));
    }

    // Move assignment works similar as move constructor.
    rw_scoped_lock& operator=(rw_scoped_lock&& other) noexcept {
        // If this differ from other.
        if (this != &other) {
            // If current scoped_lock owns a mutex, then release it.
            smart_reset();
            // We now have the scoped_lock instance that doesn't own any mutex.
            move_constructor_implementation(std::move(other));
        }
        return *this;
    }

    //! No Copy
    rw_scoped_lock(const rw_scoped_lock&) = delete;
    rw_scoped_lock& operator=(const rw_scoped_lock&) = delete;

    //! Acquire lock on given mutex.
    void acquire(Mutex& m, bool write = true) {
        __TBB_ASSERT(m_mutex == nullptr, "The mutex is already acquired");
        m_is_writer = write;
        m_mutex = &m;
        if (write) {
            m_mutex->lock();
        } else {
            m_mutex->lock_shared();
        }
    }

    //! Try acquire lock on given mutex.
    bool try_acquire(Mutex& m, bool write = true) {
        bool succeed = write ? m.try_lock() : m.try_lock_shared();
        if (succeed) {
            m_mutex = &m;
            m_is_writer = write;
        }
        return succeed;
    }

    //! Release lock.
    void release() {
        __TBB_ASSERT(m_mutex != nullptr, "The mutex is not acquired");
        Mutex* m = m_mutex;
        m_mutex = nullptr;

        if (m_is_writer) {
            m->unlock();
        } else {
            m->unlock_shared();
        }
    }

    //! Upgrade reader to become a writer.
    /** Returns whether the upgrade happened without releasing and re-acquiring the lock */
    bool upgrade_to_writer() {
        __TBB_ASSERT(m_mutex != nullptr, "The mutex is not acquired");
        if (m_is_writer) {
            return true; // Already a writer
        }
        m_is_writer = true;
        return m_mutex->upgrade();
    }

    //! Downgrade writer to become a reader.
    bool downgrade_to_reader() {
        __TBB_ASSERT(m_mutex != nullptr, "The mutex is not acquired");
        if (m_is_writer) {
            m_mutex->downgrade();
            m_is_writer = false;
        }
        return true;
    }

    bool is_writer() const {
        __TBB_ASSERT(m_mutex != nullptr, "The mutex is not acquired");
        return m_is_writer;
    }

    // Swap semantics, constexpr since C++ 20.
    // The method mustn't be called concurrently.
    _CONSTEXPR20 void swap(rw_scoped_lock& other) noexcept {
        std::swap(m_mutex, other.m_mutex);
        std::swap(m_is_writer, other.m_is_writer);
    }

protected:
    // Move constructor logic. Assumes that current scoped_lock doesn't own any mutex.
    // The method mustn't be called concurrently.
    constexpr void move_constructor_implementation(rw_scoped_lock&& other) noexcept {
        // It's sufficient to move the pointer and assign the flag.
        m_mutex = other.m_mutex;
        m_is_writer = other.m_is_writer;
        other.m_mutex = nullptr;
    }

    // Convenient method.
    // The method mustn't be called concurrently.
    void smart_reset() noexcept {
        // If current scoped_lock owns a mutex.
        if (m_mutex != nullptr) {
            // Then release it.
            release();
        }
    }

    //! The pointer to the current mutex that is held, or nullptr if no mutex is held.
    Mutex* m_mutex {nullptr};

    //! If mutex != nullptr, then is_writer is true if holding a writer lock, false if holding a reader lock.
    /** Not defined if not holding a lock. */
    bool m_is_writer {false};
};

} // namespace d1
} // namespace detail
} // namespace tbb

#endif // __TBB_detail_scoped_lock_H
