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

		// Instanciate sub_modules
#ifdef DEBUG_SRAM
if (mt_inv == NULL) // Only one NoC for requests and invalidation, pass &mt instead of &mt_inv as "Mapping table for invalidations"
{
		c_core = new soclib::caba::MccRamCore<vci_param,sram_param>("c_core",node_zero,i_ident,t_ident,mt,mt,cct,nb_p,loader,line_size,table_cost,home_addr_table,nb_m);
}
else
{
		c_core = new soclib::caba::MccRamCore<vci_param,sram_param>("c_core",node_zero,i_ident,t_ident,mt,mt_inv,cct,nb_p,loader,line_size,table_cost,home_addr_table,nb_m);
}
#else
if (mt_inv == NULL) // Only one NoC for requests and invalidation, pass &mt instead of &mt_inv as "Mapping table for invalidations"
{
		c_core = new soclib::caba::MccRamCore<vci_param,sram_param>("c_core",node_zero,i_ident,t_ident,mt,mt,cct,nb_p,line_size,table_cost,home_addr_table,nb_m);
}
else
{
		c_core = new soclib::caba::MccRamCore<vci_param,sram_param>("c_core",node_zero,i_ident,t_ident,mt,mt_inv,cct,nb_p,line_size,table_cost,home_addr_table,nb_m);
}
c_sram_32 = new soclib::caba::SRam<sram_param>("c_sram",m_segment_list, loader);
#endif

c_manometer = new soclib::caba::Manometer("c_manometer");

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
		// TODO manometer

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
		// TODO other inputs

		SC_METHOD (Transition);
		sensitive_pos << p_clk;
		dont_initialize();
	}; // end constructor

	tmpl(/**/)::~VciMccRam()
	{
		delete c_core;
		delete m_segment_list;
	};

}}
