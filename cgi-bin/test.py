#!/usr/bin/python3
import os
import sys

print('Content-Type: text/html')
print()
print('<html><body>')
print('<h1>CGI Test Successful!</h1>')
print('<p>REQUEST_METHOD: {}</p>'.format(os.environ.get('REQUEST_METHOD', 'Not set')))
print('<p>QUERY_STRING: {}</p>'.format(os.environ.get('QUERY_STRING', 'Not set')))
print('<p>SERVER_PROTOCOL: {}</p>'.format(os.environ.get('SERVER_PROTOCOL', 'Not set')))
print('</body></html>')
