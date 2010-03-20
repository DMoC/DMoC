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
#include <cstdlib>
#include "counters.h"
#include "arithmetics.h"

namespace soclib {
namespace caba {

	using namespace soclib;
	using namespace std;
	using soclib::common::uint32_log2;


	void Counters::quickSort(counter_t tableau[], counter_t indices[], counter_t p, counter_t r) {
		counter_t q;
		if(p<r) {
			q = partitionner(tableau, indices, p, r);
			quickSort(tableau, indices, p, q);
			quickSort(tableau, indices, q+1, r);
		}
	}

	counter_t Counters::partitionner(counter_t tableau[], counter_t indices[], counter_t p, counter_t r) {
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



#if 0
	const string & Counters::name(){
		return this->m_name;
	}	
#endif


	counter_t Counters::read_global_counters(unsigned int page_index){
		assert(page_index < m_NB_PAGES);
		return r_global_counters[page_index];
	}

	counter_t Counters::read_counter(unsigned int page_index, unsigned int node_index){
		assert(page_index < m_NB_PAGES);
		assert(node_index < m_NB_NODES);
		return r_counters[page_index][node_index];
	}

	counter_t Counters::read_rounded_counter(unsigned int page_index, unsigned int node_index)
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

	counter_id_t Counters::read_max_page(){
		return r_max_page;
	}
	counter_id_t Counters::evict_sel(){
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


	unsigned int Counters::reset_reg(unsigned int addr){
		m_COMPUTE_TIME = CPT_TIME;
		//assert(false); //unimplemented feature
		assert(addr < m_NB_PAGES);
		for (unsigned int i = 0; i < m_NB_NODES; i++){
			r_counters[addr][i] = 0;
		}
		r_global_counters[addr] = 0;
		r_raise_threshold = false;
		return 0;
	}

	void Counters::reset_counters(){
		// todo : reset 1 cycle is a little hard to reset all the counters
		for (unsigned int i = 0; i <  m_NB_PAGES ; i++){
			for(unsigned int j = 0 ; j < m_NB_NODES ; j++){
				r_counters[i][j]=0;
			}
			r_global_counters[i] = 0;
		}
	}


	bool Counters::is_pow2(unsigned int value) {
		for(unsigned int i=0; i < 32 ; i++)
		{
			if (value & 0x1) return  (value==1);
			value = value >> 1;
		}
		return false;

	}

}}
