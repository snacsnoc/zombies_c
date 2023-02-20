cmake_minimum_required(VERSION 3.25)
project(zombie_c C)

set(CMAKE_C_STANDARD 11)

add_executable(zombie_c
        zombie.c)
target_link_libraries(zombie_c ncurses)
