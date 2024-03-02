cmake_minimum_required(VERSION 3.26)
project(untitled1 C)

set(CMAKE_C_STANDARD 99)

include_directories(/opt/devkitpro/libnx/include/
        /usr/local/opt/sdl2/include/SDL2/
        /usr/local/opt/sdl2/lib/
        /usr/local/opt/freetype/include/freetype2/
        /usr/local/opt/freetype/lib/
        ./include/
)

add_executable(untitled1 source/main.c
        include/cursor.h
        source/cursor.c
        include/menu.h
        source/menu.c
        include/circularbuffer.h
        source/circularbuffer.c
)




