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

#ifndef SRAM_PARAM_H
#define SRAM_PARAM_H

#include <systemc>
#include <cassert>

namespace soclib{
namespace caba{

// A template class used to configure the sram and its interface,
// basically we define usefull types depending on input parameters.
template<
		unsigned int data_byte_width, // in bytes (ie. 4 -> 32 bits)
		unsigned int addr_bit_width
	> class SramParam
	{
	public :
		typedef bool ce_t;
		typedef bool oe_t;
		typedef bool we_t;
		typedef sc_dt::sc_uint<data_byte_width> be_t;
		typedef sc_dt::sc_uint<data_byte_width*8> data_t;
		typedef sc_dt::sc_uint<addr_bit_width> addr_t;
		typedef sc_dt::sc_uint<4> bk_t;
		static const unsigned int B = data_byte_width;
		static const unsigned int N = addr_bit_width;
	};
}}
#endif


