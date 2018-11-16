cmake_minimum_required(VERSION 3.10.0)
project(single_instance)

set(BOOST $ENV{DEVLIBS}/boost-1.67)
set(SPDLOG $ENV{DEVLIBS}/spdlog-1.1.0)


include_directories(${BOOST}/include ${SPDLOG}/include)
link_directories(${BOOST}/lib)

set(SINGLE_INSTANCE_GUARD_SRC
    src/single_instance_guard.h
    src/single_instance_guard.cpp
)

add_executable(test_driver src/test_driver.cpp)
add_executable(test_target WIN32 src/test_target.cpp ${SINGLE_INSTANCE_GUARD_SRC})