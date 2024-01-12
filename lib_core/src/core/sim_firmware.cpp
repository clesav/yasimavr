/*
 * sim_firmware.cpp
 *
 *  Copyright 2021 Clement Savergne <csavergne@yahoo.com>

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
#include "libelf.h"
#include "gelf.h"
#include <stdio.h>
#include <cstring>

YASIMAVR_USING_NAMESPACE


//=======================================================================================

/**
   Build a empty firmware
 */
Firmware::Firmware()
:frequency(0)
,vcc(0.0)
,aref(0.0)
,console_register(0)
,m_datasize(0)
,m_bsssize(0)
{}


Firmware::Firmware(const Firmware& other)
:Firmware()
{
    *this = other;
}


/**
   Destroy a firmware
 */
Firmware::~Firmware()
{
    for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        for (Block& b : it->second)
            if (b.size)
                free(b.buf);
    }
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


static Firmware::Area addr_to_area(Elf64_Addr addr)
{
    if (addr < 0x800000)
        return Firmware::Area_Flash;
    else if (addr < 0x810000)
        return Firmware::Area_Data;
    else if (addr < 0x820000)
        return Firmware::Area_EEPROM;
    else if (addr < 0x830000)
        return Firmware::Area_Fuses;
    else if (addr < 0x840000)
        return Firmware::Area_Lock;
    else if (addr < 0x850000)
        return Firmware::Area_Signature;
    else
        return Firmware::Area_UserSignatures;
}


static size_t elf_addr_to_area_addr(Elf64_Addr elf_addr)
{
    if (elf_addr < 0x800000)
        return elf_addr;
    else
        return elf_addr & 0x00FFFFULL;
}


/**
   Read a ELF file and build a firmware, using the section binary blocks from the file.
   The ELF format decoding relies on the library libelf.
   \param filename file path of the ELF file to read
 */
Firmware* Firmware::read_elf(const std::string& filename)
{
    Elf32_Ehdr elf_header;          // ELF header
    Elf *elf = nullptr;                // Our Elf pointer for libelf
    std::FILE *file;                // File Descriptor

    if ((file = fopen(filename.c_str(), "rb")) == nullptr) {
        global_logger().err("Unable to open ELF file '%s'", filename.c_str());
        return nullptr;
    }

    if (fread(&elf_header, sizeof(elf_header), 1, file) == 0) {
        global_logger().err("Unable to read ELF file '%s'", filename.c_str());
        fclose(file);
        return nullptr;
    }

    Firmware *firmware = new Firmware();
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

        //Read the symbol table
        if (shdr.sh_type == SHT_SYMTAB) {
            Elf_Data* sdata = elf_getdata(scn, nullptr);
            //the number of symbols is the table size divided by entry size
            int symbol_count = shdr.sh_size / shdr.sh_entsize;
            for (int i = 0; i < symbol_count; ++i) {
                GElf_Sym elf_sym;
                gelf_getsym(sdata, i, &elf_sym);
                //Only keep global symbols
                if (ELF32_ST_TYPE(elf_sym.st_info) == STT_OBJECT) {
                    //Extract the symbol name and create an entry in our table
                    const char * sym_name = elf_strptr(elf, shdr.sh_link, elf_sym.st_name);
                    Symbol sym = { elf_addr_to_area_addr(elf_sym.st_value),
                                   elf_sym.st_size,
                                   sym_name,
                                   addr_to_area(elf_sym.st_value) };
                    firmware->m_symbols.push_back(sym);
                }
            }
            continue;
        }

        char * name = elf_strptr(elf, elf_header.e_shstrndx, shdr.sh_name);

        //For bss and data sections, store the size
        if (!strcmp(name, ".bss")) {
            Elf_Data* s = elf_getdata(scn, nullptr);
            firmware->m_bsssize = s->d_size;
        }
        else if (!strcmp(name, ".data")) {
            Elf_Data* s = elf_getdata(scn, nullptr);
            firmware->m_datasize = s->d_size;
        }

        //The rest of the loop is for retrieving the binary data of the section,
        //skip it if the section is non-loadable or empty.
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
        Block b = { 0, nullptr, 0 };
        if (scn_data->d_size) {
            b.size = scn_data->d_size;
            b.buf = (unsigned char*) malloc(scn_data->d_size);
            memcpy(b.buf, scn_data->d_buf, scn_data->d_size);
        }

        //Add the firmware chunk to the corresponding memory area (Flash, etc...)
        if (!strcmp(name, ".text") || !strcmp(name, ".data") || !strcmp(name, ".rodata")) {
            b.base = lma;
            firmware->m_blocks[Area_Flash].push_back(b);
        }
        else if (!strcmp(name, ".eeprom")) {
            b.base = lma - 0x810000;
            firmware->m_blocks[Area_EEPROM].push_back(b);
        }
        else if (!strcmp(name, ".fuse")) {
            b.base = lma - 0x820000;
            firmware->m_blocks[Area_Fuses].push_back(b);
        }
        else if (!strcmp(name, ".lock")) {
            b.base = lma - 0x830000;
            firmware->m_blocks[Area_Lock].push_back(b);
        }
        else if (!strcmp(name, ".signature")) {
            b.base = lma - 0x840000;
            firmware->m_blocks[Area_Signature].push_back(b);
        }
        else if (!strcmp(name, ".user_signatures")) {
            b.base = lma - 0x850000;
            firmware->m_blocks[Area_UserSignatures].push_back(b);
        }
        else {
            global_logger().err("Firmware section unknown: '%s'", name);
        }
    }

    elf_end(elf);
    fclose(file);

    global_logger().dbg("Firmware read from ELF file '%s'", filename.c_str());

    return firmware;
}


/**
   Add a binary block to the firmware
   \param area NVM area to which the block should be added
   \param block binary data block to be added
   \note A deep copy of the binary data is made
 */
void Firmware::add_block(Area area, const Block& block)
{
    //Make a deep copy of the memory block
    Block b = { block.size, nullptr, block.base };
    if (block.size) {
        b.buf = (unsigned char*) malloc(block.size);
        memcpy(b.buf, block.buf, block.size);
    }
    //Add the copy to the area map
    m_blocks[area].push_back(b);
}


/**
   Get whether the firmware has binary data for a given NVM area.
   \param area NVM area to check
   \return true if the NVM area has data, false otherwise
 */
bool Firmware::has_memory(Area area) const
{
    return m_blocks.find(area) != m_blocks.end();
}


/**
   Return the list of NVM areas for which the firmware has binary data.
 */
std::vector<Firmware::Area> Firmware::memories() const
{
    std::vector<Area> v;
    for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it)
        v.push_back(it->first);
    return v;
}


/**
   Get the total size of binary data loaded for a given NVM area.
   \param area NVM area to check
   \return the total size of data in bytes
 */
size_t Firmware::memory_size(Area area) const
{
    auto it = m_blocks.find(area);
    if (it == m_blocks.end())
        return 0;

    size_t s = 0;
    for (const Block& block : it->second)
        s += block.size;

    return s;
}


/**
   Get the binary blocks loaded for a given NVM area.
   \param area NVM area to check
   \return the binary data blocks loaded for this area, may be empty.
 */
std::vector<Firmware::Block> Firmware::blocks(Area area) const
{
    auto it = m_blocks.find(area);
    if (it == m_blocks.end())
        return std::vector<Block>();
    else
        return it->second;
}


/**
   Copy the binary blocks loaded for a given NVM area into a NVM model.
   \param area NVM area to retrieve
   \param memory NVM model where the data should be copied
   \return true if the binary data could be copied, false if it failed
 */
bool Firmware::load_memory(Area area, NonVolatileMemory& memory) const
{
    bool status = true;

    for (Block& fb : blocks(area))
        status &= memory.program(fb, fb.base);

    return status;
}


void Firmware::add_symbol(const Symbol& s)
{
    m_symbols.push_back(s);
}


Firmware& Firmware::operator=(const Firmware& other)
{
    for (auto it = m_blocks.begin(); it != m_blocks.end(); ++it) {
        for (Block& b : it->second)
            if (b.size)
                free(b.buf);
    }
    m_blocks.clear();

    frequency = other.frequency;
    vcc = other.vcc;
    aref = other.aref;
    console_register = other.console_register;
    m_datasize = other.m_datasize;
    m_bsssize = other.m_bsssize;
    m_symbols = other.m_symbols;

    for (auto it = other.m_blocks.begin(); it != other.m_blocks.end(); ++it) {
        for (const Block& b : it->second)
            add_block(it->first, b);
    }

    return *this;
}
