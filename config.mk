# Copyright (C) 2022 <alpheratz99@protonmail.com>
# This program is free software.

CC      = cc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Os
LDLIBS  = -lxcb -lxcb-image -lxcb-keysyms
LDFLAGS = -s
