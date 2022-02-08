
#!/usr/bin/env bash

. ./env_common.sh

vhost_app_socket=$cs_dev_dir/spdk_vhost.sock
vhost_app_config=$test_dir/vhost.json
vhost_app=$root_dir/build/bin/vhost

$vhost_app --rpc-socket $vhost_app_socket -S $cs_dev_dir --shm-id 0 --config $vhost_app_config &
