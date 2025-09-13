#tHIS  FILE IS UNRELATED tO THe ESP32 PROJECT


#This is a work in progress and is not yet complete.
# it uses a custom debug cable that converts the UART signal from the scoreboard to USB 
# the converter used is a  PL2303 PL2303HX USB to UART TTL Cable Module 4p 4 pin RS232 Converter Serial Line dual male connector to connect to the scoreboard port and the usb port on the computer.
#https://www.aliexpress.us/item/3256806254984495.html
# 3d printed housing for the soldered male port: https://www.printables.com/model/929944-usb-type-a-housing-cover-case

# This script creates a web-based scoreboard display using Flask and SocketIO.
# It connects to a scoreboard via a serial port and displays the time and scores in real-time.
# The web interface is responsive and uses CSS for styling.
# The script also includes a debug page to monitor raw data from the scoreboard.
# The script is designed to be run on a local server and can be accessed via a web browser.
#



#install requirements 
#pip install flask flask-socketio pyserial

#run the script

# Run with automatic port selection from the UI
#python scoreboard_web.py

# Or specify a port directly
#python scoreboard_web.py --port COM3  # Windows

# linux and mac identify usb port using 
# ls /dev/tty.*
#then run the script with the port
#python scoreboard_web.py --port /dev/tty.usbserial-XXXX  # Mac/Linux

#A browser window will automatically open to http://localhost:5000
#If it doesn't, navigate to that URL manually


#!/usr/bin/env python3
# filepath: scoreboard_web.py
import argparse
import json
import os
import re
import serial
import serial.tools.list_ports
import threading
import time
import webbrowser
from flask import Flask, render_template, send_from_directory
from flask_socketio import SocketIO

# Configure Flask app and SocketIO
app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

# Global variables for data storage
last_data = {
    "time": "00:00",
    "home": "0",
    "away": "0"
}

# Serial connection object
ser = None
connected = False

def create_template_directory():
    """Create templates directory and HTML files if they don't exist"""
    if not os.path.exists('templates'):
        os.makedirs('templates')
    
    # Only create index.html if it does not exist
    index_path = os.path.join('templates', 'index.html')
    if not os.path.exists(index_path):
        index_html = '''
<!DOCTYPE html>
<html>
<head>
    <title>Scoreboard Display</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700&display=swap" rel="stylesheet">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 0; 
            background: #1a1a1a; 
            color: #fff;
            display: flex;
            flex-direction: column;
            height: 100vh;
        }
        .footer {
            text-align: center;
            padding: 10px;
            margin-top: 20px;
            font-size: 0.8em;
            color: #777;
            border-top: 1px solid #333;
        }
        .nav { 
            background: #333;
            padding: 10px 20px;
        }
        .nav a { 
            margin-right: 15px; 
            color: #fff; 
            text-decoration: none;
            font-weight: bold;
        }
        .scoreboard {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            flex-grow: 1;
            padding: 20px;
        }
        .time {
            font-size: 8vw;
            font-weight: 600;
            margin-bottom: 20px;
            font-family: 'Orbitron', monospace;
            color: #ffd700;
            letter-spacing: 0.05em;
            text-shadow: none !important;
        }
        .score-table {
            border-collapse: collapse;
            width: 80%;
            max-width: 600px;
            margin: 0 auto;
        }
        .score-table th {
            font-size: 2vw;
            padding: 10px;
            text-align: center;
            background-color: #333;
        }
        .score-table td {
            font-size: 10vw;
            padding: 20px;
            text-align: center;
            font-weight: bold;
            font-family: 'Orbitron', sans-serif;
            letter-spacing: 0.05em;
        }
        .home { 
            color: #0cf;
        }
        .away { 
            color: #f80; 
        }
        .status {
            position: fixed;
            bottom: 10px;
            right: 10px;
            color: #888;
            font-size: 12px;
            cursor: pointer;
        }
        .disconnected { color: #f44; }
    </style>
</head>
<body>
    <div class="nav">
        <a href="/">Scoreboard</a>
        <a href="/debug">Debug</a>
    </div>
    <div class="scoreboard">
        <div class="time" id="time">00:00</div>
        <table class="score-table">
            <tr>
                <th>HOME</th>
                <th>AWAY</th>
            </tr>
            <tr>
                <td class="home" id="home">0</td>
                <td class="away" id="away">0</td>
            </tr>
        </table>
    </div>
    <div id="status" class="status">Not connected</div>
    <div class="footer">Made with love by Face Cage CO. <span id="currentYear"></span></div>
    <script>
        document.getElementById('currentYear').textContent = new Date().getFullYear();
    </script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/socket.io/4.0.1/socket.io.js"></script>
    <script>
        const socket = io();
        const timeDisplay = document.getElementById('time');
        const homeDisplay = document.getElementById('home');
        const awayDisplay = document.getElementById('away');
        const statusDisplay = document.getElementById('status');
        socket.on('scoreboard_data', (data) => {
            if (data.time) timeDisplay.textContent = data.time;
            if (data.home) homeDisplay.textContent = data.home;
            if (data.away) awayDisplay.textContent = data.away;
        });
        socket.on('status_update', (data) => {
            statusDisplay.textContent = data.message;
            if (data.connected) {
                statusDisplay.classList.remove('disconnected');
            } else {
                statusDisplay.classList.add('disconnected');
            }
        });
    </script>
</body>
</html>
        '''
        with open(index_path, 'w') as f:
            f.write(index_html)

    # Only create debug.html if it does not exist
    debug_path = os.path.join('templates', 'debug.html')
    if not os.path.exists(debug_path):
        debug_html = '''
<!DOCTYPE html>
<html>
<head>
    <title>Scoreboard Debug</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #1a1a1a; color: #fff; }
        #serialData {
            width: 100%;
            height: 400px;
            margin: 20px 0;
            padding: 10px;
            border: 1px solid #333;
            overflow-y: scroll;
            font-family: monospace;
            background: #000;
            color: #0f0;
        }
        .nav { margin-bottom: 20px; }
        .nav a { margin-right: 10px; color: #fff; text-decoration: none; font-weight: bold; }
        .controls { margin: 10px 0; }
    .score { color: #00ffff; font-weight: bold; font-size: 1.2em; }
    .error { color: #ff4444; }
    .success { color: #4CAF50; }
    .info { color: #00ffff; }
        button { 
            background: #333; 
            color: #fff; 
            border: none; 
            padding: 8px 16px;
            cursor: pointer;
        }
        button:hover { background: #444; }
    </style>
</head>
<body>
    <div class="nav">
        <a href="/">Scoreboard</a>
        <a href="/debug">Debug</a>
    </div>
    <h1>Scoreboard Debug Monitor</h1>
    <div class="controls">
        <button onclick='clearData()'>Clear</button>
        <label><input type="checkbox" id="autoscroll" checked> Auto-scroll</label>
    </div>
    <div id="serialData"></div>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/socket.io/4.0.1/socket.io.js"></script>
    <script>
        const socket = io();
        const serialDiv = document.getElementById('serialData');
        const autoscroll = document.getElementById('autoscroll');
        socket.on('debug_data', (data) => {
            appendMessage(data.message, data.type || 'info');
        });
        socket.on('raw_data', (data) => {
            appendMessage(`Raw data received: ${data.data}`, 'raw');
        });
        function appendMessage(message, className) {
            var timestamp = new Date().toLocaleTimeString();
            var div = document.createElement('div');
            div.className = className;
            div.textContent = `[${timestamp}] ${message}`;
            serialDiv.appendChild(div);
            if (autoscroll.checked) {
                serialDiv.scrollTop = serialDiv.scrollHeight;
            }
        }
        function clearData() {
            serialDiv.innerHTML = '';
        }
    </script>
</body>
</html>
        '''
        with open(debug_path, 'w') as f:
            f.write(debug_html)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/debug')
def debug():
    return render_template('debug.html')

@socketio.on('connect')
def handle_connect():
    socketio.emit('scoreboard_data', last_data)
    status = {
        'connected': connected,
        'message': f'Connected to port {ser.port}' if connected and ser else 'Not connected to scoreboard'
    }
    socketio.emit('status_update', status)

@socketio.on('scan_ports')
def handle_scan_ports():
    ports = list_serial_ports()
    socketio.emit('port_list', {
        'ports': ports,
        'current_port': ser.port if connected and ser else None
    })

@socketio.on('connect_port')
def handle_connect_port(data):
    global ser, connected
    port = data.get('port')
    
    # Close existing connection if open
    if ser:
        try:
            ser.close()
        except:
            pass
        ser = None
        connected = False
    
    if port:
        try:
            ser = serial.Serial(port, baudrate=9600, timeout=1)
            connected = True
            socketio.emit('status_update', {
                'connected': True,
                'message': f'Connected to port {port}'
            })
            socketio.emit('debug_data', {
                'message': f'Connected to port {port}',
                'type': 'success'
            })
            # Start reading thread after successful connection
            threading.Thread(target=serial_reader, daemon=True).start()
        except Exception as e:
            socketio.emit('status_update', {
                'connected': False,
                'message': f'Failed to connect to port {port}: {str(e)}'
            })
            socketio.emit('debug_data', {
                'message': f'Connection error: {str(e)}',
                'type': 'error'
            })

def list_serial_ports():
    """List available serial ports with descriptions"""
    ports = []
    for port in serial.tools.list_ports.comports():
        ports.append({
            'name': port.device,
            'description': port.description
        })
    return ports

def parse_scoreboard_data(data):
    """Parse the raw data from the scoreboard without hardcoding markers"""
    try:
        # Remove non-printable characters from the start
        clean = ''.join(c for c in data if c.isprintable())
        # Find the first digit, which should be the channel
        for i, c in enumerate(clean):
            if c.isdigit():
                clean = clean[i:]
                break
        # Now expect exactly 13 characters
        if len(clean) >= 13:
            channel = clean[0]
            device_status = clean[1:3]
            minutes = clean[3:5]
            seconds = clean[5:7]
            milliseconds = clean[7:9]
            home_score = clean[9:11]
            away_score = clean[11:13]
            return {
                "time": f"{minutes}:{seconds}",
                "home": str(int(home_score)),
                "away": str(int(away_score))
            }
        else:
            # Log malformed data
            log_message = f"Malformed data: '{data}' (cleaned: '{clean}') at {time.strftime('%Y-%m-%d %H:%M:%S')}\n"
            with open("scoreboard_malformed.log", "a") as log_file:
                log_file.write(log_message)
            socketio.emit('debug_data', {
                'message': f"Malformed data logged: {data}",
                'type': 'warning'
            })
    except Exception as e:
        log_message = f"Error parsing data: '{data}' | Exception: {str(e)} at {time.strftime('%Y-%m-%d %H:%M:%S')}\n"
        with open("scoreboard_malformed.log", "a") as log_file:
            log_file.write(log_message)
        socketio.emit('debug_data', {
            'message': f"Error parsing data and logged: {str(e)}",
            'type': 'error'
        })
    return None

def serial_reader():
    """Background thread for reading from serial port"""
    global last_data
    if 'DEBUG_PRINT' in globals() and DEBUG_PRINT:
        print("[DEBUG] Serial reader thread started")
    socketio.emit('debug_data', {
        'message': "Serial reader thread started",
        'type': 'info'
    })
    buffer = ""
    while connected and ser:
        try:
            # Read from serial port with timeout
            if ser.in_waiting > 0:
                new_data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                if 'DEBUG_PRINT' in globals() and DEBUG_PRINT:
                    print(f"[DEBUG] Read from serial: {repr(new_data)}")
                buffer += new_data
                # Process complete lines (split on both \n and \t)
                import re
                # Split on either newline or tab
                if re.search(r'[\n\t]', buffer):
                    # Use regex split to handle both delimiters
                    lines = re.split(r'[\n\t]', buffer)
                    buffer = lines[-1]  # Keep the incomplete line
                    for line in lines[:-1]:  # Process complete lines
                        line = line.strip()
                        if line:
                            if 'DEBUG_PRINT' in globals() and DEBUG_PRINT:
                                print(f"[DEBUG] Emitting raw_data: {line}")
                            # Emit raw data
                            socketio.emit('raw_data', {'data': line})
                            # Try to parse
                            parsed = parse_scoreboard_data(line)
                            if parsed:
                                last_data = parsed
                                socketio.emit('scoreboard_data', parsed)
                                socketio.emit('debug_data', {
                                    'message': f"Time: {parsed['time']}, Home: {parsed['home']}, Away: {parsed['away']}",
                                    'type': 'score'
                                })
            else:
                # No data available, sleep briefly to avoid high CPU usage
                time.sleep(0.1)
        except Exception as e:
            if 'DEBUG_PRINT' in globals() and DEBUG_PRINT:
                print(f"[ERROR] Serial read error: {str(e)}")
            socketio.emit('debug_data', {
                'message': f"Serial read error: {str(e)}",
                'type': 'error'
            })
            time.sleep(1)  # Wait before retrying

def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(description='Scoreboard Web Interface')
    parser.add_argument('--port', type=str, help='Serial port to connect to')
    parser.add_argument('--host', type=str, default='0.0.0.0', help='Host to run the web server on')
    parser.add_argument('--web-port', type=int, default=5050, help='Port to run the web server on')
    parser.add_argument('--debug', action='store_true', help='Enable debug print output')
    args = parser.parse_args()
    global DEBUG_PRINT
    DEBUG_PRINT = args.debug
    
    create_template_directory()
    
    # Connect to serial port if specified
    if args.port:
        global ser, connected
        try:
            ser = serial.Serial(args.port, baudrate=9600, timeout=1)
            connected = True
            if args.debug:
                print(f"Connected to port {args.port}")
            # Start reading thread
            threading.Thread(target=serial_reader, daemon=True).start()
        except Exception as e:
            if args.debug:
                print(f"Failed to connect to port {args.port}: {str(e)}")
    
    # Open web browser after a delay
    def open_browser():
        time.sleep(1)
        webbrowser.open(f"http://localhost:{args.web_port}")
    
    threading.Thread(target=open_browser, daemon=True).start()
    
    # Start the Flask-SocketIO server
    if args.debug:
        print(f"Starting web server at http://{args.host}:{args.web_port}")
    socketio.run(app, host=args.host, port=args.web_port, debug=False, allow_unsafe_werkzeug=True)

if __name__ == '__main__':
    main()