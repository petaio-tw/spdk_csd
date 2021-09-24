/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * Computational Programs Command Set specification definitions
 * (refer to the NVME TP 4091 2021.07.28 - Phase 2) 
 */

#ifndef SPDK_NVME_CSD_SPEC_H
#define SPDK_NVME_CSD_SPEC_H

#include "spdk/stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "spdk/assert.h"
#include "spdk/nvme_spec.h"

#define MAX_NUM_MEMORY_RANGE		170

/**
 * CSD admin command set opcodes
 */
enum spdk_csd_admin_opcode {
	SPDK_CSD_OPC_PROGRAM_ACTIVATION		= 0x20,
	SPDK_CSD_OPC_EXECUTE_PROGRAM		= 0x21,
	/* 0x22 - reserved */
	SPDK_CSD_OPC_LOAD_PROGRAM		= 0x23,
	/* 0x25 - reserved */
	SPDK_CSD_OPC_CREATE_MEMORY_RANGE_SET	= 0x24,
	SPDK_CSD_OPC_DELETE_MEMORY_RANGE_SET	= 0x26,
};

/**
 * Log page identifiers for SPDK_NVME_OPC_GET_LOG_PAGE
 */
enum spdk_csd_log_page {
	SPDK_CSD_LOG_PROGRAM_INFORMATION	= 0x10,
	SPDK_CSD_LOG_COMPUTE_ENGINE_INFORMATION	= 0x11,
	SPDK_CSD_LOG_COMPUTE_ENGINE_LIST	= 0x12,
};

/**
 * CSD Command specific status codes extension.
 * Additional command specific codes for status code type “1h” (command specific)
 */
enum spdk_csd_command_specific_status_code {
	SPDK_CSD_SC_INSUFFICIEND_PROGRAM_RESOURCES	= 0x30,
	SPDK_CSD_SC_INVALID_COMPUTE_ENGINE		= 0x31,
	SPDK_CSD_SC_INVALID_MEMORY_RANGE_SET		= 0x32,
	SPDK_CSD_SC_INVALID_MEMORY_REGION		= 0x33,
	SPDK_CSD_SC_INVALID_PROGRAM_DATA		= 0x34,
	SPDK_CSD_SC_INVALID_PROGRAM_IDENTIFIER		= 0x35,
	SPDK_CSD_SC_INVALID_PROGRAM_TYPE		= 0x36,
	SPDK_CSD_SC_MAXIMUM_MEMORY_RANGES_EXCEEDED	= 0x37,
	SPDK_CSD_SC_MEMORY_RANGE_SET_IN_USE		= 0x38,
	SPDK_CSD_SC_PROGRAM_NOT_ACTIVATED		= 0x39,
	SPDK_CSD_SC_PROGRAM_IDENTIFIER_IN_USE		= 0x3A,
	SPDK_CSD_SC_PROGRAM_IDENTIFIER_NOT_AVAILABLE	= 0x3B,
	SPDK_CSD_SC_PROGRAM_UNLOAD_NOT_ALLOWED		= 0x3C,
};

struct spdk_csd_memory_range_definition {
	uint16_t memory_region_id;
	uint16_t reserved;
	uint32_t length;
	uint64_t offset;
	uint64_t attributes;
};

struct spdk_csd_compute_engine_descriptor_data {

	uint8_t reserved0;
	struct {
		uint8_t	activation		: 1;
		uint8_t	reserved1		: 7;
	};
	uint16_t compute_engine_identifier;
};

#define MAX_COMPUTE_ENGINE_DES		8
#define UUID_BYTE_SIZE             	16

struct spdk_csd_program_information_data {

	struct
	{
		uint8_t	program_entry_occupied	: 1;
		uint8_t	program_entry_type	: 1;
		uint8_t	reserved0		: 6;
	};

	uint8_t number_of_associated_compute_engines;
	uint8_t reserved1[2];
	struct spdk_csd_compute_engine_descriptor_data associated_compute_engine_descriptor[MAX_COMPUTE_ENGINE_DES];
	uint16_t program_type;
	uint8_t reserved2[10];
	uint8_t program_uuid[UUID_BYTE_SIZE];
};

struct spdk_csd_program_information_log {

	uint32_t number_of_records;
	uint32_t reserved[15];
	struct spdk_csd_program_information_data program_record[0];
};

struct spdk_csd_memory_region_descriptor {
	
	uint16_t memory_region_id;
	uint16_t reserved;
};

#define MAX_MEMORY_REGION			(256) 
#define MAX_PROGRAM_TYPE_IDENTIFIER		(64)
#define ENGINE_CHARACTERISTICS_BYTE_SIZE	(6)

struct spdk_csd_controller_memory_region {

	uint8_t number_of_controller_memory_regions;
	uint8_t reserved[3];
	struct spdk_csd_memory_region_descriptor memory_region[MAX_MEMORY_REGION];
	uint8_t reserved1[4];
	uint8_t number_of_supported_downloadable_program_types;
	uint8_t reserved2;
	uint16_t program_type_identifier[MAX_PROGRAM_TYPE_IDENTIFIER];
	uint8_t general_engine_characteristics[ENGINE_CHARACTERISTICS_BYTE_SIZE];

};

// get log page command : compute engine list

struct spdk_csd_compute_engine_list {

	uint16_t number_of_compute_engine;
	uint8_t reserved[6];
	uint16_t compute_engine_identifier[0];

};

#ifdef __cplusplus
}
#endif

#endif
