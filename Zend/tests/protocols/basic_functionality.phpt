--TEST--
Protocol Basic Functionality
--FILE--
<?php

class Foo implements Bar {
	public function bar() {}
}
class Baz {
	public function bar() {}
}
interface Bar {
	public function bar();
}

function foo(<Bar> $bar) {
	var_dump($bar);
}

foo(new Foo);
foo(new Baz);
?>
--EXPECT--
object(Foo)#1 (0) {
}
object(Baz)#1 (0) {
}
