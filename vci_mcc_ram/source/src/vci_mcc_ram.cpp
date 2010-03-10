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

#include "../include/vci_mcc_ram.h"
#include "soclib_endian.h"
#include "loader.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cstdio>


namespace soclib {
namespace caba {

	using soclib::common::uint32_log2;
	using namespace soclib;
	using namespace std;
#define tmpl(x) template<typename vci_param> x VciMccRam<vci_param>


	tmpl(/**/)::VciMccRam (
			sc_module_name insname,
			bool node_zero,
			const soclib::common::IntTab &i_ident,
			const soclib::common::IntTab &t_ident,
			soclib::common::CcIdTable * cct,
			const unsigned int nb_p,
			const soclib::common::Loader &loader,
			const unsigned int line_size,
			unsigned int * table_cost,
			addr_to_homeid_entry_t * home_addr_table,
			unsigned int nb_m,
			const soclib::common::MappingTable * mt,
			const soclib::common::MappingTable * mt_inv
) :
		caba::BaseModule(insname),
		m_loader(loader),
		m_MapTab(*mt)
	{

		m_segment_list = new std::list<soclib::common::Segment>(mt -> getSegmentList(t_ident));
		char generated_name[32];
		sprintf(generated_name,"%s.core",(const char *)insname);

		// Instanciate sub_modules
#ifdef DEBUG_SRAM
		if (mt_inv == NULL) // Only one NoC for requests and invalidation, pass &mt instead of &mt_inv as "Mapping table for invalidations"
		{
			c_core = new soclib::caba::MccRamCore<vci_param,sram_param>(generated_name,node_zero,i_ident,t_ident,mt,mt,cct,nb_p,loader,line_size,table_cost,home_addr_table,nb_m);
		}
		else
		{
			c_core = new soclib::caba::MccRamCore<vci_param,sram_param>(generated_name,node_zero,i_ident,t_ident,mt,mt_inv,cct,nb_p,loader,line_size,table_cost,home_addr_table,nb_m);
		}
#else
		if (mt_inv == NULL) // Only one NoC for requests and invalidation, pass &mt instead of &mt_inv as "Mapping table for invalidations"
		{
			c_core = new soclib::caba::MccRamCore<vci_param,sram_param>(generated_name,node_zero,i_ident,t_ident,mt,mt,cct,nb_p,line_size,table_cost,home_addr_table,nb_m);
		}
		else
		{
			c_core = new soclib::caba::MccRamCore<vci_param,sram_param>(generated_name,node_zero,i_ident,t_ident,mt,mt_inv,cct,nb_p,line_size,table_cost,home_addr_table,nb_m);
		}

		sprintf(generated_name,"%s.sram",(const char *)insname);
		c_sram_32 = new soclib::caba::SRam<sram_param>(generated_name,m_segment_list, loader);
#endif

		sprintf(generated_name,"%s.manometer",(const char *)insname);
		c_manometer = new soclib::caba::Manometer(generated_name);
		sprintf(generated_name,"%s.counters", (const char *)insname);
		c_counters = new soclib::caba::Counters(generated_name, nb_p, m_segment_list -> begin () -> size() / PAGE_SIZE, m_segment_list -> begin() -> baseAddress() , PAGE_SIZE);
		sprintf(generated_name,"%s.mig_control", (const char *)insname);
		c_mig_control = new soclib::caba::MigControl(generated_name, nb_p);

		// some checks, we use a 32bits Sram and for now we don't manage "address conversions". 
		assert(vci_param::N == 32);
		assert(vci_param::B == 4);
		assert(m_segment_list -> size() == 1); // Only one segment allowed per module

		// connect them
		c_core -> p_clk(p_clk);
		c_core -> p_resetn(p_resetn);
		c_core -> p_t_vci(p_t_vci);
		c_core -> p_i_vci(p_i_vci);

		c_core -> p_sram_bk(s_bk_core2sram);
		c_core -> p_sram_ce(s_ce_core2sram);
		c_core -> p_sram_oe(s_oe_core2sram);
		c_core -> p_sram_we(s_we_core2sram);
		c_core -> p_sram_be(s_be_core2sram);
		c_core -> p_sram_addr(s_addr_core2sram);
		c_core -> p_sram_dout(s_din_core2sram);
		c_core -> p_sram_din(s_dout_core2sram);
		c_core -> p_sram_ack(s_ack_core2sram);

		c_core -> p_manometer_req(s_core_req_core2manometer);

		c_core -> p_counters_enable(s_enable_core2counters);
		c_core -> p_counters_page_sel(s_page_sel_core2counters);
		c_core -> p_counters_node_id(s_node_id_core2counters);
		c_core -> p_counters_cost(s_cost_core2counters);

#if 0
		c_core -> p_in_ctrl_req(s_req_mig_control2core);
		c_core -> p_in_ctrl_cmd(s_cmd_mig_control2core);
		c_core -> p_in_ctrl_data_0(s_data_0_mig_control2core);
		c_core -> p_in_ctrl_data_1(s_data_1_mig_control2core);
		c_core -> p_in_ctrl_rsp_ack(s_rsp_ack_mig_control2core);

		c_core -> p_out_ctrl_req_ack(s_req_ack_core2mig_control);
		c_core -> p_out_ctrl_cmd(s_cmd_core2mig_control);
		c_core -> p_out_ctrl_rsp(s_rsp_core2mig_control);
		c_core -> p_out_ctrl_data_0(s_data_0_core2mig_control);
#endif

#ifndef DEBUG_SRAM
		c_sram_32 -> p_bk_sel(s_bk_core2sram);
		c_sram_32 -> p_ce(s_ce_core2sram);
		c_sram_32 -> p_oe(s_oe_core2sram);
		c_sram_32 -> p_we(s_we_core2sram);
		c_sram_32 -> p_be(s_be_core2sram);
		c_sram_32 -> p_addr(s_addr_core2sram);
		c_sram_32 -> p_din(s_din_core2sram);
		c_sram_32 -> p_dout(s_dout_core2sram);
		c_sram_32 -> p_ack(s_ack_core2sram);
#endif

		c_manometer -> p_clk(p_clk);
		c_manometer -> p_resetn(p_resetn);
		c_manometer -> p_core_req(s_core_req_core2manometer);
		c_manometer -> p_req(s_req_x2manometer);
		c_manometer -> p_cmd(s_cmd_x2manometer);
		c_manometer -> p_contention_ack(s_contention_ack_x2manometer);

		c_manometer -> p_valid(s_valid_manometer2x);
		c_manometer -> p_contention(s_contention_manometer2x);
		c_manometer -> p_ack(s_ack_manometer2x);
		c_manometer -> p_pressure(s_pressure_manometer2x);

		c_counters -> p_clk(p_clk);
		c_counters -> p_resetn(p_resetn);

		c_counters -> p_enable(s_enable_core2counters);
		c_counters -> p_page_sel(s_page_sel_core2counters);
		c_counters -> p_node_id(s_node_id_core2counters);
		c_counters -> p_cost(s_cost_core2counters);

		c_counters -> p_contention(s_contention_counters2ctrl);	

#if 0 // TODO
		c_counters -> p_req;	
		c_counters -> p_cmd;	
		c_counters -> p_page_id;	
		c_counters -> p_ack;	
		c_counters -> p_valid;	
		c_counters -> p_freeze;
		c_counters -> p_outpu;
#endif


		SC_METHOD (Transition);
		sensitive_pos << p_clk;
		dont_initialize();
	}; // end constructor

	tmpl(/**/)::~VciMccRam()
	{
		delete c_core;
		delete c_sram_32;
		delete c_manometer;
		delete c_counters;
		delete m_segment_list;
	};

}}
