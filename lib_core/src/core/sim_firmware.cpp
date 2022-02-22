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
//#include "../fw_support/avr_mcu_section.h"
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
,flashbase(0)
,flash(nullptr)
,flashsize(0)
,datasize(0)
,bsssize(0)
,eeprom(nullptr)
,eesize(0)
//,fuse(nullptr)
//,fusesize(0)
//,lockbits(nullptr)
,console_register(0)
{}

AVR_Firmware::~AVR_Firmware()
{
	if (flash)
		free(flash);
	if (eeprom)
		free(eeprom);
//	if (fuse)
//		free(fuse);
//	if (lockbits)
//		free(lockbits);
}

static void elf_copy_section(const char *name, Elf_Data *data, uint8_t **dest, uint32_t *destsize)
{
	*dest = (uint8_t*) malloc(data->d_size);
	if (*dest) {
		memcpy(*dest, data->d_buf, data->d_size);
		if (destsize)
			*destsize = data->d_size;
	}
}

//static void elf_parse_mmcu_section(AVR_Firmware* firmware, uint8_t *src, uint32_t size)
//{
//	while (size) {
//		uint8_t tag = *src++;
//		uint8_t ts = *src++;
//		unsigned int next = (size > 2 + ts) ? 2 + ts : size;
//		switch (tag) {
//			case AVR_MMCU_TAG_FREQUENCY:
//				firmware->frequency = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
//				break;
//			case AVR_MMCU_TAG_NAME:
//				firmware->variant = (char*) src;
//				break;
//			case AVR_MMCU_TAG_VCC: {
//				uint32_t v = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
//				firmware->vcc = v / 1000.0;
//			} break;
//			case AVR_MMCU_TAG_AREF: {
//				uint32_t v = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
//				firmware->aref = v / 1000.0;
//			} break;
//			case AVR_MMCU_TAG_PORT_EXTERNAL_PULL: {
//				/*for (int i = 0; i < 8; i++)
//					if (!firmware->external_state[i].port) {
//						firmware->external_state[i].port = src[2];
//						firmware->external_state[i].mask = src[1];
//						firmware->external_state[i].value = src[0];
//						break;
//					}*/
//			} break;
//			case AVR_MMCU_TAG_SIMAVR_COMMAND: {
//				//firmware->command_register_addr = src[0] | (src[1] << 8);
//			}	break;
//			case AVR_MMCU_TAG_SIMAVR_CONSOLE: {
//				firmware->console_register = src[0] | (src[1] << 8);
//			}	break;
//		}
//		size -= next;
//		src += next - 2; // already incremented
//	}
//}

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
	Elf_Data *data_data = nullptr;
	Elf_Data *data_text = nullptr;
	Elf_Data *data_ee = nullptr;
//	Elf_Data *data_fuse = nullptr;
//	Elf_Data *data_lockbits = nullptr;

	/* this is actually mandatory !! otherwise elf_begin() fails */
	if (elf_version(EV_CURRENT) == EV_NONE) {
			/* library out of date - recover from error */
	}
	// Iterate through section headers again this time well stop when we find symbols
	elf = elf_begin(fd, ELF_C_READ, nullptr);

	Elf_Scn *scn = nullptr;                   /* Section Descriptor */

	while ((scn = elf_nextscn(elf, scn)) != nullptr) {
		GElf_Shdr shdr;                 /* Section Header */
		gelf_getshdr(scn, &shdr);
		char * name = elf_strptr(elf, elf_header.e_shstrndx, shdr.sh_name);

		if (!std::strcmp(name, ".text"))
			data_text = elf_getdata(scn, nullptr);
		else if (!strcmp(name, ".data"))
			data_data = elf_getdata(scn, nullptr);
		else if (!strcmp(name, ".eeprom"))
			data_ee = elf_getdata(scn, nullptr);
//		else if (!strcmp(name, ".fuse"))
//			data_fuse = elf_getdata(scn, nullptr);
//		else if (!strcmp(name, ".lock"))
//			data_lockbits = elf_getdata(scn, nullptr);
		else if (!strcmp(name, ".bss")) {
			Elf_Data *s = elf_getdata(scn, nullptr);
			firmware->bsssize = s->d_size;
		}
//		else if (!strcmp(name, ".mmcu")) {
//			Elf_Data *s = elf_getdata(scn, nullptr);
//			elf_parse_mmcu_section(firmware, (uint8_t*) s->d_buf, s->d_size);
//		//	avr->frequency = f_cpu;
//		}
	}

	uint32_t offset = 0;
	firmware->flashsize = (data_text ? data_text->d_size : 0) + (data_data ? data_data->d_size : 0);
	firmware->flash = (uint8_t*) malloc(firmware->flashsize);

	// using unsigned int for output, since there is no AVR with 4GB
	if (data_text) {
		memcpy(firmware->flash + offset, data_text->d_buf, data_text->d_size);
		offset += data_text->d_size;
	}
	if (data_data) {
		memcpy(firmware->flash + offset, data_data->d_buf, data_data->d_size);
		offset += data_data->d_size;
		firmware->datasize = data_data->d_size;
	}
	if (data_ee)
		elf_copy_section(".eeprom", data_ee, &firmware->eeprom, &firmware->eesize);

//	if (data_fuse)
//		elf_copy_section(".fuse", data_fuse, &firmware->fuse, &firmware->fusesize);
//
//	if (data_lockbits)
//		elf_copy_section(".lock", data_lockbits, &firmware->lockbits, nullptr);

	elf_end(elf);
	fclose(file);
	AVR_StandardLogger.log(AVR_DeviceLogger::LOG_DEBUG, "Firmware read from ELF file '%s'", filename.c_str());
	return firmware;
}
