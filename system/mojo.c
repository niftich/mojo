// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <magenta/processargs.h>
#include <magenta/syscalls.h>
#include <runtime/process.h>
#include <string.h>

#include "mojo/public/c/system/handle.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/c/system/message_pipe.h"
#include "mojo/public/c/system/wait.h"

// MojoTimeTicks is in microseconds.
// mx_time_t is in nanoseconds.

// TODO(abarth): MojoTimeTicks probably shouldn't be signed.
static MojoTimeTicks TimeToMojoTicks(mx_time_t t) {
  return (MojoTimeTicks) t / 1000u;
}

static mx_time_t MojoDeadlineToTime(MojoDeadline deadline) {
  if (deadline == MOJO_DEADLINE_INDEFINITE)
    return MX_TIME_INFINITE;
  // TODO(abarth): Handle overflow.
  return (mx_time_t) deadline * 1000u;
}

// TODO(jamesr): Unify error codes.
static MojoResult StatusToMojoResult(mx_status_t err) {
  switch (err) {
    case NO_ERROR:
      return MOJO_RESULT_OK;
    case ERR_INTERNAL:
      return MOJO_RESULT_INTERNAL;
    case ERR_NOT_FOUND:
      return MOJO_RESULT_NOT_FOUND;
    case ERR_NOT_READY:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_NO_MEMORY:
      return MOJO_RESULT_RESOURCE_EXHAUSTED;
    case ERR_ALREADY_STARTED:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_NOT_VALID:
      return MOJO_RESULT_INVALID_ARGUMENT;
    case ERR_INVALID_ARGS:
      return MOJO_RESULT_INVALID_ARGUMENT;
    case ERR_NOT_ENOUGH_BUFFER:
      return MOJO_RESULT_RESOURCE_EXHAUSTED;
    case ERR_NOT_BLOCKED:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_TIMED_OUT:
      return MOJO_RESULT_DEADLINE_EXCEEDED;
    case ERR_ALREADY_EXISTS:
      return MOJO_RESULT_ALREADY_EXISTS;
    case ERR_CHANNEL_CLOSED:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_NOT_ALLOWED:
      return MOJO_RESULT_PERMISSION_DENIED;
    case ERR_BAD_PATH:
      return MOJO_RESULT_INVALID_ARGUMENT;
    case ERR_IO:
      return MOJO_RESULT_INTERNAL;
    case ERR_NOT_DIR:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_NOT_FILE:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_RECURSE_TOO_DEEP:
      return MOJO_RESULT_INTERNAL;
    case ERR_NOT_SUPPORTED:
      return MOJO_RESULT_UNIMPLEMENTED;
    case ERR_TOO_BIG:
      return MOJO_RESULT_OUT_OF_RANGE;
    case ERR_CANCELLED:
      return MOJO_RESULT_ABORTED;
    case ERR_NOT_IMPLEMENTED:
      return MOJO_RESULT_UNIMPLEMENTED;
    case ERR_CHECKSUM_FAIL:
      return MOJO_RESULT_DATA_LOSS;
    case ERR_BAD_STATE:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_BUSY:
      return MOJO_RESULT_BUSY;
    case ERR_THREAD_DETACHED:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_I2C_NACK:
      return MOJO_RESULT_DATA_LOSS;
    case ERR_OUT_OF_RANGE:
      return MOJO_RESULT_OUT_OF_RANGE;
    case ERR_NOT_MOUNTED:
      return MOJO_RESULT_FAILED_PRECONDITION;
    case ERR_FAULT:
      return MOJO_RESULT_INTERNAL;
    case ERR_NO_RESOURCES:
      return MOJO_RESULT_RESOURCE_EXHAUSTED;
    case ERR_BAD_HANDLE:
      return MOJO_RESULT_INTERNAL;
    case ERR_ACCESS_DENIED:
      return MOJO_RESULT_PERMISSION_DENIED;
    default:
      return MOJO_RESULT_UNKNOWN;
  }
}

// handle.h --------------------------------------------------------------------

MojoResult MojoClose(MojoHandle handle) {
  return StatusToMojoResult(mx_handle_close(handle));
}

MojoResult MojoDuplicateHandle(MojoHandle handle, MojoHandle* new_handle) {
  mx_handle_t result = mx_handle_duplicate(handle, MX_RIGHT_SAME_RIGHTS);
  if (result < 0)
    return StatusToMojoResult(result);
  *new_handle = result;
  return MOJO_RESULT_OK;
}

// TODO(abarth): MojoGetRights, MojoReplaceHandleWithReducedRights,
// MojoDuplicateHandleWithReducedRights

// time.h ----------------------------------------------------------------------

MojoTimeTicks MojoGetTimeTicksNow() {
  return TimeToMojoTicks(mx_current_time());
}

// wait.h ----------------------------------------------------------------------

static_assert(MOJO_HANDLE_SIGNAL_NONE == MX_SIGNAL_NONE,
              "SIGNAL_NONE must match");
static_assert(MOJO_HANDLE_SIGNAL_READABLE == MX_SIGNAL_READABLE,
              "SIGNAL_READABLE must match");
static_assert(MOJO_HANDLE_SIGNAL_WRITABLE == MX_SIGNAL_WRITABLE,
              "SIGNAL_WRITABLE must match");
static_assert(MOJO_HANDLE_SIGNAL_PEER_CLOSED == MX_SIGNAL_PEER_CLOSED,
              "PEER_CLOSED must match");

MojoResult MojoWait(MojoHandle handle,
                    MojoHandleSignals signals,
                    MojoDeadline deadline,
                    struct MojoHandleSignalsState* signals_state) {
  mx_time_t mx_deadline = MojoDeadlineToTime(deadline);
  mx_signals_t* satisfied_signals = NULL;
  mx_signals_t* satisfiable_signals = NULL;
  if (signals_state) {
    satisfied_signals = (mx_signals_t*) &signals_state->satisfied_signals;
    satisfiable_signals = (mx_signals_t*) &signals_state->satisfiable_signals;
  }
  return StatusToMojoResult(mx_handle_wait_one(
      handle, signals, mx_deadline, satisfied_signals, satisfiable_signals));
}

static mx_status_t WaitManyWithSignalsState(
    uint32_t num_handles,
    mx_handle_t* handles,
    mx_signals_t* signals,
    mx_time_t deadline,
    struct MojoHandleSignalsState* signals_states) {
  mx_signals_t satisfied_signals[num_handles];
  mx_signals_t satisfiable_signals[num_handles];
  mx_status_t status = mx_handle_wait_many(num_handles,
                                           handles,
                                           signals,
                                           deadline,
                                           satisfied_signals,
                                           satisfiable_signals);
  for (uint32_t i = 0; i < num_handles; ++i) {
    signals_states[i].satisfied_signals = satisfied_signals[i];
    signals_states[i].satisfiable_signals = satisfiable_signals[i];
  }
  return status;
}

MojoResult MojoWaitMany(const MojoHandle* handles,
                        const MojoHandleSignals* signals,
                        uint32_t num_handles,
                        MojoDeadline deadline,
                        uint32_t* result_index,
                        struct MojoHandleSignalsState* signals_states) {
  mx_handle_t* mx_handles = (mx_handle_t*)handles;
  mx_signals_t* mx_signals = (mx_signals_t*)signals;
  mx_time_t mx_deadline = MojoDeadlineToTime(deadline);

  mx_status_t status;
  if (signals_states) {
    status = WaitManyWithSignalsState(num_handles, mx_handles, mx_signals,
                                      mx_deadline, signals_states);
  } else {
    status = mx_handle_wait_many(num_handles, mx_handles, mx_signals,
                                 mx_deadline, NULL, NULL);
  }

  if (result_index) {
    // TODO(cpu): implement |result_index|, see MG-33 bug.
    *result_index = 0;
  }

  return StatusToMojoResult(status);
}

// message_pipe.h --------------------------------------------------------------

MojoResult MojoCreateMessagePipe(
    const struct MojoCreateMessagePipeOptions* options,
    MojoHandle* message_pipe_handle0,
    MojoHandle* message_pipe_handle1) {
  if (options && options->flags != MOJO_CREATE_MESSAGE_PIPE_OPTIONS_FLAG_NONE)
    return MOJO_RESULT_INVALID_ARGUMENT;
  mx_handle_t mx_handle1;
  mx_handle_t mx_handle0 = mx_message_pipe_create(&mx_handle1);
  if (mx_handle0 < 0)
    return StatusToMojoResult(mx_handle0);
  *message_pipe_handle0 = mx_handle0;
  *message_pipe_handle1 = mx_handle1;
  return MOJO_RESULT_OK;
}

MojoResult MojoWriteMessage(MojoHandle message_pipe_handle,
                            const void* bytes,
                            uint32_t num_bytes,
                            const MojoHandle* handles,
                            uint32_t num_handles,
                            MojoWriteMessageFlags flags) {
  mx_handle_t* mx_handles = (mx_handle_t*) handles;
  // TODO(abarth): Handle messages that are too big to fit.
  return StatusToMojoResult(mx_message_write(message_pipe_handle,
                                             bytes,
                                             num_bytes,
                                             mx_handles,
                                             num_handles,
                                             flags));
}

MojoResult MojoReadMessage(MojoHandle message_pipe_handle,
                           void* bytes,
                           uint32_t* num_bytes,
                           MojoHandle* handles,
                           uint32_t* num_handles,
                           MojoReadMessageFlags flags) {
  mx_handle_t* mx_handles = (mx_handle_t*) handles;
  // TODO(abarth): Handle messages that were too big to fit.
  return StatusToMojoResult(mx_message_read(message_pipe_handle,
                                            bytes,
                                            num_bytes,
                                            mx_handles,
                                            num_handles,
                                            flags));
}

// data_pipe.h -----------------------------------------------------------------

// TODO(abarth): Not implemented.

// buffer.h --------------------------------------------------------------------

// TODO(abarth): Not implemented.

// wait_set.h ------------------------------------------------------------------

// TODO(abarth): Not implemented.

int main(int argc, char** argv) {
  return MojoMain(mxr_process_get_handle(MX_HND_TYPE_MOJO_SHELL));
}
