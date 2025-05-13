#ifndef WEB_PAGES_H
#define WEB_PAGES_H

// index page with table display
const char INDEX_HTML[] PROGMEM = R"(
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
        .nav { 
            background: #333;
            padding: 10px 20px;
        }
        .nav a { 
            margin-right: 15px; 
            color: #4CAF50; 
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
            font-weight: bold;
            margin-bottom: 20px;
            font-family: 'Orbitron', monospace;
            color: #ff0;
            letter-spacing: 0.05em;
            text-shadow: 0 0 10px rgba(255, 255, 0, 0.5);
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
            text-shadow: 0 0 10px rgba(0, 204, 255, 0.5);
        }
        .away { 
            color: #f80; 
            text-shadow: 0 0 10px rgba(255, 136, 0, 0.5);
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
        <a href="/settings">Settings</a>
    </div>
    
    <div class="scoreboard">
        <div class="time" id="time">00:00</div>
        <table class="score-table">
            <tr>
                <th>HOME</th>
                <th>AWAY</th>
            </tr>
            <tr>
                <td class="home" id="home">00</td>
                <td class="away" id="away">00</td>
            </tr>
        </table>
    </div>
    
    <div id="status" class="status">Connecting...</div>

    <script>
        var wsUrl = `ws://` + location.hostname + `/ws`;
        var timeDisplay = document.getElementById(`time`);
        var homeDisplay = document.getElementById(`home`);
        var awayDisplay = document.getElementById(`away`);
        var statusDisplay = document.getElementById(`status`);
        var ws;
        var reconnectAttempts = 0;
        var maxReconnectAttempts = 20; // Try reconnecting ~10 minutes (20 * varies from 5s to 30s)
        var isConnecting = false;
        
        function connectWebSocket() {
            if (isConnecting) return; // Prevent multiple connection attempts
            
            isConnecting = true;
            statusDisplay.textContent = `Connecting...`;
            
            // Create new WebSocket connection
            ws = new WebSocket(wsUrl);
            
            ws.onopen = function() {
                isConnecting = false;
                reconnectAttempts = 0;
                statusDisplay.textContent = `Connected`;
                statusDisplay.classList.remove(`disconnected`);
            };
            
            ws.onclose = function() {
                isConnecting = false;
                statusDisplay.textContent = `Disconnected - Retrying in ` + getReconnectDelay()/1000 + `s`;
                statusDisplay.classList.add(`disconnected`);
                
                // Schedule reconnection attempt
                setTimeout(reconnect, getReconnectDelay());
            };
            
            ws.onmessage = function(event) {
                try {
                    var data = JSON.parse(event.data);
                    if (data.time) timeDisplay.textContent = data.time;
                    if (data.home) homeDisplay.textContent = data.home;
                    if (data.away) awayDisplay.textContent = data.away;
                } catch (e) {
                    console.error(`Error parsing data:`, e);
                }
            };
            
            ws.onerror = function() {
                isConnecting = false;
                statusDisplay.textContent = `Connection Error`;
                statusDisplay.classList.add(`disconnected`);
                // Error is followed by onclose, which will handle reconnection
            };
        }
        
        function reconnect() {
            reconnectAttempts++;
            
            // Update status with attempt information
            var delay = getReconnectDelay()/1000;
            if (reconnectAttempts <= maxReconnectAttempts || maxReconnectAttempts <= 0) {
                statusDisplay.textContent = `Reconnecting (Attempt ` + reconnectAttempts + `)`;
                connectWebSocket();
            } else {
                statusDisplay.textContent = `Reconnection failed after ` + reconnectAttempts + ` attempts. Click to try again.`;
            }
        }
        
        function getReconnectDelay() {
            // First few attempts faster, then slow down to 30 seconds
            if (reconnectAttempts < 3) {
                return 5000; // 5 seconds
            } else if (reconnectAttempts < 5) {
                return 10000; // 10 seconds
            } else {
                return 30000; // 30 seconds
            }
        }
        
        // Initialize connection
        connectWebSocket();
        
        // Add a manual reconnect option
        statusDisplay.addEventListener(`click`, function() {
            if (!ws || ws.readyState === WebSocket.CLOSED || ws.readyState === WebSocket.CLOSING) {
                statusDisplay.textContent = `Manual reconnect...`;
                reconnectAttempts = 0; // Reset counter for manual reconnect
                connectWebSocket();
            }
        });
    </script>
</body>
</html>
)";

//debug page
const char DEBUG_HTML[] PROGMEM = R"(
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
        .nav a { margin-right: 10px; color: #4CAF50; text-decoration: none; }
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
        #reconnect {
            margin-left: 10px;
            background: #4CAF50;
        }
        #reconnect:hover {
            background: #3d8b3d;
        }
    </style>
</head>
<body>
    <div class="nav">
        <a href="/">Scoreboard</a>
        <a href="/debug">Debug</a>
        <a href="/settings">Settings</a>
    </div>
    <h1>Scoreboard Debug Monitor</h1>
    <div class="controls">
        <button onclick='clearData()'>Clear</button>
        <label><input type="checkbox" id="autoscroll" checked> Auto-scroll</label>
        <button id="reconnect" onclick='manualReconnect()'>Reconnect</button>
    </div>
    <div id="serialData"></div>
    <script>
        var wsUrl = `ws://` + location.hostname + `/ws`;
        var serialDiv = document.getElementById(`serialData`);
        var autoscroll = document.getElementById(`autoscroll`);
        var ws;
        var reconnectAttempts = 0;
        var isConnecting = false;
        var reconnectTimer = null;
        
        function connectWebSocket() {
            if (isConnecting) return;
            
            isConnecting = true;
            appendMessage(`Connecting to WebSocket...`, `info`);
            
            ws = new WebSocket(wsUrl);
            
            ws.onopen = function() {
                isConnecting = false;
                reconnectAttempts = 0;
                appendMessage(`WebSocket Connected`, `success`);
                if (reconnectTimer) {
                    clearTimeout(reconnectTimer);
                    reconnectTimer = null;
                }
            };
            
            ws.onclose = function() {
                isConnecting = false;
                var delay = getReconnectDelay();
                appendMessage(`WebSocket Disconnected - Retrying in ` + delay/1000 + `s`, `error`);
                
                // Schedule reconnection
                reconnectTimer = setTimeout(reconnect, delay);
            };
            
            ws.onmessage = function(event) {
                console.log(`Message received:`, event);
                
                if (event.data instanceof Blob) {
                    var reader = new FileReader();
                    reader.onload = function() {
                        var data = new Uint8Array(reader.result);
                        console.log(`Binary data:`, Array.from(data));
                        
                        var minutes = data[0];
                        var seconds = data[1];
                        var homeScore = data[2];
                        var awayScore = data[3];
                        
                        var scoreText = (minutes < 10 ? `0` : ``) + minutes + `:` +
                                      (seconds < 10 ? `0` : ``) + seconds + ` ` +
                                      `Home ` + homeScore + ` - ` + 
                                      `Away ` + awayScore;
                        
                        appendMessage(scoreText, `score`);
                    };
                    reader.onerror = function(error) {
                        console.error(`FileReader error:`, error);
                        appendMessage(`Error reading data`, `error`);
                    };
                    reader.readAsArrayBuffer(event.data);
                } else {
                    appendMessage(event.data, `white`);
                }
            };
            
            ws.onerror = function(error) {
                isConnecting = false;
                appendMessage(`Connection Error`, `error`);
            };
        }
        
        function reconnect() {
            reconnectAttempts++;
            appendMessage(`Reconnecting (Attempt ` + reconnectAttempts + `)`, `info`);
            connectWebSocket();
        }
        
        function manualReconnect() {
            if (reconnectTimer) {
                clearTimeout(reconnectTimer);
                reconnectTimer = null;
            }
            
            if (ws) {
                try {
                    ws.close();
                } catch (e) {
                    console.error(`Error closing socket:`, e);
                }
            }
            
            reconnectAttempts = 0;
            appendMessage(`Manual reconnection...`, `info`);
            connectWebSocket();
        }
        
        function getReconnectDelay() {
            if (reconnectAttempts < 3) return 5000;      // 5 seconds
            else if (reconnectAttempts < 5) return 10000; // 10 seconds
            else return 30000;                            // 30 seconds
        }
        
        function appendMessage(message, className) {
            var timestamp = new Date().toLocaleTimeString();
            var div = document.createElement(`div`);
            div.className = className;
            div.textContent = `[` + timestamp + `] ` + message;
            serialDiv.appendChild(div);
            
            if (autoscroll.checked) {
                serialDiv.scrollTop = serialDiv.scrollHeight;
            }
        }
        
        function clearData() {
            serialDiv.innerHTML = ``;
        }
        
        // Initialize connection
        connectWebSocket();
    </script>
</body>
</html>
)";

// settings page
const char SETTINGS_HTML[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Settings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #1a1a1a; color: #fff; }
        .nav { margin-bottom: 20px; }
        .nav a { margin-right: 10px; color: #4CAF50; text-decoration: none; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; }
        .success { color: #4CAF50; }
        .error { color: #ff4444; }
        .info { color: #00ffff; }
        button {
            background: #333;
            color: #fff;
            border: none;
            padding: 8px 16px;
            cursor: pointer;
        }
        button:hover { background: #444; }
        select {
            background: #333;
            color: #fff;
            padding: 5px;
            border: 1px solid #555;
        }
        .danger-zone {
            margin-top: 40px;
            border: 1px solid #f44;
            padding: 15px;
            border-radius: 5px;
        }
        .danger-zone h3 {
            color: #f44;
            margin-top: 0;
        }
        .danger-button {
            background: #f44;
            color: #fff;
            border: none;
            padding: 8px 16px;
            cursor: pointer;
            margin-top: 10px;
            position: relative;
            overflow: hidden;
        }
        .danger-button:hover {
            background: #e33;
        }
        .button-progress {
            position: absolute;
            left: 0;
            bottom: 0;
            height: 4px;
            background: #fff;
            width: 0%;
            transition: width 0.1s linear;
        }
        .button-text {
            position: relative;
            z-index: 2;
        }
        #reconnect {
            background: #4CAF50;
            margin-left: 10px;
        }
        #reconnect:hover {
            background: #3d8b3d;
        }
    </style>
</head>
<body>
    <div class="nav">
        <a href="/">Scoreboard</a>
        <a href="/debug">Debug</a>
        <a href="/settings">Settings</a>
    </div>
    <h1>Settings</h1>
    <form id="settingsForm">
        <div class="form-group">
            <label for="baudRate">Baud Rate:</label>
            <select id="baudRate" name="baudRate">
                <option value="9600">9600</option>
                <option value="115200">115200</option>
                <option value="230400">230400</option>
            </select>
        </div>
        <button type="submit">Save Settings</button>
        <button type="button" id="reconnect" onclick='manualReconnect()'>Reconnect</button>
    </form>
    
    <div class='danger-zone'>
        <h3>Danger Zone</h3>
        <p>This will erase all WiFi configuration and restart the device. You will need to reconnect through the setup portal.</p>
        <p><strong>Press and hold for 10 seconds to reset WiFi</strong></p>
        <button type="button" id="resetWifiBtn" class="danger-button">
            <span class="button-text">Reset WiFi Configuration</span>
            <span class="button-progress" id="resetProgress"></span>
        </button>
    </div>
    
    <div id="status"></div>
        <script>
        var wsUrl = `ws://` + location.hostname + `/ws`;
        var statusDiv = document.getElementById(`status`);
        var ws;
        var reconnectAttempts = 0;
        var isConnecting = false;
        var reconnectTimer = null;
        
        function connectWebSocket() {
            if (isConnecting) return;
            
            isConnecting = true;
            updateStatus(`Connecting...`, `info`);
            
            ws = new WebSocket(wsUrl);
            
            ws.onopen = function() {
                isConnecting = false;
                reconnectAttempts = 0;
                updateStatus(`Connected`, `success`);
                if (reconnectTimer) {
                    clearTimeout(reconnectTimer);
                    reconnectTimer = null;
                }
            };
            
            ws.onclose = function() {
                isConnecting = false;
                var delay = getReconnectDelay();
                updateStatus(`Disconnected - Retrying in ` + delay/1000 + `s`, `error`);
                
                // Schedule reconnection
                reconnectTimer = setTimeout(reconnect, delay);
            };
            
            ws.onmessage = function(event) {
                updateStatus(`Settings updated: ` + event.data, `info`);
            };
            
            ws.onerror = function() {
                isConnecting = false;
                updateStatus(`Connection Error`, `error`);
            };
        }
        
        function reconnect() {
            reconnectAttempts++;
            updateStatus(`Reconnecting (Attempt ` + reconnectAttempts + `)`, `info`);
            connectWebSocket();
        }
        
        function manualReconnect() {
            if (reconnectTimer) {
                clearTimeout(reconnectTimer);
                reconnectTimer = null;
            }
            
            if (ws) {
                try {
                    ws.close();
                } catch (e) {
                    console.error(`Error closing socket:`, e);
                }
            }
            
            reconnectAttempts = 0;
            updateStatus(`Manual reconnection...`, `info`);
            connectWebSocket();
            
            return false; // Prevent form submission
        }
        
        function getReconnectDelay() {
            if (reconnectAttempts < 3) return 5000;      // 5 seconds
            else if (reconnectAttempts < 5) return 10000; // 10 seconds
            else return 30000;                            // 30 seconds
        }
        
        function updateStatus(message, className) {
            var div = document.createElement(`div`);
            div.className = className;
            div.textContent = message;
            statusDiv.innerHTML = ``;
            statusDiv.appendChild(div);
        }
        
        document.getElementById(`settingsForm`).onsubmit = function(e) {
            e.preventDefault();
            var data = {
                baudRate: document.getElementById(`baudRate`).value
            };
            ws.send(JSON.stringify(data));
        };
        
        // Long press implementation for reset button
        var resetBtn = document.getElementById(`resetWifiBtn`);
        var resetProgress = document.getElementById(`resetProgress`);
        var pressTimer = null;
        var pressStartTime = 0;
        var requiredPressTime = 10000; // 10 seconds in milliseconds
        
        resetBtn.addEventListener(`mousedown`, startPress);
        resetBtn.addEventListener(`touchstart`, startPress);
        resetBtn.addEventListener(`mouseup`, endPress);
        resetBtn.addEventListener(`mouseleave`, endPress);
        resetBtn.addEventListener(`touchend`, endPress);
        resetBtn.addEventListener(`touchcancel`, endPress);
        
        function startPress(e) {
            if (e.type === `touchstart`) {
                e.preventDefault(); // Prevent mousedown from firing too
            }
            
            resetProgress.style.width = `0%`;
            pressStartTime = Date.now();
            
            // Update progress bar every 100ms
            pressTimer = setInterval(function() {
                var elapsedTime = Date.now() - pressStartTime;
                var progressPercent = Math.min(100, (elapsedTime / requiredPressTime) * 100);
                resetProgress.style.width = progressPercent + `%`;
                
                // If weve reached the required time
                if (elapsedTime >= requiredPressTime) {
                    clearInterval(pressTimer);
                    resetProgress.style.width = `100%`;
                    
                    // Ask for final confirmation
                    if (confirm(`WARNING: This will erase all WiFi settings and restart the device. You will need to reconnect through the WiFi setup portal. Continue?`)) {
                        updateStatus(`Resetting WiFi configuration and restarting...`, `info`);
                        var data = {
                            command: `resetWifi`
                        };
                        ws.send(JSON.stringify(data));
                        setTimeout(function() {
                            window.location.href = `/`;
                        }, 3000);
                    } else {
                        // Reset progress on cancel
                        resetProgress.style.width = `0%`;
                    }
                }
            }, 100);
        }
        
        function endPress() {
            clearInterval(pressTimer);
            resetProgress.style.width = `0%`;
        }
        
        // Initialize connection
        connectWebSocket();
    </script>
</body>
</html>
)";

#endif