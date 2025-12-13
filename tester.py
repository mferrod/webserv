#!/usr/bin/python3
import subprocess
import os

result = subprocess.run(['/home/petazz/webserv/ubuntu_cgi_tester'], 
                       capture_output=True, 
                       text=True,
                       env=os.environ)
print(result.stdout)
if result.stderr:
    print(result.stderr, file=sys.stderr)
