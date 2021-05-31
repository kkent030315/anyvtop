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
#include <iostream>

#include "libanycall.h"
#include "cpudef.hpp"
#include "nt.hpp"
#include "kernel.hpp"

template<typename T>
struct PAGE_ENTRY
{
	T* pointer;
	T value;
};

namespace vtop
{
	//
	// system process pid always 4
	//
	inline static constexpr uint32_t system_process_pid = 4;

	//
	// fetch directory table base (DTB)
	// in case of system process which KVA belongs, DTB represents cr3
	//
	ULONG fetch_dtb( uint32_t process_id = system_process_pid )
	{
		NTSTATUS nt_status = STATUS_SUCCESS;
		PEPROCESS eprocess;

		//
		// lookup the PEPROCESS pointer
		//
		nt_status =
			ANYCALL_INVOKE( PsLookupProcessByProcessId,
				( HANDLE )process_id, &eprocess );

		if ( !NT_SUCCESS( nt_status ) || !eprocess )
		{
			return 0;
		}

		printf( "[+] PEPROCESS of %d @ %p\n", process_id, eprocess );

		ULONG dtbase = 0;

		//
		// copy DirectoryTableBase to our buffer
		// 
		// dt nt!_KPROCESS DirectoryTableBase
		//   +0x028 DirectoryTableBase : Uint8B
		//
		kernel::memcpy( 
			&dtbase, 
			( void* )( ( uint64_t )eprocess + 0x28 ), 
			sizeof( ULONG ) );

		if ( !dtbase )
		{
			return 0;
		}

		printf( "[+] directory base: 0x%lx\n", dtbase );

		//
		// another method:
		//    pte.pfn << log2( PAGE_SIZE )
		//

		return dtbase;
	}

	//
	// translate (v)irtual address to (p)hysical address
	//
	uint64_t vtop( 
		VIRTUAL_ADDRESS virtual_address, 
		uint32_t process_id = system_process_pid ) // process id of the virtual address
	{
		printf( "[~] converting...\n" );
		printf( "[~] virtual address: 0x%llX\n", ( uint64_t )virtual_address.value );

		//
		// _KPROCESS.DirectoryTableBase, the DTB, contains value from __readcr3
		// but since we are on user-mode,
		// we can't make call to __readcr3 so fetch from _EPROCESS
		//
		const auto dtb = fetch_dtb( process_id );

		printf( "[+] directory table base: 0x%lX\n", dtb );

		PAGE_ENTRY<PML4E> pml4_entry;
		PAGE_ENTRY<PDPE> pdp_entry;
		PAGE_ENTRY<PDE> pd_entry;
		PAGE_ENTRY<PTE> pt_entry;

		//
		// 1. locate PML4 pointer that represents physical address of PML4 entry
		// 2. copy PML4 entry from physical address to our buffer
		//    in order to locate PDP entry in next
		//
		pml4_entry.pointer = ( PPML4E )dtb + virtual_address.pml4_index;
		pml4_entry.value = kernel::read_physical_memory<PML4E>( ( uint64_t )pml4_entry.pointer );
		
		if ( !pml4_entry.value.value )
		{
			printf( "[!] failed to fetch pml4e from physical memory\n" );
			return 0;
		}

		printf( "[+] \033[0;102;30mpml4 index\033[0m: %d\n", 
			static_cast<uint32_t>( virtual_address.pml4_index ) );
		printf( "[+] \033[0;102;30mpml4e\033[0m: 0x%p\n", pml4_entry.pointer );
		printf( "[+] \033[0;102;30mpml4e\033[0m: 0x%llX\n", pml4_entry.value.value );

		//
		// 1. locate PDP pointer that represents physical address of PDP entry
		// 2. copy PDP entry from physical address to our buffer
		//    in order to locate PD entry in next
		//
		pdp_entry.pointer = ( PPDPE )( pml4_entry.value.pfn << PAGE_SHIFT ) + virtual_address.pdp_index;
		pdp_entry.value = kernel::read_physical_memory<PDPE>( ( uint64_t )pdp_entry.pointer );

		if ( !pdp_entry.value.value )
		{
			printf( "[!] failed to fetch pdp_entry from physical memory\n" );
			return 0;
		}

		printf( "[+] \033[0;104;30mpdp index\033[0m: %d\n", 
			static_cast<uint32_t>( virtual_address.pdp_index ) );
		printf( "[+] \033[0;104;30mpdpe\033[0m: 0x%p\n", pdp_entry.pointer );
		printf( "[+] \033[0;104;30mpdpe\033[0m: 0x%llX\n", pdp_entry.value.value );

		//
		// 1. locate PD pointer that represents physical address of PD entry
		// 2. copy PD entry from physical address to our buffer
		//    in order to locate PT entry in next
		//
		pd_entry.pointer = ( PPDE )( pdp_entry.value.pfn << PAGE_SHIFT ) + virtual_address.pd_index;
		pd_entry.value = kernel::read_physical_memory<PDE>( ( uint64_t )pd_entry.pointer );

		if ( !pd_entry.value.value )
		{
			printf( "[!] failed to fetch pd_entry from physical memory\n" );
			return 0;
		}

		printf( "[+] \033[0;105;30mpd index\033[0m: %d\n", 
			static_cast<uint32_t>( virtual_address.pd_index ) );
		printf( "[+] \033[0;105;30mpde\033[0m: 0x%p\n", pd_entry.pointer );
		printf( "[+] \033[0;105;30mpde\033[0m: 0x%llX\n", pd_entry.value.value );

		//
		// 1. locate PT pointer that represents physical address of PT entry
		// 2. copy PT entry from physical address to our buffer
		//    in order to translate to the physical address
		//
		pt_entry.pointer = ( PPTE )( pd_entry.value.pfn << PAGE_SHIFT ) + virtual_address.pt_index;
		pt_entry.value = kernel::read_physical_memory<PTE>( ( uint64_t )pt_entry.pointer );

		if ( !pt_entry.value.value )
		{
			printf( "[!] failed to fetch pt_entry from physical memory\n" );
			return 0;
		}

		printf( "[+] \033[0;106;30mpt index\033[0m: %d\n", 
			static_cast<uint32_t>( virtual_address.pt_index ) );
		printf( "[+] \033[0;106;30mpte\033[0m: 0x%p\n", pt_entry.pointer );
		printf( "[+] \033[0;106;30mpte\033[0m: 0x%llX\n", pt_entry.value.value );

		//
		// translate to the physical address using PTE's PFN and VA's offset
		//
		const uint64_t physical_address =
			( pt_entry.value.pfn << PAGE_SHIFT ) + virtual_address.offset;

		printf( "[+] \033[0;103;30mphysical address\033[0m: 0x%llX\n", physical_address );

		printf( "[+] done!\n" );
		return physical_address;
	}
} // namespace vtop