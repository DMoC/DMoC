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

#include "../include/sram.h"
#include <cassert>

#define tmpl(x) template<typename sram_param> x SRam<sram_param>

namespace soclib {
namespace caba {

	using namespace soclib;

	/***********************************************************************************************/
	/* Reload code from binary, take care of endianness of host platform                           */
	/***********************************************************************************************/
#if 0
	tmpl(void)::reload()
	{
		// TODO : hack, 32bits temporary array used to load a 32bits elf-binary.
		// problem : sc_uint<32> is 8 bytes on a 64bits machine and m_loader.load
		// don't care about this and just copy the 32bits code (two words per data_t cells!)
		uint32_t * temp = new uint32_t [m_segment.size()/sram_param::B];
		m_loader.load(temp, m_segment.baseAddress(), m_segment.size());
		for ( size_t addr = 0; addr < m_segment.size()/sram_param::B; ++addr )
		{
			temp[addr] = le_to_machine(temp[addr]);
			s_RAM[addr] = temp[addr];
		}
		delete [] temp;
	}
#endif

	tmpl(bool)::set_sram_content(void * source, unsigned int nb_bytes)
	{
		assert(nb_bytes == m_size_bytes); // Should load the exact size of memory
		switch (sram_param::B)
		{
			case 4 : // 32 bits data word
				for ( size_t cell = 0; cell < m_size_bytes/sram_param::B; ++cell )
				{
					s_RAM[cell] = ((uint32_t *)source)[cell];
				}
				return true;
				break;
			default : 
				std::cout << "error while loading code into sram, word lenght not supported " << std::endl;
				return false;
				break;
		}
	}

	/***********************************************************************************************/
	/* Read/Write method  @ s_RAM[offset], take care of byte-enable field                          */
	/***********************************************************************************************/
	tmpl(typename sram_param::data_t)::read_write_sram(
									typename sram_param::addr_t offset,
									typename sram_param::data_t wdata,
									bool we, bool oe,
									typename sram_param::be_t be)
	{
#ifdef DEBUG_SR
		uint32_t mask = 0;
		uint32_t old_value;
#else
		typename sram_param::data_t mask = 0;
		typename sram_param::data_t old_value;
#endif
		assert(!(we && oe));
		assert(offset < (m_size_bytes >> m_ADDR_WORD_SHIFT));
		if (oe)
		{
			return(s_RAM[offset]);
		}
		else
		{
			assert(we);
			// TODO : this is a 32bits write!
			for (unsigned int i = 0 ; i < sram_param::B; i++)
			{
				if (be & (0x1 << i) ) mask |= ( 0xff << (i*8));
			}

			old_value = s_RAM[offset];
			s_RAM[offset] = (s_RAM[offset] & ~mask) | (wdata & mask);
#if 0
			// This components returns the previous value of the modified data
			return old_value;
#else
			return s_RAM[offset];
#endif
		}
	}
}}

