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

#ifndef SRAM_H
#define SRAM_H
#define NOCTRL

#include <inttypes.h>
#include <systemc>
#include <cassert>
#include <fstream>


#include "caba_base_module.h"

#include "sram_param.h"
#include <string>

#ifdef DEBUG_RAM
#define DRAMCOUT(x) if (x>= DEBUG_RAM_LEVEL) cout
#else
#define DRAMCOUT(x) if(0) cout
#endif

namespace soclib{
namespace caba{
	using namespace sc_core;

	////////////////////////////////////////        
	//    structure definition
	////////////////////////////////////////


	template <typename sram_param>
		class SRam : BaseModule {

			public :

				// IO PORTS
				sc_in<bool>   p_ce;
				sc_in<bool>   p_oe;
				sc_in<bool>   p_we;
				sc_in<typename sram_param::be_t>   p_be;
				sc_in<typename sram_param::addr_t>   p_addr;
				sc_in<typename sram_param::data_t>   p_din;
				sc_out<typename sram_param::data_t>   p_dout;
				sc_out<bool>   p_ack;


			
			private :
				// Data array
#define DEBUG_SR
#ifdef DEBUG_SR
				uint32_t	* s_RAM;     
#else
				typename  sram_param::data_t	* s_RAM;     
#endif


				unsigned int	m_ADDR_WORD_SHIFT;	// data_index = address >> m_ADDR_WORD_SHIFT (index of data in r_RAM) 
				unsigned int	m_size_bytes;	 			// size of this Sram bank


			public :
				SRam (
						sc_module_name insname,
						unsigned int size_bytes
						);													

				~SRam();  

				bool set_sram_content(void * source, unsigned int nb_bytes); // success returns true, error returns false

#ifdef CDB_COMPONENT_IF_H
				// CDB overloaded virtual method's
				const char* GetModel();
				int PrintResource(modelResource *res, char **p);
				int TestResource(modelResource *res, char **p);
				int Resource(char** args);
#endif

			protected:

				SC_HAS_PROCESS(SRam);

			private:

				void genMealy();

				typename sram_param::data_t read_write_sram(
									typename sram_param::addr_t offset,
									typename sram_param::data_t wdata,
									bool we, bool oe,
									typename sram_param::be_t be);

#ifdef CDB_COMPONENT_IF_H
				// CDB utilities method's
				void printramlines(struct modelResource *d);
				void printramdata(struct modelResource *d);
#endif
		};  
}}
#endif


