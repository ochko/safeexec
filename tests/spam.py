# trying to send email

from smtplib import SMTP
import sys
s = SMTP()
s.connect()
if sys.version_info[0] == 3:
    addr = input()
else:
    addr = raw_input()
s.sendmail(addr, addr, "email sent from safeexec?")
s.quit()
