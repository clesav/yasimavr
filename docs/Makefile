# Makefile for yasimavr documentation
#
# Copyright 2023 Clement Savergne <csavergne@yahoo.com>
#
# This file is part of yasim-avr.
#
# yasim-avr is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# yasim-avr is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.


ifeq ($(OS),Windows_NT)
	detected_OS := Windows
else
	detected_OS := $(shell uname)
endif

ifeq ($(detected_OS),Windows)
	MAKE_DIR := mkdir
	RM_FILE := del /q
	RM_DIR := rmdir /q /s
	COPY_FILE := copy /y
	LIB_PREFIX :=
	LIB_EXT := dll
else
	MAKE_DIR := mkdir -p
	RM_FILE := rm
	RM_DIR := rm -r
	COPY_FILE := cp
	LIB_PREFIX := lib
	LIB_EXT := so
endif


all: sphinx_html_docs

doxygen_docs: FORCE
	-$(RM_DIR) dox
	@doxygen Doxyfile

sphinx_html_docs: doxygen_docs
	@python make_mock_libs.py
	-cd source/cpp_api && $(RM_FILE) *.*
	@sphinx-build -M html "source" "build"

clean: FORCE
	-$(RM_DIR) dox build
	-cd source/cpp_api && $(RM_FILE) *.*

FORCE:
