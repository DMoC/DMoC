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
#include "soclib_endian.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>


namespace soclib {
namespace caba {

	using soclib::common::uint32_log2;
	using namespace soclib;
	using namespace std;
#define tmpl(x) template<typename sram_param> x SRam<sram_param>


	tmpl(/**/)::SRam (
			sc_module_name insname,
			unsigned int size_bytes
			) :												
		caba::BaseModule(insname)
	{
		m_ADDR_WORD_SHIFT = uint32_log2(sram_param::B);
		m_size_bytes = size_bytes; 
		


		SC_METHOD (genMealy);
		sensitive << p_ce;
		dont_initialize();

		// Data allocation
#ifdef DEBUG_SR
		s_RAM = new uint32_t [m_size_bytes >> m_ADDR_WORD_SHIFT];
#else
		s_RAM = new typename sram_param::data_t [m_size_bytes >> m_ADDR_WORD_SHIFT];
#endif


	}; // end constructor

	tmpl(/**/)::~SRam()
	{
		delete [] s_RAM;
	};
}}
