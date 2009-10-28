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
 * 	   Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2006-2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

///////////////////////////////////////
//  genMoore method
///////////////////////////////////////

#include "vci_mcc_cache.h"
#define tmpl(x)  template<typename vci_param, typename iss_t> x VciMccCache<vci_param, iss_t>
namespace soclib { 
namespace caba {

tmpl(void)::genMoore(){

const unsigned int dec = r_REQ_DCACHE_ADDR_PHYS.read() & 0x3;
	switch ((rsp_fsm_state_e)r_VCI_RSP_FSM.read())
	{

		case RSP_IDLE :
			p_i_vci.rspack = false;
			break;


		case RSP_UNC :
		case RSP_IMISS :
		case RSP_DMISS :
		case RSP_WRITE :
		case RSP_TLB_DMISS :
		case RSP_TLB_IMISS :
		case RSP_INVAL_SEND_ACK :
			p_i_vci.rspack = true;
			break;

		default :
			assert(false); //Error, state that does not exist!
			break;
	} 

	switch (r_VCI_REQ_FSM)
	{

		case REQ_IDLE :
		case REQ_DWAIT :
		case REQ_IWAIT :
		case REQ_TLB_INVAL :
			p_i_vci.cmdval  = false;

			p_i_vci.plen    = 0;
			p_i_vci.cmd     = vci_param::CMD_NOP;
			p_i_vci.trdid   = 0;
			p_i_vci.pktid   = 0;
			p_i_vci.srcid   = m_i_ident;
			p_i_vci.cons    = false;
			p_i_vci.wrap    = false;
			p_i_vci.contig  = false;
			p_i_vci.clen    = 0;
			p_i_vci.cfixed  = false;
			p_i_vci.eop     = false;
			break;

		case REQ_LL :
			assert(r_DCACHE_BE_SAVE.read() == 0xF); // assert OK for a 32bits addr. LL and SC are at word level only
		case REQ_UNC :
			p_i_vci.cmdval = true;
			p_i_vci.address = (r_REQ_DCACHE_ADDR_PHYS.read() & (~0x3));
			p_i_vci.plen = soclib::common::fls(r_DCACHE_BE_SAVE.read())-ffs(r_DCACHE_BE_SAVE.read())+1;
			assert((soclib::common::fls(r_DCACHE_BE_SAVE.read())-ffs(r_DCACHE_BE_SAVE.read())+1) != 0);
			p_i_vci.cmd    = r_DCACHE_LL_SAVE.read() ? vci_param::CMD_LOCKED_READ : vci_param::CMD_READ;
			p_i_vci.be     = r_DCACHE_BE_SAVE.read();
			p_i_vci.trdid  = 0;
			p_i_vci.pktid  = (r_REQ_DCACHE_ADDR_VIRT.read() >> m_PAGE_SHIFT);
			p_i_vci.srcid  = m_i_ident;
			p_i_vci.cons   = false;
			p_i_vci.wrap   = false;
			p_i_vci.contig = true;
			p_i_vci.clen   = 0;
			p_i_vci.cfixed = false;
			p_i_vci.eop    = true;
			break;

		case REQ_SC :
			p_i_vci.cmdval = true;
			p_i_vci.address = (r_REQ_DCACHE_ADDR_PHYS.read() & (~0x3));
			p_i_vci.wdata   = r_REQ_DCACHE_DATA.read() << dec*8;
			p_i_vci.be     = r_DCACHE_BE_SAVE.read();
			assert(r_DCACHE_BE_SAVE.read() == 0xF); // assert OK for a 32bits addr. LL and SC are at word level only
			p_i_vci.plen   = vci_param::B;
			p_i_vci.cmd    = vci_param::CMD_STORE_COND;
			p_i_vci.trdid  = 0;
			p_i_vci.pktid  = (r_REQ_DCACHE_ADDR_VIRT.read() >> m_PAGE_SHIFT);
			p_i_vci.srcid  = m_i_ident;
			p_i_vci.cons   = false;
			p_i_vci.wrap   = false;
			p_i_vci.contig = true;
			p_i_vci.clen   = 0;
			p_i_vci.cfixed = false;
			p_i_vci.eop = true;
			break;

		case REQ_WRITE :
			p_i_vci.cmdval = true;
			p_i_vci.address = (r_REQ_DCACHE_ADDR_PHYS.read() & (~0x3));
			p_i_vci.wdata   = r_REQ_DCACHE_DATA.read() << dec*8;
			p_i_vci.be     = r_DCACHE_WF_BE_SAVE.read();
			p_i_vci.plen = (vci_param::B * r_WRITE_BURST_SIZE.read()) - (vci_param::B - soclib::common::fls(r_DCACHE_WF_BE_SAVE.read()))  - soclib::common::ctz((typename vci_param::be_t)r_DCACHE_WL_BE_SAVE.read());
			p_i_vci.cmd    = vci_param::CMD_WRITE;
			p_i_vci.trdid  = 0;
			p_i_vci.pktid  = (r_REQ_DCACHE_ADDR_VIRT.read() >> m_PAGE_SHIFT);
			p_i_vci.srcid  = m_i_ident;
			p_i_vci.cons   = false;
			p_i_vci.wrap   = false;
			p_i_vci.contig = true;
			p_i_vci.clen   = 0;
			p_i_vci.cfixed = false;
			p_i_vci.eop = !(m_WRITE_BUFFER_DATA.rok());
			break;

		case REQ_DMISS :
			p_i_vci.cmdval = true;
			p_i_vci.address = (r_REQ_DCACHE_ADDR_PHYS.read() & m_dcache_yzmask );
			p_i_vci.be     = 0xf;
			p_i_vci.plen   = s_dcache_words << m_plen_shift;
			p_i_vci.cmd    = vci_param::CMD_READ;
			p_i_vci.trdid  = 0;
			p_i_vci.pktid  = (r_REQ_DCACHE_ADDR_VIRT.read() >> m_PAGE_SHIFT);
			p_i_vci.srcid  = m_i_ident;
			p_i_vci.cons   = false;
			p_i_vci.wrap   = false;
			p_i_vci.contig = true;
			p_i_vci.clen   = 0;
			p_i_vci.cfixed = false;
			p_i_vci.eop = true; 
			break;


		case REQ_IMISS :
			p_i_vci.cmdval = true;
			p_i_vci.address = (r_REQ_ICACHE_ADDR_PHYS.read() & m_icache_yzmask);
			p_i_vci.be     = 0xf;
			p_i_vci.plen   = (s_icache_words << m_plen_shift);
			p_i_vci.cmd    = vci_param::CMD_READ;
			p_i_vci.trdid  = 0;
			p_i_vci.pktid  = (r_ICACHE_MISS_ADDR_SAVE.read() >> m_PAGE_SHIFT);
			p_i_vci.srcid  = m_i_ident;
			p_i_vci.cons   = false;
			p_i_vci.wrap   = false;
			p_i_vci.contig = true;
			p_i_vci.clen   = 0;
			p_i_vci.cfixed = false;
			p_i_vci.eop = true; 
			break;

		case REQ_TLB_IMISS :
			p_i_vci.cmdval = true;
			DCACHECOUT(2) << name() << " tlb imiss sent to : 0x" << hex << r_REQ_ICACHE_ADDR_PHYS.read() << endl;
			p_i_vci.address = r_REQ_ICACHE_ADDR_PHYS.read();
			p_i_vci.be     = 0xf;
			p_i_vci.plen   = vci_param::B;
			p_i_vci.cmd    = vci_param::CMD_READ_TLB_MISS;
			p_i_vci.trdid  = 0;
			p_i_vci.pktid  = 0;
			p_i_vci.srcid  = m_i_ident;
			p_i_vci.cons   = false;
			p_i_vci.wrap   = false;
			p_i_vci.contig = true;
			p_i_vci.clen   = 0;
			p_i_vci.cfixed = false;
			p_i_vci.eop = true; 
			break;

		case REQ_TLB_DMISS :
			p_i_vci.cmdval = true;
			p_i_vci.address = r_REQ_DCACHE_ADDR_PHYS.read();
			p_i_vci.be     = 0xf;
			p_i_vci.plen   = vci_param::B;
			p_i_vci.cmd    = vci_param::CMD_READ_TLB_MISS;
			p_i_vci.trdid  = 0;
			p_i_vci.pktid  = 0;
			p_i_vci.srcid  = m_i_ident;
			p_i_vci.cons   = false;
			p_i_vci.wrap   = false;
			p_i_vci.contig = true;
			p_i_vci.clen   = 0;
			p_i_vci.cfixed = false;
			p_i_vci.eop = true; 
			break;
			
		case REQ_TLB_INVAL_SEND_ACK :
			p_i_vci.cmdval = true;
			p_i_vci.address = r_TLB_INV_ACK_ADDR.read();
			p_i_vci.be     = 0xf;
			p_i_vci.plen   = vci_param::B;
			p_i_vci.cmd    = vci_param::CMD_TLB_INV_ACK;
			p_i_vci.trdid  = 0;
			p_i_vci.pktid  = 0;
			p_i_vci.srcid  = m_i_ident;
			p_i_vci.cons   = false;
			p_i_vci.wrap   = false;
			p_i_vci.contig = true;
			p_i_vci.clen   = 0;
			p_i_vci.cfixed = false;
			p_i_vci.eop = true; 
			break;

		default :
			assert(false); //Error, state that does not exist!
			break;

	} // end switch vci_req_fsm

	// VCI INV

	switch ((inv_fsm_state_e)r_VCI_INV_FSM.read()) {

		case INV_IDLE :
		case INV_WAIT :
			p_t_vci.cmdack = false;
			p_t_vci.rspval = false;
			//p_t_vci.rdata  = UNDEFINED;
			p_t_vci.rerror = 0;
			p_t_vci.rsrcid = r_SAV_SCRID.read();
			p_t_vci.rpktid = 0;
			p_t_vci.rtrdid = 0;
			p_t_vci.reop   = false;
			break;

		case INV_CMD :
		case INV_CMD_TLB :
			p_t_vci.cmdack = true;
			p_t_vci.rspval = false;
			//p_t_vci.rdata  = UNDEFINED;
			p_t_vci.rerror = 0;
			p_t_vci.rsrcid = 0;
			p_t_vci.rpktid = 0;
			p_t_vci.rtrdid = 0;
			p_t_vci.reop   = false;
			break;


		case INV_RSP :
			p_t_vci.cmdack = false;
			p_t_vci.rspval = true;
			p_t_vci.rdata  = 0;
			p_t_vci.rerror = 0;
			p_t_vci.rsrcid = r_SAV_SCRID.read();
			p_t_vci.rpktid = 0;
			p_t_vci.rtrdid = 0;
			p_t_vci.reop   = true;
			break;

		default :
			assert(false); //Error, state that does not exist!
			break;
	} // end switch VCI_INV_FSM

}; // end genMoore()
}}
