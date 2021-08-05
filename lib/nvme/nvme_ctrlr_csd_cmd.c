/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation. All rights reserved.
 *   Copyright (c) 2021 Mellanox Technologies LTD. All rights reserved.
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

#include "spdk/nvme_csd.h"
#include "nvme_internal.h"

#define CTRLR_STRING(ctrlr) \
	((ctrlr->trid.trtype == SPDK_NVME_TRANSPORT_TCP || ctrlr->trid.trtype == SPDK_NVME_TRANSPORT_RDMA) ? \
	ctrlr->trid.subnqn : ctrlr->trid.traddr)

#define NVME_CTRLR_CSD_ERRLOG(ctrlr, format, ...) \
	SPDK_ERRLOG("[%s] " format, CTRLR_STRING(ctrlr), ##__VA_ARGS__);

static int 
spdk_nvme_csd_ctrlr_load_program_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 uint32_t size, void *payload,
				 bool unload, uint8_t program_type, uint16_t program_slot,
				 spdk_nvme_cmd_cb cb_fn, void *cb_arg);


int
spdk_nvme_csd_ctrlr_load_program(struct spdk_nvme_ctrlr *ctrlr, void *payload, uint32_t size,
				int slot, struct spdk_nvme_status *completion_status)
{
	struct nvme_completion_poll_status	*status;
	int					res;

	if (!completion_status) {
		return -EINVAL;
	}
	memset(completion_status, 0, sizeof(struct spdk_nvme_status));
	if (size % 4) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "spdk_nvme_ctrlr_csd_load_program invalid size!\n");
		return -1;
	}

	status = calloc(1, sizeof(*status));
	if (!status) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Failed to allocate status tracker\n");
		return -ENOMEM;
	}

	if (size > ctrlr->min_page_size)
	{
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Program size bigger than MIN page size\n");
		return -ENOMEM;
	}

	if (size > 0xFFFFFFFF)
	{
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Program size bigger than 0xFFFFFFFF\n");
		return -ENOMEM;
	}

	memset(status, 0, sizeof(*status));
	res = spdk_nvme_csd_ctrlr_load_program_cmd(ctrlr, size, payload,
							1, 2, slot,
							nvme_completion_poll_cb, status);
	if (res) {
		free(status);
		return res;
	}

	if (nvme_wait_for_completion_robust_lock(ctrlr->adminq, status, &ctrlr->ctrlr_lock)) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "spdk_nvme_ctrlr_csd_load_program failed!\n");
		if (!status->timed_out) {
			free(status);
		}
		return -ENXIO;
	}
	
	return spdk_nvme_ctrlr_reset(ctrlr);
}

void spdk_nvme_csd_ctrlr_program_activate(void)
{
}

void spdk_nvme_csd_ctrlr_execute_program(void)
{
}

static int 
spdk_nvme_csd_ctrlr_load_program_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 uint32_t size, void *payload,
				 bool unload, uint8_t program_type, uint16_t program_slot,
				 spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct spdk_nvme_cmd *cmd;
	int rc;

	nvme_robust_mutex_lock(&ctrlr->ctrlr_lock);
	req = nvme_allocate_request_user_copy(ctrlr->adminq, payload, size, cb_fn, cb_arg, true);
	if (req == NULL) {
		nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);
		return -ENOMEM;
	}

	cmd = &req->cmd;
	cmd->opc = SPDK_CSD_OPC_LOAD_PROGRAM;
	cmd->cdw10_bits.csd_load_program.unl = unload;
	cmd->cdw10_bits.csd_load_program.ptype = program_type;
	cmd->cdw10_bits.csd_load_program.pslot = program_slot;
	cmd->cdw11 = size;

	rc = nvme_ctrlr_submit_admin_request(ctrlr, req);
	nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);

	return rc;
}

void spdk_nvme_csd_ctrlr_create_memory_range_set(void)
{
}

void spdk_nvme_csd_ctrlr_delete_memory_range_set(void)
{
}
