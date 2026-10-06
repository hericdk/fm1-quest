#!/usr/bin/env python3
"""Static server for web/ with no caching (so edits show on reload). Usage: python3 serve.py [port]"""
import http.server, os, sys
class H(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()
os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), "web"))
port = int(sys.argv[1]) if len(sys.argv) > 1 else 5193
http.server.ThreadingHTTPServer(("", port), H).serve_forever()
