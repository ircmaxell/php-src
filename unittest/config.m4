dnl
dnl $Id$
dnl

PHP_UNITTEST_OBJS=

PHP_SUBST(PHP_UNITTEST_OBJS)

PHP_ARG_WITH(gtest, gtest unit tests,
[  --with-gtest=DIR        The directory the gtest tests are installed in], no, no)

AC_MSG_CHECKING(for unittest build)
if test -r "$PHP_GTEST/include/gtest/gtest.h"; then
  PHP_REQUIRE_CXX()
  PHP_REQUIRE_AR()
  PHP_GTEST_ENABLED=yes
  GTEST_DIR=$PHP_GTEST
  GTEST_INCLUDE_DIR=$GTEST_DIR/include
  GTEST_INCLUDE=-I$GTEST_INCLUDE_DIR
  PHP_ADD_INCLUDE($GTEST_INCLUDE_DIR)
  PHP_SUBST(GTEST_DIR)
  PHP_SUBST(GTEST_INCLUDE)
  PHP_SUBST(GTEST_INCLUDE_DIR)
  PHP_ADD_MAKEFILE_FRAGMENT($abs_srcdir/unittest/Makefile.frag)
  
  
  UNITTEST_PATH=unittest/alltests

  BUILD_UNITTEST="\$(LIBTOOL) --mode=link \$(CXX) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_BINARY_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) \$(PHP_UNITTEST_OBJS) gtest_main.a -o \$(UNITTEST_PATH)"

  PHP_SUBST(UNITTEST_PATH)
  PHP_SUBST(BUILD_UNITTEST)
else
  AC_MSG_ERROR([Cannot find GTest header files under $PHP_GTEST.])
fi
