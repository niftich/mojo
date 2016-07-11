// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the C++ wrappers in mojo/public/cpp/system/handle.h.

#include "mojo/public/cpp/system/handle.h"

#include <map>
#include <utility>

#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

// Basic |Handle| tests.
TEST(HandleTest, Handle) {
  EXPECT_EQ(MOJO_HANDLE_INVALID, kInvalidHandleValue);

  Handle h0;
  EXPECT_EQ(kInvalidHandleValue, h0.value());
  EXPECT_EQ(kInvalidHandleValue, *h0.mutable_value());
  EXPECT_FALSE(h0.is_valid());

  Handle h1(static_cast<MojoHandle>(123));
  EXPECT_EQ(static_cast<MojoHandle>(123), h1.value());
  EXPECT_EQ(static_cast<MojoHandle>(123), *h1.mutable_value());
  EXPECT_TRUE(h1.is_valid());
  *h1.mutable_value() = static_cast<MojoHandle>(456);
  EXPECT_EQ(static_cast<MojoHandle>(456), h1.value());
  EXPECT_TRUE(h1.is_valid());

  h1.swap(h0);
  EXPECT_EQ(static_cast<MojoHandle>(456), h0.value());
  EXPECT_TRUE(h0.is_valid());
  EXPECT_FALSE(h1.is_valid());

  h1.set_value(static_cast<MojoHandle>(789));
  h0.swap(h1);
  EXPECT_EQ(static_cast<MojoHandle>(789), h0.value());
  EXPECT_TRUE(h0.is_valid());
  EXPECT_EQ(static_cast<MojoHandle>(456), h1.value());
  EXPECT_TRUE(h1.is_valid());

  // Make sure copy constructor works.
  Handle h2(h0);
  EXPECT_EQ(static_cast<MojoHandle>(789), h2.value());
  // And assignment.
  h2 = h1;
  EXPECT_EQ(static_cast<MojoHandle>(456), h2.value());

  // Make sure that we can put |Handle|s into |std::map|s.
  h0 = Handle(static_cast<MojoHandle>(987));
  h1 = Handle(static_cast<MojoHandle>(654));
  h2 = Handle(static_cast<MojoHandle>(321));
  Handle h3;
  std::map<Handle, int> handle_to_int;
  handle_to_int[h0] = 0;
  handle_to_int[h1] = 1;
  handle_to_int[h2] = 2;
  handle_to_int[h3] = 3;

  EXPECT_EQ(4u, handle_to_int.size());
  EXPECT_FALSE(handle_to_int.find(h0) == handle_to_int.end());
  EXPECT_EQ(0, handle_to_int[h0]);
  EXPECT_FALSE(handle_to_int.find(h1) == handle_to_int.end());
  EXPECT_EQ(1, handle_to_int[h1]);
  EXPECT_FALSE(handle_to_int.find(h2) == handle_to_int.end());
  EXPECT_EQ(2, handle_to_int[h2]);
  EXPECT_FALSE(handle_to_int.find(h3) == handle_to_int.end());
  EXPECT_EQ(3, handle_to_int[h3]);
  EXPECT_TRUE(handle_to_int.find(Handle(static_cast<MojoHandle>(13579))) ==
              handle_to_int.end());

  // TODO(vtl): With C++11, support |std::unordered_map|s, etc.
}

// Basic |ScopedHandle| tests.
TEST(HandleTest, ScopedHandle) {
  // Invalid |ScopedHandle|:
  {
    ScopedHandle sh0;

    EXPECT_EQ(kInvalidHandleValue, sh0.get().value());
    EXPECT_FALSE(sh0.is_valid());

    // This should be a no-op.
    Close(sh0.Pass());

    // It should still be invalid.
    EXPECT_FALSE(sh0.is_valid());

    // Move constructible:
    ScopedHandle sh1(std::move(sh0));
    EXPECT_FALSE(sh0.is_valid());
    EXPECT_FALSE(sh1.is_valid());

    // Move assignable:
    sh0 = std::move(sh1);
    EXPECT_FALSE(sh0.is_valid());
    EXPECT_FALSE(sh1.is_valid());
  }

  // "Valid" |ScopedHandle| (but note that we can't test that it closes the
  // handle on leaving scope without having a valid handle!):
  {
    Handle h0(static_cast<MojoHandle>(123));
    ScopedHandle sh0(h0);

    EXPECT_EQ(h0.value(), sh0.get().value());
    EXPECT_TRUE(sh0.is_valid());

    // Move constructible:
    ScopedHandle sh1(std::move(sh0));
    EXPECT_FALSE(sh0.is_valid());
    EXPECT_TRUE(sh1.is_valid());

    // Move assignable:
    sh0 = std::move(sh1);
    EXPECT_TRUE(sh0.is_valid());
    EXPECT_FALSE(sh1.is_valid());

    // We have to release |sh0|, since it's not really valid.
    Handle h1 = sh0.release();
    EXPECT_EQ(h0.value(), h1.value());
  }
}

TEST(HandleTest, MakeScopedHandle) {
  EXPECT_FALSE(MakeScopedHandle(Handle()).is_valid());

  Handle h(static_cast<MojoHandle>(123));
  auto sh = MakeScopedHandle(h);
  EXPECT_TRUE(sh.is_valid());
  EXPECT_EQ(h.value(), sh.get().value());
  // Have to release |sh0|, since it's not really valid.
  ignore_result(sh.release());
}

TEST(HandleTest, ScopedHandleMoveCtor) {
  // We'll use a shared buffer handle (since we need a valid handle) in a
  // |ScopedSharedBufferHandle|.
  ScopedSharedBufferHandle buffer1;
  EXPECT_EQ(MOJO_RESULT_OK, CreateSharedBuffer(nullptr, 1024, &buffer1));
  EXPECT_TRUE(buffer1.is_valid());

  ScopedSharedBufferHandle buffer2;
  EXPECT_EQ(MOJO_RESULT_OK, CreateSharedBuffer(nullptr, 1024, &buffer2));
  EXPECT_TRUE(buffer2.is_valid());

  // If this fails to close buffer1, ScopedHandleBase::CloseIfNecessary() will
  // assert.
  buffer1 = buffer2.Pass();

  EXPECT_TRUE(buffer1.is_valid());
  EXPECT_FALSE(buffer2.is_valid());
}

TEST(HandleTest, ScopedHandleMoveCtorSelf) {
  // We'll use a shared buffer handle (since we need a valid handle) in a
  // |ScopedSharedBufferHandle|.
  ScopedSharedBufferHandle buffer;
  EXPECT_EQ(MOJO_RESULT_OK, CreateSharedBuffer(nullptr, 1024, &buffer));
  EXPECT_TRUE(buffer.is_valid());

  buffer = buffer.Pass();

  EXPECT_TRUE(buffer.is_valid());
}

TEST(HandleTest, RightsReplaceAndDuplicate) {
  static constexpr auto kDuplicate = MOJO_HANDLE_RIGHT_DUPLICATE;
  static constexpr auto kTransfer = MOJO_HANDLE_RIGHT_TRANSFER;
  static constexpr auto kGetOptions = MOJO_HANDLE_RIGHT_GET_OPTIONS;

  // We'll use a shared buffer handle (since we need a valid handle that's
  // duplicatable) in a |ScopedSharedBufferHandle|.
  ScopedSharedBufferHandle buffer1;
  EXPECT_EQ(MOJO_RESULT_OK, CreateSharedBuffer(nullptr, 1024, &buffer1));
  EXPECT_TRUE(buffer1.is_valid());

  EXPECT_EQ(kDuplicate | kTransfer | kGetOptions,
            GetRights(buffer1.get()) & (kDuplicate | kTransfer | kGetOptions));

  MojoHandle old_handle_value = buffer1.get().value();
  EXPECT_TRUE(ReplaceHandleWithReducedRights(&buffer1, kTransfer));
  EXPECT_TRUE(buffer1.is_valid());
  EXPECT_NE(buffer1.get().value(), old_handle_value);
  EXPECT_EQ(kDuplicate | kGetOptions,
            GetRights(buffer1.get()) & (kDuplicate | kTransfer | kGetOptions));

  ScopedSharedBufferHandle buffer2 =
      DuplicateHandleWithReducedRights(buffer1.get(), kGetOptions);
  EXPECT_TRUE(buffer2.is_valid());
  EXPECT_NE(buffer2.get().value(), buffer1.get().value());
  EXPECT_EQ(kDuplicate,
            GetRights(buffer2.get()) & (kDuplicate | kTransfer | kGetOptions));
  EXPECT_EQ(kDuplicate | kGetOptions,
            GetRights(buffer1.get()) & (kDuplicate | kTransfer | kGetOptions));

  ScopedSharedBufferHandle buffer3 = DuplicateHandle(buffer2.get());
  EXPECT_TRUE(buffer3.is_valid());
  EXPECT_EQ(kDuplicate,
            GetRights(buffer3.get()) & (kDuplicate | kTransfer | kGetOptions));
  EXPECT_EQ(kDuplicate,
            GetRights(buffer2.get()) & (kDuplicate | kTransfer | kGetOptions));
}

// TODO(vtl): Test |CloseRaw()|.
// TODO(vtl): Test |reset()| more thoroughly?

}  // namespace mojo
}  // namespace
