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
#include "module_6.h"
#include "arithmetics.h"
#include "globals.h"

namespace soclib {
namespace caba {

	using namespace soclib;
	using namespace std;
	using soclib::common::uint32_log2;

#define tmpl(x)  x Module_6
	//#define tmpl(x) template< typename vci_param > x Module_6< vci_param>

#define m_m6_out_fifo_wok (m_m6_out_fifo_data->size() < m_m6_ring_fifo_size)
#define m_m6_out_fifo_rok (m_m6_out_fifo_data->size() > 0)
#define m_m6_in_fifo_wok (m_m6_in_fifo_data->size() < m_m6_ring_fifo_size)
#define m_m6_in_fifo_rok (m_m6_in_fifo_data->size() > 0)

#define m_m6_fifo_pending_lock_wok (m_m6_fifo_pending_lock->size() < m_m6_fifo_pending_lock_size)
#define m_m6_fifo_pending_lock_rok (m_m6_fifo_pending_lock->size() > 0)

#define m_m6_fifo_pending_updt_wok (m_m6_fifo_pending_updt_cmd->size() < m_m6_fifo_pending_updt_size)
#define m_m6_fifo_pending_updt_rok (m_m6_fifo_pending_updt_cmd->size() > 0)

#define p_out_fifo_wok ((p_out.fifo)->size() < m_m6_inter_fifo_size)
#define p_in_fifo_rok ((p_in.fifo)->size() > 0)


	tmpl(/**/)::Module_6(const string & insname,
			bool node_zero,
			unsigned int base_address,
			unsigned int line_size,
			unsigned int nb_mem,
			unsigned int nb_procs,
			unsigned int id,
			unsigned int * cost_table,
			addr_to_homeid_entry_t * home_addr_table,
			VciMccRam_interface * ram):
		m_name(insname),
		m_node_zero(node_zero),
		m_base_addr(base_address),
		m_page_shift(uint32_log2(PAGE_SIZE)),
		m_memory_line_size(line_size),
		m_nb_memory_nodes(nb_mem),
		m_nb_processor_nodes(nb_procs),
		m_id(id),
		m_table_cost(cost_table),
		m_home_addr_table(home_addr_table),
		m_ram(ram),
		m_m6_fifo_pending_lock_size(nb_mem),
		m_m6_fifo_pending_updt_size(4),
		m_m6_ring_fifo_size(M6_RING_FIFO_SIZE),
		m_m6_inter_fifo_size(M6_M10_INTER_FIFO_SIZE),
		m_cycle(0),
		m_cycle_mig(0),
		m_counting_time(false),
		p_set(0),
		p_B(false)

	{
		CTOR_OUT
			//assert(m_nb_memory_nodes == 5);
		m_m6_out_fifo_data = new list< micro_ring_data_t >;
		m_m6_out_fifo_cmd  = new list< micro_ring_cmd_t >;
		m_m6_out_fifo_id   = new list< micro_ring_id_t >;
		m_m6_out_fifo_srcid   = new list< micro_ring_id_t >;

		m_m6_in_fifo_data = new list< micro_ring_data_t >;
		m_m6_in_fifo_cmd  = new list< micro_ring_cmd_t >;
		m_m6_in_fifo_id   = new list< micro_ring_id_t >;
		m_m6_in_fifo_srcid   = new list< micro_ring_id_t >;

		m_m6_fifo_pending_lock   = new list< micro_ring_id_t >;
		m_m6_fifo_pending_updt_cmd   = new list< micro_ring_cmd_t >;
		m_m6_fifo_pending_updt_data   = new list< micro_ring_data_t >;
		m_m6_fifo_pending_updt_srcid   = new list< micro_ring_id_t >;
		r_tr_memory_line_buffer_read = new unsigned int [m_memory_line_size];
		r_tr_memory_line_buffer_write = new unsigned int [m_memory_line_size];

	}

	tmpl(unsigned int)::address_to_id(unsigned int addr)
	{
		CWOUT << "WARNING : using hardwired decoding in " << name() << " at address_to_id method " << endl;
		D6COUT(2) << "The address is : 0x" << hex << addr <<  endl;
		D6COUT(2) << "---- the table ----" << hex << endl;
		for(unsigned int i = 0; i < m_nb_memory_nodes; i++)
		{
			D6COUT(2) << "0x"<< hex <<  m_home_addr_table[i].base_addr << " - 0x" << m_home_addr_table[i].home_id << endl;
			if (addr  >= (m_home_addr_table[i].base_addr ) && (addr < (m_home_addr_table[i].base_addr + m_home_addr_table[i].size)))
				return m_home_addr_table[i].home_id;
		}
		assert(false); // Address not found
		return 0;
	}

	tmpl(void)::reset()
	{
		r_ctrl_module_fsm  = c_idle ;
		r_updt_module_fsm  = u_idle ;
		r_loc_module_fsm   = l_idle ;
		r_tr_module_fsm    = t_idle ;

		r_m6_out_fsm       = out_idle;
		r_m6_in_fsm        = in_idle;

		r_ctrl_ring_locked    = m_node_zero; // At reset time, only the first node has the lock on the ring!
		
		r_ctrl_master         = false;
		r_ctrl_mig_c          = false;
		r_ctrl_in_transaction = false;
		r_ctrl_get_location   = false;
		r_ctrl_update_home    = false;
		r_ctrl_begin_transfer = false;
		r_ctrl_rcv_finish = false;
		r_ctrl_cycle_two      = false;

		r_loc_node_count_cpt  = 0;
		r_loc_location_found  = false;	
		r_loc_my_pressure_is_lower  = false;	
		r_updt_cycle_two      = false;
		r_updt_local_update   = false;
		r_updt_update_ok      = false;
		r_updt_wiam_ok        = false;
		r_updt_update_tlb     = false;
		r_updt_update_wiam    = false;

		r_tr_transfer_req_cpt = 0;
		r_tr_transfer_rsp_cpt = 0;
		r_tr_req_end          = false;
		r_tr_rsp_end          = false;
		r_tr_transfer_ok      = false;
		r_tr_dir_buffer_write.init(m_nb_processor_nodes);
		r_tr_dir_buffer_read.init(m_nb_processor_nodes);

		p_set = 0;
		p_B = false;
	}

	tmpl(void)::compute_transfer(fifo_control_t * in_fifo, fifo_control_t * out_fifo, bool valid_request, bool in_out_fifo_forward)
	{
		switch((transfer_fsm_state_e)r_tr_module_fsm)
		{
			case t_idle : // SYNC_IN : begin_transfer SYNC_ACK : begin_transfer, TO : transfer
				{
					D6COUT(0) << name() << " [t_idle]" << endl;
					if (r_ctrl_begin_transfer)
					{
						r_tr_module_fsm = t_transfer;
						r_ctrl_begin_transfer = false;
						D6COUT(2) << name() << "    <--- begin transfer" << endl;
						D6COUT(2) << name() << "    |--- begin transfer" << endl;
					}
					break;
				}
			case t_transfer : // IN : Ring_Transfer, OUT : memory_write, Ring_Transfer , TO : transfer_dir
				{
					bool has_write = false;
					D6COUT(0) << name() << " [t_transfer]" << endl;

					if((m_m6_in_fifo_rok) && valid_request && (!r_tr_rsp_end))  // On the intput
					{  
						assert(m_m6_in_fifo_cmd->front() == RING_TRANSFER);
						if ((r_tr_transfer_rsp_cpt % m_memory_line_size)  == (m_memory_line_size - 1))  // writing line buffer to memory if possible 
						{	
							if((m_ram->is_access_sram_possible()) && ((r_tr_transfer_rsp_cpt/m_memory_line_size) != (r_tr_transfer_req_cpt/m_memory_line_size))) // access possible
							{ 
								r_tr_memory_line_buffer_write[r_tr_transfer_rsp_cpt % m_memory_line_size] = m_m6_in_fifo_data->front();
								for(unsigned int i = 0; i < m_memory_line_size; i++) // writing back buffer to ram AFTER filling the last cell
								{
									m_ram->write_seg(r_ctrl_page_addr,r_tr_transfer_rsp_cpt - i, r_tr_memory_line_buffer_write[m_memory_line_size - 1 - i]);
								}	
								if (r_tr_transfer_rsp_cpt == ((PAGE_SIZE  >> 2) - 1)){
									r_tr_rsp_end = true;
									CWOUT << "WARNING : hardwired word size" << name() << endl;
									D6COUT(1) << " RECEIVED :" << dec <<  r_tr_transfer_rsp_cpt << " requests" <<  name() << endl;
								} 
							r_tr_transfer_rsp_cpt++;
								has_write = true; // Memory has been accessed
								assert(!in_fifo->get);
								in_fifo->get = true;
							}
						}
						else // filling the line buffer write
						{
							assert(!in_fifo->get);
							in_fifo->get = true;
							r_tr_memory_line_buffer_write[r_tr_transfer_rsp_cpt % m_memory_line_size] = m_m6_in_fifo_data->front();
							if (r_tr_transfer_rsp_cpt == ((PAGE_SIZE  >> 2) - 1))
							{
								r_tr_rsp_end = true;
								D6COUT(1) << "> RECEIVED :" << dec <<  r_tr_transfer_rsp_cpt << " requests" <<  name() << endl;
								CWOUT << "WARNING : hardwired word size" << name() << endl;
								assert(false);
							} 
							r_tr_transfer_rsp_cpt++;
						}
					}

					if((m_m6_out_fifo_wok) && (!in_out_fifo_forward) && (!r_tr_req_end) && (!out_fifo->put)) // On the output
					{  
						out_fifo->cmd  = RING_TRANSFER;
						out_fifo->id   = r_loc_location_found_id;
						out_fifo->srcid  = m_id;
						if ((r_tr_transfer_req_cpt % m_memory_line_size)  == 0) // copying a line buffer from memory if possible
						{	
							if((m_ram->is_access_sram_possible()) && (has_write == false)) // is possible, no writes on this cycle
							{ 
								for(unsigned int i = 0; i < m_memory_line_size; i++)
								{
									r_tr_memory_line_buffer_read[i] = m_ram->read_seg(r_ctrl_page_addr,r_tr_transfer_req_cpt + i);
								}	
								if (r_tr_transfer_req_cpt == ((PAGE_SIZE  >> 2) - 1)){
									r_tr_req_end = true;
									CWOUT << "WARNING : hardwired word size" << name() << endl;
									D6COUT(1) << " SENT :" << dec <<  r_tr_transfer_req_cpt << " requests" <<  name() << endl;
									assert(false);
								}
								out_fifo->data = r_tr_memory_line_buffer_read[r_tr_transfer_req_cpt % m_memory_line_size];
								out_fifo->put = true;
								r_tr_transfer_req_cpt++;
							}	
						}
						else // sending line buffer data on the ring
						{
							if (r_tr_transfer_req_cpt == ((PAGE_SIZE  >> 2) - 1))
							{
								r_tr_req_end = true;
								D6COUT(1) << "> SENT :" << dec <<  r_tr_transfer_req_cpt << " requests" <<  name() << endl;
								CWOUT << "WARNING : hardwired word size" << name() << endl;
							}
							out_fifo->put = true;
							out_fifo->data = r_tr_memory_line_buffer_read[r_tr_transfer_req_cpt % m_memory_line_size];
							r_tr_transfer_req_cpt++;
						}

					}

					if ((r_tr_rsp_end) && r_tr_req_end) // Transfer done, going to transfer the directory
					{
						r_tr_module_fsm = t_transfer_dir;
						r_tr_transfer_req_cpt = 0;
						r_tr_transfer_rsp_cpt = 0;
						r_tr_req_end = false;
						r_tr_rsp_end = false;
					}
					break;
				}
			case t_transfer_dir : // IN : Ring_Transfer, OUT : dir_write, Ring_Transfer, SYNC_OUT : transfer_ok TO : idle
				{
					bool has_write = false;
					D6COUT(0) << name() << " [t_transfer_dir]" << endl;

					if(m_m6_in_fifo_rok && valid_request && (!r_tr_rsp_end)) // On the input
					{ 
						assert(m_m6_in_fifo_cmd->front() == RING_TRANSFER);
						if((m_ram->is_access_dir_possible()) && (r_tr_transfer_rsp_cpt != r_tr_transfer_req_cpt)) // Directory is accessible
						{ 
							r_tr_dir_buffer_write.Set_slice(0,m_m6_in_fifo_data->front());
							m_ram->write_dir(r_ctrl_page_addr,r_tr_transfer_rsp_cpt, &r_tr_dir_buffer_write);
							if (r_tr_transfer_rsp_cpt == (((PAGE_SIZE  >> 2)/m_memory_line_size) - 1))
							{
								r_tr_rsp_end = true;
								CWOUT << "WARNING : hardwired word size" << name() << endl;
								D6COUT(1) << "> RECEIVED :" << dec <<  r_tr_transfer_rsp_cpt << " requests" <<  name() << endl;
							} 
							r_tr_transfer_rsp_cpt++;
							assert(!in_fifo->get);
							in_fifo->get = true;
							has_write = true; 
						}
					}

					if((m_m6_out_fifo_wok)  && (!in_out_fifo_forward) && (!r_tr_req_end) && (!out_fifo->put)) // On the output
					{
						out_fifo->cmd  = RING_TRANSFER;
						out_fifo->id   = r_loc_location_found_id; 
						out_fifo->srcid = m_id;
						if((m_ram->is_access_dir_possible()) && (has_write == false)) // Ram is accessible, and we have not written on this cycle!
						{ 
							out_fifo->put = true;
							r_tr_dir_buffer_read = *(m_ram->read_dir(r_ctrl_page_addr,r_tr_transfer_req_cpt));
							out_fifo->data = r_tr_dir_buffer_read.Get_slice(0);
							if (r_tr_transfer_req_cpt == (((PAGE_SIZE  >> 2)/m_memory_line_size) - 1))
							{
								r_tr_req_end = true;
								CWOUT << "WARNING : hardwired word size" << name() << endl;
								D6COUT(1) << "> SENT :" << dec <<  r_tr_transfer_req_cpt << " requests" <<  name() << endl;
							}
							r_tr_transfer_req_cpt++;
						}	

					}

					if ((r_tr_rsp_end) && r_tr_req_end) // transfer done, going to idle.
					{
						D6COUT(2) << name() << "    ---> transfer_ok " << endl;
						r_tr_transfer_ok = true;
						r_tr_module_fsm = t_idle;
						r_tr_transfer_req_cpt = 0;
						r_tr_transfer_rsp_cpt = 0;
						r_tr_req_end = false;
						r_tr_rsp_end = false;
					}
					break;
				}
			default : // Error
				{
					assert(false);
					break;
				}
		}
	}

	tmpl(void)::compute_control(fifo_control_t * in_fifo, fifo_control_t * out_fifo, bool valid_request, bool in_out_fifo_forward)
	{
		switch((control_fsm_state_e)r_ctrl_module_fsm)
		{
			case c_idle :// IN : M10.contention, Ring_select, Ring_lock    OUT :  SYNC_IN : transfer_ok, SYNC_OUT : ring_locked, in_transation, get_location, update_home, begin_transfer SYNC_ACK : location_found
				{
					D6COUT(0) << name()  << " [c_idle]" << endl;
					if(*(p_in.req))  // Requests on the M10 command interface (not fifo) ie CONTENTION.
					{
						assert((*(p_in.cmd)==CMD_IS_CONTENTION) || (*(p_in.cmd)==CMD_PE_MIG));
						r_ctrl_mig_c = (*(p_in.cmd)==CMD_IS_CONTENTION); 

						D6COUT(2) << name() << "    <--- contention" << endl;

						if (r_ctrl_ring_locked)	// send a request for location
						{
							assert(!r_ctrl_in_transaction);
							r_ctrl_master = true;
							r_ctrl_get_location   = true;
							r_ctrl_in_transaction = true;
							D6COUT(2) << name() << "    ---> get_location" << endl;
							D6COUT(2) << name() << "    ---> master" << endl;
							D6COUT(2) << name() << "    ---> in transaction (master)" << endl;
							D6COUT(2) << name() << "    ---> (already ring locked)" << endl;
							r_ctrl_module_fsm = c_notify_m_control;
						}
						else			// get a lock on the ring
							r_ctrl_module_fsm = c_lock_ring;
					}
					else if (m_m6_in_fifo_rok && valid_request && !(*(p_in.req))) // Take requests from the in_fifo
					{ 
						switch(m_m6_in_fifo_cmd->front()) // SELECT / LOCK requests
						{
							case RING_SELECT :
								{
									if (r_ctrl_cycle_two)
									{
										r_ctrl_cycle_two = false;
										D6COUT(0) << name()  << ">select on fifo " << endl;
										// On arrive ici parce que qq d'autre a initié un transfert et nous a sélectionné!
										// Il se doit de nous fournir l'adresse de la page qu'il a sélectionné pour le transfert. Ainsi,
										// on pourra fournir cette nouvelle traduction pour le home.
										assert(!in_fifo->get);
										in_fifo->get  = true;
										r_ctrl_page_selected = m_m6_in_fifo_data->front(); // The elected node sends us the evicted page address
										if (r_ctrl_master)
										{
											r_ctrl_module_fsm = c_idle;
											r_ctrl_update_home = true;
											r_ctrl_begin_transfer = true;
											assert(r_loc_location_found_id == m_m6_in_fifo_srcid->front());
											D6COUT(2) << name() << "    ---> update_home" << endl;
											D6COUT(2) << name() << "    ---> begin_transfer" << endl;
											D6COUT(2) << name() << "    ---  page_selected " << hex << (void*) r_ctrl_page_selected << " ---" << endl;
										}
										else if(m_m6_in_fifo_id->front() == m_id) // We have been elected as the destination node
										{
											r_ctrl_module_fsm = c_elect; 
											r_loc_location_found_id = m_m6_in_fifo_srcid->front();
											r_ctrl_in_transaction = true;
											D6COUT(2) << name() << "    ---> in transaction (elect)" << endl;
											D6COUT(2) << name() << "    --- F PAGE SELECTED : " << hex << (void*) r_ctrl_page_selected << " ---" << endl;
											D6COUT(2) << name() << "    --- location found_id " << dec << r_loc_location_found_id << " ---" << endl;
										}
										else	// not for us, forward !
										{  
											assert(false); // Ceci est supposément fait en amont de cette machine à états! (sorte de valid bit qq part)
										}
									}
									else
									{
										r_ctrl_cycle_two = true;
										assert(!in_fifo->get);
										in_fifo->get  = true;
										r_ctrl_page_wis = m_m6_in_fifo_data->front(); // The elected node sends us the evicted wiam address 
										D6COUT(2) << name() << "    ---  page_wis " << hex << (void*) r_ctrl_page_wis << " ---" << endl;
									}
									break;
								}
							case RING_LOCK : // We received a lock request
								{
									D6COUT(1) << name()  << ">lock on fifo " << endl;
									if (m_m6_in_fifo_id->front() == m_id)	// we sent this request, so we have the lock granted
										// ----> loc get_location
									{
										assert(!in_fifo->get);
										in_fifo->get  = true;
										if (m_m6_in_fifo_data->front() == 1){
											r_ctrl_module_fsm = c_notify_m_control;
											r_ctrl_ring_locked    = true;
											r_ctrl_master = true;
											r_ctrl_get_location   = true;
											r_ctrl_in_transaction = true;
											D6COUT(2) << name() << "    ---> in transaction (master)" << endl;
											D6COUT(2) << name() << "    ---> master" << endl;
											D6COUT(2) << name() << "    ---> ring locked" << endl;
											D6COUT(2) << name() << "    ---> get_location" << endl;
										}
										else
										{
											assert(m_m6_in_fifo_data->front() == 0);
											r_ctrl_module_fsm = c_lock_ring; // Try again to obtain the lock!
											D6COUT(1) << name() << "    RETRY getting lock! " << endl;
										}
									}
									else
									{
										assert(false);
									}
									break;
								}
							case RING_FINISH :
										assert(!in_fifo->get);
										in_fifo->get  = true;
										r_ctrl_rcv_finish = true;
										D6COUT(1) <<  hex << name() << "                                       | received FINISH "<< std::endl;
											// We received a RING_FINISH request, but we will take it into account
											// in the c_finish state!
								break;
							default :
								{
									cout <<  name() << "Errror , message  ->" << dec << (m_m6_in_fifo_cmd->front()) << endl; 
									assert(false);
									break;
								}
						}
					}
					else if (r_updt_wiam_ok && r_updt_update_ok && r_tr_transfer_ok) // Transfer is over, TLB's update & inv over.. the page has migrated!
					{
						r_ctrl_module_fsm = c_unpoison;
						assert(r_ctrl_in_transaction); // We are in a transaction!
						D6COUT(2) << name() << "    <--- update_ok" << endl;
						D6COUT(2) << name() << "    <--- transfer_ok" << endl;
					}
					else if (r_updt_update_tlb)
					{
						r_ctrl_module_fsm = c_update_tlb;	
						D6COUT(2) << name() << "    <--- update_tlb" << endl;
					}
					else if (r_updt_update_wiam)
					{
						r_ctrl_module_fsm = c_update_wiam;	
						D6COUT(2) << name() << "    <--- update_wiam" << endl;
					}
					else if (r_loc_location_found && r_loc_location_found_is_other)
					{
						D6COUT(2) << name() << "    <--- location_found" << endl;
						D6COUT(2) << name() << "    |--- location_found" << endl;
						r_ctrl_module_fsm = c_poison;
						r_loc_location_found = false;
					}
					else if (r_loc_location_found && !r_loc_location_found_is_other)
					{
						D6COUT(2) << name() << "    <--- location_found" << endl;
						D6COUT(2) << name() << "    |--- location_found" << endl;
						D6COUT(2) << name() << "    abort migration "  << hex <<  "0x"
						<< ((r_ctrl_page_addr << m_page_shift) + m_base_addr) << std::endl;
						r_ctrl_module_fsm = c_abort;
						r_loc_location_found = false;
					}
					else
									D6COUT(0) << name()  << ">nothing???? " << endl;
					break;
				}

			case c_abort :
				{
					if (*(p_in.ack))
					{
						r_ctrl_module_fsm = c_idle;
						r_ctrl_master = false;
						r_ctrl_in_transaction = false;
						m_cycle_mig = 0;
						D6COUT(2) << name() << "    |--- master" << endl;
						D6COUT(2) << name() << "    |--- in_transaction" << endl;
					}
					break;
				}

			case c_update_wiam : // OUT : CMD_UPDT_INV  SYNC_ACK update_tlb
				{
					D6COUT(0) << name()  << " [u_update_wiam]" << endl;
					r_updt_update_wiam = false;
					if (*(p_in.ack))
					{
						if (r_ctrl_cycle_two)
						{
							r_ctrl_cycle_two = false;
							D6COUT(2) << name() << "    |--- update_wiam" << endl;
							r_ctrl_module_fsm = c_idle;
						}
						else
						{
							r_ctrl_cycle_two = true;
						}
					}
					break;
				}

			case c_update_tlb : // OUT : CMD_UPDT_INV  SYNC_ACK update_tlb
				{
					D6COUT(0) << name()  << " [c_update_tlb]" << endl;
					r_updt_update_tlb = false;
					if (*(p_in.ack))
					{
						if (r_ctrl_cycle_two)
						{
							D6COUT(2) << name() << "    |--- update_tlb" << endl;
							r_ctrl_cycle_two = false;
							r_ctrl_module_fsm = c_idle;
						}
						else
						{
							r_ctrl_cycle_two = true;
						}
					}
					break;
				}
			case c_lock_ring : // OUT : Ring_lock TO : idle
				{
					D6COUT(0) << name() << " [c_lock_ring]" << endl;
					// We lock the ring
					if (m_m6_out_fifo_wok  && (!in_out_fifo_forward) && (!out_fifo->put)) // send fifo request and go to idle (if possible)
					{
						out_fifo->data = 0;
						D6COUT(2) << name() << "    > SENDING LOCK REQUEST" << endl;
						out_fifo->cmd  = RING_LOCK;
						out_fifo->id   = m_id;
						out_fifo->srcid = m_id;
						out_fifo->put = true;
						r_ctrl_module_fsm = c_idle;
					}
					break;
				}
			case c_elect :     // OUT : CMD_ELECT,   TO : poison
				{
					if (*(p_in.ack)) // ack from M10
						D6COUT(2) << name() <<  " [c_elect : ok]" << endl;
						r_ctrl_module_fsm = c_poison;
					break;
				}
			case c_poison :    // OUT : CMD_POISON,  TO : poison_wait
				{
					m_counting_time = true;
					if (*(p_in.ack))
					{
						D6COUT(2) << name() <<  " [c_poison : ok]" << endl;
						r_ctrl_module_fsm = c_poison_wait;
					}
					break;
				}
			case c_notify_m_control :    // OUT : CMD_POISON,  TO : poison_wait
				{
					D6COUT(0) << name() <<  " [c_notify_m_control]" << endl;
					if (*(p_in.ack))
						r_ctrl_module_fsm = c_idle;
					break;
				}

			case c_poison_wait : // IN : CMD_IS_POISONNED, SET : r_ctrl_page_addr    TO : wait_page_addr_home
				{
					if (*(p_in.req)) // page is poissoned, proceed
					{
						D6COUT(2) << name() << " [c_poison_wait : ok]" << endl;
						assert(*(p_in.cmd) == CMD_IS_POISONNED);
						r_ctrl_module_fsm = c_wait_page_addr_home;
						r_ctrl_page_addr = *(p_in.data); // contains the address of the poisonned page
						D6COUT(2) << name() << "    --- page_addr 0x"   << hex << *(p_in.data) << endl;
					}
					break;
				}

			case c_wait_page_addr_home : // : CMD_PAGE_ADDR_HOME, SET : r_ctrl_page_addr_home,  TO : notify_selected  
				{
					D6COUT(0) << name() << " [c_page_addr_home]" << endl;
					if (*(p_in.req))
					{
						assert(*(p_in.cmd) == CMD_PAGE_ADDR_HOME);
						r_ctrl_module_fsm = c_notify_selected;
						r_ctrl_page_addr_home = *(p_in.data);
						D6COUT(2) << name() << "    --- page_addr_home 0x"   << hex << *(p_in.data) << endl;
					}
					break;
				}
			case c_notify_selected :  // OUT : Ring_Select SYNC_OUT : update_home, begin_transfer TO : idle 
				{
					D6COUT(1) << name() << " [c_notify_selected]" << endl;
						if (m_m6_out_fifo_wok  && (!in_out_fifo_forward) && (!out_fifo->put))
						{
							out_fifo->cmd  = RING_SELECT;
							out_fifo->srcid = m_id;
							out_fifo->put = true;

							if ((r_ctrl_cycle_two) && (r_ctrl_master))
							{
								if (r_ctrl_mig_c)
								{
									//D6COUT(3) <<  hex << name() << " C -0x" << ((r_ctrl_page_addr_home << m_page_shift) + m_base_addr) << " <--";
									D6COUT(3) << " " << std::dec << m_cycle << " " <<  hex << name() <<  " C -0x" << ((r_ctrl_page_addr_home )) << " <--";
								}
								else
								{
									D6COUT(3) <<  " " << std::dec << m_cycle << " " << hex << name() << " E -0x" << ((r_ctrl_page_addr_home)) << " <--";
								}
							}
							if ((r_ctrl_cycle_two) && (!r_ctrl_master))
							{
								D6COUT(3) << hex << "--> 0x" << ((r_ctrl_page_addr_home)) <<"-"<< name() << endl;
								//D6COUT(3) << hex << "--> 0x" << ((r_ctrl_page_addr_home << m_page_shift) + m_base_addr) <<"-"<< name() << endl;
							}
							if(r_ctrl_cycle_two)
							{
								r_ctrl_cycle_two = false;
								out_fifo->data = ((r_ctrl_page_addr << m_page_shift) + m_base_addr);
								r_ctrl_module_fsm = c_idle; 
							}
							else
							{
								r_ctrl_cycle_two = true;
								out_fifo->data = r_ctrl_page_addr_home;
							}
							if (r_ctrl_master) // send to selected node
							{
								out_fifo->id   = r_loc_location_found_id;
							}
							else  // send to the initiator node
							{ 
								CWOUT << "warning  : some clean here needed" << endl;
								if (r_ctrl_cycle_two){
									r_ctrl_update_home = true;
									r_ctrl_begin_transfer = true;
									D6COUT(2) << name() << "    ---> update_home" << endl;
									D6COUT(2) << name() << "    ---> begin_transfer" << endl;
								}
								out_fifo->id   = m_id; // This ensures that the request will try to do a loop.
								// only the one in transaction and me (the one with the same
								// id) should take into account this request
							}
						}
					break;
				}
			case c_unpoison :		  // OUT : CMD_UNPOISON , TO : finish
				{
					D6COUT(2) << name() << " [c_unpoison]" << endl;
					if (*(p_in.ack))
					{
						r_ctrl_module_fsm = c_unpoison_ok;
					}

					break;
				}
			case c_unpoison_ok :  // IN : CMD_UNPOISON_OK,SYNC_ACK : transfer_ok, update_ok, master
				{
					D6COUT(2) << name() << " [c_unpoison_ok]" << endl;
					// Here we unpoison the directory
					if (*(p_in.req))
					{
						D6COUT(2) << name() << " in CMD : CMD_POISON_OK" << endl;
						assert(*(p_in.cmd) == CMD_UNPOISON_OK);
						r_ctrl_module_fsm = c_finish;
						//if (r_ctrl_master) D6COUT(3) << endl;
						m_counting_time = false;
					}
					break;
				}
			case c_finish :
			{
				if (r_ctrl_master) // WAIT for RING_FINISH command in order to release lock and end transaction.
				{
					if (m_m6_in_fifo_rok && valid_request ) // Take requests from the in_fifo
					{ 
						assert(!r_ctrl_rcv_finish);
						assert(!in_fifo->get);
						in_fifo->get  = true;
						assert(m_m6_in_fifo_cmd->front() == RING_FINISH);
						D6COUT(2) << name() << "    |--- master" << endl;
						r_ctrl_module_fsm = c_idle;

						r_ctrl_master = false;
						r_ctrl_in_transaction = false;
						m_cycle_mig = 0;
						r_tr_transfer_ok      = false;
						r_updt_update_ok      = false;
						r_updt_wiam_ok        = false;
						D6COUT(2) << name() << "    |--- update_ok" << endl;
						D6COUT(2) << name() << "    |--- updt_wiam_ok" << endl;
						D6COUT(2) << name() << "    |--- transfer_ok" << endl;
						D6COUT(2) << name() << "    |--- in_transaction" << endl;
					}
					else if (r_ctrl_rcv_finish)
					{
						r_ctrl_rcv_finish = false;
						D6COUT(2) << name() << "    |--- master" << endl;
						r_ctrl_module_fsm = c_idle;
						r_ctrl_master = false;
						r_ctrl_in_transaction = false;
						m_cycle_mig = 0;
						r_tr_transfer_ok      = false;
						r_updt_update_ok      = false;
						r_updt_wiam_ok        = false;
						D6COUT(2) << name() << "    |--- update_ok" << endl;
						D6COUT(2) << name() << "    |--- updt_wiam_ok" << endl;
						D6COUT(2) << name() << "    |--- transfer_ok" << endl;
						D6COUT(2) << name() << "    |--- in_transaction" << endl;
					}
				}

				else  // SEND a RING_FINISH command to the master, hence, he knows that we have finished and can realase the ring lock
				{
					assert(!r_ctrl_rcv_finish);
					if (m_m6_out_fifo_wok  && (!in_out_fifo_forward) && (!out_fifo->put)) // send fifo request and go to idle (if possible)
					{
						out_fifo->data = 0;
						D6COUT(2) << name() << "    > SENDING FINISH REQUEST" << endl;
						out_fifo->cmd  = RING_FINISH;
						out_fifo->id   = r_loc_location_found_id;
						out_fifo->srcid = m_id;
						out_fifo->put = true;

						r_ctrl_module_fsm = c_idle;
						r_ctrl_in_transaction = false;
						m_cycle_mig = 0;
						r_tr_transfer_ok      = false;
						r_updt_update_ok      = false;
						r_updt_wiam_ok        = false;
						D6COUT(2) << name() << "    |--- update_ok" << endl;
						D6COUT(2) << name() << "    |--- updt_wiam_ok" << endl;
						D6COUT(2) << name() << "    |--- transfer_ok" << endl;
						D6COUT(2) << name() << "    |--- in_transaction" << endl;
					}
				}
				break;
			}
			default :  // Error 
				{
					assert(false);
					break;
				}
		}
	}

	tmpl(void)::compute_update(bool * fifo_pending_updt_get, fifo_control_t * out_fifo, bool valid_request, bool in_out_fifo_forward)
	{
		switch((update_fsm_state_e)r_updt_module_fsm)
		{
			case u_idle : // IN : Ring_Locate , SYNC_IN : update_home, is_local_update, SYNC_ACK : update_home,  TO update_tlb, receive_update_tlb SYNC_OUT :update_ok
				{
					D6COUT(0) << name()  << " [u_idle]" << endl;
					assert(!(valid_request ^ m_m6_fifo_pending_updt_rok)); 

					if (valid_request ) // Take requests from the in_fifo
					{ 
						switch(m_m6_fifo_pending_updt_cmd->front()) 
						{
							case RING_UPDATE : // TO : receive_update_tlb
								{
									r_updt_module_fsm = u_receive_update_tlb;  
									break;
								}
							case RING_UPDATE_ACK : // SYNC_OUT : update_ok
								{
									r_updt_update_ok = true;
									*fifo_pending_updt_get = true;
									D6COUT(2) << name() << "    ---> update_ok" << endl;
									break;
								}
							default :
								{
									cout <<  name() << "Errror , fsm ->" << dec << r_updt_module_fsm << endl; 
									assert(false);
									break;
								}
						}
					}

					else if (r_ctrl_update_home) // Local request, go to send_update and ack the update_home request
					{
						r_updt_module_fsm = u_send_updt_home;
						r_ctrl_update_home = false;
						D6COUT(2) << name() << "    <--- update_home" << endl;
						D6COUT(2) << name() << "    |--- update_home" << endl;
					}
					break;
				}
			case u_receive_update_tlb : // IN : Ring_Locate, SET : r_page_local_addr, r_page_new_addr TO : tlb_update SYNC_OUT : update_tlb
				{
					D6COUT(0) << name()  << " [u_receive_update_tlb]" << endl;
					D6COUT(1) << name() << "    waiting for valid req in receive update tlb" << endl;
					if (valid_request)
					{
						assert(m_m6_fifo_pending_updt_rok);
						if  (r_updt_cycle_two)
						{
							r_updt_page_new_addr = m_m6_fifo_pending_updt_data->front();
							D6COUT(2) << name() << "    --- new_page addr 0x" <<  hex <<  m_m6_fifo_pending_updt_data->front() << " received from " << m_m6_fifo_pending_updt_srcid->front() <<  endl;
							*fifo_pending_updt_get = true;
							r_updt_cycle_two = false;
							r_updt_module_fsm = u_tlb_inv_ok;
							r_updt_update_tlb = true;
							r_updt_save_src_id = m_m6_fifo_pending_updt_srcid->front();
						}
						else
						{
							r_updt_page_local_addr = m_m6_fifo_pending_updt_data->front();
							D6COUT(2) << name() << "    --- local_page addr 0x" << hex <<  m_m6_fifo_pending_updt_data->front() << endl;
							r_updt_cycle_two = true;
							*fifo_pending_updt_get = true;
						}
					}
					break;
				}
			case u_send_updt_home : // OUT : Ring_Update, SET : r_page_new_addr, r_page_local_addr, SYNC_OUT : local_update , update_tlb TO idle, update_tlb
				{
					D6COUT(0) << name()  << " [u_send_update_home]" << endl;
					r_updt_update_wiam = true;
					D6COUT(2) << name() << "    ---> update_wiam" << endl;
					r_updt_wiam_addr = r_ctrl_page_wis; // the who_is tag will be used to update the who_i_am field of the page table entry
					if (address_to_id(r_ctrl_page_addr_home) == m_id)
					{
						r_updt_local_update = true;
						r_updt_update_tlb = true;
						r_updt_module_fsm = u_tlb_inv_ok;
						r_updt_page_local_addr = r_ctrl_page_addr_home;
						r_updt_page_new_addr = r_ctrl_page_selected;
						D6COUT(2) << name() << "    --- local_page addr 0x" << hex << r_ctrl_page_addr_home << endl;
						D6COUT(2) << name() << "    --- new_page addr 0x"   << hex << r_ctrl_page_selected << endl;
					}
					else
					{
						D6COUT(1) << name() << "    --- waiting in u_send_updt_home "<< endl;
						if(m_m6_out_fifo_wok  && (!in_out_fifo_forward) && (!out_fifo->put))
						{
							D6COUT(0) << name() << "    > sent RING_UPDATE " << endl;
							out_fifo->put = true;
							out_fifo->cmd  = RING_UPDATE;
							out_fifo->id   = address_to_id(r_ctrl_page_addr_home);
							out_fifo->srcid  = m_id;
							if (r_updt_cycle_two)
							{	
								out_fifo->data = (r_ctrl_page_selected);
								r_updt_module_fsm = u_idle;
								r_updt_cycle_two = false;
							}
							else
							{
								r_updt_cycle_two = true;
								out_fifo->data = (r_ctrl_page_addr_home) ;
							}
						}
					}
					break;
				}
			case u_tlb_inv_ok : // SYNC_OUT : update_ok, SYNC_ACK : local_update, SYNC_IN : local_update  TO : idle, send_updt_ack_home
				{
					D6COUT(0) << name()  << " [u_tlb_inv_ok]" << endl;
					if ((r_updt_inv_tlb_ok))
					{
						r_updt_inv_tlb_ok = false;
						D6COUT(2) << name() << "    |--- tlb_inv_ok" << endl;
						if (r_updt_local_update)
						{
							D6COUT(2) << name() << "    <--- local_update" << endl;
							D6COUT(2) << name() << "    |--- local_update" << endl;
							D6COUT(2) << name() << "    ---> update_ok" << endl;
							r_updt_local_update = false;
							r_updt_update_ok = true;
							r_updt_module_fsm = u_idle;
						}
						else
						{
							r_updt_module_fsm = u_send_updt_ack_home;
							D6COUT(1) << name() << "    --- going to u_send_updt_ack_home "<< endl;
						}
					}else{
							D6COUT(1) << name() << "   ----------------- waiting for updt_inv_tlb_ok!!!!" << endl;
					
					}

					break;
				}
			case u_send_updt_ack_home : // OUT : Ring_Update_Ack  TO : idle
				{
					D6COUT(0) << name()  << " [u_send_updt_ack_home]" << endl;
					D6COUT(1) << name() << "    --- wok, forw, put "<< m_m6_out_fifo_wok << " " <<  (!in_out_fifo_forward) << " " <<(!out_fifo->put) << endl;
					if(m_m6_out_fifo_wok  && (!in_out_fifo_forward) && (!out_fifo->put))
					{
						out_fifo->cmd  = RING_UPDATE_ACK;
						out_fifo->id   = r_updt_save_src_id;
						out_fifo->srcid   = m_id;
						out_fifo->put = true;
						D6COUT(1) << name() << "    > sent RING_UPDATE_ACK, to id :  " << r_updt_save_src_id <<  endl;
						r_updt_module_fsm = u_idle;
					}
					break;
				}
			default :  // Error
				{
					assert(false);
					break;
				}
		} 
	}

	tmpl(void)::compute_location(fifo_control_t * in_fifo, fifo_control_t * out_fifo, bool * inter_in_fifo_get, bool valid_request, bool in_out_fifo_forward)
	{
		switch((location_fsm_state_e)r_loc_module_fsm)
		{
			case l_idle :  // IN : Ring_Locate, SYNC_IN : get_location TO : compute_location
				{
					D6COUT(0) << name()  << " [l_idle]" << endl;

					if (m_m6_in_fifo_rok && valid_request) // Take requests from the in_fifo
					{ 
						switch(m_m6_in_fifo_cmd->front()) // RING_LOCATE
						{
							case RING_LOCATE : // IN : Ring_Locate, TO : compute_pressure
								{
									r_loc_module_fsm = l_compute_pressure;  
									r_loc_location_cost = 0;
									D6COUT(0) << name() << "           -> RING_LOCATE" << endl;
									assert(r_loc_node_count_cpt == 0);
									break;
								}
							case RING_COST :
								{
									assert(false);
									break;
								}
							default :
								{
									cout <<  name() << "Errror , fsm ->" << dec << r_ctrl_module_fsm << endl; 
									assert(false);
									break;
								}
						}
					}else if(r_ctrl_get_location){
						r_loc_module_fsm = l_req_loc;  
						D6COUT(2) << name() << "    <--- get_location" << endl;
					}
					break;
				}
			case l_req_loc : // IN : p.in_fifo, SYNC_ACK : get_location, OUT : Ring_Locate,  TO : idle
				{
					// Sending pages addresses to other memory controllers, waiting for the response.
					D6COUT(0) << name() << " [l_req_loc] rok, wok, put " << p_in_fifo_rok << " " << m_m6_out_fifo_wok << " " << out_fifo->put <<  endl;
					if (m_m6_out_fifo_wok  && (!in_out_fifo_forward) && p_in_fifo_rok && (!out_fifo->put)) // Send a Ring_Locate if possible (out_wok, in_rok and get_location)
					{
						out_fifo->put = true;
						D6COUT(0) << name() << "              -> id " << dec << (int)(p_in.fifo)->front() << endl;
						#if 0
						if (r_loc_node_count_cpt == 0)
						{
							out_fifo->data = (*(p_in.pressure)); // The first paquet contains the pressure
						}
						else
						{
							out_fifo->data = (p_in.fifo)->front();
							*inter_in_fifo_get = true;
						}
						#else
							// the first paquet contains the pressure!
							out_fifo->data = (p_in.fifo)->front();
							*inter_in_fifo_get = true;
						#endif
						out_fifo->cmd  = RING_LOCATE;
						D6COUT(0) << name() << "> Send RING_LOCATE" << endl;

						out_fifo->id   = m_id;
						out_fifo->srcid = m_id;

						if (r_loc_node_count_cpt == m_nb_processor_nodes) //   m_nb_processor_nodes processors + 1 pressure count!
						{  
							r_ctrl_get_location = false;
							D6COUT(2) << name() << "    |--- get_location" << endl;
							r_loc_node_count_cpt = 0;
							r_loc_module_fsm = l_idle; 
						}
						else  // Send N nodes (of the most accessed page)
						{
							r_loc_node_count_cpt++;
						}

					}
					break;
				}

			case l_compute_pressure : // IN : Ring_locate, OUT : Ring_Locate TO : send_cost_location / wait_loc_id 
				{
					if (valid_request &&  m_m6_in_fifo_rok && m_m6_out_fifo_wok  && (!in_out_fifo_forward) && (!out_fifo->put))
					{
						assert(!in_fifo->get);
						in_fifo->get = true;
						assert(m_m6_in_fifo_cmd->front() == RING_LOCATE);
						#if 0
						#ifdef THRES_P
						r_loc_my_pressure_is_lower = ((*(p_in.pressure))) < 600;
						#else
						r_loc_my_pressure_is_lower = ((uint32_t)m_m6_in_fifo_data->front() >= (*(p_in.pressure)));
						#endif
						#else
						r_loc_my_pressure_is_lower = (((uint32_t)m_m6_in_fifo_data->front() + (*(p_in.pressure))) <  SAT_THRESHOLD);
						#endif


						r_loc_module_fsm = l_compute_location; 

						if (m_m6_in_fifo_id->front() != m_id) // In all cases (except I am the requierer), forward this message to others nodes
						{ 
							out_fifo->put  = true;
							out_fifo->data = m_m6_in_fifo_data->front();
							out_fifo->cmd  = m_m6_in_fifo_cmd->front();
							out_fifo->id   = m_m6_in_fifo_id->front();
							out_fifo->srcid = m_m6_in_fifo_srcid->front();
						}
					}
					break;
				}

			case l_compute_location : // IN : Ring_locate, OUT : Ring_Locate TO : send_cost_location / wait_loc_id 
				{
					D6COUT(0) << name() << " [l_compute_location] rok, wok, put " << m_m6_in_fifo_rok << " " 
					<< m_m6_out_fifo_wok << " " << out_fifo->put <<  endl;
					if (valid_request &&  m_m6_in_fifo_rok && m_m6_out_fifo_wok  && (!in_out_fifo_forward) && (!out_fifo->put))
					{
						// Wok because we will forward this request now, so no buffer required
						// and the following node can begin to compute location
						// There are no deadlock possible since we have locked the RING so the maximum
						// requests at this time in the ring are : 
						// N_MEM_NODES - 1  RING_LOCK requests + N_PROC_NODE LOCATE requests. This
						// number must be lower than the buffer capacity of the node, each node is either
						// in compute_location (so no ring_lock can comme from behin except for the
						// initiator). or in Idle, and RING_LOCK can be managed. or, trying to lock (ie
						// lock_ring state). This later case implies that the node wich follows is
						// either in idle or in lock_ring state. If the buffer capacity is respected,
						// there is no deadlock possible!
						assert(!in_fifo->get);
						in_fifo->get = true;
						assert(m_m6_in_fifo_cmd->front() == RING_LOCATE);
						#if 0
						if ((int)m_m6_in_fifo_data->front() != -1) // Accumulate the cost except for -1 (is a magic number in the counters : no-value)
						{
							p_set = p_set | (0x1 << (int)m_m6_in_fifo_data->front());
							assert((int)m_m6_in_fifo_data->front() < m_nb_processor_nodes);
							if (!r_loc_my_pressure_is_lower) r_loc_location_cost = M6_MAX_LOC_COST ;
							else
							{

								r_loc_location_cost = r_loc_location_cost + m_table_cost[m_m6_in_fifo_data->front()];  
							}
						}else{
							if (!r_loc_my_pressure_is_lower) r_loc_location_cost = M6_MAX_LOC_COST ;
						}
						#else
							p_set = 0x1 << m_nb_processor_nodes;
							if (!r_loc_my_pressure_is_lower) r_loc_location_cost = M6_MAX_LOC_COST ;
							else
							{

								r_loc_location_cost = r_loc_location_cost + (m_table_cost[r_loc_node_count_cpt] << m_m6_in_fifo_data->front());  
								D6COUT(1) << name()  << " " << dec << r_loc_node_count_cpt 
								                     << " -> "<< m_m6_in_fifo_data->front() 
										     << "  s "<< r_loc_location_cost << endl;
							}
						#endif
						assert(r_loc_node_count_cpt < m_nb_processor_nodes);
						if (r_loc_node_count_cpt == m_nb_processor_nodes - 1) // If N nodes computed, go to send_cost_location if I am not the requirer
						{ 
							
							D6COUT(1) << name() << dec << " T: "<< r_loc_location_cost << endl;
							r_loc_node_count_cpt = 0;
							r_loc_save_req_id = m_m6_in_fifo_id->front();
							D6COUT(1) << name() << "    --- save_req_id"   << hex <<  m_m6_in_fifo_id->front() << endl;
							if (m_m6_in_fifo_id->front() != m_id) // go to send_cost_location if I am not the requierer
							{
								r_loc_module_fsm = l_send_cost_value; // send the compute cost to the requierer
								p_set = 0;
							}
							else  // go to wait_loc_id : wait for requested results (compute_location by other modules)
							{
								D6COUT(1) << name() << "    my_id " << std::hex << m_id << " go to wait&compare with : " << dec << r_loc_location_cost <<  endl;
								r_loc_module_fsm = l_wait_loc_cost; 
								r_loc_location_found_is_other = false;	// initializing its value
								if (r_ctrl_mig_c) r_loc_location_cost = M6_MAX_LOC_COST; // if C mig, we want to find the best "outher" place
								assert(r_loc_node_count_cpt == 0);
							}
						}
						else // stay here until N nodes request have not been received and computed
						{
							r_loc_node_count_cpt++;
						}

						if (m_m6_in_fifo_id->front() != m_id) // In all cases (except I am the requierer), forward this message to others nodes
						{ 
							out_fifo->put  = true;
							out_fifo->data = m_m6_in_fifo_data->front();
							out_fifo->cmd  = m_m6_in_fifo_cmd->front();
							out_fifo->id   = m_m6_in_fifo_id->front();
							out_fifo->srcid = m_m6_in_fifo_srcid->front();
						}
					}
					break;
				}
			case l_send_cost_value :  // OUT : Ring_Cost, TO idle
				{
					D6COUT(0) << name() << " [l_send_cost_value]" << endl;
					// We lock the ring
					if (m_m6_out_fifo_wok  && (!in_out_fifo_forward) && (!out_fifo->put))
					{
						out_fifo->data = r_loc_location_cost;
						out_fifo->cmd  = RING_COST;
						out_fifo->id   = r_loc_save_req_id;
						out_fifo->srcid   = m_id;
						out_fifo->put = true;
						D6COUT(1) << name() << "                         value ->" << dec <<  r_loc_location_cost << endl;
						D6COUT(1) << name() << "                         srcid ->" << dec <<  m_id << endl;
						r_loc_module_fsm = l_idle;
					}

					break;
				}
			case l_wait_loc_cost : // IN : Ring_Cost, SYNC_OUT : location_found, 
				{
					// todo, deprecated state
					D6COUT(0) << name() << " [l_wait_loc_cost]" << endl;
					if(r_loc_node_count_cpt == m_nb_memory_nodes - 1)  // number of memory nodes minus one (us).
					{ 
						r_loc_module_fsm = l_idle;
						r_loc_location_found = true;	
						D6COUT(2) << name() << "    ---> location found" << endl;
						r_loc_node_count_cpt = 0;

						p_set = 0;
						p_B = false;
						assert(!( m_m6_in_fifo_rok && valid_request));
						break;
					}

					if ( m_m6_in_fifo_rok && valid_request)
					{
						D6COUT(0) << name() << "                 -> req and cpt : "<< r_loc_node_count_cpt << endl;
						assert(valid_request);
						assert(!in_fifo->get);
						in_fifo->get = true;
						assert(m_m6_in_fifo_id->front() == m_id);
						assert(m_m6_in_fifo_cmd->front() == RING_COST);

						D6COUT(1) << name() << "                  -> cost " << dec << m_m6_in_fifo_data->front()<< endl;
						D6COUT(1) << name() << " id : " << std::hex << m_m6_in_fifo_srcid->front()<<  "     cost : " <<  std::dec << m_m6_in_fifo_data->front() << endl;
						D6COUT(1) << name() << " my_id : " << std::hex << m_id 
						                    <<  "     my_cost : " <<  std::dec << r_loc_location_cost 
								    << endl;

						D6COUT(1) << dec << m_id<< ":"<< r_loc_location_cost
						<< " i=" <<  hex << m_m6_in_fifo_srcid->front()
						<< " c=" <<  dec << m_m6_in_fifo_data->front()<< endl;
						if (m_m6_in_fifo_data->front() >= M6_MAX_LOC_COST)
						{
							p_B = true;
						}
						D6COUT(1) << hex << " " <<r_loc_location_cost << ":"   <<  m_m6_in_fifo_data->front() ;
						if ((m_m6_in_fifo_data->front() < r_loc_location_cost))
						{	
							r_loc_location_cost = m_m6_in_fifo_data->front();
							r_loc_location_found_id = m_m6_in_fifo_srcid->front();
							D6COUT(2) << name() << "    --- location cost 0x"   << dec <<  m_m6_in_fifo_data->front() << endl;
							D6COUT(2) << name() << "    --- local_cost " << dec << r_loc_location_cost << " ---" << endl;
							D6COUT(2) << name() << "    --- location found_id " << hex << r_loc_location_found_id << " ---" << endl;
							r_loc_location_found_is_other = true;	
						}

						r_loc_node_count_cpt++;
					}


					break;
				}
		}
	}

	tmpl(void)::compute(void)
	{

		// Fifo control variable

		fifo_control_t out_fifo;
		fifo_control_t in_fifo;
		fifo_control_t in_fifo_test;
		fifo_control_t out_fifo_test;

		out_fifo.get  = false;
		out_fifo.put  = false;
		out_fifo.cmd  = RING_NOP;
		out_fifo.data = 0;
		out_fifo.id   = 0;
		out_fifo.srcid  = 0xbeef;

		in_fifo.put  = false;
		in_fifo.get  = false;
		in_fifo.cmd  = RING_NOP;
		in_fifo.data = 0;
		in_fifo.id   = 0;
		in_fifo.srcid   = 0;

		bool in_out_fifo_forward  = false;

		bool inter_in_fifo_get = false;
		bool inter_out_fifo_put = false;
		inter_fifo_data_t inter_fifo_data = 0;

		bool fifo_pending_lock_enqueue = false;
		bool fifo_pending_lock_put = false;
		bool fifo_pending_lock_get = false;
		micro_ring_id_t fifo_pending_lock_id =  0;

		bool fifo_pending_updt_put = false;
		bool fifo_pending_updt_get = false;
		micro_ring_id_t fifo_pending_updt_srcid =  0;
		micro_ring_cmd_t fifo_pending_updt_cmd =  RING_NOP;
		micro_ring_data_t fifo_pending_updt_data =  0;

		m_cycle++;
		//if (m_counting_time) m_cycle_mig++;
		if (r_ctrl_in_transaction) m_cycle_mig++;

		if (m_cycle_mig > 100000) 
		{
			std::cout << "Error, migration de plus de 100000 cycles! @ " << dec <<  m_cycle <<  std::endl; 
			assert(false);
		}

		bool m_m6_in_fifo_is_ctrl     = false;
		bool m_m6_in_fifo_is_loc      = false;
		bool m_m6_in_fifo_is_update   = false;
		bool m_m6_in_fifo_is_transfer = false;
		micro_ring_cmd_t	fifo_cmd;

		////////////////////////
		//   M6_MODULE_FSM
		///////////////////////
		if (m_id == 8) D6COUT(0) << "-------------------transition------------------------" << endl;
		D6COUT(0) <<  name() << "out fifo size = "<< dec  << m_m6_out_fifo_data->size() << endl;
		D6COUT(0) <<  name() << "in fifo size = "<< dec  << m_m6_in_fifo_data->size() << endl;
		if (m_m6_in_fifo_rok) // Forward, enqueue, and dispatch ring requests
		{
			fifo_cmd = m_m6_in_fifo_cmd->front();
			switch(fifo_cmd)
			{
				case RING_NOP : // Nothing todo
					{
						break;
					}
				case RING_LOCK : // If we are in transaction enqueue, if not for us forward, else set is_ctrl
					{
					D6COUT(0) <<  name() << "> RING_LOCK on the interface " << endl;
						if (m_m6_in_fifo_id->front() == m_id)
						{
							D6COUT(0) << "> is ctrl" << endl;
							m_m6_in_fifo_is_ctrl = true;
						}
						else
						{
							if (m_m6_in_fifo_data->front() == 1) // This request already contains the token, just forward it! (we cannot have the lock)
							{
								assert(!r_ctrl_ring_locked);
								in_out_fifo_forward = true;
							}
							else
							{
								// Even if we do not have already the lock, we enqueue this.
								// If at next cycle we are not in transaction, we will dequeue it
								D6COUT(1) <<  name() << "> enqueue" << endl;
								fifo_pending_lock_enqueue = true;
							}
						}
						break;
					}
				case RING_LOCATE :
					D6COUT(0) <<  name() << "> RING_LOCATE on the interface " << endl;
					{
						if(!r_ctrl_master) assert(!r_ctrl_in_transaction);
						m_m6_in_fifo_is_loc = true;
						break;
					}
				case RING_COST :
					{
					D6COUT(0) << name() <<  "> RING_COST on the interface " << endl;
					D6COUT(0) <<  name() << "> id / srcid " << m_m6_in_fifo_id->front() << " " << m_m6_in_fifo_srcid->front() << endl;
						if (m_m6_in_fifo_id->front() == m_id)
						{
							m_m6_in_fifo_is_loc = true;
						}
						else
						{
							in_out_fifo_forward = true;
						}
						break;
					}
				case RING_SELECT :
					{
					D6COUT(0) <<  name() << "> RING_SELECT on the interface " << endl;
						if (r_ctrl_master || (m_m6_in_fifo_id->front() == m_id))
						{
							m_m6_in_fifo_is_ctrl = true;
						}
						else
						{
							in_out_fifo_forward = true;
						}
						break;
					}
				case RING_FINISH :
					{
					D6COUT(0) <<  name() << "> RING_FINISH on the interface " << endl;
						if (r_ctrl_master)
						{
							m_m6_in_fifo_is_ctrl = true;
						}
						else
						{
							in_out_fifo_forward = true;
						}
						break;
					}
				case RING_UPDATE_ACK :
				case RING_UPDATE :
					{
						D6COUT(0) <<  name() << "> RING_UPDT/ACK on the interface" << endl;
						if (m_m6_in_fifo_id->front() == m_id)
						{
							fifo_pending_updt_put = true; // Enqueue in a fifo this request in order avoid deadlocks, udate_fsm will take its commands
														  // from this fifo when it will be able to do so.
							in_fifo.get = true;
							fifo_pending_updt_cmd = m_m6_in_fifo_cmd->front();
							fifo_pending_updt_data  = m_m6_in_fifo_data->front();
							fifo_pending_updt_srcid  = m_m6_in_fifo_srcid->front();
						}
						else
						{
							in_out_fifo_forward = true;
						}
						break;
					}
				case RING_TRANSFER :
					{
						D6COUT(0) <<  name() << "> RING_TRANSFER on the interface " << endl;
						if (m_m6_in_fifo_id->front() == m_id)
						{
							m_m6_in_fifo_is_transfer = true;
						}
						else
						{
							in_out_fifo_forward = true;
						}
						break;
					}
				default :
					assert(false);
					break;
			}
		}
		m_m6_in_fifo_is_update = m_m6_fifo_pending_updt_rok;  // The update_fsm is awaken when there is a request in this special fifo
															  // The update_fsm does not set the in_fifo.get, but, the pending_updt_get 
															  // signal instead!

		// COMPUTE THE FSM'S
			in_fifo_test.get = in_fifo.get; // default  values
			in_fifo_test.put = in_fifo.put;
			in_fifo_test.cmd = in_fifo.cmd;
			in_fifo_test.data = in_fifo.data;
			in_fifo_test.id = in_fifo.id;
			in_fifo_test.srcid = in_fifo.srcid;
			out_fifo_test.get = out_fifo.get; // default  values
			out_fifo_test.put = out_fifo.put;
			out_fifo_test.cmd = out_fifo.cmd;
			out_fifo_test.data = out_fifo.data;
			out_fifo_test.id = out_fifo.id;
			out_fifo_test.srcid = out_fifo.srcid;
			{
				int a = 0;
				if (m_m6_in_fifo_is_loc) a++;
				if (m_m6_in_fifo_is_ctrl) a++;
				//if (m_m6_in_fifo_is_update) a++;
				if (m_m6_in_fifo_is_transfer) a++;
				if (in_out_fifo_forward) a++;
				assert(a<=1);
			}

		compute_location(&in_fifo, &out_fifo, &inter_in_fifo_get, m_m6_in_fifo_is_loc, in_out_fifo_forward);
		if (in_fifo.get && !in_fifo_test.get ){
			in_fifo_test.get = in_fifo.get;
			in_fifo_test.put = in_fifo.put;
			in_fifo_test.cmd = in_fifo.cmd;
			in_fifo_test.data = in_fifo.data;
			in_fifo_test.id = in_fifo.id;
			in_fifo_test.srcid = in_fifo.srcid;
		}else if (in_fifo.get && in_fifo_test.get){
			assert(in_fifo_test.get  == in_fifo.get);
			assert(in_fifo_test.put == in_fifo.put);
			assert(in_fifo_test.cmd == in_fifo.cmd);
			assert(in_fifo_test.data == in_fifo.data);
			assert(in_fifo_test.id == in_fifo.id);
			assert(in_fifo_test.srcid == in_fifo.srcid);
		}
		if (out_fifo.put){
			out_fifo_test.get = out_fifo.get;
			out_fifo_test.put = out_fifo.put;
			out_fifo_test.cmd = out_fifo.cmd;
			out_fifo_test.data = out_fifo.data;
			out_fifo_test.id = out_fifo.id;
			out_fifo_test.srcid = out_fifo.srcid;
		}
		compute_control(&in_fifo, &out_fifo, m_m6_in_fifo_is_ctrl, in_out_fifo_forward);
		if (in_fifo.get && in_fifo_test.get){
			assert(in_fifo_test.get  == in_fifo.get);
			assert(in_fifo_test.put == in_fifo.put);
			assert(in_fifo_test.cmd == in_fifo.cmd);
			assert(in_fifo_test.data == in_fifo.data);
			assert(in_fifo_test.id == in_fifo.id);
			assert(in_fifo_test.srcid == in_fifo.srcid);
		}
		if (out_fifo.put && out_fifo_test.put){
			assert(out_fifo_test.get  == out_fifo.get);
			assert(out_fifo_test.put == out_fifo.put);
			assert(out_fifo_test.cmd == out_fifo.cmd);
			assert(out_fifo_test.data == out_fifo.data);
			assert(out_fifo_test.id == out_fifo.id);
			assert(out_fifo_test.srcid == out_fifo.srcid);
		}
		compute_update(&fifo_pending_updt_get, &out_fifo, m_m6_in_fifo_is_update, in_out_fifo_forward);
		if (out_fifo.put && out_fifo_test.put){
			assert(out_fifo_test.get  == out_fifo.get);
			assert(out_fifo_test.put == out_fifo.put);
			assert(out_fifo_test.cmd == out_fifo.cmd);
			assert(out_fifo_test.data == out_fifo.data);
			assert(out_fifo_test.id == out_fifo.id);
			assert(out_fifo_test.srcid == out_fifo.srcid);
		}
		compute_transfer(&in_fifo, &out_fifo, m_m6_in_fifo_is_transfer, in_out_fifo_forward);
		if (in_fifo.get && in_fifo_test.get){
			assert(in_fifo_test.get  == in_fifo.get);
			assert(in_fifo_test.put == in_fifo.put);
			assert(in_fifo_test.cmd == in_fifo.cmd);
			assert(in_fifo_test.data == in_fifo.data);
			assert(in_fifo_test.id == in_fifo.id);
			assert(in_fifo_test.srcid == in_fifo.srcid);
		}
		if (out_fifo.put && out_fifo_test.put){
			assert(out_fifo_test.get  == out_fifo.get);
			assert(out_fifo_test.put == out_fifo.put);
			assert(out_fifo_test.cmd == out_fifo.cmd);
			assert(out_fifo_test.data == out_fifo.data);
			assert(out_fifo_test.id == out_fifo.id);
			assert(out_fifo_test.srcid == out_fifo.srcid);
		}

		switch((out_fsm_state_e)r_m6_out_fsm) // The Output FSM
		{
			case out_idle : 
				{
					if (m_m6_out_fifo_rok) // There is some request
					{
						if ((p_out.ring)->ack)    // request accepted by following node
						{ 
							out_fifo.get = true; // we can pop the data
						}
					}
					break;
				}
			default :
				{
					assert(false);
					break;
				}
		}

		switch((in_fsm_state_e)r_m6_in_fsm) // The Input FSM
		{
			case in_idle : 
				{
					if (m_m6_in_fifo_wok && ((p_in.ring)->cmd != CMD_NOP))
					{
						in_fifo.put = true;
						in_fifo.cmd = (p_in.ring)->cmd;
						in_fifo.data = (p_in.ring)->data;
						in_fifo.id = (p_in.ring)->id;
						in_fifo.srcid = (p_in.ring)->srcid;
					}
					break;
				}
			default : // Error
				{
					assert(false);

					break;
				}
		}
		// Nothing to do, not in transaction, release enqueued RING_LOCKS if possible
		// IMPORTANT: also check for p_in->req to avoid in the same cycle, releasing the lock and setting r_ctrl_in_transaction!
		// IMPORTANT: also check for m_m6_in_fifo_is_ctrl to avoid in the same cycle, releasing the lock and setting r_ctrl_in_transaction!
		if (m_m6_out_fifo_wok && m_m6_fifo_pending_lock_rok && !r_ctrl_in_transaction  && !in_out_fifo_forward && (!out_fifo.put) && !(*(p_in.req)) && !m_m6_in_fifo_is_ctrl)
		{
			out_fifo.data = r_ctrl_ring_locked.read(); // give the token if we have it!
			out_fifo.cmd  = RING_LOCK;
			out_fifo.id   = m_m6_fifo_pending_lock->front();
			out_fifo.srcid   = m_id;
			D6COUT(1) << name() << "> release a LOCK cmd by :"<< dec << out_fifo.id  << " with " << out_fifo.data << endl;
			assert(m_m6_fifo_pending_lock->front() != m_id);

			r_ctrl_ring_locked = false;
			out_fifo.put = true;
			fifo_pending_lock_get  = true;
		}
		// FIFO FSMs
		if (fifo_pending_lock_enqueue)
		{
			assert(m_m6_fifo_pending_lock_wok);
			assert(m_m6_in_fifo_rok);
			assert(!in_fifo.get);
			D6COUT(1) << name() << "> enqueue lock" << endl;
			in_fifo.get   = true;
			fifo_pending_lock_put  = true;
			fifo_pending_lock_id = m_m6_in_fifo_id->front() ;
		};
		if (in_out_fifo_forward)
		{
			//assert(!out_fifo.put);
			assert(!in_fifo.get);
			if(m_m6_out_fifo_wok && !(out_fifo.put))
			{
				D6COUT(0) << name() << "> forward" << endl;
				in_fifo.get   = true; 
				if (in_fifo.get) D6COUT(0) <<  name() << "> -- ----------------------------------------------------get --  by forward " << endl;
				out_fifo.put  = true;
				out_fifo.data = m_m6_in_fifo_data->front();
				out_fifo.cmd  = m_m6_in_fifo_cmd->front();
				out_fifo.id   = m_m6_in_fifo_id->front();
				out_fifo.srcid   = m_m6_in_fifo_srcid->front();
			}
			else
			{
				D6COUT(0) << name() << "> X cannot forward" << endl;
			}
		}
		if (in_fifo.put)
		{
			m_m6_in_fifo_cmd->push_back(in_fifo.cmd);
			m_m6_in_fifo_data->push_back(in_fifo.data);
			m_m6_in_fifo_id->push_back(in_fifo.id);
			m_m6_in_fifo_srcid->push_back(in_fifo.srcid);
		}
		if (in_fifo.get)
		{
			m_m6_in_fifo_cmd->pop_front();
			m_m6_in_fifo_data->pop_front();
			m_m6_in_fifo_id->pop_front();
			m_m6_in_fifo_srcid->pop_front();
		}
		if (out_fifo.put)
		{
			m_m6_out_fifo_cmd->push_back(out_fifo.cmd);
			m_m6_out_fifo_data->push_back(out_fifo.data);
			m_m6_out_fifo_id->push_back(out_fifo.id);
			m_m6_out_fifo_srcid->push_back(out_fifo.srcid);
		}
		if (out_fifo.get)
		{
			m_m6_out_fifo_cmd->pop_front();
			m_m6_out_fifo_data->pop_front();
			m_m6_out_fifo_id->pop_front();
			m_m6_out_fifo_srcid->pop_front();
		}
		if (inter_in_fifo_get)
		{
			(p_in.fifo)->pop_front();
		}
		if (inter_out_fifo_put)
		{
			(p_out.fifo)->push_back(inter_fifo_data);
		}
		if (fifo_pending_lock_put)
		{
			m_m6_fifo_pending_lock->push_back(fifo_pending_lock_id);
		}
		if (fifo_pending_lock_get)
		{
			m_m6_fifo_pending_lock->pop_front();
		}
		if (fifo_pending_updt_put)
		{
			m_m6_fifo_pending_updt_cmd->push_back(fifo_pending_updt_cmd);
			m_m6_fifo_pending_updt_data->push_back(fifo_pending_updt_data);
			m_m6_fifo_pending_updt_srcid->push_back(fifo_pending_updt_srcid);
		}
		if (fifo_pending_updt_get)
		{
			m_m6_fifo_pending_updt_cmd->pop_front();
			m_m6_fifo_pending_updt_data->pop_front();
			m_m6_fifo_pending_updt_srcid->pop_front();
		}

	}

	tmpl(void)::set_tlb_inv_ok(void)
	{
		D6COUT(2) << name() << "    ---> r_updt_inv_tlb_ok " << endl;
		r_updt_inv_tlb_ok = true;
	}

	tmpl(void)::set_wiam_ok(void)
	{
		r_updt_wiam_ok = true;
		D6COUT(2) << name() << "    ---> r_updt_wiam_ok " << endl;
	}

	tmpl(void)::gen_outputs(void)

	{
		////////////////////////
		//   M6_MODULE_FSM
		///////////////////////

		// Default values : 
		*(p_out.req) = false;
		*(p_out.cmd) = CMD_NOP;
		*(p_out.data) = 0;
		*(p_out.ack) = false;

		// RING OUT
		// todo , values are wrong

		(p_out.ring)->id      = 0;
		(p_out.ring)->srcid   = 0xdead;
		(p_out.ring)->cmd     = 0;
		(p_out.ring)->data    = 0;

		// RING IN

		(p_in.ring)->ack      = true;

		switch((control_fsm_state_e)r_ctrl_module_fsm)
		{
			case c_idle :
			case c_unpoison_ok :
				*(p_out.ack) = true;
				break;
			case c_elect :   
				*(p_out.req) = true;
				*(p_out.cmd) = CMD_ELECT;
				*(p_out.data) = 0; // oki 
				break;

			case c_poison : 
				*(p_out.req) = true;
				*(p_out.cmd) = CMD_POISON;
				*(p_out.data) = 0; // oki 
				break;

			case c_notify_m_control : 
				*(p_out.req) = true;
				*(p_out.cmd) = CMD_BEGIN_TRANSACTION;
				*(p_out.data) = 0; // oki 
				break;
			case c_unpoison :
				*(p_out.req) = true;
				*(p_out.cmd) = CMD_UNPOISON;
				*(p_out.data) = 0; // oki 
				break;

			case c_abort :
				*(p_out.req) = true;
				*(p_out.cmd) = CMD_ABORT;
				*(p_out.data) = 0; // oki 
				break;
			case c_update_tlb :
				*(p_out.req) = true;
				*(p_out.cmd) = CMD_UPDT_INV;
				*(p_out.data) = (r_ctrl_cycle_two ? r_updt_page_new_addr : r_updt_page_local_addr ); // oki 
				break;
			case c_update_wiam :
				*(p_out.req) = true;
				*(p_out.cmd) = CMD_UPDT_WIAM;
				*(p_out.data) = (r_ctrl_cycle_two ? r_updt_wiam_addr : r_updt_page_local_addr ); // oki 
				break;

			case c_lock_ring : 
			case c_poison_wait :
			case c_finish :
			case c_wait_page_addr_home :
			case c_notify_selected : 
				break;
		}

		// Specific values :

		switch((out_fsm_state_e)r_m6_out_fsm)
		{
			case out_idle : 
				if (m_m6_out_fifo_rok)
				{
					(p_out.ring)->id      = m_m6_out_fifo_id->front();
					(p_out.ring)->cmd     = m_m6_out_fifo_cmd->front();
					(p_out.ring)->data    = m_m6_out_fifo_data->front();
					(p_out.ring)->srcid   = m_m6_out_fifo_srcid->front();
					assert((p_out.ring)->srcid   != 0xdead);
					if ((p_out.ring)->srcid  == 0xbeef) cout << "beef by " << m_m6_out_fifo_cmd->front()  << endl;
					assert((p_out.ring)->srcid   != 0xbeef);
				}
				break;
			default :
				assert(false);
				break;
		}

		switch((in_fsm_state_e)r_m6_in_fsm)
		{
			case in_idle : 
				(p_in.ring)->ack = m_m6_in_fifo_wok;
				break;
			default :
				assert(false);
				break;
		}

	}

	tmpl(const string &)::name()
	{
		return this->m_name;
	}

	tmpl(/**/)::~Module_6()
	{
		DTOR_OUT
		delete m_m6_out_fifo_data;
		delete m_m6_out_fifo_cmd;
		delete m_m6_out_fifo_id;
		delete m_m6_out_fifo_srcid;

		delete m_m6_in_fifo_data;
		delete m_m6_in_fifo_cmd;
		delete m_m6_in_fifo_id;
		delete m_m6_in_fifo_srcid;

		delete m_m6_fifo_pending_lock;
		delete [] r_tr_memory_line_buffer_read;
		delete [] r_tr_memory_line_buffer_write;
	}

}
}
