unittest: $(UNITTEST_PATH)

CPPFLAGS += -isystem $(GTEST_DIR)/include

$(UNITTEST_PATH): $(PHP_GLOBAL_OBJS) $(PHP_BINARY_OBJS) $(PHP_UNITTEST_OBJS) unittest/gtest_main.a
	$(BUILD_UNITTEST)

GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
unittest/gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

unittest/gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

unittest/gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

unittest/gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^
