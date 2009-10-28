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
 *         Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>
 *
 */

#ifndef SOC_DIR
#define SOC_DIR

#include <systemc.h>
#include <iostream>



struct SOCLIB_DIRECTORY{

public : 

uint32_t Get_nb_slices(void);
uint32_t Get_nb_p(void);
int32_t Get_next_id(void);

bool Is_M(void);
bool Is_p(uint32_t x);
bool Is_empty(void);
void Set_M(void);
void Set_p(uint32_t x);
void Reset_M(void);
void Reset_p(uint32_t x);
void Reset_all(void);
void Print(void);
uint32_t Get_slice(uint32_t i);
void Set_slice(uint32_t i, uint32_t value);
bool Is_Other(uint32_t x);
bool Is_E(void);
void Set_E(void);
void Reset_E(void);
void init(uint32_t nb_procs);
void operator=(SOCLIB_DIRECTORY& dir);
SOCLIB_DIRECTORY(SOCLIB_DIRECTORY& dir);
SOCLIB_DIRECTORY(uint32_t nb_procs);
SOCLIB_DIRECTORY(void);
~SOCLIB_DIRECTORY(void);

private :
uint32_t NB_P;
uint32_t* dir_slice;
uint32_t NB_SLICES; 
};
#endif
