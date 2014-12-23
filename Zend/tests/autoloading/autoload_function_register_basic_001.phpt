--TEST--
Test php\autoload_function_register(): basic behavior 001
--FILE--
<?php

echo "*** Testing php\autoload_function_register() : basic behavior ***\n";

php\autoload_function_register("var_dump");

foo();

?>
--EXPECTF--
*** Testing php\autoload_function_register() : basic behavior ***
string(3) "foo"

Fatal error: Call to undefined function foo() in %s on line %d