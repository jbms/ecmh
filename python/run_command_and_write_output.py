#!/usr/bin/env python2

import sys, tempfile, subprocess, os

output = sys.argv[1]
command = sys.argv[2:]

with tempfile.NamedTemporaryFile(dir = os.path.dirname(output), prefix = os.path.basename(output), delete = False) as f:
    try:
        subprocess.check_call(command, stdout = f)
        os.rename(f.name, output)
    finally:
        try:
            os.unlink(f.name)
        except:
            pass
