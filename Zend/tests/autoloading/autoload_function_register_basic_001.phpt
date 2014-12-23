--TEST--
Test php\autoload_register(): basic function behavior 001
--FILE--
<?php

echo "*** Testing php\autoload_register() : basic behavior ***\n";

php\autoload_register(php\AUTOLOAD_FUNCTION, "var_dump");

foo();

?>
--EXPECTF--
*** Testing php\autoload_register() : basic behavior ***
string(3) "foo"

Fatal error: Call to undefined function foo() in %s on line %d