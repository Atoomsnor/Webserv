#!/usr/bin/python3

import sys

# Read from stdin
input_string = sys.stdin.read()

# Convert to uppercase
output_string = input_string.upper()

num = 1 / 0

# Output HTTP headers
print("Content-Type: text/plain", end='\r\n')
print("Content-Length: {}".format(len(output_string)), end='\r\n')
print("", end='\r\n')

# Output the converted string
print(output_string, end='')