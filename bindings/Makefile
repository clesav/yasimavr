# Makefile for the python bindings of yasim-avr

# Copyright 2021 Clement Savergne <csavergne@yahoo.com>

# This file is part of yasim-avr.

# yasim-avr is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# yasim-avr is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.

ifeq ($(OS),Windows_NT)
	detected_OS := Windows
else
	detected_OS := $(shell uname)
endif

ifeq ($(detected_OS),Windows)
	MAKE_DIR := mkdir
	RM_FILE := del
	RM_DIR := rmdir /q /s
	COPY_FILE := copy /y
	LIB_EXT := pyd
else
	MAKE_DIR := mkdir -p
	RM_FILE := rm
	RM_DIR := rm -r
	COPY_FILE := cp
	LIB_EXT := so
endif


SIPLIB_DIR := siplib/sip/yasimavr/lib/
SIPGEN_DIR := sipgen/
SRC_DIR = src/
ifeq ($(detected_OS),Windows)
	SIPLIB_DIR_BKSL = $(subst /,\,$(SIPLIB_DIR))
	SIPGEN_DIR_BKSL = $(subst /,\,$(SIPGEN_DIR))
	SRC_DIR_BKSL = $(subst /,\,$(SRC_DIR))
else
	SIPLIB_DIR_BKSL = $(SIPLIB_DIR)
	SIPGEN_DIR_BKSL = $(SIPGEN_DIR)
	SRC_DIR_BKSL = $(SRC_DIR)
endif


all: siplib bindings

debug: siplib bindings-debug

siplib: FORCE
	-@$(MAKE_DIR) siplib
	cd src && python generate_siplib.py
	cd siplib/sip && python -B setup.py build --build-lib "."

bindings: siplib
	-@$(MAKE_DIR) sipgen
	-@$(MAKE_DIR) Release
	$(COPY_FILE) $(SRC_DIR_BKSL)pyproject.Release.toml $(SIPGEN_DIR_BKSL)pyproject.toml
	$(COPY_FILE) project.py sipgen
	cd sipgen && sip-build
	$(COPY_FILE) $(SIPGEN_DIR_BKSL)*.$(LIB_EXT) Release
	$(COPY_FILE) $(SIPGEN_DIR_BKSL)*.pyi Release
	$(COPY_FILE) $(SIPLIB_DIR_BKSL)_sip*.$(LIB_EXT) Release

bindings-debug: siplib
	-@$(MAKE_DIR) sipgen
	-@$(MAKE_DIR) Debug
	$(COPY_FILE) $(SRC_DIR_BKSL)pyproject.Debug.toml $(SIPGEN_DIR_BKSL)pyproject.toml
	$(COPY_FILE) project.py sipgen
	cd sipgen && sip-build --tracing --gdb --no-line-directive
	$(COPY_FILE) $(SIPGEN_DIR_BKSL)*.$(LIB_EXT) Debug
	$(COPY_FILE) $(SIPGEN_DIR_BKSL)*.pyi Debug
	$(COPY_FILE) $(SIPLIB_DIR_BKSL)_sip*.$(LIB_EXT) Debug

clean: FORCE
	-$(RM_DIR) sipgen
	-$(RM_DIR) siplib
	-$(RM_DIR) Release
	-$(RM_DIR) Debug
	-$(RM_DIR) __pycache__

FORCE:
