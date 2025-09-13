# POLO_SCOREBOARD_PY

This folder contains the Python web interface for the scoreboard project. It connects to the scoreboard via a serial port and displays the time and scores in real-time on a web interface.

## Requirements
- Python 3.7+
- pip (Python package manager)
- USB-to-Serial adapter (e.g., PL2303HX)

## Installation
1. **Clone or copy this folder to your computer.**
2. **Open a terminal and navigate to this folder:**
   ```bash
   cd /path/to/POLO_SCOREBOARD_PY
   ```
3. **(Optional but recommended) Create a virtual environment:**
   ```bash
   python3 -m venv venv
   source venv/bin/activate
   ```
4. **Install required Python packages:**
   ```bash
   pip install flask flask-socketio pyserial
   ```

## Usage
1. **Connect your scoreboard to your computer using the USB-to-Serial cable.**
2. **Find your serial port name:**
   - On Mac/Linux: `ls /dev/tty.*`
   - On Windows: Check Device Manager for COM ports
3. **Run the web server:**
   - To select the port from the web UI:
     ```bash
     python scoreboard_web.py
     ```
   - Or to specify the port directly:
     ```bash
     python scoreboard_web.py --port /dev/tty.usbserial-XXXX  # Mac/Linux
     python scoreboard_web.py --port COM3                     # Windows
     ```
4. **Open your browser to** [http://localhost:5050](http://localhost:5050) (should open automatically).

## Features
- Real-time scoreboard display (time, home, away)
- Debug page for raw serial data
- Set a folder to save a `.txt` file with the latest scores (enter folder path in the UI)
- No drop shadows for a flat UI look

## Notes
- The `templates/` folder is auto-generated if missing.
- The `.txt` file is updated in real time when new serial data is received (if save folder is set).
- If you have issues with permissions or missing packages, ensure your Python environment is activated and dependencies are installed.

## Troubleshooting
- **ModuleNotFoundError:** Run `pip install flask flask-socketio pyserial`
- **Serial port not found:** Double-check your cable and port name.
- **Web page not loading:** Make sure the server is running and you are on the correct port (default is 5050).

## License
MIT License
