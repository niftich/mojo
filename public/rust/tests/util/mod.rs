// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! This module contains useful functions and macros for testing.

/// This macro sets up tests by adding in Mojo embedder
/// initialization.
macro_rules! tests {
    ( $(fn $i:ident() $b:block)* ) => {
        use std::sync::{Once, ONCE_INIT};
        static START: Once = ONCE_INIT;
        $(
            #[test]
            fn $i() {
                START.call_once(|| unsafe {
                    util::InitializeMojoEmbedder();
                });
                $b
            }
        )*
    }
}

#[link(name = "stdc++")]
extern "C" {}

#[link(name = "rust_embedder")]
extern "C" {
    pub fn InitializeMojoEmbedder();
}
