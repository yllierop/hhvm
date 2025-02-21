(*
 * Copyright (c) 2018, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
 *)

open Aast
open Typing_defs

let handler = object
  inherit Tast_visitor.handler_base

  method! at_expr env = function
    | _, Obj_get (((_, ty),_) , _, _)
      when
        Tast_env.is_sub_type_for_union env ty (Reason.none, Tdynamic) ||
        Tast_env.is_sub_type_for_union env ty (Reason.none, Typing_defs.make_tany ()) ||
        Tast_env.is_sub_type_for_union env ty (Reason.none, Terr) -> ()
    | _, Obj_get (_, ((p, _), Lvar _) , _) -> Errors.lvar_in_obj_get p
    | _ -> ()
  end
