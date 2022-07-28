cmake_minimum_required(VERSION 3.14)
project(dll-bundler)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(LLVM REQUIRED CONFIG)

add_executable(dll-bundler "dll-bundler.cpp")
llvm_map_components_to_libnames(dll-bundler_llvm_libs object support)
target_link_libraries(dll-bundler PRIVATE ${dll-bundler_llvm_libs})

install(TARGETS dll-bundler DESTINATION "${CMAKE_INSTALL_BINDIR}")
