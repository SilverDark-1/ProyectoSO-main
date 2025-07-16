#!/bin/bash

# Start QEMU with our OS kernel
echo "Starting Pepin OS..."
timeout 10 qemu-system-i386 -kernel kernel -m 64M -nographic -no-reboot -serial mon:stdio 2>&1 | tee os_test.log

echo "OS test completed. Check os_test.log for output."