#include "zend.h"
#include "zend_API.h"
#include "zend_hash.h"
#include "gtest/gtest.h"

class zend_hash_test : public testing::Test {
public:
	zend_hash_test() {
		zend_hash_init(&test, 8, NULL, NULL, 1);
	}
	
	~zend_hash_test() {
		zend_hash_destroy(&test);
	}

protected:
	HashTable test;
};

TEST(zend_hash_test, supporting) {
	EXPECT_EQ(zend_inline_hash_func("l", 1), zend_inline_hash_func("l", 1));
	EXPECT_EQ(zend_inline_hash_func("lo", 2), zend_inline_hash_func("lo", 2));
	EXPECT_EQ(zend_inline_hash_func("lon", 3), zend_inline_hash_func("lon", 3));
	EXPECT_EQ(zend_inline_hash_func("long", 4), zend_inline_hash_func("long", 4));
	EXPECT_EQ(zend_inline_hash_func("longe", 5), zend_inline_hash_func("longe", 5));
	EXPECT_EQ(zend_inline_hash_func("longer", 6), zend_inline_hash_func("longer", 6));
	EXPECT_EQ(zend_inline_hash_func("longert", 7), zend_inline_hash_func("longert", 7));
	EXPECT_EQ(zend_inline_hash_func("longerte", 8), zend_inline_hash_func("longerte", 8));
	EXPECT_EQ(zend_inline_hash_func("longertes", 9), zend_inline_hash_func("longertes", 9));
	EXPECT_EQ(zend_inline_hash_func("longertest", 10), zend_inline_hash_func("longertest", 10));
	EXPECT_EQ(zend_inline_hash_func("longertesti", 11), zend_inline_hash_func("longertesti", 11));
	EXPECT_EQ(zend_inline_hash_func("longertestin", 12), zend_inline_hash_func("longertestin", 12));
	EXPECT_EQ(zend_inline_hash_func("longertesting", 13), zend_inline_hash_func("longertesting", 13));
	EXPECT_EQ(zend_inline_hash_func("longertestingt", 14), zend_inline_hash_func("longertestingt", 14));
	EXPECT_EQ(zend_inline_hash_func("longertestingth", 15), zend_inline_hash_func("longertestingth", 15));
	EXPECT_EQ(zend_inline_hash_func("longertestingthi", 16), zend_inline_hash_func("longertestingthi", 16));
	EXPECT_EQ(zend_inline_hash_func("longertestingthis", 17), zend_inline_hash_func("longertestingthis", 17));
}

TEST(zend_hash_test, basic) {
	
}
