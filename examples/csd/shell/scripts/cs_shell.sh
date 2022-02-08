
#!/usr/bin/env bash

. ./env_common.sh

cs_shell_app_socket=$cs_dev_dir/spdk_cs_shell.sock
cs_shell_app_config=$test_dir/cs_shell.json
cs_shell_app=$root_dir/build/examples/cs_shell

$cs_shell_app --rpc-socket $cs_shell_app_socket --config $cs_shell_app_config -g