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

	// Instruction cache requests
	const typename vci_param::addr_t		icache_address = icache_req_port.addr;
	const unsigned int	icache_y = m_i_y[icache_address];
	const unsigned int	icache_x = m_i_x[icache_address];
	const typename vci_param::addr_t		icache_z = m_i_z[icache_address] | V_BIT;
	const bool		icache_hit = (icache_z == s_ICACHE_TAG[icache_y].read());
	const bool		icache_req = icache_req_port.valid;


	// Data cache requests
	const typename vci_param::addr_t		dcache_address  = dcache_req_port.addr;
	const typename vci_param::data_t		dcache_wdata  = dcache_req_port.wdata;
	const unsigned int	dcache_x = m_d_x[dcache_address];
	const unsigned int	dcache_y = m_d_y[dcache_address];
	const typename vci_param::addr_t		dcache_z = m_d_z[dcache_address] | V_BIT;
	const bool		dcache_hit = (dcache_z == s_DCACHE_TAG[dcache_y].read());
	const bool		dcache_req = dcache_req_port.valid;
	const typename iss_t::be_t dcache_be = dcache_req_port.be;
	const int dcache_type = dcache_req_port.type;

	// Invalidations
	const typename vci_param::addr_t		dcache_inv_address = r_RAM_INV_ADDR;
	const unsigned int									dcache_inv_y = m_d_y[dcache_inv_address] ;
	const typename vci_param::addr_t	 	dcache_inv_z = m_d_z[dcache_inv_address] | V_BIT ;
	const bool		dcache_ram_inval = r_DCACHE_RAM_INVAL_REQ;

	// control the write buffer
	bool		write_buffer_put = false;
	bool		write_buffer_get = false;

	ncycles++;
#ifdef USE_STATS
	stats.ncycles++;
#if CONTI_STATS
	if (stats.ncycles%10000 == 0) // compute and print a chunck stat!
	{
		stats.delta_data_latency = (float)(stats.ddelay_chunck)/(float)(stats.dcache_chunck_miss);
		stats.delta_inst_latency = (float)(stats.idelay_chunck)/(float)(stats.icache_chunck_miss);
		stats.ddelay_chunck = 0;
		stats.idelay_chunck = 0;
		stats.dcache_chunck_miss = 0;
		stats.icache_chunck_miss = 0;
		file_stats << std::dec << m_id << " lat_data " << std::dec << stats.delta_data_latency << std::endl; 
		file_stats << std::dec << m_id << " lat_inst " << std::dec << stats.delta_inst_latency << std::endl; 
	}
#endif
#endif

	//////////////////////////////
	//        RESET
	//////////////////////////////
	if (p_resetn == false) { 
		m_iss.reset();
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

	switch((icache_fsm_state_e)r_ICACHE_FSM.read())
	{
		case ICACHE_INIT :
			s_ICACHE_TAG[r_ICACHE_CPT_INIT] = 0;
			r_ICACHE_CPT_INIT = r_ICACHE_CPT_INIT - 1;
			if (r_ICACHE_CPT_INIT == 0) { r_ICACHE_FSM = ICACHE_IDLE; }  
			break;

		case ICACHE_IDLE :
			if (!icache_req) break; // nothing to do
			if (icache_hit) // the common case, return the instruction
			{
				icache_rsp_port.valid          = icache_hit;
				icache_rsp_port.instruction    = s_ICACHE_DATA[icache_y][icache_x].read();
#ifdef USE_STATS
				stats.icache_hit++;
#endif
			}
			else // miss, issue a miss request and wait for the response
			{
#ifdef USE_STATS
				stats.icache_miss++;
				stats.icache_chunck_miss++;
#endif
				r_ICACHE_MISS_ADDR_SAVE = icache_address;
				r_ICACHE_FSM = ICACHE_WAIT;
				r_ICACHE_MISS_REQ = true;
			}
			break;

		case ICACHE_WAIT : // wait until the response arrives
#ifdef USE_STATS
			stats.idelay++;
			stats.inack_chunck++;
			stats.idelay_chunck++;
#endif
			if (r_IRSP_OK)
			{
				r_ICACHE_FSM = ICACHE_UPDT;
			}
			break;

		case ICACHE_UPDT :
#ifdef USE_STATS
			stats.idelay++;
			stats.idelay_chunck++;
#endif
			s_ICACHE_TAG[icache_y] = icache_z;
			for (size_t i=0 ; i<s_icache_words ; i++) // Fill up the memory line
			{
				s_ICACHE_DATA[icache_y][i] = r_RSP_ICACHE_MISS_BUF[i];
			}
			r_ICACHE_FSM = ICACHE_IDLE; // Go to idle, the next cycle the request will hit in cache 
			break;

		default : // an impossible case
			assert(false);
			break;
	} // end switch ICACHE_FSM

	switch ((dcache_fsm_state_e)r_DCACHE_FSM.read())
	{
		case DCACHE_INIT :
			s_DCACHE_TAG[r_DCACHE_CPT_INIT] = 0;
			r_DCACHE_CPT_INIT = r_DCACHE_CPT_INIT - 1;
			if (r_DCACHE_CPT_INIT == 0) { r_DCACHE_FSM = DCACHE_IDLE; }  
			break;

		case DCACHE_IDLE : 
			{
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
#ifdef USE_STATS
						stats.dcache_writes+=m_WRITE_BUFFER_DATA.filled_status() + 1;
#endif
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
#ifdef USE_STATS
							stats.dcache_writes+=m_WRITE_BUFFER_DATA.filled_status() + 1;
#endif
							r_WRITE_BURST_SIZE = m_WRITE_BUFFER_DATA.filled_status() + 1; // could be implemented with a little 4bits adder
							assert(m_WRITE_BUFFER_DATA.wok());
						}
					}
					else // DATA_FIFO not full, merge if possible new write request in this cell, or flush the write-buffer
					{
						if (((dcache_address & m_dcache_zmask) == r_DCACHE_ENQUEUED_ADDR_SAVE.read()) // Address of a byte in the same address word
								&& (dcache_type == iss_t::DATA_WRITE )
								&& (ffs(dcache_be) == (soclib::common::fls(r_DCACHE_WL_BE_SAVE.read()) + 1))) // consecutive be's  
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
#ifdef USE_STATS
							stats.dcache_writes+=m_WRITE_BUFFER_DATA.filled_status() + 1;
#endif
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
#ifdef USE_STATS
									stats.dcache_miss++;
									stats.dcache_chunck_miss++;
#endif
								}
								else   // hit, going to idle, deliver the data to the processor
								{
#ifdef USE_STATS
									stats.dcache_hit++;
#endif
									r_DCACHE_FSM = DCACHE_IDLE;
									dcache_rsp_port.valid = true;
									dcache_rsp_port.rdata = s_DCACHE_DATA[dcache_y][dcache_x].read();
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
#ifdef USE_STATS
			stats.ddelay++;
			stats.dnack_chunck++;
			stats.ddelay_chunck++;
#endif
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
#ifdef USE_STATS
				stats.ddelay++;
				stats.ddelay_chunck++;
#endif
				for (size_t i=0 ; i < s_dcache_words ; i++)
				{
					s_DCACHE_DATA[y][i] = r_RSP_DCACHE_MISS_BUF[i];
				}
				dcache_rsp_port.valid = true;
				dcache_rsp_port.rdata = r_RSP_DCACHE_MISS_BUF[x].read();
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
			if(p_irq[i].read()) { it |= (1<<i); }
		}
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
#ifdef USE_STATS
					if (address_to_id(r_DCACHE_ADDR_SAVE.read()) != -1)
					{ // accessing a memory node (tty, fdacces etc returns -1)
						stats . total_dist_reqs += m_table_cost[address_to_id(r_DCACHE_ADDR_SAVE.read())];
						stats . total_reqs += 1;
					}
#endif

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

#ifdef USE_STATS
					if (address_to_id(m_WRITE_BUFFER_ADDR.read()) != -1)
					{
						stats . total_dist_reqs += m_table_cost[address_to_id(m_WRITE_BUFFER_ADDR.read())];
						stats . total_reqs += 1;
					}
#endif
					r_REQ_DCACHE_ADDR_PHYS   = m_WRITE_BUFFER_ADDR.read();
					r_REQ_DCACHE_DATA        = m_WRITE_BUFFER_DATA.read();
					assert(m_WRITE_BUFFER_DATA.rok());

					write_buffer_get = true;
					r_VCI_REQ_FSM = REQ_WRITE;
				}
				else if (r_ICACHE_MISS_REQ) // Else, process instruction miss
				{
#ifdef USE_STATS
					if (address_to_id(r_ICACHE_MISS_ADDR_SAVE.read()) != -1)
					{
						stats . total_dist_reqs += m_table_cost[address_to_id(r_ICACHE_MISS_ADDR_SAVE.read())];
						stats . total_reqs += 1;
					}
#endif

					r_REQ_ICACHE_ADDR_PHYS = r_ICACHE_MISS_ADDR_SAVE.read();
					r_VCI_REQ_FSM = REQ_IMISS;

				}
			break;
			}



		case REQ_IMISS :
			if (p_i_vci.cmdack.read())
			{ 
				r_VCI_REQ_FSM = REQ_IWAIT;
#ifdef COHERENT_XCACHE_DEBUG
				file_chrono << std::dec << ncycles << " " << m_i_ident << " : " << " INST @0x" << std::hex <<  (r_ICACHE_MISS_ADDR_SAVE & m_icache_yzmask)
					<< " page4k P_" << std::dec << ((r_ICACHE_MISS_ADDR_SAVE & 0x00FFFFFF)>>12) << " pc_0x" << std::hex << icache_address;
#endif
			}
			break;

		case REQ_UNC :
		case REQ_LL : 
		case REQ_SC : 
		case REQ_DMISS :
			// Commands wich requires a  1 cell packet
			if (p_i_vci.cmdack.read())
			{ 
#ifdef COHERENT_XCACHE_DEBUG
				file_chrono << std::dec << ncycles << " " << m_i_ident << " : " << " DATA-R @0x" << std::hex <<  (r_REQ_DCACHE_ADDR_PHYS & m_dcache_yzmask )
					<< " page4k P_" << std::dec << ((r_REQ_DCACHE_ADDR_PHYS & 0x00FFFFFF )>>12) << " pc_0x" << std::hex << icache_address;
#endif
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
#ifdef COHERENT_XCACHE_DEBUG
				file_chrono << std::dec << " lat : " << std::dec << delay << std::endl; 
#endif
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
#ifdef COHERENT_XCACHE_DEBUG
				file_chrono << std::dec << " lat : " << std::dec << delay << std::endl; 
#endif
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
#ifdef COHERENT_XCACHE_DEBUG
						file_chrono << std::dec << ncycles << " " << m_i_ident << " : " << " DATA-W BURST SIZE " << std::dec  << r_WRITE_BURST_SIZE.read()
							<< std::hex <<  " @0x" << std::hex <<  (r_REQ_DCACHE_ADDR_PHYS & m_dcache_yzmask )
							<< " page4k P_" << std::dec << ((r_REQ_DCACHE_ADDR_PHYS & 0x00FFFFFF )>>12) << " pc_0x" << std::hex << icache_address;
#endif
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
#ifdef USE_STATS
					stats.inack_chunck = 0;
#endif
				}
				else if ((r_RSP_CPT == s_icache_words -1) && ((p_i_vci.rerror.read() != vci_param::ERR_NORMAL) || r_IRETRY_REQ.read()))
				// All responses received but an error was saw, so "retry" the request (DCACHE_FSM still blocked).
				{  
					r_VCI_RSP_FSM = RSP_IDLE; 
#ifdef USE_STATS
					stats.iread_nack += 1;
					stats.nack_idelay += (stats.inack_chunck + 1);
					stats.inack_chunck = 0;
#endif
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
#ifdef USE_STATS
					stats.dnack_chunck = 0;
#endif
				}
				else if ((r_RSP_CPT == s_dcache_words -1) && ((p_i_vci.rerror.read() != vci_param::ERR_NORMAL) || r_DRETRY_REQ.read()))
				{ 
					r_VCI_RSP_FSM = RSP_IDLE; 
#ifdef USE_STATS
					stats.dread_nack += 1;
					stats.nack_ddelay += (stats.dnack_chunck + 1);
					stats.dnack_chunck = 0;
#endif
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
#ifdef USE_STATS
					stats.dwrite_nack += 1;
#endif
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
					if (!m_segment.contains(dr_inv_address)){
						std::cout << "> SoCLib segmentation fault" << std::endl; 
						std::cout << "> A request has been sent by a component to a non-mapped address, it was cached by the target :" << name() << std::endl;
						std::cout << " 		target @ : 0x" << std::hex << (int)dr_inv_address << std::endl;
						std::cout << " 		data     : 0x" << std::hex << (int)p_t_vci.wdata.read() << std::endl;
						std::cout << " 		cmd      : 0x" << std::hex << (int)p_t_vci.cmd.read() << std::endl;
						std::cout << " 		eop      : 0x" << std::hex << (int)p_t_vci.eop.read() << std::endl;
						std::cout << " 		srcid    : " << std::hex << (int)p_t_vci.srcid.read() << std::endl;
						std::cout << " 		ncycles  : " << std::hex <<  ncycles << std::endl;
						raise(SIGINT);
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
