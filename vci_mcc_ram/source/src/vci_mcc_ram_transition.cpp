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

#include "../include/vci_mcc_ram.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>


namespace soclib {
namespace caba {
#define tmpl(x) template<typename vci_param> x VciMccRam<vci_param>

	/////////////////////////////////////////////////////
	//    Transition() method
	/////////////////////////////////////////////////////
	tmpl(void)::Transition()
	{
		if (!p_resetn.read()) // Reset_N signal
		{
#ifndef DEBUG_SRAM
			// The code load is done at reset (and not in the constructor) to allow a
			// clean reset of the system at any time.
			switch (sram_param::B)
			{
				case 4 :
					{
						uint32_t * temp;
						temp = new uint32_t [m_segment.size()/sram_param::B];
						m_loader.load(temp, m_segment.baseAddress(), m_segment.size());
						for ( size_t addr = 0; addr < m_segment.size()/sram_param::B; ++addr )
						{
							temp[addr] = le_to_machine(temp[addr]);
						}
						assert(c_sram_32 -> set_sram_content( (void *) temp ,(unsigned int) m_segment.size()));
						delete [] temp;
					}
					break;
				default :
					std::cout << "unsupported data width in vci_mmc_ram" << std::endl;
					assert(false);
					break;
			}
#endif
		}
	};
}}
