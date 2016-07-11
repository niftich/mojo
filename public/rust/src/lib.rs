// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#[macro_use]
pub mod system;

pub use system::MojoResult;

#[macro_export]
/// This macro must be used at the top-level in any
/// Rust Mojo application. It defines and abstracts away the
/// hook needed by Mojo in order to set up the basic
/// functionality (see mojo::system::ffi). It must take the
/// name of a function who returns a MojoResult and takes
/// exactly one argument: a mojo::handle::Handle, or on in
/// other words, an untyped handle.
macro_rules! set_mojo_main {
    ( $i:ident ) => {
        #[allow(bad_style)]
        #[no_mangle]
        pub fn MojoMain(app_request_handle: mojo::system::MojoHandle) -> mojo::MojoResult
        {
            use std::panic;
            use mojo::system::CastHandle;
            let handle = unsafe {
                mojo::system::message_pipe::MessageEndpoint::from_untyped(
                    mojo::system::acquire(app_request_handle)
                )
            };
            let result = panic::catch_unwind(|| {
                $i(handle)
            });
            match result {
                Ok(value) => value,
                Err(_) => {
                    mojo::MojoResult::Aborted
                },
            }
        }
    }
}

#[macro_export]
/// This macro assists in generating flags for
/// functions and methods found in mojo::system::message_pipe.
///
/// See mojo::system::message_pipe for the available flags
/// that may be passed.
///
/// # Examples
///
/// # mpflags!(Create::None);
/// # mpflags!(Read::MayDiscard);
macro_rules! mpflags {
    ( $( $t:path ),* ) => {{
        use mojo::system::message_pipe::*;
        $(
            ($t as u32)
        )|*
    }}
}

#[macro_export]
/// This macro assists in generating flags for
/// functions and methods found in mojo::system::data_pipe.
///
/// See mojo::system::data_pipe for the available flags
/// that may be passed.
///
/// # Examples
///
/// # dpflags!(Create::None);
/// # dpflags!(Read::AllOrNone, Read::Discard);
macro_rules! dpflags {
    ( $( $t:path ),* ) => {{
        use mojo::system::data_pipe::*;
        $(
            ($t as u32)
        )|*
    }}
}

#[macro_export]
/// This macro assists in generating flags for
/// functions and methods found in mojo::system::shared_buffer.
///
/// See mojo::system::shared_buffer for the available flags
/// that may be passed.
///
/// # Examples
///
/// # sbflags!(Create::None);
/// # sbflags!(Map::None);
macro_rules! sbflags {
    ( $( $t:path ),* ) => {{
        use mojo::system::shared_buffer::*;
        $(
            ($t as u32)
        )|*
    }}
}

#[macro_export]
/// This macro assists in generating MojoSignals to be
/// used in wait() and wait_many(), part of mojo::system::core.
///
/// See mojo::system::handle for the available signals
/// that may be checked for by wait() and wait_many().
///
/// # Examples
///
/// # signals!(Signals::Readable, Signals::Writable);
/// # signals!(Signals::PeerClosed);
macro_rules! signals {
    ( $( $t:path ),* ) => {{
        use mojo::system::Signals;
        mojo::system::HandleSignals::new(
        $(
            ($t as u32)
        )|*
        )
    }}
}
