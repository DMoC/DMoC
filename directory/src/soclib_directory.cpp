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
#include "soclib_directory.h"
#include <cassert>

using namespace std;

SOCLIB_DIRECTORY::SOCLIB_DIRECTORY(uint32_t nb_procs): NB_P(nb_procs){
	dir_slice = new uint32_t[((nb_procs+1)>>5)+1]; //Directory allocation
	NB_SLICES = ((nb_procs+1)>>5)+1;
	for (uint32_t i=0; i < NB_SLICES; i++){ dir_slice[i]=0;}
}

SOCLIB_DIRECTORY::SOCLIB_DIRECTORY(SOCLIB_DIRECTORY& dir){
	NB_SLICES = dir.Get_nb_slices();
	NB_P = dir.Get_nb_p();
	dir_slice = new uint32_t[NB_SLICES];
	for (uint32_t i=0; i < NB_SLICES; i++){ dir_slice[i]=dir.dir_slice[i];}
}

SOCLIB_DIRECTORY::SOCLIB_DIRECTORY(void){
	NB_SLICES = 0;
	NB_P = 0;
	dir_slice = NULL;
}

void SOCLIB_DIRECTORY::operator=(SOCLIB_DIRECTORY& dir){
	assert(this != &dir);
	assert(dir_slice != NULL);
	assert(this->NB_P == dir.Get_nb_p());
	for (uint32_t i=0; i < NB_SLICES; i++){ dir_slice[i]=dir.dir_slice[i];}
}


void SOCLIB_DIRECTORY::init(uint32_t nb_procs){
	assert(dir_slice==NULL);
	dir_slice = new uint32_t[((nb_procs+1)>>5)+1]; //Directory allocation
	NB_SLICES = ((nb_procs+1)>>5)+1;
	NB_P=nb_procs;
	for (uint32_t i=0; i < NB_SLICES; i++){ dir_slice[i]=0;}
}

bool SOCLIB_DIRECTORY::Is_M(void){
	assert(dir_slice != NULL);
	return ((dir_slice[NB_SLICES-1] & 0x80000000) != 0);
}

bool SOCLIB_DIRECTORY::Is_E(void){
	return ((dir_slice[NB_SLICES-1] & 0x40000000) != 0);
}


bool SOCLIB_DIRECTORY::Is_p(uint32_t x){
	assert(dir_slice != NULL);
#ifdef DEBUG_DIR
	if ((x>=0) && (x < (NB_SLICES<<5)-1)){
		return ((dir_slice[(((uint32_t)(x))>>5)] & (0x1 << (x%32))) !=0);
	} else {
		cout << "Erreur soclib_directory" << endl;
		cout << "Is_p , index out of range : "<< x << endl;
		exit(1);
	}
#else
	return ((dir_slice[(((uint32_t)(x))>>5)] & (0x1 << (x%32))) !=0);
#endif
}

bool SOCLIB_DIRECTORY::Is_empty(void){
	uint32_t acc = 0;
	assert(dir_slice != NULL);
	for (uint32_t i=0; i<NB_SLICES; i++){
		acc |= dir_slice[i];
	}
	return (acc == 0);
}

void SOCLIB_DIRECTORY::Set_M(void){
	assert(dir_slice != NULL);
	dir_slice[NB_SLICES-1] |= 0x80000000;
}

void SOCLIB_DIRECTORY::Set_E(void){
	assert(dir_slice != NULL);
	dir_slice[NB_SLICES-1] |= 0x40000000;
}


void SOCLIB_DIRECTORY::Set_p(uint32_t x){
	assert(dir_slice != NULL);
#ifdef DEBUG_DIR
	if ((x>=0) && (x < (NB_SLICES<<5)-1) && (x < NB_P)){
		dir_slice[(((uint32_t)(x))>>5)] |= (0x1 << (x%32));
	} else {
		cout << "Erreur soclib_directory" << endl;
		cout << "Set_p , index out of range : "<< x << endl;
		exit(1);
	}
#else
	dir_slice[(((uint32_t)(x))>>5)] |= (0x1 << (x%32));
#endif
}

void SOCLIB_DIRECTORY::Reset_M(void){
	assert(dir_slice != NULL);
	dir_slice[NB_SLICES-1] &= 0x7fffffff;
}
void SOCLIB_DIRECTORY::Reset_E(void){
	assert(dir_slice != NULL);
	dir_slice[NB_SLICES-1] &= 0xBfffffff;
}
void SOCLIB_DIRECTORY::Reset_p(uint32_t x){
	assert(dir_slice != NULL);
#ifdef DEBUG_DIR
	if ((x>=0) && (x < (NB_SLICES<<5)-1) && (x < NB_P)){
		dir_slice[(((uint32_t)(x))>>5)] &= ~(0x1 << (x%32));
	} else {
		cout << "Erreur soclib_directory" << endl;
		cout << "Reset_p , index out of range : "<< x << endl;
		exit(1);
	}

#else
	dir_slice[(((uint32_t)(x))>>5)] &= ~(0x1 << (x%32));
#endif
}

void SOCLIB_DIRECTORY::Reset_all(void){
	assert(dir_slice != NULL);
	for (uint32_t i=0; i<NB_SLICES; i++){
		dir_slice[i] = 0;
	}	
}

uint32_t SOCLIB_DIRECTORY::Get_nb_slices(void){
	assert(dir_slice != NULL);
	return NB_SLICES;
}
uint32_t SOCLIB_DIRECTORY::Get_nb_p(void){
	assert(dir_slice != NULL);
	return NB_P;
}

int32_t SOCLIB_DIRECTORY::Get_next_id(void){
	for (uint32_t i = 0; i < NB_P ; i++)
	{
		if (Is_p(i)) return i;
	}
	return -1;
}

uint32_t SOCLIB_DIRECTORY::Get_slice(uint32_t i){
	assert(dir_slice != NULL);
#ifdef DEBUG_DIR
	if ((i < NB_SLICES) && (i>=0)) {
		// 	cout << std::hex << "GET SLICE" << dir_slice[i] << endl;
		return dir_slice[i]; }
	else {
		cout << "Erreur soclib_directory" << endl;
		cout << "Get_slice , index out of range : "<< i << endl;
		exit(1);
	}
#else
	return dir_slice[i];
#endif
}

void SOCLIB_DIRECTORY::Set_slice(uint32_t i, uint32_t value){
	assert(dir_slice != NULL);
#ifdef DEBUG_DIR
	if ((i < NB_SLICES) && (i>=0)) {
		// 	cout << std::hex <<"SET SLICE" << value << endl;
		dir_slice[i] = value; }
	else {
		cout << "Erreur soclib_directory" << endl;
		cout << "Set_slice , index out of range : "<< i << endl;
		exit(1);
	}
#else
	dir_slice[i] = value;
#endif
}

bool SOCLIB_DIRECTORY::Is_Other(uint32_t x){
	uint32_t  acc =0;
	assert(dir_slice != NULL);
#ifdef DEBUG_DIR
	if ((x>=0) && (x < (NB_SLICES<<5)-1) ){
		for (uint32_t j =0; j < (uint32_t)NB_SLICES; j++){
			if (j==(uint32_t)(NB_SLICES-1)){
				if(j == (((uint32_t)(x))>>5)){
					acc |= (0x3fffffff & (dir_slice[j] & ~(0x1<<(x%32))));
				} else {
					acc |= (0x3fffffff & dir_slice[j]);
				}
			} else if (j == (((uint32_t)(x))>>5)) {
				acc |= (dir_slice[j] & ~(0x1<<(x%32)));

			} else { acc |= dir_slice[j];}
		}
		return (acc != 0);
	} else {
		cout << "Erreur soclib_directory" << endl;
		cout << "Is_Other , index out of range : "<< x << endl;
		exit(1);
	}
#else
	for (uint32_t j =0; j < (uint32_t)NB_SLICES; j++){
		if (j==(uint32_t)(NB_SLICES-1)){
			if(j == (((uint32_t)(x))>>5)){
				acc |= (0x3fffffff & (dir_slice[j] & ~(0x1<<(x%32))));
			} else {
				acc |= (0x3fffffff & dir_slice[j]);
			}
		} else if (j == (((uint32_t)(x))>>5)) {
			acc |= (dir_slice[j] & ~(0x1<<(x%32)));

		} else { acc |= dir_slice[j];}
	}
	return (acc != 0);
#endif
}



void SOCLIB_DIRECTORY::Print(void){
	assert(dir_slice != NULL);
	cout << "directory  : ";
	for (int i=(NB_SLICES-1); i>=0; i--){
		cout << std::hex << (void *)dir_slice[i] ;
	}
	cout << std::endl;
}


SOCLIB_DIRECTORY::~SOCLIB_DIRECTORY(void){
	assert(dir_slice != NULL);
	delete dir_slice;
} 
