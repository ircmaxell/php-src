--TEST--
function_exists function : autoloading
--FILE--
<?php
/* 
 * proto bool function_exists(string function_name [, bool use_autoload = true])
 * Function is implemented in Zend/zend_builtin_functions.c
*/ 

php\autoload_function_register(function ($name) {
    echo "$name: ";
});

echo "*** Testing function_exists() : autoloading functionality ***\n";

echo "Internal function: ";
var_dump(function_exists('function_exists'));

echo "User defined function: ";
function f() {}
var_dump(function_exists('f'));

echo "Non-Existant User Defined Function: ";
var_dump(function_exists('f1'));

echo "Non existent function, autoload off: ";
var_dump(function_exists('g', false));

?>
===Done===
--EXPECT--
*** Testing function_exists() : autoloading functionality ***
Internal function: bool(true)
User defined function: bool(true)
Non-Existant User Defined Function: f1: bool(false)
Non existent function, autoload off: bool(false)
===Done===
