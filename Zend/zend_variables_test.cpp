#include "zend_variables.h"
#include "gtest/gtest.h"
#include "zend.h"
#include "zend_API.h"

TEST(zval, addRef) {
	zval *pzv = (*pzv) malloc(sizeof(pzv));
	zend_unit old_refcount = Z_REFCOUNT_P(pzv);
	zval_add_ref(&pzv);
	EXPECT_EQUALS(old_refcount + 1, Z_REFCOUNT_P(pzv));
	free(pzv);
}
