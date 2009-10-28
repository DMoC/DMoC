/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) TIMA
 *         Perre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr> 2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

#include "../include/mcc_ram_core.h"
#include <cassert>

#define tmpl(x) template<typename vci_param, typename sram_param> x MccRamCore<vci_param,sram_param>

namespace soclib {
namespace caba {

#ifdef DEBUG_SRAM
	/***********************************************************************************************/
	/* Reload code from binary, take care of endianness of host platform                           */
	/***********************************************************************************************/
	tmpl(void)::reload()
	{
		// TODO : hack, 32bits temporary array used to load a 32bits elf-binary.
                // problem : sc_uint<32> is 8 bytes on a 64bits machine and m_loader.load
                // don't care about this and just copy the 32bits code (two words per data_t cells!)
		uint32_t * temp = new uint32_t [m_segment.size()/vci_param::B];
		m_loader.load(temp, m_segment.baseAddress(), m_segment.size());
		for ( size_t addr = 0; addr < m_segment.size()/vci_param::B; ++addr )
		{
			temp[addr] = le_to_machine(temp[addr]);
			s_RAM[addr] = temp[addr];
		}
		delete [] temp;
	}
#endif

	/***********************************************************************************************/
	/* Read/Write method  @ tab[index], take care of byte-enable field                             */
	/***********************************************************************************************/
	tmpl(unsigned int)::rw_seg(typename vci_param::data_t* tab, unsigned int index,
			typename vci_param::data_t wdata, typename vci_param::be_t be,
			typename vci_param::cmd_t cmd)
	{
#ifndef DEBUG_SRAM
		assert(false); // deprecated
#endif
		unsigned int mask = 0;
		if (cmd == vci_param::CMD_READ) {    // read
			return(tab[index]);
		} else if(cmd == vci_param::CMD_WRITE) {        
			if ( be & 1 ) mask |= 0x000000ff;
			if ( be & 2 ) mask |= 0x0000ff00;
			if ( be & 4 ) mask |= 0x00ff0000;
			if ( be & 8 ) mask |= 0xff000000;

			tab[index] = (tab[index] & ~mask) | (wdata & mask);
			return(tab[index]);
		}
		assert(false); // unsupported request
	}

	tmpl(SOCLIB_DIRECTORY *)::read_dir(unsigned int page_index,unsigned int line_page_offset)
	{
		assert(false); // Todo : outdated, should be done with a real interface (sc_in/out).
		return  &s_DIRECTORY[(((page_index << m_PAGE_SHIFT) >> m_ADDR_BLOCK_SHIFT) + line_page_offset)];
	}

	tmpl(void)::write_dir(unsigned int page_index,unsigned int line_page_offset, SOCLIB_DIRECTORY * wdata){
		assert(false); // Todo : outdated, should be done with a real interface (sc_in/out).
		s_DIRECTORY[(((page_index << m_PAGE_SHIFT) >> m_ADDR_BLOCK_SHIFT) + line_page_offset)] = *wdata;
	}

	tmpl(bool)::is_access_sram_possible(void){
		assert(false); // Todo : outdated, should be done with a real interface (sc_in/out).
	}

	tmpl(unsigned int)::read_seg(unsigned int page_index,unsigned int word_page_offset){
		assert(false); // Todo : outdated, should be done with a real interface (sc_in/out).
#if 0
		return  rw_seg(s_RAM, (((page_index << m_PAGE_SHIFT) >> m_ADDR_WORD_SHIFT) + word_page_offset), 0, 0, vci_param::CMD_READ);
#endif
	}

	tmpl(void)::write_seg(unsigned int page_index,unsigned int word_page_offset, unsigned int wdata)
	{
		assert(false); // Todo : outdated, should be done with a real interface (sc_in/out).
#if 0
		rw_seg(s_RAM,(((page_index << m_PAGE_SHIFT) >> m_ADDR_WORD_SHIFT) + word_page_offset), wdata, 0xF, vci_param::CMD_WRITE);
#endif
	}

}}

