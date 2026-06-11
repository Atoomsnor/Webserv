#!/usr/bin/env python3

import time
import sys

# Output HTTP headers
print("Content-Type: text/plain")
print("Content-Length: 1000000")
print("")
sys.stdout.flush()

# Infinite loop
while True:
    print("test")
    sys.stdout.flush()
    time.sleep(1)