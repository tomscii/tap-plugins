/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef TAP_PLATFORM_H
#define TAP_PLATFORM_H

#ifdef _MSC_VER
#include <Windows.h>
#endif

#ifdef _MSC_VER
#define __CONSTRUCTOR
#else
#define __CONSTRUCTOR __attribute__((constructor))
#endif

#ifdef _MSC_VER
#define __DESTRUCTOR
#else
#define __DESTRUCTOR __attribute__((destructor))
#endif

#ifdef _MSC_VER
#define __INIT_FINI(initfn, finifn)                                                    \
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)   \
{                                                                                      \
    switch (ul_reason_for_call)                                                        \
    {                                                                                  \
    case DLL_PROCESS_ATTACH:                                                           \
        initfn();                                                                      \
        break;                                                                         \
    case DLL_PROCESS_DETACH:                                                           \
        finifn();                                                                      \
        break;                                                                         \
    }                                                                                  \
    return TRUE;                                                                       \
}
#else
#define __INIT_FINI(init, fini)
#endif

#endif