
#!/usr/bin/env bash

. ./env_common.sh

vhost_app_socket=$cs_dev_dir/spdk_vhost_scsi.sock
vhost_app_config=$test_dir/vhost_scsi.json
vhost_app=$root_dir/build/bin/vhost

if [ $# -lt 1 ]; then
	echo "usage: $0 <1/0>"
	echo "1: start vhost scsi"
	echo "0: stop vhost scsi"
	echo ""
	exit 1
fi

if [ $1 -eq 1 ]; then
	$vhost_app --rpc-socket $vhost_app_socket -S $cs_dev_dir --shm-id 0 --config $vhost_app_config &
else
	$rpc_py -s $vhost_app_socket spdk_kill_instance SIGTERM
fi
