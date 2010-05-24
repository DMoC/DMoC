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
 * Copyright (c) UPMC, Lip6, Asim
 *         Adapted from soclib_vci_multiram_wt.h by Frederic Arzel 20/09/2004
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
#include "globals.h"


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
			const soclib::common::MappingTable &mt,
			const soclib::common::MappingTable &mt_inv,
			soclib::common::CcIdTable * cct,
			const unsigned int nb_p,
			const soclib::common::Loader &loader,
			const unsigned int line_size,
			unsigned int * table_cost,
			addr_to_homeid_entry_t * home_addr_table,
			unsigned int nb_m) :
		caba::BaseModule(insname),
		m_vci_fsm(p_t_vci, mt.getSegmentList(t_ident), (uint32_log2(PAGE_SIZE)) ),
		m_PAGE_SHIFT(uint32_log2(PAGE_SIZE)),

		r_RAM_FSM("RAM_FASM"),
		r_INV_FSM("INV_FSM"),
		r_IN_TRANSACTION("IN_TRANSACTION"),
		r_SAV_ADDRESS("SAV_ADDRESS"),
		r_SAV_SEGNUM("SAV_SEGNUM"),
		r_SAV_SCRID("SAV_SCRID"),
		r_SAV_BLOCKNUM("SAV_BLOCKNUM"),
		r_INV_BLOCKADDRESS("INV_BLOCKADDRESS"),
		r_INV_TARGETADDRESS("INV_TARGETADDRESS"),
		r_INV_ADDR_OFFSET("INV_ADDR_OFFSET"),
		r_WAITING_TLB_INV_ACK("WAITING_TLB_INV_ACK"),
		m_loader(loader),
		m_MapTab(mt),
		m_MapTab_inv(mt_inv),
		m_cct(cct),
		m_SEGLIST(mt.getSegmentList(t_ident)),
#if 0
		m_NB_SEG(mt.getSegmentList(t_ident).size()/2),
#else
		m_NB_SEG(1), // waiting for counter access interface
#endif
		m_BLOCK_SIZE(line_size),
		m_ADDR_BLOCK_SHIFT(uint32_log2(vci_param::B) + uint32_log2(line_size)),
		m_ADDR_WORD_SHIFT(uint32_log2(vci_param::B)),
		m_I_IDENT(mt_inv.indexForId(i_ident)),
		m_NB_PROCS(nb_p),
		m_BLOCKMASK((~0)<<(uint32_log2(line_size) + uint32_log2(vci_param::B))),
		m_LSBMASK(~((~0)<< uint32_log2(vci_param::B))),
		m_node_zero(node_zero),
		m_nbcycles(0),
		m_waiting(0),
		m_last_write_nack(false),
		m_last_address(0)

	{
		std::list<soclib::common::Segment>::iterator     iter;
		unsigned int	base;
		unsigned int	size;
		unsigned int	index;

		std::list<soclib::common::Segment>    temp; // target segment for invalidations

		m_vci_fsm.on_read_write_nlock_tlb(on_read, on_write);
		m_vci_fsm.on_tlb_miss_ack_nlock_tlb(on_tlb_miss, on_tlb_ack);
		m_vci_fsm.on_is_busy_nlock_tlb(is_busy);

		CTOR_OUT
#ifdef USE_STATS
			stats_chemin = "./";
			stats_chemin += name().c_str();
			stats_chemin += ".stats"; 
			file_stats.setf(ios_base::hex);
			cout << "chemin : " << stats_chemin.c_str() << endl;
			file_stats.open(stats_chemin.c_str(),ios::trunc);
#endif
		SC_METHOD (transition);
		sensitive_pos << p_clk;
		dont_initialize();

		SC_METHOD (genMoore);
		sensitive_neg << p_clk;
		dont_initialize();
		// internal modules 
		
		m_SEG_5_LIST = new std::list<soclib::common::Segment>;

		// Now, we only support one segment! This is due to contention computation
		// and counters.
		DRAMCOUT(0) << "   m_NB_SEG->" << m_NB_SEG << endl; 
#if ONE_SEG_MODE
		assert(m_NB_SEG == 1);
#endif

		// set the iterator on the first counter segment
		for (iter = m_SEGLIST.begin(), index=0 ; index < m_NB_SEG ; index++) {
			m_SEG_5_LIST->push_back(*iter);
			iter++;
		}

		
		
		m5_name << "r" << m_I_IDENT << "_m5";
		m_5 = new Module_5 (m5_name.str(),m_SEG_5_LIST,nb_p,PAGE_SIZE);
		m6_name << "r" << m_I_IDENT << "_m6";
		m_6 = new Module_6 (m6_name.str(), m_node_zero, m_SEGLIST.begin()->baseAddress(), m_BLOCK_SIZE,  nb_m,nb_p,m_I_IDENT,table_cost, home_addr_table,this);
		// Connecting m_5 and m_6
		m_6->p_out.fifo          = &(m_5->p_in->m6_fifo);
		m_6->p_out.data          = &(m_5->p_in->m6_data);
		m_6->p_out.cmd           = &(m_5->p_in->m6_cmd);
		m_6->p_out.ack           = &(m_5->p_in->m6_ack);
		m_6->p_out.req           = &(m_5->p_in->m6_req);

		m_6->p_in.fifo          = &(m_5->p_out->m6_fifo);
		m_6->p_in.data          = &(m_5->p_out->m6_data);
		m_6->p_in.cmd           = &(m_5->p_out->m6_cmd);
		m_6->p_in.ack           = &(m_5->p_out->m6_ack);
		m_6->p_in.req           = &(m_5->p_out->m6_req);
		m_6->p_in.pressure      = &(m_5->p_out->m6_pressure);
		
		// Connecting m_6 to vci interfaces
		m_6->p_in.ring          = &(m_m6_in); // RING IN
		m_6->p_out.ring         = &(m_m6_out); // RING OUT

		#if 0
		// Need to copy value by value and cannot connect m_m6_in/out to the port since they
		// are not sc_signals
		p_ring_in(m_m6_in);
		p_ring_out(m_m6_out);
		#endif

		//m_5->p_in->m6_fifo_wok = true;
		// TODO
		// TODO
		// TODO
		// TODO

#if 0
		assert(!(mt.getSegmentList(t_ident).size()%2)); // Nombre pair de segment (1 segment compteurs par segment donnée)
#endif
		m_alias_registers = new int[m_NB_SEG];
		for (unsigned int i = 0; i < m_NB_SEG; i++){
			m_alias_registers[i] = i + m_NB_SEG; // seg_i -> counters_i
		}
		

		// page table allocation, 1 per segment
		m_PAGE_TABLE = new PHYS_PAGE_TABLE * [m_NB_SEG];
		r_RAM = new unsigned int * [m_NB_SEG];
		r_DIRECTORY = new SOCLIB_DIRECTORY * [m_NB_SEG];
		r_POISONNED = new bool * [m_NB_SEG];
		m_SIZE = new unsigned int[m_NB_SEG];
		m_BASE = new unsigned int[m_NB_SEG];
		m_DIR_SIZE = new unsigned int[m_NB_SEG];
		// segments allocation
#ifdef USE_STATS
		m_nb_poisonned_nack = new unsigned long long [m_NB_PROCS];
		m_nb_busy_nack = new unsigned long long [m_NB_PROCS];
		m_nb_success = new unsigned long long [m_NB_PROCS];
		for (unsigned int i = 0; i < m_NB_PROCS ; i++)
		{
			m_nb_poisonned_nack[i] = 0;
			m_nb_busy_nack[i] = 0;
			m_nb_success[i] = 0;
		}
#endif
		for (iter = m_SEGLIST.begin(), index=0 ; index < m_NB_SEG ; ++iter, index++) {
			base=(*iter).baseAddress();
			size=(*iter).size();
			assert(!(base & m_LSBMASK));  // base must be word aligned
			assert(!(size & m_LSBMASK));  // size must be word multiple
			m_SIZE[index]    = size;
			m_BASE[index]    = base;
			r_RAM[index]     = new unsigned int[size >> m_ADDR_WORD_SHIFT];
			m_DIR_SIZE[index]    = (size >> m_ADDR_BLOCK_SHIFT);        // number of lines
			unsigned int test    = (size >> 2)/m_BLOCK_SIZE;			// todo, suppress this test
			assert(m_DIR_SIZE[index] == test);

			r_DIRECTORY[index]   = new SOCLIB_DIRECTORY[m_DIR_SIZE[index]];     // 1 line per block
			for (unsigned int i = 0; i < m_DIR_SIZE[index]; i++){
				r_DIRECTORY[index][i].init(m_NB_PROCS);
			}
			r_POISONNED[index] = new bool [size/PAGE_SIZE];
			m_PAGE_TABLE[index] = new PHYS_PAGE_TABLE(size/PAGE_SIZE,PAGE_SIZE,base);
			assert(!(size%PAGE_SIZE));
		}  // end for
#if ONE_SEG_MODE
		std::cout << name() << std::dec << " [nombre de pages] : "  << (m_SIZE[0]/PAGE_SIZE) << std::endl;
		r_TLB_DIRECTORY = new SOCLIB_DIRECTORY [m_SIZE[0]/PAGE_SIZE];
		for (unsigned int i = 0; i < (m_SIZE[0]/PAGE_SIZE) ; i++)
				r_TLB_DIRECTORY[i].init(m_NB_PROCS);
#else
		assert(false);
#endif

	}; // end constructor

	tmpl(/**/)::~VciMccRam(){
		DTOR_OUT
#ifdef USE_STATS
		file_stats << dec <<  " Poisonned/busy/access NACKS : ";
		for (unsigned int i = 0; i < m_NB_PROCS ; i ++)
		{
			file_stats <<" "<<  m_nb_poisonned_nack[i] 
				<< "/" << m_nb_busy_nack[i]
				<< "/" << (m_nb_busy_nack[i] + m_nb_poisonned_nack[i] + m_nb_success[i]); 
		}
		file_stats << endl;
		file_stats.close();
		delete [] m_nb_busy_nack;
		delete [] m_nb_poisonned_nack;
		delete [] m_nb_success;
#endif

		delete m_5;	
		delete m_6;	
		delete m_SEG_5_LIST;
		for (unsigned int i = 0; i < m_NB_SEG; i++){
			delete [] r_RAM[i];
			delete [] r_DIRECTORY[i];
			delete [] r_POISONNED[i];
			delete m_PAGE_TABLE[i];
		}	
		delete [] r_RAM;
		delete [] r_DIRECTORY;
		delete [] r_TLB_DIRECTORY;
		delete [] r_POISONNED;
		delete [] m_SIZE;
		delete [] m_BASE;
		delete [] m_DIR_SIZE;
		delete [] m_PAGE_TABLE;

	};
	tmpl(void)::alias(const std::string seg, const std::string register_seg){
		int i = 0;
		int reg_id = -1;
		int seg_id = -1;
		bool found_seg = false;
		bool found_reg = false;
		std::list<soclib::common::Segment>::iterator     iter;
		for (iter = m_SEGLIST.begin() ; iter != m_SEGLIST.end() ; ++iter) {
			if (iter->name() == seg){
				seg_id = i;
				assert(!found_seg);
				found_seg = true;	
			}
			if (iter->name() == register_seg){
				reg_id = i; 
				assert(!found_reg);
				found_reg = true;	
			}
			i++;
		}
		if (found_reg && found_seg){
			assert(seg_id != -1);
			assert(reg_id != -1);
			m_alias_registers[seg_id] = reg_id;
			DRAMCOUT(0) << "aliasing : " << std::dec << reg_id << " -> " << seg_id << std::endl;
		}else{
			assert(false);
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//        initSegmentExec()
	//    This method is used to load executable binary code 
	//    in a given segment.
	//    - char    *segname    : segment name
	//    - char    *filename    : binary file pathname
	//    - char  **sections    : pointer on a table of section names
	//////////////////////////////////////////////////////////////////////////
	tmpl(void)::reload()
	{
		for ( size_t i=0; i<m_NB_SEG; ++i ) {
			m_loader.load(&r_RAM[i][0], m_vci_fsm.getBase(i), m_vci_fsm.getSize(i));
			for ( size_t addr = 0; addr < m_vci_fsm.getSize(i)/vci_param::B; ++addr )
				r_RAM[i][addr] = le_to_machine(r_RAM[i][addr]);
		}
	}

	///////////////////////////////////////////////////////
	//    rw_seg() method
	//    The memory organisation is litle endian
	///////////////////////////////////////////////////////        


	tmpl(unsigned int)::rw_seg(unsigned int* tab, unsigned int index, unsigned int wdata, unsigned int be, unsigned int cmd)
	{
		unsigned int mask = 0;
		if (cmd == vci_param::CMD_READ) {    // read
			return(tab[index]);
		} else if(cmd == vci_param::CMD_WRITE) {        
			if ( be & 1 ) mask |= 0x000000ff;
			if ( be & 2 ) mask |= 0x0000ff00;
			if ( be & 4 ) mask |= 0x00ff0000;
			if ( be & 8 ) mask |= 0xff000000;

			tab[index] = (tab[index] & ~mask) | (wdata & mask);
			return(0);
		} else {
			assert(false); // unsupported request
		} // end write
	} // end rw_seg()

	//////////////////////////////////////////////
	//    Directory Initialisation method 
	//////////////////////////////////////////////
	tmpl(void)::directory_init()
	{
		unsigned int i;
		for (i=0; i < m_NB_SEG; i++) {
			// todo Directory should be initialized at reset time, but it can be time
			// consumming to initialize 1 gb memory !!
			
			//Directory initialized at construction
			//         for(j=0; j < m_DIR_SIZE[i]; j++){
			//             DIRECTORY[i][j] = 0;
			//         }
		}
	}


tmpl(bool)::is_busy(void)
{

	if (r_POISON_REQ){
		return true;
	}

	if (r_WAITING_TLB_INV_ACK){
		if (r_SAVE_TLB_DIRECTORY.Is_empty())
		{
			return true;
		}
	}

	if (r_UNPOISON_REQ){
		return true;
	}

	if (m_5->p_out->updt_tlb_req){
		return true;
	}

	if (m_5->p_out->updt_wiam_req){
		return true;
	}
	return false;
}

tmpl(bool)::on_write(size_t seg, vci_addr_t addr, vci_data_t data, int be, bool eop)
{
	unsigned int srcid = m_cct->translate_to_id(m_vci_fsm.currentSourceId());
	unsigned int rsrcid = m_vci_fsm.currentSourceId();
	unsigned int page_index = addr >> m_PAGE_SHIFT;

	if (r_IN_TRANSACTION.read() == true)
	{
		if ((m_last_address + vci_param::B) != addr)
		{
			cout << std::hex <<  name() << " [W] > virt : " << (m_PAGE_TABLE[0] -> wiam(addr)) << endl;
			cout << name() << "     > phys : " << addr << endl;
			cout << name() << "     > last : " << m_last_address << endl;
			cout << name() << "     > srcid: " << rsrcid << endl;
		}
		assert((m_last_address + vci_param::B) == addr);

		if (m_last_write_nack)
			assert(r_POISONNED[0][page_index]);
	}

	m_last_address = addr;

	int blocknum = (((addr) & m_BLOCKMASK) >> m_ADDR_BLOCK_SHIFT);

	DRAMCOUT(0) << name() << " [RAM_WRITE] " << endl;

	m_BLOCKSTATE = r_DIRECTORY[seg][blocknum];
	m_dir_accessed_idle_state = true;
	r_IN_TRANSACTION = !eop;
	#if 1
	if (m_last_write_nack)
	{ // Ne pas prendre en compte un "demi" paquet
		if (eop)
		{
			m_last_write_nack = false;
		}
		return false;
	}
	#endif
	if (r_POISONNED[0][page_index])
	{ // recalculer la page!
		m_last_write_nack = ! eop;
		return false;
	}
	else
	{


	DRAMCOUT(1) << name() << " [W] > virt : " << (m_PAGE_TABLE[0] -> wiam(addr)) << endl;
	DRAMCOUT(1) << name() << "     > phys : " << addr << endl;
	DRAMCOUT(1) << name() << "     > id   : " << srcid << endl;
	DRAMCOUT(1) << name() << "     > srcid: " << rsrcid << endl;
#if 0
		if (((m_vci_fsm.currentPktid() != ((m_PAGE_TABLE[0] -> wiam(addr)) >> m_PAGE_SHIFT ))) && (srcid < m_NB_PROCS))
		{
			cerr << name() << " error : accessing a data with the wrong tag " << endl;
			cerr << name() << "         request to phys address     0x " << hex << addr <<  endl;
			cerr << name() << "         with virtual address        0x " << hex << m_vci_fsm.currentPktid() << endl;
			cerr << name() << "         but mapped virt address is  0x " << hex << m_PAGE_TABLE[0] -> wiam(addr) << endl;
			cerr << name() << "         procid is  " << dec << srcid << endl;
		}
		assert((m_vci_fsm.currentPktid() == ((m_PAGE_TABLE[0] -> wiam(addr)) >> m_PAGE_SHIFT )) || (srcid >= m_NB_PROCS));
#endif
		if (eop)
		{
			m_segnum_5	 = seg;
			m_node_id_5	 = srcid;
			m_page_sel_5 = page_index; //((address & ~m_LSBMASK) - iter->baseAddress()) >> m_PAGE_SHIFT;
			m_cost_5	 = 1;   // default cost, todo
			m_enable_5	 = true;

		}
		rw_seg(r_RAM[seg], addr/vci_param::B, data, be, vci_param::CMD_WRITE);
		if (m_BLOCKSTATE.Is_Other(srcid) && eop ) // Send invalidation only if it is the last cell of the paquet in order to avoid deadlocks (shared - incomming fifo)
		{ // "hit"
			r_SAV_ADDRESS = m_vci_fsm.getBase(seg)+addr;
			r_SAV_SEGNUM = seg;
			r_SAV_BLOCKNUM = blocknum;
			r_SAV_SCRID = rsrcid;

			r_RAM_FSM = RAM_INVAL;
			if (srcid < m_NB_PROCS)
			{      // directory will raise an error if we modify a presence bit out of range (0 - nb processors -1)
				m_BLOCKSTATE.Reset_p(srcid); //  this cannot be done in the same clock cycle,
			}				 //  but may be avoided by working in INV_FSM on the 
			//  directory with a boolean mask based on r_SAV_SCRID
			//  Nevertheless, it is easier to use this method
			assert(m_BLOCKSTATE.Is_Other(srcid));
		}
		return true;
	}
}

tmpl(bool)::on_read(size_t seg, vci_addr_t addr, vci_data_t &data, bool eop)
{
	int blocknum = (((addr) & m_BLOCKMASK) >> m_ADDR_BLOCK_SHIFT);
	unsigned int srcid = m_cct->translate_to_id(m_vci_fsm.currentSourceId());
	unsigned int rsrcid = m_vci_fsm.currentSourceId();
	unsigned int page_index = addr >> m_PAGE_SHIFT;

	//assert(r_IN_TRANSACTION.read() == false);

	// access directory
	m_BLOCKSTATE = r_DIRECTORY[seg][blocknum];
	m_dir_accessed_idle_state = true;

	r_IN_TRANSACTION = !eop;
	if (m_last_write_nack)
	{ // Ne pas prendre en compte un "demi" paquet
		if (eop)
		{
			m_last_write_nack = false;
		}
		return false;
	}
	if (r_POISONNED[0][page_index])
	{ // recalculer la page!
#ifdef USE_STATS
		if (srcid < m_NB_PROCS)
			m_nb_poisonned_nack[srcid] = m_nb_poisonned_nack[srcid] + 1;
#endif
		m_last_write_nack = !eop;
		data = 0xdeadbeef;	
		return false;
	}
	else
	{

		DRAMCOUT(1) << name() << " [R] > virt : " << (m_PAGE_TABLE[0] -> wiam(addr)) << endl;
		DRAMCOUT(1) << name() << "     > phys : " << addr << endl;
		DRAMCOUT(1) << name() << "     > id   : " << srcid << endl;
		DRAMCOUT(1) << name() << "     > srcid: " << rsrcid << endl;
#if 0
		if ((m_vci_fsm.currentPktid() != ((m_PAGE_TABLE[0] -> wiam(addr)) >> m_PAGE_SHIFT)) && (srcid < m_NB_PROCS))
		{
			cerr << name() << " error : accessing a data with the wrong tag " << endl;
			cerr << name() << "         request to phys address     0x " << hex << addr <<  endl;
			cerr << name() << "         with virtual address        0x " << hex << m_vci_fsm.currentPktid() << endl;
			cerr << name() << "         but mapped virt address is  0x " << hex << m_PAGE_TABLE[0] -> wiam(addr) << endl;
			cerr << name() << "         procid is  " << dec << srcid << endl;
		}
		assert((m_vci_fsm.currentPktid() == (( m_PAGE_TABLE[0] -> wiam(addr) ) >> m_PAGE_SHIFT) ) || (srcid >= m_NB_PROCS));
#endif
		if (eop)
		{
			m_segnum_5		= seg;
			m_node_id_5		= srcid;
			m_page_sel_5	= page_index; //((address & ~m_LSBMASK) - iter->baseAddress()) >> m_PAGE_SHIFT;
			m_cost_5		= 1;   // default cost, todo
			m_enable_5		= true;

		}
		if ((m_BLOCKSTATE.Is_p(srcid)==false)
				&& (srcid < m_NB_PROCS) && eop) {
			// update necessary    
			r_SAV_ADDRESS = m_vci_fsm.getBase(seg)+addr;
			r_SAV_SEGNUM = seg;
			r_RAM_FSM = RAM_DIRUPD;
			r_SAV_BLOCKNUM = blocknum;
			r_SAV_SCRID = rsrcid;
		}
		data  = r_RAM[seg][addr/vci_param::B];
#ifdef USE_STATS
		if (srcid < m_NB_PROCS)
			m_nb_success[srcid] = m_nb_success[srcid] + 1;
#endif
		return true;
	}
}

tmpl(bool)::on_tlb_miss(size_t seg, vci_addr_t addr, vci_data_t &data )
{
	//if (r_RAM_FSM != RAM_IDLE) return false; // NAck!
	unsigned int srcid = m_cct->translate_to_id(m_vci_fsm.currentSourceId());
	unsigned int page_index = addr >> m_PAGE_SHIFT;
	//assert(p_t_vci.cmdval.read()); // a non ack request should not disapear
	DRAMCOUT(1) << name() << " [RAM_TLB_MISS] " << endl;
	// Updating the directory for this TLB entry :
	r_TLB_DIRECTORY[page_index] . Set_p(srcid);
	data  = m_PAGE_TABLE[seg] -> translate(addr);
	return true;
}

tmpl(bool)::on_tlb_ack(size_t seg, vci_addr_t full_addr)
{
	//if (r_RAM_FSM != RAM_IDLE) return false; // NAck!

	unsigned int srcid = m_cct->translate_to_id(m_vci_fsm.currentSourceId());

	DRAMCOUT(3) << name() << " (> RAM_TLB_ACK for  " << std::dec << srcid <<  endl;
	if (full_addr != m_PAGE_TO_INVALIDATE)
	{
		cout << "error in " << name() << " address is 0x" << hex << full_addr << " and m_PAGE_TO_INVALIDATE 0x" << m_PAGE_TO_INVALIDATE << endl;
	}
	assert(full_addr == m_PAGE_TO_INVALIDATE);
	// An ACK for the TLB invalidation was received, when all the ack will be received, 
	// we will go to RAM_TLB_INV_OK state
	assert(r_SAVE_TLB_DIRECTORY . Is_p(srcid));
	r_SAVE_TLB_DIRECTORY . Reset_p(srcid);
	return true;
 }
	//////////////////////////////////////////////
	//    Transition method
	//////////////////////////////////////////////
	tmpl(void)::transition()
	{
		m_segnum_5 	 = 0;
		m_node_id_5  = 0;
		m_page_sel_5 = 0;
		m_cost_5 	 = 0;
		m_enable_5	 = false;
		m_dir_accessed_idle_state = false;

		Module_7::m7_cmd_t cmd_7        = Module_7::CMD7_NOP;
		Module_7::m7_req_t req_7        = false;

		m_nbcycles++;


		if (!p_resetn.read()) {
			directory_init();
			m_vci_fsm.reset();
			r_RAM_FSM = RAM_IDLE;
			r_INV_FSM = RAM_INV_IDLE;
			m_BLOCKSTATE.init(m_NB_PROCS);
			m_INV_BLOCKSTATE.init(m_NB_PROCS);
			r_SAVE_TLB_DIRECTORY.init(m_NB_PROCS);
			{
				for (unsigned int i = 0; i <  m_NB_SEG; i++){
					m_PAGE_TABLE[i]->Reset();
					for (unsigned int j = 0; j < (m_SIZE[i]/PAGE_SIZE); j++){
						r_POISONNED[i][j] = false;
					}
				} 

			}
			reload();
			m_5->reset();
			m_6->reset();
			r_POISON_REQ = false;
			r_UNPOISON_REQ = false;
			r_WAITING_TLB_INV_ACK = false;
			CWOUT << name() << "warning, r_POISON_REQ and r_UNPOINSONREQ should be sc_signals" << endl;
			r_IN_TRANSACTION = false;
		} else {
			
			/////////////////////////////////////
			//         r_INV_FSM
			/////////////////////////////////////
			switch (r_INV_FSM.read()) {
				case RAM_INV_IDLE :
					{
						int targetid = -1;
						if (((ram_fsm_state_e)r_RAM_FSM.read()== RAM_INVAL)){ // synchronize on r_RAM_FSM
						m_INV_BLOCKSTATE = m_BLOCKSTATE; // We save and work on INV_BLOCKSTATE beacause since the ram_fsm is free, it can overwrite BLOCKSTATE
							for (unsigned int i = 0; (i < m_NB_PROCS) ; i++) {
								if (m_INV_BLOCKSTATE.Is_p(i)) {
									targetid = i;
									break;
								}
							}
							assert(targetid>=0); // blockstate not empty
							m_INV_BLOCKSTATE.Reset_p(targetid);
							DRAMCOUT(2) << endl << name() << "   req : sending inv to cache" << dec << targetid << endl;
							DRAMCOUT(2) << endl << name() <<  "  invalidating 0x" << hex << (r_INV_BLOCKADDRESS << m_PAGE_SHIFT) << endl;
							r_INV_TARGETADDRESS = m_MapTab_inv.getSegment(m_cct->translate_to_target(targetid)).baseAddress();
							r_INV_FSM = RAM_INV_REQ;
							r_INV_ADDR_OFFSET = 0;
						} else if(((ram_fsm_state_e)r_RAM_FSM.read() == RAM_TLB_DIR_UPDT) && (!m_BLOCKSTATE.Is_empty())){
							m_INV_BLOCKSTATE = m_BLOCKSTATE;
							for (unsigned int i = 0; (i < m_NB_PROCS) ; i++) {
								if (m_INV_BLOCKSTATE.Is_p(i)) {
									targetid = i;
									break;
								}
							}
							assert(targetid>=0); // blockstate not empty
							m_INV_BLOCKSTATE.Reset_p(targetid);

							r_INV_TARGETADDRESS = m_MapTab_inv.getSegment(m_cct->translate_to_target(targetid)).baseAddress() + 4;
							DRAMCOUT(2) << endl << name() << "   req : sending TLB inv to cache" << dec << targetid << endl;
							DRAMCOUT(2) << endl << name() <<  "  invalidating 0x" << hex << (r_INV_BLOCKADDRESS << m_PAGE_SHIFT) << endl;
							r_INV_FSM = RAM_INV_REQ;
							r_INV_ADDR_OFFSET = 4;
						}
						// If the blockstate was empty, no invalidation was sent!
						break;
					}

				case RAM_INV_REQ :
					if (p_i_vci.cmdack.read()) {
						r_INV_FSM = RAM_INV_RSP;
						DRAMCOUT(1) << endl << name() <<  "  going to inv_rsp" << endl;
					
					}
					break;

				case RAM_INV_RSP :
					{
						int targetid = -1;
						if ((p_i_vci.rspval.read()) && (!m_INV_BLOCKSTATE.Is_empty())){
							for (unsigned int i = 0; (i < m_NB_PROCS) ; i++) {
								if (m_INV_BLOCKSTATE.Is_p(i)) {
									targetid = i;
									break;
								}
							}
							DRAMCOUT(2) << endl << name() <<  "    rsp : sending TLB inv to cache" << dec << targetid << endl;
							DRAMCOUT(2) << endl << name() <<  "    invalidating " << hex << (r_INV_BLOCKADDRESS << m_PAGE_SHIFT) << endl;
							assert(targetid>=0); 
							m_INV_BLOCKSTATE.Reset_p(targetid);
							r_INV_TARGETADDRESS = m_MapTab_inv.getSegment(m_cct->translate_to_target(targetid)).baseAddress() + r_INV_ADDR_OFFSET;
							r_INV_FSM = RAM_INV_REQ;
						} else if ((p_i_vci.rspval.read()) && (m_INV_BLOCKSTATE.Is_empty())) {
							r_INV_FSM = RAM_INV_IDLE;
							DRAMCOUT(2) << endl << name() <<  "    rsp : response received, going to idle" << dec << targetid << endl;
						}
						break;
					}
				default :
					assert(false);
					break;

			} // end switch r_INV_FSM

			/////////////////////////////////////
			//         r_RAM_FSM
			/////////////////////////////////////
			switch (r_RAM_FSM.read()) {

				case RAM_IDLE :
					{
						m_vci_fsm.transition();
						DRAMCOUT(0) << name() << " [RAM_IDLE] " << endl;
						// module_5 -> module_9
						#if 0
						bool register_access = false; // to identify the access is to the counters or to the memory
						unsigned int i = 0;
						unsigned int page_index = 0; 
#endif

						if (!r_IN_TRANSACTION)
						{
							if (r_POISON_REQ){
								m_vci_fsm.break_link(r_PAGE_TO_POISON);
								r_RAM_FSM = RAM_POISON;
								r_POISON_REQ = false; // we have taken into account this request
								r_VIRT_POISON_PAGE = m_PAGE_TABLE[0] -> wiam(r_PAGE_TO_POISON << m_PAGE_SHIFT); 
								DRAMCOUT(3) << name() << hex << " <-- POISON_REQ " << std::endl;
								DRAMCOUT(3) << name() << hex << "r_VIRT_POISON_PAGE (wiam) <- 0x" << m_PAGE_TABLE[0] -> wiam(r_PAGE_TO_POISON << m_PAGE_SHIFT) << endl;
								DRAMCOUT(1) << name() << "r_PAGE_TO_POISON <- 0x" <<  r_PAGE_TO_POISON << endl;
								break;
							}
							if (r_WAITING_TLB_INV_ACK){
								if (r_SAVE_TLB_DIRECTORY.Is_empty())
								{
									r_WAITING_TLB_INV_ACK = false;
									DRAMCOUT(3) << name() << hex << " > ALL TLB ACK has been received" << endl;
									r_RAM_FSM = RAM_TLB_INV_OK;
									break;
								}
								else
								{
									DRAMCOUT(3) << name() << hex << " > Waiting for TLB_ACK!" << endl;
								}
							}
							if (r_UNPOISON_REQ){
								r_UNPOISON_REQ = false;
								DRAMCOUT(3) << name() << hex << " <-- UNPOISON REQ" << std::endl;
								r_RAM_FSM = RAM_UNPOISON;
								break;
							}
							if (m_5->p_out->updt_tlb_req){
								CWOUT << name() << "check for no-deadlock guaranty on updt_tlb_req" << endl;
									DRAMCOUT(3) << name() << hex << " <-- UPDT_TLB_REQ" << endl;
								r_RAM_FSM = RAM_TLB_INV;
								break;
							}
							if (m_5->p_out->updt_wiam_req){
								CWOUT << name() << "check for no-deadlock guaranty on updt_tlb_req" << endl;
								r_HOME_PAGE = m_5->p_out->home_entry;
								r_NEW_PAGE_ADDR = m_5->p_out->new_entry;
								r_RAM_FSM = RAM_WIAM_UPDT;
								break;
							}
						}
#if 0
						else
						{
							if (r_POISON_REQ){
								DRAMCOUT(3) << name() << hex << "Not processing POISON REQ" << std::endl;
								break;
							}
							if (r_WAITING_TLB_INV_ACK){
									DRAMCOUT(3) << name() << hex << "Not processing waiting tlb inv ack" << endl;
							}
							if (r_UNPOISON_REQ){
								DRAMCOUT(3) << name() << hex << "Not processing UNPOISON REQ" << std::endl;
								break;
							}
							if (m_5->p_out->updt_tlb_req){
									DRAMCOUT(3) << name() << hex << "Not processing updt_tlb_req" << endl;
								break;
							}
							if (m_5->p_out->updt_wiam_req){
									DRAMCOUT(3) << name() << hex << "Not processing wiam req" << endl;
								break;
							}

						}
#endif
						break;
					}

				case RAM_WIAM_UPDT : // Update the Who_I_AM entry of the page table, and go to idle
					{
						assert(m_5->p_out->updt_wiam_req);
						DRAMCOUT(3) << name() << " [RAM_WIAM_UPDT] " << endl;
						DRAMCOUT(2) << name() << " WIAM : r_HOME_PAGE <- 0x" << hex << r_HOME_PAGE << " - 0x" << r_NEW_PAGE_ADDR <<  endl;
						m_PAGE_TABLE[0] -> update_wiam ( r_HOME_PAGE  , (r_NEW_PAGE_ADDR >> m_PAGE_SHIFT));
						m_6->set_wiam_ok();
						r_RAM_FSM = RAM_IDLE;
						break;
					}

				case RAM_TLB_INV :
					{
						assert(m_5->p_out->updt_tlb_req);
						DRAMCOUT(3) << name() << " [RAM_TLB_INV] " << endl;
						r_HOME_PAGE = m_5->p_out->home_entry;
						r_NEW_PAGE_ADDR = m_5->p_out->new_entry;
#if ONE_SEG_MODE
						if (!((  ((m_5->p_out->home_entry - m_BASE[0]) >> m_PAGE_SHIFT) < (m_SIZE[0]/PAGE_SIZE))))
						{
							std::cerr << name() << " [error] , accessing a page index out of range " << std::endl;
							std::cerr << name() << "           home_entry : " << std::hex << m_5->p_out->home_entry  << std::endl;
							std::cerr << name() << "           base       : " << std::hex << m_BASE[0]  << std::endl;
							std::cerr << name() << "           index      : " << std::dec << ((m_5->p_out->home_entry - m_BASE[0]) >> m_PAGE_SHIFT) << std::endl;
							std::cerr << name() << "           toal index : " << std::dec << (m_SIZE[0]/PAGE_SIZE) << std::endl;
						}
						assert(  ((m_5->p_out->home_entry - m_BASE[0]) >> m_PAGE_SHIFT) < (m_SIZE[0]/PAGE_SIZE));
						m_BLOCKSTATE = r_TLB_DIRECTORY[ (m_5->p_out->home_entry - m_BASE[0]) >> m_PAGE_SHIFT];//todo directory for this TLB entry
						r_SAVE_TLB_DIRECTORY = r_TLB_DIRECTORY[ (m_5->p_out->home_entry - m_BASE[0]) >> m_PAGE_SHIFT];//todo directory for this TLB entry
						r_WAITING_TLB_INV_ACK = true;
						m_PAGE_TO_INVALIDATE = m_5->p_out->home_entry; // debug only
						r_INV_BLOCKADDRESS = (m_5->p_out->home_entry >> m_PAGE_SHIFT); 
						DRAMCOUT(3) << name() << " page to invalidate " << hex << m_PAGE_TO_INVALIDATE  << endl;
						DRAMCOUT(3) << name() << " Processors to invalidate TLB :  ";
						m_BLOCKSTATE.Print();
#else
						assert(false);
#endif
						r_RAM_FSM = RAM_TLB_DIR_UPDT;
						break;
					}

				case  RAM_TLB_DIR_UPDT :
					{

						//bool new_page_is_local = ((r_NEW_PAGE_ADDR >> m_PAGE_SHIFT) == (m_BASE[0] >> m_PAGE_SHIFT));
						bool new_page_is_local = false;
						DRAMCOUT(3) << name() << " [RAM_TLB_DIR_UPDT] " << endl;
#if ONE_SEG_MODE
						// Reseting all the presence flag for this TLB entry
						r_TLB_DIRECTORY[(r_HOME_PAGE - m_BASE[0]) >> m_PAGE_SHIFT].Reset_all();
						// Updating the TLB entry
#ifdef USE_STATS
						//file_stats << std::dec << m_nbcycles << " " <<  std::hex << r_HOME_PAGE  << endl;
#endif
						m_PAGE_TABLE[0] -> update_virt ( ((r_HOME_PAGE - m_BASE[0]) >> m_PAGE_SHIFT) ,
								(r_NEW_PAGE_ADDR >> m_PAGE_SHIFT),
								new_page_is_local);
#else
						assert(false);
#endif
						// We send the tag only
						DRAMCOUT(3) << name() << " TLB_DIR_UPDT : r_HOME_PAGE <- 0x" << hex << r_HOME_PAGE << " - 0x" << r_NEW_PAGE_ADDR <<  endl;

						r_RAM_FSM = RAM_IDLE;
						// @@inv_tlb : go to idle, in idle, receive TLB_INV_ACK for the saved TLB_DIRECTORY (TLB_ENTRY_INDEX for assert verification)
						// when TLB_DIRECTORY is empty, set the tlb_inv_ok!
						break;
					}
				break;

				case RAM_TLB_INV_OK :
					DRAMCOUT(3) << name() << " [RAM_TLB_INV_OK] " << endl;
					r_RAM_FSM = RAM_IDLE;
					m_6->set_tlb_inv_ok();
					break;


				case RAM_POISON :
				////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// To go to this state, we can wait for the RAM_FSM to be in idle state and check for a poison command. This
				// will not add any deadlock beacause the RAM_FSM will alway go to idle, one day or another.
				// What may prevent this : 
				// 	- response fifo full : since a response is alway taken into account by the initiator, it will never
				// 	  be full forever
				// 	- wait for invalidation responses : invalidations are prehemptive on cache, so they are always taken
				// 	  into account.
				////////////////////////////////////////////////////////////////////////////////////////////////////////////

				// [0] because module 10 has no mean to know witch segment to access. Now, there is only one segment per ram
				// memory module. A way to change this, is to have unique global counters and one counter per page per segment.
				//
					DRAMCOUT(3) << name() << " [RAM_POISON] " << endl;
					r_POISONNED[0][r_PAGE_TO_POISON] = true;
					CWOUT << "WARNING : r_PAGE_TO_POISON should be saved " << name() <<  endl;
#ifdef ONE_SEG_MODE
					assert(r_PAGE_TO_POISON < (m_SIZE[0]/PAGE_SIZE));
				
#else
						assert(false); // accessing page_table 0 (make the assumption that there is only one segment)
#endif
					r_RAM_FSM = RAM_IDLE;
				break;

				case RAM_UNPOISON :
					DRAMCOUT(3)<< name() << " [RAM_UNPOISON] " << endl;
					CWOUT << "WARNING : an opitmization can be done in module_10, there is no possible request between" << endl;
					CWOUT << "          unpoison and unposon_ok" << endl;
					r_RAM_FSM = RAM_IDLE;
					assert(r_POISONNED[0][r_PAGE_TO_POISON]);
#ifdef ONE_SEG_MODE
					r_POISONNED[0][r_PAGE_TO_POISON] = false;
#else
	#error "one segment mode"
#endif
				break;


					
#if 0
				case RAM_REGISTERS_REQ :
					{
						addr_t		address	= p_t_vci.address.read(); 
						unsigned int segnum = 0 ; // todo, simplify segnum search
						std::list<soclib::common::Segment>::iterator iter = m_SEGLIST.begin();
						DRAMCOUT(0) <<  name() << " [RAM_REGISTERS_REQ] " << endl;
						if(p_t_vci.cmdval.read()){
							// get the segment and block number
							while (!iter->contains(address)){
								iter++;
								segnum++;
							}
							assert(segnum >= m_NB_SEG); // second half of registers

							// Lock the register bank.
							cmd_7 = Module_7::CMD7_LOCK;	
							segnum_5 =  segnum - m_NB_SEG;
							req_7 = true;
							if (m_5->p_in->m7_rsp){ // Lock was acknownledged by module 10
								r_RAM_FSM = RAM_REGISTERS_RSP;
							} 
							m_waiting=0;
						}else{
						m_waiting++;
							if (m_waiting==1000) assert(false);
						}

						break;
					}
				
				case RAM_REGISTERS_RSP :    // and directory access
					{
						addr_t		address	= p_t_vci.address.read(); 
						eop_t		eop	= p_t_vci.eop.read();
						uint32_t	rsrcid  = p_t_vci.srcid.read();
						uint32_t srcid = m_cxct->translate_to_id(p_t_vci.srcid.read()); // For Directory check
						cmd_t	cmd     = p_t_vci.cmd.read();
						unsigned int    rdata; // undefined values

						unsigned int segnum = 0 ; // todo, simplify segnum search
						std::list<soclib::common::Segment>::iterator iter = m_SEGLIST.begin();
						DRAMCOUT(0) <<  name() << " [RAM_REGISTERS_RSP] " << endl;
						if(p_t_vci.cmdval.read()){

							// get the segment and block number
							while (!iter->contains(address)){
								iter++;
								segnum++;
							}
							assert(segnum >= m_NB_SEG); // second half of registers

							// To access the banc registers, we do some simple 1 cycle access as if it was a
							// simple register banc. If this is not possible, we should add a new state
							switch (cmd) {
								case vci_param::CMD_WRITE :
									//rdata = m_5->reset_reg(address  - iter->baseAddress(),segnum - m_NB_SEG);
									assert(false);
									break;
								case vci_param::CMD_READ :
									// To access in a combinational form a data, we need to implement function calls.
									// Hence, the response is here in the critical path, and not in the next cycle.
									rdata = m_5->read_reg(address  - iter->baseAddress(),segnum - m_NB_SEG);
									break;
								default :
									assert(false); // Unsupported command
									break;
							}

							if (m_FIFO_RDATA.wok()) {

								rsp_fifo_put = true;	// send a response in the response fifo

								fifo_rdata  = rdata; 
								fifo_rerror = 0;
								fifo_reop   = eop;
								fifo_rsrcid = rsrcid;
								fifo_rtrdid = p_t_vci.trdid.read();
								fifo_rpktid = p_t_vci.pktid.read();
								// Burst of read or write on those are allowed. Moreover they are encouraged
								// because they are mapped lineary in memory.
								if (eop)
									r_RAM_FSM = RAM_REGISTERS_END;
							}
							m_waiting=0;
						}else{
						m_waiting++;
							if (m_waiting==1000) assert(false);
						}

						break;
					}
				case RAM_REGISTERS_END :
					// We send a request to unlock the register bank
					req_7 = true;
					cmd_7 = Module_7::CMD7_UNLOCK;
					DRAMCOUT(0) << name() << " [RAM_REGISTERS_END] " << endl;
					if (m_5->p_in->m7_rsp){ // Lock was acknownledged by module 10
						DRAMCOUT(0) <<  name() << " [RAM_REGISTERS_END] " << endl;
						r_RAM_FSM = RAM_IDLE;
					} 
				
				break;
#endif

				case RAM_DIRUPD :
					{
						unsigned int blocknum;

						// directory update:
						DRAMCOUT(0) << name() << " [RAM_DIRUPDT] " << endl;
						blocknum = (((r_SAV_ADDRESS.read() - m_BASE[r_SAV_SEGNUM.read()]) & m_BLOCKMASK) >> m_ADDR_BLOCK_SHIFT);
						r_DIRECTORY[r_SAV_SEGNUM.read()][blocknum].Set_p(m_cct->translate_to_id(r_SAV_SCRID.read()));
							r_RAM_FSM = RAM_IDLE;
						break;
					}

				case RAM_INVAL : // and directory update
					{
						unsigned int blocknum = (((r_SAV_ADDRESS.read() - m_BASE[r_SAV_SEGNUM.read()]) & m_BLOCKMASK) >> m_ADDR_BLOCK_SHIFT);
						bool save_p = r_DIRECTORY[r_SAV_SEGNUM.read()][blocknum].Is_p(m_cct->translate_to_id(r_SAV_SCRID.read()));
						addr_t original_addr = r_SAV_ADDRESS.read();
						addr_t wiam_addr = 0;
						if((inv_fsm_state_e)r_INV_FSM.read()==RAM_INV_IDLE){ // We stay heere until the INV_FSM is not free. This will not generate a deadlock
							// because none of the request sent by this fsm requires that the CACHE fsm waits for a memory node. This is the case
							// because cache invalidations are preemptive and tlb invalidation can respond immediately.
							DRAMCOUT(1) << name() << " [RAM_INVAL] " << endl;
							r_DIRECTORY[r_SAV_SEGNUM.read()][blocknum].Reset_all();
							if (save_p){
								r_DIRECTORY[r_SAV_SEGNUM.read()][blocknum].Set_p(m_cct->translate_to_id(r_SAV_SCRID.read())); 
							}
#if 0
							r_INV_BLOCKADDRESS = (r_SAV_ADDRESS.read() & m_BLOCKMASK); 
#else
#if ONE_SEG_MODE
							wiam_addr = m_PAGE_TABLE[0] -> wiam(original_addr - m_BASE[0]); // Invalidations are for the blocs that are actually here, ie wiam and not
							// virtual translation.
							DRAMCOUT(1) << name() << hex << " wiam address for invalidation : 0x" << original_addr << " to 0x" << wiam_addr  << endl;
							assert((wiam_addr & (PAGE_SIZE -1)) == 0);
							r_INV_BLOCKADDRESS = (wiam_addr | (r_SAV_ADDRESS.read() & (PAGE_SIZE - 1) & m_BLOCKMASK)); 
#else
							assert(false);
#endif
#endif
							r_RAM_FSM = RAM_INVAL_WAIT;
							// (keep initiator bit set, if previously set)
						}
						break;
					}

				case RAM_INVAL_WAIT :
					DRAMCOUT(0) << name() << " [RAM_INVAL_WAIT] " << endl;
					if ((inv_fsm_state_e)r_INV_FSM.read()==RAM_INV_IDLE){ //stay in this state waiting for completion of the invalidation
						DRAMCOUT(1) << name() << hex << " invalidation complete at: 0x" <<  r_INV_BLOCKADDRESS.read() << endl;
							r_RAM_FSM = RAM_IDLE; 
					}
					break;

				case RAM_ERROR :
					assert(false);
						r_RAM_FSM = RAM_IDLE;
					break;

				default : 
					assert(false); // unknown state
					break;

			} // end switch r_RAM_FSM

			/////////////////////////////////////////////////
			//	module 5                               //
			/////////////////////////////////////////////////


			#if 0
			m_5->p_in->idle = ((ram_fsm_state_e)r_RAM_FSM.read()== RAM_IDLE || (ram_fsm_state_e)r_RAM_FSM.read() == RAM_TLB_MISS); // todo
											// Necessary to avoid migrating pages that does not
											// generate contention (contention being TLB accesses)
			#endif
			m_5->p_in->node_id  = m_node_id_5;
			m_5->p_in->page_sel = m_page_sel_5;
			m_5->p_in->cost     = m_cost_5;
			m_5->p_in->segnum   = m_segnum_5;
			m_5->p_in->enable   = m_enable_5;

			m_5->p_in->m7_cmd   = cmd_7; // these signals reflects the actual state, so they must be set here.
			m_5->p_in->m7_req   = req_7;
		
			m_5->compute();
			if (m_5->p_out->poison_req)
			{
					r_POISON_REQ = true; // set in the gen_outputs of module 10, available in this
									 // transition step, latched, and available for testing with
									 // one cycle latency
					r_PAGE_TO_POISON = m_5->p_out->poison_page;
			}
			if (m_5->p_out->unpoison_req) r_UNPOISON_REQ = true;

			/////////////////////////////////////////////////
			//	module 6                               //
			/////////////////////////////////////////////////
			

			//// RING IN

			m_m6_in.id   = p_ring_in.id.read();
			m_m6_in.cmd  = p_ring_in.cmd.read();
			m_m6_in.data = p_ring_in.data.read();
			m_m6_in.srcid = p_ring_in.srcid.read();

			//// RING OUT

			m_m6_out.ack = p_ring_out.ack.read();


			m_6->compute();


			//assert(!(cost_5 ^ enable_5));



		} // end RESETN

	}; // end transition()

	/////////////////////////////////////////////////////
	//    genMoore() method
	/////////////////////////////////////////////////////
	tmpl(void)::genMoore()
	{
		// call gen_outputs of different components
		m_5->gen_outputs();
		m_6->gen_outputs();
		// POISON

		//r_PAGE_TO_POISON = m_5->p_out->poison_page;

		// RING OUT
		
		p_ring_out.id  = m_m6_out.id;
		p_ring_out.cmd  = m_m6_out.cmd;
		p_ring_out.data = m_m6_out.data;
		p_ring_out.srcid = m_m6_out.srcid;

		// RING IN

		p_ring_in.ack = m_m6_in.ack;
		// default input for m_5
		m_5->p_in->poison_ack = false;
		m_5->p_in->unpoison_ok = false;
		m_5->p_in->virt_poison_page = r_VIRT_POISON_PAGE;
		m_5->p_in->updt_tlb_ack = false;
		m_5->p_in->updt_wiam_ack = false;
		/////////////////////////////////////////////////////
		// VCI TARGET CMDACK signal
		/////////////////////////////////////////////////////

		switch (r_RAM_FSM.read()) {

			case RAM_WIAM_UPDT :
				m_5->p_in->updt_wiam_ack = true;
				p_t_vci.cmdack = false;
				p_t_vci.rspval = false;
				break;

			case RAM_TLB_INV :
				m_5->p_in->updt_tlb_ack = true;
				// We stay in this state for many cycles, but it is not important because
				// module_10 will not care about this signal for more than one cycle
				p_t_vci.cmdack = false;
				p_t_vci.rspval = false;
				break;

			case RAM_POISON :
				m_5->p_in->poison_ack = true;
				p_t_vci.cmdack = false;
				p_t_vci.rspval = false;
				break;

			case RAM_UNPOISON :
				m_5->p_in->unpoison_ok = true;
				p_t_vci.cmdack = false;
				p_t_vci.rspval = false;
				break;

			case RAM_IDLE :
				m_vci_fsm . genMoore();

				break;

#if 0
			case RAM_REGISTERS_REQ :
			case RAM_REGISTERS_END :
#endif
			case RAM_INVAL :
			case RAM_INVAL_WAIT :
			case RAM_TLB_INV_OK :
			case RAM_TLB_DIR_UPDT :
			case RAM_DIRUPD :
				p_t_vci.cmdack = false;
				p_t_vci.rspval = false;
				break;

#if 0
			case RAM_TLB_MISS :
			case RAM_TLB_ACK :
			case RAM_REGISTERS_RSP :

				p_t_vci.cmdack = m_FIFO_RDATA.wok();
				break;
#endif

			default :
				assert(false); // unknown state
				break;

		} // end switch r_RAM_FSM

		/////////////////////////////////////////////////////
		// VCI TARGET response signals 
		/////////////////////////////////////////////////////

		/////////////////////////////////////////////////////
		// VCI INITIATOR signals 
		/////////////////////////////////////////////////////

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
				p_i_vci.cmdval  = true;
				p_i_vci.rspack  = false;
				p_i_vci.address = (sc_uint<32>) r_INV_TARGETADDRESS.read();
				p_i_vci.wdata   = (sc_uint<32>) r_INV_BLOCKADDRESS.read();
				p_i_vci.be      = 0xF;
				p_i_vci.plen    = 1;
			//	if((ram_fsm_state_e)r_RAM_FSM.read() == RAM_TLB_INV_WAIT){
			//		p_i_vci.cmd = vci_param::CMD_INV_TLB;
			//	}else
				p_i_vci.cmd = vci_param::CMD_WRITE;
				//else{
				//	cerr << name() <<  " Error, RAM_FSM should be in TLB_INV_WAIT or INVAL_WAIT but it is in : " << hex <<  r_RAM_FSM.read() << endl; 
				//	assert(false);
				//}
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

	}; // end genMoore()

tmpl(void)::printramlines(struct modelResource *d)
{
        #define Z(i) ((char)(tab[line*m_BLOCK_SIZE + li]>>i)>=' '&&(char)(tab[line*m_BLOCK_SIZE + li]>>i)<='~'?(char)(tab[line*m_BLOCK_SIZE + li]>>i):'.')
        unsigned int line;
        unsigned int li;
	unsigned int start      = d->start;
        unsigned int len        = d->len;
        unsigned int segnum = d->usr2;
        unsigned int *tab;
        unsigned int nb_max_lines;
        
        tab = r_RAM[segnum];
        // Test on start and len parameters, adjust them if necessary to avoid out-of-bound accesses.
        nb_max_lines = ((m_SIZE[segnum] >> 2)/m_BLOCK_SIZE);
        if (start > nb_max_lines) start = 0;
        if (start + len > nb_max_lines) len = nb_max_lines - start;
        
        // Display each line in start -> start + len.
        putc('\n',stdout); //todo
		assert(segnum == 0);
        for (line =  start ; line < start + len; line++) {
                printf( "0x%08x TAG=(",(((line*m_BLOCK_SIZE) << 2) + m_BASE[segnum]));
                (r_DIRECTORY[segnum][line]).Print();
                printf(" %08x" ,((r_DIRECTORY[segnum][line]).Get_slice(0)));
                printf(") %c ", ((!(r_DIRECTORY[segnum][line]).Is_empty()) ? 'S' : '-'));
                for (li = 0; li < m_BLOCK_SIZE; li++)
                        printf("%08x ", tab[line*m_BLOCK_SIZE + li]);
                for (li = 0; li < m_BLOCK_SIZE; li++)
                        printf( " %c%c%c%c", Z(0), Z(8), Z(16), Z(24));
                puts("\n");
        }
#undef Z
}
tmpl(const char*)::GetModel()
{
    return("Write-through Write-invalidate Multisegment S-RAM memory");
}
//
tmpl(void)::printramdata(struct modelResource *d)
{
        #define W tab[word]
        #define Z(i) ((char)(W>>i)>=' '&&(char)(W>>i)<='~'?(char)(W>>i):'.')
        unsigned int address  = d->start;
        unsigned int segnum = d->usr2;
        unsigned int *tab;
        unsigned int word;
        
        
        tab = r_RAM[segnum];
        if ((address < m_BASE[segnum]) || (address > (m_BASE[segnum] + m_SIZE[segnum]))){
                word = 0;
                printf("address out of range");
        } else word = (((address & ~m_LSBMASK) - m_BASE[segnum]) >> 2);


        printf("line:%d %08x ",word/m_BLOCK_SIZE ,W);
        printf("%c%c%c%c\n", Z(0), Z(8), Z(16), Z(24));
        #undef W
        #undef Z
}

tmpl(int)::PrintResource(modelResource *res,char **p)
{
	std::list<soclib::common::Segment>::iterator     iter;
	int i = 1;
	int index = -1;
	char * misc;


	assert((p!=NULL)||(res->usr1!=0)); // Assert that it is a first call to PrintRessource (p!= NULL)  
	/* Registers : I do not try to optimize this (as it used to be for
	 * the specific and software named registers) since it is not 
	 * anymore in the critical loop */
	if (!p){
		switch (res->usr1){
			case 1:
				printramlines(res);
				return 0;
				break;
			case 2:
				printramdata(res);
				return 0;
				break;
			default:
				std::cerr << "CACHE: No such ressource" << std::endl; //todo, CACHE should be name
				break;
		}
	}else if(*p[0]>0){
		index = -1;
		for (iter = m_SEGLIST.begin() ; iter != m_SEGLIST.end() ; ++iter) {
			index++; 
			if (strcmp(iter->name().c_str(),p[1]) == 0) {
				break;
			}
		}
		if (index != -1){
			i++;
			if (*p[i] == (char)0) {
				res->start = 	0;
				res->len =  (iter->size() >> 2) / m_BLOCK_SIZE;
				res->usr1 = 1;
			} else if (*p[i + 1] == (char)0) {
				res->start = 	strtoul(p[i],&misc,0);
				res->usr1 = 2;
			} else {
				res->start = 	strtoul(p[i],&misc,0);
				res->len = 	strtoul(p[i + 1],&misc,0);
				res->usr1 = 1;
			}
			res->obj = this;
			printf("Registering this = %x\n",(unsigned int )this);
			res->usr2 = index;
			return 0;
		} else
			fprintf(stderr, "%s: No such segment: %s\n",name().c_str(),p[i]);
	}
	return -1;
}

tmpl(int)::TestResource(modelResource *res,char **p)
{
int i = 1;
int index;
unsigned int address;
char * misc;
std::list<soclib::common::Segment>::iterator     iter;
	/* Shall we make a specific error function for all
	* print/display/test functions, so to be homogeneous */
	
	if (*p[i] == (char)0) {
		fprintf(stderr, "needs a segment name argument!\n");
		return -1;
	}
	index = -1 ;
	for (iter = m_SEGLIST.begin() ; iter != m_SEGLIST.end() ; ++iter) {
		index++;
		if (strcmp(iter->name().c_str(),p[1]) == 0) {
			break;
		}
	}
	if (index != -1) {
		i++;
		if (*p[i + 1] == (char)0) {
			address = strtoul(p[i],&misc,0);
			res->addr =    (int*)&r_RAM[index][((address & ~m_LSBMASK) - m_BASE[index]) >> 2];
			return 0;
		}else
			fprintf(stderr, "Error, expected: <segment> <addr>\n");
	}else
		fprintf(stderr, "%s: No such segment: %s\n",name().c_str(),p[i]);   
	return -1;
}

tmpl(int)::Resource(char **args)
{
	size_t  i = 1;
	std::list<soclib::common::Segment>::iterator     iter;

	static const char *RAMessources[] = {
		"p\t[lin]\t: Dumps whole RAM",
		"p\t[str]\t [n]\t: Prints ram value at address n",
		"p\t[int]\t [n1] [n2]\t: Dumps n2 ram lines from line n1",
	};

	if (*args[i] != (char)0) {
		fprintf(stderr, "Ressources RAM :  junk at end of command!\n");
		return 1;
	}
	for (i = 0; i < sizeof(RAMessources)/sizeof(*RAMessources); i++)
		fprintf(stdout,"%s\n",RAMessources[i]);
	cout << "available segments : ";	
	for (iter = m_SEGLIST.begin() ; iter != m_SEGLIST.end() ; ++iter) {
		cout << iter->name() <<  " ";	
	}
	cout << std::endl;
	return 0;
}

	tmpl(bool)::is_access_dir_possible(void){
#if 0
		return( !((((ram_fsm_state_e)r_RAM_FSM.read()== RAM_WRITE) || ((ram_fsm_state_e)r_RAM_FSM.read()== RAM_READ)) && m_FIFO_RDATA.wok())
				&&
				!(((ram_fsm_state_e)r_RAM_FSM.read()== RAM_DIRUPD) || ((ram_fsm_state_e)r_RAM_FSM.read()== RAM_INVAL)));
		// DIR is accessed on READ or WRITE state when the
		// output fifo is not full
		// and in RAM_INVAL or RAM_DIRUPD
#else
		return( !(m_dir_accessed_idle_state)
				&&
				!(((ram_fsm_state_e)r_RAM_FSM.read()== RAM_DIRUPD) || ((ram_fsm_state_e)r_RAM_FSM.read()== RAM_INVAL)));
	// DIR is accessed on IDLE_state when there is a request, and in RAM_INVAL or RAM_DIRUPD state
	// use the fact that on_read/on_write function are executed BEFORE the transition of different modules_x
	//
#endif
	}

tmpl(SOCLIB_DIRECTORY *)::read_dir(unsigned int page_index,unsigned int line_page_offset){
#if ONE_SEG_MODE
	DRAMCOUT(0) << name()  << dec <<  "DIRECTORY : page " << page_index << " line " << line_page_offset << hex <<  " 0x" << (((page_index << m_PAGE_SHIFT) + (line_page_offset << m_ADDR_BLOCK_SHIFT)) + m_BASE[0])
		<< " ----> ";
	//r_DIRECTORY[0][(((page_index << m_PAGE_SHIFT) >> m_ADDR_BLOCK_SHIFT) + line_page_offset)].Print();
	DRAMCOUT(0) << endl;

	return  &r_DIRECTORY[0][(((page_index << m_PAGE_SHIFT) >> m_ADDR_BLOCK_SHIFT) + line_page_offset)];
#else
	assert(false);
#endif
}

	tmpl(void)::write_dir(unsigned int page_index,unsigned int line_page_offset, SOCLIB_DIRECTORY * wdata){
#if ONE_SEG_MODE
		DRAMCOUT(0) << name()  << hex <<  "DIRECTORY 0x" << (((page_index << m_PAGE_SHIFT) + (line_page_offset << m_ADDR_BLOCK_SHIFT)) + m_BASE[0])
			  << " <---- ";
			  //wdata.Print();
			  DRAMCOUT(0) << endl;
		r_DIRECTORY[0][(((page_index << m_PAGE_SHIFT) >> m_ADDR_BLOCK_SHIFT) + line_page_offset)] = *wdata;
#else
		assert(false);
#endif
	}
	tmpl(bool)::is_access_sram_possible(void){
		return(! m_dir_accessed_idle_state );
			// SRAM is accessed only on READ or WRITE state when the
			// output fifo is not full
	}
	tmpl(unsigned int)::read_seg(unsigned int page_index,unsigned int word_page_offset){
#if ONE_SEG_MODE
		DRAMCOUT(0) << name()  << hex <<  " 0x" << (((page_index << m_PAGE_SHIFT) + (word_page_offset << 2)) + m_BASE[0])
			  << " ----> "
			  << hex << " 0x" <<  rw_seg(r_RAM[0], (((page_index << m_PAGE_SHIFT) >> m_ADDR_WORD_SHIFT) + word_page_offset), 0, 0, vci_param::CMD_READ)
			  << endl;
		
		return  rw_seg(r_RAM[0], (((page_index << m_PAGE_SHIFT) >> m_ADDR_WORD_SHIFT) + word_page_offset), 0, 0, vci_param::CMD_READ);
#else
		assert(false);
#endif
	}
	tmpl(void)::write_seg(unsigned int page_index,unsigned int word_page_offset, unsigned int wdata){
#if ONE_SEG_MODE
		DRAMCOUT(0) << name() 
			  << hex << " 0x" <<  (((page_index << m_PAGE_SHIFT) + (word_page_offset << 2)) + m_BASE[0])
			  << " <---- "
			  << hex << " 0x" <<  wdata
			  << endl;
		
		rw_seg(r_RAM[0],(((page_index << m_PAGE_SHIFT) >> m_ADDR_WORD_SHIFT) + word_page_offset), wdata, 0xF, vci_param::CMD_WRITE);
		DRAMCOUT(0) << name()  << hex <<  " 0x" << (((page_index << m_PAGE_SHIFT) + (word_page_offset << 2)) + m_BASE[0])
			  << " read conf----> "
			  << hex << " 0x" <<  rw_seg(r_RAM[0], (((page_index << m_PAGE_SHIFT) >> m_ADDR_WORD_SHIFT) + word_page_offset), 0, 0, vci_param::CMD_READ)
			  << endl;
#else
		assert(false);
#endif
	}
}
}
