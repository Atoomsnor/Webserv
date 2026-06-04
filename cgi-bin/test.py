import os
import sys

method = os.environ.get("REQUEST_METHOD", "")
query  = os.environ.get("QUERY_STRING", "")
clen   = int(os.environ.get("CONTENT_LENGTH", 0) or 0)
body   = sys.stdin.read(clen) if clen > 0 else ""

print("Content-Type: text/html\r")
print("\r")
print(f"<html><body>")
print(f"<p>method: {method}</p>")
print(f"<p>query: {query}</p>")
print(f"<p>body: {body}</p>")
print(f"</body></html>")
