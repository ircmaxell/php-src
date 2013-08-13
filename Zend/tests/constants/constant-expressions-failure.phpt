--TEST--
Constant Expressions Failure
--FILE--
<?php
const FOO = 1;
const BAR = FOO | 1;
?>
--EXPECTF--
Parse error: syntax error, unexpected '|', expecting ',' or ';' in %s/constant-expressions-failure.php on line %d
