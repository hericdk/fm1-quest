#!/usr/bin/env python3
"""Static server for web/ with no caching (so edits show on reload). Usage: python3 serve.py [port]
Dev only: with FM1_DEV=1 a POST to /__dev/save/<name> writes the body to quest/build/<name> (used by the sprite baker)."""
import http.server, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
class H(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()
    def do_POST(self):
        if os.environ.get("FM1_DEV") != "1" or not self.path.startswith("/__dev/save/"):
            self.send_error(404); return
        name = os.path.basename(self.path[len("/__dev/save/"):])
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        os.makedirs(os.path.join(HERE, "build"), exist_ok=True)
        open(os.path.join(HERE, "build", name), "wb").write(body)
        self.send_response(200); self.end_headers(); self.wfile.write(b"ok")
os.chdir(os.path.join(HERE, "web"))
port = int(sys.argv[1]) if len(sys.argv) > 1 else 5193
http.server.ThreadingHTTPServer(("", port), H).serve_forever()
