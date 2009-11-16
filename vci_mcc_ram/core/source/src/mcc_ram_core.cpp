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
#include "soclib_endian.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "mcc_globals.h"


namespace soclib {
namespace caba {

	using soclib::common::uint32_log2;
	using namespace soclib;
	using namespace std;
#define tmpl(x) template<typename vci_param,typename  sram_param> x MccRamCore<vci_param,sram_param>


	tmpl(/**/)::MccRamCore (
			sc_module_name insname,
			bool node_zero,
			const soclib::common::IntTab &i_ident,
			const soclib::common::IntTab &t_ident,
			const soclib::common::MappingTable * mt,
			const soclib::common::MappingTable * mt_inv,
			soclib::common::CcIdTable * cct,
			const unsigned int nb_p,
#ifdef DEBUG_SRAM
			const soclib::common::Loader &loader,				// Code loader
#endif
			const unsigned int line_size,
			unsigned int * table_cost,
			addr_to_homeid_entry_t * home_addr_table,
			unsigned int nb_m) :
		caba::BaseModule(insname),
		r_RAM_FSM("RAM_FSM"),
		r_INV_FSM("INV_FSM"),
		r_IN_TRANSACTION("IN_TRANSACTION"),
		r_PROCESSING_TLB_INV("PROCESSING_TLB_INV"),
		r_SAV_ADDRESS("SAV_ADDRESS"),
		r_SAV_SEGNUM("SAV_SEGNUM"),
		r_SAV_SCRID("SAV_SCRID"), 
		r_SAV_BLOCKNUM("SAV_BLOCKNUM"),
		r_INV_BLOCKADDRESS("INV_BLOCKADDRESS"),
		r_INV_TARGETADDRESS("INV_TARGETADDRESS"),
		r_INV_ADDR_OFFSET("INV_ADDR_OFFSET"),
		r_TARGET_PAGE_ADDR("TARGET_PAGE_ADDR"),
		r_NEW_PAGE_ADDR("NEW_PAGE_ADDR"),
		r_VIRT_POISON_PAGE("VIRT_POISON_PAGE"),
		m_vci_fsm(p_t_vci, mt -> getSegmentList(t_ident), (uint32_log2(PAGE_SIZE))),
#ifdef DEBUG_SRAM
		m_loader(loader),
#endif
		m_MapTab(*mt),
		m_MapTab_inv(*mt_inv),
		m_segment(*(mt -> getSegmentList(t_ident)).begin())
	{
		m_cct = cct;
		m_PAGE_SHIFT = uint32_log2(PAGE_SIZE);
		m_ADDR_WORD_SHIFT = uint32_log2(vci_param::B);
		m_ADDR_BLOCK_SHIFT = uint32_log2(vci_param::B) + uint32_log2(line_size);
		m_BLOCKMASK = (~0)<<(uint32_log2(line_size) + uint32_log2(vci_param::B));
		m_I_IDENT = mt_inv -> indexForId(i_ident);
		m_NB_PROCS = nb_p;
		m_node_zero = node_zero;
		m_nbcycles = 0;
		m_last_write_nack = false;

		CTOR_OUT


		m_vci_fsm.on_read_write_nlock_tlb(on_read, on_write);
		m_vci_fsm.on_tlb_miss_ack_nlock_tlb(on_tlb_miss, on_tlb_ack);
		m_vci_fsm.on_is_busy_nlock_tlb(is_busy);

		SC_METHOD (transition);
		sensitive_pos << p_clk;
		dont_initialize();

		SC_METHOD (genMoore);
		sensitive_neg << p_clk;
		dont_initialize();

#ifndef DEBUG_SRAM
		SC_METHOD (genMealy);
		sensitive << p_sram_ack;
		dont_initialize();
#endif

		// Some checks
		assert(mt -> getSegmentList(t_ident).size() == 1);

		// Data allocation/initialisation (Data, page table, directories).
#ifdef DEBUG_SRAM
		s_RAM = new typename vci_param::data_t [m_segment.size() >> m_ADDR_WORD_SHIFT];
#endif
		s_PAGE_TABLE = new PHYS_PAGE_TABLE(m_segment.size()/PAGE_SIZE,PAGE_SIZE,m_segment.baseAddress());
		s_DIRECTORY = new SOCLIB_DIRECTORY [m_segment.size() >> m_ADDR_BLOCK_SHIFT];
		s_POISONNED = new sc_signal<bool> [m_segment.size()/PAGE_SIZE];
		s_PT_DIRECTORY = new SOCLIB_DIRECTORY [m_segment.size()/PAGE_SIZE];
		for (unsigned int i = 0; i < (m_segment.size() >> m_ADDR_BLOCK_SHIFT); i++)
		{
			s_DIRECTORY[i].init(m_NB_PROCS);
		}
		for (unsigned int i = 0; i < (m_segment.size()/PAGE_SIZE) ; i++)
		{
			s_PT_DIRECTORY[i].init(m_NB_PROCS);
		}

	}; // end constructor

	tmpl(/**/)::~MccRamCore()
	{
		DTOR_OUT
#ifdef DEBUG_SRAM
		delete [] s_RAM;
#endif
		delete [] s_DIRECTORY;
		delete [] s_POISONNED;
		delete s_PAGE_TABLE;
		delete [] s_PT_DIRECTORY;

	};

}}
