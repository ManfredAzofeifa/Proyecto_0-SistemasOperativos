cmake_minimum_required(VERSION 4.1)
project(proyecto_1_puente C)

set(CMAKE_C_STANDARD 17)

add_executable(proyecto_1_puente main.c
        puente.c
        auto.c
        auto.h
        config.c
        cola.h
        cola.c
        visual.c
        visual.h
)
