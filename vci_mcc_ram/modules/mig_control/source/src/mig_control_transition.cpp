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
#include "mig_control.h"
#include "arithmetics.h"
#include "mcc_globals.h"

namespace soclib {
	namespace caba {

		using namespace soclib;
		using namespace std;
		using soclib::common::uint32_log2;
		
		void MigControl::transition( void )
		{
			if(!p_resetn.read()){
				cout << name() << " <<<< reseting >>>> " << std::endl;
				d_cycles = 0;

				r_disable_contention_raise = false;
				r_master_transaction = false;
				r_slave_transaction  = false;
				r_pending_reset = false;

				r_MIG_TODO_FSM = t_idle;
				r_MIG_M_CTRL_FSM = m_idle;
				r_MIG_PP_CTRL_FSM = pp_idle;	
				r_MIG_PT_CTRL_FSM = pt_idle;
				r_MIG_C_CTRL_FSM = c_idle;

				r_c_max_p = 0xDEADDEAD;
				r_c_virt_max_p_addr = 0xDEADDEAD;
				r_c_index_cpt = 0xDEADDEAD;

				r_pt_page_local_addr = 0xDEADDEAD;
				r_pt_page_new_addr = 0xDEADDEAD;
				for (int i = 0; i < 16 ; i++)
				{
					r_todo_board[ i ] = 0;
				}
				return;
			}

			d_cycles++;
#ifdef USE_STATS
			stats.cycles++;
#endif
// TODO :
// 	-> Reset périodique des compteurs 
// 	-> interconnection avec le module 6

			// T : TODO FSM, process requests from/to mig_engine through a
			// "todo" blackboard.
			switch((fsm_m_state_e)r_MIG_TODO_FSM.read())
			{
				case t_idle :
					if (p_in_mig_engine_req.read()) {
						switch (p_in_mig_engine_cmd.read())
						{
							case CMD_UPDT_INV :
							case CMD_UPDT_WIAM :
#if 0
								r_page_local_addr  = p_in_mig_engine_data_0.read();
								r_page_new_addr    = p_in_mig_engine_data_1.read();
#endif
							case CMD_BEGIN_TRANSACTION :
							case CMD_ABORT :
							case CMD_POISON :
							case CMD_UNPOISON :
							case CMD_ELECT :
								if (r_todo_board[ (unsigned int)p_in_mig_engine_cmd.read() ]  == false){
									r_MIG_TODO_FSM = t_register;
								}
								break;
							default:
								assert(false);
								break;
						}
					}
					break;

				case t_register :
					// Moore ack of incomming request
					r_todo_board[ (unsigned int)p_in_mig_engine_cmd.read() ] = true;
					r_MIG_TODO_FSM = t_idle;
					break;

				default : 
					assert(false);
					break;
			}

			// M : Manometer, process "contention" signals from manometer or
			// counters (or other sources).
			switch((fsm_m_state_e)r_MIG_M_CTRL_FSM.read())
			{
				case m_idle:
					if((p_in_manometer_contention.read() || (p_in_counters_contention.read())) && !r_master_transaction && !r_slave_transaction )
					{

					D10COUT(0) << name() << " contention detected " << std::endl;
					if (p_in_manometer_contention.read()) {
						D10COUT(0) << name() << " > manometer " << std::endl;
					}
					if (p_in_counters_contention.read()) {
						D10COUT(0) << name() << " > counters " << std::endl;
					}
#ifdef USE_STATS
						if (p_in_manometer_contention.read()) { stats.c_mig++; }
						if (p_in_counters_contention.read()) { stats.e_mig++; }
#endif
						r_MIG_M_CTRL_FSM = m_raise_contention; 
					}
					if (r_todo_board[ CMD_ABORT ] == true){
								r_MIG_M_CTRL_FSM = m_abort;
					}
					break;

				case m_raise_contention :
					
					// Moore : send a contention signal, wait for ack
#if 0
					if(*(p_in.m6_ack))
					{ // Ack from Module 6
						r_master_contention = true; 
						D10COUT(2) << name() << "    ---> contention by m6 ack (master) "  << endl;
						r_MIG_M_CTRL_FSM = m_idle;
					}
#else
					D10COUT(0) << name() << " ack .. going back idle " << std::endl;
					r_MIG_M_CTRL_FSM = m_idle;
					r_master_transaction = true; 
					r_todo_board[CMD_ELECT] = true; // HACK 
#endif
					break;


				case m_abort :
					assert(false);
					// Moore : send a reset signal to counters and manometer, wait for ack
#if 0
					if (p_in_counters_ack.read()){ // Reset command was accepted
						r_MIG_M_CTRL_FSM =  m_idle;
						D10COUT(2) << name() << "  abort on " << std::hex << (r_max_p << 12) << endl; 
						D10COUT(2) << name() << "    |--- contention end (master)  "  << endl;
#ifdef USE_STATS
						stats.aborted++;
#endif
						assert(r_slave == false);
						assert(r_master_contention);
						r_master_contention = false;
					}
#endif
					break;
			}

			// C : Counters, read/write counters, read max page
			switch((fsm_c_state_e)r_MIG_C_CTRL_FSM.read())
			{
				case c_idle :
					if (r_todo_board[ CMD_BEGIN_TRANSACTION ] == true)
					{
						r_MIG_C_CTRL_FSM = c_access_value;
					}
					else if(r_todo_board[ CMD_ELECT ] == true)
					{
						r_MIG_C_CTRL_FSM = c_elect;
					}
				break;

				case c_elect :
#ifdef USE_STATS
					stats.i_mig++;
#endif
					if (p_in_counters_valid.read() && p_in_counters_ack.read()){
						r_slave_transaction = true;
						r_c_elect_p = p_in_counters_output.read(); 
						D10COUT(0) << name() << " > received elected page " << std::endl;
						r_MIG_C_CTRL_FSM = c_idle;
					}
#if 0
					// Send data to mig_engine 
#endif
					break;

				case c_access_value :
					{
						// 1 retrieve the max page index 
						assert(false);
						if (p_in_counters_valid.read() && p_in_counters_ack.read()){
							r_c_max_p = p_in_counters_output.read();
							r_MIG_C_CTRL_FSM = c_access_counters;
						}
#if 0
					// Send data to mig_engine 
#endif
					}
					break;

				case c_access_counters :
					assert(false);
#if 0
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
							r_MIG_CTRL_FSM = idle;
							r_index_cpt = 0;
						}
						else
						{
							r_index_cpt++;
						}
					}
#endif
					break;
				default :
				assert(false);
				break;
			}

			// PP : Poison Page : wathever is related to page poisonning state
		
			switch((fsm_pp_state_e)r_MIG_PP_CTRL_FSM.read())
			{
				case pp_idle :
					if (r_todo_board[ CMD_POISON ] == true)
					{
						r_MIG_PP_CTRL_FSM = pp_poison;
					}
					else if (r_todo_board[ CMD_UNPOISON ] == true)
					{
						r_MIG_PP_CTRL_FSM = pp_unpoison;
					}
					break;
			
				case pp_poison :
#if 0
					if (p_in_core_rsp_1.read() && p_in_core_ack_1.read())
					{
						switch (p_in_core_cmd.read)
						{
							case MigControl::CTRL_VIRT_PP :
								r_virt_max_p_addr = p_in_core_data_0.read();
								r_MIG_PP_CTRL_FSM = pp_poison_ok;
								break;
							default :
								assert(false);
								break;
						}
					}
#endif
					break;

				case pp_poison_ok : // Notify the poison state and send the index of the page to be transfered
					r_MIG_PP_CTRL_FSM =  pp_send_home_addr;
					break;

				case pp_send_home_addr : // Send the virtual address of the page that was switched with the one to transfer now 
					r_MIG_PP_CTRL_FSM =  pp_idle;
					break;

				case pp_unpoison :
					// We go to idle, in order to avoid deadlocks or complexity at the ram_fsm. Because, the REQ_REGISTER state
					// of ram_fsm can be stalled waiting for a the lock, meanwhile we are here waiting for the ram_fsm to be
					// in IDLE and take our poison request.
					r_MIG_PP_CTRL_FSM = pp_unpoison_ok;
					break;

				case pp_unpoison_ok : // Notify the poison state and send the index of the page to be transfered
#if 0
					assert(*(p_in.m6_ack)); // Useless ack, but used to "vérifier" that everything is ok!
#endif
					// Wait some time before setting r_contention to false, hence, the contention created
					// by the page migration will not be taken into account!

					if (r_slave_transaction == false)
					{
						assert(r_master_transaction.read());
						r_master_transaction = false;
						// reset r_master_contention only if we were the master on this transaction to avoid multiple "masters" on
						// the ring. This is due to the fact that we will be able to raise another "contention" request when we
						// will retrieve the ring token for the previous "contention request", thus, we will duplicate all the
						// the commands and things will not work.
					}
					else
					{
						r_slave_transaction = false;
					}

					r_MIG_PP_CTRL_FSM = pp_finish;
					break;

				case pp_finish :
					r_MIG_PP_CTRL_FSM =  pp_idle;
					break;

			}

			switch((fsm_pt_state_e)r_MIG_PT_CTRL_FSM.read())
			{
				case pt_idle :
					if (r_todo_board[ CMD_UPDT_INV ] == true)
					{
						r_MIG_PT_CTRL_FSM = pt_updt_inv;
					}
					else if(r_todo_board[ CMD_UPDT_WIAM ] == true)
					{
						r_MIG_PT_CTRL_FSM = pt_updt_wiam;
					}
					break;

				case pt_updt_inv :
					assert(false);
#if 0
					if (*(p_in.m6_cmd) != CMD_UPDT_INV) cout << "error : m6_cmd == " << dec << *(p_in.m6_cmd) << endl;
					assert(*(p_in.m6_cmd) == CMD_UPDT_INV);
					D10COUT(1) << name() << "    --- page_new_addr 0x" << hex << (*(p_in.m6_data)) << endl;
					r_MIG_CTRL_FSM =  updt_table;
#endif
					break;

				case pt_updt_wiam :
					assert(false);
#if 0
					if (*(p_in.m6_cmd) != CMD_UPDT_WIAM) cout << "error : m6_cmd == " << dec << *(p_in.m6_cmd) << endl;
					assert(*(p_in.m6_cmd) == CMD_UPDT_WIAM);
					r_page_wiam_addr  = (*(p_in.m6_data)); // This is the second data
					r_MIG_CTRL_FSM =  updt_wiam_ok;
#endif
					break;

				case pt_updt_wiam_ok :
					assert(false);
#if 0
					if (p_in_core_ack_2.read())
					{
						D10COUT(1) << name() << " [updt_wiam]" << endl;
						r_MIG_PT_CTRL_FSM = pt_idle;
					}
#endif
					break;

				case pt_updt_table :
					assert(false);
#if 0
					D10COUT(1) << name() << " [updt_table]" << endl;
					if (p_in_core_ack_2.read())
					{
						r_MIG_PT_CTRL_FSM = pt_idle;
					}
#endif
					break;

				default :
					assert(false);
#if 0
					TODO
					case MigControl::CTRL_INV_OK :
#endif
					break;
			}
		}
}}
