
/*
	Copyright (C) Peta IO
	File	: common_type.h
	Author	: Jongman Yoon
	date	: 2/25/2017
*/


#ifndef COMMON_TYPE_H
#define COMMON_TYPE_H

typedef signed char			INT8;
typedef unsigned char			UINT8;
typedef signed short			INT16;
typedef unsigned short			UINT16;
typedef signed int			INT32;
typedef unsigned int			UINT32;
typedef signed long long		INT64;
typedef unsigned long long		UINT64;

typedef UINT8				BOOL8;
typedef UINT16				BOOL16;
typedef UINT32				BOOL32;
typedef UINT32				BOOL;

typedef union _UINT32_T
{
	UINT32	dw;
	UINT16	w[2];
	UINT8	b[4];
} UINT32_T;

typedef union _UINT16_T
{
	UINT16	w;
	UINT8	b[2];
} UINT16_T;

// This structure is for little endian.
typedef union _UINT64_T
{
	UINT64	ddw;
	UINT32	dw[2];
	UINT16	w[4];
	UINT8	b[8];
	struct
	{
		UINT32	low;
		UINT32	high;
	};
} UINT64_T;

typedef union _UINT128_T
{
	UINT64  qw[2];
	UINT32  dw[4];
	UINT16  w[8];
	UINT8   b[16];

} UINT128_T;

#define FF8						((UINT8)0xFF)
#define FF16					((UINT16)0xFFFF)
#define FF32					(0xFFFFFFFFUL)
#define FF64					(0xFFFFFFFFFFFFFFFFULL)

#define INVALID8				((UINT8)0xFF)
#define INVALID16				((UINT16)0xFFFF)
#define INVALID32				(0xFFFFFFFFUL)
#define INVALID64				(0xFFFFFFFFFFFFFFFFULL)

#ifdef NULL
#undef NULL
#endif
#define NULL					(0)

#ifndef TRUE
#define TRUE					(1)
#endif

#ifndef FALSE
#define FALSE					(0)
#endif

#define SUCCESS					(0)
#define FAIL					(1)

#define BITS_FOR_UINT8			(8)
#define BITS_FOR_UINT16			(16)
#define BITS_FOR_UINT32			(32)

#define GiB_f					(1073741824.0)
#define GB_f					(1000000000.0)

#define KBYTE					(0x1ULL << 10)
#define MBYTE					(0x1ULL << 20)
#define GBYTE					(0x1ULL << 30)
#define TBYTE					(0x1ULL << 40)

#define KHZ						(1000ULL)
#define MHZ						(1000ULL * 1000ULL)
#define GHZ						(1000ULL * 1000ULL * 1000ULL)

#endif  // COMMON_TYPE_H
