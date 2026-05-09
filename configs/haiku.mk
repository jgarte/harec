# install locations
PREFIX = /boot/home/config/non-packaged
BINDIR = $(PREFIX)/bin

# variables used during build
PLATFORM = haiku
ARCH = x86_64
HARECFLAGS = -N "" -m .main
QBEFLAGS =
ASFLAGS =
LDLINKFLAGS = 
CFLAGS = -g -std=c11 -D_XOPEN_SOURCE=700 -Iinclude \
	-Wall -Wextra -Werror -pedantic -Wno-unused-parameter
LDFLAGS =
LIBS = -lm

# commands used by the build script
CC = cc
AS = as
LD = cc
QBE = qbe

# build locations
HARECACHE = .cache
BINOUT = .bin

# variables that will be embedded in the binary with -D definitions
DEFAULT_TARGET = $(ARCH)
VERSION = $$(./scripts/version)
