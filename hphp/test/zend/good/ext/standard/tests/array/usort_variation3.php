<?hh
/* Prototype  : bool usort(&array $array_arg, string $cmp_function)
 * Description: Sort an array by values using a user-defined comparison function
 * Source code: ext/standard/array.c
 */

/*
 * Pass an array with different data types as keys to usort() to test how it is re-ordered
 */

function cmp_function($value1, $value2)
{
  if($value1 == $value2) {
    return 0;
  }
  else if($value1 > $value2) {
    return -1;
  }
  else {
    return 1;
  }
}
<<__EntryPoint>> function main(): void {
echo "*** Testing usort() : usage variation ***\n";

// different heredoc strings

// single line heredoc string
$simple_heredoc = <<<EOT2
simple
EOT2;

// multiline heredoc string
$multiline_heredoc = <<<EOT3
multiline heredoc with 123
and speci@! ch@r..\ncheck\talso
EOT3;

$array_arg = array(
  // numeric keys
  -2 => 9,
  8.9 => 8,
  012 => 7,
  0x34 => 6,

  // string keys
  'key' => 5,  //single quoted key
  "two" => 4,  //double quoted key
  " " => 0,  // space as key

  // bool keys
  TRUE => 100,
  FALSE => 25,

  // null keys
  NULL => 35,

  // binary key
  "a".chr(0)."b" => 45,
  b"binary" => 30,

  //heredoc keys
  $simple_heredoc => 75,
  $multiline_heredoc => 200,

  // default key
  1,
);

var_dump( usort(&$array_arg, fun('cmp_function')) );
echo "\n-- Sorted array after usort() function call --\n";
var_dump($array_arg);
echo "===DONE===\n";
}
