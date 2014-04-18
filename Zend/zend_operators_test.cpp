#include "zend_operators.h"
#include "gtest/gtest.h"

TEST(zend_atoi, basicOperation) {
	EXPECT_EQ(1024, zend_atoi("1k", 2));
	EXPECT_EQ(2048, zend_atoi("2k", 2));
	EXPECT_EQ(3145728, zend_atoi("3m", 2));
	// check for 32 bit overflow
        EXPECT_EQ(0, zend_atoi("4g", 2));
	EXPECT_EQ(1, zend_atoi("1", 1));
}

TEST(zend_string_to_double, basicOperation) {
	EXPECT_EQ(1.0, zend_string_to_double("1", 1));
	EXPECT_EQ(1.5, zend_string_to_double("1.5", 3));
	EXPECT_EQ(1.5, zend_string_to_double("1.5fed", 6));
}
