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

COPY_FILE = copy /y

all: siplib bindings

debug: siplib bindings-debug

sipdirs: FORCE
	@if not exist siplib mkdir siplib
	@if not exist sipgen mkdir sipgen
	@if not exist Release mkdir Release
	@if not exist Debug mkdir Debug

siplib: sipdirs
	cd src && python generate_siplib.py
	cd siplib/sip && python setup.py build --build-lib "."
	copy /y siplib\sip\yasimavr\_sip*.pyd Release
	copy /y siplib\sip\yasimavr\_sip*.pyd Debug

bindings: siplib
	$(COPY_FILE) src\pyproject.Release.toml sipgen\pyproject.toml
	cd sipgen && sip-build
	$(COPY_FILE) sipgen\gen\core\yasimavr\core.pyd Release
	$(COPY_FILE) sipgen\gen\core\core.pyi Release
	$(COPY_FILE) sipgen\gen\arch_avr\yasimavr\arch_avr.pyd Release
	$(COPY_FILE) sipgen\gen\arch_avr\arch_avr.pyi Release
	$(COPY_FILE) sipgen\gen\arch_m0\yasimavr\arch_m0.pyd Release
	$(COPY_FILE) sipgen\gen\arch_m0\arch_m0.pyi Release

bindings-debug: siplib
	$(COPY_FILE) src\pyproject.Debug.toml sipgen\pyproject.toml
	$(COPY_FILE) src\project.py sipgen
	cd sipgen && sip-build --tracing --gdb
	$(COPY_FILE) sipgen\gen\core\yasimavr\core.pyd Debug
	$(COPY_FILE) sipgen\gen\core\core.pyi Debug
	$(COPY_FILE) sipgen\gen\arch_avr\yasimavr\arch_avr.pyd Debug
	$(COPY_FILE) sipgen\gen\arch_avr\arch_avr.pyi Debug
	$(COPY_FILE) sipgen\gen\arch_m0\yasimavr\arch_m0.pyd Debug
	$(COPY_FILE) sipgen\gen\arch_m0\arch_m0.pyi Debug

clean: FORCE
	-rmdir /q /s sipgen
	-rmdir /q /s siplib
	-del Release\*.pyd
	-del Release\*.pyi
	-del Debug\*.pyd
	-del Debug\*.pyi

FORCE:
