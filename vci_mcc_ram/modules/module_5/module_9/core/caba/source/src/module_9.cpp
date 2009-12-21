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
#include "module_9.h"
#include "arithmetics.h"
#include "globals.h"
#include "mapping.h"

namespace soclib {
namespace caba {

using namespace soclib;
using namespace std;
using soclib::common::uint32_log2;

Module_9::Module_9(const string  &insname,unsigned int nb_nodes,unsigned int nb_pages,unsigned int base_addr,unsigned int page_size):
	m_name(insname),
	m_BASE_ADDR(base_addr),
	m_PAGE_SHIFT(uint32_log2(page_size)),
	m_NB_PAGES(nb_pages),
	m_NB_NODES(nb_nodes),
	m_COMPUTE_TIME(0),

	m_ADDR_NCOUNT_PSEL_SHIFT(uint32_log2(m_NB_NODES)+uint32_log2(sizeof(counter_t))),
	m_ADDR_NCOUNT_PSEL_MASK((m_NB_PAGES-1) << m_ADDR_NCOUNT_PSEL_SHIFT),
	m_ADDR_NCOUNT_NSEL_SHIFT(uint32_log2(sizeof(counter_t))),
	m_ADDR_NCOUNT_NSEL_MASK((m_NB_NODES-1) << m_ADDR_NCOUNT_NSEL_SHIFT),

	m_ADDR_GCOUNT_PSEL_SHIFT(uint32_log2(sizeof(counter_t))),	
	m_ADDR_GCOUNT_PSEL_MASK((m_NB_PAGES -1) << m_ADDR_GCOUNT_PSEL_SHIFT),

	m_ADDR_TIDP_PSEL_SHIFT(uint32_log2(m_NB_NODES)+uint32_log2(sizeof(counter_id_t))),	
	m_ADDR_TIDP_PSEL_MASK((m_NB_PAGES -1) << m_ADDR_TIDP_PSEL_SHIFT),
	m_ADDR_TIDP_NSEL_SHIFT(uint32_log2(sizeof(counter_id_t))),	
	m_ADDR_TIDP_NSEL_MASK((m_NB_NODES -1) << m_ADDR_TIDP_NSEL_SHIFT),

	m_ADDR_MAXIP_PSEL_SHIFT(uint32_log2(sizeof(counter_id_t))),	
	m_ADDR_MAXIP_PSEL_MASK((m_NB_PAGES -1) << m_ADDR_MAXIP_PSEL_SHIFT),	

	m_ADDR_TP_SHIFT(uint32_log2(sizeof(counter_id_t))),	
	m_ADDR_TP_MASK((TOP_P -1) << m_ADDR_TP_SHIFT)	,

	m_last_evicted(0)
	{
		srand(0);
		// Assertion :
		assert(is_pow2(m_NB_PAGES));
		assert(is_pow2(m_NB_NODES));
		assert(is_pow2(TOP_P));
		assert(is_pow2(m_NB_NODES));
		assert(is_pow2(m_NB_NODES));
		// allocation des tableaux de compteurs
		CTOR_OUT
#ifdef USE_STATS
		stats_chemin = "./";
		stats_chemin += m_name.c_str();
		stats_chemin += ".stats"; 
		file_stats.setf(ios_base::hex);
		file_stats.open(stats_chemin.c_str(),ios::trunc);
		file_stats << "----| STARTING INFORMATION RECORD" << endl;
#endif
		r_counters = new counter_t * [m_NB_PAGES];
		r_global_counters = new counter_t [m_NB_PAGES];
		d_global_counters = new counter_t [m_NB_PAGES];
		r_migrable = new bool [m_NB_PAGES];
		r_max_id_perpage = new unsigned int [m_NB_PAGES]; // todo should be  log2(m_NB_NODES)
		r_top3_id_perpage = new int * [m_NB_PAGES]; 
		r_top3_pages = new int [TOP_P];
		for(unsigned int i = 0; i < m_NB_PAGES ; i++){
			r_counters[i] = new  counter_t  [m_NB_NODES];
			r_top3_id_perpage[i] = new int [m_NB_NODES]; // should be log2(m_NB_NODES)
		}

	}

void Module_9::quickSort(counter_t tableau[], counter_t indices[], counter_t p, counter_t r) {
        counter_t q;
        if(p<r) {
                q = partitionner(tableau, indices, p, r);
                quickSort(tableau, indices, p, q);
                quickSort(tableau, indices, q+1, r);
        }
}
 
counter_t Module_9::partitionner(counter_t tableau[], counter_t indices[], counter_t p, counter_t r) {
        counter_t pivot = tableau[p], i = p-1, j = r+1;
        counter_t temp;
        while(1) {
                do
                        j--;
                while(tableau[j] > pivot);
                do
                        i++;
                while(tableau[i] < pivot);
                if(i<j) {
                        temp = tableau[i];
                        tableau[i] = tableau[j];
                        tableau[j] = temp;

                        temp = indices[i];
                        indices[i] = indices[j];
                        indices[j] = temp;
                }
                else
                        return j;
        }
        return j;
}


magnitud_order Module_9::order(unsigned int old_v, unsigned int new_v, unsigned int bit_order){
	unsigned int order_o = 0;
	unsigned int order_n = 0;
	for (unsigned int i = 0; i < 32 ; i ++){
		if ((old_v<<i) & 0x80000000){
			order_o = (32 - i);
			break;
		}
	}
	for (unsigned int i = 0; i < 32 ; i ++){
		if ((new_v<<i) & 0x80000000){
			order_n = (32 - i);
			break;
		}
	}
	if (new_v > old_v){
		if ((order_n - order_o) > (bit_order)) return G;
		if ((order_n - order_o) <= (bit_order)) return S;
	}else{
		if ((order_o - order_n) > (bit_order)) return L;
		if ((order_o - order_n) <= (bit_order)) return S;
	}
	assert(false);
	// bit_order is the number of bit shift that can be done inside the same order of mangnitude ex, bit_order = 2 implies that 32 and 2
	// are of the same order
}
bool Module_9::busy(){
	// Warning : assert that we call busy only before compute!
	return (m_COMPUTE_TIME != 0);
}
const string & Module_9::name(){
	return this->m_name;
}	


void Module_9::set_not_migrable(unsigned int page_index){
	r_migrable[page_index]=false;
}
void Module_9::set_migrable(unsigned int page_index){
	r_migrable[page_index]=true;
}
counter_t Module_9::read_global_counters(unsigned int page_index){
	assert(page_index < m_NB_PAGES);
	return r_global_counters[page_index];
}
counter_t Module_9::read_counter(unsigned int page_index, unsigned int node_index){
	assert(page_index < m_NB_PAGES);
	assert(node_index < m_NB_NODES);
	return r_counters[page_index][node_index];
}

counter_t Module_9::read_rounded_counter(unsigned int page_index, unsigned int node_index)
{
	assert(page_index < m_NB_PAGES);
	assert(node_index < m_NB_NODES);
	uint32_t value = r_counters[page_index][node_index];
	for (int i = 31 ; i >= 0; i --)
	{
		if (value  & (0x1 << i))
			return i;
	}
	return 0;

}
counter_id_t Module_9::read_max_id_perpage(unsigned int page_index){
	assert(page_index < m_NB_PAGES);
	return r_max_id_perpage[page_index];
}
counter_id_t Module_9::read_max_page(){
	return r_max_page;
}
counter_id_t Module_9::evict_sel(){
#if 0
#if 1
	m_last_evicted ++;
	if (m_last_evicted == m_NB_PAGES)
		m_last_evicted = 0;
	return m_last_evicted;
#else
	return 0;
#endif
#else
	return  (counter_id_t)((double)rand() / ((double)RAND_MAX + 1) * m_NB_PAGES); 
#endif
}
counter_id_t Module_9::read_top_id_perpage(unsigned int page_index,unsigned int top_index){
	assert(page_index < m_NB_PAGES);
	assert(top_index < m_NB_NODES);
	return r_top3_id_perpage[page_index][top_index];
}
counter_id_t Module_9::read_top_pages(unsigned int top_index){
	assert(top_index < TOP_P);
	return r_top3_pages[top_index];
}

unsigned int Module_9::read_reg(unsigned int addr){
	unsigned int result = 0;
	assert(!(addr&0x3)); // word aligned address, this is the case because addresses on the bus are word alligned. Is up to the cache controller
			     // or the processor to retrieve the correct sub-data from this 32bits data (ie, 16 low/up bits, or 8 low,low-mid,mid-up,up
			     // bits)
			     // Here, we will give a complete and correct "32bits, unsigned int" data
	switch((addr & MASK_NCOUNT) >> SHIFT_NCOUNT){
		case (NCOUNT) :
			{
				unsigned int offset = addr & ~(MASK_NCOUNT);    
				unsigned int offset_page = (offset & m_ADDR_NCOUNT_PSEL_MASK) >> m_ADDR_NCOUNT_PSEL_SHIFT;
				unsigned int offset_node = (offset & m_ADDR_NCOUNT_NSEL_MASK) >> m_ADDR_NCOUNT_NSEL_SHIFT;

				D9COUT << name() << "Access to NCOUNT " << std::hex <<"addr : 0x" << addr << " <-> 0x" <<  (void*)(m_BASE_ADDR + (offset_page << m_PAGE_SHIFT)) << endl;
				D9COUT << name() << "                 " << std::dec <<"page : " << offset_page  << endl;
				D9COUT << name() << "                 " << std::dec <<"shift page: " << m_ADDR_NCOUNT_PSEL_SHIFT  << endl;
				D9COUT << name() << "                 " << std::dec <<"shift node: " << m_ADDR_NCOUNT_NSEL_SHIFT  << endl;
				D9COUT << name() << "                 " << std::dec <<"mask page: " << m_ADDR_NCOUNT_PSEL_MASK  << endl;
				D9COUT << name() << "                 " << std::dec <<"mask node: " << m_ADDR_NCOUNT_NSEL_MASK  << endl;
				D9COUT << name() << " test uint32_log2(4) :" << uint32_log2(4) << endl;
				                                   D9COUT << name() <<"node : " << offset_node << endl;
				// build a response which is an agregate of continous sub-32bits data
				for(unsigned int i = 0, j = 0; i < sizeof(unsigned int); i+= sizeof(counter_t), j++){
					// alligments assert that (offset_node + j) << m_NB_NODES
					assert((offset_node + j) < m_NB_NODES);
					result = result | (r_counters[offset_page][offset_node + j] << sizeof(counter_t)*8*j);
					D9COUT << name() << std::hex <<" 0x" << result << std::dec << "("<< r_counters[offset_page][offset_node+j] << ")"; 
				}
				D9COUT << name() << endl;
				return result;
				break; 
			}
		default :
			{
				assert(!((addr & MASK_NCOUNT) >> SHIFT_NCOUNT));
				switch ((addr & MASK_NOT_NCOUNT) >> SHIFT_NOT_NCOUNT){
					case GCOUNT:
						{
							unsigned int offset = addr & ~(MASK_NOT_NCOUNT);    
							unsigned int offset_page = (offset & m_ADDR_GCOUNT_PSEL_MASK) >> m_ADDR_GCOUNT_PSEL_SHIFT;
							D9COUT << name() << "Access to GCOUNT " << std::hex << (void*)(m_BASE_ADDR + (offset_page << m_PAGE_SHIFT)) << endl; 

							// build a response which is an agregate of continous sub-32bits data
							for(unsigned int i = 0, j = 0; i < sizeof(unsigned int); i+= sizeof(counter_t), j++){
								// alligments assert that (offset_node + j) << m_NB_NODES
								assert((offset_page + j) < m_NB_PAGES);
								result = result | (r_global_counters[offset_page+j] << sizeof(counter_t)*8*j);
								D9COUT << name() << std::hex <<" 0x" << result << std::dec << "("<< r_global_counters[offset_page+j]
									<< ")"; 
							}
							D9COUT << name() << endl;
							return result;
							break;
						}
					case TIDP :
						{
							unsigned int offset = addr & ~(MASK_NOT_NCOUNT);    
							unsigned int offset_page = (offset & m_ADDR_TIDP_PSEL_MASK) >> m_ADDR_TIDP_PSEL_SHIFT;
							unsigned int offset_node = (offset & m_ADDR_TIDP_NSEL_MASK) >> m_ADDR_TIDP_NSEL_SHIFT;
							D9COUT << name() << "Access to TIDP " << std::hex << (void*)(m_BASE_ADDR + (offset_page << m_PAGE_SHIFT)) << endl; 
							D9COUT << name() << "         page :" << std::dec << offset_page << endl;
							D9COUT << name() << "         node :" << std::dec << offset_node << endl;
							// build a response which is an agregate of continous sub-32bits data
							for(unsigned int i = 0, j = 0; i < sizeof(unsigned int); i+= sizeof(counter_id_t), j++){
								// alligments assert that (offset_node + j) << m_NB_NODES
								assert((offset_node + j) < m_NB_NODES);
								result = result | (r_top3_id_perpage[offset_page][offset_node+j] << sizeof(counter_id_t)*j*8);
								D9COUT << name() << std::hex <<" 0x" << result << std::dec << "("<< r_top3_id_perpage[offset_page][offset_node+j] << ")"; 
					
							}
							D9COUT << name() << endl;
							return result;
							break;
						}
						break;
					case MAXIP :
						{
							unsigned int offset = addr & ~(MASK_NOT_NCOUNT);    
							unsigned int offset_page = (offset & m_ADDR_MAXIP_PSEL_MASK) >> m_ADDR_MAXIP_PSEL_SHIFT;
							D9COUT << name() << "Access to MAX ID P " << std::hex << (void*)(m_BASE_ADDR + (offset_page << m_PAGE_SHIFT)) << endl;
							// build a response which is an agregate of continous sub-32bits data
							for(unsigned int i = 0, j = 0; i < sizeof(unsigned int); i+= sizeof(counter_id_t), j++){
								// alligments assert that (offset_node + j) << m_NB_NODES
								assert((offset_page + j) < m_NB_PAGES);
								result = result | (r_max_id_perpage[offset_page+j] << sizeof(counter_id_t)*8*j);
								D9COUT << name() << std::hex <<" 0x" << result << std::dec << "("<< r_max_id_perpage[offset_page+j] << ")"; 
					
							}
							D9COUT << name() << endl;
							return result;
							break;
						} 
					case TMAXP :
						D9COUT << name() << "Acess T MAX_PAGE :" << endl;
						D9COUT << name() << "            addr :" << std::hex << (void*)addr << endl;
						D9COUT << name() << "            mask :" << std::hex << (void*)MASK_T_MAX_PAGE << endl;
						D9COUT << name() << "            shift:" << std::dec << (void*)SHIFT_T_MAX_PAGE << endl;
						switch (((addr & ~(MASK_NOT_NCOUNT)) & MASK_T_MAX_PAGE) >> SHIFT_T_MAX_PAGE){
							case TP :
								{
									unsigned int offset = addr & ~(MASK_T_MAX_PAGE);    
									unsigned int offset_id = (offset & m_ADDR_TP_MASK) >> m_ADDR_TP_SHIFT;
									D9COUT << name() << "Access to Top3 Maxpage " << std::hex << (void*)(m_BASE_ADDR)  << endl;
									D9COUT << name() << "            the top is :" << endl;

									for(unsigned int i = 0; i < TOP_P ; i++){
										D9COUT << name() << "            " << i << " : " << r_top3_pages[i]  << endl;
									}
									// build a response which is an agregate of continous sub-32bits data
									for(unsigned int i = 0, j = 0; i < sizeof(unsigned int); i+= sizeof(counter_id_t), j++){
										// alligments assert that (offset_node + j) << m_NB_NODES
										assert((offset_id + j) < TOP_P);
										result = result | (r_top3_pages[offset_id+j] << sizeof(counter_id_t)*8*j);
										D9COUT << name() << std::hex <<" 0x" << result << std::dec << "("<< r_top3_pages[offset_id+j] << ")"; 

									}
									D9COUT << name() << endl;
									return result;
									break;
								} 
							case MAX : 
									D9COUT << name() << "Access to Maxpage " << std::hex << (void*)(m_BASE_ADDR)  << endl;
									// build a response which is an agregate of continous sub-32bits data
									for(unsigned int i = 0, j = 0; i < sizeof(unsigned int); i+= sizeof(counter_id_t), j++){
										// alligments assert that (offset_node + j) << m_NB_NODES
										result = result | (r_max_page << sizeof(counter_id_t)*8*j);
										D9COUT << name() << std::hex <<" 0x" << result << std::dec << "("<< result << ")"; 

									}
									D9COUT << name() << endl;
								return result;
								break;
							default :
								assert(false);
								break;
						}
						break;
					default :
						assert(false);
						break;
				}
				break;
			}
	}

	assert(addr < m_NB_PAGES);	
	return 0;
}

unsigned int Module_9::reset_reg(unsigned int addr){
	//assert(false); //unimplemented feature
	assert(addr < m_NB_PAGES);
	for (unsigned int i = 0; i < m_NB_NODES; i++){
		r_counters[addr][i] = 0;
	}
	for(unsigned int j = 0 ; j < m_NB_NODES ; j++)
		r_top3_id_perpage[addr][j] = -1; // reset value, empty
	r_global_counters[addr] = 0;
	r_raise_threshold = false;
	for(unsigned int j = 0 ; j < TOP_P ; j++)
		r_top3_pages[j] = -1; // reset value, empty
	return 0;
}

void Module_9::reset(){
		// todo : reset 1 cycle is a little hard to reset all the counters

		for (unsigned int i = 0; i <  m_NB_PAGES ; i++){
			for(unsigned int j = 0 ; j < m_NB_NODES ; j++){
				r_counters[i][j]=0;
			}
			for(unsigned int j = 0 ; j < m_NB_NODES ; j++)
				r_top3_id_perpage[i][j] = -1; // reset value, empty
			r_max_id_perpage[i] = 0; // by default node 0 is the max
			r_global_counters[i] = 0;
			d_global_counters[i] = 0;
			r_migrable[i] = true;
		}
		for(unsigned int j = 0 ; j < TOP_P ; j++)
			r_top3_pages[j] = -1; // reset value, empty

		r_max_page = 0; // by default the first page is the maximum
		r_index_id = 0;
		r_index_p = 0;
		r_raise_threshold = false;
}

void Module_9::reset_counters(){
		// todo : reset 1 cycle is a little hard to reset all the counters
		for (unsigned int i = 0; i <  m_NB_PAGES ; i++){
			for(unsigned int j = 0 ; j < m_NB_NODES ; j++){
				r_counters[i][j]=0;
			}
			r_global_counters[i] = 0;
		}
}

void Module_9::gen_outputs(){
	*(p_out.is_busy) = busy();
	*(p_out.is_threshold) = r_raise_threshold;
}

void Module_9::compute()
{
	const bool p_in_enable   = *(p_in.enable);
	const unsigned int p_in_page_sel = *(p_in.page_sel);
	const unsigned int p_in_node_id  = *(p_in.node_id);
	const counter_t p_in_cost     = *(p_in.cost);
	const bool freeze = *(p_in.freeze);
	// counters compute
	//
	if(p_in_enable && (m_COMPUTE_TIME == 0) && (freeze)){
		D9COUT << name() << name() << "----------  froozen, can not compute! ---------" << endl;
	}
	r_raise_threshold = false; // this signal is raised for one cycle, and is maybe taken into account!
	// On n'incrémente les compteurs que sous certaines conditions, et l'une d'entre elles est que la page
	// est migrable! ie, qu'elle n'a pas abortée une migration au round précédent!
	if(p_in_enable && (m_COMPUTE_TIME == 0) && (!freeze) && (r_migrable[p_in_page_sel])){
		const unsigned int p = p_in_page_sel;
		const unsigned int n = p_in_node_id;
		const counter_t c = p_in_cost; // todo, warning, cost sould be unsigned, implicit cast will be applied
		const unsigned int max_p = r_max_page;
		const unsigned int index_p = r_index_p;
		const unsigned int index_id = r_index_id;
		assert(p < m_NB_PAGES);
		assert(c == 1);
		assert(max_p < m_NB_PAGES);
		assert(index_p < TOP_P);
		assert(index_id < m_NB_NODES);
		//assert((order(r_global_counters[max_p], r_global_counters[p], NBIT_ORDER) == S ) || 
		//		(order(r_global_counters[max_p], r_global_counters[p], NBIT_ORDER) == L ));
		//assert((unsigned int)r_global_counters[p] <= 0xFFFF);

		// Only compute page access for processor modules
		// This may introduce error in top3 since contention may appear (module_8) due to
		// heavy DMA acces to pages not acccessed by a processor
		D9COUT <<  name() << " p_in_page_sel->" << p_in_page_sel <<  endl;
		D9COUT <<  name() << " p_in_node_id->" << p_in_node_id <<  endl;
		if (!(n < m_NB_NODES)){
			CWOUT << "access by node : "<< dec <<  n << endl;
			return;
		}
		// How many cycles will be needed to compute the entry
		m_COMPUTE_TIME = COMPUTE_TOP_CYCLES ;


		{
			counter_t sum = 0;
			for (unsigned int j = 0; j < m_NB_NODES; j++){
				sum+=r_counters[p][j];
			}
			if (r_global_counters[p] != sum){
	
				for (unsigned int j = 0; j < m_NB_NODES; j++){
					D9COUT << name() << dec << r_counters[p][j] << endl;
				}
				D9COUT << name() << r_global_counters[p] << endl;
				D9COUT << name() << sum << endl;
			}
			assert(r_global_counters[p] == sum);
		}
		// Computing the new id with is the most active user of this page
		// this kind of access is realistic, first load a complete line , and then select the correct word's (like in a cache)
		
		// computing the top 3 of pages
		switch (order(r_global_counters[max_p], (r_global_counters[p] + c), NBIT_ORDER)){
			// todo : I think this cannot be done in one cycle, .. perhaps we should compute maximum only every 100 cycles?
			case S :
				{
					bool is_already_here = false;
					D9COUT << name() << "S : r_max_page->" << max_p << " value->" << r_global_counters[max_p] << endl;
					D9COUT << name() << "    vs p->" << p << " value->" << (r_global_counters[p] + c)<< endl;
					assert(p<0x7FFFFFFFu); //This is nevertheless not possible, 4G pages in a ram bank?
					for (int i = 0; i < TOP_P ; i++){
						if(r_top3_pages[i]== (int) p) // warning p is unsigned, p should not be > 0x7FFFFFFF
							is_already_here = true;
					}
					if (!is_already_here){
						r_top3_pages[index_p] = p;
						if (index_p == TOP_P - 1)
							r_index_p = 0;
						else
							r_index_p = index_p + 1;
					}
				break;
				}
			case G :
				// update max
				r_max_page = p;
				D9COUT << name() << "G : r_max_page->" << max_p << " value->" << r_global_counters[max_p] << endl;
				D9COUT << name() << "    vs p->" << p << " value->" << (r_global_counters[p] + c)<< endl;
				for (unsigned int i = 0; i < TOP_P ; i++){
					if (i != index_p)
						r_top3_pages[i] = -1;
					else
						r_top3_pages[i] = p;
				}
				if (index_p == TOP_P - 1)
					r_index_p = 0;
				else
					r_index_p = index_p + 1;
				break;
			case L :
				D9COUT << name() << "L : r_max_page->" << max_p << " value->" << r_global_counters[max_p] << endl;
				D9COUT << name() << "    vs p->" << p << " value->" << (r_global_counters[p] + c)<< endl;
				// nothing to do
				break;
			default :
				assert(false);
		}
		// computing the top 3 for this page
		switch (order((r_counters[p][r_max_id_perpage[p]]) , (r_counters[p][n] + c), NBIT_ORDER)){
			case S :
				{
					bool is_already_here = false;
					assert(n<0x7FFFFFFFu); //This is nevertheless not possible, 4G processors in a platform?
					for (int i = 0; i < m_NB_NODES ; i++){
						if(r_top3_id_perpage[p][i]== (int) n)// warning p is unsigned, p should not be > 0x7FFFFFFF
							is_already_here = true;
					}
					if (!is_already_here){
						r_top3_id_perpage[p][index_id] = n;
						if ((index_id) == m_NB_NODES-1)
							r_index_id = 0;
						else
							r_index_id = index_id + 1;
						
					}
				break;
				}
			case G :
				// update max
				r_max_id_perpage[p] = n;
				for (unsigned int i = 0; i < m_NB_NODES ; i++){
					if (i != index_id)
						r_top3_id_perpage[p][i] = -1;
					else
						r_top3_id_perpage[p][i] = n;
				}
				if (index_id == m_NB_NODES - 1)
					r_index_id = 0;
				else
					r_index_id = index_id + 1;
				break;
			case L :
				// nothing to do	
				break;
			default :
				assert(false);
				break;
		}
		
		
		// Increment page acces cost for [page_number][node_id] and global counter[page_number]
		if ((counter_t)((counter_t)r_global_counters[p] + c ) < (counter_t)r_global_counters[p]){
			// overflow of this counter, reset the line
			D9COUT << name() << "OVERFLOW" << endl;
			for (unsigned int i = 0; i < m_NB_NODES; i++){
				r_counters[p][i] = 0;
				r_global_counters[p] = 0;
				d_global_counters[p] = 0;
			}	
		}else{
			r_counters[p][n] = r_counters[p][n] + c;
			r_global_counters[p] = r_global_counters[p] + c;
			d_global_counters[p] = d_global_counters[p] + c;

#ifdef PE_ON_GLOBAL
			if (r_global_counters[p] M9_COMP_OP PE_PERIOD) r_raise_threshold = true;
#else
			if (r_counters[p][n] M9_COMP_OP PE_PERIOD) r_raise_threshold = true;
#endif

			D9COUT << name() << "page->" << p << " value->" << r_global_counters[p] << endl;
		}
	}
	// wait for
	if (m_COMPUTE_TIME != 0){
		m_COMPUTE_TIME--;
	}
}

bool Module_9::is_pow2(unsigned int value) {
	for(unsigned int i=0; i < 32 ; i++)
	{
		if (value & 0x1) return  (value==1);
		value = value >> 1;
	}
	return false;

}
Module_9::~Module_9(){	
	DTOR_OUT
	counter_t * array = new counter_t[m_NB_PAGES];
	counter_t * array_index = new counter_t[m_NB_PAGES];
	unsigned long long sum = 0;
	unsigned long long total_sum = 0;
	float top3_percent = 0.0;
	float real_top3_percent = 0.0;
	int top3_total = 0;
	int real_top3_total = 0;
	file_stats << name() << " STATS OUTPUT " << endl;	
	//file_stats << std::hex << " r_max_page : " <<  (void*)(m_BASE_ADDR + (r_max_page << m_PAGE_SHIFT)) << endl;
	for(unsigned int i = 0; i < m_NB_PAGES; i++){
		sum = 0;
		total_sum += d_global_counters[i];
		array[i] = d_global_counters[i];
		array_index[i] = i;
	}
	// sorting array page access 
	quickSort(array,array_index,0,m_NB_PAGES-1);	

	// hardware calculated top3 percentage
	//file_stats << " r_top_pages : " << endl;
	for(unsigned int i = 0; i < TOP_P ; i++){
			real_top3_total += array[m_NB_PAGES-(1+i)]; 
//			file_stats << std::hex << (void*)(m_BASE_ADDR + (array_index[m_NB_PAGES-(i+1)] << m_PAGE_SHIFT)) << endl;
	}
	real_top3_percent = (((float)real_top3_total*100.0)/(float)total_sum);

	// printing non zero values
	for(unsigned int i = 0 ; i < m_NB_PAGES ; i++){
		#if 1
		if(array[i]!=0)
			file_stats << std::hex << (void*)(m_BASE_ADDR + (array_index[i] << m_PAGE_SHIFT)) 
			     << " -> " << std::dec << array[i] 
			     << " " << ((float)(array[i]*100)/(float)total_sum) << "%" << endl;
			     #endif
	}
	file_stats << std::dec << "Total access : " << total_sum << endl;
	file_stats << std::dec << "Top 3 represents: " << top3_percent << endl;
	file_stats << std::dec << "Real Top 3 represents: " << real_top3_percent << endl;

	delete [] array;
	delete [] array_index;
	for(unsigned int i = 0 ; i < m_NB_PAGES ; i++){
		delete[] r_counters[i];
	}
	delete [] r_counters;
	delete [] d_global_counters;
	delete [] r_global_counters;
	delete [] r_max_id_perpage;
	for(unsigned int i = 0 ; i < m_NB_PAGES ; i++){
		delete [] r_top3_id_perpage[i]; 
	}
	delete [] r_top3_id_perpage; 
	delete [] r_top3_pages;
	
}

}}
