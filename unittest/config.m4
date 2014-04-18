dnl
dnl $Id$
dnl

PHP_SUBST(PHP_UNITTEST_OBJS)

PHP_ARG_WITH(gtest, gtest unit tests,
[  --with-gtest=DIR        The directory the gtest tests are installed in], no, no)

if test -r "$PHP_GTEST/include/gtest/gtest.h"; then
  PHP_GTEST_ENABLED=yes
  GTEST_DIR=$PHP_GTEST
  GTEST_INCLUDE=-I$GTEST_DIR
  PHP_ADD_INCLUDE($GTEST_DIR)
  PHP_SUBST(GTEST_INCLUDE)

  PHP_ADD_MAKEFILE_FRAGMENT($abs_src_dir/unittest/Makefile.frag)

  UNITTEST_PATH=unittest/alltests

  BUILD_UNITTEST="\$(LIBTOOL) --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_BINARY_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) \$(PHP_UNITTEST_OBJS) \$GTEST_DIR/include/gtest/gtest_main.cc -o \$(UNITTEST_PATH)"

  PHP_SUBST(UNITTEST_PATH)
  PHP_SUBST(BUILD_UNITTEST)
else
  AC_MSG_ERROR([Cannot find GTest header files under $PHP_GTEST.])
fi
