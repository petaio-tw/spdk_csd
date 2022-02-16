/**
 * @file cs.h
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief CS_APIs define in SNIA spec.
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
#define MAX_CS_NAME_LENGTH	32

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
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t s32;

typedef uint32_t CS_FUNCTION_ID;
#if 0
typedef uint32_t CS_DEV_HANDLE;
typedef uint32_t CS_CSE_HANDLE;
typedef uint32_t CS_MEM_HANDLE;
typedef uint32_t CS_STREAM_HANDLE;
typedef uint32_t CS_EVT_HANDLE;
typedef uint32_t CS_BATCH_HANDLE;
#else
typedef void* CS_DEV_HANDLE;
typedef void* CS_CSE_HANDLE;
typedef void* CS_MEM_HANDLE;
typedef void* CS_STREAM_HANDLE;
typedef void* CS_EVT_HANDLE;
typedef void* CS_BATCH_HANDLE;
#endif

typedef void* CS_MEM_PTR;
typedef uint32_t CS_BATCH_INDEX;

//-----------------------------------
typedef enum {
	CS_SUCCESS = 0,			// The action was completed with success
	CS_COULD_NOT_MAP_MEMORY,	// The requested memory allocated could not be mapped
	CS_DEVICE_ERROR,		// The device is in error and is not able to make progress
	CS_DEVICE_NOT_AVAILABLE,	// The CSx is unavailable
	CS_DEVICE_NOT_READY,		// The device is not ready for any transactions
	CS_DEVICE_NOT_PRESENT,		// The requested device is not present
	CS_ENODEV,			// The device name specified does not exist
	CS_ENOENT,			// No such device, file or directory exists
	CS_ENTITY_NOT_ON_DEVICE,	// The entity does not exist on requested device
	CS_ENXIO,			// No Storage or CSE was available
	CS_ERROR_IN_EXECUTION,		// There was an error that occurred in the execution path
	CS_FATAL_ERROR,			// There was a fatal error that occurred
	CS_HANDLE_IN_USE,		// The requested handle is already in use
	CS_INVALID_HANDLE,		// An invalid handle was passed
	CS_INVALID_ARG,			// One or more invalid arguments were provided
	CS_INVALID_EVENT,		// The event specified was invalid
	CS_INVALID_ID,			// The specified input ID was invalid and does not exist
	CS_INVALID_LENGTH,		// The specified buffer is not of sufficient length
	CS_INVALID_OPTION,		// An invalid option was specified
	CS_INVALID_FUNCTION,		// The function specified was invalid 
	CS_INVALID_FUNCTION_NAME,	// The function name specified does not exist or is invalid
	CS_IO_TIMEOUT,			// An IO submitted has timed out
	CS_LOAD_ERROR,			// The specified download could not be initialized
	CS_MEMORY_IN_USE,		// The requested memory is still in use
	CS_NO_PERMISSIONS,		// There were insufficient permissions to proceed with request
	CS_NOT_DONE,			// The request is not done
	CS_NOT_ENOUGH_MEMORY,		// There is not enough memory to satisfy the request
	CS_NO_SUCH_ENTITY_EXISTS,	// There is no such entry that exists
	CS_OUT_OF_RESOURCES,		// The system is out of resources to satisfy the request
	CS_QUEUED,			// The request was successfully queued
	CS_UNKNOWN_MEMORY,		// The memory referenced was unknown
	CS_UNKNOWN_COMPUTE_FUNCTION,	// The function referenced is unknown 
	CS_UNSUPPORTED,			// The request is not supported
} CS_STATUS;

typedef enum { 
	CS_AFDM_TYPE = 1, 
	CS_32BIT_VALUE_TYPE = 2, 
	CS_64BIT_VALUE_TYPE = 3, 
	CS_STREAM_TYPE = 4, 
	CS_DESCRIPTOR_TYPE = 5 
} CS_COMPUTE_ARG_TYPE; 

typedef enum { 
	CS_STORAGE_BLOCK_IO = 1, 
  	CS_STORAGE_FILE_IO = 2 
} CS_STORAGE_REQ_MODE;

typedef enum { 
	CS_STORAGE_LOAD_TYPE = 1, 
	CS_STORAGE_STORE_TYPE = 2 
} CS_STORAGE_IO_TYPE; 

typedef enum { 
    	CS_COPY_TO_DEVICE = 1, 
    	CS_COPY_FROM_DEVICE = 2 
} CS_MEM_COPY_TYPE;

typedef enum { 
	CS_BATCH_SERIAL = 1, 
	CS_BATCH_PARALLEL = 2, 
	CS_BATCH_HYBRID = 3
} CS_BATCH_MODE;

typedef enum { 
	CS_COPY_AFDM = 1, 
	CS_STORAGE_IO = 2, 
	CS_QUEUE_COMPUTE = 3 
} CS_BATCH_REQ_TYPE;

typedef enum { 
	CS_STAT_CSE_USAGE = 1,  	// query to provide CSE runtime statistics 
	CS_STAT_CSx_MEM_USAGE = 2,  	// query CSx memory usage 
	CS_STAT_FUNCTION = 3    	// query statistics on a specific function 
} CS_STAT_TYPE;

typedef enum { 
	CS_CAPABILITY_CSx_TEMP = 1, 
	CS_CAPABILITY_CSx_MAX_IOS = 2 
	// TODO: define additional configuration options 
} CS_CAP_TYPE;

typedef enum  { 
	CS_FPGA_BITSTREAM = 1, 
	CS_BPF_PROGRAM = 2, 
	CS_CONTAINER_IMAGE = 3, 
	CS_OPERATING_SYSTEM_IMAGE = 4, 
	CS_LARGE_DATA_SET = 5 
} CS_DOWNLOAD_TYPE;

typedef enum { 
	CS_CSF_TYPE = 1, 
	CS_VENDOR_SPECIFIC = 2 
} CS_CONFIG_TYPE;

typedef enum { 
  	CS_STREAM_COMPUTE_TYPE = 1 
} CS_ALOC_STREAM_TYPE;

typedef enum { 
	CS_FILE_SYSTEMS_SUPPORTED = 1, 
	// TODO: 
	CS_RESERVED = 2 
} CS_LIBRARY_SUPPORT;

typedef enum { 
	CS_PLUGIN_COMPUTE = 1, 
	CS_PLUGIN_NVME = 2, 
	CS_PLUGIN_FILE_SYSTEM = 4, 
	CS_PLUGIN_CUSTOM = 8 
       	// TODO: 
} CS_PLUGIN_TYPE;
//----------------------------------
typedef void(*csQueueCallbackFn)(void *QueueContext, CS_STATUS Status);
typedef void(*csQueryPluginCallbackFn)(CS_PLUGIN_TYPE Type, char *Buffer);
typedef void(*csDevNotificationFn)( u32 Notification, void *Context, CS_STATUS Status, int Length, void *Buffer);

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

typedef struct {     
	CS_STORAGE_IO_TYPE Type;     	// see 6.3.4.1.5 
	u32 StorageIndex;       	// denotes the index in a CSA, zero otherwise 
	u32 NamespaceId;       		// represents a LUN or namespace  
	u64 StartLba; 
	u32 NumBlocks; 
	CsDevAFDM DevMem;      		// see 6.3.4.2.1 
} CsBlockIo; 

typedef struct {     
	CS_STORAGE_IO_TYPE Type;    	// see 6.3.4.1.5 
	void *FileHandle; 
	u64 Offset; 
	u32 Bytes; 
	CsDevAFDM DevMem;       	// see 6.3.4.2.1 
} CsFileIo;

typedef struct {     
	CS_STORAGE_REQ_MODE Mode;    	// see 6.3.4.1.4 
	CS_DEV_HANDLE DevHandle; 
	union { 
		CsBlockIo BlockIo;		// see 6.3.4.2.2   
		CsFileIo FileIo;  		// see 6.3.4.2.3   
	} u;   
} CsStorageRequest; 

typedef struct { 
	CS_MEM_COPY_TYPE Type;    	// see 6.3.4.1.3 
	void *HostVAddress; 
	CsDevAFDM DevMem;      		// see 6.3.4.2.1 
	unsigned int Bytes; 
} CsCopyMemRequest;

typedef struct { 
	CS_BATCH_REQ_TYPE ReqType;  	// see 6.3.4.1.8 
	u32 reqLength; 
	union { 
		CsCopyMemRequest CopyMem;  	// see 6.3.4.2.5 
		CsStorageRequest StorageIo;  	// see 6.3.4.2.4 
		CsComputeRequest Compute;  	// see 6.3.4.2.7 
	} u; 
} CsBatchRequest;

typedef struct { 
	CS_FUNCTION_ID FunctionId; 
	u8 NumUnits;  			//number of instances of this function available 
	char Name[MAX_CS_NAME_LENGTH]; 
} CsFunctionInfo;

typedef struct { 
	u16 HwVersion; 
	u16 SwVersion; 
	char UniqueName[32];    		// an identifiable string for this CSE 
	u16 NumBuiltinFunctions;  		// number of available preloaded functions 
	u32 MaxRequestsPerBatch;  		// maximum number of requests supported per batch request 
	u32 MaxFunctionParametersAllowed;  	// maximum number of parameters supported 
	u32 MaxConcurrentFunctionInstances;  	// maximum number of function instances supported 
} CSEProperties; 

typedef struct { 
	u16 HwVersion;     			// specifies the hardware version of this CSx 
	u16 SwVersion;     			// specifies the software version that runs on this CSx 
	u16 VendorId;      			// specifies the vendor id of this CSx 
	u16 DeviceId;      			// specifies the device id of this CSx 
	char FriendlyName[MAX_CS_NAME_LENGTH];  // an identifiable string for this CSx 
	u32 CFMinMB;       			// amount of CFM in megabytes installed in device 
	u32 FDMinMB;      			// amount of FDM in megabytes installed in device 
	struct { 
		u64 FDMIsDeviceManaged : 1;    	// FDM allocations managed by device 
		u64 FDMIsHostVisible : 1;    	// FDM may be mapped to host address space 
		u64 BatchRequestsSupported : 1; // CSx supports batch requests in hardware 
		u64 StreamsSupported : 1;    	// CSx supports streams in hardware 
		u64 Reserved : 60; 
	} Flags; 
	
	u16 NumCSEs; 
	CSEProperties CSE[1];	// see 6.3.4.1.14 
} CSxProperties;

typedef struct { 
	// specifies the fixed functionality device capability 
	struct {
		u64 Compression : 1; 
		u64 Decompression : 1; 
		u64 Encryption : 1; 
		u64 Decryption : 1; 
		u64 RAID : 1; 
		u64 EC : 1; 
		u64 Dedup : 1; 
		u64 Hash : 1; 
		u64 Checksum : 1; 
		u64 RegEx : 1; 
		u64 DbFilter : 1; 
		u64 ImageEncode : 1; 
		u64 VideoEncode : 1; 
		u64 CustomType : 48; 
	} Functions; 
} CsCapabilities; 

typedef struct { 
	u32 PowerOnMins; 
	u32 IdleTimeMins; 
	u64 TotalFunctionExecutions; 	// total number of executions performed by CSE 
} CSEUsage;

// FDM: Function Data Memory
// CSFM: Computational Storage Function Memory
typedef struct { 
	u64 TotalAllocatedFDM;  	// denotes the total FDM in bytes that have been allocated 
	u64 LargestBlockAvailableFDM; 	// denotes the largest amount of FDM that may be allocated 
	u64 AverageAllocatedSizeFDM; 	// denotes the average size of FDM allocations in bytes 
	u64 TotalFreeCSFM;    		// denotes the total CSFM memory that is not in use
	u64 TotalAllocationsFDM;  	// count of total number of FDM allocations 
	u64 TotalDeAllocationsFDM;  	// count of total number of FDM deallocations 
	u64 TotalFDMtoHostinMB;  	// total FDM transferred to host memory in megabytes 
	u64 TotalHosttoFDMinMB;  	// total host memory transferred to FDM in megabytes
	u64 TotalFDMtoStorageinMB;   	// total FDM transferred to storage in megabytes 
	u64 TotalStoragetoFDMinMB;   	// total storage transferred to FDM in megabytes 
} CSxMemoryUsage;

typedef struct { 
	u64 TotalUptimeSeconds;  	// total utilized time by function in seconds 
	u64 TotalExecutions;    	// number of executions performed 
	u64 ShortestTimeUsecs;  	// the shortest time the function ran in microseconds 
	u64 LongestTimeUsecs;   	// the longest time the function ran in microseconds 
	u64 AverageTimeUsecs;   	// the average runtime in microseconds 
} CSFUsage; 

typedef union { 
	CSEUsage CSEDetails; 
	CSxMemoryUsage MemoryDetails;  	// see 6.3.4.2.12 
	CSFUsage FunctionDetails;      	// see 6.3.4.2.13 
} CsStatsInfo;

typedef union { 
	// defines temperature details to set 
	struct { 
		s32 TemperatureLevel; 
	} CSxTemperature; 
	// defines CSx Max outstanding IOs allowed  
	struct { 
		u32 TotalOutstandingIOs; 
	} MaxIOs; 
} CsCapabilityInfo;

typedef struct { 
	CS_DOWNLOAD_TYPE Type; 
	int SubType;        	// type dependent 
	int Index;        	// program slot etc 
	int Unload;        	// unload previously loaded entity 
	int Length;        	// length in bytes of data in DataBuffer 
	void *DataBuffer;      	// download data for program 
} CsDownloadInfo;

typedef struct { 
	CS_CONFIG_TYPE Type; 
	int SubType;        	// type dependent 
	int Index;        	// program slot etc if applicable 
	int Length;        	// length in bytes of data in DataBuffer 
	void *DataBuffer;      	// configuration data to download 
} CsConfigInfo;

typedef struct { 
	CS_PLUGIN_TYPE Type; 	// see 6.3.4.1.13 
	u32 InterfaceLength; 
	u16 Id; 
	union {
#if 0		
		TypeA;  // TODO 
		TypeB; 
		TypeC; 
#endif		
	} Interface; 
} CsQueryPluginRequest;

typedef struct { 
	CS_PLUGIN_TYPE Type;	// see 6.3.4.1.13 
	u32 InterfaceLength; 
	u16 Id; 
	union {
#if 0		
		TypeA;  // TODO 
		TypeB; 
		TypeC;
#endif		
	} Interface; 
} CsPluginRequest; 
/************************************************************/
/*                                                          */ 
/* EXPORTED SUBPROGRAM SPECIFICATIONS                       */
/* {Function routine define for other components reference.}*/
/*                                                          */
/************************************************************/
//-----------------------------
// Device Discovery
//-----------------------------
/**
 * @brief This function returns one or more CSEs available based on the query criteria. 
 * Editor’s note: Need mechanism to associate CSE name with a CSx. Query CSEs 
 * within a CSx (once CSx is open) 
 * 
 * @param[in] FunctionName Name of computational storage function to query
 * @param[inout] Length Length in bytes of buffer passed for output
 * @param[out] Buffer Returns a list of CSENames
 * @return CS_STATUS
 */
CS_STATUS csQueryCSEList(char *FunctionName, int *Length, char *Buffer);

/**
 * @brief This function returns zero or more functions available based on the query criteria.
 * 
 * @param[in] Path A string that denotes a path to a file, directory that resides 
on a device or a device path or a CSE or a CSx. The file/directory may indirectly 
refer to a namespace and partition. 
 * @param[inout] Length Length of buffer passed for output
 * @param[out] Buffer Returns a list of comma separated function names in Path
 * @return CS_STATUS 
 */
CS_STATUS csQueryFunctionList(char *Path, int *Length, char *Buffer);

/**
 * @brief This function returns the CSx associated with the specified file or directory path.
 * 
 * @param[in] Path A string that denotes a path to a file, directory that resides 
on a device or a device path. The file/directory may indirectly refer to a namespace and partition. 
 * @param[inout] Length Length of buffer passed for output
 * @param[out] DevName Returns the qualified name to the CSx
 * @return CS_STATUS 
 */
CS_STATUS csGetCSxFromPath(char *Path, unsigned int *Length, char *DevName);

/**
 * @brief This function returns one or more CSE’s associated with the specified CSx.
 * 
 * @param[in] DevHandle A handle to the CSx to query
 * @param[inout] Length Length of buffer passed for output
 * @param[out] CSEName Returns the qualified name to CSE
 * @return CS_STATUS 
 */
CS_STATUS csGetCSEFromCSx(CS_DEV_HANDLE DevHandle, unsigned int *Length, char *CSEName);

//-----------------------------
// Device Access
//-----------------------------
/**
 * @brief Return a handle to the CSx associated with the specified device name.
 * 
 * @param[in] DevName A string that denotes the full name of the device
 * @param[in] DevContext A user specified context to associate with the device for 
future notifications
 * @param[out] DevHandle Returns the handle to the CSE device
 * @return CS_STATUS 
 */
CS_STATUS csOpenCSx(char *DevName, void *DevContext, CS_DEV_HANDLE *DevHandle); 

/**
 * @brief Close a CSx previously opened and associated with the specified handle.
 * 
 * @param[in] DevHandle Handle to CSx
 * @return CS_STATUS 
 */
CS_STATUS csCloseCSx(CS_DEV_HANDLE DevHandle);

/**
 * @brief Return a handle to the CSE associated with the specified device name.
 * 
 * @param[in] CSEName A string that denotes the full name of the CSE
 * @param[in] CSEContext A user specified context to associate with the CSE for 
future notifications
 * @param[out] CSEHandle Returns the handle to the CSE device
 * @return CS_STATUS 
 */
CS_STATUS csOpenCSE(char *CSEName, void *CSEContext, CS_CSE_HANDLE *CSEHandle); 

/**
 * @brief Close a CSE previously opened and associated with the specified handle.
 * 
 * @param[in] CSEHandle Handle to CSE
 * @return CS_STATUS 
 */
CS_STATUS csCloseCSE(CS_CSE_HANDLE CSEHandle);

/**
 * @brief Register a callback function to be notified based on various computational storage 
events across all CSx’s and CSEs. This is an optional function. 
 * 
 * @param[in] DevName A string that denotes a specific CSE or CSx to provide 
notifications for. If NULL, all CSEs and CSxes will be registered
 * @param[in] NotifyOptions Denotes the notification types to registered to
 * @param[in] NotifyFn A user specified callback notification function
 * @return CS_STATUS 
 */
CS_STATUS csRegisterNotify(char *DevName, u32 NotifyOptions, csDevNotificationFn NotifyFn);

/**
 * @brief Deregister a previously registered callback function for notifications on computational 
storage events. A callback function may have been previously registered using csRegisterNotify(). 
This is an optional function.
 * 
 * @param[in] DevName A string that denotes a specific CSE or CSx to deregister 
notifications from. If NULL, all CSEs and CSxes will be deregistered 
 * @param[in] NotifyFn The callback notification function previously registered
 * @return CS_STATUS 
 */
CS_STATUS csDeregisterNotify(char *DevName, csDevNotificationFn NotifyFn); 
//-----------------------------
// FDM management
//-----------------------------
/**
 * @brief Allocates memory from the FDM for the requested size in bytes. 
 * 
 * @param[in] DevHandle Handle to CSx
 * @param[in] Bytes Length in bytes of FDM to allocate
 * @param[in] MemFlags Reserved, shall be zero
 * @param[out] MemHandle Pointer to hold the memory handle once allocated
 * @param[out] VAddressPtr Pointer to hold the virtual address of device memory 
allocated in host system address space. This is optional and may be NULL if 
memory is not required to be mapped
 * @return CS_STATUS 
 */
CS_STATUS csAllocMem(CS_DEV_HANDLE DevHandle, int Bytes, unsigned int MemFlags, 
		     CS_MEM_HANDLE *MemHandle, CS_MEM_PTR *VAddressPtr); 

/**
 * @brief Frees AFDM for the memory handle specified.
 * 
 * @param[in] MemHandle Handle to AFDM
 * @return CS_STATUS 
 */
CS_STATUS csFreeMem(CS_MEM_HANDLE MemHandle);

//-----------------------------
// Storage IOs
//-----------------------------
/**
 * @brief Queues a storage IO request to the device.
 * 
 * @param[in] Req Structure to the storage request 
 * @param[in] Context A user specified context for the storage request when asynchronous. 
 The parameter is required only if CallbackFn or EventHandle is specified. 
 * @param[in] CallbackFn A callback function if the request needs to be asynchronous.
 * @param[in] EventHandle A handle to an event previously created using csCreateEvent().
This value may be NULL if CallbackFn parameter is specified to be a valid value or if 
the request is synchronous.
 * @param[out] CompValue Additional completion value provided as part of completion. 
This may be optional depending on the implementation.
 * @return CS_STATUS 
 */
CS_STATUS csQueueStorageRequest(CsStorageRequest *Req, void *Context, csQueueCallbackFn CallbackFn,
				CS_EVT_HANDLE EventHandle, u32 *CompValue); 
//-----------------------------
// CSE Data movement
//-----------------------------
/**
 * @brief Copies data between host memory and AFDM in the direction requested.
 * 
 * @param[in] CopyReq A request structure that describes the source and 
destination details of the copy request 
 * @param[in] Context A user specified context for the copy request when 
asynchronous. The parameter is required only if CallbackFn or EventHandle is specified.
 * @param[in] CallbackFn A callback function if the copy request needs to be asynchronous.
 * @param[in] EventHandle A handle to an event previously created using csCreateEvent().
This value may be NULL if CallbackFn parameter is specified to be valid value or if also set to 
NULL when the request needs to be synchronous. 
 * @param[out] CompValue Additional completion value provided as part of completion. 
This may be optional depending on the implementation. 
 * @return CS_STATUS 
 */
CS_STATUS csQueueCopyMemRequest(CsCopyMemRequest *CopyReq, void *Context, csQueueCallbackFn CallbackFn,
			  	CS_EVT_HANDLE EventHandle, u32 *CompValue); 

//-----------------------------
// CSE function and scheduling
//-----------------------------
/**
 * @brief Fetches the CSF specified for scheduling compute offload tasks.
 * 
 * @param[in] CSEHandle Handle to CSE
 * @param[in] FunctionName A pre-specified hardware function name
 * @param[in] Context A pointer to specify a context to the hardware function loaded
 * @param[out] FunctionId A pointer to hold the function id to the function requested if successful
 * @return CS_STATUS 
 */
CS_STATUS csGetFunction(CS_CSE_HANDLE CSEHandle, char *FunctionName, void *Context, CS_FUNCTION_ID *FunctionId);

/**
 * @brief Stops the CSF specified if it is running any tasks.
 * 
 * @param[in] FunctionId The function id to the CSF to stop
 * @return CS_STATUS 
 */
CS_STATUS csStopFunction(CS_FUNCTION_ID FunctionId);

/**
 * @brief Queues a compute offload request to the device to be executed synchronously or 
asynchronously in the device.
 * 
 * @param[in] Req A request structure that describes the CSE function and its 
arguments to queue.
 * @param[in] Context A user specified context for the queue request when 
asynchronous. The parameter is required only if CallbackFn or EventHandle is specified. 
 * @param[in] CallbackFn A callback function if the queue request needs to be 
asynchronous. 
 * @param[in] EventHandle A handle to an event previously created using  csCreateEvent(). 
This value may be NULL if  CallbackFn parameter is specified to be valid value or 
if also set to NULL when the request needs to be synchronous.
 * @param[out] CompValue Additional completion value provided as part of completion. 
This may be optional depending on the implementation. 
 * @return CS_STATUS 
 */
CS_STATUS csQueueComputeRequest(CsComputeRequest *Req, void *Context, csQueueCallbackFn CallbackFn,
			        CS_EVT_HANDLE EventHandle, u32 *CompValue); 

/**
 * @brief Helper function that are able to optionally be used to set an argument for a compute 
request.
 * 
 * @param[in] ArgPtr A pointer to the argument in CsComputeRequest to be set. 
 * @param[in] Type The argument type to set. This may be one of the enum values. 
 * @param[in] ... One or more variables that make up the argument by type. 
 */
void csHelperSetComputeArg(CsComputeArg *ArgPtr, CS_COMPUTE_ARG_TYPE Type,...);

/**
 * @brief Allocates a batch handle that may be used to submit batch requests. The handle 
resource may be set up with the individual requests that need to be batch processed. 
The allocation may be requested for serial, parallel or hybrid batched request flows that 
support storage, compute and data copy requests all in one function.
 * 
 * @param[in] Mode The requested batch mode namely, serial, parallel or hybrid. 
 * @param[in] MaxReqs The maximum number of requests the caller perceives added to
this batch resource. This parameter provides a hint to the sub-system for resource management. 
 * @param[out] BatchHandle The created handle for batch request processing if successful. 
 * @return CS_STATUS 
 */
CS_STATUS csAllocBatchRequest(CS_BATCH_MODE Mode, int MaxReqs, CS_BATCH_HANDLE *BatchHandle);

/**
 * @brief Frees a batch handle previously allocated with a call to csAllocBatchRequest().
 * 
 * @param[in] BatchHandle The handle previously allocated for batch requests.
 * @return CS_STATUS 
 */
CS_STATUS csFreeBatchRequest(CS_BATCH_HANDLE BatchHandle);

/**
 * @brief Add a request to the batch request resource represented by the input handle. The 
request type is: storage, compute, or copy memory. Additionally, the batch index 
parameters places the request at the required point in the list of requests.
 * 
 * @param[in] BatchHandle The batch request handle that describes the CSx batch 
items that may contain more than one CSx based work items that may include
storage requests, compute hardware functions and device memory copy requests. 
 * @param[in] Req The request to add to the batch of requests represented by 
BatchHandle parameter. Denotes a compound request structure that describes 
the CSx batch items that contain the CSx based work item that may include 
storage request, compute hardware functions or compute memory copy requests. 
 * @param[in] Before A batch entry index that denotes the position of an existing 
request entry that the current request will be inserted in front of. A zero value
denotes the current request must be the first request. Any other non-zero value 
must represent a valid entry returned back a previous call to this function. 
 * @param[in] After A batch entry index that denotes the position of an existing 
request entry that the current request will be inserted in after of. A zero value
denotes the current request must be the first request. Any other non-zero value
must represent a valid entry returned back a previous call to this function. 
 * @param[out] Curr A pointer to hold the output of the batch entry index for 
current request of successful. 
 * @return CS_STATUS 
 */
CS_STATUS csAddBatchEntry(CS_BATCH_HANDLE BatchHandle, CsBatchRequest *Req, CS_BATCH_INDEX Before, 
			  CS_BATCH_INDEX After, CS_BATCH_INDEX *Curr);

/**
 * @brief Helps reconfigure an existing batch request entry with new request information.
 * 
 * @param[in] BatchHandle The handle previously allocated for batch requests.
 * @param[in] Entry The request’s batch entry index that is reconfigured.
 * @param[in] Req The new batch request entry details. 
 * @return CS_STATUS 
 */
CS_STATUS csHelperReconfigureBatchEntry(CS_BATCH_HANDLE BatchHandle, CS_BATCH_INDEX Entry, CsBatchRequest *Req);

/**
 * @brief Resizes an existing batch request for the maximum number of requests that it is able to 
accommodate.
 * 
 * @param[in] BatchHandle The handle previously allocated for batch requests that is resized. 
 * @param[in] MaxReqs The maximum number of requests the caller perceives that this batch 
resource is resized to. The parameter may not exceed the maximum supported by the CSE. 
 * @return CS_STATUS 
 */
CS_STATUS csHelperResizeBatchRequest(CS_BATCH_HANDLE BatchHandle, int MaxReqs); 

/**
 * @brief Queues a data graph request to the device to be executed synchronously or 
asynchronously in the device. The request is able to support serial, parallel or a mixed 
variety of batched jobs defined by their data flow and support storage, compute and 
data copy requests all in one function. The handle must already have been populated 
with the list of batched requests.
 * 
 * @param[in] BatchHandle The handle previously allocated for batch requests. 
 * @param[in] Context A user specified context for the queue request when 
asynchronous. The parameter is required only if CallbackFn or EventHandle is specified.
 * @param[in] CallbackFn A callback function if the queue request needs to be asynchronous.
 * @param[in] EventHandle A handle to an event previously created using csCreateEvent().
This value may be NULL if CallbackFn parameter is specified to be valid value or
if also set to NULL when the request needs to be synchronous.
 * @param[out] CompValue Additional completion value provided as part of completion. 
This may be optional depending on the implementation.
 * @return CS_STATUS 
 */
CS_STATUS csQueueBatchRequest(CS_BATCH_HANDLE BatchHandle, void *Context, csQueueCallbackFn CallbackFn,
			      CS_EVT_HANDLE EventHandle, u32 *CompValue); 

//-----------------------------
// Event Management
//-----------------------------
/**
 * @brief Allocates an event resource and returns a handle when successful.
 * 
 * @param[in] EventHandle Pointer to hold the event handle once allocated
 * @return CS_STATUS 
 */
CS_STATUS csCreateEvent(CS_EVT_HANDLE *EventHandle);

/**
 * @brief Frees a previously allocated event resource. 
 * 
 * @param[in] EventHandle The event handle that needs to be freed 
 * @return CS_STATUS 
 */
CS_STATUS csDeleteEvent(CS_EVT_HANDLE EventHandle);

/**
 * @brief Polls the event specified for any pending events.
 * 
 * @param[in] EventHandle The event handle that needs to be polled
 * @param[out] Context The context to the event that completed
 * @return CS_STATUS
 1. CS_NOT_DONE is returned if there no pending events.
 2. CS_SUCCESS is returned if the pending work item completed successfully without errors.
 */
CS_STATUS csPollEvent(CS_EVT_HANDLE EventHandle, void *Context); 
//-----------------------------
// Device Management
//-----------------------------
/**
 * @brief Queries the CSE for its resident CSFs. Functions predefined in the device are returned 
as an array that will include a count and name.
 * 
 * @param[in] DevHandle Handle to CSx
 * @param[inout] Size A pointer to the size of FunctionInfo buffer.
 * @param[out] FunctionInfo A pointer to a buffer that is able to hold all the functions 
resident in the CSE. 
 * @return CS_STATUS 
 */
CS_STATUS csQueryDeviceForComputeList(CS_DEV_HANDLE DevHandle, int *Size, CsFunctionInfo *FunctionInfo);

/**
 * @brief Queries the CSx for its properties.
 * 
 * @param[in] DevHandle Handle to CSx
 * @param[in] Length Length in bytes of buffer passed for output 
 * @param[out] Buffer A pointer to a buffer that is able to hold all the device properties.
 * @return CS_STATUS 
 */
CS_STATUS csQueryDeviceProperties(CS_DEV_HANDLE DevHandle, int *Length, CSxProperties *Buffer);

/**
 * @brief Queries the CSE for its capabilities. These capabilities may be computational storage 
related functions that are built-in.
 * 
 * @param[in] DevHandle Handle to CSx 
 * @param[out] Caps A pointer to a buffer that is able to hold all the CSx capabilities
 * @return CS_STATUS 
 */
CS_STATUS csQueryDeviceCapabilities(CS_DEV_HANDLE DevHandle, CsCapabilities *Caps);

/**
 * @brief Queries the CSx for specific runtime statistics. These could vary depending on the 
requested type inputs. Details on CSFs and the CSx may be queried. This is a privileged function.
 * 
 * @param[in] DevHandle Handle to CSx
 * @param[in] Type Statistics type to query
 * @param[in] Identifier Additional options based on Type.
 * ex. When used for CSEDetails, the Identifier field refers to the UniqueName field in CSEProperties.
 * @param[out] Stats A pointer to a buffer that will hold the requested CSE statistics
 * @return CS_STATUS 
 */
CS_STATUS csQueryDeviceStatistics(CS_DEV_HANDLE DevHandle, CS_STAT_TYPE Type, void *Identifier, CsStatsInfo *Stats);

/**
 * @brief Set the CSx’s specific capability. A specific capability setting is able to be changed by 
the requested type. This is a privileged function. 
 * 
 * @param[in] DevHandle Handle to CSx
 * @param[in] Type Capability type to set
 * @param[in] Details A pointer to a structure that holds the capability details to set 
 * @return CS_STATUS 
 */
CS_STATUS csSetDeviceCapability(CS_DEV_HANDLE DevHandle, CS_CAP_TYPE Type, CsCapabilityInfo *Details);

/**
 * @brief Downloads a specified CSF to a CSE that is programmable. A function may also be 
downloadable that may contain one or more CSFs. It is implementation specific as to 
how the downloaded code is secured. This is a privileged function.
 * 
 * @param[in] DevHandle Handle to CSE 
 * @param[in] ProgramInfo A pointer to a buffer that holds the program details to download
 * @return CS_STATUS 
 */
CS_STATUS csDownload(CS_DEV_HANDLE DevHandle, CsDownloadInfo *ProgramInfo);

/**
 * @brief Downloads a specified configuration to a CSE or one of its CSFs. The configuration is 
implementation specific. It is also implementation specific as to how the downloaded 
configuration is secured. This is a privileged function.
 * 
 * @param[in] CSEHandle Handle to CSE
 * @param[in] Info A pointer to a buffer that holds the configuration details to download
 * @return CS_STATUS 
 */
CS_STATUS csConfig(CS_CSE_HANDLE CSEHandle, CsConfigInfo *Info);

/**
 * @brief Aborts all outstanding and queued transactions to the CSE. 
This is a privileged function.
 * 
 * @param[in] CSEHandle Handle to CSE 
 * @return CS_STATUS 
 */
CS_STATUS csAbortCSE(CS_CSE_HANDLE CSEHandle);

/**
 * @brief Resets the CSE. This is a privileged function.
 * 
 * @param[in] CSEHandle Handle to CSE
 * @return CS_STATUS 
 */
CS_STATUS csResetCSE(CS_CSE_HANDLE CSEHandle);
//-----------------------------
// Stream Management
//-----------------------------

/**
 * @brief Allocates a stream resource with the device.
 * 
 * @param[in] DevHandle Handle to CSx
 * @param[in] Type The type of stream to allocate. This parameter is currently 
reserved or not in use.
 * @param[out] StreamHandle A pointer to a buffer to hold the returned stream handle if successful
 * @return CS_STATUS 
 */
CS_STATUS csAllocStream(CS_DEV_HANDLE DevHandle, CS_ALOC_STREAM_TYPE Type, CS_STREAM_HANDLE *StreamHandle);

/**
 * @brief Releases a previously allocated stream resource with the device.
 * 
 * @param[in] StreamHandle A stream handle that was previously allocated with csAllocStream() request
 * @return CS_STATUS 
 */
CS_STATUS csFreeStream(CS_STREAM_HANDLE StreamHandle);

//-----------------------------
// Library Management
//-----------------------------
/**
 * @brief Queries the API library for supported functionality. Any application that uses the library 
is able to use this query. 
 * 
 * @param[in] Type Library support type query
 * @param[in] Length Length of buffer passed for output
 * @param[out] Buffer Returns a list of CSEs
 * @return CS_STATUS 
 */
CS_STATUS csQueryLibrarySupport(CS_LIBRARY_SUPPORT Type, int *Length, char *Buffer);

/**
 * @brief Queries the API library for registered plugins. This is a privileged function.
 * 
 * @param[in] Req Request structure for type of plugins to query
 * @param[in] CallbackFn Callback function to call into when requested query is satisfied
 * @return CS_STATUS 
 */
CS_STATUS csQueryPlugin(CsQueryPluginRequest *Req, csQueryPluginCallbackFn CallbackFn);

/**
 * @brief Registers a specified plugin with the API library. This is a privileged function.
 * 
 * @param[in] Req Request structure to register a plugin
 * @return CS_STATUS 
 */
CS_STATUS csRegisterPlugin(CsPluginRequest *Req);

/**
 * @brief Deregisters a specified plugin from the API library.
 * 
 * @param[in] Req Request structure to deregister a plugin
 * @return CS_STATUS 
 */
CS_STATUS csDeregisterPlugin(CsPluginRequest *Req);
/* End of CS_H */
#endif
