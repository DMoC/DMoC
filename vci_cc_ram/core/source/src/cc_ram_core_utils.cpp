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
#include <cassert>

#define tmpl(x) template<typename vci_param, typename sram_param> x CcRamCore<vci_param,sram_param>

namespace soclib {
namespace caba {

	using namespace soclib;


	/***********************************************************************************************/
	/* Called by target_fsm to know if the request can be processed (ack of incomming request),    */
  /* priority set to requests from external modules (related to page migration)                  */
	/***********************************************************************************************/
	tmpl(bool)::is_busy(void)
	{
#ifdef DEADLOCK_NACK_POLICY
		return (r_RAM_FSM != RAM_IDLE);
#else
		return false;
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
		int blocknum = (addr  >> m_ADDR_BLOCK_SHIFT);


		DRAMCOUT(0) << name() << " [RAM_WRITE] " << endl;
#if 0
		std::cout << "ram_utils, write :" << " seg " << std::dec << seg; 
		std::cout << " addr " << std::hex << addr; 
		std::cout << " be " << std::hex << be; 
		std::cout << " eop " << std::hex << eop; 
		std::cout << " wdata  " << std::hex << data << std::endl; 
#endif

		r_IN_TRANSACTION = !eop;

#ifdef DEADLOCK_NACK_POLICY
		if (r_RAM_FSM != RAM_IDLE)
		{
			return false;
		}
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

		if ((((node_id == -1) && !s_DIRECTORY[seg][blocknum].Is_empty())
				|| ((node_id != -1) && s_DIRECTORY[seg][blocknum].Is_Other(node_id)))
				&& eop)
		{
			// Send invalidation only if it is the last cell of the paquet
			// in order to avoid deadlocks, report to TODO point for an explanation 
			r_RAM_FSM = RAM_DATA_INVAL;
			r_SAV_ADDRESS = m_vci_fsm.getBase(seg)+addr;
			r_SAV_SEGNUM = seg;
			r_SAV_BLOCKNUM = blocknum;
			r_SAV_SCRID = source_id;
		}
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
		int blocknum = (addr  >> m_ADDR_BLOCK_SHIFT);


		r_IN_TRANSACTION = !eop;
#ifdef DEADLOCK_NACK_POLICY
		if (s_POISONNED[page_index])
		{ 
			m_last_write_nack = !eop;
			return false;
		}
#endif

		// Save request in all cases, and set correct values for
		// the S-Ram access (ce, be, we, oe, etc...)

		m_sram_addr = (addr/vci_param::B);
		m_sram_oe = true;
		m_sram_we = false;
		m_sram_ce = true;
		m_sram_bk = seg;
		

		m_fsm_data = &data; // save the data location pointer, will be set in genMealy function.

		if (node_id == -1) return true; // the initiator does not support coherence (ex. fd_access).
		assert((unsigned int)node_id < m_NB_PROCS); // if not, an initiator which is not a processor but support coherence has sent
																	// this request.
		if ((s_DIRECTORY[seg][blocknum].Is_p(node_id)==false) &&  eop) // Update the directory
		{
			r_RAM_FSM = RAM_DIRUPD;
			r_SAV_ADDRESS = m_vci_fsm.getBase(seg)+addr;
			r_SAV_SEGNUM = seg;
			r_SAV_BLOCKNUM = blocknum;
			r_SAV_SCRID = source_id;
		}
		return true;
	}
}}

