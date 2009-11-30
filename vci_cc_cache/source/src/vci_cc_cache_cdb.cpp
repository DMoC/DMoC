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

#ifdef CDB_COMPONENT_IF_H
#include "vci_cc_cache.h"
#include <iomanip>
namespace soclib { 
namespace caba {
#define SC

#define tmpl(x)  template<typename vci_param, typename iss_t> x VciCcCache<vci_param, iss_t>
	tmpl(void)::printdcachelines(struct modelResource *d)
	{
		unsigned int start      = d->start;
		unsigned int len        = d->len;

		// Test on start and len parameters, adjust them if necessary to avoid out-of-bound accesses.

		if (start > s_icache_lines) start = 0;
		if (start + len > s_icache_lines) len = s_icache_lines - start;

		// Display each line in start -> start + len.
		std::cout << std::endl;
		for (unsigned int y_adr =  start ; y_adr < start + len; y_adr++)
		{
			std::cout << " l-" << std::dec << y_adr << " ";
			for (unsigned int li = 0; li < s_icache_words; li++)
				std::cout << std::hex << std::setfill('0') <<  std::setw(10) << s_DCACHE_DATA[y_adr][li].read() << " ";
			std::cout << " TAG "  << std::hex << std::setfill('0') <<  std::setw(10) << s_DCACHE_TAG[y_adr].read() << std::endl;
		}

#if 0
#define W s_DCACHE_DATA[y_adr][li].read().read()
#define Z(i) ((char)(W>>i)>=' '&&(char)(W>>i)<='~'?(char)(W>>i):'.')
		unsigned int start      = d->start;
		unsigned int len        = d->len;

		// Test on start and len parameters, adjust them if necessary to avoid out-of-bound accesses.

		if (start > s_dcache_lines) start = 0;
		if (start + len > s_dcache_lines) len = s_dcache_lines - start;

		// Display each line in start -> start + len.
		puts("\n");
		for (unsigned int y_adr =  start ; y_adr < start + len; y_adr++) {
			printf( "%d TAG=(0x%08x) %c",y_adr,(unsigned int)s_DCACHE_TAG[y_adr].read().read(),
					((s_DCACHE_TAG[y_adr].read().read() & 0x80000000) ? 'V' : 'I'));
			for (unsigned int li = 0; li < s_dcache_words; li++)
				printf( "%08x ", (unsigned int)(W));
			for (unsigned int li = 0; li < s_dcache_words; li++)
				printf( " %c%c%c%c", Z(0), Z(8), Z(16), Z(24));
			puts("\n");
		}
#undef W
#undef Z
#endif
	}

	tmpl(void)::printdcachedata(struct modelResource *d)
	{
		unsigned int x_adr, y_adr, z_adr;
		unsigned int address     = d->start;

		x_adr = m_i_x[address];
		y_adr = m_i_y[address];
		z_adr = m_i_z[address];
		std::cout << " addr " << std::hex << std::setw(10) << address <<  " l-" << std::dec << y_adr << " " << " cached tag ";
		std::cout << std::hex << std::setfill('0') <<  std::setw(10) << (s_DCACHE_TAG[y_adr].read() );
		std::cout << " " <<  std::setfill('0') << std::setw(10) << s_DCACHE_DATA[y_adr][x_adr].read() << std::endl;

#if 0
#define W s_DCACHE_DATA[y_adr][x_adr].read().read()
#define Z(i) ((char)(W>>i)>=' '&&(char)(W>>i)<='~'?(char)(W>>i):'.')
		typename vci_param::addr_t x_adr, y_adr, z_adr;
		typename vci_param::addr_t address     = d->start;


		std::cout << "dcache data" << std::endl;
		x_adr = m_d_x[address];
		y_adr = m_d_y[address];
		z_adr = m_d_z[address];

		printf( "%08x line:%d TAG=(%02x) %c: ", (unsigned int)address.read(), (unsigned int)y_adr.read(), (unsigned int)(s_DCACHE_TAG[(unsigned int)y_adr].read().read()),
				((z_adr | 0x80000000) == (s_DCACHE_TAG[(unsigned int)y_adr].read().read()) ) ? 'V' : 'I');
		printf( "%08x ", (unsigned int)W);
		printf( "%c%c%c%c\n", Z(0), Z(8), Z(16), Z(24));
#undef W
#undef Z
#endif
	}


	tmpl(void)::printicachelines(struct modelResource *d)
	{
		unsigned int start      = d->start;
		unsigned int len        = d->len;

		// Test on start and len parameters, adjust them if necessary to avoid out-of-bound accesses.

		if (start > s_icache_lines) start = 0;
		if (start + len > s_icache_lines) len = s_icache_lines - start;

		// Display each line in start -> start + len.
		std::cout << std::endl;
		for (unsigned int y_adr =  start ; y_adr < start + len; y_adr++)
		{
			std::cout << " l-" << std::dec << y_adr << " ";
			for (unsigned int li = 0; li < s_icache_words; li++)
				std::cout << std::hex << std::setfill('0') <<  std::setw(10) << s_ICACHE_DATA[y_adr][li].read() << " ";
			std::cout << " TAG "  << std::hex << std::setfill('0') <<  std::setw(10) << s_ICACHE_TAG[y_adr].read() << std::endl;
		}
	}

	tmpl(void)::printicachedata(struct modelResource *d)
	{
		unsigned int x_adr, y_adr, z_adr;
		unsigned int address     = d->start;

		x_adr = m_i_x[address];
		y_adr = m_i_y[address];
		z_adr = m_i_z[address];
		std::cout << " addr " << std::hex << std::setw(10) << address <<  " l-" << std::dec << y_adr << " " << " cached tag ";
		std::cout << std::hex << std::setfill('0') <<  std::setw(10) << (s_ICACHE_TAG[y_adr].read() );
		std::cout << " " <<  std::setfill('0') << std::setw(10) << s_ICACHE_DATA[y_adr][x_adr].read() << std::endl;
	}

	tmpl(const char *)::GetModel()
	{
		return(m_model);
	}


	tmpl(int)::PrintResource(modelResource *res,char **p)
	{
		int i = 1;
		char * misc;
		assert((p!=NULL)||(res->usr1!=0)); // Assert that it is a first call to PrintRessource (p!= NULL)  
		// or a repetitive one (usr1 !=0 and p==NULL) 

		// usr1 is used in this component as a "command type" to select another member call
		if (!p){
			if(res->usr2 == 1){
#if 1
				return m_iss.local_PrintResource(res,p);
#else
				assert(false);
#endif
			}else{
				switch (res->usr1){
					case 1:
						printdcachelines(res);
						return 0;
						break;
					case 2:
						printdcachedata(res);
						return 0;
						break;
					case 3:
						printicachelines(res);
						return 0;
						break;
					case 4:
						printicachedata(res);
						return 0;
						break;
					default:
						std::cerr  << name() <<  " No such ressource" << std::endl; 
						break;
				}
			}
		}else if(*p[0]>0){
			if (!strcasecmp(p[i], "dcache")) {
				i++;
				// res->f / res->addr / res->usr2 : unused fields
				if (*p[i] == (char)0) {
					res->start = 	0;
					res->len = 	s_dcache_lines;
					res->usr1 = 1; //printdcachelines
					res->obj = this; //printdcachelines
					return 0;
				} else if (*p[i + 1] == (char)0) {
					res->start = strtoul(p[i],&misc ,0);
					res->usr1 = 2; //printdcachedata
					res->obj = this; //printdcachelines
					return 0;
				} else {
					res->start = strtoul(p[i],&misc ,0);
					res->len = strtoul(p[i+1],&misc ,0);
					res->usr1 = 1; //printdcachelines
					res->obj = this; //printdcachelines
					return 0;
				}
				res->usr2 = 0;
			}else if (!strcasecmp(p[i], "icache")) {
				i++;
				// res->f / res->addr / res->usr2 : unused fields
				res->usr2 = 0;
				if (*p[i] == (char)0) {
					res->start = 	0;
					res->len = 	s_icache_lines;
					res->usr1 = 3; //printicachelines
					res->obj = this; //printicachelines
					return 0;
				} else if (*p[i + 1] == (char)0) {
					res->start = strtoul(p[i],&misc ,0);
					res->usr1 = 4; //printicachedata
					res->obj = this; //printicachelines
					return 0;
				} else {
					res->start = strtoul(p[i],&misc ,0);
					res->len = strtoul(p[i+1],&misc ,0);
					res->usr1 = 3; //printicachelines
					res->obj = this; //printicachelines
					return 0;
				}
			}else if (!strcasecmp(p[i], "proc")){
				res->usr2 = 1;
				res->obj = this; //printdcachelines
#if 1
				return m_iss.local_PrintResource(res,p);
#else
				assert(false);
#endif
			}
		}else{
			std::cerr << name() << " Error argument list is non null but empty" << std::endl;
		}
		return -1;
	}

	tmpl(int)::TestResource(modelResource *res,char **p)
	{
		int i = 1;
		char * misc;
		unsigned int x_adr, y_adr;
		typename vci_param::addr_t adr;

		if (*p[i] == (char)0) {
			std::cerr << name() << " Needs a ressource argument!" << std::endl;
			return -1;
		}

		if (!strcasecmp(p[i], "dcache")) {
			i++;
			if (*p[i + 1] == (char)0) {
				adr = strtoul(p[i],&misc ,0);
				x_adr = m_d_x[adr];
				y_adr = m_d_y[adr];
#ifdef SC 
				assert(false);
#else
				res->addr = (int*)(s_DCACHE_DATA[y_adr][x_adr].get_pointer()); //todo : is a valid cast?
#endif
				return 0;
			}else
				std::cerr << name() << " Error, expected: dcache <addr>" << std::endl;
		}else if(!strcasecmp(p[i], "icache")){
			i++;
			if (*p[i + 1] == (char)0) {
				adr = strtoul(p[i],&misc ,0);
				x_adr = m_d_x[adr];
				y_adr = m_d_y[adr];
#ifdef SC 
				assert(false);
#else
				res->addr = (int*)(s_ICACHE_DATA[y_adr][x_adr].get_pointer()); //todo : is a valid cast?
#endif

				return 0;
			}else
				std::cerr << name() << " Error, expected: icache <addr>" << std::endl;
		}else if(!strcasecmp(p[i], "proc")){
#if 1
			return 	m_iss.local_TestResource(res,&p[i]);
#else
			assert(false);
#endif
		}
		std::cerr << name() << " No TestResource implemented" << std::endl;
		return -1;
	}

	tmpl(int)::Resource(char **args)
	{
		size_t  i = 1;

		static char *CACHERessources[] = {
			(char *)"p\t[lin]\tdcache\t\t: Dumps whole D cache",
			(char *)"p\t[str]\tdcache [n]\t: Prints cached value at address n",
			(char *)"p\t[int]\tdcache [n1] [n2]\t: Dumps n2 Dcache lines from line n1",
			(char *)"p\t[lin]\ticache\t\t: Dumps whole I cache",
			(char *)"p\t[str]\ticache [n]\t: Prints cached value at address n",
			(char *)"p\t[int]\ticache [n1] [n2]\t: Dumps n2 Icache lines from line n1",
		};

		if (*args[i] != (char)0) {
			std::cerr << name() << " Ressources CACHE :  junk at end of command!" << std::endl;
			return 1;
		}
		for (i = 0; i < sizeof(CACHERessources)/sizeof(*CACHERessources); i++)
			std::cout << CACHERessources[i] << std::endl;
#if 1
		m_iss.local_Resource(args);
#else
		assert(false);
#endif
		return 0;
	}
	}}
#endif
