// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_UTILITY_RUN_LOOP_H_
#define MOJO_PUBLIC_CPP_UTILITY_RUN_LOOP_H_

#include <map>
#include <queue>

#include "mojo/public/c/system/time.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/system/handle.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/system/wait_set.h"
#include "mojo/public/cpp/utility/run_loop_handler.h"

namespace mojo {

// Run loop (a.k.a. message loop): watches handles for signals and calls
// handlers when they occur; can also execute posted (delayed) tasks. This class
// is not thread-safe.
class RunLoop {
 public:
  RunLoop();
  ~RunLoop();

  // Returns the RunLoop for the current thread or null if not yet created.
  static RunLoop* current();

  // Registers a RunLoopHandler for the specified handle. Returns an |Id|
  //
  // The handler's OnHandleReady() method is invoked after one of the signals in
  // |handle_signals| occurs. Note that the handler remains registered until
  // explicitly removed or an error occurs.
  //
  // The handler's OnHandleError() method is invoked if the deadline elapses, an
  // error is detected, or the RunLoop is being destroyed (with result
  // MOJO_RESULT_ABORTED in this case). The handler is automatically
  // unregistered before calling OnHandleError(), so it will not receive any
  // further notifications.
  //
  // A handler may call AddHandler() again in both OnHandleReady() and
  // OnHandleError(). Warning: If OnHandleError() was called due to the RunLoop
  // being destroyed, the newly-added handler's OnHandleError() will also be
  // called; this may lead to an infinite loop if it again calls AddHandler() ad
  // infinitum.
  RunLoopHandler::Id AddHandler(RunLoopHandler* handler,
                                const Handle& handle,
                                MojoHandleSignals handle_signals,
                                MojoDeadline deadline);
  void RemoveHandler(RunLoopHandler::Id id);

  // Adds a task to be performed after delay has elapsed.
  void PostDelayedTask(const Closure& task, MojoTimeTicks delay);

  // Runs the loop servicing handles and tasks as they become ready. Returns
  // when Quit() is invoked, or there are no more handles or tasks.
  void Run();

  // Runs the loop servicing any handles and tasks that are ready. Does not wait
  // for handles or tasks to become ready before returning. Returns early if
  // Quit() is invoked.
  void RunUntilIdle();

  void Quit();

  // Returns the number of registered handlers. (This is mostly used for
  // testing.)
  size_t num_handlers() const { return handlers_.size(); }

 private:
  static constexpr MojoTimeTicks kInvalidTimeTicks = 0;

  // Contains the information that was passed to |AddHandler()|. These are
  // stored in |handlers|, which is a map from |RunLoopHandler::Id|s
  // (generated/returned by |AddHandler()| to |HandlerInfo|s. Each entry in
  // |handlers_| also has a corresponding entry in |wait_set_| (with cookie the
  // |RunLoopHandler::Id|).
  struct HandlerInfo {
    HandlerInfo(RunLoopHandler* handler,
                MojoHandleSignals handle_signals,
                MojoTimeTicks absolute_deadline)
        : handler(handler),
          handle_signals(handle_signals),
          absolute_deadline(absolute_deadline) {}

    RunLoopHandler* handler;
    MojoHandleSignals handle_signals;
    // |kInvalidTimeTicks| means forever/no deadline/indefinite.
    MojoTimeTicks absolute_deadline;
  };
  using IdToHandlerInfoMap = std::map<RunLoopHandler::Id, HandlerInfo>;

  // Contains information about a handler with a deadline. These are stored in
  // the |handler_deadlines_| priority queue (with the earliest/lowest
  // |RunLoopHandler::Id| at the top). If |id| is not in |handlers_|, then this
  // deadline is no longer valid (i.e., is stale).
  struct HandlerDeadlineInfo {
    HandlerDeadlineInfo(RunLoopHandler::Id id, MojoTimeTicks absolute_deadline)
        : id(id), absolute_deadline(absolute_deadline) {}

    // Needed to be in a priority queue. Note that |std::priority_queue<>|'s top
    // is the "greatest" element, whereas we want the earliest.
    bool operator<(const HandlerDeadlineInfo& other) const {
      return (absolute_deadline == other.absolute_deadline)
                 ? id < other.id
                 : absolute_deadline > other.absolute_deadline;
    }

    RunLoopHandler::Id id;
    MojoTimeTicks absolute_deadline;
  };
  using HandlerDeadlineQueue = std::priority_queue<HandlerDeadlineInfo>;

  // Contains information about a task posted using |PostDelayedTask()|. (Even
  // though tasks are not handlers, we also assign them |RunLoopHandler::Id|s
  // from the same namespace.) These are stored in the |delayed_tasks_| priority
  // queue (with the earliest/lowest |RunLoopHandler::Id| at the top).
  struct DelayedTaskInfo {
    DelayedTaskInfo(RunLoopHandler::Id id,
                    const Closure& task,
                    MojoTimeTicks absolute_run_time);
    ~DelayedTaskInfo();

    bool operator<(const DelayedTaskInfo& other) const {
      return (absolute_run_time == other.absolute_run_time)
                 ? id > other.id
                 : absolute_run_time > other.absolute_run_time;
    }

    RunLoopHandler::Id id;
    Closure task;
    MojoTimeTicks absolute_run_time;
  };
  using DelayedTaskQueue = std::priority_queue<DelayedTaskInfo>;

  // Inside of |Run()|/|RunUntilIdle()| (i.e., really in |RunInternal()|), we
  // have one of these on the stack. |current_run_state_| points to the current
  // one. (This is needed to handle nested execution.)
  struct RunState;

  // Helper for |Run()| and |RunUntilIdle()|, which loops and executes delayed
  // tasks and handlers as handles become "ready". It will if:
  //   - there are no more tasks or registered handlers,
  //   - |Quit()| is called, or
  //   - no work is done in a given iteration if |quit_when_idle| is true.
  void RunInternal(bool quit_when_idle);

  // Executes one iteration of the run loop. Returns true if the run loop should
  // continue.
  bool DoIteration(bool quit_when_idle);

  // Notifies handlers corresponding to the wait results in |results| (which
  // should not be empty). Returns true if work was done (i.e., any handler was
  // called).
  bool NotifyResults(const std::vector<MojoWaitSetResult>& results);

  // Notifies any handlers with a deadline up to |absolute_deadline| was that
  // their deadline was exceeded. Returns true if work was done (i.e., any
  // handler was called).
  bool NotifyHandlersDeadlineExceeded(MojoTimeTicks absolute_deadline);

  // Calculates the absolute deadline (to be turned into a relative deadline)
  // for the wait set wait. This should only be called if |handlers_| is
  // nonempty. Returns |kInvalidTimeTicks| for "forever"/indefinite. Sets
  // |*is_delayed_task| to true if the deadline is for a delayed task.
  MojoTimeTicks CalculateAbsoluteDeadline(bool* is_delayed_task);

  RunLoopHandler::Id next_id_ = 1u;
  IdToHandlerInfoMap handlers_;
  ScopedWaitSetHandle wait_set_;
  HandlerDeadlineQueue handler_deadlines_;
  DelayedTaskQueue delayed_tasks_;

  RunState* current_run_state_ = nullptr;

  MOJO_DISALLOW_COPY_AND_ASSIGN(RunLoop);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_UTILITY_RUN_LOOP_H_
