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
 * Copyright (c)  TIMA
 * 	   Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2006-2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

//////////////////////////////////////////////////////////////////////////////////
// Description:
// 	-24/07/2008
// 	This components implements a simple Write-through instruction/data cache.
// 	It also implements a directory-based, write-invalidate cache 
// 	coherence protocol.
// 	
//////////////////////////////////////////////////////////////////////////////////


#include <cassert>
#include <cstring>
#include <iostream>
#include "arithmetics.h"
#include "alloc_elems.h"
#include "vci_mcc_cache.h"
#include "mcc_globals.h" // utilisé pour connaitre la taille des pages du système 

#ifdef CDB_COMPONENT_IF_H
#include <stdlib.h> // related to CDB (strtoul)
#include <signal.h> // related to CDB (raise)
#endif

#ifdef DEBUG_CACHE
#define DCACHECOUT(x) if(x>=DEBUG_CACHE_LEVEL) cout
#else
#define DCACHECOUT(x) if(0) cout
#endif

#ifndef PAGE_BURST
#define PAGE_BURST 0
#endif

#define V_BIT 0x80000000

#define tmpl(x)  template<typename vci_param, typename iss_t> x VciMccCache<vci_param, iss_t>


namespace soclib { 
namespace caba {

using soclib::common::uint32_log2;
using namespace std;

tmpl(const char *)::m_model = "Special invention cache";

tmpl(/**/)::VciMccCache(
		sc_module_name insname,
		const soclib::common::IntTab &i_index,
		const soclib::common::IntTab &t_index,
		size_t icache_lines,
		size_t icache_words,
		size_t dcache_lines,
		size_t dcache_words,
		unsigned int procid,	
		uint32_t migrability_mask,
		const soclib::common::MappingTable * mt,
		const soclib::common::MappingTable * mt_inv
		)
: soclib::caba::BaseModule(insname),

	p_clk("clk"),
	p_resetn("resetn"),
	p_i_vci("p_i_vci"),
	p_t_vci("p_t_vci"),

	m_cacheability_table(mt -> getCacheabilityTable()),
	m_iss(this->name(), procid),

	s_dcache_lines(dcache_lines),
	s_dcache_words(dcache_words),
	s_icache_lines(icache_lines),
	s_icache_words(icache_words),

	s_write_buffer_size(dcache_words),
	m_plen_shift(uint32_log2(vci_param::B)),

	m_i_x( uint32_log2(s_icache_words), uint32_log2(vci_param::B) ),
	m_i_y( uint32_log2(s_icache_lines), uint32_log2(s_icache_words) + uint32_log2(vci_param::B) ),
	m_i_z( vci_param::N-uint32_log2(s_icache_lines) - uint32_log2(s_icache_words) - uint32_log2(vci_param::B),
			uint32_log2(s_icache_lines) + uint32_log2(s_icache_words) + uint32_log2(vci_param::B) ),
	m_icache_yzmask((~0)<<(uint32_log2(s_icache_words) + uint32_log2(vci_param::B))),

	m_d_x( uint32_log2(s_dcache_words), uint32_log2(vci_param::B) ),
	m_d_y( uint32_log2(s_dcache_lines), uint32_log2(s_dcache_words) + uint32_log2(vci_param::B) ),
	m_d_z( vci_param::N-uint32_log2(s_dcache_lines) - uint32_log2(s_dcache_words) - uint32_log2(vci_param::B),
			uint32_log2(s_dcache_lines) + uint32_log2(s_dcache_words) + uint32_log2(vci_param::B) ),
	m_dcache_yzmask((~0)<<(uint32_log2(s_dcache_words) + uint32_log2(vci_param::B))),
	m_dcache_zmask((~0)<<(uint32_log2(vci_param::B))),
	m_full_be( ~(~(0) << (vci_param::B))),
	m_PAGE_SHIFT(uint32_log2(PAGE_SIZE)),

	m_TLB(TLB_SIZE,PAGE_SIZE,migrability_mask),
		r_DCACHE_FSM("DCACHE_FSM"),
		r_PREV_DCACHE_FSM("PREV_DCACHE_FSM"),
		r_ICACHE_FSM("ICACHE_FSM"),
		r_VCI_REQ_FSM("VCI_REQ_FSM"),
		r_VCI_RSP_FSM("VCI_RSP_FSM"),
		r_VCI_INV_FSM("VCI_INV_FSM"),
		r_DCACHE_ADDR_SAVE("DCACHE_ADDR_SAVE"),
		r_DCACHE_DATA_SAVE("DCACHE_DATA_SAVE"),
		r_DCACHE_BE_SAVE("DCACHE_BE_SAVE"),
		r_DCACHE_UNCACHED_SAVE("DCACHE_UNCACHED_SAVE"),
		r_DCACHE_LL_SAVE("DCACHE_LL_SAVE"),
		r_DCACHE_SC_SAVE("DCACHE_SC_SAVE"),
		r_ICACHE_MISS_ADDR_SAVE("ICACHE_MISS_ADDR_SAVE"),
		r_DCACHE_ENQUEUED_ADDR_SAVE("DCACHE_ENQUEUED_ADDR_SAVE"),
		r_DCACHE_WUPDT_DATA_SAVE("DCACHE_WUPDT_DATA_SAVE"),
		r_DCACHE_WF_BE_SAVE("DCACHE_WF_BE_SAVE"),
		r_DCACHE_WL_BE_SAVE("DCACHE_WL_BE_SAVE"),
		r_DCACHE_PENDING_WRITE("DCACHE_PENDING_WRITE"),
		r_DCACHE_FIRST_CELL("DCACHE_FIRST_CELL"),
		r_DCACHE_REQ("DCACHE_REQ"),
		r_DCACHE_FLUSH_WB_REQ("DCACHE_FLUSH_WB_REQ"),
		r_WRITE_BURST_SIZE("WRITE_BURST_SIZE"),
		r_ICACHE_MISS_REQ("ICACHE_MISS_REQ"),
		r_DRSP_OK("DRSP_OK"),
		r_IRSP_OK("IRSP_OK"),
		r_DCACHE_RAM_INVAL_REQ("DCACHE_RAM_INVAL_REQ"),
		r_DRETRY_REQ("DRETRY_REQ"),
		r_IRETRY_REQ("IRETRY_REQ"),
		r_RETRY_ACK_REQ("RETRY_ACK_REQ"),
		m_WRITE_BUFFER_DATA("WRITE_BUFFER_DATA",s_write_buffer_size),
		m_WRITE_BUFFER_ADDR("WRITE_BUFFER_ADDR",s_write_buffer_size),
		r_DATA_FIFO_INPUT("DATA_FIFO_INPUT"),
		r_ADDR_FIFO_INPUT("ADDR_FIFO_INPUT"),
		r_REQ_ICACHE_ADDR_PHYS("REQ_ICACHE_ADDR_PHYS"),
		r_REQ_DCACHE_DATA("REQ_DCACHE_DATA"),
		r_REQ_DCACHE_ADDR_VIRT("REQ_DCACHE_ADDR_VIRT"),
		r_REQ_DCACHE_ADDR_PHYS("REQ_DCACHE_ADDR_PHYS"),
		r_REQ_CPT("REQ_CPT"),
		r_RSP_DCACHE_UNC_BUF("RSP_DCACHE_UNC_BUF"),
		r_RSP_CPT("RSP_CPT"),
		r_SAV_SCRID("SAV_SCRID"),
		r_SAV_PKTID("SAV_PKTID"),
		r_SAV_TRDID("SAV_TRDID"),
		r_TLB_INV_TAG_0("TLB_INV_TAG_0"),
		r_TLB_INV_TAG_1("TLB_INV_TAG_1"),
		r_IS_TLB_INV_0("IS_TLB_INV_0"),
		r_IS_TLB_INV_1("IS_TLB_INV_1"),
		r_TLB_INV_ACK_ADDR("TLB_INV_ACK_ADDR"),
		r_RAM_INV_ADDR("RAM_INV_ADDR"),
		r_DCACHE_CPT_INIT("DCACHE_CPT_INIT"),
		r_ICACHE_CPT_INIT("ICACHE_CPT_INIT"),
	m_id(procid),
	ncycles(0)
{
	assert(IS_POW_OF_2(icache_lines));
	assert(IS_POW_OF_2(dcache_lines));
	assert(IS_POW_OF_2(icache_words));
	assert(IS_POW_OF_2(dcache_words));
	assert(icache_words);
	assert(dcache_words);
	assert(icache_lines);
	assert(dcache_lines);
	assert(icache_words <= 16);
	assert(dcache_words <= 16);
	assert(icache_lines <= 1024);
	assert(dcache_lines <= 1024);

	// Some initialisations :
	if (mt_inv == NULL) // Only one NoC is used to transport requests AND invalidations,
											//it MUST not have "hierarchy" in order to avoid deadlocks
	{
		m_segment = new soclib::common::Segment(mt -> getSegment(t_index));
		m_t_ident = mt -> indexForId(t_index) ;  
	}
	else
	{
		m_segment = new soclib::common::Segment(mt_inv -> getSegment(t_index));
		m_t_ident = mt_inv -> indexForId(t_index) ;  
	}
	m_i_ident = mt -> indexForId(i_index) ;  
	// Allocation arrays and buffers, use alloc_elems to allocate contiguously the data and enhance performances
	// as an array, but calling a specific constructor for each object
	s_DCACHE_DATA = new sc_signal<typename vci_param::data_t>*[dcache_lines];
	for ( size_t i=0; i < dcache_lines; ++i )
	{
					std::ostringstream o;
					o << "DCACHE_DATA[" << i << "]";
					s_DCACHE_DATA[i] = soclib::common::alloc_elems<sc_signal<typename vci_param::data_t> >(o.str(), dcache_words);
	}

	s_DCACHE_TAG = soclib::common::alloc_elems<sc_signal<typename vci_param::addr_t> >("DCACHE_TAG", dcache_lines);

	s_ICACHE_DATA = new sc_signal<typename vci_param::data_t>*[icache_lines];
	for ( size_t i=0; i < icache_lines; ++i )
	{
					std::ostringstream o;
					o << "ICACHE_DATA[" << i << "]";
					s_ICACHE_DATA[i] = soclib::common::alloc_elems<sc_signal<typename vci_param::data_t> >(o.str(), icache_words);
	}

	s_ICACHE_TAG = soclib::common::alloc_elems<sc_signal<typename vci_param::addr_t> >("ICACHE_TAG", icache_lines);
	r_RSP_DCACHE_MISS_BUF = soclib::common::alloc_elems<sc_signal<typename vci_param::data_t> >("RSP_DCACHE_MISS_BUFF", dcache_words);
	r_RSP_ICACHE_MISS_BUF = soclib::common::alloc_elems<sc_signal<typename vci_param::data_t> >("RSP_DCACHE_MISS_BUFF", icache_words);

	SC_METHOD (transition);
	sensitive_pos << p_clk;
	dont_initialize();

	SC_METHOD (genMoore);
	sensitive_neg << p_clk;
	dont_initialize();
} // end  constructor

tmpl(/**/)::~VciMccCache()
{
	for ( size_t i=0; i<s_dcache_lines; ++i )
		soclib::common::dealloc_elems(s_DCACHE_DATA[i], s_dcache_words);
	soclib::common::dealloc_elems(s_DCACHE_TAG, s_dcache_lines);

	for ( size_t i=0; i<s_icache_lines; ++i )
		soclib::common::dealloc_elems(s_ICACHE_DATA[i], s_icache_words);
	soclib::common::dealloc_elems(s_ICACHE_TAG, s_icache_lines);
	soclib::common::dealloc_elems(r_RSP_DCACHE_MISS_BUF, s_dcache_words);
	soclib::common::dealloc_elems(r_RSP_ICACHE_MISS_BUF, s_icache_words);

	delete [] s_DCACHE_DATA;
	delete [] s_ICACHE_DATA;
	delete m_segment;
};

}} // name spaces caba soclib

