cmake_minimum_required(VERSION 3.10)
project(jsonref C)

set(CMAKE_C_STANDARD 99)

if(WIN32)
    message(STATUS "Building for Windows")
elseif(APPLE)
    message(STATUS "Building for macOS")

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()
    message(STATUS "Homebrew prefix: ${HOMEBREW_PREFIX}")

    set(CMAKE_FIND_ROOT_PATH ${HOMEBREW_PREFIX} ${CMAKE_FIND_ROOT_PATH})
    set(CMAKE_PREFIX_PATH ${HOMEBREW_PREFIX} ${CMAKE_PREFIX_PATH})

elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux/Unix")
endif()

find_program(PHP_CONFIG php-config
    PATHS
        /usr/bin
        /usr/local/bin
        /opt/homebrew/bin
        /opt/local/bin
)

if(NOT PHP_CONFIG)
    message(FATAL_ERROR "php-config not found. Please install PHP development package.")
endif()

message(STATUS "Found php-config: ${PHP_CONFIG}")

execute_process(
    COMMAND ${PHP_CONFIG} --includes
    OUTPUT_VARIABLE PHP_INCLUDE_RAW
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(PHP_INCLUDE_ARGS UNIX_COMMAND "${PHP_INCLUDE_RAW}")
set(PHP_INCLUDE_DIRS "")
foreach(arg IN LISTS PHP_INCLUDE_ARGS)
    string(REGEX REPLACE "^-I" "" clean_dir "${arg}")
    list(APPEND PHP_INCLUDE_DIRS "${clean_dir}")
endforeach()

execute_process(
    COMMAND ${PHP_CONFIG} --extension-dir
    OUTPUT_VARIABLE PHP_EXTENSION_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${PHP_CONFIG} --cflags
    OUTPUT_VARIABLE PHP_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "PHP include dirs: ${PHP_INCLUDE_DIRS}")
message(STATUS "PHP extension dir: ${PHP_EXTENSION_DIR}")

if(APPLE)
    message(STATUS "Searching for jansson on macOS...")

    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(JANSSON QUIET jansson)
    endif()

    if(NOT JANSSON_FOUND)
        message(STATUS "pkg-config didn't find jansson, trying manual search")

        if(EXISTS "${HOMEBREW_PREFIX}/lib/libjansson.dylib")
            message(STATUS "Found jansson at ${HOMEBREW_PREFIX}")
            set(JANSSON_INCLUDE_DIR "${HOMEBREW_PREFIX}/include")
            set(JANSSON_LIBRARY "${HOMEBREW_PREFIX}/lib/libjansson.dylib")
            set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
            set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
            set(JANSSON_FOUND TRUE)
        else()
            find_path(JANSSON_INCLUDE_DIR jansson.h
                PATHS
                    ${HOMEBREW_PREFIX}/include
                    /usr/local/include
                    /opt/local/include
                NO_DEFAULT_PATH
            )

            find_library(JANSSON_LIBRARY
                NAMES jansson
                PATHS
                    ${HOMEBREW_PREFIX}/lib
                    /usr/local/lib
                    /opt/local/lib
                NO_DEFAULT_PATH
            )

            set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
            set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
        endif()
    endif()

    message(STATUS "Jansson include dir (final): ${JANSSON_INCLUDE_DIRS}")
    message(STATUS "Jansson library (final): ${JANSSON_LIBRARIES}")

    if(NOT EXISTS "${JANSSON_INCLUDE_DIRS}/jansson.h")
        message(FATAL_ERROR "jansson.h not found at ${JANSSON_INCLUDE_DIRS}")
    endif()

    if(NOT JANSSON_LIBRARIES)
        message(FATAL_ERROR "Jansson library not found")
    endif()

    if(NOT IS_ABSOLUTE "${JANSSON_LIBRARIES}")
        message(STATUS "Jansson library is not absolute path: ${JANSSON_LIBRARIES}")
        find_library(JANSSON_LIBRARY_FULL
            NAMES jansson
            PATHS ${HOMEBREW_PREFIX}/lib /usr/local/lib /opt/local/lib
            REQUIRED
        )
        set(JANSSON_LIBRARIES ${JANSSON_LIBRARY_FULL})
        message(STATUS "Found full path: ${JANSSON_LIBRARIES}")
    endif()

    if(NOT EXISTS "${JANSSON_LIBRARIES}")
        message(FATAL_ERROR "libjansson.dylib not found at ${JANSSON_LIBRARIES}")
    endif()

else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JANSSON REQUIRED jansson)
endif()

message(STATUS "Jansson include dirs: ${JANSSON_INCLUDE_DIRS}")
message(STATUS "Jansson libraries: ${JANSSON_LIBRARIES}")

add_library(jsonref SHARED jsonref.c)

target_include_directories(jsonref PRIVATE
    ${PHP_INCLUDE_DIRS}
    ${JANSSON_INCLUDE_DIRS}
)

target_link_libraries(jsonref PRIVATE
    ${JANSSON_LIBRARIES}
)

target_compile_options(jsonref PRIVATE
    -Wall -Wextra
)

if(APPLE)
    target_compile_options(jsonref PRIVATE
        -Wno-typedef-redefinition
        -Wno-unused-parameter
    )

    target_link_options(jsonref PRIVATE
        -undefined dynamic_lookup
    )

    target_link_directories(jsonref PRIVATE
        ${HOMEBREW_PREFIX}/lib
    )
endif()

target_compile_definitions(jsonref PRIVATE
    ZEND_ENABLE_STATIC_TSRMLS_CACHE=1
    COMPILE_DL_JSONREF=1
)

if(WIN32)
    set_target_properties(jsonref PROPERTIES
        PREFIX ""
        SUFFIX ".dll"
        OUTPUT_NAME "jsonref"
    )
else()
    set_target_properties(jsonref PROPERTIES
        PREFIX ""
        SUFFIX ".so"
        OUTPUT_NAME "jsonref"
    )
endif()

install(TARGETS jsonref
    DESTINATION ${PHP_EXTENSION_DIR}
)

message(STATUS "Build configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  PHP extension dir: ${PHP_EXTENSION_DIR}")
message(STATUS "  Output: jsonref${CMAKE_SHARED_LIBRARY_SUFFIX}")