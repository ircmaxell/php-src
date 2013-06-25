--TEST--
Protocol Basic Error
--FILE--
<?php

class Foo {
	public function bar($abc) {}
}
interface Bar {
	public function bar();
}

function foo(<Bar> $bar) {
	var_dump($bar);
}

foo(new Foo);
?>
--EXPECTF--
Catchable fatal error: Argument 1 passed to foo() must look like Bar, instance of Foo given, called in %s/basic_errors.php on line %d and defined in %s/basic_errors.php on line %d
