#!/usr/bin python3
# Usage: sudo python3 send_test_frame.py enp1s0

import sys
from scapy.all import Ether, sendp

if len(sys.argv) < 2:
    print("Usage: sudo python3 send_test_frame.py <interface>")
    sys.exit(1)

iface = sys.argv[1]

payload = b"TamgaOS!" + b"\x00" * 16

frame = Ether(
    dst="02:00:00:00:00:01",   # board's MAC (matches eth_init's config)
    src="02:00:00:00:00:02",   # arbitrary PC-side MAC
    type=0x88B5
) / payload

print(f"Sending frame on {iface}: dst=02:00:00:00:00:01 type=0x88B5")
sendp(frame, iface=iface, count=5, inter=1.0, verbose=True)
print("Done — sent 5 frames, 1 per second.")