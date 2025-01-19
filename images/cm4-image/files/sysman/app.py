from flask import Flask
import subprocess
import threading
import time

app = Flask(__name__)

def shutdown_t():
    time.sleep(3)
    subprocess.run(['shutdown', '-h', '0'])

@app.route('/shutdown')
def shutdown():
    thread = threading.Thread(target = shutdown_t)
    thread.start()
    return 'Shutting down'

@app.route('/ping')
def ping():
    return 'Pong'
