--TEST--
Class Property Expressions Failure
--FILE--
<?php
class Foo {
	const BAR = 1 << 0;
	const BAZ = 1 << 1;
	public $bar = self::BAR | self::BAZ;
}
?>
--EXPECTF--
Parse error: syntax error, unexpected '|', expecting ',' or ';' in %s/class_properties_dynamic-failure.php on line %d
