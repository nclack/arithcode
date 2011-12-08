cmake_minimum_required(VERSION 2.8)
project(ac)

include_directories(src)

###############################################################################
#  Targets
###############################################################################

  file(GLOB SOURCES src/*.h src/*.c)
  add_executable(eg app/test.c ${SOURCES})

###############################################################################
#  Testing
###############################################################################

set(GTEST_ROOT ~/src/3rdParty/gtest-1.6.0 CACHE PATH "The root directory of the gtest install prefix")
find_package(gtest)
if(GTEST_FOUND)
  enable_testing()
  include_directories(${GTEST_INCLUDE_DIRS})
  file(GLOB TEST_SOURCES test/*.cc)
  add_executable(alltests 
    ${TEST_SOURCES}
    ${SOURCES}
    )
  target_link_libraries(alltests ${GTEST_BOTH_LIBRARIES})
  add_test(AllTests alltests)
endif()
