
#!/usr/bin/env bash

. ./env_common.sh

bdevperf_socket=$cs_dev_dir/spdk_bdevperf_virtio.sock
bdevperf_blk_config=$test_dir/virtio_blk.json
bdevperf_scsi_config=$test_dir/virtio_scsi.json
bdevperf=$root_dir/test/bdev/bdevperf/bdevperf

if [ $# -lt 1 ]; then
	echo "usage: $0 <1/0> <b/s>"
	echo "1: start bdevperf"
	echo "0: stop bdevperf"
	echo "b: virtio_blk"
	echo "s: virtio_scsi"
	echo ""
	exit 1
fi

if [ $1 -eq 1 ]; then
	if [ $# -lt 2 ]; then
		echo "usage: $0 <1/0> <b/s>"
		echo "1: start bdevperf"
		echo "0: stop bdevperf"
		echo "b: virtio_blk"
		echo "s: virtio_scsi"
		echo ""
		exit 1	
	fi

	if [ "$2" = "b" ]; then
		bdevperf_config=$bdevperf_blk_config
	else
		bdevperf_config=$bdevperf_scsi_config
	fi
	$bdevperf -q 128 -o 4096 -w verify -t 10 -z -r $bdevperf_socket --config $bdevperf_config -m 0x2 -g&

	export PYTHONPATH=$PYTHONPATH:$root_dir/scripts/
	sleep 3
	$root_dir/test/bdev/bdevperf/bdevperf.py -s $bdevperf_socket -t 1000 perform_tests

else
	$rpc_py -s $bdevperf_socket spdk_kill_instance SIGTERM
fi