--TEST--
Protocol Test That Protected And Private Methods Are Ignored
--FILE--
<?php

class Foo {
	public function bar($abc) {}
}
class Bar {
	public function bar($abc) {}
	protected function bar2() {}
	private function bar3() {}
}

function foo(<Bar> $bar) {
	var_dump($bar);
}

foo(new Foo);
?>
--EXPECT--
object(Foo)#1 (0) {
}
