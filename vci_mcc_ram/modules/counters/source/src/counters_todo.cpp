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


#if 0
unsigned int Counters::read_reg(unsigned int addr){
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
#endif
}}
