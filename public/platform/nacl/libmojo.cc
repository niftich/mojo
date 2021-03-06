// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <mojo/result.h>
#include <mojo/system/buffer.h>
#include <mojo/system/data_pipe.h>
#include <mojo/system/handle.h>
#include <mojo/system/message_pipe.h>
#include <mojo/system/time.h>
#include <mojo/system/wait.h>
#include <mojo/system/wait_set.h>
#include <stdlib.h>

#include "mojo/public/platform/nacl/mojo_irt.h"
#include "native_client/src/untrusted/irt/irt.h"

bool g_irt_mojo_valid = false;
struct nacl_irt_mojo g_irt_mojo;

struct nacl_irt_mojo* get_irt_mojo() {
  if (!g_irt_mojo_valid) {
    size_t rc = nacl_interface_query(NACL_IRT_MOJO_v0_1, &g_irt_mojo,
                                     sizeof(g_irt_mojo));
    if (rc != sizeof(g_irt_mojo))
      return NULL;
    else
      g_irt_mojo_valid = true;
  }
  return &g_irt_mojo;
}

MojoResult _MojoGetInitialHandle(MojoHandle* handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->_MojoGetInitialHandle(handle);
}

MojoTimeTicks MojoGetTimeTicksNow() {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoGetTimeTicksNow();
}

MojoResult MojoClose(MojoHandle handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoClose(handle);
}

MojoResult MojoGetRights(MojoHandle handle, MojoHandleRights* rights) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoGetRights(handle, rights);
}

MojoResult MojoReplaceHandleWithReducedRights(MojoHandle handle,
                                              MojoHandleRights rights_to_remove,
                                              MojoHandle* replacement_handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoReplaceHandleWithReducedRights(handle, rights_to_remove,
                                                      replacement_handle);
}

MojoResult MojoDuplicateHandleWithReducedRights(
    MojoHandle handle,
    MojoHandleRights rights_to_remove,
    MojoHandle* new_handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoDuplicateHandleWithReducedRights(
      handle, rights_to_remove, new_handle);
}

MojoResult MojoDuplicateHandle(MojoHandle handle, MojoHandle* new_handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoDuplicateHandle(handle, new_handle);
}

MojoResult MojoWait(MojoHandle handle,
                    MojoHandleSignals signals,
                    MojoDeadline deadline,
                    struct MojoHandleSignalsState* signals_state) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoWait(handle, signals, deadline, signals_state);
}

MojoResult MojoWaitMany(const MojoHandle* handles,
                        const MojoHandleSignals* signals,
                        uint32_t num_handles,
                        MojoDeadline deadline,
                        uint32_t* result_index,
                        struct MojoHandleSignalsState* signals_states) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoWaitMany(handles, signals, num_handles, deadline,
                                result_index, signals_states);
}

MojoResult MojoCreateMessagePipe(
    const struct MojoCreateMessagePipeOptions* options,
    MojoHandle* message_pipe_handle0,
    MojoHandle* message_pipe_handle1) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoCreateMessagePipe(options, message_pipe_handle0,
                                         message_pipe_handle1);
}

MojoResult MojoWriteMessage(MojoHandle message_pipe_handle,
                            const void* bytes,
                            uint32_t num_bytes,
                            const MojoHandle* handles,
                            uint32_t num_handles,
                            MojoWriteMessageFlags flags) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoWriteMessage(message_pipe_handle, bytes, num_bytes,
                                    handles, num_handles, flags);
}

MojoResult MojoReadMessage(MojoHandle message_pipe_handle,
                           void* bytes,
                           uint32_t* num_bytes,
                           MojoHandle* handles,
                           uint32_t* num_handles,
                           MojoReadMessageFlags flags) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoReadMessage(message_pipe_handle, bytes, num_bytes,
                                   handles, num_handles, flags);
}

MojoResult MojoCreateDataPipe(const struct MojoCreateDataPipeOptions* options,
                              MojoHandle* data_pipe_producer_handle,
                              MojoHandle* data_pipe_consumer_handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoCreateDataPipe(options, data_pipe_producer_handle,
                                      data_pipe_consumer_handle);
}

MojoResult MojoSetDataPipeProducerOptions(
    MojoHandle data_pipe_producer_handle,
    const struct MojoDataPipeProducerOptions* options) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoSetDataPipeProducerOptions(data_pipe_producer_handle,
                                                  options);
}

MojoResult MojoGetDataPipeProducerOptions(
    MojoHandle data_pipe_producer_handle,
    struct MojoDataPipeProducerOptions* options,
    uint32_t options_num_bytes) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoGetDataPipeProducerOptions(data_pipe_producer_handle,
                                                  options, options_num_bytes);
}

MojoResult MojoWriteData(MojoHandle data_pipe_producer_handle,
                         const void* elements,
                         uint32_t* num_bytes,
                         MojoWriteDataFlags flags) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoWriteData(data_pipe_producer_handle, elements, num_bytes,
                                 flags);
}

MojoResult MojoBeginWriteData(MojoHandle data_pipe_producer_handle,
                              void** buffer,
                              uint32_t* buffer_num_bytes,
                              MojoWriteDataFlags flags) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoBeginWriteData(data_pipe_producer_handle, buffer,
                                      buffer_num_bytes, flags);
}

MojoResult MojoEndWriteData(MojoHandle data_pipe_producer_handle,
                            uint32_t num_bytes_written) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoEndWriteData(data_pipe_producer_handle,
                                    num_bytes_written);
}

MojoResult MojoSetDataPipeConsumerOptions(
    MojoHandle data_pipe_consumer_handle,
    const struct MojoDataPipeConsumerOptions* options) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoSetDataPipeConsumerOptions(data_pipe_consumer_handle,
                                                  options);
}

MojoResult MojoGetDataPipeConsumerOptions(
    MojoHandle data_pipe_consumer_handle,
    struct MojoDataPipeConsumerOptions* options,
    uint32_t options_num_bytes) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoGetDataPipeConsumerOptions(data_pipe_consumer_handle,
                                                  options, options_num_bytes);
}

MojoResult MojoReadData(MojoHandle data_pipe_consumer_handle,
                        void* elements,
                        uint32_t* num_bytes,
                        MojoReadDataFlags flags) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoReadData(data_pipe_consumer_handle, elements, num_bytes,
                                flags);
}

MojoResult MojoBeginReadData(MojoHandle data_pipe_consumer_handle,
                             const void** buffer,
                             uint32_t* buffer_num_bytes,
                             MojoReadDataFlags flags) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoBeginReadData(data_pipe_consumer_handle, buffer,
                                     buffer_num_bytes, flags);
}

MojoResult MojoEndReadData(MojoHandle data_pipe_consumer_handle,
                           uint32_t num_bytes_read) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoEndReadData(data_pipe_consumer_handle, num_bytes_read);
}

MojoResult MojoCreateSharedBuffer(
    const struct MojoCreateSharedBufferOptions* options,
    uint64_t num_bytes,
    MojoHandle* shared_buffer_handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoCreateSharedBuffer(options, num_bytes,
                                          shared_buffer_handle);
}

MojoResult MojoDuplicateBufferHandle(
    MojoHandle buffer_handle,
    const struct MojoDuplicateBufferHandleOptions* options,
    MojoHandle* new_buffer_handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoDuplicateBufferHandle(buffer_handle, options,
                                             new_buffer_handle);
}

MojoResult MojoGetBufferInformation(MojoHandle buffer_handle,
                                    struct MojoBufferInformation* info,
                                    uint32_t info_num_bytes) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoGetBufferInformation(buffer_handle, info,
                                            info_num_bytes);
}

MojoResult MojoMapBuffer(MojoHandle buffer_handle,
                         uint64_t offset,
                         uint64_t num_bytes,
                         void** buffer,
                         MojoMapBufferFlags flags) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoMapBuffer(buffer_handle, offset, num_bytes, buffer,
                                 flags);
}

MojoResult MojoUnmapBuffer(void* buffer) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoUnmapBuffer(buffer);
}

MojoResult MojoCreateWaitSet(const struct MojoCreateWaitSetOptions* options,
                             MojoHandle* handle) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoCreateWaitSet(options, handle);
}

MojoResult MojoWaitSetAdd(MojoHandle wait_set_handle,
                          MojoHandle handle,
                          MojoHandleSignals signals,
                          uint64_t cookie,
                          const struct MojoWaitSetAddOptions* options) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoWaitSetAdd(wait_set_handle, handle, signals, cookie,
                                  options);
}

MojoResult MojoWaitSetRemove(MojoHandle wait_set_handle, uint64_t cookie) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoWaitSetRemove(wait_set_handle, cookie);
}

MojoResult MojoWaitSetWait(MojoHandle wait_set_handle,
                           MojoDeadline deadline,
                           uint32_t* num_results,
                           struct MojoWaitSetResult* results,
                           uint32_t* max_results) {
  struct nacl_irt_mojo* irt_mojo = get_irt_mojo();
  if (!irt_mojo)
    abort();
  return irt_mojo->MojoWaitSetWait(wait_set_handle, deadline, num_results,
                                   results, max_results);
}
