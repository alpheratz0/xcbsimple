# Copyright (C) 2022 <alpheratz99@protonmail.com>
# This program is free software.

CC=cc
INCS=-I/usr/X11R6/include
CFLAGS=-std=c99 -pedantic -Wall -Wextra -Os $(INCS)
LDLIBS=-lxcb -lxcb-image -lxcb-keysyms
LDFLAGS=-L/usr/X11R6/lib -s
