<?hh


<<__EntryPoint>>
function main_2() {
apc_add("ts", "TestString");
apc_add("ta", array("a" => 1, "b" => 2));
apc_add("ts", "NewValue");
apc_add("ta", array("newelement"));
apc_add(array("a" => 1, "b" => 2, "f" => "facebook"));
apc_add(array(
              "a" => array("b" => 1,
              "c" => array("d", "e")),
              "f" => array(1,2,3),
              "h" => "hello",
));

if (__hhvm_intrinsics\apc_fetch_no_check("ts") !== "TestString") {
  echo "no1\n";
}
if (__hhvm_intrinsics\apc_fetch_no_check("ta") !== array("a" => 1, "b" => 2)) {
  echo "no2\n";
}

if (__hhvm_intrinsics\apc_fetch_no_check("ts") !== "TestString") {
  echo "no3\n";
}
if (__hhvm_intrinsics\apc_fetch_no_check("ta") !== array("a" => 1, "b" => 2)) {
  echo "no4\n";
}
if (__hhvm_intrinsics\apc_fetch_no_check("a") !== 1) {
  echo "no19\n";
}
if (__hhvm_intrinsics\apc_fetch_no_check("f") !== "facebook") {
  echo "no20\n";
}

apc_add("texp", "TestString", 1);
sleep(2);
if (__hhvm_intrinsics\apc_fetch_no_check("texp") !== false) {
  echo "no5\n";
}

$ret = apc_store("foo", false);
if ($ret !== true) {
  echo "no6\n";
}
$ret = apc_add("foo", false);
if ($ret !== false) {
  echo "no7\n";
}
$ret = apc_fetch("foo", &$success);
if ($ret !== false) {
  echo "no8\n";
}
if ($success !== true) {
  echo "no9\n";
}
$ret = apc_fetch("bar", &$success);
if ($ret !== false) {
  echo "no10\n";
}
if ($success !== false) {
  echo "no11\n";
}

$map1 = array("foo" => false);
$ret = apc_fetch(array("foo"), inout $success);
if ($ret !== $map1) {
  echo "no12\n";
}
$ret = apc_fetch(array("bar"), inout $success);
if ($ret !== array()) {
  echo "no13\n";
}
if ($success !== false) echo "no14\n";
$ret = apc_fetch(array("foo", "bar"), inout $success);
if ($ret !== $map1) echo "no15\n";
if ($success !== true) echo "no16\n";
$ret = apc_fetch(array("foo", "bar", "foo", "bar"), inout $success);
if ($ret !== $map1) echo "no17\n";
if ($success !== true) echo "no18\n";

echo "ok\n";
}
