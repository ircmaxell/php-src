#include "zend_operators.h"
#include "gtest/gtest.h"

TEST(zend_atoi, basicOperation) {
	EXPECT_EQ(1024, zend_atoi("1k", 2));
}
