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
spdk_nvme_csd_ctrlr_program_activate_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 	uint16_t ce_id, uint16_t p_id, uint8_t action,
					spdk_nvme_cmd_cb cb_fn, void *cb_arg);
static int 
spdk_nvme_csd_ctrlr_load_program_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 uint32_t size, void *payload,
				 bool unload, uint8_t program_type, uint16_t program_id,
				 spdk_nvme_cmd_cb cb_fn, void *cb_arg);
static int 
spdk_nvme_csd_ctrlr_execute_program_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 uint16_t ce_id, uint16_t p_id, 
				 uint16_t rs_id,
				 void * d_ptr,
				 void * c_param,
				 spdk_nvme_cmd_cb cb_fn, void *cb_arg);
static int
spdk_nvme_csd_ctrlr_create_memory_range_set_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 uint32_t nu_mr, void *data,
				 spdk_nvme_cmd_cb cb_fn, void *cb_arg);
static int
spdk_nvme_csd_ctrlr_delete_memory_range_set_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 		uint16_t rs_id,
						spdk_nvme_cmd_cb cb_fn, void *cb_arg);

int
spdk_nvme_csd_ctrlr_program_activate(struct spdk_nvme_ctrlr *ctrlr, 
					uint16_t ce_id, uint16_t p_id, uint8_t action,
					struct spdk_nvme_status *completion_status)
{
	struct nvme_completion_poll_status	*status;
	int					res;

	if (!completion_status) {
		return -EINVAL;
	}
	memset(completion_status, 0, sizeof(struct spdk_nvme_status));

	status = calloc(1, sizeof(*status));
	if (!status) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Failed to allocate status tracker\n");
		return -ENOMEM;
	}

	memset(status, 0, sizeof(*status));
	res = spdk_nvme_csd_ctrlr_program_activate_cmd(ctrlr,
							ce_id, p_id, action,
							nvme_completion_poll_cb, status);
	if (res) {
		free(status);
		return res;
	}

	if (nvme_wait_for_completion_robust_lock(ctrlr->adminq, status, &ctrlr->ctrlr_lock)) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "spdk_nvme_csd_ctrlr_program_activate failed!\n");
		if (!status->timed_out) {
			free(status);
		}
		return -ENXIO;
	}
	
	return spdk_nvme_ctrlr_reset(ctrlr);
}

int
spdk_nvme_csd_ctrlr_load_program(struct spdk_nvme_ctrlr *ctrlr, void *payload, uint32_t size,
				 bool unload, uint8_t program_type, uint16_t program_id,
				 struct spdk_nvme_status *completion_status)
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
		return -EFBIG;
	}

	if (size > 0xFFFFFFFF)
	{
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Program size bigger than 0xFFFFFFFF\n");
		return -EFBIG;
	}

	memset(status, 0, sizeof(*status));
	res = spdk_nvme_csd_ctrlr_load_program_cmd(ctrlr, size, payload,
							unload, program_type, program_id,
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

int
spdk_nvme_csd_ctrlr_execute_program(struct spdk_nvme_ctrlr *ctrlr,
				 uint16_t ce_id, uint16_t p_id, 
				 uint16_t rs_id,
				 void * d_ptr,
				 void * c_param,
				 struct spdk_nvme_status *completion_status)
{
	struct nvme_completion_poll_status	*status;
	int					res;

	if (!completion_status) {
		return -EINVAL;
	}
	memset(completion_status, 0, sizeof(struct spdk_nvme_status));

	status = calloc(1, sizeof(*status));
	if (!status) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Failed to allocate status tracker\n");
		return -ENOMEM;
	}

	memset(status, 0, sizeof(*status));
	res = spdk_nvme_csd_ctrlr_execute_program_cmd(ctrlr,
							ce_id, p_id, rs_id,
							d_ptr, c_param,
							nvme_completion_poll_cb, status);
	if (res) {
		free(status);
		return res;
	}

	if (nvme_wait_for_completion_robust_lock(ctrlr->adminq, status, &ctrlr->ctrlr_lock)) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "spdk_nvme_csd_ctrlr_execute_program failed!\n");
		if (!status->timed_out) {
			free(status);
		}
		return -ENXIO;
	}
	
	return spdk_nvme_ctrlr_reset(ctrlr);
}

int
spdk_nvme_csd_ctrlr_create_memory_range_set(struct spdk_nvme_ctrlr *ctrlr, 
						uint32_t nu_mr,
						void *data,
				 		struct spdk_nvme_status *completion_status)
{
	struct nvme_completion_poll_status	*status;
	int					res;

	if (!completion_status) {
		return -EINVAL;
	}
	memset(completion_status, 0, sizeof(struct spdk_nvme_status));

	status = calloc(1, sizeof(*status));
	if (!status) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Failed to allocate status tracker\n");
		return -ENOMEM;
	}

	memset(status, 0, sizeof(*status));
	res = spdk_nvme_csd_ctrlr_create_memory_range_set_cmd(ctrlr, nu_mr, data,
							nvme_completion_poll_cb, status);
	if (res) {
		free(status);
		return res;
	}

	if (nvme_wait_for_completion_robust_lock(ctrlr->adminq, status, &ctrlr->ctrlr_lock)) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "spdk_nvme_csd_ctrlr_create_memory_range_set failed!\n");
		if (!status->timed_out) {
			free(status);
		}
		return -ENXIO;
	}
	
	return spdk_nvme_ctrlr_reset(ctrlr);
}

int
spdk_nvme_csd_ctrlr_delete_memory_range_set(struct spdk_nvme_ctrlr *ctrlr, 
				 		uint16_t rs_id,
				 		struct spdk_nvme_status *completion_status)
{
	struct nvme_completion_poll_status	*status;
	int					res;

	if (!completion_status) {
		return -EINVAL;
	}
	memset(completion_status, 0, sizeof(struct spdk_nvme_status));

	status = calloc(1, sizeof(*status));
	if (!status) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Failed to allocate status tracker\n");
		return -ENOMEM;
	}

	memset(status, 0, sizeof(*status));
	res = spdk_nvme_csd_ctrlr_delete_memory_range_set_cmd(ctrlr,
							rs_id,
							nvme_completion_poll_cb, status);
	if (res) {
		free(status);
		return res;
	}

	if (nvme_wait_for_completion_robust_lock(ctrlr->adminq, status, &ctrlr->ctrlr_lock)) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "spdk_nvme_csd_ctrlr_delete_memory_range_set failed!\n");
		if (!status->timed_out) {
			free(status);
		}
		return -ENXIO;
	}
	
	return spdk_nvme_ctrlr_reset(ctrlr);
}

int
spdk_nvme_csd_ctrlr_get_log_page(struct spdk_nvme_ctrlr *ctrlr, 
				uint8_t log_page, uint32_t nsid, 
				void *payload, uint32_t payload_size, uint64_t offset)
{
	struct nvme_completion_poll_status *status;
	int rc;

	status = calloc(1, sizeof(*status));
	if (status == NULL) {
		NVME_CTRLR_CSD_ERRLOG(ctrlr, "Failed to allocate status tracker\n");
		return -ENOMEM;
	}

	rc = spdk_nvme_ctrlr_cmd_get_log_page(ctrlr, 
						log_page, nsid, 
						payload, payload_size, offset,
					 	nvme_completion_poll_cb, status);
	if (rc != 0) {
		free(status);
		return rc;
	}

	if (nvme_wait_for_completion_robust_lock_timeout(ctrlr->adminq, status, &ctrlr->ctrlr_lock,
			ctrlr->opts.admin_timeout_ms * 1000)) {
		if (!status->timed_out) {
			free(status);
		}
		return -EIO;
	}

	free(status);
	return 0;
}

static int
spdk_nvme_csd_ctrlr_program_activate_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 	uint16_t ce_id, uint16_t p_id, uint8_t action,
					spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct spdk_nvme_cmd *cmd;
	int rc;

	nvme_robust_mutex_lock(&ctrlr->ctrlr_lock);
	req = nvme_allocate_request_null(ctrlr->adminq, cb_fn, cb_arg);
	if (req == NULL) {
		nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);
		return -ENOMEM;
	}

	cmd = &req->cmd;
	cmd->opc = SPDK_CSD_OPC_PROGRAM_ACTIVATION;
	cmd->cdw10_bits.csd_program_activation.ceid = ce_id;
	cmd->cdw10_bits.csd_program_activation.pid = p_id;
	cmd->cdw11_bits.csd_program_activation.action = action;

	rc = nvme_ctrlr_submit_admin_request(ctrlr, req);
	nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);

	return rc;
}

static int
spdk_nvme_csd_ctrlr_execute_program_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 uint16_t ce_id, uint16_t p_id, 
				 uint16_t rs_id,
				 void * d_ptr,
				 void * c_param,
				 spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct spdk_nvme_cmd *cmd;
	int rc;
	uint16_t size = 6 * sizeof(uint32_t);

	nvme_robust_mutex_lock(&ctrlr->ctrlr_lock);
	req = nvme_allocate_request_user_copy(ctrlr->adminq, d_ptr, size, cb_fn, cb_arg, true);
	if (req == NULL) {
		nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);
		return -ENOMEM;
	}

	cmd = &req->cmd;
	cmd->opc = SPDK_CSD_OPC_EXECUTE_PROGRAM;
	cmd->cdw2_bits.csd_execute_program.ceid = ce_id;
	cmd->cdw2_bits.csd_execute_program.pid = p_id;
	cmd->cdw3_bits.csd_execute_program.rsid = rs_id;
	memcpy(&cmd->cdw10, c_param, size);

	rc = nvme_ctrlr_submit_admin_request(ctrlr, req);
	nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);

	return rc;
}

static int 
spdk_nvme_csd_ctrlr_load_program_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 uint32_t size, void *payload,
				 bool unload, uint8_t program_type, uint16_t program_id,
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
	cmd->cdw10_bits.csd_load_program.pid = program_id;
	cmd->cdw11 = size;

	rc = nvme_ctrlr_submit_admin_request(ctrlr, req);
	nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);

	return rc;
}

static int
spdk_nvme_csd_ctrlr_create_memory_range_set_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 		uint32_t nu_mr, void *data,
						spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct spdk_nvme_cmd *cmd;
	int rc;
	uint32_t size = nu_mr * sizeof(struct spdk_csd_memory_range_definition);

	nvme_robust_mutex_lock(&ctrlr->ctrlr_lock);
	req = nvme_allocate_request_user_copy(ctrlr->adminq, data, size, cb_fn, cb_arg, true);
	if (req == NULL) {
		nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);
		return -ENOMEM;
	}

	cmd = &req->cmd;
	cmd->opc = SPDK_CSD_OPC_CREATE_MEMORY_RANGE_SET;
	cmd->cdw10 = nu_mr;

	rc = nvme_ctrlr_submit_admin_request(ctrlr, req);
	nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);

	return rc;
}

static int
spdk_nvme_csd_ctrlr_delete_memory_range_set_cmd(struct spdk_nvme_ctrlr *ctrlr,
				 		uint16_t rs_id,
						spdk_nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct spdk_nvme_cmd *cmd;
	int rc;

	nvme_robust_mutex_lock(&ctrlr->ctrlr_lock);
	req = nvme_allocate_request_null(ctrlr->adminq, cb_fn, cb_arg);
	if (req == NULL) {
		nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);
		return -ENOMEM;
	}

	cmd = &req->cmd;
	cmd->opc = SPDK_CSD_OPC_DELETE_MEMORY_RANGE_SET;
	cmd->cdw10_bits.csd_delete_memory_range_set.rsid = rs_id;

	rc = nvme_ctrlr_submit_admin_request(ctrlr, req);
	nvme_robust_mutex_unlock(&ctrlr->ctrlr_lock);

	return rc;
}