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

#include "../include/vci_cc_ram.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>


namespace soclib {
namespace caba {
#define tmpl(x) template<typename vci_param> x VciCcRam<vci_param>

	/////////////////////////////////////////////////////
	//    Transition() method
	/////////////////////////////////////////////////////
	tmpl(void)::Transition()
	{
		s_t1 = c_sram_32->read_write_sram(2, 0x9829, 0, false, true, 0xf);
		s_t2 = c_sram_32->read_write_sram(2, 0x9a39, 0, false, true, 0xf);
		s_t3 = c_sram_32->read_write_sram(2, 0x9c49, 0, false, true, 0xf);
		s_t4 = c_sram_32->read_write_sram(2, 0x9e59, 0, false, true, 0xf);

		if (!p_resetn.read()) // Reset_N signal
		{
			// The code load is done at reset (and not in the constructor) to allow a
			// clean reset of the system at any time.
			switch (sram_param::B)
			{
				case 4 :
					if(!(c_sram_32 -> reload()))
						assert(false);
					break;
				default :
					assert(false);
					// Not a 32 bits modeled architecture, must rewrite or support other architecture
					// int the reload method of generic_sram component.
					break;
			}
		}
	};
}}
