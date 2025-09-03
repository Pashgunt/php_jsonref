cmake_minimum_required(VERSION 3.10)
project(jsonref C)

set(CMAKE_C_STANDARD 99)

execute_process(
        COMMAND php-config --includes
        OUTPUT_VARIABLE PHP_INCLUDE_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "PHP includes raw: ${PHP_INCLUDE_RAW}")

separate_arguments(PHP_INCLUDE_DIRS UNIX_COMMAND "${PHP_INCLUDE_RAW}")

set(PHP_INCLUDE_PATHS "")
foreach(dir IN LISTS PHP_INCLUDE_DIRS)
    string(REPLACE "-I" "" clean_dir "${dir}")
    message(STATUS "PHP includes clear dir: ${clean_dir}")
    list(APPEND PHP_INCLUDE_PATHS "${clean_dir}")
endforeach()

execute_process(
        COMMAND php-config --extension-dir
        OUTPUT_VARIABLE PHP_EXTENSION_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "PHP extension dir: ${PHP_EXTENSION_DIR}")

find_package(PkgConfig REQUIRED)
pkg_search_module(JANSSON REQUIRED jansson)

add_library(jsonref SHARED jsonref.c)

target_include_directories(jsonref PRIVATE
        ${PHP_INCLUDE_PATHS}
        ${JANSSON_INCLUDE_DIRS}
)

target_link_libraries(jsonref PRIVATE ${JANSSON_LIBRARIES})

target_compile_definitions(jsonref PRIVATE
        -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1
        -DCOMPILE_DL_JSONREF=1
)

set_target_properties(jsonref PROPERTIES
        PREFIX ""
        SUFFIX ".so"
        OUTPUT_NAME "jsonref"
)

install(TARGETS jsonref DESTINATION ${PHP_EXTENSION_DIR})
