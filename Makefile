# Top-level Makefile fo yasim-avr
#
# Copyright 2021 Clement Savergne <csavergne@yahoo.com>
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
	RM_FILE := del
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


LIB_TARGET_DIR = py/yasimavr/lib/
LIB_CORE_REL_DIR := lib_core/Release/
LIB_CORE_DBG_DIR := lib_core/Debug/
LIB_ARCH_AVR_REL_DIR := lib_arch_avr/Release/
LIB_ARCH_AVR_DBG_DIR := lib_arch_avr/Debug/
LIB_ARCH_XT_REL_DIR := lib_arch_xt/Release/
LIB_ARCH_XT_DBG_DIR := lib_arch_xt/Debug/
BINDINGS_REL_DIR := bindings/Release/
BINDINGS_DBG_DIR := bindings/Debug/
ifeq ($(detected_OS),Windows)
	LIB_TARGET_DIR_BKSL = $(subst /,\,$(LIB_TARGET_DIR))
	LIB_CORE_REL_DIR_BKSL = $(subst /,\,$(LIB_CORE_REL_DIR))
	LIB_CORE_DBG_DIR_BKSL = $(subst /,\,$(LIB_CORE_DBG_DIR))
	LIB_ARCH_AVR_REL_DIR_BKSL = $(subst /,\,$(LIB_ARCH_AVR_REL_DIR))
	LIB_ARCH_AVR_DBG_DIR_BKSL = $(subst /,\,$(LIB_ARCH_AVR_DBG_DIR))
	LIB_ARCH_XT_REL_DIR_BKSL = $(subst /,\,$(LIB_ARCH_XT_REL_DIR))
	LIB_ARCH_XT_DBG_DIR_BKSL = $(subst /,\,$(LIB_ARCH_XT_DBG_DIR))
	BINDINGS_REL_DIR_BKSL = $(subst /,\,$(BINDINGS_REL_DIR))
	BINDINGS_DBG_DIR_BKSL = $(subst /,\,$(BINDINGS_DBG_DIR))
else
	LIB_TARGET_DIR_BKSL = $(LIB_TARGET_DIR)
	LIB_CORE_REL_DIR_BKSL = $(LIB_CORE_REL_DIR)
	LIB_CORE_DBG_DIR_BKSL = $(LIB_CORE_DBG_DIR)
	LIB_ARCH_AVR_REL_DIR_BKSL = $(LIB_ARCH_AVR_REL_DIR)
	LIB_ARCH_AVR_DBG_DIR_BKSL = $(LIB_ARCH_AVR_DBG_DIR)
	LIB_ARCH_XT_REL_DIR_BKSL = $(LIB_ARCH_XT_REL_DIR)
	LIB_ARCH_XT_DBG_DIR_BKSL = $(LIB_ARCH_XT_DBG_DIR)
	BINDINGS_REL_DIR_BKSL = $(BINDINGS_REL_DIR)
	BINDINGS_DBG_DIR_BKSL = $(BINDINGS_DBG_DIR)
endif


all: release

release: libs py-bindings

libs: lib-core \
	  lib-arch-avr \
	  lib-arch-xt

debug: libs-debug py-bindings-debug

libs-debug: lib-core-debug \
	        lib-arch-avr-debug \
	        lib-arch-xt-debug

clean: lib-core-clean \
	   lib-arch-avr-clean \
	   lib-arch-xt-clean \
	   py-bindings-clean \
	   docs-clean \
	   dist-clean
	-$(RM_DIR) build
	-$(RM_DIR) dist
	-$(RM_DIR) yasimavr.egg-info
	

dist-clean: FORCE
	-cd $(LIB_TARGET_DIR) && $(RM_FILE) *.pyd *.pyi *.dll *.so


lib-core: FORCE
	cd lib_core && $(MAKE) release
	$(COPY_FILE) $(LIB_CORE_REL_DIR_BKSL)$(LIB_PREFIX)yasimavr_core.$(LIB_EXT) $(LIB_TARGET_DIR_BKSL)

lib-core-debug: FORCE
	cd lib_core && $(MAKE) debug
	$(COPY_FILE) $(LIB_CORE_DBG_DIR_BKSL)$(LIB_PREFIX)yasimavr_core.$(LIB_EXT) $(LIB_TARGET_DIR_BKSL)

lib-core-clean: FORCE
	-cd lib_core && $(MAKE) clean


lib-arch-avr: lib-core
	cd lib_arch_avr && $(MAKE) release
	$(COPY_FILE) $(LIB_ARCH_AVR_REL_DIR_BKSL)$(LIB_PREFIX)yasimavr_arch_avr.$(LIB_EXT) $(LIB_TARGET_DIR_BKSL)

lib-arch-avr-debug: lib-core-debug
	cd lib_arch_avr && $(MAKE) debug
	$(COPY_FILE) $(LIB_ARCH_AVR_DBG_DIR_BKSL)$(LIB_PREFIX)yasimavr_arch_avr.$(LIB_EXT) $(LIB_TARGET_DIR_BKSL)

lib-arch-avr-clean: FORCE
	-cd lib_arch_avr && $(MAKE) clean


lib-arch-xt: lib-core
	cd lib_arch_xt && $(MAKE) release
	$(COPY_FILE) $(LIB_ARCH_XT_REL_DIR_BKSL)$(LIB_PREFIX)yasimavr_arch_xt.$(LIB_EXT) $(LIB_TARGET_DIR_BKSL)

lib-arch-xt-debug: lib-core-debug
	cd lib_arch_xt && $(MAKE) debug
	$(COPY_FILE) $(LIB_ARCH_XT_DBG_DIR_BKSL)$(LIB_PREFIX)yasimavr_arch_xt.$(LIB_EXT) $(LIB_TARGET_DIR_BKSL)

lib-arch-xt-clean: FORCE
	-cd lib_arch_xt && $(MAKE) clean


py-bindings: libs
	cd bindings && $(MAKE) all
	$(COPY_FILE) $(BINDINGS_REL_DIR_BKSL)*.* $(LIB_TARGET_DIR_BKSL)

py-bindings-debug: libs-debug
	cd bindings && $(MAKE) debug
	$(COPY_FILE) $(BINDINGS_DBG_DIR_BKSL)*.* $(LIB_TARGET_DIR_BKSL)

py-bindings-clean: FORCE
	-cd bindings && $(MAKE) clean

docs: FORCE
	-cd docs && $(MAKE)

docs-clean: FORCE
	-cd docs && $(MAKE) clean

FORCE:
