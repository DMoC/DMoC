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
 *         Perre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr> 2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

#ifndef VCI_MCC_RAM_INTERFACE_H
#define VCI_MCC_RAM_INTERFACE_H
#include "soclib_directory.h"
namespace soclib{
namespace caba{
using namespace sc_core;

class VciMccRam_interface {
	// Method accessed by module_6, declared in interface to avoir recursive inclusion
	public :
	virtual bool	is_access_sram_possible(void) = 0;
	virtual unsigned int read_seg(unsigned int page_index,unsigned int word_page_offset) = 0;
	virtual void write_seg(unsigned int page_index,unsigned int word_page_offset, unsigned int wdata) = 0;
	virtual bool	is_access_dir_possible(void) = 0;
	virtual SOCLIB_DIRECTORY* read_dir(unsigned int page_index,unsigned int line_page_offset) = 0;
	virtual void write_dir(unsigned int page_index,unsigned int line_page_offset, SOCLIB_DIRECTORY  * wdata) = 0;
	virtual ~VciMccRam_interface(){};

};  

}}
#endif


