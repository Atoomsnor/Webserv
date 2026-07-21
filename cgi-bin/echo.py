#!/usr/bin/env python3
import os, sys, hashlib

n = int(os.environ.get("CONTENT_LENGTH", 0))
data = sys.stdin.buffer.read(n)          # read exactly CONTENT_LENGTH bytes

print("Content-Type: text/plain")
print()
print("received", len(data), "bytes")
print("md5", hashlib.md5(data).hexdigest())