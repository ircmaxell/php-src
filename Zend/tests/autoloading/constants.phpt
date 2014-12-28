--TEST--
Constant autoloading
--FILE--
<?php

php\autoload_register(php\AUTOLOAD_CONSTANT, function ($name) {
    echo "Autoloader called with name \"$name\"", PHP_EOL;
    define($name, "foobar");
});

var_dump(FOO1);
var_dump(Foo2);
var_dump(foo3);
var_dump(FOO1\bar);
var_dump(Foo2\bar);
var_dump(foo3\bar);
var_dump(foo\BAR1);
var_dump(foo\Bar2);
var_dump(foo\bar3);
--EXPECTF--
Autoloader called with name "FOO1"
string(6) "foobar"
Autoloader called with name "Foo2"
string(6) "foobar"
Autoloader called with name "foo3"
string(6) "foobar"
Autoloader called with name "FOO1\bar"
string(6) "foobar"
Autoloader called with name "Foo2\bar"
string(6) "foobar"
Autoloader called with name "foo3\bar"
string(6) "foobar"
Autoloader called with name "foo\BAR1"
string(6) "foobar"
Autoloader called with name "foo\Bar2"
string(6) "foobar"
Autoloader called with name "foo\bar3"
string(6) "foobar"
