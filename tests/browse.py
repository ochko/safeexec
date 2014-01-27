# trying to retrieve a webpage

import http.client
conn = http.client.HTTPConnection("www.python.org")
conn.request("GET", "/robots.txt")
r1 = conn.getresponse()
print(r1.status, r1.reason)
