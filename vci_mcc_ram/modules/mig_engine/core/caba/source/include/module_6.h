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
 *         Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>
 *
 */
#ifndef MODULE_6_H
#define MODULE_6_H

#include "globals.h"
#include "types_sls.h"
#include "micro_ring_signals.h"
#include "vci_mcc_ram_interface.h"
#include <systemc>

#include <stdint.h>
#include <string>
#include <list>


#ifdef DEBUG_M6
	#define D6COUT(x) if ((m_cycle > START_TRACE_CYCLE) && (x>=(DEBUG_M6_LEVEL))) cout
#else
	#define D6COUT(x) if(0) cout
#endif


namespace soclib {
namespace caba {

using namespace std;

//template < typename vci_param > class VciMccRam;
//template < typename vci_param > class VciMccRam;

typedef enum { CMD_NOP, CMD_IS_POISONNED, CMD_BEGIN_TRANSACTION, CMD_PAGE_ADDR_HOME,
			CMD_IS_CONTENTION,CMD_PE_MIG, CMD_POISON, CMD_UNPOISON,
			CMD_UNPOISON_OK, CMD_ELECT, CMD_UPDT_INV, CMD_UPDT_WIAM, CMD_ABORT} m6_cmd_t;
typedef micro_ring_data_t m6_data_t;
typedef micro_ring_id_t   m6_id_t;
typedef bool  m6_ack_t;
typedef bool  m6_req_t;
typedef unsigned int inter_fifo_data_t;
typedef list<inter_fifo_data_t> m6_fifo_t;

class Module_6 {
//template < typename vci_param > class Module_6 {

	public :

		typedef struct{
			m6_cmd_t  * cmd;
			m6_data_t * data;
			m6_ack_t  * ack;
			m6_req_t  * req;
			m6_fifo_t * fifo; // Will contain the top pages address, and page data
			uint32_t  * pressure;
			no_sc_MicroRingSignals  * ring;
		} m6_inputs_t;

		typedef struct{
			m6_cmd_t  * cmd;
			m6_data_t * data;
			m6_ack_t  * ack;
			m6_req_t  * req;
			m6_fifo_t * fifo; // Will page data todo , type should be micro_ring_data_t
			no_sc_MicroRingSignals  * ring;
		} m6_outputs_t;
		
		m6_inputs_t p_in;
		m6_outputs_t p_out;

		Module_6(const string & insname, 
				bool node_zero,
			 	unsigned int base_address,
				unsigned int line_size,
				unsigned int nb_mem,
				unsigned int nb_procs,
				unsigned int id,
				unsigned int * cost_table,
				addr_to_homeid_entry_t * home_addr_table,
				VciMccRam_interface * ram);
		~Module_6(void);
		void compute(void);
		void gen_outputs(void);
		void reset(void);
		const string & name(void);
		void set_tlb_inv_ok(void);
		void set_wiam_ok(void);
	private :
		unsigned int address_to_id(unsigned int addr);
		// Each fsm has its own compute/genoutputs function to
		// clarify the source code
		
		typedef struct {
			bool put;
			bool get;
			micro_ring_id_t id;
			micro_ring_cmd_t cmd;
			micro_ring_data_t data;
			micro_ring_id_t srcid;
		} fifo_control_t;

		void compute_control(fifo_control_t * in_fifo, fifo_control_t * out_fifo, bool valid_request, bool in_out_fifo_forward);
		void compute_transfer(fifo_control_t * in_fifo, fifo_control_t * out_fifo, bool valid_request, bool in_out_fifo_forward);
		void compute_update(bool * fifo_pending_updt_get, fifo_control_t * out_fifo, bool valid_request, bool in_out_fifo_forward);
		void compute_location(fifo_control_t * in_fifo, fifo_control_t * out_fifo, bool * inter_in_fifo_get, bool valid_request, bool in_out_fifo_forward);


	private :
		// MODULE_6_FSM STATES
		enum control_fsm_state_e{
			c_idle,
			c_lock_ring,      // get ring
			c_notify_m_control,	 // Notify module 10 that lock was acquired and we are the master, so the transaction can proceed  

			c_elect,	  // only for choosen node, elect (ex: random) a page for migration
			c_poison,
			c_poison_wait,	  
			c_wait_page_addr_home,	// (2) page poisonned and home address retrieved 

			c_notify_selected,	//  notify selected node

			c_update_tlb,
			c_update_wiam,
			c_unpoison,		// at (3) and (4), unpoison the page	
			c_unpoison_ok,
			c_abort,
			c_finish
		};


		enum transfer_fsm_state_e{
			t_idle,
			t_transfer,
			t_transfer_dir	// (4) the transfer is finished
		};

		enum update_fsm_state_e{
			u_idle,		// at (2) the nodes which will migrate data can send to home nodes the update
					// values for their tlb.
		u_send_updt_home,
		u_receive_update_tlb,
		u_tlb_inv_ok,	// (3) all inv's ack have been received
		u_send_updt_ack_home 
	};

	enum location_fsm_state_e{
		l_idle,		// at (1) nodes will receive request for "location", and send back the computed cost
				// the requesting node will wait for responses. memorised on idle state
		l_req_loc,	  //  (1) request for location
		l_compute_location,
		l_compute_pressure,
		l_send_cost_value,
		l_wait_loc_cost
	};

	// REQ_FSM STATES
	enum out_fsm_state_e{
		out_idle,
		out_locate,
		out_select,
		out_update,
		out_transfer
	};

	// RSP_FSM STATES
	enum in_fsm_state_e{
		in_idle,
		in_locate,
		in_transfer,
		in_ack_update
	};

	// STRUCTURAL PARAMETER
	string m_name;
	bool m_node_zero;

	unsigned int m_base_addr;
	unsigned int m_page_shift;
	unsigned int m_memory_line_size; // in words
	
	unsigned int m_nb_memory_nodes;
	unsigned int m_nb_processor_nodes;
	unsigned int m_id;
	
	unsigned int * m_table_cost;
	addr_to_homeid_entry_t * m_home_addr_table;
	VciMccRam_interface * m_ram;

	// REGISTERS
	// fsm's

	unsigned int r_ctrl_module_fsm;
	unsigned int r_tr_module_fsm;
	unsigned int r_updt_module_fsm;
	unsigned int r_loc_module_fsm;

	unsigned int r_m6_out_fsm;
	unsigned int r_m6_in_fsm;

	// control registers
	sc_signal<bool>    r_ctrl_ring_locked;    // the lock was acquired, this is the token! .. At reset time, only one node has this signal set to 1!
	sc_signal<bool>    r_ctrl_in_transaction; // We are transfering a page (as a master or a slave)
	sc_signal<bool>    r_ctrl_master;         // I am the master during the transfer ie : who received the contention signal
	sc_signal<bool>    r_ctrl_mig_c;          // Discriminate C and E migrations
	bool    r_ctrl_cycle_two; // used for two cycle states

	unsigned int r_ctrl_page_selected;        // contains the evicted page from a foreign node, ie the page to switch with.
	unsigned int r_ctrl_page_wis; 
	m6_data_t r_ctrl_page_addr;               // contains the frame index of the page to transfert 
	m6_data_t r_ctrl_page_addr_home;          // contains the address of the exchanged page, ie : the location of the original page

	sc_signal<bool>	r_ctrl_get_location;
	sc_signal<bool>	r_ctrl_update_home;
	sc_signal<bool>	r_ctrl_begin_transfer;
	sc_signal<bool>	r_ctrl_rcv_finish;


	// transfer registers
	unsigned short r_tr_transfer_req_cpt;
	unsigned short r_tr_transfer_rsp_cpt;
	bool r_tr_req_end;
	sc_signal<bool> r_tr_transfer_ok;
	bool r_tr_rsp_end;
	unsigned int * r_tr_memory_line_buffer_read;
	unsigned int * r_tr_memory_line_buffer_write;
	SOCLIB_DIRECTORY r_tr_dir_buffer_read;
	SOCLIB_DIRECTORY r_tr_dir_buffer_write;

	// location registers
	unsigned int r_loc_node_count_cpt;
	unsigned int r_loc_location_cost;
	sc_signal<bool> r_loc_location_found;
	sc_signal<bool> r_loc_location_found_is_other;
	sc_signal<bool> r_loc_my_pressure_is_lower;
	m6_id_t r_loc_location_found_id;
	m6_id_t r_loc_save_req_id;
		sc_signal<bool> r_loc_abort_transfer;

		// update registers
		bool    r_updt_cycle_two; // used for two cycle states
		sc_signal<bool> r_updt_update_tlb; // used for two cycle states
		sc_signal <bool> r_updt_update_wiam; 
		sc_signal<bool> r_updt_inv_tlb_ok; // shared with the ram, it is set to 1 when the tlb were invalidated
		sc_signal<bool> r_updt_wiam_ok; 
		sc_signal<bool> r_updt_update_ok; 
		unsigned int r_updt_page_local_addr; 	
		unsigned int r_updt_page_new_addr; 	
		unsigned int r_updt_wiam_addr;
		sc_signal<bool> r_updt_local_update;
		unsigned int r_updt_save_src_id;


		// fifo's out
		list<micro_ring_data_t> * m_m6_out_fifo_data;
		list<micro_ring_cmd_t>  * m_m6_out_fifo_cmd;
		list<micro_ring_id_t>   * m_m6_out_fifo_id;
		list<micro_ring_id_t>   * m_m6_out_fifo_srcid;

		// fifo's in 
		list<micro_ring_data_t> * m_m6_in_fifo_data;
		list<micro_ring_cmd_t>  * m_m6_in_fifo_cmd;
		list<micro_ring_id_t>   * m_m6_in_fifo_id;
		list<micro_ring_id_t>   * m_m6_in_fifo_srcid;

		// pending lock fifo
		list< micro_ring_id_t >   * m_m6_fifo_pending_lock; // todo should be a bit-vector, but to simplify implementation
								    // (ie, more than 64 bits, we use a simple fifo)
		list< micro_ring_cmd_t >   * m_m6_fifo_pending_updt_cmd; // Used to avoid deadlocks on the ring, size is 4 
		list< micro_ring_data_t >   * m_m6_fifo_pending_updt_data; // Used to avoid deadlocks on the ring, size is 4 
		list< micro_ring_id_t >   * m_m6_fifo_pending_updt_srcid; 

		// fifo_in/fifo_out size
		unsigned int m_m6_fifo_pending_lock_size;
		// pending lock fifo size
		unsigned int m_m6_fifo_pending_updt_size;
		// pending updt fifo size
		unsigned int m_m6_ring_fifo_size; // out and in fifo to the ring
		unsigned int m_m6_inter_fifo_size; // out and in fifo to module_10

		unsigned int m_cycle;
		unsigned int m_cycle_mig;
		bool m_counting_time;
		unsigned int p_set;
		unsigned int p_B;



};

}}
#endif
