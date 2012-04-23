cmake_minimum_required(VERSION 2.8)
project(ArithmeticCoding)
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0a)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY 
  "Arithmetic coding in C"
    )

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

###############################################################################
#  Documentation
###############################################################################

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
                 @ONLY
                 )
  add_custom_target(apidocs
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doc/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT           "Generating API Docs"
    VERBATIM
    )
endif()
