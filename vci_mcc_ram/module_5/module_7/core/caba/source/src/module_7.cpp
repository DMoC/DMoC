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
 * License along with SoCLib; if not, write to the Free Software
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
#include "module_7.h"
#include "arithmetics.h"
#include "globals.h"

namespace soclib {
namespace caba {

using namespace soclib;
using namespace std;
using soclib::common::uint32_log2;

Module_7::Module_7(const string & insname, Module_9 * link):
	m_name(insname),
	m_linked_counter_set(link)
	{
		CTOR_OUT
	}
void Module_7::reset(){
	r_locked = false;
}

unsigned int Module_7::read_reg(unsigned int addr){
	return m_linked_counter_set->read_reg(addr);
} 
unsigned int Module_7::reset_reg(unsigned int addr){
	return m_linked_counter_set->reset_reg(addr);
} 
void Module_7::compute(){

// forwarding with no delay the request to module_10
// Todo : add an addr signal, and process some requests localy
// without any intervention from Module_10
//
	*(p_out.req) = *(p_in.req); 
	*(p_out.cmd) = *(p_in.cmd); 
}
void Module_7::gen_outputs(){
		*(p_in.rsp) = *(p_out.rsp);
}

const string & Module_7::name(){
	return this->m_name;
}
Module_7::~Module_7()
{
	DTOR_OUT
}

}}
