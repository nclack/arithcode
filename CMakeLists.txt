cmake_minimum_required(VERSION 2.8)
project(ac)

include_directories(src)

###############################################################################
#  Targets
###############################################################################

  file(GLOB SOURCES src/*.h src/*.c)
  add_executable(test app/test.c ${SOURCES})
