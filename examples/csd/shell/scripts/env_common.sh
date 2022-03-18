
#!/usr/bin/env bash

test_dir=$(readlink -f $(dirname $0))
root_dir=$(readlink -f $test_dir/../../../..)
script_dir=$root_dir/scripts
rpc_py=$script_dir/rpc.py
cs_dev_dir=$(readlink -f /tmp/cs_dev)

if [ ! -d "$cs_dev_dir" ]; then
	echo "create directory $cs_dev_dir..."
	mkdir $cs_dev_dir
fi
