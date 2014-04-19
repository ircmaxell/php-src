--TEST--
Description: returned type does not match the type declaration

--FILE--
<?php

function get_config(): array {
    return 42;
}


--EXPECTF--
Fatal error: the function get_config was expected to return an array and returned an integer in %s on line %d



