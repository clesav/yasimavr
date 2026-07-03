# Makefile for yasim-avr automatic tests firmware
#
# Copyright 2026 Clement Savergne <csavergne@yahoo.com>
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


#if your avr-gcc does not support atmega328 or atmega4809 natively,
#a path to the atpack extracted content must be provided in the
#environment variable ATPACK_MEGA


define f_dev_opts
	ifeq ($(1),)
		DEV_OPTS = -mmcu=$(2)
	else
		DEV_OPTS = -mmcu=$(2) -B"$(1)/gcc/dev/$(2)" -I"$(1)/include"
	endif
endef

all: testfw_atmega328.elf \
	testfw_atmega4809.elf

testfw_atmega328.elf: testfw_mega.c Makefile
	$(eval $(call f_dev_opts,$(ATPACK_MEGA),atmega328))
	avr-gcc $(DEV_OPTS) -O1 -o $@ testfw_mega.c

testfw_atmega4809.elf: testfw_xt_mega.c Makefile
	$(eval $(call f_dev_opts,$(ATPACK_MEGA),atmega4809))
	avr-gcc $(DEV_OPTS) -O1 -o $@ testfw_xt_mega.c
