cmake_minimum_required(VERSION 2.8)
project(ac)

include_directories(src)

###############################################################################
#  Targets
###############################################################################

  file(GLOB SOURCES src/3/*.h src/3/*.c)
  add_executable(test app/test.c ${SOURCES})
