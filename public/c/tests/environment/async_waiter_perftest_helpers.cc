// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/tests/environment/async_waiter_perftest_helpers.h"

#include <assert.h>
#include <mojo/system/handle.h>
#include <mojo/system/message_pipe.h>

#include <vector>

#include "mojo/public/cpp/system/macros.h"

#undef CHECK_OK
#ifdef NDEBUG
#define CHECK_OK(result) \
  do {                   \
    (void)result;        \
  } while (false)
#else
#define CHECK_OK(result)                       \
  do {                                         \
    MojoResult result_internal = (result);     \
    assert(result_internal == MOJO_RESULT_OK); \
  } while (false)
#endif

namespace mojo {
namespace test {
namespace {

class AsyncWaiterPerfTest {
 public:
  explicit AsyncWaiterPerfTest(const MojoAsyncWaiter* async_waiter,
                               uint32_t num_handles,
                               std::function<void()> run_loop_function)
      : async_waiter_(async_waiter),
        num_handles_(num_handles),
        run_loop_function_(run_loop_function),
        handle0s_(num_handles, MOJO_HANDLE_INVALID),
        handle1s_(num_handles, MOJO_HANDLE_INVALID),
        contexts_(num_handles) {}
  ~AsyncWaiterPerfTest() {}

  uint64_t DoIt() {
    for (uint32_t i = 0; i < num_handles_; i++) {
      CHECK_OK(MojoCreateMessagePipe(nullptr, &handle0s_[i], &handle1s_[i]));
      AddAsyncWaiter(i);
    }

    // "Signal" the first async wait (i.e., write a message).
    CHECK_OK(MojoWriteMessage(handle1s_[0], nullptr, 0, nullptr, 0,
                              MOJO_WRITE_MESSAGE_FLAG_NONE));

    run_loop_function_();

    for (uint32_t i = 0; i < num_handles_; i++) {
      CancelAsyncWaiter(i);
      CHECK_OK(MojoClose(handle0s_[i]));
      CHECK_OK(MojoClose(handle1s_[i]));
    }

    return callback_count_;
  }

 private:
  struct Context {
    AsyncWaiterPerfTest* thiz = nullptr;
    uint32_t index = 0;
    MojoAsyncWaitID id = 0;
  };

  void AddAsyncWaiter(uint32_t index) {
    assert(index < num_handles_);

    Context& context = contexts_[index];
    context.thiz = this;
    context.index = index;
    context.id = async_waiter_->AsyncWait(
        handle0s_[index], MOJO_HANDLE_SIGNAL_READABLE, MOJO_DEADLINE_INDEFINITE,
        &AsyncWaiterPerfTest::AsyncWaitCallbackThunk, &context);
  }

  void CancelAsyncWaiter(uint32_t index) {
    async_waiter_->CancelWait(contexts_[index].id);
  }

  static void AsyncWaitCallbackThunk(void* closure, MojoResult result) {
    CHECK_OK(result);
    auto context = static_cast<Context*>(closure);
    context->thiz->AsyncWaitCallback(context);
  }

  void AsyncWaitCallback(Context* context) {
    callback_count_++;

    uint32_t index = context->index;

    // "Unsignal" (i.e., consume a message)).
    CHECK_OK(MojoReadMessage(handle0s_[index], nullptr, nullptr, nullptr,
                             nullptr, MOJO_READ_MESSAGE_FLAG_MAY_DISCARD));

    // Replace ourself.
    AddAsyncWaiter(index);

    // "Signal" the next one (i.e., write a message).
    CHECK_OK(MojoWriteMessage(handle1s_[(index + 1) % num_handles_], nullptr, 0,
                              nullptr, 0, MOJO_WRITE_MESSAGE_FLAG_NONE));
  }

  const MojoAsyncWaiter* const async_waiter_;
  const uint32_t num_handles_;
  const std::function<void()> run_loop_function_;

  // We'll always wait on the |handle0s_| and "signal" from the |handle1s_|.
  std::vector<MojoHandle> handle0s_;
  std::vector<MojoHandle> handle1s_;
  std::vector<Context> contexts_;

  uint64_t callback_count_ = 0;

  MOJO_DISALLOW_COPY_AND_ASSIGN(AsyncWaiterPerfTest);
};

}  // namespace

uint64_t DoAsyncWaiterPerfTest(const MojoAsyncWaiter* async_waiter,
                               uint32_t num_handles,
                               std::function<void()> run_loop_function) {
  return AsyncWaiterPerfTest(async_waiter, num_handles, run_loop_function)
      .DoIt();
}

}  // namespace test
}  // namespace mojo
