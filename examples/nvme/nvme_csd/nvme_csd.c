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

#include "spdk/stdinc.h"

#include "spdk/nvme.h"
#include "spdk/env.h"
#include "spdk/string.h"
#include "spdk/util.h"
#include "spdk/opal.h"

#include "spdk/nvme_csd.h"

#define FOR_ONLY_ONE_TEST_DEVICE	1

#define MAX_DEVS 64
#define MAX_SHELL_CMD_DESC_STR		100

#define NUM_NS				1
#define NS_START_IDX			1
#define NUM_CE				4
#define NUM_PROGRAM			4
#define NUM_MEM_RANGE_SET		4

typedef void (*SHELL_FUNC_PTR)(void);

struct dev {
	struct spdk_pci_addr			pci_addr;
	struct spdk_nvme_ctrlr			*ctrlr;
	const struct spdk_nvme_ctrlr_data	*cdata;
	struct spdk_nvme_ns_data		*common_ns_data;
	int					outstanding_admin_cmds;
	struct spdk_opal_dev			*opal_dev;
};

struct ctrlr_entry {
	struct spdk_nvme_ctrlr		*ctrlr;
	TAILQ_ENTRY(ctrlr_entry)	link;
	char				name[1024];
};

struct ns_entry {
	struct spdk_nvme_ctrlr	*ctrlr;
	struct spdk_nvme_ns	*ns;
	TAILQ_ENTRY(ns_entry)	link;
	struct spdk_nvme_qpair	*qpair;
};

struct csd_sequence {
	struct ns_entry	*ns_entry;
	char		*buf;
	unsigned        using_cmb_io;
	int		is_completed;
};

static struct dev devs[MAX_DEVS];
static int num_devs = 0;
static int g_shm_id = -1;
static bool g_exit_flag = false;
static TAILQ_HEAD(, ctrlr_entry) g_controllers = TAILQ_HEAD_INITIALIZER(g_controllers);
static TAILQ_HEAD(, ns_entry) g_namespaces = TAILQ_HEAD_INITIALIZER(g_namespaces);

#define foreach_dev(iter) \
	for (iter = devs; iter - devs < num_devs; iter++)

enum controller_display_model {
	CONTROLLER_DISPLAY_ALL			= 0x0,
	CONTROLLER_DISPLAY_SIMPLISTIC		= 0x1,
};

enum shell_cmd_idx {
	SHELL_CMD_PROGRAM_ACTIVATION		= 0,
	SHELL_CMD_EXECUTE_PROGRAM,
	SHELL_CMD_LOAD_PROGRAM,
	SHELL_CMD_CREATE_MEMORY_RANGE_SET,
	SHELL_CMD_DELETE_MEMORY_RANGE_SET,
	SHELL_CMD_PROGRAM_INFORMATION_LOG,
	SHELL_CMD_COMPUTE_ENGINE_INFORMATION,
	SHELL_CMD_COMPUTE_ENGINE_LIST,
	SHELL_CMD_QUIT,

	SHELL_CMD_MAX
};

struct shell_content {
	SHELL_FUNC_PTR	funcPtr;
	char			desc[MAX_SHELL_CMD_DESC_STR];
};

static void shell_cmd_quit(void);
static void nvme_csd_program_activation(void);
static void nvme_csd_execute_program(void);
static void nvme_csd_load_program(void);
static void nvme_csd_create_memory_range_set(void);
static void nvme_csd_delete_memory_range_set(void);
static void nvme_csd_get_program_information_log(void);
static void nvme_csd_get_compute_engine_information(void);
static void nvme_csd_get_compute_engine_list(void);
static int
nvme_csd_get_log_page(struct spdk_nvme_ctrlr *ctrlr, 
			uint8_t log_page, 
			void *payload, uint32_t payload_size);

struct shell_content g_shell_content[SHELL_CMD_MAX] = 
{
	{nvme_csd_program_activation,			"SHELL_CMD_PROGRAM_ACTIVATION"},
	{nvme_csd_execute_program,			"SHELL_CMD_EXECUTE_PROGRAM"},
	{nvme_csd_load_program,				"SHELL_CMD_LOAD_PROGRAM"},
	{nvme_csd_create_memory_range_set,		"SHELL_CMD_CREATE_MEMORY_RANGE_SET"},
	{nvme_csd_delete_memory_range_set,		"SHELL_CMD_DELETE_MEMORY_RANGE_SET"},
	{nvme_csd_get_program_information_log,		"SHELL_CMD_PROGRAM_INFORMATION_LOG"},
	{nvme_csd_get_compute_engine_information,	"SHELL_CMD_COMPUTE_ENGINE_INFORMATION"},
	{nvme_csd_get_compute_engine_list,		"SHELL_CMD_COMPUTE_ENGINE_LIST"},
	{shell_cmd_quit,				"SHELL_CMD_QUIT"},
};

static void
register_ns(struct spdk_nvme_ctrlr *ctrlr, struct spdk_nvme_ns *ns)
{
	struct ns_entry *entry;

	if (!spdk_nvme_ns_is_active(ns)) {
		return;
	}

	entry = malloc(sizeof(struct ns_entry));
	if (entry == NULL) {
		perror("ns_entry malloc");
		exit(1);
	}

	entry->ctrlr = ctrlr;
	entry->ns = ns;
	TAILQ_INSERT_TAIL(&g_namespaces, entry, link);

	printf("  Namespace ID: %d size: %juMB\n", spdk_nvme_ns_get_id(ns),
	       spdk_nvme_ns_get_size(ns) / 1000000);
}

static void shell_cmd_quit(void)
{
	g_exit_flag = true;
}

static int
cmp_devs(const void *ap, const void *bp)
{
	const struct dev *a = ap, *b = bp;

	return spdk_pci_addr_compare(&a->pci_addr, &b->pci_addr);
}

static bool
probe_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	 struct spdk_nvme_ctrlr_opts *opts)
{
	return true;
}

static void
identify_common_ns_cb(void *cb_arg, const struct spdk_nvme_cpl *cpl)
{
	struct dev *dev = cb_arg;

	if (cpl->status.sc != SPDK_NVME_SC_SUCCESS) {
		/* Identify Namespace for NSID = FFFFFFFFh is optional, so failure is not fatal. */
		spdk_dma_free(dev->common_ns_data);
		dev->common_ns_data = NULL;
	}

	dev->outstanding_admin_cmds--;
}

static void
attach_cb(void *cb_ctx, const struct spdk_nvme_transport_id *trid,
	  struct spdk_nvme_ctrlr *ctrlr, const struct spdk_nvme_ctrlr_opts *opts)
{
	struct dev *dev;
	struct spdk_nvme_cmd cmd;
	int nsid, num_ns;
	struct ctrlr_entry *entry;
	struct spdk_nvme_ns *ns;

	/* add to dev list */
	dev = &devs[num_devs++];
	spdk_pci_addr_parse(&dev->pci_addr, trid->traddr);
	dev->ctrlr = ctrlr;

	/* Retrieve controller data */
	dev->cdata = spdk_nvme_ctrlr_get_data(dev->ctrlr);

	dev->common_ns_data = spdk_dma_zmalloc(sizeof(struct spdk_nvme_ns_data), 4096, NULL);
	if (dev->common_ns_data == NULL) {
		fprintf(stderr, "common_ns_data allocation failure\n");
		return;
	}

	/* Identify Namespace with NSID set to FFFFFFFFh to get common namespace capabilities. */
	memset(&cmd, 0, sizeof(cmd));
	cmd.opc = SPDK_NVME_OPC_IDENTIFY;
	cmd.cdw10_bits.identify.cns = 0; /* CNS = 0 (Identify Namespace) */
	cmd.nsid = SPDK_NVME_GLOBAL_NS_TAG;

	dev->outstanding_admin_cmds++;
	if (spdk_nvme_ctrlr_cmd_admin_raw(ctrlr, &cmd, dev->common_ns_data,
					  sizeof(struct spdk_nvme_ns_data), identify_common_ns_cb, dev) != 0) {
		dev->outstanding_admin_cmds--;
		spdk_dma_free(dev->common_ns_data);
		dev->common_ns_data = NULL;
	}

	while (dev->outstanding_admin_cmds) {
		spdk_nvme_ctrlr_process_admin_completions(ctrlr);
	}

	entry = malloc(sizeof(struct ctrlr_entry));
	if (entry == NULL) {
		perror("ctrlr_entry malloc");
		exit(1);
	}

	printf("Attached to %s\n", trid->traddr);

	snprintf(entry->name, sizeof(entry->name), "%-20.20s (%-20.20s)", dev->cdata->mn, dev->cdata->sn);

	entry->ctrlr = ctrlr;
	TAILQ_INSERT_TAIL(&g_controllers, entry, link);
	/*
	 * Each controller has one or more namespaces.  An NVMe namespace is basically
	 *  equivalent to a SCSI LUN.  The controller's IDENTIFY data tells us how
	 *  many namespaces exist on the controller.  For Intel(R) P3X00 controllers,
	 *  it will just be one namespace.
	 *
	 * Note that in NVMe, namespace IDs start at 1, not 0.
	 */
	num_ns = spdk_nvme_ctrlr_get_num_ns(ctrlr);
	printf("Using controller %s with %d namespaces.\n", entry->name, num_ns);
	for (nsid = 1; nsid <= num_ns; nsid++) {
		ns = spdk_nvme_ctrlr_get_ns(ctrlr, nsid);
		if (ns == NULL) {
			continue;
		}
		register_ns(ctrlr, ns);
	}
}

static void
execute_program_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct csd_sequence		*sequence = arg;
	struct ns_entry			*ns_entry = sequence->ns_entry;

	/* Assume the I/O was successful */
	sequence->is_completed = 1;

	/* See if an error occurred. If so, display information
	 * about it, and set completion value so that I/O
	 * caller is aware that an error occurred.
	 */
	if (spdk_nvme_cpl_is_error(completion)) {
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Write I/O failed, aborting run\n");
		sequence->is_completed = 2;
		exit(1);
	}
	/*
	 * The write I/O has completed.  Free the buffer associated with
	 *  the write I/O and allocate a new zeroed buffer for reading
	 *  the data back from the NVMe namespace.
	 */
	if (sequence->using_cmb_io) {
		spdk_nvme_ctrlr_unmap_cmb(ns_entry->ctrlr);
	} else {
		spdk_free(sequence->buf);
	}
}

static void
cleanup(void)
{
	struct ns_entry *ns_entry, *tmp_ns_entry;
	struct ctrlr_entry *ctrlr_entry, *tmp_ctrlr_entry;
	struct spdk_nvme_detach_ctx *detach_ctx = NULL;

	TAILQ_FOREACH_SAFE(ns_entry, &g_namespaces, link, tmp_ns_entry) {
		TAILQ_REMOVE(&g_namespaces, ns_entry, link);
		free(ns_entry);
	}

	TAILQ_FOREACH_SAFE(ctrlr_entry, &g_controllers, link, tmp_ctrlr_entry) {
		TAILQ_REMOVE(&g_controllers, ctrlr_entry, link);
		spdk_nvme_detach_async(ctrlr_entry->ctrlr, &detach_ctx);
		free(ctrlr_entry);
	}

	if (detach_ctx) {
		spdk_nvme_detach_poll(detach_ctx);
	}
}

static void usage(void)
{
	int i = 0;
	printf("NVMe Management Options");
	printf("\n");
	for (i = 0; i < SHELL_CMD_MAX; i++)
	{
		printf("\t[%d: %s]\n", i,  g_shell_content[i].desc);
	}
}

#if !FOR_ONLY_ONE_TEST_DEVICE
static void
display_namespace_dpc(const struct spdk_nvme_ns_data *nsdata)
{
	if (nsdata->dpc.pit1 || nsdata->dpc.pit2 || nsdata->dpc.pit3) {
		if (nsdata->dpc.pit1) {
			printf("PIT1 ");
		}

		if (nsdata->dpc.pit2) {
			printf("PIT2 ");
		}

		if (nsdata->dpc.pit3) {
			printf("PIT3 ");
		}
	} else {
		printf("Not Supported\n");
		return;
	}

	if (nsdata->dpc.md_start && nsdata->dpc.md_end) {
		printf("Location: Head or Tail\n");
	} else if (nsdata->dpc.md_start) {
		printf("Location: Head\n");
	} else if (nsdata->dpc.md_end) {
		printf("Location: Tail\n");
	} else {
		printf("Not Supported\n");
	}
}

static void
display_namespace(struct spdk_nvme_ns *ns)
{
	const struct spdk_nvme_ns_data		*nsdata;
	uint32_t				i;

	nsdata = spdk_nvme_ns_get_data(ns);

	printf("Namespace ID:%d\n", spdk_nvme_ns_get_id(ns));

	printf("Size (in LBAs):              %lld (%lldM)\n",
	       (long long)nsdata->nsze,
	       (long long)nsdata->nsze / 1024 / 1024);
	printf("Capacity (in LBAs):          %lld (%lldM)\n",
	       (long long)nsdata->ncap,
	       (long long)nsdata->ncap / 1024 / 1024);
	printf("Utilization (in LBAs):       %lld (%lldM)\n",
	       (long long)nsdata->nuse,
	       (long long)nsdata->nuse / 1024 / 1024);
	printf("Format Progress Indicator:   %s\n",
	       nsdata->fpi.fpi_supported ? "Supported" : "Not Supported");
	if (nsdata->fpi.fpi_supported && nsdata->fpi.percentage_remaining) {
		printf("Formatted Percentage:	%d%%\n", 100 - nsdata->fpi.percentage_remaining);
	}
	printf("Number of LBA Formats:       %d\n", nsdata->nlbaf + 1);
	printf("Current LBA Format:          LBA Format #%02d\n",
	       nsdata->flbas.format);
	for (i = 0; i <= nsdata->nlbaf; i++)
		printf("LBA Format #%02d: Data Size: %5d  Metadata Size: %5d\n",
		       i, 1 << nsdata->lbaf[i].lbads, nsdata->lbaf[i].ms);
	printf("Data Protection Capabilities:");
	display_namespace_dpc(nsdata);
	if (SPDK_NVME_FMT_NVM_PROTECTION_DISABLE == nsdata->dps.pit) {
		printf("Data Protection Setting:     N/A\n");
	} else {
		printf("Data Protection Setting:     PIT%d Location: %s\n",
		       nsdata->dps.pit, nsdata->dps.md_start ? "Head" : "Tail");
	}
	printf("Multipath IO and Sharing:    %s\n",
	       nsdata->nmic.can_share ? "Supported" : "Not Supported");
	printf("\n");
}

static void
display_controller(struct dev *dev, int model)
{
	struct spdk_nvme_ns			*ns;
	const struct spdk_nvme_ctrlr_data	*cdata;
	uint8_t					str[128];
	uint32_t				nsid;

	cdata = spdk_nvme_ctrlr_get_data(dev->ctrlr);

	if (model == CONTROLLER_DISPLAY_SIMPLISTIC) {
		printf("%04x:%02x:%02x.%02x ",
		       dev->pci_addr.domain, dev->pci_addr.bus, dev->pci_addr.dev, dev->pci_addr.func);
		printf("%-40.40s %-20.20s ",
		       cdata->mn, cdata->sn);
		printf("%5d ", cdata->cntlid);
		printf("\n");
		return;
	}

	printf("=====================================================\n");
	printf("NVMe Controller:	%04x:%02x:%02x.%02x\n",
	       dev->pci_addr.domain, dev->pci_addr.bus, dev->pci_addr.dev, dev->pci_addr.func);
	printf("============================\n");
	printf("Controller Capabilities/Features\n");
	printf("Controller ID:		%d\n", cdata->cntlid);
	snprintf(str, sizeof(cdata->sn) + 1, "%s", cdata->sn);
	printf("Serial Number:		%s\n", str);
	printf("\n");

	printf("Admin Command Set Attributes\n");
	printf("============================\n");
	printf("Namespace Manage And Attach:		%s\n",
	       cdata->oacs.ns_manage ? "Supported" : "Not Supported");
	printf("Namespace Format:			%s\n",
	       cdata->oacs.format ? "Supported" : "Not Supported");
	printf("\n");
	printf("NVM Command Set Attributes\n");
	printf("============================\n");
	if (cdata->fna.format_all_ns) {
		printf("Namespace format operation applies to all namespaces\n");
	} else {
		printf("Namespace format operation applies to per namespace\n");
	}
	printf("\n");
	printf("Namespace Attributes\n");
	printf("============================\n");
	for (nsid = spdk_nvme_ctrlr_get_first_active_ns(dev->ctrlr);
	     nsid != 0; nsid = spdk_nvme_ctrlr_get_next_active_ns(dev->ctrlr, nsid)) {
		ns = spdk_nvme_ctrlr_get_ns(dev->ctrlr, nsid);
		assert(ns != NULL);
		display_namespace(ns);
	}
}
#endif

static char *
get_line(char *buf, int buf_size, FILE *f, bool secret)
{
	char *ch;
	size_t len;
	struct termios default_attr = {}, new_attr = {};
	int ret;

	if (secret) {
		ret = tcgetattr(STDIN_FILENO, &default_attr);
		if (ret) {
			return NULL;
		}

		new_attr = default_attr;
		new_attr.c_lflag &= ~ECHO;  /* disable echo */
		ret = tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_attr);
		if (ret) {
			return NULL;
		}
	}

	ch = fgets(buf, buf_size, f);
	if (ch == NULL) {
		return NULL;
	}

	if (secret) {
		ret = tcsetattr(STDIN_FILENO, TCSAFLUSH, &default_attr); /* restore default confing */
		if (ret) {
			return NULL;
		}
	}

	len = strlen(buf);
	if (len > 0 && buf[len - 1] == '\n') {
		buf[len - 1] = '\0';
	}
	return buf;
}

#if !FOR_ONLY_ONE_TEST_DEVICE
static struct dev *
get_controller(void)
{
	struct spdk_pci_addr			pci_addr;
	char					address[64];
	char					*p;
	int					ch;
	struct dev				*iter;

	memset(address, 0, sizeof(address));

	foreach_dev(iter) {
		display_controller(iter, CONTROLLER_DISPLAY_SIMPLISTIC);
	}

	printf("Please Input PCI Address(domain:bus:dev.func):\n");

	while ((ch = getchar()) != '\n' && ch != EOF);
	p = get_line(address, 64, stdin, false);
	if (p == NULL) {
		return NULL;
	}

	while (isspace(*p)) {
		p++;
	}

	if (spdk_pci_addr_parse(&pci_addr, p) < 0) {
		return NULL;
	}

	foreach_dev(iter) {
		if (spdk_pci_addr_compare(&pci_addr, &iter->pci_addr) == 0) {
			return iter;
		}
	}
	return NULL;
}
#endif

static void
args_usage(const char *program_name)
{
	printf("%s [options]", program_name);
	printf("\n");
	printf("options:\n");
	printf(" -i         shared memory group ID\n");
}

static int
parse_args(int argc, char **argv)
{
	int op;

	while ((op = getopt(argc, argv, "i:")) != -1) {
		switch (op) {
		case 'i':
			g_shm_id = spdk_strtol(optarg, 10);
			if (g_shm_id < 0) {
				fprintf(stderr, "Invalid shared memory ID\n");
				return g_shm_id;
			}
			break;
		default:
			args_usage(argv[0]);
			return 1;
		}
	}

	return 0;
}

static void nvme_csd_program_activation(void)
{
	int					rc;
	int 					compute_engine_id;
	int 					program_id;
	int 					action;
	struct dev				*ctrlr;
	struct spdk_nvme_status			status;

#if FOR_ONLY_ONE_TEST_DEVICE
	ctrlr = devs;
#else
	ctrlr = get_controller();
	if (ctrlr == NULL) {
		printf("Invalid controller PCI BDF.\n");
		return;
	}
#endif

	printf("Please input compute engine identifier :\n");
	if (!scanf("%d", &compute_engine_id)) {
		printf("Invalid compute engine identifier\n");
		while (getchar() != '\n');
		return;
	}

	printf("Please input program identifier :\n");
	if (!scanf("%d", &program_id)) {
		printf("Invalid program identifier\n");
		while (getchar() != '\n');
		return;
	}

	printf("Please input action :\n");
	if (!scanf("%d", &action)) {
		printf("Invalid action\n");
		while (getchar() != '\n');
		return;
	}

	rc = spdk_nvme_csd_ctrlr_program_activate(ctrlr->ctrlr,
						compute_engine_id, program_id, action,
						&status);
	if (rc) {
		printf("spdk_nvme_csd_ctrlr_program_activate failed\n");
	} else {
		printf("spdk_nvme_csd_ctrlr_program_activate success\n");
	}
}

static void nvme_csd_execute_program(void)
{
	int					rc;
	int					fd = -1;
	int					ns_id = 1, ns_idx;
	int					ce_id = 0;
	int					p_id = 0;
	int					rs_id = 0;
	void					*c_param = NULL;
	size_t					buffer_size;
	size_t					size;
	struct stat				data_stat;
	char					path[256] = "/home/joshua/test_bin.bin";
	struct ns_entry				*ns_entry;
	struct csd_sequence			sequence;
	int					ch;

	printf("Please Input The Path Of data\n");

	while ((ch = getchar()) != '\n' && ch != EOF);
	if (get_line(path, sizeof(path), stdin, false) == NULL) {
		printf("Invalid path setting\n");
		while (getchar() != '\n');
		return;
	}

	printf("path:%s\r\n", path);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Open file failed");
		goto _ERR_OUT;
	}
	rc = fstat(fd, &data_stat);
	if (rc < 0) {
		printf("Fstat failed\n");
		goto _ERR_OUT;
	}

	if (data_stat.st_size % 4) {
		printf("data size is not multiple of 4\n");
		goto _ERR_OUT;
	}
	
	size = data_stat.st_size;

	printf("Please input NS ID:\n");
	if (!scanf("%d", &ns_id)) {
		printf("Invalid NS ID\n");
		while (getchar() != '\n');
		goto _ERR_OUT;
	}
	if ((ns_id < NS_START_IDX) || (ns_id >= (NS_START_IDX + NUM_NS)))
	{
		printf("Invalid NS ID\n");
		while (getchar() != '\n');
		goto _ERR_OUT;
	}

	printf("Please input computational engine ID :\n");
	if (!scanf("%d", &ce_id)) {
		printf("Invalid computational engine ID\n");
		while (getchar() != '\n');
		goto _ERR_OUT;
	}
	if (ce_id >= NUM_CE)
	{
		printf("Invalid computational engine ID\n");
		while (getchar() != '\n');
		goto _ERR_OUT;
	}

	printf("Please input program ID(0 - 3):\n");
	if (!scanf("%d", &p_id)) {
		printf("Invalid program ID\n");
		while (getchar() != '\n');
		goto _ERR_OUT;
	}
	if (p_id >= NUM_PROGRAM)
	{
		printf("Invalid program ID\n");
		while (getchar() != '\n');
		goto _ERR_OUT;
	}

	printf("Please input memory range set:\n");
	if (!scanf("%d", &rs_id)) {
		printf("Invalid memory range set\n");
		while (getchar() != '\n');
		goto _ERR_OUT;
	}
	if (rs_id >= NUM_MEM_RANGE_SET)
	{
		printf("Invalid memory range set\n");
		while (getchar() != '\n');
		goto _ERR_OUT;
	}

	c_param = spdk_dma_zmalloc(sizeof(uint32_t) * 6, 4, NULL);
	if (c_param == NULL) {
		printf("Allocation error\n");
		goto _ERR_OUT;
	}

	ns_entry = TAILQ_FIRST(&g_namespaces);
	for (ns_idx = NS_START_IDX; ns_idx < ns_id; ns_idx++)
	{
		if (!ns_entry) {
			printf("Invalid NS ID\n");
			goto _ERR_OUT;
		}
		ns_entry = TAILQ_NEXT(ns_entry, link);
	}

	/*
		* Allocate an I/O qpair that we can use to submit read/write requests
		*  to namespaces on the controller.  NVMe controllers typically support
		*  many qpairs per controller.  Any I/O qpair allocated for a controller
		*  can submit I/O to any namespace on that controller.
		*
		* The SPDK NVMe driver provides no synchronization for qpair accesses -
		*  the application must ensure only a single thread submits I/O to a
		*  qpair, and that same thread must also check for completions on that
		*  qpair.  This enables extremely efficient I/O processing by making all
		*  I/O operations completely lockless.
		*/
	ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(ns_entry->ctrlr, NULL, 0);
	if (ns_entry->qpair == NULL) {
		printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
		goto _ERR_OUT;
	}

	/*
		* Use spdk_dma_zmalloc to allocate a 4KB zeroed buffer.  This memory
		* will be pinned, which is required for data buffers used for SPDK NVMe
		* I/O operations.
		*/
	sequence.using_cmb_io = 1;
	sequence.buf = spdk_nvme_ctrlr_map_cmb(ns_entry->ctrlr, &buffer_size);
	if (sequence.buf == NULL || buffer_size < 0x1000) {
		sequence.using_cmb_io = 0;
		buffer_size = 0x1000;
		sequence.buf = spdk_zmalloc(buffer_size, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
	}

	if (sequence.buf == NULL) {
		printf("ERROR: write buffer allocation failed\n");
		goto _ERR_OUT;
	}
	if (sequence.using_cmb_io) {
		printf("INFO: using controller memory buffer for IO\n");
	} else {
		printf("INFO: using host memory buffer for IO\n");
	}

	sequence.is_completed = 0;
	sequence.ns_entry = ns_entry;

	if (read(fd, sequence.buf, size) != ((ssize_t)(size))) {
		printf("Read data failed\n");
		goto _ERR_OUT;
	}
	
	rc = spdk_nvme_csd_ctrlr_execute_program(ns_entry->ns, ns_entry->qpair, 
						ce_id, p_id, rs_id,
						sequence.buf, size, c_param, 
						execute_program_complete, &sequence, 0);

	if (rc != 0) {
		fprintf(stderr, "starting write I/O failed\n");
		goto _ERR_OUT;
	}
	/*
		* Poll for completions.  0 here means process all available completions.
		*  In certain usage models, the caller may specify a positive integer
		*  instead of 0 to signify the maximum number of completions it should
		*  process.  This function will never block - if there are no
		*  completions pending on the specified qpair, it will return immediately.
		*
		* When the write I/O completes, write_complete() will submit a new I/O
		*  to read LBA 0 into a separate buffer, specifying read_complete() as its
		*  completion routine.  When the read I/O completes, read_complete() will
		*  print the buffer contents and set sequence.is_completed = 1.  That will
		*  break this loop and then exit the program.
		*/
	while (!sequence.is_completed) {
		spdk_nvme_qpair_process_completions(ns_entry->qpair, 0);
	}

	/*
		* Free the I/O qpair.  This typically is done when an application exits.
		*  But SPDK does support freeing and then reallocating qpairs during
		*  operation.  It is the responsibility of the caller to ensure all
		*  pending I/O are completed before trying to free the qpair.
		*/
	spdk_nvme_ctrlr_free_io_qpair(ns_entry->qpair);

_ERR_OUT:
	if (fd)
	{
		close(fd);
	}
	if (c_param)
	{
		spdk_dma_free(c_param);
	}
	return;
}

static void nvme_csd_load_program(void)
{
	int					rc;
	int					fd = -1;
	int 					unload;
	int 					program_type;
	int					program_id;
	unsigned int				size;
	struct stat				program_stat;
	char					path[256];
	void					*program_image;
	struct dev				*ctrlr;
	struct spdk_nvme_status			status;
	int					ch;

#if FOR_ONLY_ONE_TEST_DEVICE
	ctrlr = devs;
#else
	ctrlr = get_controller();
	if (ctrlr == NULL) {
		printf("Invalid controller PCI BDF.\n");
		return;
	}
#endif

	printf("Please Input The Path Of program Image\n");

	while ((ch = getchar()) != '\n' && ch != EOF);
	if (get_line(path, sizeof(path), stdin, false) == NULL) {
		printf("Invalid path setting\n");
		while (getchar() != '\n');
		return;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Open file failed");
		return;
	}
	rc = fstat(fd, &program_stat);
	if (rc < 0) {
		printf("Fstat failed\n");
		close(fd);
		return;
	}

	if (program_stat.st_size % 4) {
		printf("program image size is not multiple of 4\n");
		close(fd);
		return;
	}
	
	size = program_stat.st_size;

	program_image = spdk_dma_zmalloc(size, 4096, NULL);
	if (program_image == NULL) {
		printf("Allocation error\n");
		close(fd);
		return;
	}

	if (read(fd, program_image, size) != ((ssize_t)(size))) {
		printf("Read program image failed\n");
		close(fd);
		spdk_dma_free(program_image);
		return;
	}
	close(fd);

	printf("Please input unload setting :\n");
	if (!scanf("%d", &unload)) {
		printf("Invalid unload setting\n");
		spdk_dma_free(program_image);
		while (getchar() != '\n');
		return;
	}

	printf("Please input program type :\n");
	if (!scanf("%d", &program_type)) {
		printf("Invalid program type\n");
		spdk_dma_free(program_image);
		while (getchar() != '\n');
		return;
	}

	printf("Please input program ID(0 - 3):\n");
	if (!scanf("%d", &program_id)) {
		printf("Invalid program ID\n");
		spdk_dma_free(program_image);
		while (getchar() != '\n');
		return;
	}

	rc = spdk_nvme_csd_ctrlr_load_program(ctrlr->ctrlr, program_image, size, 
						unload, program_type, program_id, &status);
	if (rc) {
		printf("spdk_nvme_csd_ctrlr_load_program failed\n");
	} else {
		printf("spdk_nvme_csd_ctrlr_load_program success\n");
	}
	spdk_dma_free(program_image);
}

static void nvme_csd_create_memory_range_set(void)
{
	int					rc;
	int					fd = -1;
	int 					number_memory_ranges;
	unsigned int				size;
	struct stat				definition_stat;
	char					path[256];
	void					*definition_image;
	struct dev				*ctrlr;
	struct spdk_nvme_status			status;
	int					ch;

#if FOR_ONLY_ONE_TEST_DEVICE
	ctrlr = devs;
#else
	ctrlr = get_controller();
	if (ctrlr == NULL) {
		printf("Invalid controller PCI BDF.\n");
		return;
	}
#endif

	printf("Please Input The Path Of memory ranges definition Image\n");

	while ((ch = getchar()) != '\n' && ch != EOF);
	if (get_line(path, sizeof(path), stdin, false) == NULL) {
		printf("Invalid path setting\n");
		while (getchar() != '\n');
		return;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Open file failed");
		return;
	}
	rc = fstat(fd, &definition_stat);
	if (rc < 0) {
		printf("Fstat failed\n");
		close(fd);
		return;
	}

	if (definition_stat.st_size % sizeof(struct spdk_csd_memory_range_definition)) {
		printf("program image size is not multiple of spdk_csd_memory_range_definition\n");
		close(fd);
		return;
	}
	
	size = definition_stat.st_size;

	definition_image = spdk_dma_zmalloc(size, 4096, NULL);
	if (definition_image == NULL) {
		printf("Allocation error\n");
		close(fd);
		return;
	}

	if (read(fd, definition_image, size) != ((ssize_t)(size))) {
		printf("Read definition image failed\n");
		close(fd);
		spdk_dma_free(definition_image);
		return;
	}
	close(fd);

	printf("Please input number memory range :\n");
	if (!scanf("%d", &number_memory_ranges)) {
		printf("Invalid number memory range\n");
		spdk_dma_free(definition_image);
		while (getchar() != '\n');
		return;
	}

	rc = spdk_nvme_csd_ctrlr_create_memory_range_set(ctrlr->ctrlr,
						number_memory_ranges, definition_image, 
						&status);
	if (rc) {
		printf("spdk_nvme_csd_ctrlr_create_memory_range_set failed\n");
	} else {
		printf("spdk_nvme_csd_ctrlr_create_memory_range_set success\n");
	}
	spdk_dma_free(definition_image);
}

static void nvme_csd_delete_memory_range_set(void)
{
	int					rc;
	int 					ranges_set_id;
	struct dev				*ctrlr;
	struct spdk_nvme_status			status;

#if FOR_ONLY_ONE_TEST_DEVICE
	ctrlr = devs;
#else
	ctrlr = get_controller();
	if (ctrlr == NULL) {
		printf("Invalid controller PCI BDF.\n");
		return;
	}
#endif

	printf("Please input memory range set identifier :\n");
	if (!scanf("%d", &ranges_set_id)) {
		printf("Invalid memory range set identifier\n");
		while (getchar() != '\n');
		return;
	}

	rc = spdk_nvme_csd_ctrlr_delete_memory_range_set(ctrlr->ctrlr,
						ranges_set_id, 
						&status);
	if (rc) {
		printf("spdk_nvme_csd_ctrlr_create_memory_range_set failed\n");
	} else {
		printf("spdk_nvme_csd_ctrlr_create_memory_range_set success\n");
	}
}

static void nvme_csd_get_program_information_log(void)
{
	int					rc;
	struct dev				*ctrlr;
	struct spdk_csd_program_information_log * p_program_info_log;
	size_t size;
	uint32_t number_of_records;

#if FOR_ONLY_ONE_TEST_DEVICE
	ctrlr = devs;
#else
	ctrlr = get_controller();
	if (ctrlr == NULL) {
		printf("Invalid controller PCI BDF.\n");
		return;
	}
#endif

	size = sizeof(p_program_info_log->number_of_records);
	size = SPDK_ALIGN_CEIL(size, sizeof(uint32_t));
	p_program_info_log = spdk_zmalloc(size, 1, NULL,
						SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	if (p_program_info_log == NULL)
	{
		printf("could not allocate log page buffer(size:%ld)\n", size);
		return ;
	}

	rc = nvme_csd_get_log_page(ctrlr->ctrlr,
				SPDK_CSD_LOG_PROGRAM_INFORMATION,
				p_program_info_log, size);
	if (rc) {
		printf("spdk_nvme_csd_ctrlr_get_log_page failed (err:%d)\n", rc);
	} else {
		printf("spdk_nvme_csd_ctrlr_get_log_page success\n");
	}

	number_of_records = p_program_info_log->number_of_records;

	printf("number_of_records:%d\r\n", number_of_records);

	spdk_free(p_program_info_log);

	if (number_of_records != 0)
	{
		size = sizeof(struct spdk_csd_program_information_data) * (number_of_records + 1);
		size = SPDK_ALIGN_CEIL(size, sizeof(uint32_t));
		p_program_info_log = spdk_zmalloc(size, 1, NULL,
							SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

		if (p_program_info_log == NULL)
		{
			printf("could not allocate log page buffer(size:%ld)\n", size);
			return ;
		}

		rc = nvme_csd_get_log_page(ctrlr->ctrlr,
					SPDK_CSD_LOG_PROGRAM_INFORMATION,
					p_program_info_log, size);
		if (rc) {
			printf("spdk_nvme_csd_ctrlr_get_log_page failed (err:%d)\n", rc);
		} else {
			printf("spdk_nvme_csd_ctrlr_get_log_page success\n");
		}

		uint32_t i, j;
		struct spdk_csd_program_information_data * p_program_info_data = 
			&p_program_info_log->program_record[0];
		for (i = 0; i < number_of_records; i++)
		{
			printf("program_entry_type:%d, program_entry_occupied:%d\r\n",
				p_program_info_data->program_entry_type, p_program_info_data->program_entry_occupied);

			printf("number_of_associated_compute_engines:%d\r\n", 
				p_program_info_data->number_of_associated_compute_engines);

			for (j = 0; j < p_program_info_data->number_of_associated_compute_engines; j++)
			{	
				printf("activation:%d, compute_engine_identifier:%d\r\n",
					p_program_info_data->associated_compute_engine_descriptor[j].activation,
					p_program_info_data->associated_compute_engine_descriptor[j].compute_engine_identifier);
			}
			
			printf("program_type:%d\r\n", p_program_info_data->program_type);

			printf("UUID\r\n");
			for (j = 0; j < UUID_BYTE_SIZE; j++)
			{
				printf("%02X", p_program_info_data->program_uuid[j]);
			}
			printf("\r\n");

			p_program_info_data++;
		}
		spdk_free(p_program_info_log);
	}
}

static void nvme_csd_get_compute_engine_information(void)
{
	int					rc;
	struct dev				*ctrlr;
	struct spdk_csd_controller_memory_region * p_controller_memory_region;
	size_t size;
	int i;

#if FOR_ONLY_ONE_TEST_DEVICE
	ctrlr = devs;
#else
	ctrlr = get_controller();
	if (ctrlr == NULL) {
		printf("Invalid controller PCI BDF.\n");
		return;
	}
#endif

	size = sizeof(struct spdk_csd_controller_memory_region);
	size = SPDK_ALIGN_CEIL(size, sizeof(uint32_t));
	printf("allocate log page buffer(size:%ld)\n", size);
	p_controller_memory_region = spdk_zmalloc(size, 1, NULL,
						SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	if (p_controller_memory_region == NULL)
	{
		printf("could not allocate log page buffer(size:%ld)\n", size);
		return ;
	}

	rc = nvme_csd_get_log_page(ctrlr->ctrlr,
				SPDK_CSD_LOG_COMPUTE_ENGINE_INFORMATION,
				p_controller_memory_region, size);
	if (rc) {
		printf("spdk_nvme_csd_ctrlr_get_log_page failed (err:%d)\n", rc);
	} else {
		printf("spdk_nvme_csd_ctrlr_get_log_page success\n");
	}

	printf("number_of_controller_memory_regions:%d\r\n", p_controller_memory_region->number_of_controller_memory_regions);

	for (i = 0; i < p_controller_memory_region->number_of_controller_memory_regions; i++)
	{
		struct spdk_csd_memory_region_descriptor * p_memory_region =
			(struct spdk_csd_memory_region_descriptor *)&p_controller_memory_region->memory_region[i];
		printf("[%d]memory_region_id:%d\r\n", i, p_memory_region->memory_region_id);
	}

	printf("number_of_supported_downloadable_program_types:%d\r\n", p_controller_memory_region->number_of_supported_downloadable_program_types);

	for (i = 0; i < p_controller_memory_region->number_of_supported_downloadable_program_types; i++)
	{
		printf("[%d]program_type_identifier:%d\r\n", i, p_controller_memory_region->program_type_identifier[i]);
	}

	printf("engine charactreistics :\r\n");
	for (i = 0; i < ENGINE_CHARACTERISTICS_BYTE_SIZE; i++)
	{
		printf("%02X", p_controller_memory_region->general_engine_characteristics[i]);
	}
	printf("\r\n");

	spdk_free(p_controller_memory_region);
}

static void nvme_csd_get_compute_engine_list(void)
{
	int					rc;
	struct dev				*ctrlr;
	struct spdk_csd_compute_engine_list * p_compute_engine_list;
	size_t size;
	uint32_t number_of_compute_engine;

#if FOR_ONLY_ONE_TEST_DEVICE
	ctrlr = devs;
#else
	ctrlr = get_controller();
	if (ctrlr == NULL) {
		printf("Invalid controller PCI BDF.\n");
		return;
	}
#endif

	size = sizeof(p_compute_engine_list->number_of_compute_engine);
	size = SPDK_ALIGN_CEIL(size, sizeof(uint32_t));
	p_compute_engine_list = spdk_zmalloc(size, 1, NULL,
						SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

	if (p_compute_engine_list == NULL)
	{
		printf("could not allocate log page buffer(size:%ld)\n", size);
		return ;
	}

	rc = nvme_csd_get_log_page(ctrlr->ctrlr,
				SPDK_CSD_LOG_COMPUTE_ENGINE_LIST,
				p_compute_engine_list, size);
	if (rc) {
		printf("spdk_nvme_csd_ctrlr_get_log_page failed (err:%d)\n", rc);
	} else {
		printf("spdk_nvme_csd_ctrlr_get_log_page success\n");
	}

	number_of_compute_engine = p_compute_engine_list->number_of_compute_engine;

	printf("number_of_compute_engine:%d\r\n", number_of_compute_engine);

	spdk_free(p_compute_engine_list);

	if (number_of_compute_engine != 0)
	{
		size = 8 + (sizeof(uint16_t) * number_of_compute_engine);
		size = SPDK_ALIGN_CEIL(size, sizeof(uint32_t));
		p_compute_engine_list = spdk_zmalloc(size, 1, NULL,
							SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);

		if (p_compute_engine_list == NULL)
		{
			printf("could not allocate log page buffer(size:%ld)\n", size);
			return ;
		}

		rc = nvme_csd_get_log_page(ctrlr->ctrlr,
					SPDK_CSD_LOG_COMPUTE_ENGINE_LIST,
					p_compute_engine_list, size);
		if (rc) {
			printf("spdk_nvme_csd_ctrlr_get_log_page failed (err:%d)\n", rc);
		} else {
			printf("spdk_nvme_csd_ctrlr_get_log_page success\n");
		}

		uint32_t i;
		for (i = 0; i < p_compute_engine_list->number_of_compute_engine; i++)
		{
			printf("[%d]compute_engine_identifier:%d\r\n",
				i, p_compute_engine_list->compute_engine_identifier[i]);
		}
		spdk_free(p_compute_engine_list);
	}
}

static int
nvme_csd_get_log_page(struct spdk_nvme_ctrlr *ctrlr, 
			uint8_t log_page, 
			void *payload, uint32_t payload_size)
{
	int rc;
	uint64_t offset;
	uint64_t tansfer_byte;
	uint64_t max_transfer_byte;
	const struct spdk_nvme_ctrlr_data *cdata;
	cdata = spdk_nvme_ctrlr_get_data(ctrlr);

	max_transfer_byte = cdata->lpa.edlp ? 0x10000 : 0x100;

	offset = 0;
	while (payload_size > 0)
	{
		tansfer_byte = spdk_min(max_transfer_byte, payload_size);

		rc = spdk_nvme_csd_ctrlr_get_log_page(ctrlr,
							log_page, 0,
							payload + offset, tansfer_byte,
							offset);
		if (rc) {
			return rc;
		}
		offset += tansfer_byte;
		payload_size -= tansfer_byte;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int			rc;
	struct spdk_env_opts	opts;
	struct dev		*dev;
	struct spdk_nvme_detach_ctx *detach_ctx = NULL;

	rc = parse_args(argc, argv);
	if (rc != 0) {
		return rc;
	}

	spdk_env_opts_init(&opts);
	opts.name = "nvme_manage";
	opts.core_mask = "0x1";
	opts.shm_id = g_shm_id;
	if (spdk_env_init(&opts) < 0) {
		fprintf(stderr, "Unable to initialize SPDK env\n");
		return 1;
	}

	if (spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL) != 0) {
		fprintf(stderr, "spdk_nvme_probe() failed\n");
		return 1;
	}

	if (TAILQ_EMPTY(&g_controllers)) {
		fprintf(stderr, "no NVMe controllers found\n");
		cleanup();
		return 1;
	}
	printf("Initialization complete.\n");

	qsort(devs, num_devs, sizeof(devs[0]), cmp_devs);

	usage();

	while (1) {
		int cmd;
		g_exit_flag = false;

		if (!scanf("%d", &cmd)) {
			printf("Invalid Command: command must be number 0-%d\n", SHELL_CMD_MAX);
			while (getchar() != '\n');
			usage();
			continue;
		}

		if (cmd < SHELL_CMD_MAX)
		{
			g_shell_content[cmd].funcPtr();
		}
		else
		{
			printf("Invalid Command\n");
		}

		if (g_exit_flag) {
			break;
		}

		while (getchar() != '\n');
		printf("press Enter to display cmd menu ...\n");
		while (getchar() != '\n');
		usage();
	}

	printf("Cleaning up...\n");

	foreach_dev(dev) {
		spdk_nvme_detach_async(dev->ctrlr, &detach_ctx);
	}

	if (detach_ctx) {
		spdk_nvme_detach_poll(detach_ctx);
	}

	return 0;
}
