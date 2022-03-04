
#!/usr/bin/env bash

. ./env_common.sh

shell_app_socket=$cs_dev_dir/spdk_virtio_scsi.sock
shell_app_config=$test_dir/virtio_scsi.json
shell_app=$root_dir/build/examples/cs_shell

$shell_app --rpc-socket $shell_app_socket --config $shell_app_config -g