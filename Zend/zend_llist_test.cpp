#include "zend.h"
#include "zend_API.h"
#include "zend_hash.h"
#include "gtest/gtest.h"

#include "zend_llist.h"

class zend_llist_test : public testing::Test {
public:
	zend_llist_test() {
		zend_llist_init(&test, 8, NULL, 1);
	}
	
	~zend_llist_test() {
		zend_llist_destroy(&test);
	}

protected:
	zend_llist test;
};

TEST_F(zend_llist_test, basic) {
	int head = 1;
	int tail = 2;
	int *ptr = NULL;
	
	zend_llist_add_element(&test, static_cast<void*>(&tail));
	EXPECT_EQ(1, zend_llist_count(&test));
	
	zend_llist_prepend_element(&test, static_cast<void *>(&head));
	EXPECT_EQ(2, zend_llist_count(&test));
	
	ptr = static_cast<int*>(zend_llist_get_first(&test));
	EXPECT_EQ(1, *ptr);
	
	ptr = static_cast<int*>(zend_llist_get_last(&test));
	EXPECT_EQ(2, *ptr);
	
	zend_llist_clean(&test);
	EXPECT_EQ(0, zend_llist_count(&test));
}

