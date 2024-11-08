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

#ifndef ARENA_TRACE_H
#define ARENA_TRACE_H 1

#include <tbb/combinable.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/task_scheduler_observer.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_set>

class arena_tracer {
public:
  arena_tracer(const std::string& name) : m_t0(std::chrono::steady_clock::now()), m_name(name)  { }
  ~arena_tracer() { dump_trace(); }
 
  void add_arena(const std::string& n, tbb::task_arena& a) {
    m_observers.local().emplace_back( std::make_shared<tracing_observer>(m_events, m_arena_id++, m_t0, n, a )); 
  }

private:
  using time_point_type = std::chrono::time_point<std::chrono::steady_clock>;
  time_point_type m_t0;
  const std::string m_name;
  std::atomic<int> m_arena_id = 0;

  struct trace_event {
    time_point_type t;
    char ph;
    bool is_worker;
    std::thread::id tid;
    int slot;
    int pid;
    trace_event( time_point_type t_, char ph_, 
                 bool is_worker_, std::thread::id tid_, 
                 int slot_, int pid_ ) : t(t_), ph(ph_), is_worker(is_worker_), tid(tid_), slot(slot_), pid(pid_) {}
  };

  using ets_vector_type = std::vector<trace_event>;
  using ets_value_type = ets_vector_type;
  using ets_type = tbb::combinable<ets_value_type>;
  ets_type m_events;
 
  // observer class
  class tracing_observer : public oneapi::tbb::task_scheduler_observer {
    ets_type& m_events;
    int m_arena_id;
    time_point_type m_t;
    const std::string m_name;

  public:
    tracing_observer( ets_type& e, int arena_id, time_point_type t0,
                      const std::string& fn, oneapi::tbb::task_arena &a ) 
        : oneapi::tbb::task_scheduler_observer(a), m_events(e), m_arena_id(arena_id), m_t(t0), m_name(fn) {
        observe(true); // activate the observer
    }

    ~tracing_observer() { }

    int get_arena_id() { return m_arena_id; }
    const std::string& get_arena_name() { return m_name; }

    void on_scheduler_entry( bool worker ) override {
      auto& l = m_events.local();
      std::thread::id tid = std::this_thread::get_id();

      l.emplace_back(
        std::chrono::steady_clock::now(), 
        'B', // ph
        worker, // is_worker
        tid,
        oneapi::tbb::this_task_arena::current_thread_index(),
        m_arena_id 
      );
    }
    void on_scheduler_exit( bool worker ) override {
      auto& l = m_events.local();
      std::thread::id tid = std::this_thread::get_id();

      l.emplace_back(
        std::chrono::steady_clock::now(), 
        'E', // ph
        worker, // is_worker
        tid,
        oneapi::tbb::this_task_arena::current_thread_index(),
        m_arena_id 
      );
    }
  };

  void dump_trace() {
    std::unordered_set<std::thread::id> was_seen;

    std::ofstream out(m_name);
    out << "[";

    bool first = true;
    for (auto& v : m_observers.range()) {
      for (auto& obs : v) {
        if (!first) out << ","; 
        first = false; 
        out << "\n{\"name\": \"process_name\", \"ph\" : \"M\"" 
            << ", \"pid\" : " << obs->get_arena_id()  
            << ", \"args\" : { \"name\" : \"" << obs->get_arena_name() << "\" }"  
            << "}";
      }
    } 

    ets_vector_type r;
    m_events.combine_each(
      [&r](const ets_vector_type& v) {
        r.insert(r.end(), v.begin(), v.end());
      });
    
    std::sort(r.begin(), r.end(), [](const trace_event& a, const trace_event& b) { return a.t < b.t; });

    for (auto& e : r) {
      out << ",\n{"
          <<   "\"name\": \"" << e.tid << "\""
          << ", \"cat\": \"arena\""
          << ", \"ph\": \"" << e.ph << "\""
          << ", \"ts\": " << std::chrono::duration_cast<std::chrono::microseconds>(e.t-m_t0).count()
          << ", \"pid\": " << e.pid
          << ", \"tid\": " << e.slot
          << "}";
      if (e.ph == 'B') {
        out << ",\n{"
            << "\"name\": \"flow\""
            << ", \"id\": " << e.tid 
            << ", \"cat\": \"arena\""
            << ", \"ts\": " << std::chrono::duration_cast<std::chrono::microseconds>(e.t-m_t0).count()
            << ", \"pid\": " << e.pid
            << ", \"tid\": " << e.slot;
        if (was_seen.find(e.tid) != was_seen.end()) {
          out << ", \"ph\": \"t\", \"bp\": \"e\" }";
        } else {
          was_seen.insert(e.tid);
          out << ", \"ph\": \"s\", \"bp\": \"e\" }";
        }
      }
    }

    out << "\n]\n";
  }

  tbb::enumerable_thread_specific<std::vector<std::shared_ptr<tracing_observer>>> m_observers;
};

#endif
