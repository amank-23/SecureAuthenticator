import smtplib
import sys
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
import datetime

# --- SECURITY UPDATE ---
# We import the password from secrets.py
try:
    from secrets import SENDER_EMAIL, SENDER_PASSWORD
except ImportError:
    SENDER_EMAIL = None
    SENDER_PASSWORD = None
    print("[Error] secrets.py not found! Please create it with your credentials.")
    sys.exit(1)

def send_email(alert_type, recipient, location):
    msg = MIMEMultipart()
    msg['From'] = SENDER_EMAIL
    msg['To'] = recipient

    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # ... (Rest of your logic remains exactly the same) ...
    # Just ensure you use SENDER_EMAIL and SENDER_PASSWORD variables below

    if alert_type == "SUCCESS":
        msg['Subject'] = "Security Alert: New Login Detected"
        body = f"<h2>New Sign-in Detected</h2><p>Location: {location}</p><p>Time: {timestamp}</p>"
    elif alert_type == "CRITICAL":
        msg['Subject'] = "URGENT: Brute Force Attack Blocked"
        body = f"<h2 style='color:red'>Threat Blocked</h2><p>Location: {location}</p><p>Time: {timestamp}</p>"
    else:
        msg['Subject'] = "Warning: Failed Login Attempt"
        body = f"<h2>Failed Login</h2><p>Location: {location}</p><p>Time: {timestamp}</p>"

    msg.attach(MIMEText(body, 'html'))

    try:
        server = smtplib.SMTP('smtp.gmail.com', 587)
        server.starttls()
        server.login(SENDER_EMAIL, SENDER_PASSWORD)
        text = msg.as_string()
        server.sendmail(SENDER_EMAIL, recipient, text)
        server.quit()
        print(f"[Python] Email sent successfully to {recipient}")
    except Exception as e:
        print(f"[Python Error] Failed to send email: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python email_sender.py <TYPE> <EMAIL> <LOCATION>")
    else:
        send_email(sys.argv[1], sys.argv[2], sys.argv[3])