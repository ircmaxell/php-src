--TEST--
Function Argument Parsing #003
--FILE--
<?php
function t1($a = 1 + 1, $b = 1 << 2, $c = "foo" . "bar") {
	var_dump($a, $b, $c);
}

t1();
?>
--EXPECT--
int(2)
int(4)
string(6) "foobar"
