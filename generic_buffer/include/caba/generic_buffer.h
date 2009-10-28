
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
 * Copyright (c) UPMC, Lip6, SoC, TIMA
 * 	   Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2006-2008
 *         Alain Greiner <alain.greiner@lip6.fr>, 2003
 *
 * Maintainers: Pierre Guironnet de Massas
 */

//////////////////////////////////////////////////////////////////////////////////
// Description:
// 	08/04/09
// 	This component implements a write buffer. This buffer is not just à simple
// 	fifo, but, it can be walked whitout getting out the data. 
////////////////////////////////////////////////////////////////////////////////////
#ifndef GENERIC_BUFFER_H
#define GENERIC_BUFFER_H

#include <systemc>
#include "register.h"

namespace soclib {
namespace caba   {

using namespace sc_core;

	template<typename T>
	class GenericBuffer
	{
		T *m_data;

		sc_signal<int>	r_ptr;
		sc_signal<int>	r_ptw;
		sc_signal<int>	r_fill_state;

		int m_depth;
		public:
		typedef T data_t;

		size_t size() const
		{
			return m_depth;
		}


		void init() 
		{
			r_ptr = 0;
			r_ptw = 0;
			r_fill_state = 0;
		}

		inline uint32_t filled_status() const
		{
			// Différence entre le pointeur d'écriture, et le pointeur de lecture
			return (uint32_t)r_fill_state;
		}

		inline bool full() const
		{
			// Plein lorsque le pointeur d'écriture arrive au bout
			return r_ptw == m_depth;
		}

			// Ajout simple, on décalse si possible le pointeur d'écriture
		void simple_put(const T &din)
		{
			if (r_ptw != m_depth) {
				r_fill_state = r_fill_state + 1;
				r_ptw +=  1;
				m_data[r_ptw] = din;
			}
		}

		void simple_get()
		{
			if (r_fill_state != 0) {
				r_fill_state = r_fill_state - 1;
				r_ptr += 1;
			}
		}

		void init_read()
		{
			r_ptr = 0;
			r_fill_state = r_ptw;
		}


		inline bool rok() const
		{
			return (r_fill_state != 0);
		}

		inline bool wok() const
		{
			return (r_ptw != m_depth);
		}

		inline const T &read() const
		{
			return m_data[r_ptr];
		}

		GenericBuffer(const std::string &name, size_t depth)
			: m_data(new T[depth]),
			r_ptr((name+"_r_ptr").c_str()),
			r_ptw((name+"_r_ptw").c_str()),
			r_fill_state((name+"_r_fill_state").c_str()),
			m_depth(depth)
		{
		}

		~GenericBuffer()
		{
			delete [] m_data;
		}
	};
}}
#endif
