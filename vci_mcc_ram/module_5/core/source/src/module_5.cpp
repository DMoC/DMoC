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
#include "module_5.h"
#include "arithmetics.h"
#include "globals.h"
#include "mapping.h"

namespace soclib {
namespace caba {

using namespace soclib;
using namespace std;
using soclib::common::uint32_log2;
#define tmpl(x)  x Module_5
tmpl(/**/)::Module_5(const string &insname,std::list<soclib::common::Segment> * seglist,unsigned int nb_procs, unsigned int page_size):
	m_name(insname),
	m_nb_segments(seglist->size()),
	m_seglist(seglist)
	{
		std::list<soclib::common::Segment>::iterator     iter = m_seglist->begin();
		CTOR_OUT

		/////////////////////////////////////////
		// instanciation des modules internes
		/////////////////////////////////////////
		
		p_in = new m5_inputs_t;
		p_out = new m5_outputs_t;
		m_enable_counter = new bool[m_nb_segments];
		m_enable_access_register = new bool[m_nb_segments];
		compute_contention = new Module_8(gen_name(name().c_str(),"_cc_",0));
		counters = new Module_9 * [m_nb_segments];
		register_access = new Module_7 * [m_nb_segments];
		m_unresolved_output_m7_m10_link_req = new Module_7::m7_req_t[m_nb_segments];
		m_unresolved_output_m7_m10_link_cmd = new Module_7::m7_cmd_t[m_nb_segments];
		m_unresolved_output_m9_m10_busy = new bool[m_nb_segments];
		m_unresolved_output_m9_m10_threshold = new bool[m_nb_segments];
		for (unsigned int i = 0; i < m_nb_segments ; i++, iter++){
			counters[i] = new Module_9(gen_name(name().c_str(),"_counters_",0),nb_procs,(iter->size()/page_size),iter->baseAddress(),page_size);
			register_access[i] = new Module_7 (gen_name(name().c_str(),"_reg_access_",i), counters[i]);
			D5COUT << "counters for :" << iter->name() << endl;
		}
#if ONE_SEG_MODE
		control = new Module_10(gen_name(name().c_str(),"_ctrl_",0), counters[0],compute_contention,nb_procs);
#else
		assert(false); // multisegment not supported, control must refer to one set of counters/segment
#endif
		assert(iter == m_seglist->end());

		/////////////////////////////////////////
		// Connection of components
		/////////////////////////////////////////
		compute_contention->p_in.is_req = &(p_in->enable); // contention is before control, take care of this! 	
		compute_contention->p_in.is_abort = &m_10_to_8_abort; // contention is before control, take care of this! 	
		compute_contention->p_out.contention = &m_8_to_10_contention;
		control->p_in.contention = &m_8_to_10_contention;

		control->p_in.m6_cmd = &(p_in->m6_cmd);
		control->p_in.m6_data = &(p_in->m6_data);
		control->p_in.m6_ack = &(p_in->m6_ack);
		control->p_in.m6_req = &(p_in->m6_req);
		cout << "addr control p_in.req : " << hex << (control->p_in.m6_req) << endl;
		cout << "addr control p_in.cmd : " << hex << (control->p_in.m6_cmd) << endl;
		control->p_in.m6_fifo  = &(p_in->m6_fifo);
		control->p_in.poison_ack  = &(p_in->poison_ack);
		control->p_in.unpoison_ok  = &(p_in->unpoison_ok);
		control->p_in.virt_poison_page  = &(p_in->virt_poison_page);
		control->p_in.updt_tlb_ack  = &(p_in->updt_tlb_ack);
		control->p_in.updt_wiam_ack  = &(p_in->updt_wiam_ack);

		control->p_in.m7_cmd = &(m7_m10_link.m7_cmd);
		control->p_in.m7_req = &(m7_m10_link.m7_req);
		control->p_in.m7_rsp = &(m7_m10_link.m7_rsp);
		

		for (unsigned int i = 0; i < m_nb_segments ; i++){
			register_access[i]->p_in.cmd   = &(p_in->m7_cmd);
			register_access[i]->p_in.req   = &(m_enable_access_register[i]);
			register_access[i]->p_in.rsp   = &(p_in->m7_rsp);

			// todo : some assertion should be done to resolve the signal
			#if 0
			register_access[i]->p_out.cmd = &(m7_m10_link.m7_cmd);
			register_access[i]->p_out.req = &(m7_m10_link.m7_req);
#else
			// rsp connected directly, everyone will forward the same value (OR resolv)
			register_access[i]->p_out.rsp = &(m7_m10_link.m7_rsp);
			// cmd and req are resolved with an OR between all the output.
			register_access[i]->p_out.cmd = &(m_unresolved_output_m7_m10_link_cmd[i]);
			register_access[i]->p_out.req = &(m_unresolved_output_m7_m10_link_req[i]);
#endif

			counters[i]->p_in.node_id   = &(p_in->node_id);
			counters[i]->p_in.page_sel  = &(p_in->page_sel);
			counters[i]->p_in.cost      = &(p_in->cost);
			counters[i]->p_in.enable    = &(m_enable_counter[i]);
			counters[i]->p_in.freeze    = &(m_10_to_9_freeze);
			counters[i]->p_out.is_busy  = &(m_unresolved_output_m9_m10_busy[i]);
			counters[i]->p_out.is_threshold  = &(m_unresolved_output_m9_m10_threshold[i]);
		}

		control->p_out.m6_cmd    = &(p_out->m6_cmd);
		control->p_out.m6_data   = &(p_out->m6_data);
		control->p_out.m6_ack    = &(p_out->m6_ack);
		control->p_out.m6_req    = &(p_out->m6_req);
		control->p_out.m6_fifo   = &(p_out->m6_fifo);
		control->p_out.m9_freeze = &(m_10_to_9_freeze);
		control->p_in.m9_is_busy    = &(m_9_to_10_is_busy);
		control->p_in.m9_is_threshold = &(m_9_to_10_is_threshold);
		control->p_out.m6_pressure    = &(p_out->m6_pressure);

		control->p_out.poison_page    = &(p_out->poison_page);
		control->p_out.poison_req    = &(p_out->poison_req);
		control->p_out.unpoison_req    = &(p_out->unpoison_req);
		
		control->p_out.updt_tlb_req    = &(p_out->updt_tlb_req);
		control->p_out.updt_wiam_req    = &(p_out->updt_wiam_req);
		control->p_out.home_entry    = &(p_out->home_entry);
		control->p_out.new_entry    = &(p_out->new_entry);

		control->p_out.m8_abort    = &(m_10_to_8_abort);

		// control->p_out.fifo_write    = &(p_out->m6_fifo_write);
		// control->p_out.fifo_data    = &(p_out->m6_fifo_data);
	}

#if 1
tmpl(const char*)::gen_name(const char * name_pref, const char* name, unsigned int id){
	std::ostringstream n;
	char *cstr;
	n << name_pref << name << id;
	cstr = new char[n.str().size() + 1];
	n.str().copy(cstr, std::string::npos);
	cstr[n.str().size()] = '\0';
	return cstr;
}
#endif

tmpl(const string &)::name(){
	return this->m_name;
}

tmpl(void)::gen_outputs(){
	// a kind of genMoore(), set output signals
		// compute_contention 
		compute_contention->gen_outputs();
		// control 
		control->gen_outputs();
		// counters
		for (unsigned int i = 0; i < m_nb_segments ; i++){
			m_9_to_10_is_busy = false;
			m_9_to_10_is_threshold = false;
			counters[i]->gen_outputs();
			if (m_unresolved_output_m9_m10_busy[i])
				m_9_to_10_is_busy = true;
				// the busy signal was set in the gen_outputs of counters[i]
			if (m_unresolved_output_m9_m10_threshold[i])
			{
				m_9_to_10_is_threshold = true;
			}

			// register_access
			register_access[i]->gen_outputs();
		}
}

tmpl(void)::compute(){
		// register_access
		m7_m10_link.m7_req = false;
		m7_m10_link.m7_cmd = Module_7::CMD7_NOP;
		for (unsigned int i = 0; i < m_nb_segments ; i++){
			m_enable_access_register[i] = (p_in->m7_req && (p_in->segnum == i));
			register_access[i]->compute(); // this is done BEFORE control, because there are some combinational logic used as an input by "control"
			if (m_unresolved_output_m7_m10_link_req[i]){ 
				D7COUT << name() << "req on" << dec <<  i << endl;
				D7COUT << name() << "cmd-> " << dec <<  m_unresolved_output_m7_m10_link_cmd[i] << endl;
				m7_m10_link.m7_cmd = m_unresolved_output_m7_m10_link_cmd[i]; 
				m7_m10_link.m7_req = m_unresolved_output_m7_m10_link_req[i]; 
			}
		}
		compute_contention->compute();
		// control 
		// busy output was set in the gen_outputs at the previous cycle,
		control->compute();
		// counters
		D5COUT << name() << " segnum->" << p_in->segnum << " enable->" << p_in->enable << endl;
		D5COUT << "      node_id->" << p_in->node_id << endl;
		D5COUT << "      page_sel->" << p_in->page_sel << endl;
		D5COUT << "      cost->" << p_in->cost << endl;
		for (unsigned int i = 0; i < m_nb_segments ; i++){
			m_enable_counter[i] = (p_in->enable && (p_in->segnum == i));
			counters[i]->compute();
		}
}

tmpl(void)::reset(){
	compute_contention->reset();
	control->reset();
	for (unsigned int i = 0; i < m_nb_segments ; i++){
		counters[i]->reset();
		register_access[i]->reset();
	}
}

#if 1
tmpl(unsigned int)::reset_reg(unsigned int addr,unsigned int segnum){
	return register_access[segnum]->reset_reg(addr);
}

tmpl(unsigned int)::read_reg(unsigned int addr,unsigned int segnum){
	return register_access[segnum]->read_reg(addr);
}
#endif

tmpl(/**/)::~Module_5(){
	DTOR_OUT
	delete control;
	delete compute_contention;
	for(unsigned int i = 0; i < m_nb_segments; i++){
		delete register_access[i];
		delete counters[i];
	}
	delete [] register_access;
	delete [] counters;
	
	delete [] m_unresolved_output_m7_m10_link_req;
	delete [] m_unresolved_output_m7_m10_link_cmd;
	delete [] m_unresolved_output_m9_m10_busy;
	delete [] m_unresolved_output_m9_m10_threshold;
	delete [] m_enable_access_register;
	delete [] m_enable_counter;
	delete p_in;
	delete p_out;
}

}}
