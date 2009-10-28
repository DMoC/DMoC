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

#include "../include/cc_ram_core.h"
#include "soclib_endian.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>


namespace soclib {
namespace caba {
#define tmpl(x) template<typename vci_param, typename sram_param> x CcRamCore<vci_param,sram_param>

	/////////////////////////////////////////////////////
	//    genMoore() method
	/////////////////////////////////////////////////////
	tmpl(void)::genMoore()
	{

// Setting Vci and Ctrl output signals

		switch (r_RAM_FSM.read()) {

			case RAM_IDLE :
#ifdef DEBUG_SRAM
				m_vci_fsm . genMoore();
#endif
				break;

#if 0
			case RAM_REGISTERS_REQ :
			case RAM_REGISTERS_END :
#endif
			case RAM_DATA_INVAL :
			case RAM_DATA_INVAL_WAIT :
			case RAM_DIRUPD :
				p_t_vci.cmdack = false;
				p_t_vci.rspval = false;
				break;

			default :
				assert(false); // unknown state
				break;

		} // end switch r_RAM_FSM


		switch (r_INV_FSM.read()) {

			case RAM_INV_IDLE :
				p_i_vci.cmdval  = false;
				p_i_vci.rspack  = false;
				p_i_vci.address = 0;
				p_i_vci.wdata   = 0;
				p_i_vci.be      = 0;
				p_i_vci.plen    = 0;
				p_i_vci.cmd     = vci_param::CMD_NOP;
				p_i_vci.trdid   = 0;
				p_i_vci.pktid   = 0;
				p_i_vci.srcid   = 0;
				p_i_vci.cons    = false;
				p_i_vci.wrap    = false;
				p_i_vci.contig  = false;
				p_i_vci.clen    = 0;
				p_i_vci.cfixed  = false;
				p_i_vci.eop     = false;
				break;

			case RAM_INV_REQ :
				// Invalidation command coded as a CMD_WRITE,
				// address offset depends on DATA or TLB invalidation (0x0 or 0x4).
				p_i_vci.cmdval  = true;
				p_i_vci.rspack  = false;
				p_i_vci.address = r_INV_TARGETADDRESS.read();
				p_i_vci.wdata   = r_INV_BLOCKADDRESS.read();
				p_i_vci.be      = 0xF;
				p_i_vci.plen    = 1;
				p_i_vci.cmd			= vci_param::CMD_WRITE;
				p_i_vci.trdid   = 0;
				p_i_vci.pktid   = 0;
				p_i_vci.srcid   = m_I_IDENT;
				p_i_vci.cons    = false;
				p_i_vci.wrap    = false;
				p_i_vci.contig  = false;
				p_i_vci.clen    = 0;
				p_i_vci.cfixed  = false;
				p_i_vci.eop     = true;
				break;

			case RAM_INV_RSP :
				p_i_vci.cmdval  = false;
				p_i_vci.rspack  = true;
				p_i_vci.address = 0;
				p_i_vci.wdata   = 0;
				p_i_vci.be      = 0;
				p_i_vci.plen    = 0;
				p_i_vci.cmd     = vci_param::CMD_NOP;
				p_i_vci.trdid   = 0;
				p_i_vci.pktid   = 0;
				p_i_vci.srcid   = 0;
				p_i_vci.cons    = false;
				p_i_vci.wrap    = false;
				p_i_vci.contig  = false;
				p_i_vci.clen    = 0;
				p_i_vci.cfixed  = false;
				p_i_vci.eop     = false;
				break;

			default :
				assert(false); // State that does not exist!
				break;

		} // end switch r_INV_FSM

		p_sram_ce   = m_sram_ce;
		p_sram_oe   = m_sram_oe;
		p_sram_we   = m_sram_we;
		p_sram_be   = m_sram_be;
		p_sram_addr = m_sram_addr;
		p_sram_dout =  m_sram_wdata;
		m_sram_ce = false; // CE set for one cycle only
	}; // end genMoore()
}}
