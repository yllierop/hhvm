(*
 * Copyright (c) 2018, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
*)

open Core_kernel
open Aast
open Typing_defs

module Env = Tast_env
module TCO = TypecheckerOptions
module SN = Naming_special_names

(** Produce an error on (string) casts of objects. Currently it is allowed in HHVM to
    cast an object if it is Stringish (i.e., has a __toString() method), but all
    (string) casts of objects will be banned in the future. Eventually,
    __toString/(string) casts of objects will be removed from HHVM entirely. *)

let check__toString m =
  let (pos, name) = m.m_name in
  if name = SN.Members.__toString
  then begin
    if m.m_visibility <> Public || m.m_static
    then Errors.toString_visibility pos;
    match hint_of_type_hint m.m_ret with
    | Some (_, Hprim Tstring) -> ()
    | Some (p, _) -> Errors.toString_returns_string p
    | None -> ()
  end

let rec is_stringish env ty =
  let env, ety = Env.expand_type env ty in
  match snd ety with
  | Toption ty' -> is_stringish env ty'
  | Tunion tyl -> List.for_all ~f:(is_stringish env) tyl
  | Tintersection tyl -> List.exists ~f:(is_stringish env) tyl
  | Tabstract _ ->
    let env, tyl = Env.get_concrete_supertypes env ty in
    List.for_all ~f:(is_stringish env) tyl
  | Tclass (x, _, _) ->
    Option.is_none (Env.get_class env (snd x))
  | Tany _ | Terr | Tdynamic | Tobject | Tnonnull | Tprim _ ->
    true
  | Tarraykind _ | Tvar _ | Ttuple _ | Tanon (_, _) | Tfun _ | Tshape _ | Tdestructure _ ->
    false

let handler = object
  inherit Tast_visitor.handler_base

  method! at_expr env ((p, _), expr) =
    match expr with
    | Cast ((_, Hprim Tstring), te) ->
      let ((_, ty), _) = te in
      if not (is_stringish env ty)
      then Errors.string_cast p (Env.print_ty env ty)
    | _ -> ()

  method! at_method_ _ m = check__toString m
end
