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
#if 0
        #define Z(i) ((char)(tab[line*m_BLOCK_SIZE + li]>>i)>=' '&&(char)(tab[line*m_BLOCK_SIZE + li]>>i)<='~'?(char)(tab[line*m_BLOCK_SIZE + li]>>i):'.')
        unsigned int line;
        unsigned int li;
				unsigned int start      = d->start;
        unsigned int len        = d->len;
        unsigned int segnum = d->usr2;
        unsigned int *tab;
        unsigned int nb_max_lines;
        
        tab = r_RAM;
        // Test on start and len parameters, adjust them if necessary to avoid out-of-bound accesses.
        nb_max_lines = ((m_segment.Size() >> 2)/m_BLOCK_SIZE);
        if (start > nb_max_lines) start = 0;
        if (start + len > nb_max_lines) len = nb_max_lines - start;
        
        // Display each line in start -> start + len.
        putc('\n',stdout); //todo
		assert(segnum == 0);
        for (line =  start ; line < start + len; line++) {
                printf( "0x%08x TAG=(",(((line*m_BLOCK_SIZE) << 2) + m_segment.baseAddress()));
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
#endif
}
tmpl(const char*)::GetModel()
{
    return("Write-through Write-invalidate Multisegment S-RAM memory");
}
//
tmpl(void)::printramdata(struct modelResource *d)
{
#if  0
        #define W r_RAM[word]
        #define Z(i) ((char)(W>>i)>=' '&&(char)(W>>i)<='~'?(char)(W>>i):'.')
        unsigned int address  = d->start;
        unsigned int segnum = d->usr2;
        unsigned int word;
        
        
        if ((address < m_segment.baseAddress()) || (address > (m_segment.baseAddress() + m_segment.size()))){
                word = 0;
                printf("address out of range");
        } else word = (((address & ~m_LSBMASK) - m_segment.baseAddress()) >> 2);


        printf("line:%d %08x ",word/m_BLOCK_SIZE ,W);
        printf("%c%c%c%c\n", Z(0), Z(8), Z(16), Z(24));
        #undef W
        #undef Z
#endif
}

tmpl(int)::PrintResource(modelResource *res,char **p)
{
#if 0
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
#endif
	return -1;
}

tmpl(int)::TestResource(modelResource *res,char **p)
{
#if 0
int i = 1;
int index;
unsigned int address;
char * misc;
std::list<soclib::common::Segment>::iterator     iter;
	/* Shall we make a specific error function for all
	* print/display/test functions, so to be homogeneous */
	
#if 0
	if (*p[i] == (char)0) {
		fprintf(stderr, "needs a segment name argument!\n");
		return -1;
	}
	index = -1 ;
	if (index != -1) {
		i++;
		if (*p[i + 1] == (char)0) {
			address = strtoul(p[i],&misc,0);
			res->addr =    (int*)&r_RAM[index][((address & ~m_LSBMASK) - m_segment.base()) >> 2];
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
#if 0
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
#endif
	return 0;
}
#endif
}}
