
/******************************************************************************
File                    : Vtype.h
Module                  : SYSTEM
Description             : Contains All the Videonetics Type
******************************************************************************/

#ifndef _V_TYPE_H_
#define _V_TYPE_H_

#define DSP_PLATFORM 0

typedef enum _VAEnableType_ {
	VA_ENABLE = 0,
	VA_DISABLE
}VAEnableType;

typedef char                    Char8;
typedef unsigned char           UChar8;
typedef short                   Int16;
typedef unsigned short          UInt16;
typedef signed int              Int32;
typedef unsigned int            UInt32;
typedef int				        Long32;
typedef unsigned long          	ULong32;
typedef long			        Long64;
typedef unsigned long          	ULong64;

#if DSP_PLATFORM
typedef double                  Int64;
#else
typedef long long               Int64;
#endif

typedef unsigned long long      UInt64;
typedef float                   Float32;
typedef double                  Float64;

typedef unsigned char           Bool;

//Pointers
typedef Char8 *                 PChar8;
typedef UInt16					VChannelIdType;
typedef Int16					VIndexType;

#ifndef NULL
#define NULL 0
#endif

#ifdef _WIN32
//#define strcmpi _strnicmp
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define chdir _chdir

#define V2A_CDECL __cdecl
#define V2A_STDCALL __stdcall
#ifndef INFINITE
#define V2A_INFINITE 0xFFFFFFFF
#else
#define V2A_INFINITE INFINITE
#endif
#else
//typedef int SOCKET;
#define V2A_CDECL
#define V2A_STDCALL
#define V2A_INFINITE 0xFFFFFFFF

#endif	/* _WIN32 */
#endif /* _V_TYPE_H_ */
