/**
 * @file cs.c
 * @author Alex Hsu (ahsu@petaio.com)
 * @brief 
 * @version 0.1
 * @date 2022-02-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

/**********************************************************/
/*                                                        */  
/* INCLUDE FILE DECLARATIONS                              */
/*                                                        */
/**********************************************************/
#include "spdk/env.h"
#include "spdk/cs.h"

/**********************************************************/
/*                                                        */  
/* NAMING CONSTANT DECLARATIONS                           */  
/* {Constants define for LOCAL reference ONLY.}           */  
/*                                                        */  
/**********************************************************/

/**********************************************************/
/*                                                        */
/* MACRO FUNCTION DECLARATIONS                            */
/* {MACRO functions define for LOCAL reference ONLY.}     */
/*                                                        */
/**********************************************************/

/**********************************************************/
/*                                                        */
/* DATA TYPE DECLARATIONS                                 */
/* {DATA TYPE define for LOCAL reference ONLY.}           */
/*                                                        */  
/**********************************************************/

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM DECLARATIONS                          */
/* {Function routines define for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/

/**********************************************************/
/*                                                        */
/* STATIC VARIABLE DECLARATIONS                           */
/* {STATIC VARIABLES defines for LOCAL reference ONLY.}   */
/*                                                        */
/**********************************************************/

/**********************************************************/
/*                                                        */
/* EXPORTED SUBPROGRAM BODIES                             */
/* {C code body of each EXPORTED function routine.}       */
/*                                                        */
/**********************************************************/
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
CS_STATUS csQueryCSEList(char *FunctionName, int *Length, char *Buffer)
{
	return CS_SUCCESS;
}

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
CS_STATUS csQueryFunctionList(char *Path, int *Length, char *Buffer)
{
	return CS_SUCCESS;
}

/**
 * @brief This function returns the CSx associated with the specified file or directory path.
 * 
 * @param[in] Path A string that denotes a path to a file, directory that resides 
on a device or a device path. The file/directory may indirectly refer to a namespace and partition. 
 * @param[inout] Length Length of buffer passed for output
 * @param[out] DevName Returns the qualified name to the CSx
 * @return CS_STATUS 
 */
CS_STATUS csGetCSxFromPath(char *Path, unsigned int *Length, char *DevName)
{
	return CS_SUCCESS;
}

/**
 * @brief This function returns one or more CSE’s associated with the specified CSx.
 * 
 * @param[in] DevHandle A handle to the CSx to query
 * @param[inout] Length Length of buffer passed for output
 * @param[out] CSEName Returns the qualified name to CSE
 * @return CS_STATUS 
 */
CS_STATUS csGetCSEFromCSx(CS_DEV_HANDLE DevHandle, unsigned int *Length, char *CSEName)
{
	return CS_SUCCESS;
}

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
CS_STATUS csOpenCSx(char *DevName, void *DevContext, CS_DEV_HANDLE *DevHandle)
{
	return CS_SUCCESS;
}

/**
 * @brief Close a CSx previously opened and associated with the specified handle.
 * 
 * @param[in] DevHandle Handle to CSx
 * @return CS_STATUS 
 */
CS_STATUS csCloseCSx(CS_DEV_HANDLE DevHandle)
{
	return CS_SUCCESS;
}

/**
 * @brief Return a handle to the CSE associated with the specified device name.
 * 
 * @param[in] CSEName A string that denotes the full name of the CSE
 * @param[in] CSEContext A user specified context to associate with the CSE for 
future notifications
 * @param[out] CSEHandle Returns the handle to the CSE device
 * @return CS_STATUS 
 */
CS_STATUS csOpenCSE(char *CSEName, void *CSEContext, CS_CSE_HANDLE *CSEHandle) 
{
	return CS_SUCCESS;
}

/**
 * @brief Close a CSE previously opened and associated with the specified handle.
 * 
 * @param[in] CSEHandle Handle to CSE
 * @return CS_STATUS 
 */
CS_STATUS csCloseCSE(CS_CSE_HANDLE CSEHandle)
{
	return CS_SUCCESS;
}

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
CS_STATUS csRegisterNotify(char *DevName, u32 NotifyOptions, csDevNotificationFn NotifyFn)
{
	return CS_SUCCESS;
}

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
CS_STATUS csDeregisterNotify(char *DevName, csDevNotificationFn NotifyFn)
{
	return CS_SUCCESS;
}

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
		     CS_MEM_HANDLE *MemHandle, CS_MEM_PTR *VAddressPtr)
{
	return CS_SUCCESS;
}

/**
 * @brief Frees AFDM for the memory handle specified.
 * 
 * @param[in] MemHandle Handle to AFDM
 * @return CS_STATUS 
 */
CS_STATUS csFreeMem(CS_MEM_HANDLE MemHandle)
{
	return CS_SUCCESS;
}

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
				CS_EVT_HANDLE EventHandle, u32 *CompValue)
{
	return CS_SUCCESS;
}
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
			  	CS_EVT_HANDLE EventHandle, u32 *CompValue)
{
	return CS_SUCCESS;
}

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
CS_STATUS csGetFunction(CS_CSE_HANDLE CSEHandle, char *FunctionName, void *Context, CS_FUNCTION_ID *FunctionId)
{
	return CS_SUCCESS;
}

/**
 * @brief Stops the CSF specified if it is running any tasks.
 * 
 * @param[in] FunctionId The function id to the CSF to stop
 * @return CS_STATUS 
 */
CS_STATUS csStopFunction(CS_FUNCTION_ID FunctionId)
{
	return CS_SUCCESS;
}

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
			        CS_EVT_HANDLE EventHandle, u32 *CompValue)
{
	return CS_SUCCESS;
}

/**
 * @brief Helper function that are able to optionally be used to set an argument for a compute 
request.
 * 
 * @param[in] ArgPtr A pointer to the argument in CsComputeRequest to be set. 
 * @param[in] Type The argument type to set. This may be one of the enum values. 
 * @param[in] ... One or more variables that make up the argument by type. 
 */
void csHelperSetComputeArg(CsComputeArg *ArgPtr, CS_COMPUTE_ARG_TYPE Type,...)
{
	return;
}

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
CS_STATUS csAllocBatchRequest(CS_BATCH_MODE Mode, int MaxReqs, CS_BATCH_HANDLE *BatchHandle)
{
	return CS_SUCCESS;
}

/**
 * @brief Frees a batch handle previously allocated with a call to csAllocBatchRequest().
 * 
 * @param[in] BatchHandle The handle previously allocated for batch requests.
 * @return CS_STATUS 
 */
CS_STATUS csFreeBatchRequest(CS_BATCH_HANDLE BatchHandle)
{
	return CS_SUCCESS;
}

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
			  CS_BATCH_INDEX After, CS_BATCH_INDEX *Curr)
{
	return CS_SUCCESS;
}

/**
 * @brief Helps reconfigure an existing batch request entry with new request information.
 * 
 * @param[in] BatchHandle The handle previously allocated for batch requests.
 * @param[in] Entry The request’s batch entry index that is reconfigured.
 * @param[in] Req The new batch request entry details. 
 * @return CS_STATUS 
 */
CS_STATUS csHelperReconfigureBatchEntry(CS_BATCH_HANDLE BatchHandle, CS_BATCH_INDEX Entry, CsBatchRequest *Req)
{
	return CS_SUCCESS;
}

/**
 * @brief Resizes an existing batch request for the maximum number of requests that it is able to 
accommodate.
 * 
 * @param[in] BatchHandle The handle previously allocated for batch requests that is resized. 
 * @param[in] MaxReqs The maximum number of requests the caller perceives that this batch 
resource is resized to. The parameter may not exceed the maximum supported by the CSE. 
 * @return CS_STATUS 
 */
CS_STATUS csHelperResizeBatchRequest(CS_BATCH_HANDLE BatchHandle, int MaxReqs)
{
	return CS_SUCCESS;
}

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
			      CS_EVT_HANDLE EventHandle, u32 *CompValue)
{
	return CS_SUCCESS;
}

//-----------------------------
// Event Management
//-----------------------------
/**
 * @brief Allocates an event resource and returns a handle when successful.
 * 
 * @param[in] EventHandle Pointer to hold the event handle once allocated
 * @return CS_STATUS 
 */
CS_STATUS csCreateEvent(CS_EVT_HANDLE *EventHandle)
{
	return CS_SUCCESS;
}

/**
 * @brief Frees a previously allocated event resource. 
 * 
 * @param[in] EventHandle The event handle that needs to be freed 
 * @return CS_STATUS 
 */
CS_STATUS csDeleteEvent(CS_EVT_HANDLE EventHandle)
{
	return CS_SUCCESS;
}

/**
 * @brief Polls the event specified for any pending events.
 * 
 * @param[in] EventHandle The event handle that needs to be polled
 * @param[out] Context The context to the event that completed
 * @return CS_STATUS
 1. CS_NOT_DONE is returned if there no pending events.
 2. CS_SUCCESS is returned if the pending work item completed successfully without errors.
 */
CS_STATUS csPollEvent(CS_EVT_HANDLE EventHandle, void *Context)
{
	return CS_SUCCESS;
}

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
CS_STATUS csQueryDeviceForComputeList(CS_DEV_HANDLE DevHandle, int *Size, CsFunctionInfo *FunctionInfo)
{
	return CS_SUCCESS;
}

/**
 * @brief Queries the CSx for its properties.
 * 
 * @param[in] DevHandle Handle to CSx
 * @param[in] Length Length in bytes of buffer passed for output 
 * @param[out] Buffer A pointer to a buffer that is able to hold all the device properties.
 * @return CS_STATUS 
 */
CS_STATUS csQueryDeviceProperties(CS_DEV_HANDLE DevHandle, int *Length, CSxProperties *Buffer)
{
	return CS_SUCCESS;
}

/**
 * @brief Queries the CSE for its capabilities. These capabilities may be computational storage 
related functions that are built-in.
 * 
 * @param[in] DevHandle Handle to CSx 
 * @param[out] Caps A pointer to a buffer that is able to hold all the CSx capabilities
 * @return CS_STATUS 
 */
CS_STATUS csQueryDeviceCapabilities(CS_DEV_HANDLE DevHandle, CsCapabilities *Caps)
{
	return CS_SUCCESS;
}

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
CS_STATUS csQueryDeviceStatistics(CS_DEV_HANDLE DevHandle, CS_STAT_TYPE Type, void *Identifier, CsStatsInfo *Stats)
{
	return CS_SUCCESS;
}

/**
 * @brief Set the CSx’s specific capability. A specific capability setting is able to be changed by 
the requested type. This is a privileged function. 
 * 
 * @param[in] DevHandle Handle to CSx
 * @param[in] Type Capability type to set
 * @param[in] Details A pointer to a structure that holds the capability details to set 
 * @return CS_STATUS 
 */
CS_STATUS csSetDeviceCapability(CS_DEV_HANDLE DevHandle, CS_CAP_TYPE Type, CsCapabilityInfo *Details)
{
	return CS_SUCCESS;
}

/**
 * @brief Downloads a specified CSF to a CSE that is programmable. A function may also be 
downloadable that may contain one or more CSFs. It is implementation specific as to 
how the downloaded code is secured. This is a privileged function.
 * 
 * @param[in] DevHandle Handle to CSE 
 * @param[in] ProgramInfo A pointer to a buffer that holds the program details to download
 * @return CS_STATUS 
 */
CS_STATUS csDownload(CS_DEV_HANDLE DevHandle, CsDownloadInfo *ProgramInfo)
{
	return CS_SUCCESS;
}

/**
 * @brief Downloads a specified configuration to a CSE or one of its CSFs. The configuration is 
implementation specific. It is also implementation specific as to how the downloaded 
configuration is secured. This is a privileged function.
 * 
 * @param[in] CSEHandle Handle to CSE
 * @param[in] Info A pointer to a buffer that holds the configuration details to download
 * @return CS_STATUS 
 */
CS_STATUS csConfig(CS_CSE_HANDLE CSEHandle, CsConfigInfo *Info)
{
	return CS_SUCCESS;
}

/**
 * @brief Aborts all outstanding and queued transactions to the CSE. 
This is a privileged function.
 * 
 * @param[in] CSEHandle Handle to CSE 
 * @return CS_STATUS 
 */
CS_STATUS csAbortCSE(CS_CSE_HANDLE CSEHandle)
{
	return CS_SUCCESS;
}

/**
 * @brief Resets the CSE. This is a privileged function.
 * 
 * @param[in] CSEHandle Handle to CSE
 * @return CS_STATUS 
 */
CS_STATUS csResetCSE(CS_CSE_HANDLE CSEHandle)
{
	return CS_SUCCESS;
}

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
CS_STATUS csAllocStream(CS_DEV_HANDLE DevHandle, CS_ALOC_STREAM_TYPE Type, CS_STREAM_HANDLE *StreamHandle)
{
	return CS_SUCCESS;
}

/**
 * @brief Releases a previously allocated stream resource with the device.
 * 
 * @param[in] StreamHandle A stream handle that was previously allocated with csAllocStream() request
 * @return CS_STATUS 
 */
CS_STATUS csFreeStream(CS_STREAM_HANDLE StreamHandle)
{
	return CS_SUCCESS;
}

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
CS_STATUS csQueryLibrarySupport(CS_LIBRARY_SUPPORT Type, int *Length, char *Buffer)
{
	return CS_SUCCESS;
}

/**
 * @brief Queries the API library for registered plugins. This is a privileged function.
 * 
 * @param[in] Req Request structure for type of plugins to query
 * @param[in] CallbackFn Callback function to call into when requested query is satisfied
 * @return CS_STATUS 
 */
CS_STATUS csQueryPlugin(CsQueryPluginRequest *Req, csQueryPluginCallbackFn CallbackFn)
{
	return CS_SUCCESS;
}

/**
 * @brief Registers a specified plugin with the API library. This is a privileged function.
 * 
 * @param[in] Req Request structure to register a plugin
 * @return CS_STATUS 
 */
CS_STATUS csRegisterPlugin(CsPluginRequest *Req)
{
	return CS_SUCCESS;
}

/**
 * @brief Deregisters a specified plugin from the API library.
 * 
 * @param[in] Req Request structure to deregister a plugin
 * @return CS_STATUS 
 */
CS_STATUS csDeregisterPlugin(CsPluginRequest *Req)
{
	return CS_SUCCESS;
}

/**********************************************************/
/*                                                        */
/* LOCAL SUBPROGRAM BODIES                                */
/* {C code body of each LOCAL function routines.}         */
/*                                                        */
/**********************************************************/

/* End of CS_C */
