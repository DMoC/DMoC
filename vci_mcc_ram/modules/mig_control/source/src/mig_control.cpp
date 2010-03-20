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

		MigControl::MigControl(sc_module_name insname,unsigned int nbproc):
			soclib::caba::BaseModule(insname),
			m_NBPROC(nbproc),
			d_cycles(0),
			r_disable_contention_raise("DISABLE_CONTENTION_RAISE"),
			r_master_transaction("MASTER_TRANSACTION"),
			r_slave_transaction("SLAVE_TRANSACTION"),
			r_pending_reset("PENDING_RESET"),
			r_c_max_p("C_MAX_P"),
			r_c_virt_max_p_addr("C_VIRT_MAX_P_ADDR"),
			r_c_index_cpt("C_INDEX_CPT"),
			r_MIG_M_CTRL_FSM("MIG_M_CTRL_FSM"),
			r_MIG_PP_CTRL_FSM("MIG_PP_CTRL_FSM"),
			r_MIG_PT_CTRL_FSM("MIG_PT_CTRL_FSM"),
			r_MIG_C_CTRL_FSM("MIG_C_CTRL_FSM")
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
			SC_METHOD (transition);
			sensitive_pos << p_clk;
			dont_initialize();

			SC_METHOD (genMoore);
			sensitive_neg << p_clk;
			dont_initialize();

		}

		MigControl::~MigControl()
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
