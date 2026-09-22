cmake_minimum_required(VERSION 3.20)

project(static_threading_task LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(static_threading_task
    main.cpp
)

if (MSVC)
    target_compile_options(static_threading_task PRIVATE
        /W4
        /permissive-
    )
else()
    target_compile_options(static_threading_task PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
    )
endif()

find_package(Threads REQUIRED)

target_link_libraries(static_threading_task PRIVATE
    Threads::Threads
)