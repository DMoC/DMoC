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
 * License ashort with SoCLib; if not, write to the Free Software
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
#include <cstdlib>
#include "arithmetics.h"
#include "mcc_globals.h"
#include "counters.h"

namespace soclib {
namespace caba {

	using namespace soclib;
	using namespace std;
	using soclib::common::uint32_log2;

	// Manque reset
	// que fait freeze
	void Counters::transition( void )
	{
		const bool			p_in_enable		= p_enable.read();

		const unsigned int	p_in_node_id	= p_ctrl_node_id.read();
	
		const bool			p_in_req		= p_ctrl_req.read();
		const cter_cmd_t	p_in_cmd		= p_ctrl_cmd.read();
		const unsigned int	p_in_page_id	= p_ctrl_page_id.read();

#if 0
		const bool p_in_freeze				= p_freeze.read();
#endif

		if (!p_resetn.read())
		{
			for (unsigned int i = 0; i <  m_NB_PAGES ; i++)
			{
				for(unsigned int j = 0 ; j < m_NB_NODES ; j++){
					r_counters[i][j]=0;
				}
				r_global_counters[i] = 0;
				d_global_counters[i] = 0;
			}

			r_max_page = 0; // by default the first page is the maximum
			r_index_p = 0;
			r_raise_threshold = false;
			m_raise_threshold = false;
			r_CTER_FSM = CTER_IDLE;
			D_COUNTERS_COUT << name() << " reset done  " << endl;
			return;
		}	

		switch ((cter_fsm_state_e) r_CTER_FSM.read())
		{
			case CTER_SEND_RSP :
				r_CTER_FSM = CTER_IDLE;
			break;

			case CTER_IDLE :
				r_raise_threshold = false;
				if (p_in_req) // If a command request (read, write etc)
				{
					switch ( p_in_cmd )
					{
						case R_MAX_GLOBAL_COUNTER :
							r_out_value = (sc_uint <32> )r_out_global_counter.read();	
							r_CTER_FSM = CTER_SEND_RSP;
							break;
						case R_MAX_COUNTER :
							assert(p_ctrl_node_id.read() < m_NB_NODES);
							r_out_value = (sc_uint <32> )r_out_counter[p_ctrl_node_id.read()].read();	
							r_CTER_FSM = CTER_SEND_RSP;
							break;
						case R_MAX_ROUNDED_COUNTER :
							assert(p_ctrl_node_id.read() < m_NB_NODES);
							r_out_value = (sc_uint <32> )uint32_log2((unsigned int)r_out_counter[p_ctrl_node_id.read()].read());	
							r_CTER_FSM = CTER_SEND_RSP;
							break;
						case R_MAX_ID_PAGE :
							r_CTER_FSM = CTER_SEND_RSP;
							r_out_value = (sc_uint <32> )r_out_max_p;
							break;
						case W_RESET_PAGE_CTER :
						case NOP :
							assert(false);
						case ELECT :
							r_CTER_FSM = CTER_SEND_RSP;
							r_out_value = (sc_uint <32> )evict_sel();
							break;
						break;
						default :
							assert(false);
							break;
					}
				}
				else
				if (p_in_enable) // Else, compute any access to a page
				{
					if (!(p_in_node_id < m_NB_NODES))
					{
						CWOUT << "access by node : "<< dec <<  p_in_node_id << endl;
						break;
					}
					// Save the request
					r_save_page_sel	= p_page_sel.read();
					r_save_node_id	= p_node_id.read();
					r_save_cost		= p_cost.read();

					r_CTER_FSM = CTER_COMPUTING;
					m_COMPUTE_TIME = CPT_TIME;
					break;
				}
				break;

			case CTER_COMPUTING :
				{

					const unsigned int	p = r_save_page_sel;
					const unsigned int	n = r_save_node_id;
					const counter_t		c = r_save_cost; 

					const unsigned int max_p = r_max_page;

					assert(p < m_NB_PAGES);
					assert(max_p < m_NB_PAGES);

					// Computation takes 
					if (m_COMPUTE_TIME != 0)
					{
						m_COMPUTE_TIME--;
						break;
					}
					// Only compute page access for processor modules
					// This may introduce error in top3 since contention may appear (module_8) due to
					// heavy DMA acces to pages not acccessed by a processor
					//D_COUNTERS_COUT <<  name() << " p_in_page_sel->" << r_save_page_sel
					//				<< " p_in_node_id->" << r_save_node_id
					//				<< " p_in_cost->" << r_save_cost <<  endl;
					{
						// some check
						counter_t sum = 0;
						for (unsigned int j = 0; j < m_NB_NODES; j++)
						{
							sum+=r_counters[p][j];
						}
						assert(r_global_counters[p] == sum);
					}
					// Computing the new id with is the most active user of this page
					// this kind of access is realistic, first load a complete line , and then select the correct word's (like in a cache)

					// Increment page acces cost for [page_number][node_id] and global counter[page_number]
					if ((counter_t)((counter_t)r_global_counters[p] + c ) < (counter_t)r_global_counters[p]){
						// overflow of this counter, reset the line
						D_COUNTERS_COUT << name() << "OVERFLOW" << endl;
						for (unsigned int i = 0; i < m_NB_NODES; i++){
							r_counters[p][i] = 0;
							r_global_counters[p] = 0;
							d_global_counters[p] = 0;
						}	
					}else{
						r_counters[p][n] = r_counters[p][n] + c;
						if ((r_global_counters[p] + c) > r_global_counters[max_p])
						{
							r_max_page = p;
						}
						r_global_counters[p] = r_global_counters[p] + c;
						d_global_counters[p] = d_global_counters[p] + c;

#ifdef PE_ON_GLOBAL
						if (r_global_counters[p] M9_COMP_OP PE_PERIOD) m_raise_threshold = true;
#else
						if (r_counters[p][n] M9_COMP_OP PE_PERIOD) m_raise_threshold = true;
#endif
						if (m_raise_threshold == true) {
							D_COUNTERS_COUT << name() << " PE mig at  "<< std::dec << r_counters[p][n] << endl;
							r_out_max_p = p;
							r_out_global_counter = r_global_counters[p] + c;
							m_COMPUTE_TIME = m_NB_NODES;
							r_CTER_FSM = CTER_SAVE;
							r_raise_threshold = m_raise_threshold;
							m_raise_threshold = false;
						}

						//D_COUNTERS_COUT << name() << "page->" << p << " value->" << r_global_counters[p] << endl;
					}
					m_COMPUTE_TIME = CPT_TIME;
					r_CTER_FSM = CTER_IDLE;
				}
				break;

			case CTER_SAVE :
				{
					const unsigned int	p = r_save_page_sel;
					// Computation takes 
					if (m_COMPUTE_TIME != 0)
					{
						m_COMPUTE_TIME--;
						break;
					}
					for (unsigned int j = 0; j < m_NB_NODES; j++)
					{
						r_out_counter[j] = r_counters[p][j];
					}
					m_COMPUTE_TIME = CPT_TIME;
					r_CTER_FSM = CTER_IDLE;
				}
			break;

			default :
				assert(false);
				break;

		}
	}
}}