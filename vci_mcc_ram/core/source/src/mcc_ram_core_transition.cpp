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
#define tmpl(x) template<typename vci_param, typename sram_param> x MccRamCore<vci_param,sram_param>

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
			r_SAVE_PT_DIRECTORY.init(m_NB_PROCS);
			s_PAGE_TABLE->Reset();
			for (unsigned int j = 0; j < (m_segment.size()/PAGE_SIZE); j++)
			{
				s_POISONNED[j] = false;
			}
			r_PROCESSING_TLB_INV = false;
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
				//       Cache_Target_addr + 0x4  : TLB  invalidation
				//       Cache_Target_addr + 0x0  : DATA invalidation
				{
					// TODO check if we send invalidations for the requesting node
					int32_t targetid;
					if ((((ram_fsm_state_e)r_RAM_FSM.read() == RAM_DATA_INVAL_WAIT)) || 
							(((ram_fsm_state_e)r_RAM_FSM.read()  == RAM_TLB_INVAL))) 
					{ 
						targetid = r_INV_BLOCKSTATE.Get_next_id();
						assert(targetid>=0);										 // blockstate not empty
						assert((uint32_t)targetid < m_NB_PROCS); // just to be shure :-)
						r_INV_BLOCKSTATE.Reset_p(targetid);
						r_INV_TARGETADDRESS = m_MapTab_inv.getSegment(m_cct->translate_to_target(targetid)).baseAddress();
						r_INV_FSM = RAM_INV_REQ;

						if (((ram_fsm_state_e)r_RAM_FSM.read() == RAM_TLB_INVAL)) { r_INV_ADDR_OFFSET = 4; }
						else { r_INV_ADDR_OFFSET = 0; }
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
						// State change according to on_read/write/tlb_miss/tlb_ack functions.
						m_vci_fsm.transition();


						// If there is no vci request to process, or being processed, check for external
						// modules requests. 
				#ifndef NOCTRL
						if (!r_IN_TRANSACTION.read() && p_in_ctrl_req.read())
						{
							DRAMCOUT(3) << name() << "[CTRL REQUEST]  " << std::endl;
							DRAMCOUT(3) << name() << "> CMD   "     << std::hex << p_in_ctrl_cmd.read() << std::endl;
							DRAMCOUT(3) << name() << "> DATA_F1   " << std::hex << p_in_ctrl_data_0.read() << std::endl;
							DRAMCOUT(3) << name() << "> DATA_F2   " << std::hex << p_in_ctrl_data_1.read() << std::endl;

							switch(p_in_ctrl_cmd.read())
							{
								// TODO : acquiter la requête dans le moore de l'état atteint !
								// r_PAGE_TO_POISON devrait être plutot une adresse pour "unifier" la com.
								case MigControl::CTRL_POISON_REQ :
									m_vci_fsm.break_link(p_in_ctrl_data_0.read());
									r_TARGET_PAGE_ADDR  = p_in_ctrl_data_0.read();
									r_VIRT_POISON_PAGE = s_PAGE_TABLE -> wiam(p_in_ctrl_data_0.read() << m_PAGE_SHIFT); 
									assert(p_in_ctrl_rsp_ack.read());
									r_RAM_FSM = RAM_POISON;
									break;

								case MigControl::CTRL_UNPOISON_REQ :
									r_TARGET_PAGE_ADDR = p_in_ctrl_data_0.read(); 
									r_RAM_FSM = RAM_UNPOISON;
									break;

								case MigControl::CTRL_UPDT_PT_F1_REQ :
									if (r_PROCESSING_TLB_INV.read()) break; // Avoid overwritting some registers (r_INV_BLOCKADRESS for example)
									r_TARGET_PAGE_ADDR = p_in_ctrl_data_0.read(); 
									r_NEW_PAGE_ADDR = p_in_ctrl_data_1.read();
									r_RAM_FSM = RAM_PAGE_TABLE_DIR_SAVE;
									break;

								case MigControl::CTRL_UPDT_PT_F2_REQ :
									r_TARGET_PAGE_ADDR = p_in_ctrl_data_0.read(); 
									r_NEW_PAGE_ADDR = p_in_ctrl_data_1.read();
									r_RAM_FSM = RAM_WIAM_UPDT;
									break;
							}
						}
#endif
						break;
					}

				case RAM_WIAM_UPDT :
						// > Update the Who_I_AM entry of the page table, and go to idle
					{
						DRAMCOUT(3) << name() << " [RAM_WIAM_UPDT] " << endl;
						DRAMCOUT(2) << name() << " WIAM : r_TARGET_PAGE_ADDR <- 0x" << hex << r_TARGET_PAGE_ADDR << " - 0x" << r_NEW_PAGE_ADDR <<  endl;
						s_PAGE_TABLE -> update_wiam ( r_TARGET_PAGE_ADDR.read()  , (r_NEW_PAGE_ADDR.read() >> m_PAGE_SHIFT));
						r_RAM_FSM = RAM_IDLE;
						break;
					}

				case RAM_PAGE_TABLE_DIR_SAVE :
						// > Save Page table directory entry before updating it
            //   in the next FSM state.
					{
						DRAMCOUT(3) << name() << " [RAM_PAGE_TABLE_DIR_SAVE] " << endl;
						DRAMCOUT(3) << name() << " page to invalidate " << hex << r_TARGET_PAGE_ADDR.read()  << endl;
#ifndef NOCTRL
						assert(p_in_ctrl_cmd.read() == MigControl::CTRL_UPDT_PT_F1_REQ);
#endif
						assert(check_page_access(r_TARGET_PAGE_ADDR.read()));
						r_SAVE_PT_DIRECTORY = s_PT_DIRECTORY[ (r_TARGET_PAGE_ADDR.read() - m_segment.baseAddress()) >> m_PAGE_SHIFT ];
						r_RAM_FSM = RAM_PAGE_TABLE_DIR_UPDT;
						break;
					}

				case  RAM_PAGE_TABLE_DIR_UPDT :
					{
						// > Update the page table record according to the new location of a page
						// > Reset the presence-bit flag (ie. PT_DIRECTORY entry for this page)
						// r_TAGET_PAGE_ADDR : page adress concerned
						// r_NEW_PAGE_ADDR   : new location for the page
						DRAMCOUT(3) << name() << " [RAM_PAGE_TABLE_DIR_UPDT] " << endl;
						DRAMCOUT(3) << name() << " TLB_DIR_UPDT : r_TARGET_PAGE_ADDR <- 0x" << hex << r_TARGET_PAGE_ADDR << " - 0x" << r_NEW_PAGE_ADDR <<  endl;
						s_PAGE_TABLE -> update_virt (((r_TARGET_PAGE_ADDR - m_segment.baseAddress()) >> m_PAGE_SHIFT), (r_NEW_PAGE_ADDR >> m_PAGE_SHIFT), false); 
						if (!r_SAVE_PT_DIRECTORY.Is_empty()) // Invalidations must be sent
						{
								r_RAM_FSM = RAM_TLB_INVAL;
						}
						else // Nothing to do, go to inform control module that the uptate of page table was done
						{
								r_RAM_FSM = RAM_TLB_INV_OK;
						}
						break;
					}
				break;

				case RAM_TLB_INVAL :
						// > Send invalidation requests for TLB's. 
						// > This state awakes de r_INV_FSM.
							if ((inv_fsm_state_e)r_INV_FSM.read()==RAM_INV_IDLE)  
							{
								r_PROCESSING_TLB_INV = true;
								r_INV_BLOCKSTATE = r_SAVE_PT_DIRECTORY;
								assert(false);
								r_INV_BLOCKSTATE.Print();
								r_INV_BLOCKADDRESS = (r_TARGET_PAGE_ADDR.read() >> m_PAGE_SHIFT); 
								r_RAM_FSM = RAM_IDLE;
							}
				break;

				case RAM_TLB_INV_OK :
						// > Check if all the ack's for the TLB's invalidations have been
						// > received, if it is the case, inform the control module.
						DRAMCOUT(3) << name() << " [RAM_TLB_INV_OK] " << endl;
						// TODO check for possible deadlock
						if (r_SAVE_PT_DIRECTORY.Is_empty())
						{
#ifndef NOCTRL
							if (p_in_ctrl_rsp_ack.read())
							{
								r_PROCESSING_TLB_INV = false;
								r_RAM_FSM = RAM_IDLE;
							}
#endif
						}
						else
						{
							r_RAM_FSM = RAM_IDLE;
						}
				break;

				// TODO ////////////////////////////////////////////////////////////////////////////////////////////////////
				// To go to this state, we can wait for the RAM_FSM to be in idle state and check for a poison command. This
				// will not add any deadlock beacause the RAM_FSM will alway go to idle, one day or another.
				// What may prevent this : 
				// 	- response fifo full : since a response is alway taken into account by the initiator, it will never
				// 	  be full forever
				// 	- wait for invalidation responses : invalidations are prehemptive on cache, so they are always taken
				// 	  into account.
				////////////////////////////////////////////////////////////////////////////////////////////////////////////


				case RAM_POISON :
					// > Poison the page, accesses to this page will now be negatively-acknowledged (NACK).
          //   because the page is being migrated
				case RAM_UNPOISON :
					// > Unpoison the page 
					assert((r_TARGET_PAGE_ADDR >> m_PAGE_SHIFT) < (m_segment.size()/PAGE_SIZE));
					// if POISON_REQ set true, else if UNPOISON_REQ set false
#ifndef NOCTRL
					s_POISONNED[(r_TARGET_PAGE_ADDR >> m_PAGE_SHIFT)] = (p_in_ctrl_cmd.read() == MigControl::CTRL_POISON_REQ);
#endif
					r_RAM_FSM = RAM_IDLE;
				break;

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
						addr_t wiam_addr = s_PAGE_TABLE -> wiam(r_SAV_ADDRESS.read() - m_segment.baseAddress());

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
							assert((wiam_addr & (PAGE_SIZE -1)) == 0);
							r_INV_BLOCKADDRESS = (wiam_addr | (r_SAV_ADDRESS.read() & (PAGE_SIZE - 1) & m_BLOCKMASK)); 

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
