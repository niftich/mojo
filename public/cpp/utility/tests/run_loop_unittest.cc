// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/utility/run_loop.h"

#include <string>

#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/cpp/test_support/test_utils.h"
#include "mojo/public/cpp/utility/run_loop_handler.h"
#include "third_party/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

class TestRunLoopHandler : public RunLoopHandler {
 public:
  TestRunLoopHandler() {}
  ~TestRunLoopHandler() override {}

  void clear_ready_count() { ready_count_ = 0; }
  int ready_count() const { return ready_count_; }

  void clear_error_count() { error_count_ = 0; }
  int error_count() const { return error_count_; }

  void clear_expected_handler_id() { have_expected_handler_id_ = false; }
  void set_expected_handler_id(Id id) {
    have_expected_handler_id_ = true;
    expected_handler_id_ = id;
  }

  MojoResult last_error_result() const { return last_error_result_; }

  // RunLoopHandler:
  void OnHandleReady(Id id) override {
    ready_count_++;
    if (have_expected_handler_id_) {
      EXPECT_EQ(expected_handler_id_, id);
      clear_expected_handler_id();
    }
  }

  void OnHandleError(Id id, MojoResult result) override {
    error_count_++;
    last_error_result_ = result;
    if (have_expected_handler_id_) {
      EXPECT_EQ(expected_handler_id_, id);
      clear_expected_handler_id();
    }
  }

 private:
  int ready_count_ = 0;
  int error_count_ = 0;
  bool have_expected_handler_id_ = false;
  Id expected_handler_id_ = 0u;
  MojoResult last_error_result_ = MOJO_RESULT_OK;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestRunLoopHandler);
};

// Trivial test to verify Run() with no added handles returns.
TEST(RunLoopTest, ExitsWithNoHandles) {
  RunLoop run_loop;
  run_loop.Run();
}

class RemoveOnReadyRunLoopHandler : public TestRunLoopHandler {
 public:
  RemoveOnReadyRunLoopHandler() : run_loop_(nullptr) {}
  ~RemoveOnReadyRunLoopHandler() override {}

  void set_run_loop(RunLoop* run_loop) { run_loop_ = run_loop; }

  // RunLoopHandler:
  void OnHandleReady(Id id) override {
    run_loop_->RemoveHandler(id);
    TestRunLoopHandler::OnHandleReady(id);
  }

 private:
  RunLoop* run_loop_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(RemoveOnReadyRunLoopHandler);
};

// Verifies RunLoop quits when no more handles (handle is removed when ready).
TEST(RunLoopTest, HandleReady) {
  RemoveOnReadyRunLoopHandler handler;
  MessagePipe test_pipe;
  EXPECT_TRUE(test::WriteTextMessage(test_pipe.handle1.get(), std::string()));

  RunLoop run_loop;
  handler.set_run_loop(&run_loop);
  auto id = run_loop.AddHandler(&handler, test_pipe.handle0.get(),
                                MOJO_HANDLE_SIGNAL_READABLE,
                                MOJO_DEADLINE_INDEFINITE);
  handler.set_expected_handler_id(id);
  run_loop.Run();
  EXPECT_EQ(1, handler.ready_count());
  EXPECT_EQ(0, handler.error_count());
  EXPECT_EQ(0u, run_loop.num_handlers());
}

class QuitOnReadyRunLoopHandler : public TestRunLoopHandler {
 public:
  QuitOnReadyRunLoopHandler() {}
  ~QuitOnReadyRunLoopHandler() override {}

  void set_run_loop(RunLoop* run_loop) { run_loop_ = run_loop; }

  // RunLoopHandler:
  void OnHandleReady(Id id) override {
    run_loop_->Quit();
    TestRunLoopHandler::OnHandleReady(id);
  }

 private:
  RunLoop* run_loop_ = nullptr;

  MOJO_DISALLOW_COPY_AND_ASSIGN(QuitOnReadyRunLoopHandler);
};

// Verifies Quit() from OnHandleReady() quits the loop.
TEST(RunLoopTest, QuitFromReady) {
  QuitOnReadyRunLoopHandler handler;
  MessagePipe test_pipe;
  EXPECT_TRUE(test::WriteTextMessage(test_pipe.handle1.get(), std::string()));

  RunLoopHandler::Id id2;
  {
    RunLoop run_loop;
    handler.set_run_loop(&run_loop);
    auto id1 = run_loop.AddHandler(&handler, test_pipe.handle0.get(),
                                   MOJO_HANDLE_SIGNAL_READABLE,
                                   MOJO_DEADLINE_INDEFINITE);
    handler.set_expected_handler_id(id1);
    // Quitting should keep this handler.
    id2 = run_loop.AddHandler(&handler, test_pipe.handle1.get(),
                              MOJO_HANDLE_SIGNAL_READABLE,
                              MOJO_DEADLINE_INDEFINITE);
    EXPECT_NE(id2, id1);
    run_loop.Run();
    EXPECT_EQ(1, handler.ready_count());
    EXPECT_EQ(0, handler.error_count());
    EXPECT_EQ(1u, run_loop.num_handlers());

    // Destroying the RunLoop should call the second handler's OnHandleError().
    handler.set_expected_handler_id(id2);
  }
}

class QuitOnErrorRunLoopHandler : public TestRunLoopHandler {
 public:
  QuitOnErrorRunLoopHandler() {}
  ~QuitOnErrorRunLoopHandler() override {}

  void set_run_loop(RunLoop* run_loop) { run_loop_ = run_loop; }

  // RunLoopHandler:
  void OnHandleError(Id id, MojoResult result) override {
    run_loop_->Quit();
    TestRunLoopHandler::OnHandleError(id, result);
  }

 private:
  RunLoop* run_loop_ = nullptr;

  MOJO_DISALLOW_COPY_AND_ASSIGN(QuitOnErrorRunLoopHandler);
};

// Verifies Quit() when the deadline is reached works.
TEST(RunLoopTest, QuitWhenDeadlineExpired) {
  QuitOnErrorRunLoopHandler handler;
  MessagePipe test_pipe;
  RunLoop run_loop;
  handler.set_run_loop(&run_loop);
  auto id = run_loop.AddHandler(&handler, test_pipe.handle0.get(),
                                MOJO_HANDLE_SIGNAL_READABLE,
                                static_cast<MojoDeadline>(10000));
  handler.set_expected_handler_id(id);
  run_loop.Run();
  EXPECT_EQ(0, handler.ready_count());
  EXPECT_EQ(1, handler.error_count());
  EXPECT_EQ(MOJO_RESULT_DEADLINE_EXCEEDED, handler.last_error_result());
  EXPECT_EQ(0u, run_loop.num_handlers());
}

// Test that handlers are notified of loop destruction.
TEST(RunLoopTest, Destruction) {
  TestRunLoopHandler handler;
  MessagePipe test_pipe;
  {
    RunLoop run_loop;
    auto id = run_loop.AddHandler(&handler, test_pipe.handle0.get(),
                                  MOJO_HANDLE_SIGNAL_READABLE,
                                  MOJO_DEADLINE_INDEFINITE);
    handler.set_expected_handler_id(id);
  }
  EXPECT_EQ(1, handler.error_count());
  EXPECT_EQ(MOJO_RESULT_ABORTED, handler.last_error_result());
}

class RemoveManyRunLoopHandler : public TestRunLoopHandler {
 public:
  RemoveManyRunLoopHandler() {}
  ~RemoveManyRunLoopHandler() override {}

  void set_run_loop(RunLoop* run_loop) { run_loop_ = run_loop; }
  void add_id(Id id) { ids_.push_back(id); }

  // RunLoopHandler:
  void OnHandleError(Id id, MojoResult result) override {
    for (size_t i = 0; i < ids_.size(); i++)
      run_loop_->RemoveHandler(ids_[i]);
    TestRunLoopHandler::OnHandleError(id, result);
  }

 private:
  std::vector<Id> ids_;
  RunLoop* run_loop_ = nullptr;

  MOJO_DISALLOW_COPY_AND_ASSIGN(RemoveManyRunLoopHandler);
};

// Test that handlers are notified of loop destruction.
TEST(RunLoopTest, MultipleHandleDestruction) {
  RemoveManyRunLoopHandler odd_handler;
  TestRunLoopHandler even_handler;
  MessagePipe test_pipe1, test_pipe2, test_pipe3;
  {
    RunLoop run_loop;
    odd_handler.set_run_loop(&run_loop);
    odd_handler.add_id(run_loop.AddHandler(
        &odd_handler, test_pipe1.handle0.get(), MOJO_HANDLE_SIGNAL_READABLE,
        MOJO_DEADLINE_INDEFINITE));
    auto even_id = run_loop.AddHandler(&even_handler, test_pipe2.handle0.get(),
                                       MOJO_HANDLE_SIGNAL_READABLE,
                                       MOJO_DEADLINE_INDEFINITE);
    even_handler.set_expected_handler_id(even_id);
    odd_handler.add_id(run_loop.AddHandler(
        &odd_handler, test_pipe3.handle0.get(), MOJO_HANDLE_SIGNAL_READABLE,
        MOJO_DEADLINE_INDEFINITE));
  }
  EXPECT_EQ(1, odd_handler.error_count());
  EXPECT_EQ(1, even_handler.error_count());
  EXPECT_EQ(MOJO_RESULT_ABORTED, odd_handler.last_error_result());
  EXPECT_EQ(MOJO_RESULT_ABORTED, even_handler.last_error_result());
}

class AddHandlerOnErrorHandler : public TestRunLoopHandler {
 public:
  AddHandlerOnErrorHandler() {}
  ~AddHandlerOnErrorHandler() override {}

  void set_run_loop(RunLoop* run_loop) { run_loop_ = run_loop; }
  void set_handle(Handle handle) { handle_ = handle; }

  // RunLoopHandler:
  void OnHandleError(Id id, MojoResult result) override {
    EXPECT_EQ(MOJO_RESULT_ABORTED, result);

    TestRunLoopHandler::OnHandleError(id, result);

    if (!on_handle_error_was_called_) {
      on_handle_error_was_called_ = true;
      auto id = run_loop_->AddHandler(
          this, handle_, MOJO_HANDLE_SIGNAL_READABLE, MOJO_DEADLINE_INDEFINITE);
      set_expected_handler_id(id);
    }
  }

 private:
  RunLoop* run_loop_ = nullptr;
  bool on_handle_error_was_called_ = false;
  Handle handle_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(AddHandlerOnErrorHandler);
};

TEST(RunLoopTest, AddHandlerOnError) {
  AddHandlerOnErrorHandler handler;
  MessagePipe test_pipe;
  {
    RunLoop run_loop;
    handler.set_run_loop(&run_loop);
    handler.set_handle(test_pipe.handle0.get());
    auto id = run_loop.AddHandler(&handler, test_pipe.handle0.get(),
                                  MOJO_HANDLE_SIGNAL_READABLE,
                                  MOJO_DEADLINE_INDEFINITE);
    handler.set_expected_handler_id(id);
  }
  EXPECT_EQ(2, handler.error_count());
}

TEST(RunLoopTest, Current) {
  EXPECT_TRUE(RunLoop::current() == nullptr);
  {
    RunLoop run_loop;
    EXPECT_EQ(&run_loop, RunLoop::current());
  }
  EXPECT_TRUE(RunLoop::current() == nullptr);
}

class NestingRunLoopHandler : public TestRunLoopHandler {
 public:
  static constexpr size_t kDepthLimit = 10;
  static constexpr char kSignalMagic = 'X';

  NestingRunLoopHandler() {}
  ~NestingRunLoopHandler() override {}

  void set_run_loop(RunLoop* run_loop) { run_loop_ = run_loop; }
  void set_pipe(MessagePipe* pipe) { pipe_ = pipe; }
  bool reached_depth_limit() const { return reached_depth_limit_; }

  // RunLoopHandler:
  void OnHandleReady(Id id) override {
    TestRunLoopHandler::OnHandleReady(id);

    ReadSignal();
    size_t current_depth = ++depth_;
    if (current_depth < kDepthLimit) {
      AddHandlerAndWriteSignal();
      run_loop_->Run();
      // The innermost loop stops running due to Quit() being called; the outer
      // loops stop running due to having no more handlers. No errors/timeouts
      // should ever occur.
      EXPECT_EQ(error_count(), 0);
    } else {
      EXPECT_EQ(current_depth, kDepthLimit);
      reached_depth_limit_ = true;
      run_loop_->Quit();
    }
    --depth_;
  }

  void AddHandlerAndWriteSignal() {
    run_loop_->AddHandler(this, pipe_->handle0.get(),
                          MOJO_HANDLE_SIGNAL_READABLE,
                          static_cast<MojoDeadline>(10000));
    MojoResult write_result =
        WriteMessageRaw(pipe_->handle1.get(), &kSignalMagic, 1, nullptr, 0,
                        MOJO_WRITE_MESSAGE_FLAG_NONE);
    EXPECT_EQ(write_result, MOJO_RESULT_OK);
  }

  void ReadSignal() {
    char read_byte = 0;
    uint32_t bytes_read = 1;
    uint32_t handles_read = 0;
    MojoResult read_result =
        ReadMessageRaw(pipe_->handle0.get(), &read_byte, &bytes_read, nullptr,
                       &handles_read, MOJO_READ_MESSAGE_FLAG_NONE);
    EXPECT_EQ(read_result, MOJO_RESULT_OK);
    EXPECT_EQ(read_byte, kSignalMagic);
  }

 private:
  RunLoop* run_loop_ = nullptr;
  MessagePipe* pipe_ = nullptr;
  size_t depth_ = 0u;
  bool reached_depth_limit_ = false;

  MOJO_DISALLOW_COPY_AND_ASSIGN(NestingRunLoopHandler);
};

// static
const size_t NestingRunLoopHandler::kDepthLimit;

// static
const char NestingRunLoopHandler::kSignalMagic;

TEST(RunLoopTest, NestedRun) {
  NestingRunLoopHandler handler;
  MessagePipe test_pipe;
  RunLoop run_loop;
  handler.set_run_loop(&run_loop);
  handler.set_pipe(&test_pipe);
  handler.AddHandlerAndWriteSignal();
  run_loop.Run();

  EXPECT_TRUE(handler.reached_depth_limit());
  // See the comment in NestingRunLoopHandler::OnHandleReady() above.
  EXPECT_EQ(handler.error_count(), 0);
}

struct Task {
  Task(int num, std::vector<int>* sequence) : num(num), sequence(sequence) {}

  void Run() const { sequence->push_back(num); }

  int num;
  std::vector<int>* sequence;
};

TEST(RunLoopTest, DelayedTaskOrder) {
  std::vector<int> sequence;
  RunLoop run_loop;
  run_loop.PostDelayedTask(Closure(Task(1, &sequence)), 0);
  run_loop.PostDelayedTask(Closure(Task(2, &sequence)), 0);
  run_loop.PostDelayedTask(Closure(Task(3, &sequence)), 0);
  run_loop.RunUntilIdle();

  ASSERT_EQ(3u, sequence.size());
  EXPECT_EQ(1, sequence[0]);
  EXPECT_EQ(2, sequence[1]);
  EXPECT_EQ(3, sequence[2]);
}

struct QuittingTask {
  explicit QuittingTask(RunLoop* run_loop) : run_loop(run_loop) {}

  void Run() const { run_loop->Quit(); }

  RunLoop* run_loop;
};

TEST(RunLoopTest, QuitFromDelayedTask) {
  TestRunLoopHandler handler;
  MessagePipe test_pipe;
  RunLoop run_loop;
  run_loop.AddHandler(&handler,
                      test_pipe.handle0.get(),
                      MOJO_HANDLE_SIGNAL_READABLE,
                      MOJO_DEADLINE_INDEFINITE);
  run_loop.PostDelayedTask(Closure(QuittingTask(&run_loop)), 0);
  run_loop.Run();
}

}  // namespace
}  // namespace mojo
