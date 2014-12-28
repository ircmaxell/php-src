--TEST--
Constant autoloading failure
--FILE--
<?php

php\autoload_register(php\AUTOLOAD_CONSTANT, function ($name) {
    echo "Useless autoloader called with name \"$name\"", PHP_EOL;
});

var_dump(foo);
var_dump(foo\bar);
--EXPECTF--
Useless autoloader called with name "foo"

Notice: Use of undefined constant foo - assumed 'foo' in %s on line %d
string(3) "foo"
Useless autoloader called with name "foo\bar"

Fatal error: Undefined constant 'foo\bar' in %s on line %d
