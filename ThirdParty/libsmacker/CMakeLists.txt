cmake_minimum_required(VERSION 3.2)

# libsmacker 1.2.0 (LGPL-2.1, see COPYING): an unmodified copy of
# https://github.com/greg-kennedy/libsmacker at commit 76094fb. Decodes the Smacker movies in CLAW.FEC.
project(libsmacker VERSION 1.2.0 LANGUAGES C)

add_library(libsmacker STATIC
        smacker.h
        smk_malloc.h
        smacker.c
        )
