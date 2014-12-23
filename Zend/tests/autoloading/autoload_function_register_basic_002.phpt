--TEST--
Test php\autoload_register(): basic behavior 002
--FILE--
<?php
/**
 * This tests the proper fallback behavior of function calls that are not "use"d.
 *
 * Only the global function name should be attempted to be autoloaded
 */
namespace Bar {
echo "*** Testing php\autoload_register() : basic behavior ***\n";

\php\autoload_register(\php\AUTOLOAD_FUNCTION, "var_dump");

Foo();

}

?>
--EXPECTF--
*** Testing php\autoload_register() : basic behavior ***
string(3) "Foo"

Fatal error: Call to undefined function Bar\Foo() in %s on line %d