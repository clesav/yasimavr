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

COPY          = copy /y
DEL_FILE      = del /q

LIB_TARGET_DIR   = py\yasimavr\lib

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
	   py-bindings-clean
	-cd "$(LIB_TARGET_DIR)" && $(DEL_FILE) *.*


lib-core: FORCE
	cd "lib_core" && $(MAKE) release
	@if not exist "$(LIB_TARGET_DIR)" mkdir "$(LIB_TARGET_DIR)"
	$(COPY) "lib_core\Release\yasimavr_core.dll" "$(LIB_TARGET_DIR)"

lib-core-debug: FORCE
	cd "lib_core" && $(MAKE) debug
	@if not exist "$(LIB_TARGET_DIR)" mkdir "$(LIB_TARGET_DIR)"
	$(COPY) "lib_core\Debug\yasimavr_core.dll" "$(LIB_TARGET_DIR)"

lib-core-clean: FORCE
	-cd "lib_core" && $(MAKE) clean


lib-arch-avr: lib-core
	cd lib_arch_avr && $(MAKE) release
	$(COPY) "lib_arch_avr\Release\yasimavr_arch_avr.dll" "$(LIB_TARGET_DIR)"

lib-arch-avr-debug: lib-core-debug
	cd lib_arch_avr && $(MAKE) debug
	$(COPY) "lib_arch_avr\Debug\yasimavr_arch_avr.dll" "$(LIB_TARGET_DIR)"

lib-arch-avr-clean: FORCE
	-cd "lib_arch_avr" && $(MAKE) clean


lib-arch-xt: lib-core
	cd lib_arch_xt && $(MAKE) release
	$(COPY) "lib_arch_xt\Release\yasimavr_arch_xt.dll" "$(LIB_TARGET_DIR)"

lib-arch-xt-debug: lib-core-debug
	cd lib_arch_xt && $(MAKE) debug
	$(COPY) "lib_arch_xt\Debug\yasimavr_arch_xt.dll" "$(LIB_TARGET_DIR)"

lib-arch-xt-clean: FORCE
	-cd "lib_arch_xt" && $(MAKE) clean


py-bindings: libs
	cd bindings && $(MAKE) all
	$(COPY) bindings\Release\*.* "$(LIB_TARGET_DIR)"

py-bindings-debug: libs-debug
	cd bindings && $(MAKE) debug
	$(COPY) bindings\Debug\*.* "$(LIB_TARGET_DIR)"

py-bindings-clean: FORCE
	-cd bindings && $(MAKE) clean


FORCE:
