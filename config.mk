# Copyright (C) 2022-2023 <alpheratz99@protonmail.com>
# This program is free software.

PREFIX = /usr/local

PKG_CONFIG = pkg-config

DEPENDENCIES = xcb xcb-image xcb-keysyms

INCS = $(shell $(PKG_CONFIG) --cflags $(DEPENDENCIES))
LIBS = $(shell $(PKG_CONFIG) --libs $(DEPENDENCIES))

CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os $(INCS)
LDFLAGS = -s $(LIBS)

CC = cc
