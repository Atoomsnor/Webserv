#!/usr/bin/env python3
import os

CGI_VARS = [
    "AUTH_TYPE", "CONTENT_LENGTH", "CONTENT_TYPE", "GATEWAY_INTERFACE",
    "PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING", "REMOTE_ADDR",
    "REMOTE_HOST", "REMOTE_IDENT", "REMOTE_USER", "REQUEST_METHOD",
    "SCRIPT_NAME", "SERVER_NAME", "SERVER_PORT", "SERVER_PROTOCOL",
    "SERVER_SOFTWARE", "SCRIPT_FILENAME",
]

print("Content-Type: text/plain\r")
print("\r")

print("=== Standard CGI vars ===")
for var in CGI_VARS:
    print(f"{var}={os.environ.get(var, '<missing>')}")

print("\n=== HTTP_* vars ===")
for key, val in os.environ.items():
    if key.startswith("HTTP_"):
        print(f"{key}={val}")

print("\n=== All env ===")
for key, val in os.environ.items():
    print(f"{key}={val}")
