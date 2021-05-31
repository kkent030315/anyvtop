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

#include <windows.h>
#include <iostream>

#include "vtop.hpp"
#include "console.hpp"

int main( int argc, const char** argv, const char** envp )
{
    console::enable_ansi_escape();

    SetConsoleTitleA( "anyvtop by Kento Oki at www.godeye.club" );

    if ( !libanycall::init( "ntdll.dll", "NtTraceControl" ) )
    {
        printf( "[!] failed to init libanycall\n" );
        return EXIT_FAILURE;
    }

    printf( "\n[=] anyvtop by Kento Oki at www.godeye.club\n\n" );

    uint64_t i_am_virtual_address = 0xABCDEF;
    VIRTUAL_ADDRESS virtual_address;
    virtual_address.value = &i_am_virtual_address;

    //const auto nt_close = libanycall::find_ntoskrnl_export( "NtClose\n" );
    //VIRTUAL_ADDRESS virtual_address = { ( void* )nt_close };

    const PHYSICAL_ADDRESS mm_pa =
        ANYCALL_INVOKE( MmGetPhysicalAddress, ( PVOID )virtual_address.value );

    uint64_t physical_address = 
        vtop::vtop( virtual_address, GetCurrentProcessId() );

    const bool is_correct = 
        physical_address == mm_pa.QuadPart;

    printf( "[+]                 vtop : 0x%llX \n", physical_address );
    printf( "[+] MmGetPhysicalAddress : 0x%llX \n", mm_pa.QuadPart );
    printf( is_correct ? "[+] correct!\n" : "[+] invalid\n" );

    std::cin.ignore();
    return EXIT_SUCCESS;
}