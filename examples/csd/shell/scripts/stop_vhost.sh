
#!/usr/bin/env bash

. ./env_common.sh

vhost_app_socket=$cs_dev_dir/spdk_vhost.sock
vhost_app_config=$test_dir/vhost.json
vhost_app=$root_dir/build/bin/vhost

$rpc_py -s $vhost_app_socket spdk_kill_instance SIGTERM
