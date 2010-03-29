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
 * 	   Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2006-2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

///////////////////////////////////////
//  Transition method
///////////////////////////////////////
#include "vci_cc_cache.h"

namespace soclib { 
namespace caba {
#define tmpl(x)  template<typename vci_param, typename iss_t> x VciCcCache<vci_param, iss_t>
tmpl(void)::transition()
{

	typename iss_t::InstructionRequest icache_req_port = ISS_IREQ_INITIALIZER;
	typename iss_t::InstructionResponse icache_rsp_port = ISS_IRSP_INITIALIZER;
	typename iss_t::DataRequest dcache_req_port = ISS_DREQ_INITIALIZER;
	typename iss_t::DataResponse dcache_rsp_port = ISS_DRSP_INITIALIZER;

	m_iss.getRequests( icache_req_port, dcache_req_port );


	// Invalidations
	const typename vci_param::addr_t		dcache_inv_address = r_RAM_INV_ADDR;
	const unsigned int									dcache_inv_y = m_d_y[dcache_inv_address] ;
	const typename vci_param::addr_t	 	dcache_inv_z = m_d_z[dcache_inv_address] | V_BIT ;
	const bool		dcache_ram_inval = r_DCACHE_RAM_INVAL_REQ;

	// control the write buffer
	bool		write_buffer_put = false;
	bool		write_buffer_get = false;

	ncycles++;

	//////////////////////////////
	//        RESET
	//////////////////////////////
	if (p_resetn == false) { 
		m_iss.reset(); // reset also the target_fsm
		r_DCACHE_FSM = DCACHE_INIT; 
		r_ICACHE_FSM = ICACHE_INIT; 
		r_VCI_REQ_FSM = REQ_IDLE;
		r_VCI_RSP_FSM = RSP_IDLE;
		r_VCI_INV_FSM = INV_IDLE;

		r_ICACHE_MISS_REQ = false;

		r_DATA_FIFO_INPUT = 0; // is not necessary
		r_ADDR_FIFO_INPUT = 0; // is not necessary

		r_ICACHE_CPT_INIT = s_icache_lines - 1;
		r_DCACHE_CPT_INIT = s_dcache_lines - 1;
		r_DCACHE_RAM_INVAL_REQ = false;

		r_DCACHE_REQ = false;
		r_DCACHE_PENDING_WRITE = false;
		r_DCACHE_FLUSH_WB_REQ  = false;

		m_WRITE_BUFFER_DATA.init();
		m_WRITE_BUFFER_ADDR.init();
		r_DRETRY_REQ = false;
		r_IRETRY_REQ = false;

		r_DRSP_OK = false;
		r_IRSP_OK = false;
		return;
	} 

	// Icache FSM : controls Instruction-cache interface from processor.
	// It works as follow : 
  // 1) check that there is a request,
  // 2) a hit in cache will give the value in the same cycle, a miss
  //    will stall the processor waiting for a response from memory (ICACHE_WAIT) state. 
	// 3) When the response is here, come back to IDLE, in the next cycle the request will be
	//    a hit in cache :-).

	// Regarding requests : request are managed by the REQ/RSP_FSM which are trigerred by
	// some signals  ICACHE_REQ , when request has been completed and
	// the response received the IRSP_OK signal will be set unlocking (from ICACHE_WAIT state) the
	// ICACHE_FSM. The response will be available in a buffer.

	// Regarding invalidations : Instruction cache cannot be invalidated, we suppose that there is
	// no-self modifying code. In such cache, the processor should call a CPU_INVAL request (not supported yet,
	// but easy to implement). 
	switch((icache_fsm_state_e)r_ICACHE_FSM.read())
	{
		case ICACHE_INIT : // Init, reset TAG at reset time
			s_ICACHE_TAG[r_ICACHE_CPT_INIT] = 0;
			r_ICACHE_CPT_INIT = r_ICACHE_CPT_INIT - 1;
			if (r_ICACHE_CPT_INIT == 0) { r_ICACHE_FSM = ICACHE_IDLE; }  
			break;

		case ICACHE_IDLE :
			{
				// Instruction cache requests
				typename vci_param::addr_t		icache_address = icache_req_port.addr;
				unsigned int	icache_y = m_i_y[icache_address];
				unsigned int	icache_x = m_i_x[icache_address];
				typename vci_param::addr_t		icache_z = m_i_z[icache_address] | V_BIT;
				bool		icache_hit = (icache_z == s_ICACHE_TAG[icache_y].read());
				bool		icache_req = icache_req_port.valid;

				if (!icache_req) break; // nothing to do
				if (icache_hit) // the common case, return the instruction in the SAME cycle,
					// this is the case beacause m_iss.executeNCycles is called AFTER 
					// setting the values hereafter (.valid, and .instruction).
					{
						assert(icache_req);
						icache_rsp_port.valid          = icache_hit;
						icache_rsp_port.instruction    = s_ICACHE_DATA[icache_y][icache_x].read();
						if ((s_ICACHE_DATA[icache_y][icache_x].read() == 0xFEED0BAD))
						{
							std::cout << name() << "WARNING : processor reads an instruction  that it was probably not initialized," << std::endl;
							std::cout << name() << "          it contained the magic 0xFEED0BAD set at allocation time of S-ram memory." << std::endl;
						}
					}
				else // miss, issue a miss request and wait for the response
				{
					r_ICACHE_MISS_ADDR_SAVE = icache_address;
					r_ICACHE_FSM = ICACHE_WAIT;
					r_ICACHE_MISS_REQ = true;
				}
			}
			break;

		case ICACHE_WAIT : // wait until the response arrives
			if (r_IRSP_OK)
			{
				r_ICACHE_FSM = ICACHE_UPDT;
			}
			break;

		case ICACHE_UPDT :
			{
				unsigned int	icache_y = m_i_y[r_ICACHE_MISS_ADDR_SAVE.read()];
				typename vci_param::addr_t		icache_z = m_i_z[r_ICACHE_MISS_ADDR_SAVE.read()] | V_BIT;

				s_ICACHE_TAG[icache_y] = icache_z;
				for (size_t i=0 ; i<s_icache_words ; i++) // Fill up the memory line
				{
					s_ICACHE_DATA[icache_y][i] = r_RSP_ICACHE_MISS_BUF[i];
				}
				r_ICACHE_FSM = ICACHE_IDLE; // Go to idle, the next cycle the request will hit in cache 
			}
			break;

		default : // an impossible case
			assert(false);
			break;
	} // end switch ICACHE_FSM

	// Dcache FSM : controls data-cache interface from processor.
	// It works as follow : 
  // 1) check that there is a request,
  // 2) if the Write-buffer is NOT empty, check if the request
	//    is a write that is contiguous with the previous write-request
	//    in order to create a "write burst", if yes, enqueue, if not, flush the buffer.
	// 3) the Write-buffer is EMPTY (recently flushed or not), process the request.
  // 4) a hit will give the value in the same cycle, a miss or uncached request
  //    will stall the processor waiting for a response from memory (*_DWAIT) state. 
	// 5) not wating anymore, give the value (if requested) to the processor.


	// Regarding requests : request are managed by the REQ/RSP_FSM which are trigerred by
	// some signals  DCACHE_REQ and DCACHE_FLUSH_WB_REQ, when request has been completed and
	// the response received the DRSP_OK signal will be set unlocking (from *_WAIT state) the
	// DCACHE_FSM. The response will be available in a buffer (depends on the request).


  // Regarding invalidations : invalidations are received on the target interface
	// (managed by the INV_FSM). A signal is raised (DCACHE_RAM_INVAL_REQ), and we take it into
	// account as soon as possible in IDLE state and when there is nothing to do in *_WAIT states.
  // This is done in this way to avoid deadlocks (INV_FSM waits until invalidation has been processed
	// for sending the response and thus ensure coherency).

	switch ((dcache_fsm_state_e)r_DCACHE_FSM.read())
	{
		case DCACHE_INIT :
			s_DCACHE_TAG[r_DCACHE_CPT_INIT] = 0;
			r_DCACHE_CPT_INIT = r_DCACHE_CPT_INIT - 1;
			if (r_DCACHE_CPT_INIT == 0) { r_DCACHE_FSM = DCACHE_IDLE; }  
			break;

		case DCACHE_IDLE : 
			{
				// Data cache requests
				typename vci_param::addr_t		dcache_address  = dcache_req_port.addr;
				typename vci_param::data_t		dcache_wdata  = dcache_req_port.wdata;
				unsigned int	dcache_x = m_d_x[dcache_address];
				unsigned int	dcache_y = m_d_y[dcache_address];
				typename vci_param::addr_t		dcache_z = m_d_z[dcache_address] | V_BIT;
				bool		dcache_hit = (dcache_z == s_DCACHE_TAG[dcache_y].read());
				bool		dcache_req = dcache_req_port.valid;
				typename iss_t::be_t dcache_be = dcache_req_port.be;
				int dcache_type = dcache_req_port.type;

				bool dr_cached;
				bool dr_ll = false;
				bool dr_sc = false;
				typename vci_param::data_t mask = be_to_mask((typename iss_t::be_t) dcache_be);

				if (dcache_ram_inval)  // There is an invalidation and has priority over other requets
				{
					r_DCACHE_FSM = DCACHE_RAMINVAL;
					r_DCACHE_RAM_INVAL_REQ = false;
					r_PREV_DCACHE_FSM = DCACHE_IDLE; 
					break;
				}	

				if (!dcache_req) // No request on the interface, break
				{

#if 0
					// Flushing write buffer when there is nothing to do seems to degrade performances
					// (more traffic on the noc ?)
					if (m_WRITE_BUFFER_DATA.rok() && (!r_DCACHE_FLUSH_WB_REQ))
					{
						assert(r_DCACHE_PENDING_WRITE.read()==true);
						assert(r_DCACHE_FLUSH_WB_REQ.read()==false);
						r_DCACHE_FLUSH_WB_REQ = true;
						r_DCACHE_FIRST_CELL = false;
						write_buffer_put = true;
						r_WRITE_BURST_SIZE = m_WRITE_BUFFER_DATA.filled_status() + 1; 
					}
#endif
					break;
				}

				// So, there is a request, .. 
				dr_cached = m_cacheability_table[dcache_address]; // to a cached or uncached segment ?

				// > Manage WRITE-BUFFER
				if (r_DCACHE_PENDING_WRITE.read() && !r_DCACHE_FLUSH_WB_REQ.read())
					// A pending write is there, if the next request is a write on a
					// consecutive address enqueue the request, else flush the buffer and wait for completion. 
				{
					if (r_DCACHE_WL_BE_SAVE.read() == (m_full_be)) // r_DATA_FIFO_INPUT is full, enqueue it and check if a flush is required
					{
						write_buffer_put = true; 
						r_DCACHE_FIRST_CELL = false;

						if (((dcache_address & m_dcache_zmask) == r_DCACHE_ENQUEUED_ADDR_SAVE.read() + vci_param::B)
								&& (dcache_be & 0x1) // a burst paquet MUST have (vci standard) contiguous byte-enable over cells : ex 001 1111 1111 1110 
								&& (dcache_type == iss_t::DATA_WRITE )
								&& (m_WRITE_BUFFER_DATA.filled_status() != s_write_buffer_size - 1)
								&& ((dcache_address & (PAGE_SIZE-1)) != 0))
							// consecutive word address, write request , same page and fifo not full
							// update last enqueued address, be of current DATA_FIFO_INPUT, and DATA/ADDR_FIFO_INPUT
						{
							r_DCACHE_ENQUEUED_ADDR_SAVE = dcache_address & m_dcache_zmask;
							r_DCACHE_WL_BE_SAVE = dcache_be; 
							r_DATA_FIFO_INPUT = dcache_wdata; 
							r_ADDR_FIFO_INPUT = dcache_address & m_dcache_zmask; 
							dcache_rsp_port.valid = true;
							dcache_rsp_port.rdata = 0;
							if (dcache_hit) // On hit, going update the line
							{ 
								r_DCACHE_FSM = DCACHE_WUPDT;
								r_DCACHE_WUPDT_DATA_SAVE = s_DCACHE_DATA[dcache_y][dcache_x].read();
							}	
						}
						else // flush write-buffer. 
						{
							r_DCACHE_FLUSH_WB_REQ = true;
							r_WRITE_BURST_SIZE = m_WRITE_BUFFER_DATA.filled_status() + 1; // could be implemented with a little 4bits adder
							assert(m_WRITE_BUFFER_DATA.wok());
						}
					}
					else // DATA_FIFO not full, merge if possible new write request in this cell, or flush the write-buffer
					{
						if (((dcache_address & m_dcache_zmask) == r_DCACHE_ENQUEUED_ADDR_SAVE.read()) // Address of a byte in the same address word
								&& (dcache_type == iss_t::DATA_WRITE )
								&& (ffs((uint32_t)dcache_be) == (soclib::common::fls((uint32_t)r_DCACHE_WL_BE_SAVE.read()) + 1))) // consecutive be's  
							// merge the next data in the same cell
						{
							// update DATA_FIFO_INPUT and current be (DCACHE_WL_BE_SAVE)
							r_DCACHE_WL_BE_SAVE = r_DCACHE_WL_BE_SAVE.read() | dcache_be; 
							r_DATA_FIFO_INPUT = (r_DATA_FIFO_INPUT.read() & ~(mask)) | (dcache_wdata & mask);
							dcache_rsp_port.valid = true;
							dcache_rsp_port.rdata = 0;
							if (r_DCACHE_FIRST_CELL.read())
								// for first cell of a burst, update the WF_BE in order to compute a correct plen.
							{
								r_DCACHE_WF_BE_SAVE = r_DCACHE_WF_BE_SAVE.read() | dcache_be;
							}
							if (dcache_hit) // On hit, going update the line
							{
								r_DCACHE_FSM = DCACHE_WUPDT;
								r_DCACHE_WUPDT_DATA_SAVE = s_DCACHE_DATA[dcache_y][dcache_x].read();
							}	
						}
						else // Can not merge the next request, enqueue "as it" ADDR/DATA_FIFO_INPUT and flush write buffer
						{
							write_buffer_put = true;
							r_DCACHE_FIRST_CELL = false;
							r_DCACHE_FLUSH_WB_REQ = true;
							r_WRITE_BURST_SIZE = m_WRITE_BUFFER_DATA.filled_status() + 1; // could be implemented with a
							// little (4bits? -> 16 words) full adder
							assert(m_WRITE_BUFFER_DATA.wok());
						}
					}
				} 
				else if (!r_DCACHE_PENDING_WRITE.read()) // > No pending write, we can issue a new request, 
				{
					switch(dcache_type)
						{
						case iss_t::DATA_READ:
							if ( dr_cached ) // Cached request, check for cache hit/miss
							{
								if ( ! dcache_hit ) // miss, need to retrieve data
								{
									r_DCACHE_FSM = DCACHE_MISSWAIT;
									s_DCACHE_TAG[dcache_y] = dcache_z;
									r_DCACHE_REQ = true;
									// Set tag in this state and not in miss update to
									// avoid race conditions with invalidations
								}
								else   // hit, going to idle, deliver the data to the processor
								{
									r_DCACHE_FSM = DCACHE_IDLE;
									dcache_rsp_port.valid = true;
									dcache_rsp_port.rdata = s_DCACHE_DATA[dcache_y][dcache_x].read();
									if((s_DCACHE_DATA[dcache_y][dcache_x].read() == 0xFEED0BAD))
									{
											std::cout << name() << "WARNING : processor reads a data that it was probably not initialized," << std::endl;
											std::cout << name() << "          it contained the magic 0xFEED0BAD set at allocation time of S-ram memory." << std::endl;
									}
								}
							}
							else // Issue an unached request to memory
							{
								r_DCACHE_FSM = DCACHE_UNC_LL_SC_WAIT;
								r_DCACHE_REQ = true;
							}
							break;

						case iss_t::DATA_SC: // Store conditionnal (is an uncached request)
							dr_sc = true;
							r_DCACHE_FSM = DCACHE_UNC_LL_SC_WAIT;
							r_DCACHE_REQ = true;
							break;

						case iss_t::DATA_LL: // Load Linked (is an uncached request)
							dr_ll = true;
							r_DCACHE_FSM = DCACHE_UNC_LL_SC_WAIT;
							r_DCACHE_REQ = true;
							break;

						case iss_t::DATA_WRITE: // First write of a (possible) burst,  
							assert(m_WRITE_BUFFER_DATA.wok());  // No pending write, so empty fifo
							assert(!m_WRITE_BUFFER_DATA.rok()); // No pending write, so empty fifo
							r_DCACHE_PENDING_WRITE = true;
							if (dcache_hit) // On hit, going update the line
							{
								r_DCACHE_FSM = DCACHE_WUPDT;
								r_DCACHE_WUPDT_DATA_SAVE = s_DCACHE_DATA[dcache_y][dcache_x];
							}	
							r_DCACHE_ENQUEUED_ADDR_SAVE = dcache_address & m_dcache_zmask;
							r_DATA_FIFO_INPUT = dcache_wdata;
							r_ADDR_FIFO_INPUT = dcache_address & m_dcache_zmask; 
							r_DCACHE_WF_BE_SAVE = dcache_be;
							r_DCACHE_WL_BE_SAVE = dcache_be;
							r_DCACHE_FIRST_CELL = true;
							dcache_rsp_port.valid = true;
							dcache_rsp_port.rdata = 0;
							break;

						case iss_t::XTN_READ :
						case iss_t::XTN_WRITE :
							switch (dcache_req_port.addr/4)
							{
								case iss_t::XTN_SYNC : // Cache Sync (flush) request, being here guaranties that the write-buffer is empty :-)
									dcache_rsp_port.valid = true;
									dcache_rsp_port.rdata = 0;
									break;
                case iss_t::XTN_DCACHE_INVAL : // Cache invalidate
									dcache_rsp_port.valid = true;
									dcache_rsp_port.rdata = 0;
                  if (dcache_hit) r_DCACHE_FSM = DCACHE_CPUINVAL; 
                  break;
								default : 
									assert(false);
									break;
							}
							break;
						default :
							assert(false);
							//unsuported commands:
							break;
					}
				}
				// Saving the request
				r_DCACHE_ADDR_SAVE = dcache_address;
				r_DCACHE_DATA_SAVE = dcache_wdata;

				r_DCACHE_BE_SAVE = dcache_be;
				r_DCACHE_UNCACHED_SAVE = !(dr_cached);
				r_DCACHE_LL_SAVE = dr_ll;
				r_DCACHE_SC_SAVE = dr_sc;
				break;
			}

		case DCACHE_WUPDT:
			{
				typename vci_param::addr_t req_addr = r_DCACHE_ADDR_SAVE.read();
				typename vci_param::data_t req_data = r_DCACHE_DATA_SAVE.read();
				typename vci_param::data_t req_prev = r_DCACHE_WUPDT_DATA_SAVE.read();
				typename vci_param::data_t mask = be_to_mask((typename iss_t::be_t) r_DCACHE_BE_SAVE.read());
				const unsigned int x = m_d_x[req_addr];
				const unsigned int y = m_d_y[req_addr];
				s_DCACHE_DATA[y][x] = (mask & req_data) | (~mask & req_prev);
				// We are now in PENDING_WRITE_MODE, going to idle.
				// The response for the processor has been
				// sent in the previous clock cycle
				r_DCACHE_FSM = DCACHE_IDLE;
				break;
			}     


		case DCACHE_MISSWAIT :
			if (r_DRSP_OK) { r_DCACHE_FSM = DCACHE_MISSUPDT; } // cache line received go to update
			else if (dcache_ram_inval) // else, check for pending invalidation request
			{
				r_DCACHE_FSM = DCACHE_RAMINVAL;
				r_PREV_DCACHE_FSM = DCACHE_MISSWAIT;
				r_DCACHE_RAM_INVAL_REQ = false; 
			} 
			break;

		case DCACHE_MISSUPDT :  // Copy cache line from buffer to cache memory, give requested data to the processor
			{
				const unsigned int y = m_d_y[r_DCACHE_ADDR_SAVE.read()];
				const unsigned int x = m_d_x[r_DCACHE_ADDR_SAVE.read()];
				for (size_t i=0 ; i < s_dcache_words ; i++)
				{
					s_DCACHE_DATA[y][i] = r_RSP_DCACHE_MISS_BUF[i];
				}
				dcache_rsp_port.valid = true;
				dcache_rsp_port.rdata = r_RSP_DCACHE_MISS_BUF[x].read();
				if((r_RSP_DCACHE_MISS_BUF[x].read() == 0xFEED0BAD))
				{
					std::cout << name() << "WARNING : processor reads a data that it was probably not initialized," << std::endl;
					std::cout << name() << "          it contained the magic 0xFEED0BAD set at allocation time of S-ram memory." << std::endl;
				}
				r_DCACHE_FSM = DCACHE_IDLE;
				break;
			}

		case DCACHE_UNC_LL_SC_WAIT : 
			if (r_DRSP_OK) { r_DCACHE_FSM = DCACHE_UNC_LL_SC_GO; } // response received, proceed
			else if (dcache_ram_inval) // else, check for pending invalidation request
			{
				r_DCACHE_FSM = DCACHE_RAMINVAL;
				r_PREV_DCACHE_FSM = DCACHE_UNC_LL_SC_WAIT;
				r_DCACHE_RAM_INVAL_REQ = false;
			} 
			break;

		case DCACHE_UNC_LL_SC_GO : 
			{
				// invalidating the line to avoid consistency problems
				const int y = m_d_y[r_DCACHE_ADDR_SAVE.read()];
				s_DCACHE_TAG[y] = 0;

				dcache_rsp_port.valid = true;
				dcache_rsp_port.rdata = r_RSP_DCACHE_UNC_BUF.read();
				if ((r_RSP_DCACHE_UNC_BUF.read() == 0xFEED0BAD))
				{
					std::cout << name() << "WARNING : processor reads a data (unc, ll or sc)  that it was probably not initialized," << std::endl;
					std::cout << name() << "          it contained the magic 0xFEED0BAD set at allocation time of S-ram memory." << std::endl;
				}
				r_DCACHE_FSM = DCACHE_IDLE;

				break;
			}

		case DCACHE_CPUINVAL :
			{	
				const int y = m_d_y[r_DCACHE_ADDR_SAVE.read()];
				s_DCACHE_TAG[y] = 0;
				r_DCACHE_FSM = DCACHE_IDLE;     
				break; 
			}

		case DCACHE_RAMINVAL :
			if (dcache_inv_z == (s_DCACHE_TAG[dcache_inv_y].read()))	{ s_DCACHE_TAG[dcache_inv_y] = 0; }  // invalidate only if tags match

			// Check if a 1 cycle signal wich may have unlocked the r_DCACHE_FSM is there
			if (r_DRSP_OK)
			{
				if (r_PREV_DCACHE_FSM == DCACHE_MISSWAIT) { r_DCACHE_FSM = DCACHE_MISSUPDT; }
				else if (r_PREV_DCACHE_FSM == DCACHE_UNC_LL_SC_WAIT) { r_DCACHE_FSM = DCACHE_UNC_LL_SC_GO; }
			}
			else { r_DCACHE_FSM = r_PREV_DCACHE_FSM;} //idle, misswait, uncwait 
			break;

		default :
			assert(false);
			break;
	} // end switch r_DCACHE_FSM 


	{
		// xx_rsp_port have been filled, execute one simulator cycle
		uint32_t it = 0;
		for (size_t i=0; i<(size_t)iss_t::n_irq; i++)
		{
			if(p_irq[i].read()) {
					 it |= (1<<i);
			}
		}

#if 0
    std::cout << name () << "[===== " << ncycles << " ====]" << std::endl;

    std::cout << name () << icache_req_port << std::endl;
    std::cout << name () << icache_rsp_port << std::endl;

    std::cout << name () << dcache_req_port << std::endl;
    std::cout << name () << dcache_rsp_port << std::endl;
#endif

		m_iss.executeNCycles(1, icache_rsp_port, dcache_rsp_port, it);
	}

	switch ((req_fsm_state_e)r_VCI_REQ_FSM.read())
	{
		case REQ_IDLE :
			{
				const bool dr_uncached = r_DCACHE_UNCACHED_SAVE.read();
				const bool dr_ll = r_DCACHE_LL_SAVE.read();
				const bool dr_sc = r_DCACHE_SC_SAVE.read();

				assert(!(dr_ll && dr_sc));

				if (r_VCI_RSP_FSM != RSP_IDLE) { break; } // Synchronize FSM's

				r_REQ_CPT = 0;
				assert(! (r_DCACHE_REQ && r_DCACHE_FLUSH_WB_REQ));	
				if (r_DCACHE_REQ) // A Data cache request 
				{

					r_REQ_DCACHE_ADDR_PHYS  = r_DCACHE_ADDR_SAVE.read(); // PHYS addr : the one to send on wires 
					if (dr_ll) { r_VCI_REQ_FSM = REQ_LL; }
					else if (dr_sc)
					{
						r_VCI_REQ_FSM = REQ_SC;
						r_REQ_DCACHE_DATA   = r_DCACHE_DATA_SAVE.read();
					}
					else if (dr_uncached)	{ r_VCI_REQ_FSM = REQ_UNC; }// ll and sc prioritaires, ils peuvent être uncached
					else { r_VCI_REQ_FSM = REQ_DMISS; }
				}
				else if (r_DCACHE_FLUSH_WB_REQ) // A Write-buffer flush request
				{
					// The write-Buffer is flushed, if fact on a read we don't pop an item, we move an internal pointer. Consequently,
					// if we need to "retry" the request, the buffer still contains the data until we call init() when all the requests
					// have been ackownledged (response received)..

					r_REQ_DCACHE_ADDR_PHYS   = m_WRITE_BUFFER_ADDR.read();
					r_REQ_DCACHE_DATA        = m_WRITE_BUFFER_DATA.read();
					assert(m_WRITE_BUFFER_DATA.rok());

					write_buffer_get = true;
					r_VCI_REQ_FSM = REQ_WRITE;
				}
				else if (r_ICACHE_MISS_REQ) // Else, process instruction miss
				{

					r_REQ_ICACHE_ADDR_PHYS = r_ICACHE_MISS_ADDR_SAVE.read();
					r_VCI_REQ_FSM = REQ_IMISS;

				}
			break;
			}



		case REQ_IMISS :
			if (p_i_vci.cmdack.read())
			{ 
				r_VCI_REQ_FSM = REQ_IWAIT;
			}
			break;

		case REQ_UNC :
		case REQ_LL : 
		case REQ_SC : 
		case REQ_DMISS :
			// Commands wich requires a  1 cell packet
			if (p_i_vci.cmdack.read())
			{ 
				r_VCI_REQ_FSM = REQ_DWAIT;
			}
			break;

		case REQ_IWAIT :
			assert(!(r_IRSP_OK && r_IRETRY_REQ));
			if (r_IRSP_OK)
			{
				r_VCI_REQ_FSM = REQ_IDLE;
				r_ICACHE_MISS_REQ = false;
				r_IRSP_OK = false;
			}
			else if (r_IRETRY_REQ)
			{
				r_VCI_REQ_FSM = REQ_IDLE;
			}
			break;

		case REQ_DWAIT :
			assert(!(r_DRSP_OK && r_DRETRY_REQ));
			if (r_DRSP_OK)
			{
				r_VCI_REQ_FSM = REQ_IDLE;
				r_DCACHE_REQ = false;
				r_DCACHE_FLUSH_WB_REQ = false;
				r_DCACHE_PENDING_WRITE = false;
				r_DRSP_OK = false;
				// ATTENTION! l'information que l'on a un SC doit survivre jusqu'à la fin 
				// de la réception des réponses, ceci permet de renvoyer au processeur la
				// donnée lue!
			}
			else if (r_DRETRY_REQ)
			{
				r_VCI_REQ_FSM = REQ_IDLE;
			}
			break;

		case REQ_WRITE : // CMD_WRITE : as many cells as data in write buffer
			{
				if (p_i_vci.cmdack.read())
				{
					assert(r_WRITE_BURST_SIZE.read() == (m_WRITE_BUFFER_DATA.filled_status() + r_REQ_CPT.read()) + 1);
					r_REQ_CPT = r_REQ_CPT +1;
					if (r_REQ_CPT == r_WRITE_BURST_SIZE - 1)
					{
						r_VCI_REQ_FSM = REQ_DWAIT;
						assert(!m_WRITE_BUFFER_DATA.rok());
					}
					else
					{
						write_buffer_get = true; 
						r_REQ_DCACHE_DATA        = m_WRITE_BUFFER_DATA.read();
						r_REQ_DCACHE_ADDR_PHYS   = m_WRITE_BUFFER_ADDR.read();
						assert(m_WRITE_BUFFER_DATA.rok());
					}
				}
				break;
			}

	} 


// Manage Write-buffer through two commands (get and put)
	assert(!(write_buffer_put && write_buffer_get));
	if (write_buffer_put)
	{ 
		m_WRITE_BUFFER_DATA.simple_put(r_DATA_FIFO_INPUT.read()); 
		m_WRITE_BUFFER_ADDR.simple_put(r_ADDR_FIFO_INPUT.read()); 
	}

	if (write_buffer_get)
	{ 
		m_WRITE_BUFFER_DATA.simple_get(); 
		m_WRITE_BUFFER_ADDR.simple_get(); 
	}


	switch ((rsp_fsm_state_e)r_VCI_RSP_FSM.read()) {
		case RSP_IDLE :
			{
				const bool dr_uncached = r_DCACHE_UNCACHED_SAVE.read();
				const bool dr_ll = r_DCACHE_LL_SAVE.read();
				const bool dr_sc = r_DCACHE_SC_SAVE.read();

				if (r_VCI_REQ_FSM == REQ_IDLE)
				{ 
					// A retry policy on busy Ram memory can be used to avoid deadlocks on invalidations, other
					// way is to :
					// 1) have a "flat" interconnect with no hierarchy (gmn,crossbar etc).
					// 2) transfer invalidations on another NoC.
					// 3) transfer invalidations in virtual channels with a higher priority.
					// We prefer solutions 1 and 2 for simplicity, retry policy IS NOT SCALABLE.
					r_IRETRY_REQ = false;
					r_DRETRY_REQ = false;
					r_RSP_CPT = 0;
					assert(! (r_DCACHE_REQ && r_DCACHE_FLUSH_WB_REQ));	
					if (r_DCACHE_REQ)
					{
							if (dr_uncached || dr_ll || dr_sc)
							{
								r_VCI_RSP_FSM = RSP_UNC; // same behavior for LL and SC, we put the result in UNCACHED_BUF
							}
							else
							{
								r_VCI_RSP_FSM = RSP_DMISS;
							}
					}
					else if (r_DCACHE_FLUSH_WB_REQ)
					{
							r_VCI_RSP_FSM = RSP_WRITE;
					}
					else if (r_ICACHE_MISS_REQ)
					{
							r_VCI_RSP_FSM = RSP_IMISS;
					}
				}
				break;
			}


		case RSP_IMISS :
			if (p_i_vci.rspval.read()) { 
				assert(!(r_RSP_CPT == s_icache_words));  //illegal VCI response packet for instruction read miss
				r_RSP_CPT = r_RSP_CPT + 1; 
				r_RSP_ICACHE_MISS_BUF[r_RSP_CPT] = p_i_vci.rdata.read(); 
				if ( (r_RSP_CPT == s_icache_words -1) && (p_i_vci.rerror.read() == vci_param::ERR_NORMAL) && !r_IRETRY_REQ.read())
				// No error, all responses received, send IRSP_OK signal to unlock the DCACHE_FSM
				{ 
					r_VCI_RSP_FSM = RSP_IDLE; 
					r_IRSP_OK = true;
				}
				else if ((r_RSP_CPT == s_icache_words -1) && ((p_i_vci.rerror.read() != vci_param::ERR_NORMAL) || r_IRETRY_REQ.read()))
				// All responses received but an error was saw, so "retry" the request (DCACHE_FSM still blocked).
				{  
					r_VCI_RSP_FSM = RSP_IDLE; 
				}
				// When there is at leas one cell that was NACK'd (error set to 1), we must re-send the request
				if (p_i_vci.rerror.read() != vci_param::ERR_NORMAL) r_IRETRY_REQ = true; 
			}
			break;



		case RSP_DMISS :
			if (p_i_vci.rspval.read())
			{
				assert(!(r_RSP_CPT == s_dcache_words));  //illegal VCI response packet for data read miss
				r_RSP_CPT = r_RSP_CPT + 1; 
				r_RSP_DCACHE_MISS_BUF[r_RSP_CPT] = p_i_vci.rdata.read(); 
				if ((r_RSP_CPT == s_dcache_words -1) && (p_i_vci.rerror.read() == vci_param::ERR_NORMAL) && !r_DRETRY_REQ.read())
				{ 
					r_VCI_RSP_FSM = RSP_IDLE; 
					r_DRSP_OK = true;
				}
				else if ((r_RSP_CPT == s_dcache_words -1) && ((p_i_vci.rerror.read() != vci_param::ERR_NORMAL) || r_DRETRY_REQ.read()))
				{ 
					r_VCI_RSP_FSM = RSP_IDLE; 
				}  
				if (p_i_vci.rerror.read() != vci_param::ERR_NORMAL) r_DRETRY_REQ = true;
			}
			break;

		case RSP_WRITE :
			if (p_i_vci.rspval.read())
			{ 
				assert(p_i_vci.reop.read());    // illegal VCI response paquet for write 
				if (p_i_vci.rerror.read() == vci_param::ERR_NORMAL)
				{
					r_DRSP_OK = true;
					m_WRITE_BUFFER_ADDR.init();
					m_WRITE_BUFFER_DATA.init();
				} 
				else
				{
					r_DRETRY_REQ = true;
					// reset the internal pointer of the buffer but do not erase the content
					m_WRITE_BUFFER_ADDR.init_read(); 
					m_WRITE_BUFFER_DATA.init_read();
				}
				r_VCI_RSP_FSM = RSP_IDLE;
			} 
			break;

		case RSP_UNC : 
			if (p_i_vci.rspval.read())
			{ 
				r_RSP_DCACHE_UNC_BUF = p_i_vci.rdata.read(); 
				assert(p_i_vci.reop.read());    // illegal VCI response paquet for data read uncached
				if (p_i_vci.rerror.read() == vci_param::ERR_NORMAL)
				{
					r_DRSP_OK = true;
				} 
				else
				{
					r_DRETRY_REQ = true;
				} 
				r_VCI_RSP_FSM = RSP_IDLE;
			} 
			break;



		default :
			assert(false);
			break;

	} // end switch VCI_RSP_FSM 


	//////////////////////////////////////////////////////////////////////////
	// The VCI_INV controler
	////////////////////////////////////////////////////////////////////////////

	switch ((inv_fsm_state_e)r_VCI_INV_FSM.read()) {

		case INV_IDLE:
			{
				const typename vci_param::addr_t	dr_inv_address = p_t_vci.address.read();
				const typename vci_param::cmd_t 	dr_inv_command = p_t_vci.cmd.read();
				const typename vci_param::eop_t	dr_inv_eop = p_t_vci.eop.read();

				if (p_t_vci.cmdval.read())
				{ 
					if (!m_segment -> contains(dr_inv_address)){
						std::cout << "> SoCLib segmentation fault" << std::endl; 
						std::cout << "> A request has been sent by a component to a non-mapped address, it was cached by the target :" << name() << std::endl;
						std::cout << " 		target @ : 0x" << std::hex << (int)dr_inv_address << std::endl;
						std::cout << " 		data     : 0x" << std::hex << (int)p_t_vci.wdata.read() << std::endl;
						std::cout << " 		cmd      : 0x" << std::hex << (int)p_t_vci.cmd.read() << std::endl;
						std::cout << " 		eop      : 0x" << std::hex << (int)p_t_vci.eop.read() << std::endl;
						std::cout << " 		srcid    : " << std::hex << (int)p_t_vci.srcid.read() << std::endl;
						std::cout << " 		ncycles  : " << std::hex <<  ncycles << std::endl;
						sc_stop();
						return;
					}
					assert(dr_inv_command == vci_param::CMD_WRITE);    // invalidations are coded as write commands
					assert(dr_inv_eop);				  // always a one cell packet
					switch((dr_inv_address >> 2) & 0xf) // Discriminate TLB and DCACHE invalidations
						{
						case 0 :
							r_DCACHE_RAM_INVAL_REQ = true; // Send a signal to DCACHE FSM to inform the presence of an invalidation
							r_VCI_INV_FSM = INV_CMD; 
							break;
						default : assert(false);
											break;
					}
				}
				break;
			}

		case INV_CMD: // Save the invalidation request
			r_RAM_INV_ADDR = p_t_vci.wdata.read();
			r_SAV_SCRID = p_t_vci.srcid.read();
			r_SAV_PKTID = p_t_vci.pktid.read();
			r_SAV_TRDID = p_t_vci.trdid.read();
			r_VCI_INV_FSM = INV_WAIT; 
			break;


		case INV_WAIT: // For data invalidation, wait for completion in order to guarantee coherency
			if (!r_DCACHE_RAM_INVAL_REQ)
			{
				r_VCI_INV_FSM = INV_RSP;
			}	
			break;

		case INV_RSP:
			if (p_t_vci.rspack.read())
			{
				r_VCI_INV_FSM = INV_IDLE;
			}
			break;
	} // end switch VCI_INV_FSM 
}; 
}}
