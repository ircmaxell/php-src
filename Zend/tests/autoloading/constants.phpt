--TEST--
Constant autoloading
--FILE--
<?php

php\autoload_register(php\AUTOLOAD_CONSTANT, function ($name) {
    echo "Autoloader called with name \"$name\"", PHP_EOL;
    define($name, "foobar");
});

$a = FOO1;
$b = Foo2;
$c = foo3;
$d = FOO1\bar;
$e = Foo2\bar;
$f = foo3\bar;
$g = foo\BAR1;
$h = foo\Bar2;
$i = foo\bar3;

php\autoload_unregister_all();
php\autoload_register(php\AUTOLOAD_CONSTANT, function ($name) {
    echo "Useless autoloader called with name \"$name\"", PHP_EOL;
});

$j = foo\bar;
--EXPECTF--
Autoloader called with name "FOO1"
Autoloader called with name "Foo2"
Autoloader called with name "foo3"
Autoloader called with name "FOO1\bar"
Autoloader called with name "Foo2\bar"
Autoloader called with name "foo3\bar"
Autoloader called with name "foo\BAR1"
Autoloader called with name "foo\Bar2"
Autoloader called with name "foo\bar3"
Useless autoloader called with name "foo\bar"

Fatal error: Undefined constant 'foo\bar' in %s on line %d
