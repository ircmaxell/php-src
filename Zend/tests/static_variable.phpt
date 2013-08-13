--TEST--
Static Variable Expressions
--FILE--
<?php
function foo() {
	static $a = 1 + 1;
	static $b = [ 1 + 1, 1 << 2 ];
	var_dump($a, $b);
}

foo();
?>
--EXPECT--
int(2)
array(2) {
  [0]=>
  int(2)
  [1]=>
  int(4)
}
