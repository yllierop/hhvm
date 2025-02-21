(*
 * Copyright (c) 2015, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
 *)

open Core_kernel
open Typing_defs
module Reason = Typing_reason

class type ['a] type_visitor_type = object
  method on_tany : 'a -> Reason.t -> 'a
  method on_terr : 'a -> Reason.t -> 'a
  method on_tmixed : 'a -> Reason.t -> 'a
  method on_tnonnull : 'a -> Reason.t -> 'a
  method on_tdynamic : 'a -> Reason.t -> 'a
  method on_tnothing : 'a -> Reason.t -> 'a
  method on_tthis : 'a -> Reason.t -> 'a
  method on_tarray : 'a -> Reason.t -> 'b ty option -> 'b ty option -> 'a
  method on_tvarray_or_darray : 'a -> Reason.t -> decl ty -> 'a
  method on_tgeneric : 'a -> Reason.t -> string -> 'a
  method on_toption : 'a -> Reason.t -> 'b ty -> 'a
  method on_tlike : 'a -> Reason.t -> 'b ty -> 'a
  method on_tprim : 'a -> Reason.t -> Aast.tprim -> 'a
  method on_tvar : 'a -> Reason.t -> Ident.t -> 'a
  method on_type : 'a -> 'b ty -> 'a
  method on_tfun : 'a -> Reason.t -> 'b fun_type -> 'a
  method on_tabstract : 'a -> Reason.t -> abstract_kind -> locl ty option -> 'a
  method on_tapply : 'a -> Reason.t -> Aast.sid -> decl ty list -> 'a
  method on_ttuple : 'a -> Reason.t -> 'b ty list -> 'a
  method on_tanon : 'a -> Reason.t -> locl fun_arity -> Ident.t -> 'a
  method on_tunion : 'a -> Reason.t -> locl ty list -> 'a
  method on_tintersection : 'a -> Reason.t -> locl ty list -> 'a
  method on_tobject : 'a -> Reason.t -> 'a
  method on_tshape :
    'a
      -> Reason.t
      -> shape_kind
      -> 'b shape_field_type Nast.ShapeMap.t
      -> 'a
  method on_taccess : 'a -> Reason.t -> taccess_type -> 'a
  method on_tclass : 'a -> Reason.t -> Aast.sid -> exact -> locl ty list -> 'a
  method on_tarraykind : 'a -> Reason.t -> array_kind -> 'a
  method on_tlist : 'a -> Reason.t -> locl ty list -> 'a
end

class virtual ['a] type_visitor : ['a] type_visitor_type = object(this)
  method on_tany acc _ = acc
  method on_terr acc _ = acc
  method on_tmixed acc _ = acc
  method on_tnonnull acc _ = acc
  method on_tdynamic acc _ = acc
  method on_tnothing acc _ = acc
  method on_tthis acc _ = acc
  method on_tarray: type a. _ -> Reason.t -> a ty option -> a ty option -> _ =
    fun acc _ ty1_opt ty2_opt ->
    let acc = Option.fold ~f:this#on_type ~init:acc ty1_opt in
    let acc = Option.fold ~f:this#on_type ~init:acc ty2_opt in
    acc
  method on_tvarray_or_darray acc _ ty =
    this#on_type acc ty
  method on_tgeneric acc _ _ = acc
  method on_toption: type a. _ -> Reason.t -> a ty -> _ =
    fun acc _ ty -> this#on_type acc ty
  method on_tlike: type a. _ -> Reason.t -> a ty -> _ =
    fun acc _ ty -> this#on_type acc ty
  method on_tprim acc _ _ = acc
  method on_tvar acc _ _ = acc
  method on_tfun: type a. _ -> Reason.t -> a fun_type -> _ =
    fun acc _ {ft_params; ft_tparams; ft_ret; _} ->
    let acc = List.fold_left ~f:this#on_type ~init:acc
      (List.map ft_params (fun x -> x.fp_type.et_type)) in
    let tparams = List.map (fst ft_tparams) (fun t -> List.map t.tp_constraints snd) in
    let acc = List.fold_left tparams ~f:(fun acc tp ->
      List.fold ~f:this#on_type ~init:acc tp) ~init:acc in
    this#on_type acc ft_ret.et_type
  method on_tabstract acc _ ak ty_opt =
    let acc =
      match ak with
      | AKnewtype (_, tyl) -> List.fold_left tyl ~f:this#on_type ~init:acc
      | AKgeneric _ -> acc
      | AKdependent _ -> acc in
    let acc = Option.fold ~f:this#on_type ~init:acc ty_opt in
    acc
  method on_tapply acc _ _ tyl = List.fold_left tyl ~f:this#on_type ~init:acc
  method on_taccess acc _ (root, _ids) = this#on_type acc root
  method on_ttuple: type a. _ -> Reason.t -> a ty list -> _ =
    fun acc _ tyl -> List.fold_left tyl ~f:this#on_type ~init:acc
  method on_tanon acc _ _ _ = acc
  method on_tunion acc _ tyl = List.fold_left tyl ~f:this#on_type ~init:acc
  method on_tintersection acc _ tyl = List.fold_left tyl ~f:this#on_type ~init:acc
  method on_tobject acc _ = acc
  method on_tshape: type a. _ -> Reason.t -> shape_kind
    -> a shape_field_type Nast.ShapeMap.t -> _ =
    fun acc _ _ fdm ->
    let f _ { sft_ty; _ } acc = this#on_type acc sft_ty in
    Nast.ShapeMap.fold f fdm acc
  method on_tclass acc _ _ _ tyl =
    List.fold_left tyl ~f:this#on_type ~init:acc
  method on_tarraykind acc _ array_kind =
    match array_kind with
    | AKany -> acc
    | AKempty -> acc
    | AKvarray_or_darray ty
    | AKvarray ty
    | AKvec ty -> this#on_type acc ty
    | AKdarray (tk, tv)
    | AKmap (tk, tv) ->
      let acc = this#on_type acc tk in
      this#on_type acc tv
  method on_tlist acc _ tyl = List.fold_left tyl ~f:this#on_type ~init:acc
  method on_type: type a. _ -> a ty -> _ = fun acc (r, x) ->
    match x with
    | Tany _ -> this#on_tany acc r
    | Terr -> this#on_terr acc r
    | Tmixed -> this#on_tmixed acc r
    | Tnonnull -> this#on_tnonnull acc r
    | Tdynamic -> this#on_tdynamic acc r
    | Tnothing -> this#on_tnothing acc r
    | Tthis -> this#on_tthis acc r
    | Tarray (ty1_opt, ty2_opt) ->
      this#on_tarray acc r ty1_opt ty2_opt
    | Tdarray (ty1, ty2) ->
      this#on_type acc (r, Tarray (Some ty1, Some ty2))
    | Tvarray ty ->
      this#on_type acc (r, Tarray (Some ty, None))
    | Tvarray_or_darray ty ->
      this#on_tvarray_or_darray acc r ty
    | Tgeneric s -> this#on_tgeneric acc r s
    | Toption ty -> this#on_toption acc r ty
    | Tlike ty -> this#on_tlike acc r ty
    | Tprim prim -> this#on_tprim acc r prim
    | Tvar id -> this#on_tvar acc r id
    | Tfun fty -> this#on_tfun acc r fty
    | Tabstract (ak, ty_opt) -> this#on_tabstract acc r ak ty_opt
    | Tapply (s, tyl) -> this#on_tapply acc r s tyl
    | Taccess aty -> this#on_taccess acc r aty
    | Ttuple tyl -> this#on_ttuple acc r tyl
    | Tanon (arity, id) -> this#on_tanon acc r arity id
    | Tunion tyl -> this#on_tunion acc r tyl
    | Tintersection tyl -> this#on_tintersection acc r tyl
    | Tobject -> this#on_tobject acc r
    | Tshape (shape_kind, fdm) -> this#on_tshape acc r shape_kind fdm
    | Tclass (cls, exact, tyl) -> this#on_tclass acc r cls exact tyl
    | Tarraykind akind -> this#on_tarraykind acc r akind
    | Tdestructure tyl -> this#on_tlist acc r tyl
end
