#!/usr/bin/env bash

function scan_nvme_csx() {
	for uio_dev in $(ls /sys/class/uio/ 2>/dev/null); do
		bdf=$(readlink -f /sys/class/uio/${uio_dev})
		bdf=${bdf%%/uio/$uio_dev}
		bdf=nvme${uio_dev##*uio}-PCIe-${bdf##*/}
		echo "$bdf"		
	done
}

scan_nvme_csx