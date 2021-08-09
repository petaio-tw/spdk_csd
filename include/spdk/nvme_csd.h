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
 * NVMe driver public API extension for Computational Programs Command Set
 */

#ifndef SPDK_NVME_CSD_H
#define SPDK_NVME_CSD_H

#include "spdk/stdinc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "spdk/nvme.h"
#include "spdk/nvme_csd_spec.h"

int
spdk_nvme_csd_ctrlr_program_activate(struct spdk_nvme_ctrlr *ctrlr, 
					uint16_t ce_id, uint16_t p_id, uint8_t action,
					struct spdk_nvme_status *completion_status);
int
spdk_nvme_csd_ctrlr_execute_program(struct spdk_nvme_ctrlr *ctrlr,
				 uint16_t ce_id, uint16_t p_id, 
				 uint16_t rs_id,
				 void * d_ptr,
				 void * c_param,
				 struct spdk_nvme_status *completion_status);
int
spdk_nvme_csd_ctrlr_load_program(struct spdk_nvme_ctrlr *ctrlr, void *payload, uint32_t size,
				 bool unload, uint8_t program_type, uint16_t program_id,
				 struct spdk_nvme_status *completion_status);
int
spdk_nvme_csd_ctrlr_create_memory_range_set(struct spdk_nvme_ctrlr *ctrlr, 
						uint32_t nu_mr,
						void *data,
				 		struct spdk_nvme_status *completion_status);
int spdk_nvme_csd_ctrlr_delete_memory_range_set(struct spdk_nvme_ctrlr *ctrlr, 
				 		uint16_t rs_id,
				 		struct spdk_nvme_status *completion_status);

#ifdef __cplusplus
}
#endif

#endif
