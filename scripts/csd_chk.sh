#!/usr/bin/env bash

function add_csd_to_allowed_list() {
	for nvme_dev in $(ls /dev/nvme* 2>/dev/null | grep -E 'nvme*[0-9]$'); do		
		if ./nvme cp-id-ctrl ${nvme_dev} > /dev/null 2>&1; then
			echo "find csd:${nvme_dev}"
			NVME_ALLOWED+="$(cat /sys/class/nvme/${nvme_dev##*/}/address) "
		fi
	done
	echo "allowed bdf list:$NVME_ALLOWED"
}

add_csd_to_allowed_list