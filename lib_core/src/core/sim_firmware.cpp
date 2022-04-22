/*
 * sim_firmware.cpp
 *
 *	Copyright 2021 Clement Savergne <csavergne@yahoo.com>

 	This file is part of yasim-avr.

	yasim-avr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	yasim-avr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with yasim-avr.  If not, see <http://www.gnu.org/licenses/>.
 */

//=======================================================================================

#include "sim_firmware.h"
#include "sim_device.h"
#include <libelf.h>
#include <gelf.h>
#include <stdio.h>
#include <cstring>


//=======================================================================================

AVR_Firmware::AVR_Firmware()
:variant("")
,frequency(0)
,vcc(0.0)
,aref(0.0)
,flashsize(0)
,datasize(0)
,bsssize(0)
,console_register(0)
{}

AVR_Firmware::~AVR_Firmware()
{
	for (auto block: flashblocks)
		if (block.size)
			free(block.buf);
}

static Elf32_Phdr* elf_find_phdr(Elf32_Phdr* phdr_table, size_t phdr_count, GElf_Shdr* shdr)
{
	for (size_t i = 0; i < phdr_count; ++i) {
		Elf32_Phdr* phdr = phdr_table + i;
		if (phdr->p_type != PT_LOAD) continue;
		if (shdr->sh_offset < phdr->p_offset) continue;
		if ((shdr->sh_offset + shdr->sh_size) > (phdr->p_offset + phdr->p_filesz)) continue;
		return phdr;
	}
	return nullptr;
}

AVR_Firmware* AVR_Firmware::read_elf(const std::string& filename)
{
	Elf32_Ehdr elf_header;			// ELF header
	Elf *elf = nullptr;                // Our Elf pointer for libelf
	std::FILE *file;				// File Descriptor

	if ((file = fopen(filename.c_str(), "rb")) == nullptr) {
		AVR_StandardLogger.log(AVR_DeviceLogger::LOG_ERROR, "Unable to open ELF file '%s'", filename.c_str());
		return nullptr;
	}

	if (fread(&elf_header, sizeof(elf_header), 1, file) == 0) {
		AVR_StandardLogger.log(AVR_DeviceLogger::LOG_ERROR, "Unable to read ELF file '%s'", filename.c_str());
		fclose(file);
		return nullptr;
	}

	AVR_Firmware *firmware = new AVR_Firmware();
	int fd = fileno(file);

	/* this is actually mandatory !! otherwise elf_begin() fails */
	if (elf_version(EV_CURRENT) == EV_NONE) {
			/* library out of date - recover from error */
	}

	elf = elf_begin(fd, ELF_C_READ, nullptr);

	//Obtain the Program Header table pointer and size
	size_t phdr_count;
	elf_getphdrnum(elf, &phdr_count);
	Elf32_Phdr* phdr_table = elf32_getphdr(elf);

	//Iterate through all sections
	Elf_Scn* scn = nullptr;
	while ((scn = elf_nextscn(elf, scn)) != nullptr) {
		//Get the section header and its name
		GElf_Shdr shdr;
		gelf_getshdr(scn, &shdr);
		char * name = elf_strptr(elf, elf_header.e_shstrndx, shdr.sh_name);

		//For bss and data sections, store the size only
		if (!strcmp(name, ".bss")) {
			Elf_Data* s = elf_getdata(scn, nullptr);
			firmware->bsssize = s->d_size;
		}
		else if (!strcmp(name, ".data")) {
			Elf_Data* s = elf_getdata(scn, nullptr);
			firmware->datasize = s->d_size;
		}

		//The rest of the loop is to retrieve the binary data of the section.
		//Only if it's a loadable non-empty section.
		if (((shdr.sh_flags & SHF_ALLOC) == 0) || (shdr.sh_type != SHT_PROGBITS))
			continue;
		if (shdr.sh_size == 0)
			continue;

		//Find the Program Header segment containing this section
		Elf32_Phdr* phdr = elf_find_phdr(phdr_table, phdr_count, &shdr);
		if (!phdr) continue;

		Elf_Data* scn_data = elf_getdata(scn, nullptr);

		//Load Memory Address calculation
		unsigned int lma = phdr->p_paddr + shdr.sh_offset - phdr->p_offset;

		//Create the memory block
		Block b = { .size = scn_data->d_size, .base = lma, .buf = nullptr };
		if (scn_data->d_size) {
			b.buf = (uint8_t*) malloc(scn_data->d_size);
			memcpy(b.buf, scn_data->d_buf, scn_data->d_size);
		}

		//Add the block to the proper memory category (Flash, etc...)
		if (!strcmp(name, ".text") || !strcmp(name, ".rodata")) {
			firmware->flashblocks.push_back(b);
			firmware->flashsize += scn_data->d_size;
		}
		//TODO: add EEPROM, fuses, SIGROW, etc...
	}

	elf_end(elf);
	fclose(file);

	AVR_StandardLogger.log(AVR_DeviceLogger::LOG_DEBUG, "Firmware read from ELF file '%s'", filename.c_str());

	return firmware;
}

bool AVR_Firmware::load_flash(uint8_t* flash, uint8_t* tag, flash_addr_t flashend) const
{
	for (auto block : flashblocks) {
		if (block.size) {

			if (block.base + block.size > flashend + 1)
				return false;

			memcpy(flash + block.base, block.buf, block.size);
			memset(tag + block.base, 1, block.size);
		}
	}

	return true;
}
