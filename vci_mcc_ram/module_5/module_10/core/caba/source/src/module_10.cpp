/* -*- c++ -*-
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
 * 	Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2008
 *
 */



#include <cassert>
#include <iostream>
#include "module_10.h"
#include "module_6.h"
#include "arithmetics.h"
#include "globals.h"

namespace soclib {
	namespace caba {

		using namespace soclib;
		using namespace std;
		using soclib::common::uint32_log2;

#define p_out_fifo_wok ((p_out.m6_fifo)->size() < m_inter_fifo_size)
#define p_in_fifo_rok ((p_out.m6_fifo)->size() > 0)

		Module_10::Module_10(const string & insname,Module_9 * counters, Module_8 * contention,unsigned int nbproc):
			m_name(insname),
			m_NBPROC(nbproc),
			m_inter_fifo_size(M6_M10_INTER_FIFO_SIZE),
			m_counters(counters),
			m_contention(contention),
			m_wait_cycles(TIME_TO_WAIT),
			m_cycles(0),
			d_cycles(0),
			r_pending_reset(false)
		{
			CTOR_OUT
#ifdef USE_STATS
			stats_chemin = "./";
			stats_chemin += m_name.c_str();
			stats_chemin += ".stats"; 
			file_stats.setf(ios_base::hex);
			file_stats.open(stats_chemin.c_str(),ios::trunc);
			stats.cycles = 0;
			stats.c_mig = 0;
			stats.e_mig = 0;
			stats.i_mig = 0;
			stats.aborted = 0;
#endif
		}

		void Module_10::reset()
		{
			r_master_contention = false;
			r_m9_locked  = false;
			r_fsm = idle;
			r_max_p = 0xDEADBEFF;
			r_mig_c = false;
			r_index_cpt = 0;
			r_slave = false;
			r_waiting_time = false;
		}


		////////////////////////////////////////////////////////////////////////////////////
		//
		//  In compute : 
		//  	To avoid contention and deadlock some requests have some priority.
		//  	Requests from RING -> top_level priority
		//  	Requests from NODE -> medium priority
		//  	Raising an event   -> low level priority
		//
		//  	This later request cannot be issued if a request with a higher priority has
		//  	locked some structure
		//
		// 	This will need a good cleanup when thing will be completly defined
		//
		////////////////////////////////////////////////////////////////////////////////////
		
		void Module_10::compute()
		{
			bool inter_fifo_put = false;
			bool inter_fifo_get = false;
			inter_fifo_data_t inter_fifo_data = 0;
			m_cycles++;
			d_cycles++;
			#ifdef USE_STATS
			stats.cycles++;
			#endif
			if (m_cycles  == PE_PERIOD_CYCLES)
			{
				r_pending_reset = true;
				m_cycles = 0;

			}
			if (r_waiting_time)
			{
				if(m_wait_cycles>0)
				{
					m_wait_cycles--;
				}
				else
				{	
					m_wait_cycles = TIME_TO_WAIT;
					r_waiting_time = false;
					r_pending_reset = true;
				}
			}
			if ((r_pending_reset) && (r_master_contention == false) && (r_slave == false))
			{
				if (!(*(p_in.m9_is_busy)))
				{
					m_counters->reset_counters();
					m_contention->reset();
					r_pending_reset = false;
				}
			}

			switch((fsm_state_e)r_fsm)
			{
				// WARNING : try to avoid deadlocks
				case idle : 
					D10COUT(0) << name() << " [idle]" << endl;
					if (!(*(p_in.m6_req)))
					{ // No request on the interface
						// priority to requests from Module_6
						if (*(p_in.poison_ack))
						{
							// reading the virt_poison_page address
							r_virt_max_p_addr = *(p_in.virt_poison_page);
							D10COUT(2) << name() <<  "    --- vir_max_p_addr 0x" << hex << *(p_in.virt_poison_page) << endl;
							r_fsm = poison_ok;
							break;
						}
						if (*(p_in.unpoison_ok))
						{
							D10COUT(1) << "p_in.unpoison_ok"<< endl;
							r_fsm = unpoison_ok;
							break;
						}

						if (!(*p_in.m7_req)) // priority to requests from Module 7
						{ 
#if 1
							if((*(p_in.contention) || *(p_in.m9_is_threshold))&& !r_master_contention && !r_slave && !r_m9_locked && !r_waiting_time)
							{

								if (*(p_in.contention))
								{
									r_mig_c = true; // a C migration
#ifdef USE_STATS
									stats.c_mig++;
#endif
								}
								if (*(p_in.m9_is_threshold))
								{
									r_mig_c = false; // a E migration 
#ifdef USE_STATS
									stats.e_mig++;
#endif
								}
								D10COUT(0) << "    <--- !contention" << endl;
								D10COUT(0) << "    <--- !m9_locked" << endl;
								D10COUT(2) << name() << "    going to raise contention ( future master ?)  "  << endl;
								r_fsm = raise_contention; // raise contention only if we have not already raised one
								// and if counters are not locked
								// in fact, this action becomes the less priority one
							}
#endif 
						}
						else
						{
							switch (*(p_in.m7_cmd))
							{
								case Module_7::CMD7_LOCK:
									D10COUT(1) << "    << CMD7_LOCK" << endl;
									assert(!r_m9_locked);
									if (!(*(p_in.m9_is_busy)))
										r_fsm = counter_lock;
									break;
								case Module_7::CMD7_UNLOCK:
									D10COUT(1) << "    << CMD7_UNLOCK" << endl;
									assert(r_m9_locked);
									r_fsm = counter_unlock;
									break;
								default :
									assert(false);
									break;
							}
						}

					}
					else
					{
						D10COUT(2) << "   <else>" << endl;
						switch (*(p_in.m6_cmd))
						{
							case CMD_POISON :
								D10COUT(2) << "    << CMD_POISON" << endl;
								r_fsm = poison;
								break;
							case CMD_ABORT :
								D10COUT(2) << "    << CMD_ABORT" << endl;
								r_fsm = abort;
								break;
							case CMD_UNPOISON :
								D10COUT(2) << "    << CMD_UNPOISON" << endl;
								r_fsm = unpoison;
								break;
							case CMD_ELECT :
								D10COUT(2) << "    << CMD_ELECT" << endl;
								r_fsm = elect;
								D10COUT(2) << name() << "    ---> contention by elect "  << endl;
								r_slave = true;
								break;
							case CMD_BEGIN_TRANSACTION :
								D10COUT(2) << "    << CMD_BEGIN_TRANSACTION" << endl;
								r_fsm = access_value;
								D10COUT(1) << name() << "    ---> transaction (contention taken into account by m6)" << endl;
								break;
							case CMD_UPDT_INV :
								D10COUT(2) << "    << CMD_UPDT_INV" << endl;
								r_page_local_addr  = (*(p_in.m6_data));
								D10COUT(1) << name() <<  "    --- page_local_addr 0x" << hex << (*(p_in.m6_data)) << endl;
								r_fsm = updt_inv;
								break;
							case CMD_UPDT_WIAM :
								D10COUT(1) << "    << CMD_UPDT_WIAM" << endl;
								r_page_local_addr  = (*(p_in.m6_data));
								D10COUT(1) << name() <<  "    --- page_local_addr_bis 0x" << hex << (*(p_in.m6_data)) << endl;
								r_fsm = updt_wiam;
								break;
							default :
								assert(false);
								break;
						}
					}
					break;	

				case updt_inv :
					D10COUT(1) << name() << " [updt_inv]" << endl;
					if (*(p_in.m6_cmd) != CMD_UPDT_INV) cout << "error : m6_cmd == " << dec << *(p_in.m6_cmd) << endl;
					assert(*(p_in.m6_cmd) == CMD_UPDT_INV);
					r_page_new_addr  = (*(p_in.m6_data)); // This is the second data
					D10COUT(1) << name() << "    --- page_new_addr 0x" << hex << (*(p_in.m6_data)) << endl;
					r_fsm =  updt_table;
					break;

				case updt_wiam :
					D10COUT(1) << name() << " [updt_wiam]" << endl;
					if (*(p_in.m6_cmd) != CMD_UPDT_WIAM) cout << "error : m6_cmd == " << dec << *(p_in.m6_cmd) << endl;
					assert(*(p_in.m6_cmd) == CMD_UPDT_WIAM);
					r_page_wiam_addr  = (*(p_in.m6_data)); // This is the second data
					D10COUT(1) << name() << "    --- page_wiam_addr 0x" << hex << (*(p_in.m6_data)) << endl;
					r_fsm =  updt_wiam_ok;
					break;

				case updt_wiam_ok :
					if (*(p_in.updt_wiam_ack))
					{
						D10COUT(1) << name() << " [updt_wiam]" << endl;
						r_fsm = idle;
					}
					break;
				case updt_table :
					D10COUT(1) << name() << " [updt_table]" << endl;
					if (*(p_in.updt_tlb_ack))
					{
						r_fsm = idle;
					}
					break;
				case counter_lock : 
					D10COUT(1) << name() << " [counter_lock]" << endl;
					r_m9_locked = true;
					D10COUT(0) << name() << "    ---> m9_locked" << endl;
					r_fsm = idle;
					break;
				case counter_unlock : 
					D10COUT(1) << name() << " [counter_unlock]" << endl;
					r_m9_locked = false;
					D10COUT(0) << name() << "    |--- m9_locked" << endl;
					r_fsm = idle;
					break;


				case raise_contention :
					D10COUT(1) << name() << " [raise_contention]" << endl;
					if (*(p_in.m6_req))
					{ // No request on the interface
						assert(!*(p_in.m6_ack));
						switch (*(p_in.m6_cmd))
						{
							case CMD_ELECT :
								r_fsm = elect;
								// The local contention will not be migrated, but
								// we will work on the migration of a selected page
								// on the orders of a foreign node.
								// When the unpoison will be done, contention will still be there
								// so the migration of the local page will be done later
								D10COUT(1) << name() << "    ---> transaction (by elect)" << endl;
								D10COUT(2) << name() << "    ---> contention by elect "  << endl;
								r_slave = true;
								break;
							case CMD_BEGIN_TRANSACTION :
								r_fsm = access_value;
								r_master_contention = true;
								D10COUT(1) << name() << "    ---> transaction (contention taken into account by m6)" << endl;
								CWOUT << name() << "this can be cleaned, r_master_contention is set too many times!" << endl;
								D10COUT(2) << name() << "    ---> contention by begin_transaction (master) "  << endl;
								break;
							default :
								cerr << name() << " Error, received unexpected command 0x" << hex << (*(p_in.m6_cmd)) << endl;
								cout << name() << " WARNING, received command 0x" << hex << (*(p_in.m6_cmd)) << endl;
#ifdef USE_STATS
								cerr << name() << " At : " << dec << stats.cycles << endl;
#endif
								r_fsm = idle;
								//assert(false);
								assert(!*(p_in.m6_ack));
								break;
						}
					}
					if(*(p_in.m6_ack))
					{ // Ack from Module 6
						r_master_contention = true; 
						D10COUT(2) << name() << "    ---> contention by m6 ack (master) "  << endl;
						r_fsm = idle;
					}
					break;

				case elect :
					D10COUT(1) << name() << " [elect]" << endl;
					r_max_p = m_counters->evict_sel();
					D10COUT(1) << name() << "    --- r_max_p (elect)" << dec <<  m_counters->evict_sel() << endl;
#ifdef USE_STATS
					stats.i_mig++;
#endif
					r_fsm = idle;
					break;

				case access_value :
					{
						D10COUT(2) << name() << " [access_value]" << endl;
						// 1 retrieve the max page index 
						r_max_p = m_counters->read_max_page();
						if (!(*(p_in.m9_is_busy)) && p_out_fifo_wok )
						{
							inter_fifo_put = true;
							inter_fifo_data = m_counters->read_global_counters( r_max_p ); 
							r_fsm = access_counters;
						}
					}
					break;

				case access_counters :
					D10COUT(2) << name() << " [access_counters]" << endl;
					// Accessing the counters and putting them into the fifo
					// The freeze signal is set here, but it will not stall the counters until 
					// the current operation has finished. When is done, is_busy signals goes to false and
					// we can access registers. Moreover, being here implies that no one else has a lock granted
					// on the counters.


					D10COUT(1) << name() << "    --- r_max_p (counters) " << dec <<  m_counters->read_max_page() << endl;

					// 2 send the ids of the most consumming processors for this page
					if (!(*(p_in.m9_is_busy)) && p_out_fifo_wok )
					{

						inter_fifo_put = true;
						inter_fifo_data = m_counters->read_rounded_counter( r_max_p, r_index_cpt); 
						D10COUT(1) << name() << "   putting in inter_fifo" << dec <<  inter_fifo_data << endl;
						D10COUT(1) << name() << "   index " << dec <<  r_index_cpt << endl;
						D10COUT(1) << name() << "   m_NBPROC " << dec <<  m_NBPROC << endl;
						if (r_index_cpt == (m_NBPROC - 1))
						{
							D10COUT(0) << name() << " going to idle" << endl;
							r_fsm = idle;
							r_index_cpt = 0;
						}
						else
						{
							r_index_cpt++;
						}
					}
					break;


				case unpoison :
					D10COUT(1) << name() << " [unpoison]" << endl;
					// We go to idle, in order to avoid deadlocks or complexity at the ram_fsm. Because, the REQ_REGISTER state
					// of ram_fsm can be stalled waiting for a the lock, meanwhile we are here waiting for the ram_fsm to be
					// in IDLE and take our poison request.
					r_fsm = idle;
					break;
				case poison :
					D10COUT(2) << name() << " [poison]" << endl;

					r_fsm = idle;
					// We go to idle, in order to avoid deadlocks or complexity at the ram_fsm. Because, the REQ_REGISTER state
					// of ram_fsm can be stalled waiting for a the lock, meanwhile we are here waiting for the ram_fsm to be
					// in IDLE and take our poison request.
					break;
				case poison_ok : // Notify the poison state and send the index of the page to be transfered
					D10COUT(2) << name() << " [poison_ok]" << endl;
					r_fsm =  send_home_addr;
					break;

				case abort :
					D10COUT(2) << name() << " [abort]" << endl;
					if (!(*(p_in.m9_is_busy)))
					{
						r_fsm =  idle;
						D10COUT(2) << name() << "  abort on " << std::hex << (r_max_p << 12) << endl; 
						m_counters->reset_reg(r_max_p);
						m_contention->reset();
#ifdef USE_STATS
						stats.aborted++;
#endif

						assert(r_slave == false);
						assert(r_master_contention);
						r_master_contention = false;
						D10COUT(2) << name() << "    |--- contention end (master)  "  << endl;
						// Reseting when possible the counters, hence, we will not migrate 
						// the previous migrated page on the next contention signal!
					}
					break;

				case unpoison_ok : // Notify the poison state and send the index of the page to be transfered
					D10COUT(1) << name() << " [unpoison_ok]" << endl;
					assert(*(p_in.m6_ack)); // Useless ack, but used to "vérifier" that everything is ok!
					r_waiting_time = true;
						// Wait some time before setting r_contention to false, hence, the contention created
						// by the page migration will not be taken into account!
						
					if (r_slave == false)
					{
						assert(r_master_contention);
						r_master_contention = false;
						// reset r_master_contention only if we were the master on this transaction to avoid multiple "masters" on
						// the ring. This is due to the fact that we will be able to raise another "contention" request when we
						// will retrieve the ring token for the previous "contention request", thus, we will duplicate all the
						// the commands and things will not work.
						D10COUT(2) << name() << "    |--- contention end (master)  "  << endl;
					}else{
						r_slave = false;
						D10COUT(2) << name() << "    |--- contention end (slave)  "  << endl;
					}

					r_fsm = finish;
					break;

				case finish :
					D10COUT(1) << name() << " [finish]" << endl;
					r_fsm =  idle;
					break;

				case send_home_addr : // Send the virtual address of the page that was switched with the one to transfer now 
					D10COUT(1) << name() << " [send_home_addr]" << endl;
					r_fsm =  idle;
					break;
				default : 
					assert(false);
					break;
			}

			////////////////////////
			//   FIFO FSMs
			///////////////////////
			if (inter_fifo_put)
			{
				(p_out.m6_fifo)->push_back(inter_fifo_data);
			}
			if (inter_fifo_get)
			{
				(p_in.m6_fifo)->pop_front();
			}
		}
		void Module_10::gen_outputs()
		{
			// default values
			*(p_out.m6_ack) = false;
			*(p_out.m6_cmd) = CMD_NOP;
			*(p_out.m6_data)= 0;
			*(p_out.m6_req) = false;

			*(p_in.m7_rsp) = false;

			*(p_out.poison_page) = 0;
			*(p_out.poison_req) = false;
			*(p_out.unpoison_req) = false;

			*(p_out.updt_wiam_req) = false;
			*(p_out.updt_tlb_req) = false;
			*(p_out.home_entry) = 0xDEADBEEF;
			*(p_out.new_entry) = 0xDEADBEEF;
			// freeze = counter_lock OR r_m9_locked
			// this should avoid race condition on test busy signal and set freeze signal
			*(p_out.m9_freeze) = r_m9_locked;
			*(p_out.m6_pressure) = m_contention->read_pressure(); // Always send pressure signal
			*(p_out.m8_abort) = false;
			switch((fsm_state_e)r_fsm)
			{
				case idle : 
					*(p_out.m6_ack) = true;
					break;

				case raise_contention :
					// Todo : clean the interface!
					if (r_mig_c)
					{
						*(p_out.m6_cmd) = CMD_IS_CONTENTION;
					}
					else
					{
						*(p_out.m6_cmd) = CMD_PE_MIG;
					}
					*(p_out.m6_data)= 0;
					*(p_out.m6_req) = true;
					break;

				case counter_lock :
					*(p_out.m9_freeze) = true; // freeze = counter_lock OR r_m9_locked
				case counter_unlock :
					*(p_in.m7_rsp) = true;
					break;

				case unpoison :
					*(p_out.unpoison_req) = true;
					break;
				case poison :
					D10COUT(1) << "poisoning :" << hex << r_max_p << endl;
					*(p_out.poison_page) = r_max_p;
					*(p_out.poison_req) = true;
					break;
				case poison_ok :
					*(p_out.m6_cmd) = CMD_IS_POISONNED;
					*(p_out.m6_req) = true;
					*(p_out.m6_data)= r_max_p; 
					break;
				case unpoison_ok :
					*(p_out.m6_cmd) = CMD_UNPOISON_OK;
					*(p_out.m6_req) = true;
					break;
				case send_home_addr :
					*(p_out.m6_cmd) = CMD_PAGE_ADDR_HOME;
					*(p_out.m6_req) = true;
					*(p_out.m6_data)= r_virt_max_p_addr; 
					break;
				case access_counters :
				case access_value :
					*(p_out.m9_freeze) = true; // freeze = counter_lock OR r_m9_locked
					break;
				case abort :
					*(p_out.m8_abort) = true;
				case elect :
				case finish :
					break;
				case updt_table :
					*(p_out.home_entry) = r_page_local_addr;
					*(p_out.new_entry) = r_page_new_addr;
					*(p_out.updt_tlb_req) = true;
					break;
				case updt_wiam :
					*(p_out.m6_ack) = true;
					break;
				case updt_wiam_ok :
					*(p_out.home_entry) = r_max_p;
					*(p_out.new_entry) = r_page_wiam_addr;
					*(p_out.updt_wiam_req) = true;
					break;
				case updt_inv :
					*(p_out.m6_ack) = true;
					break;
				default : 
					assert(false);
					break;
			}
		}
		const string & Module_10::name()
		{
			return this->m_name;
		}
		Module_10::~Module_10()
		{
#ifdef USE_STATS
			file_stats << " NB-E-out migrations  : " << std::dec << stats.e_mig << std::endl;
			file_stats << " NB-C-out migrations  : " << std::dec << stats.c_mig << std::endl;
			file_stats << " aborted migrations   : " << std::dec << stats.aborted << std::endl;
			file_stats << " NB-in migrations     : " << std::dec << stats.i_mig << std::endl;
			file_stats << " total migrations     : " << std::dec << (stats.e_mig + stats.c_mig) << std::endl;
			file_stats << " total effective mig. : " << std::dec << (stats.e_mig + stats.c_mig - stats.aborted) << std::endl;
#endif
			DTOR_OUT
		}

	}}
