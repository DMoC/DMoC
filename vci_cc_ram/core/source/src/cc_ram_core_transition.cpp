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


namespace soclib {
namespace caba {
#define tmpl(x) template<typename vci_param, typename sram_param> x CcRamCore<vci_param,sram_param>

	/***********************************************************************/
	/* Transition :                                                        */
	/* Called by system-C/CASS engine on rising clock edge.                */
  /* It computes the new state (registers/memory values) according to    */
  /* actual state and entries                                            */
  /***********************************************************************/

	tmpl(void)::transition()
	{
		m_nbcycles++;

		if (!p_resetn.read()) // Reset_N signal
		{
			m_vci_fsm.reset();
			r_RAM_FSM = RAM_IDLE;
			r_INV_FSM = RAM_INV_IDLE;
			r_INV_BLOCKSTATE.init(m_NB_PROCS);
			r_IN_TRANSACTION = false;
#ifdef DEBUG_SRAM
			reload();
#endif
			return;
		} 

		/*  Compute INV_FSM */
		switch (r_INV_FSM.read()) {

			case RAM_INV_IDLE :  // Idle, wait for a request (ie. RAM_FSM on a specific state) 
				// Requests are sent to :
				//       Cache_Target_addr + 0x0  : DATA invalidation
				{
					// TODO check if we send invalidations for the requesting node
					int32_t targetid;
					if (((ram_fsm_state_e)r_RAM_FSM.read() == RAM_DATA_INVAL_WAIT))
					{ 
						targetid = r_INV_BLOCKSTATE.Get_next_id();
						assert(targetid>=0);										 // blockstate not empty
						assert((uint32_t)targetid < m_NB_PROCS); // just to be shure :-)
						r_INV_BLOCKSTATE.Reset_p(targetid);
						r_INV_TARGETADDRESS = m_MapTab_inv.getSegment(m_cct->translate_to_target(targetid)).baseAddress();
						r_INV_FSM = RAM_INV_REQ;
					}
					break;
				}

			case RAM_INV_REQ :
				if (p_i_vci.cmdack.read())
				{
					r_INV_FSM = RAM_INV_RSP;
				}
				break;

			case RAM_INV_RSP :
				{
					int32_t targetid; 
					if (!(p_i_vci.rspval.read())) break;
					if (!r_INV_BLOCKSTATE.Is_empty())
					{
						targetid = r_INV_BLOCKSTATE.Get_next_id();
						assert((targetid>=0) && ((uint32_t)targetid < m_NB_PROCS));
						r_INV_BLOCKSTATE.Reset_p(targetid);
						r_INV_TARGETADDRESS = m_MapTab_inv.getSegment(m_cct->translate_to_target(targetid)).baseAddress();
						r_INV_FSM = RAM_INV_REQ;
					}
					else
					{
						r_INV_FSM = RAM_INV_IDLE;
					}
					break;
				}

			default :
				assert(false);
				break;

		}

		/*  Compute RAM_FSM */
			switch (r_RAM_FSM.read())
			{
				case RAM_IDLE :
					{
						DRAMCOUT(0) << name() << " [RAM_IDLE] " << endl;
						// Process and issue requests from VCI interconnect,
						// State change according to on_read/write functions.
						m_vci_fsm.transition();
						break;
					}

#if 0
			// TODO : handle soft-ware access to the counters
			// for an OS-control. Check module 7 draft
#endif

				case RAM_DIRUPD :
						// > Update the directory, set the presence bit indexed by the processor id.
					{
						unsigned int blocknum;
						DRAMCOUT(0) << name() << " [RAM_DIRUPDT] " << endl;
						blocknum = (((r_SAV_ADDRESS.read() - m_segment.baseAddress()) & m_BLOCKMASK) >> m_ADDR_BLOCK_SHIFT);
						s_DIRECTORY[blocknum].Set_p(m_cct->translate_to_id(r_SAV_SCRID.read()));
						r_RAM_FSM = RAM_IDLE;
						break;
					}

				case RAM_DATA_INVAL : // and directory update
					{
						DRAMCOUT(1) << name() << " [RAM_DATA_INVAL] " << endl;
						unsigned int blocknum = (((r_SAV_ADDRESS.read() - m_segment.baseAddress()) & m_BLOCKMASK) >> m_ADDR_BLOCK_SHIFT);
						bool save_p = s_DIRECTORY[blocknum].Is_p(m_cct->translate_to_id(r_SAV_SCRID.read()));

						if((inv_fsm_state_e)r_INV_FSM.read() == RAM_INV_IDLE)
						{ // We stay heere until the INV_FSM is not free. This will not generate a deadlock
							// because none of the request sent by this fsm requires that the CACHE fsm waits for a memory node. This is the case
							// because cache invalidations are preemptive and tlb invalidation can respond immediately.

							// Set the INV_BLOCKSTATE used to invalidate the OTHER copies of this block (disable source id flag).
							r_INV_BLOCKSTATE = s_DIRECTORY[blocknum];
							r_INV_BLOCKSTATE.Reset_p(m_cct->translate_to_id(r_SAV_SCRID.read()));
							assert(!r_INV_BLOCKSTATE.Is_empty());

							// Reset the Directory with the exception of source id flag, could be done with a XOR and a shifter	
							s_DIRECTORY[blocknum].Reset_all();
							if (save_p){
								s_DIRECTORY[blocknum].Set_p(m_cct->translate_to_id(r_SAV_SCRID.read())); 
							}

							// Set the address of block to be invalidated, it depends of the address of the page
							// contained in this frame (ie. Who-I-Am address of the page accessed by the write request).
							r_INV_BLOCKADDRESS = (r_SAV_ADDRESS.read() & m_BLOCKMASK); 

							// Go and Wait for acknowledge of invalidations in order to guarantee coherency.
							r_RAM_FSM = RAM_DATA_INVAL_WAIT;
						}
						break;
					}

				case RAM_DATA_INVAL_WAIT :
					DRAMCOUT(0) << name() << " [RAM_DATA_INVAL_WAIT] " << endl;
					if ((inv_fsm_state_e)r_INV_FSM.read() == RAM_INV_IDLE)
					{ 
							r_RAM_FSM = RAM_IDLE; 
					}
					break;

				default : 
					assert(false); // unknown state
					break;

			} // end switch r_RAM_FSM

	}; 
}}
