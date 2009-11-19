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

#include "../include/cc_ram_core.h"
#include "soclib_endian.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "cc_globals.h"


namespace soclib {
namespace caba {

	using soclib::common::uint32_log2;
	using namespace soclib;
	using namespace std;
#define tmpl(x) template<typename vci_param,typename  sram_param> x CcRamCore<vci_param,sram_param>


	tmpl(/**/)::CcRamCore (
			sc_module_name insname,
			const soclib::common::IntTab &i_ident,
			const soclib::common::IntTab &t_ident,
			const soclib::common::MappingTable * mt,
			const soclib::common::MappingTable * mt_inv,
			soclib::common::CcIdTable * cct,
			const unsigned int nb_p,
			const unsigned int line_size) :
		caba::BaseModule(insname),
		r_RAM_FSM("RAM_FSM"),
		r_INV_FSM("INV_FSM"),
		r_IN_TRANSACTION("IN_TRANSACTION"),
		r_SAV_ADDRESS("SAV_ADDRESS"),
		r_SAV_SEGNUM("SAV_SEGNUM"),
		r_SAV_SCRID("SAV_SCRID"), 
		r_SAV_BLOCKNUM("SAV_BLOCKNUM"),
		r_INV_BLOCKADDRESS("INV_BLOCKADDRESS"),
		r_INV_TARGETADDRESS("INV_TARGETADDRESS"),
		m_vci_fsm(p_t_vci, mt -> getSegmentList(t_ident), (uint32_log2(PAGE_SIZE))),
		m_MapTab(*mt),
		m_MapTab_inv(*mt_inv)
	{
		m_cct = cct;
		m_ADDR_WORD_SHIFT = uint32_log2(vci_param::B);
		m_ADDR_BLOCK_SHIFT = uint32_log2(vci_param::B) + uint32_log2(line_size);
		m_BLOCKMASK = (~0)<<(uint32_log2(line_size) + uint32_log2(vci_param::B));
		m_I_IDENT = mt_inv -> indexForId(i_ident);
		m_NB_PROCS = nb_p;
		m_nbcycles = 0;

		m_sram_oe = false;
		m_sram_we = false;
		m_sram_ce = false;

		CTOR_OUT

		std::list<soclib::common::Segment>::iterator     iter;

		m_vci_fsm.on_read_write_nlock(on_read, on_write);
		m_vci_fsm.on_is_busy_nlock(is_busy);

		SC_METHOD (transition);
		sensitive_pos << p_clk;
		dont_initialize();

		SC_METHOD (genMoore);
		sensitive_neg << p_clk;
		dont_initialize();

#ifndef DEBUG_SRAM
		SC_METHOD (genMealy);
		sensitive << p_sram_ack;
		sensitive << p_sram_din;
		sensitive_neg << p_clk;
		dont_initialize();
#endif

		// Some checks
		s_DIRECTORY = new SOCLIB_DIRECTORY * [m_vci_fsm.nbSegments()];
		for ( size_t i=0; i<m_vci_fsm.nbSegments(); ++i )
		{
			assert(m_vci_fsm.getSize(i) <= m_vci_fsm.getSize(i));
			// Data allocation/initialisation (Data, page table, directories).
			s_DIRECTORY[i] = new SOCLIB_DIRECTORY [m_vci_fsm.getSize(i) >> m_ADDR_BLOCK_SHIFT];
			for (unsigned int cell = 0; cell < (m_vci_fsm.getSize(i) >> m_ADDR_BLOCK_SHIFT); cell++)
			{
				s_DIRECTORY[i][cell].init(m_NB_PROCS);
			}
		}
	}; // end constructor

	tmpl(/**/)::~CcRamCore()
	{
		DTOR_OUT
		for ( size_t i=0; i<m_vci_fsm.nbSegments(); ++i )
		{
			delete [] s_DIRECTORY[i];
		}
		delete [] s_DIRECTORY;

	};

}}
