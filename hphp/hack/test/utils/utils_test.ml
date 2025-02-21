open Asserter

let assert_ns_split name assert_left assert_right =
  let (left, right) = Utils.split_ns_from_name name in
  String_asserter.assert_equals left assert_left "Namespace is wrong";
  String_asserter.assert_equals right assert_right "Namespace is wrong"

let test_namespace_splitter () =
  assert_ns_split "HH\\Lib\\Str\\Format" "HH\\Lib\\Str\\" "Format";
  assert_ns_split "NameWithoutANamespace" "\\" "NameWithoutANamespace";
  assert_ns_split "HH\\Lib\\Str\\" "HH\\Lib\\Str\\" "";
  assert_ns_split
    "\\HH\\Lib\\Hellothisisafunction"
    "\\HH\\Lib\\"
    "Hellothisisafunction";
  true

let assert_cm_split str expected : unit =
  Printf.printf "Testing [%s]\n" str;
  let r = Utils.split_class_from_method str in
  let success =
    match (r, expected) with
    | (None, None) -> true
    | (Some (a, b), Some (c, d)) -> a = c && b = d
    | _ -> false
  in
  ( if not success then
    let msg =
      Printf.sprintf "ASSERTION FAILURE: [%s] did not split correctly" str
    in
    failwith msg );
  ()

let test_class_meth_splitter () =
  assert_cm_split "A::B" (Some ("A", "B"));
  assert_cm_split
    "AReallyLongName::AnotherLongName"
    (Some ("AReallyLongName", "AnotherLongName"));
  assert_cm_split "::B" None;
  assert_cm_split "A::" None;
  assert_cm_split "::" None;
  assert_cm_split "Justsomerandomtext" None;
  true

let () =
  Unit_test.run_all
    [ ("test ability to split namespaces", test_namespace_splitter);
      ("test ability to split class::meth", test_class_meth_splitter) ]
