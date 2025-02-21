// Copyright (c) 2019, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the "hack" directory of this source tree.

#[macro_use]
extern crate ocaml;

use facts_rust as facts;
use oxidized::relative_path::RelativePath;

use facts::facts_parser::*;

caml!(extract_as_json_ffi(
    php5_compat_mode,
    hhvm_compat_mode,
    allow_new_attribute_syntax,
    filename,
    text
) {
    use ocaml::ToValue;
    return extract_as_json_ffi0(
        php5_compat_mode.i32_val() != 0,
        hhvm_compat_mode.i32_val() != 0,
        allow_new_attribute_syntax.i32_val() != 0,
        RelativePath::from_ocamlvalue(&filename),
        ocaml::Str::from(text).as_str(),
    ).to_value();
});

fn extract_as_json_ffi0(
    php5_compat_mode: bool,
    hhvm_compat_mode: bool,
    allow_new_attribute_syntax: bool,
    filename: RelativePath,
    text: &str,
) -> String {
    let opts = ExtractAsJsonOpts {
        php5_compat_mode,
        hhvm_compat_mode,
        allow_new_attribute_syntax,
        filename,
    };
    // return empty string in case of failure (ambiguous because "" is not a valid JSON)
    // as ocaml-rs doesn't offer conversion from/to option (caller will use None for failure)
    extract_as_json(text, opts).unwrap_or(String::from(""))
}
