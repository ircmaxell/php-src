dnl
dnl $Id$
dnl

PHP_UNITTEST_OBJS=

PHP_SUBST(PHP_UNITTEST_OBJS)

PHP_ARG_ENABLE(unittest, gtest unit tests,
[  --enable-unittest    Build unit tests (requires a C++ compiler)], no, no)

AC_MSG_CHECKING(for unittest build)
if test "$PHP_UNITTEST" != "no"; then
  PHP_REQUIRE_CXX()
  PHP_REQUIRE_AR()
  PHP_GTEST_ENABLED=yes
  GTEST_DIR=$abs_srcdir/unittest/gtest
  GTEST_INCLUDE_DIR=$GTEST_DIR/include
  GTEST_INCLUDE=-I$GTEST_INCLUDE_DIR
  PHP_ADD_INCLUDE($GTEST_INCLUDE_DIR)
  PHP_SUBST(GTEST_DIR)
  PHP_SUBST(GTEST_INCLUDE)
  PHP_SUBST(GTEST_INCLUDE_DIR)
  PHP_ADD_MAKEFILE_FRAGMENT($abs_srcdir/unittest/Makefile.frag)
  
  
  UNITTEST_PATH=unittest/alltests

  BUILD_UNITTEST="\$(LIBTOOL) --mode=link \$(CXX) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_BINARY_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) \$(PHP_UNITTEST_OBJS) unittest/gtest_main.a -o \$(UNITTEST_PATH)"

  PHP_SUBST(UNITTEST_PATH)
  PHP_SUBST(BUILD_UNITTEST)
fi
