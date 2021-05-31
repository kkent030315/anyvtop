/*

	MIT License

	Copyright (c) 2021 Kento Oki

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

*/

#pragma once
#include <windows.h>

#include "libanycall.h"

namespace kernel
{
	//
	// this pointer holds ntoskrnl's exported memcpy
	// not rva, absolute address
	//
	inline void* ntoskrnl_memcpy = {};

	//
	// memcpy of kernel virtual memory
	// invoke memcpy inside ntoskrnl
	//
	void memcpy( void* dst, void* src, size_t size )
	{
		if ( !ntoskrnl_memcpy )
			ntoskrnl_memcpy = ( void* )
			libanycall::find_ntoskrnl_export( "memcpy" );

		libanycall::invoke_void<decltype( &memcpy )>
			( ntoskrnl_memcpy, dst, src, size );
	}

	//
	// read physical memory directly
	//
	void read_physical_memory( 
		void* buffer, uint64_t physical_address, size_t size )
	{
		//
		// map specified physical memory to virtual address
		//
		const auto mapped_va = 
			libanycall::map_physical_memory( physical_address, size );

		//
		// copy to buffer
		//
		memcpy( buffer, ( void* )mapped_va, size );

		//
		// unmap
		//
		libanycall::unmap_physical_memory( mapped_va, size );
	}

	template<class T>
	T read_physical_memory( uint64_t physical_address )
	{
		T buffer;

		read_physical_memory( &buffer, physical_address, sizeof( T ) );

		return buffer;
	}
}