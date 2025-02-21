(*
 * Copyright (c) 2017, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
 *)

open Core_kernel
open Typing_defs

module Cls = Decl_provider.Class
module MakeType = Typing_make_type
module Env = Typing_env

let wrap_like ty =
  let r = Typing_reason.Renforceable (Typing_reason.to_pos (fst ty)) in
  MakeType.like r ty

let rec pessimize_type env ?(trust_awaitable=false) (ty: decl ty) =
  if not (TypecheckerOptions.pessimize_types (Env.get_tcopt env)) then ty else
  match ty with
  | _, Terr
  | _, Tany _
  | _, Tnonnull
  | _, Tprim _
  | _, Tdynamic
  | _, Tmixed
  | _, Tnothing ->
    ty
  | _, Tthis ->
    wrap_like ty
  | r, Tarray (ty1, ty2) ->
    let ty1 = Option.map ~f:(pessimize_wrap env) ty1 in
    let ty2 = Option.map ~f:(pessimize_wrap env) ty2 in
    r, Tarray (ty1, ty2)
  | r, Tdarray (tk, tv) ->
    r, Tdarray (pessimize_wrap env tk, pessimize_wrap env tv)
  | r, Tvarray tv ->
    r, Tvarray (pessimize_wrap env tv)
  | r, Tvarray_or_darray tv ->
    r, Tvarray_or_darray (pessimize_wrap env tv)
  | _, Tgeneric x ->
    if Env.get_reified env x = Aast.Reified
    then
      if Env.get_enforceable env x
      then ty
      else wrap_like ty
    else
      ty
  | r, Toption ty ->
    r, Toption (pessimize_type env ty)
  | _, Tlike ty ->
    pessimize_wrap env ty
  | r, Tfun ft ->
    wrap_like (r, Tfun (pessimize_fun_type env ft))
  | r, Tapply ((p, x), argl) when Env.is_typedef x || Env.is_enum env x ->
    (* Enums don't have type arguments so the next line is a no-op for them *)
    let argl = List.map ~f:(pessimize_wrap env) argl in
    wrap_like (r, Tapply ((p, x), argl))
  | r, Tapply ((p, cid), targs) when cid = Naming_special_names.Classes.cAwaitable ->
    let f =
      if trust_awaitable
      then pessimize_type env ~trust_awaitable:false
      else pessimize_wrap env in
    r, Tapply ((p, cid), List.map ~f targs)
  | r, Tapply ((p, cid), targs) ->
    let targs = match Env.get_class env cid with
    | Some cls ->
      pessimize_targs env targs (Cls.tparams cls)
    | None ->
      targs in
    r, Tapply ((p, cid), targs)
  | r, Ttuple tyl ->
    let tyl = List.map ~f:(pessimize_wrap env) tyl in
    wrap_like (r, Ttuple tyl)
  | _, Taccess _ ->
    wrap_like ty
  | r, Tshape (shape_kind, fields_map) ->
    let fields_map = Nast.ShapeMap.map (fun shape_field_ty ->
      let { sft_ty; _ } = shape_field_ty in
      let sft_ty = pessimize_wrap env sft_ty in
      { shape_field_ty with sft_ty }
    ) fields_map in
    wrap_like (r, Tshape (shape_kind, fields_map))

and pessimize_targs env targs tparams =
  if not (TypecheckerOptions.pessimize_types (Env.get_tcopt env)) then targs else
  let open List in
  let new_targs = map2 targs tparams ~f:(fun targ tparam ->
    (* Trust reified type arguments *)
    if tparam.tp_reified = Aast.Reified
    then targ
    else pessimize_wrap env targ
  ) in
  match new_targs with
  | Or_unequal_lengths.Ok new_targs ->
    new_targs
  | Or_unequal_lengths.Unequal_lengths ->
    targs

and pessimize_wrap env ty =
  let ty = pessimize_type env ty in
  match ty with
  | _, Terr
  | _, Tany _ -> ty (* like Tany is useless *)
  | _, Tlike _ -> ty
  | _, Tgeneric x when Env.get_reified env x <> Aast.Reified -> ty
  | _, Tapply ((_, x), []) when x = Naming_special_names.Typehints.wildcard -> ty
  | _ -> wrap_like ty

(* For erased generics with constraints, add super dynamic and make the as constraints like types *)
and pessimize_tparam_constraints env (t: decl tparam) =
  if not (TypecheckerOptions.pessimize_types (Env.get_tcopt env)) then t else
  match t.tp_reified with
  | Aast.Reified -> t
  | _ ->
    let tp_constraints = List.map t.tp_constraints ~f:(fun (ck, cstr_ty) ->
      match ck with
      | Ast_defs.Constraint_as | Ast_defs.Constraint_eq ->
        ck, pessimize_wrap env cstr_ty
      | _ ->
        ck, cstr_ty
    ) in
    let dyn = MakeType.dynamic (Reason.Renforceable (fst t.tp_name)) in
    let tp_constraints =
      (Ast_defs.Constraint_super, dyn) :: tp_constraints in
    { t with tp_constraints }

and pessimize_fun_type env (ft: decl fun_type) =
  (* TODO: It may be necessary to pessimize ft_arity and ft_where_constraints *)
  let {
    ft_params;
    ft_ret;
    ft_fun_kind;
    ft_tparams;
    _
  } = ft in
  (* The runtime will enforce the inner type of an Awaitable in the return of an
   * async function *)
  let trust_awaitable = match ft_fun_kind with
  | Ast_defs.FAsync | Ast_defs.FAsyncGenerator -> true
  | _ -> false in
  let ft_ret_type = pessimize_type env ~trust_awaitable ft_ret.et_type in
  let ft_params = List.map ft_params ~f:(fun param ->
    let ty = pessimize_type env param.fp_type.et_type in
    { param with fp_type = { param.fp_type with et_type = ty } }
  ) in
  let ft_tparams = Tuple.T2.map_fst ft_tparams
    ~f:(List.map ~f:(pessimize_tparam_constraints env)) in
  { ft with
    ft_params;
    ft_ret = { ft_ret with et_type = ft_ret_type };
    ft_tparams;
  }

let rec is_enforceable (env: Env.env) (ty: decl ty) =
  match snd ty with
  | Tthis -> false
  | Tapply ((_, name), _) when Env.is_enum env name -> false
  | Tapply ((_, name), _) when Env.is_typedef name ->
    begin match Env.get_typedef env name with
    | Some { td_vis = Aast.Transparent; td_tparams; td_type; _ } ->
      (* So that the check does not collide with reified generics *)
      let env = Env.add_generic_parameters env td_tparams in
      is_enforceable env td_type
    | _ -> false end
  | Tapply ((_, name), tyl) ->
    begin match Env.get_class env name with
    | Some tc ->
      let tparams = Cls.tparams tc in
      begin match tyl with
      | [] -> true
      | targs ->
        let open List.Or_unequal_lengths in
        begin match List.fold2 ~init:true targs tparams ~f:(fun acc targ tparam ->
          match targ with
          | _, Tdynamic (* We accept the inner type being dynamic regardless of reification *)
          | _, Tlike _ ->
            acc
          | _ ->
            match tparam.tp_reified with
            | Aast.Erased -> false
            | Aast.SoftReified -> false
            | Aast.Reified -> is_enforceable env targ && acc
        ) with
        | Ok new_acc -> new_acc
        | Unequal_lengths -> true
        end
      end
    | None -> true
    end
  | Tgeneric name ->
    begin match Env.get_reified env name, Env.get_enforceable env name with
    | Aast.Erased, _ -> false
    | Aast.SoftReified, _ -> false
    | Aast.Reified, false -> false
    | Aast.Reified, true ->
      true
    end
  | Taccess _ -> false
  | Tlike _ -> false
  | Tarray (None, None) -> true
  | Tarray _ -> false
  | Tprim prim ->
    begin match prim with
      | Aast.Tvoid
      | Aast.Tnoreturn -> false
      | _ -> true
    end
  | Tany _ -> true
  | Terr -> true
  | Tnonnull -> true
  | Tdynamic -> true
  | Tfun _ -> false
  | Ttuple _ -> false
  | Tshape _ -> false
  | Tmixed -> true
  | Tnothing -> true
  | Tdarray _ -> false
  | Tvarray _ -> false
  (* With no parameters, we enforce varray_or_darray just like array *)
  | Tvarray_or_darray (_, Tany _) -> true
  | Tvarray_or_darray _ -> false
  | Toption ty ->
    is_enforceable env ty

let is_enforced env ty =
  let enforceable = is_enforceable env ty in
  let is_hhi =
    fst ty |>
    Reason.to_pos |>
    Pos.filename |>
    Relative_path.prefix |>
    (=) Relative_path.Hhi in
  enforceable && not is_hhi

let pessimize_type_simple env (ty: decl ty) =
  if not env.Env.pessimize then ty else
  match ty with
  | _, Tprim (Aast.Tvoid | Aast.Tnoreturn) -> ty
  | _ -> wrap_like ty

let compute_enforced_and_pessimize_ty_simple env (ty: decl ty) =
  let et_enforced = is_enforced env ty in
  let et_type =
    if not et_enforced
    then pessimize_type_simple env ty
    else ty in
  { et_type; et_enforced }

let compute_enforced_and_pessimize_fun_type_simple env (ft: decl fun_type) =
  let { ft_params; ft_ret = { et_type; _ }; _ } = ft in
  let ft_ret = compute_enforced_and_pessimize_ty_simple env et_type in
  let ft_params = List.map ~f:(fun fp ->
    let { fp_type = { et_type; _ }; _ } = fp in
    let fp_type = compute_enforced_and_pessimize_ty_simple env et_type in
    { fp with fp_type }
  ) ft_params in
  { ft with ft_params; ft_ret }
