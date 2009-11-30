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

#include "../include/sram.h"
#include "soclib_endian.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace soclib {
namespace caba {
#ifdef CDB_COMPONENT_IF_H
#define tmpl(x) template<typename sram_param> x SRam<sram_param>
tmpl(void)::printramlines(struct modelResource *d)
{
#if 1
				unsigned int start      = d->start;
        unsigned int len        = d->len;
        unsigned int segnum = d->usr2;
        
				// get the correct memory bank!
				std::list<soclib::common::Segment>::iterator     iter = m_segment_list -> begin();
				for (unsigned int i = 0; i < segnum ; i++)
				{
					iter++; 
				}
				assert(iter != m_segment_list->end());

				for (unsigned int i = 0; i < len ; i ++)
				{
						std :: cout << std::hex << ( iter->baseAddress() + ((((start + i)*8)) << m_ADDR_WORD_SHIFT)) << " : " ;
						for (unsigned int j = 0 ; j < 8 ; j++)
							{
									std::cout << (uint32_t) read_write_sram(segnum, ((start + i) * 8 + j), 0, false , true, 0) << " ";
							}
							std::cout << std::endl;
				} 

				
#endif
}

tmpl(const char*)::GetModel()
{
    return("Generic S-ram");
}
//
tmpl(void)::printramdata(struct modelResource *d)
{
	typename sram_param::addr_t addr   = d->start;
	typename sram_param::addr_t offset;
	unsigned int segnum = d->usr2;

	// get the correct memory bank!
	std::list<soclib::common::Segment>::iterator     iter = m_segment_list -> begin();
	for (unsigned int i = 0; i < segnum ; i++)
	{
		iter++; 
	}
	assert(iter != m_segment_list->end());
	offset = (addr - iter -> baseAddress()) >> m_ADDR_WORD_SHIFT;
	

	std :: cout << std::dec << " line : "<< (offset / 8) << std::hex << " addr : " <<  addr << " : " ;
	std::cout << (uint32_t) read_write_sram(segnum, offset, 0, false , true, 0) << " ";
	std::cout << std::endl;
}

tmpl(int)::PrintResource(modelResource *res,char **p)
{
#if 1
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
				std::cerr << "ram : No such ressource" << std::endl; //todo, CACHE should be name
				break;
		}
	}else if(*p[0]>0){
		index = -1;
		for (iter = m_segment_list->begin() ; iter != m_segment_list->end() ; ++iter) {
			index++; 
			if (strcmp(iter->name().c_str(),p[1]) == 0) {
				break;
			}
		}
		if (index != -1){
			i++;
			if (*p[i] == (char)0) {
				res->start = 	0;
				res->len =  (iter->size() / sram_param::B) / 8; // print by 8 data per line
				res->usr1 = 1;
			} else if (*p[i + 1] == (char)0) {
				res->start = 	strtoul(p[i],&misc,0); // start est une adresse!
				res->usr1 = 2;
			} else {
				res->start = 	strtoul(p[i],&misc,0);
				res->len = 	strtoul(p[i + 1],&misc,0);
				res->usr1 = 1;
			}
			res->obj = this;
			res->usr2 = index;
			return 0;
		} else
			fprintf(stderr, "%s: No such segment: %s\n",name().c_str(),p[i]);
	}
#endif
	return -1;
}

tmpl(int)::TestResource(modelResource *res,char **p)
{
#if 1
int i = 1;
int index;
unsigned int address;
char * misc;
std::list<soclib::common::Segment>::iterator     iter;
	/* Shall we make a specific error function for all
	* print/display/test functions, so to be homogeneous */
	
#if 1
	if (*p[i] == (char)0) {
		fprintf(stderr, "needs a segment name argument!\n");
		return -1;
	}
	index = -1 ;
	for (iter = m_segment_list->begin() ; iter != m_segment_list->end() ; ++iter) {
		index++; 
		if (strcmp(iter->name().c_str(),p[1]) == 0) {
			break;
		}
	}

	if (index != -1) {
		i++;
		if (*p[i + 1] == (char)0) {
			address = strtoul(p[i],&misc,0);
			res->addr =    (int*)&s_RAM[index][(address - (iter -> baseAddress())) >> m_ADDR_WORD_SHIFT];
			return 0;
		}else
			fprintf(stderr, "Error, expected: <segment> <addr>\n");
	}else
		fprintf(stderr, "%s: No such segment: %s\n",name().c_str(),p[i]);   
#endif
#endif
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
	for (iter = m_segment_list -> begin() ; iter != m_segment_list -> end() ; ++iter) {
		cout << iter->name() <<  " ";	
	}
	cout << std::endl;
	return  0;
}
#endif
}}
