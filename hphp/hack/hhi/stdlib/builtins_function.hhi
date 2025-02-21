<?hh
/**
 * Copyright (c) 2014, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
 */

namespace {

<<__PHPStdLib>>
function get_defined_functions(): array<string, array<string>>;
<<__PHPStdLib, __Rx>>
function function_exists(string $function_name, bool $autoload = true): bool;
<<__PHPStdLib, __Rx>>
/* HH_FIXME[2088] byref arg is not Rx */
function is_callable($v, bool $syntax = false, &$name = null): bool;
<<__PHPStdLib, __Rx>>
/* HH_FIXME[2088] byref arg is not Rx */
function is_callable_with_name($v, bool $syntax, &$name): bool;
function call_user_func_array($function, Container<mixed> $params);
function call_user_func($function, ...$args);
<<__PHPStdLib>>
function register_postsend_function($function, ...$args);
<<__PHPStdLib>>
function register_shutdown_function($function, ...$args);
}

namespace HH {
<<__Rx>>
function fun_get_function(mixed $fun): string;
}
