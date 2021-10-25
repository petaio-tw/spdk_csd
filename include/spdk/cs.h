/**
 * @file cs.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2021-10-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef CS_H
#define CS_H
/************************************************************/
/*                                                          */
/* INCLUDE FILE DECLARATIONS                                */
/*                                                          */
/************************************************************/
#include <stdint.h>

/************************************************************/
/*                                                          */
/* NAMING CONSTANT DECLARATIONS                             */ 
/* {Constants defines for other components reference.}      */
/*                                                          */
/************************************************************/ 

/************************************************************/
/*                                                          */
/* MACRO FUNCTION DECLARATIONS                              */          
/* {MACRO functions defines for other components reference.}*/
/*                                                          */ 
/************************************************************/

/************************************************************/
/*                                                          */
/* DATA TYPE DECLARATIONS                                   */ 
/* {DATA TYPE defines for other components reference.}      */
/*                                                          */
/************************************************************/
//-----------------------------------
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint32_t CS_DEV_HANDLE;
typedef uint32_t CS_CSE_HANDLE;
typedef uint32_t CS_FUNCTION_ID;
typedef uint32_t CS_MEM_HANDLE;
typedef uint32_t CS_STREAM_HANDLE;
typedef uint32_t CS_EVT_HANDLE;

typedef void* CS_MEM_PTR;

//-----------------------------------
typedef enum {
	CS_SUCCESS 			= 0,
	CS_NOT_ENOUGH_MEMORY,
	CS_OUT_OF_RESOURCES
} CS_STATUS;

typedef enum { 
	CS_AFDM_TYPE = 1, 
	CS_32BIT_VALUE_TYPE = 2, 
	CS_64BIT_VALUE_TYPE = 3, 
	CS_STREAM_TYPE = 4, 
	CS_DESCRIPTOR_TYPE = 5 
} CS_COMPUTE_ARG_TYPE; 

//----------------------------------
typedef void(*csQueueCallbackFn)(void *QueueContext, CS_STATUS Status);

//----------------------------------
typedef struct { 
	CS_MEM_HANDLE MemHandle;     	// an opaque memory handle for AFDM 
	unsigned long ByteOffset;     	// denotes the offset with AFDM 
} CsDevAFDM;

typedef struct { 
	CS_COMPUTE_ARG_TYPE Type; 
	union { 
		CsDevAFDM DevMem;      // see 6.3.4.2.1 
		u64 Value64; 
		u32 Value32; 
		CS_STREAM_HANDLE StreamHandle; 
	} u; 
} CsComputeArg; 

typedef struct { 
	CS_CSE_HANDLE CSEHandle; 
	CS_FUNCTION_ID FunctionId; 
	int NumArgs;      		// set to total arguments to CSF 
	CsComputeArg Args[1];   	// see 6.3.4.2.6 
	// allocate enough space past this for multiple arguments
} CsComputeRequest;
/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
CS_STATUS csGetCSxFromPath(char *Path, unsigned int *Length, char *DevName);
CS_STATUS csQueryCSEList(char *FunctionName, int *Length, char *Buffer);
CS_STATUS csQueueComputeRequest(
	CsComputeRequest 	*Req,
	void 			*Context,
	csQueueCallbackFn 	CallbackFn,
	CS_EVT_HANDLE 		EventHandle,
	u32 			*CompValue);
CS_STATUS csAllocMem(
	CS_DEV_HANDLE 	DevHandle,
	int 		Bytes, 
	unsigned int 	MemFlags,
	CS_MEM_HANDLE 	*MemHandle, 
	CS_MEM_PTR 	*VAddressPtr);
void csHelperSetComputeArg(CsComputeArg *ArgPtr, CS_COMPUTE_ARG_TYPE Type,...);
/* End of CS_H */
#endif
