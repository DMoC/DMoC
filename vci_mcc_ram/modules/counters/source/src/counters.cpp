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

Counters::Counters(sc_module_name insname,unsigned int nb_nodes,unsigned int nb_pages,unsigned int base_addr,unsigned int page_size):
	soclib::caba::BaseModule(insname),
	r_max_page("max_page"),
	r_index_p("index_p"),
	r_raise_threshold("raise_threshold"),
	m_raise_threshold(false),
	r_save_page_sel("save_page_sel"),
	r_save_node_id("save_node_id"),
	r_save_cost("save_cost"),
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


	m_ADDR_MAXIP_PSEL_SHIFT(uint32_log2(sizeof(counter_id_t))),	
	m_ADDR_MAXIP_PSEL_MASK((m_NB_PAGES -1) << m_ADDR_MAXIP_PSEL_SHIFT)

	{
		srand(0);
		// Assertion :
		assert(is_pow2(m_NB_PAGES));
		assert(is_pow2(m_NB_NODES));
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
		r_counters = new sc_signal< counter_t > * [m_NB_PAGES];
		r_global_counters = new sc_signal< counter_t > [m_NB_PAGES];
		d_global_counters = new sc_signal< counter_t > [m_NB_PAGES];


		for(unsigned int i = 0; i < m_NB_PAGES ; i++){
			r_counters[i] = new  sc_signal< counter_t >  [m_NB_NODES];
		}

	}

Counters::~Counters(){	
	DTOR_OUT
	counter_t * array = new counter_t[m_NB_PAGES];
	counter_t * array_index = new counter_t[m_NB_PAGES];
	unsigned long long sum = 0;
	unsigned long long total_sum = 0;
	float real_top3_percent = 0.0;
	int real_top3_total = 0;
#ifdef USE_STATS
	file_stats << name() << " STATS OUTPUT " << endl;	
#endif
	//file_stats << std::hex << " r_max_page : " <<  (void*)(m_BASE_ADDR + (r_max_page << m_PAGE_SHIFT)) << endl;
	for(unsigned int i = 0; i < m_NB_PAGES; i++){
		sum = 0;
		total_sum += d_global_counters[i];
		array[i] = d_global_counters[i];
		array_index[i] = i;
	}
	// sorting array page access 
	quickSort(array,array_index,0,m_NB_PAGES-1);	

	for(unsigned int i = 0; i < 3 ; i++){
			real_top3_total += array[m_NB_PAGES-(1+i)]; 
//			file_stats << std::hex << (void*)(m_BASE_ADDR + (array_index[m_NB_PAGES-(i+1)] << m_PAGE_SHIFT)) << endl;
	}
	real_top3_percent = (((float)real_top3_total*100.0)/(float)total_sum);

	// printing non zero values
	for(unsigned int i = 0 ; i < m_NB_PAGES ; i++){
		#if 0
		if(array[i]!=0)
			file_stats << std::hex << (void*)(m_BASE_ADDR + (array_index[i] << m_PAGE_SHIFT)) 
			     << " -> " << std::dec << array[i] 
			     << " " << ((float)(array[i]*100)/(float)total_sum) << "%" << endl;
			     #endif
	}
#ifdef USE_STATS
	file_stats << std::dec << "Total access : " << total_sum << endl;
	file_stats << std::dec << "Real Top 3 represents: " << real_top3_percent << endl;
#endif

	delete [] array;
	delete [] array_index;
	for(unsigned int i = 0 ; i < m_NB_PAGES ; i++){
		delete[] r_counters[i];
	}
	delete [] r_counters;
	delete [] d_global_counters;
	delete [] r_global_counters;
	
}

}}
