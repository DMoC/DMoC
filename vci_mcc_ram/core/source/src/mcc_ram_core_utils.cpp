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

	using namespace soclib;

	/***********************************************************************************************/
	/* Check if virtual address of the request is equal to the virtual address of the accessed     */
  /* page. The virtual address is contained in the PacketId VCI field which is unused now a day. */
	/***********************************************************************************************/
	tmpl(bool)::check_virtual_access(typename vci_param::addr_t addr, unsigned int node_id)
	{
		DRAMCOUT(1) << name() << " [R/W] > virt : " << (s_PAGE_TABLE -> wiam(addr)) << endl;
		DRAMCOUT(1) << name() << "     > phys : " << addr << endl;
		DRAMCOUT(1) << name() << "     > id   : " << node_id << endl;
#if 0
		// Check of virtual adress location
		if (((m_vci_fsm.currentPktid() != ((s_PAGE_TABLE -> wiam(addr)) >> m_PAGE_SHIFT ))) && (node_id < m_NB_PROCS))
		{
			cerr << name() << " error : accessing a data with the wrong tag " << endl;
			cerr << name() << "         request to phys address     0x " << hex << addr <<  endl;
			cerr << name() << "         with virtual address        0x " << hex << m_vci_fsm.currentPktid() << endl;
			cerr << name() << "         but mapped virt address is  0x " << hex << s_PAGE_TABLE -> wiam(addr) << endl;
			cerr << name() << "         procid is  " << dec << node_id << endl;
		}
		assert((m_vci_fsm.currentPktid() == ((s_PAGE_TABLE -> wiam(addr)) >> m_PAGE_SHIFT )) || (node_id >= m_NB_PROCS));
#else
		return true;
#endif
	}

	/***********************************************************************************************/
	/* Check if page index is in the range of this segment                                         */
	/***********************************************************************************************/
	tmpl(bool)::check_page_access(typename vci_param::addr_t addr)
	{
		if (!((  ((addr - m_segment.baseAddress() ) >> m_PAGE_SHIFT) < (m_segment.size()/PAGE_SIZE))))
		{
			std::cerr << name() << " [error] , accessing a page index out of range " << std::endl;
			std::cerr << name() << "           home_entry    : " << std::hex << addr  << std::endl;
			std::cerr << name() << "           base          : " << std::hex << m_segment.baseAddress()  << std::endl;
			std::cerr << name() << "           index         : " << std::dec << ((addr - m_segment.baseAddress()) >> m_PAGE_SHIFT) << std::endl;
			std::cerr << name() << "           total indexes : " << std::dec << (m_segment.size()/PAGE_SIZE) << std::endl;
		}
		return(  ((addr - m_segment.baseAddress()) >> m_PAGE_SHIFT) < (m_segment.size()/PAGE_SIZE));
	}



	/***********************************************************************************************/
	/* Called by target_fsm to know if the request can be processed (ack of incomming request),    */
  /* priority set to requests from external modules (related to page migration)                  */
	/***********************************************************************************************/
	tmpl(bool)::is_busy(void)
	{
#ifndef NOCTRL
		return (p_in_ctrl_req.read());
#else
#ifdef DEADLOCK_NACK_POLICY
		return (r_RAM_FSM != RAM_IDLE);
#else
		return false;
#endif
#endif
	}

	/***********************************************************************************************/
	/* Called by target_fsm on a VCI write request, return NACK if the page is poisonned,          */
	/* check for invalidations to be sent to sharer caches.                                        */
	/***********************************************************************************************/
	tmpl(bool)::on_write(size_t seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be, bool eop)
	{
		// node_id : in [0 - nb_coherent_initiators_on_the_platform[, if node_id > 0 the
		//           initiator was a processor (cache), or a coherent initiator (??) 
		// source_id : srcid field of the request, with a N-level NoC it could be anything
		int node_id = m_cct->translate_to_id(m_vci_fsm.currentSourceId());
		unsigned int source_id = m_vci_fsm.currentSourceId();
		unsigned int page_index = addr >> m_PAGE_SHIFT;
		int blocknum = (addr  >> m_ADDR_BLOCK_SHIFT);

		assert(seg == 0);

		DRAMCOUT(0) << name() << " [RAM_WRITE] " << endl;

		r_IN_TRANSACTION = !eop;

		if (s_POISONNED[page_index])
		{
			m_last_write_nack = ! eop;
			return false;
		}
#ifdef DEADLOCK_NACK_POLICY
		if (r_RAM_FSM != RAM_IDLE)
		{
			return false;
		}
#endif
		// a Write-burst is processed
		assert(!m_last_write_nack);
		assert(check_virtual_access(addr, node_id));

#ifdef DEBUG_SRAM
		m_should_get = rw_seg(s_RAM, (unsigned int)(addr/vci_param::B), data, be, vci_param::CMD_WRITE); // write data in S-RAM
#endif

		// Save request in all cases, and set correct values for
		// the S-Ram access (ce, be, we, oe, etc...)

		m_sram_addr = (addr/vci_param::B);
		m_sram_wdata = data;
		m_sram_be = be;
		m_sram_oe = false;
		m_sram_we = true;
		m_sram_ce = true;
		m_sram_bk = seg;

		if (((node_id == -1) && !s_DIRECTORY[blocknum].Is_empty())
				|| ((node_id != -1) && s_DIRECTORY[blocknum].Is_Other(node_id)))
		{
			// Send invalidation only if it is the last cell of the paquet
			// in order to avoid deadlocks, report to TODO point for an explanation 
			r_RAM_FSM = RAM_DATA_INVAL;
			r_SAV_ADDRESS = m_vci_fsm.getBase(seg)+addr;
			r_SAV_SEGNUM = seg;
			r_SAV_BLOCKNUM = blocknum;
			r_SAV_SCRID = source_id;
		}

		m_counters_page_sel = page_index;
		m_counters_cost++; // TODO, dynamic cost, for instance is paquet lenght!	
		m_counters_enable = eop;	
		m_counters_node_id = node_id;

		return true;
	}

	/***********************************************************************************************/
	/* Called by target_fsm on a VCI read request, return NACK if the page is poisonned,           */
	/* update the directory if the requestor is a cache.                                           */
	/***********************************************************************************************/
	tmpl(bool)::on_read(size_t seg, typename vci_param::addr_t addr, typename vci_param::data_t &data, bool eop)
	{
		// node_id : in [0 - nb_coherent_initiators_on_the_platform[, if node_id == -1 the
		//           initiator was a processor (cache), or a coherent initiator (??) 
		// source_id : srcid field of the request, with a N-level NoC it could be anything
		int node_id = m_cct->translate_to_id(m_vci_fsm.currentSourceId());
		unsigned int source_id = m_vci_fsm.currentSourceId();
		unsigned int page_index = addr >> m_PAGE_SHIFT;
		int blocknum = (addr  >> m_ADDR_BLOCK_SHIFT);

		assert(seg == 0);
		assert(check_virtual_access(addr, node_id));

		r_IN_TRANSACTION = !eop;
		if (s_POISONNED[page_index])
		{ 
			m_last_write_nack = !eop;
			return false;
		}
		assert(!m_last_write_nack);

#ifdef DEBUG_SRAM
		m_should_get  = s_RAM[addr/vci_param::B];
		data = m_should_get;
#endif
		// Save request in all cases, and set correct values for
		// the S-Ram access (ce, be, we, oe, etc...)

		m_sram_addr = (addr/vci_param::B);
		m_sram_oe = true;
		m_sram_we = false;
		m_sram_ce = true;
		m_sram_bk = seg;
		
		m_counters_page_sel = page_index;
		m_counters_cost++; // TODO, dynamic cost, for instance is paquet lenght!	
		m_counters_enable = eop;	
		m_counters_node_id = node_id;

		m_fsm_data = &data; // save the data location pointer, will be set in genMealy function.

		if (node_id == -1) return true; // the initiator does not support coherence (ex. fd_access).
		assert((unsigned int)node_id < m_NB_PROCS); // if not, an initiator which is not a processor but support coherence has sent
																	// this request.
		if ((s_DIRECTORY[blocknum].Is_p(node_id)==false) &&  eop) // Update the directory
		{
			r_RAM_FSM = RAM_DIRUPD;
			r_SAV_ADDRESS = m_vci_fsm.getBase(seg)+addr;
			r_SAV_SEGNUM = seg;
			r_SAV_BLOCKNUM = blocknum;
			r_SAV_SCRID = source_id;
		}

		return true;
	}

	/***********************************************************************************************/
	/* Called by target_fsm on a VCI tlb_miss request, return the new location address of this     */
  /* physical page frame, update page table directory.                                           */
	/***********************************************************************************************/
	tmpl(bool)::on_tlb_miss(size_t seg, typename vci_param::addr_t addr, typename vci_param::data_t &data )
	{
		unsigned int node_id = m_cct->translate_to_id(m_vci_fsm.currentSourceId());
		unsigned int page_index = addr >> m_PAGE_SHIFT;
		DRAMCOUT(1) << name() << " [RAM_TLB_MISS] " << endl;
		assert(seg == 0);
		s_PT_DIRECTORY[page_index] . Set_p(node_id);
		data  = s_PAGE_TABLE -> translate(addr);
		return true;
	}

	/***********************************************************************************************/
	/* Called by target_fsm on a VCI tlb_ack request, when all ack will be received a command will */
	/* be sent to the external module (for page migration)  to inform that all the TLB's of the    */
  /* system are clean.                                                                           */
	/***********************************************************************************************/
	tmpl(bool)::on_tlb_ack(size_t seg, typename vci_param::addr_t full_addr)
	{
		unsigned int srcid = m_cct->translate_to_id(m_vci_fsm.currentSourceId());
		DRAMCOUT(3) << name() << " (> RAM_TLB_ACK for  " << std::dec << srcid <<  endl;
		assert(r_SAVE_PT_DIRECTORY . Is_p(srcid));
		assert(!r_IN_TRANSACTION);
		assert(seg == 0);
		r_SAVE_PT_DIRECTORY . Reset_p(srcid);
		r_RAM_FSM = RAM_TLB_INV_OK;
		return true;
	}
}}

