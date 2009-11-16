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
			std::list<soclib::common::Segment> * seg_list,
			const soclib::common::Loader &loader
			) :												
		caba::BaseModule(insname),
		m_loader(loader)
	{
		unsigned int segnum  = 0;
		m_ADDR_WORD_SHIFT = uint32_log2(sram_param::B);


		m_segment_list = seg_list;
		m_nb_banks = m_segment_list -> size();
		assert((m_nb_banks > 0) && (m_nb_banks < 16));
		m_size_bytes =  new unsigned int[m_nb_banks]; 


		SC_METHOD (genMealy);
		sensitive << p_ce;
		dont_initialize();

		// Array allocation, data allocated in build_bank(..)
#ifdef DEBUG_SR
		s_RAM = new uint32_t * [m_nb_banks];
		for (unsigned int i = 0 ; i < m_nb_banks ; i++)
		{
			s_RAM[i] = NULL;
			m_size_bytes[i] = 0;
		}
#else
		s_RAM = new typename sram_param::data_t * [m_nb_banks];
		for (unsigned int i = 0 ; i < m_nb_banks ; i++)
		{
			s_RAM[i] = NULL;
			m_size_bytes[i] = 0;
		}
#endif

		std::list<soclib::common::Segment>::iterator     iter;
		for (segnum = 0, iter = m_segment_list -> begin(); iter != m_segment_list -> end() ; iter++, segnum++)
		{
			m_size_bytes[segnum] = iter -> size();
#ifdef DEBUG_SR
			s_RAM[segnum] = new uint32_t [iter -> size()/sram_param::B]; 
#else
			s_RAM[segnum] = new typename sram_param::data_t [iter -> size()/sram_param::B]; 
			std::cout << name() << "new SRAM bank @" << std::hex <<  iter -> baseAddress() << " - " << (iter -> size()) << std::endl;
#endif
		}

	}; // end constructor

	tmpl(/**/)::~SRam()
	{
		for (unsigned int i = 0 ; i < m_nb_banks ; i++)
		{
			delete [] s_RAM[i];
		}
		delete [] s_RAM;
		delete [] m_size_bytes;
	};
}}
