cmake_minimum_required(VERSION 3.10)
project(jsonref C)

set(CMAKE_C_STANDARD 99)

if(WIN32)
    message(STATUS "Building for Windows")
elseif(APPLE)
    message(STATUS "Building for macOS")
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

find_package(PkgConfig REQUIRED)

pkg_check_modules(JANSSON QUIET jansson)

if(NOT JANSSON_FOUND)
    message(STATUS "jansson not found via pkg-config, trying manual search")

    if(APPLE)
        find_path(JANSSON_INCLUDE_DIR jansson.h
            PATHS
                /opt/homebrew/include
                /usr/local/include
                /opt/local/include
        )

        find_library(JANSSON_LIBRARY
            NAMES jansson
            PATHS
                /opt/homebrew/lib
                /usr/local/lib
                /opt/local/lib
        )

        set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
        set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})

    elseif(UNIX AND NOT APPLE)
        find_path(JANSSON_INCLUDE_DIR jansson.h
            PATHS
                /usr/include
                /usr/local/include
        )

        find_library(JANSSON_LIBRARY
            NAMES jansson
            PATHS
                /usr/lib
                /usr/lib/x86_64-linux-gnu
                /usr/local/lib
        )

        set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
        set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})

    elseif(WIN32)
        find_path(JANSSON_INCLUDE_DIR jansson.h
            PATHS
                C:/vcpkg/installed/x64-windows/include
                C:/Program Files/jansson/include
        )

        find_library(JANSSON_LIBRARY
            NAMES jansson libjansson
            PATHS
                C:/vcpkg/installed/x64-windows/lib
                C:/Program Files/jansson/lib
        )

        set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
        set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
    endif()

    if(NOT JANSSON_INCLUDE_DIR OR NOT JANSSON_LIBRARY)
        message(FATAL_ERROR "jansson library not found. Please install it:\n"
            "  - Ubuntu: sudo apt install libjansson-dev\n"
            "  - macOS: brew install jansson\n"
            "  - Windows: vcpkg install jansson"
        )
    endif()
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
        -Wno-typedef-redefinition  # Подавляем предупреждения о переопределении типов
        -Wno-unused-parameter
    )

    target_link_options(jsonref PRIVATE
        -undefined dynamic_lookup  # Разрешаем неопределенные символы (будут подгружены из PHP)
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
