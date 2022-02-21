
#!/usr/bin/env bash

. ./env_common.sh

shell_app_socket=$cs_dev_dir/spdk_shell_virtio_blk.sock
shell_app_config=$test_dir/virtio_blk.json
shell_app=$root_dir/build/examples/cs_shell

$shell_app --rpc-socket $shell_app_socket --config $shell_app_config -g