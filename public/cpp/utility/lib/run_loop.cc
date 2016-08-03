// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/utility/run_loop.h"

#include <assert.h>
#include <pthread.h>

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include "mojo/public/c/system/macros.h"
#include "mojo/public/cpp/system/time.h"
#include "mojo/public/cpp/system/wait.h"
#include "mojo/public/cpp/utility/run_loop_handler.h"

namespace mojo {
namespace {

// The initial and maximum number of results that we'll accept from
// |WaitSetWait()|. TODO(vtl): I just made up these numbers.
constexpr uint32_t kInitialWaitSetNumResults = 16u;
constexpr uint32_t kMaximumWaitSetNumResults = 256u;

pthread_key_t g_current_run_loop_key;

// Ensures that the "current run loop" functionality is available (i.e., that we
// have a TLS slot).
void InitializeCurrentRunLoopIfNecessary() {
  static pthread_once_t current_run_loop_key_once = PTHREAD_ONCE_INIT;
  int error = pthread_once(&current_run_loop_key_once, []() {
    int error = pthread_key_create(&g_current_run_loop_key, nullptr);
    MOJO_ALLOW_UNUSED_LOCAL(error);
    assert(!error);
  });
  MOJO_ALLOW_UNUSED_LOCAL(error);
  assert(!error);
}

void SetCurrentRunLoop(RunLoop* run_loop) {
  InitializeCurrentRunLoopIfNecessary();

  int error = pthread_setspecific(g_current_run_loop_key, run_loop);
  MOJO_ALLOW_UNUSED_LOCAL(error);
  assert(!error);
}

}  // namespace

RunLoop::DelayedTaskInfo::DelayedTaskInfo(RunLoopHandler::Id id,
                                          const Closure& task,
                                          MojoTimeTicks absolute_run_time)
    : id(id), task(task), absolute_run_time(absolute_run_time) {}

RunLoop::DelayedTaskInfo::~DelayedTaskInfo() {}

struct RunLoop::RunState {
  bool should_quit = false;
  uint32_t results_size = kInitialWaitSetNumResults;
  std::vector<MojoWaitSetResult> results;
};

// static
constexpr MojoTimeTicks RunLoop::kInvalidTimeTicks;

RunLoop::RunLoop() {
  MojoResult result = CreateWaitSet(nullptr, &wait_set_);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
  assert(wait_set_.is_valid());

  assert(!current());
  SetCurrentRunLoop(this);
}

RunLoop::~RunLoop() {
  assert(current() == this);

  // Notify all handlers that they've been aborted. Note that handlers could
  // conceivably call |RemoveHandler()| (which would be a bit shady, admittedly,
  // even if we handle it correctly). (They could also call |AddHandler()|,
  // which would be even shadier; we handle this "correctly", but we may still
  // end up looping infinitely in that case.)
  while (!handlers_.empty()) {
    auto it = handlers_.begin();
    auto handler = it->second.handler;
    auto id = it->first;
    handlers_.erase(it);
    handler->OnHandleError(id, MOJO_RESULT_ABORTED);
  }

  SetCurrentRunLoop(nullptr);
}

// static
RunLoop* RunLoop::current() {
  InitializeCurrentRunLoopIfNecessary();
  return static_cast<RunLoop*>(pthread_getspecific(g_current_run_loop_key));
}

RunLoopHandler::Id RunLoop::AddHandler(RunLoopHandler* handler,
                                       const Handle& handle,
                                       MojoHandleSignals handle_signals,
                                       MojoDeadline deadline) {
  assert(current() == this);
  assert(handler);
  assert(handle.is_valid());

  // Generate a |RunLoopHandler::Id|.
  auto id = next_id_++;

  // Calculate the absolute deadline.
  auto absolute_deadline = kInvalidTimeTicks;  // Default to "forever".
  static constexpr auto kMaxMojoTimeTicks =
      std::numeric_limits<MojoTimeTicks>::max();
  if (deadline <= static_cast<MojoDeadline>(kMaxMojoTimeTicks)) {
    auto now = GetTimeTicksNow();
    if (deadline <= static_cast<MojoDeadline>(kMaxMojoTimeTicks - now)) {
      absolute_deadline = now + static_cast<MojoTimeTicks>(deadline);
      handler_deadlines_.push(HandlerDeadlineInfo(id, absolute_deadline));
    }
    // Else either |deadline| or |now| is so large (hopefully the former) that
    // |now + deadline| would overflow. We'll take that to mean forever.
  }
  // Else |deadline| is either very large (which we may as well take as forever)
  // or |MOJO_DEADLINE_INDEFINITE| (which is forever).

  // Add an entry to |handlers_|.
  handlers_.insert(std::make_pair(
      id, HandlerInfo(handler, handle_signals, absolute_deadline)));
  // Add an entry to the wait set.
  MojoResult result =
      WaitSetAdd(wait_set_.get(), handle, handle_signals, id, nullptr);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);

  return id;
}

void RunLoop::RemoveHandler(RunLoopHandler::Id id) {
  assert(current() == this);

  // Remove the entry from |handlers_|.
  auto it = handlers_.find(id);
  if (it == handlers_.end())
    return;
  handlers_.erase(it);
  // Remove the entry from the wait set.
  MojoResult result = WaitSetRemove(wait_set_.get(), id);
  MOJO_ALLOW_UNUSED_LOCAL(result);
  assert(result == MOJO_RESULT_OK);
}

void RunLoop::PostDelayedTask(const Closure& task, MojoTimeTicks delay) {
  assert(current() == this);

  // Generate a |RunLoopHandler::Id|.
  auto id = next_id_++;

  // Calculate the absolute run time.
  auto now = GetTimeTicksNow();
  assert(delay <= std::numeric_limits<MojoTimeTicks>::max() - now);
  auto absolute_run_time = now + delay;

  // Add an entry to |delayed_tasks_|.
  delayed_tasks_.push(DelayedTaskInfo(id, task, absolute_run_time));
}

void RunLoop::Run() {
  RunInternal(false);
}

void RunLoop::RunUntilIdle() {
  RunInternal(true);
}

void RunLoop::Quit() {
  assert(current() == this);

  if (current_run_state_)
    current_run_state_->should_quit = true;
}

void RunLoop::RunInternal(bool quit_when_idle) {
  assert(current() == this);

  auto old_run_state = current_run_state_;
  RunState run_state;
  current_run_state_ = &run_state;

  while (DoIteration(quit_when_idle))
    ;  // The work is done in |DoIteration()|.

  current_run_state_ = old_run_state;
}

bool RunLoop::DoIteration(bool quit_when_idle) {
  assert(current_run_state_);
  RunState& run_state = *current_run_state_;
  assert(!run_state.should_quit);

  bool should_continue = false;

  auto now = GetTimeTicksNow();

  // First, execute any already-enqueued tasks that are ready.

  // This is a fake task that we use to compare to enqueued tasks (if one were
  // to post a task now with no delay, it'd look like this). This is convenient
  // since |DelayedTaskInfo| has an |operator<| (used by the priority queue
  // |delayed_tasks_|).
  //
  // We want to execute tasks that are "greater" than |now_task| (i.e.,
  // |now_task| is less than them) -- this includes all tasks that are currently
  // ready, but not any newly-posted tasks (i.e., those that are posted as a
  // result of executing ready tasks).
  DelayedTaskInfo now_task(next_id_, Closure(), now);

  while (!delayed_tasks_.empty() && now_task < delayed_tasks_.top()) {
    // We could just execute the task directly from |delayed_tasks_.top()|,
    // since no newly-posted task should change the top of the priority queue,
    // but doing the below is more obviously correct.
    Closure task = delayed_tasks_.top().task;
    delayed_tasks_.pop();
    task.Run();
    should_continue = true;

    if (run_state.should_quit)
      return false;
  }

  // Next, "wait" and deal with handles/handlers.

  if (handlers_.empty())
    return should_continue;

  // Calculate the deadline for the wait. Don't wait if |quit_when_idle| is
  // true. Otherwise, the minimum of the earliest delayed task run time and the
  // earliest handler deadline (or "forever" if there are no delayed tasks and
  // no handler deadlines). (Warning: |CalculateAbsoluteDeadline()| may return a
  // deadline earlier than |now|.)
  bool absolute_deadline_is_for_delayed_task = false;
  MojoTimeTicks absolute_deadline =
      quit_when_idle
          ? now
          : CalculateAbsoluteDeadline(&absolute_deadline_is_for_delayed_task);
  MojoDeadline relative_deadline =
      (absolute_deadline == kInvalidTimeTicks)
          ? MOJO_DEADLINE_INDEFINITE
          : static_cast<MojoDeadline>(std::max(now, absolute_deadline) - now);

  run_state.results.resize(run_state.results_size);
  uint32_t max_results = run_state.results_size;
  switch (WaitSetWait(wait_set_.get(), relative_deadline,
                      &current_run_state_->results, &max_results)) {
    case MOJO_RESULT_OK:
      // If there were more results than we could accept, try increasing the
      // number we accept (up to our limit).
      if (max_results > run_state.results_size) {
        run_state.results_size =
            std::min(kMaximumWaitSetNumResults, run_state.results_size * 2u);
      }
      should_continue |= NotifyResults(run_state.results);
      break;
    case MOJO_RESULT_INVALID_ARGUMENT:
      assert(false);  // This shouldn't happen.
      return false;
    case MOJO_RESULT_CANCELLED:
      assert(false);  // This shouldn't happen.
      return false;
    case MOJO_RESULT_RESOURCE_EXHAUSTED:
      assert(false);  // Sadness.
      return false;
    case MOJO_RESULT_BUSY:
      assert(false);  // This shouldn't happen.
      return false;
    case MOJO_RESULT_DEADLINE_EXCEEDED:
      should_continue |= NotifyHandlersDeadlineExceeded(absolute_deadline);
      // If we timed out due for a delayed task, pretend that we did work since
      // we're not idle yet (there'll be work to do immediately the next time
      // through the loop).
      should_continue |= absolute_deadline_is_for_delayed_task;
      break;
    default:
      assert(false);  // This *really* shouldn't happen.
      return false;
  }

  if (run_state.should_quit)
    return false;

  return quit_when_idle ? should_continue : !handlers_.empty();
}

MojoTimeTicks RunLoop::CalculateAbsoluteDeadline(bool* is_delayed_task) {
  assert(!handlers_.empty());

  // Default to "forever".
  MojoTimeTicks absolute_deadline = kInvalidTimeTicks;
  if (delayed_tasks_.empty()) {
    *is_delayed_task = false;
  } else {
    // If there are delayed tasks, our deadline can be no later than the
    // earliest run time.
    absolute_deadline = delayed_tasks_.top().absolute_run_time;
    *is_delayed_task = true;
  }

  // Find the earliest handler deadline.
  while (!handler_deadlines_.empty()) {
    const HandlerDeadlineInfo& info = handler_deadlines_.top();
    const auto it = handlers_.find(info.id);
    // We might have a stale entry at the top. If so, remove it and continue.
    if (it == handlers_.end()) {
      handler_deadlines_.pop();
      continue;
    }

    if (absolute_deadline == kInvalidTimeTicks ||
        info.absolute_deadline < absolute_deadline) {
      absolute_deadline = info.absolute_deadline;
      *is_delayed_task = false;
    }

    break;
  }

  return absolute_deadline;
}

bool RunLoop::NotifyResults(const std::vector<MojoWaitSetResult>& results) {
  assert(!results.empty());

  bool did_work = false;
  for (const auto& result : results) {
    auto id = result.cookie;
    auto it = handlers_.find(id);
    // Though we should find an entry for the first result, a handler that we
    // invoke may remove other handlers.
    if (it == handlers_.end())
      continue;

    auto handler = it->second.handler;
    handlers_.erase(it);
    MojoResult r = WaitSetRemove(wait_set_.get(), id);
    MOJO_ALLOW_UNUSED_LOCAL(r);
    assert(r == MOJO_RESULT_OK);
    if (result.wait_result == MOJO_RESULT_OK)
      handler->OnHandleReady(id);
    else
      handler->OnHandleError(id, result.wait_result);
    did_work = true;

    if (current_run_state_->should_quit)
      break;
  }
  return did_work;
}

bool RunLoop::NotifyHandlersDeadlineExceeded(MojoTimeTicks absolute_deadline) {
  assert(!handlers_.empty());
  assert(absolute_deadline != kInvalidTimeTicks);

  bool did_work = false;
  while (!handler_deadlines_.empty()) {
    const HandlerDeadlineInfo& info = handler_deadlines_.top();

    if (info.absolute_deadline > absolute_deadline)
      break;

    const auto it = handlers_.find(info.id);
    // Though the top shouldn't be stale, there may be stale entries after it
    // (with the same deadline). Moreover, previously-run handlers may have
    // removed yet-to-be-run handlers.
    if (it == handlers_.end()) {
      handler_deadlines_.pop();
      continue;
    }

    auto handler = it->second.handler;
    auto id = info.id;
    handlers_.erase(it);       // Invalidates |it|.
    handler_deadlines_.pop();  // Invalidates |info|.
    handler->OnHandleError(id, MOJO_RESULT_DEADLINE_EXCEEDED);
    did_work = true;

    if (current_run_state_->should_quit)
      break;
  }
  return did_work;
}

}  // namespace mojo
