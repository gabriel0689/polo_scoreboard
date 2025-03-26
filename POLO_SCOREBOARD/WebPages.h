#ifndef WEB_PAGES_H
#define WEB_PAGES_H
//todo, expose baud rate to settings page
//todo: allow clearing the data in the internal storage from the settings page

// index page with table display
const char INDEX_HTML[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Scoreboard Display</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
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
            font-family: 'Courier New', monospace;
            color: #ff0;
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
            font-family: 'Arial Black', sans-serif;
        }
        .home { color: #0cf; }
        .away { color: #f80; }
        .status {
            position: fixed;
            bottom: 10px;
            right: 10px;
            color: #888;
            font-size: 12px;
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
        var ws = new WebSocket('ws://' + location.hostname + '/ws');
        var timeDisplay = document.getElementById('time');
        var homeDisplay = document.getElementById('home');
        var awayDisplay = document.getElementById('away');
        var statusDisplay = document.getElementById('status');
        
        ws.onopen = function() {
            statusDisplay.textContent = "Connected";
            statusDisplay.classList.remove('disconnected');
        };
        
        ws.onclose = function() {
            statusDisplay.textContent = "Disconnected";
            statusDisplay.classList.add('disconnected');
            setTimeout(function() {
                window.location.reload();
            }, 5000);
        };
        
        ws.onmessage = function(event) {
            try {
                var data = JSON.parse(event.data);
                if (data.time) timeDisplay.textContent = data.time;
                if (data.home) homeDisplay.textContent = data.home;
                if (data.away) awayDisplay.textContent = data.away;
            } catch (e) {
                console.error("Error parsing data:", e);
            }
        };
        
        ws.onerror = function() {
            statusDisplay.textContent = "Connection Error";
            statusDisplay.classList.add('disconnected');
        };
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
        <a href="/settings">Settings</a>
    </div>
    <h1>Scoreboard Debug Monitor</h1>
    <div class="controls">
        <button onclick='clearData()'>Clear</button>
        <label><input type="checkbox" id="autoscroll" checked> Auto-scroll</label>
    </div>
    <div id="serialData"></div>
    <script>
        var ws = new WebSocket('ws://' + location.hostname + '/ws');
        var serialDiv = document.getElementById('serialData');
        var autoscroll = document.getElementById('autoscroll');
        
        ws.onopen = function() {
            appendMessage('WebSocket Connected', 'success');
            console.log('WebSocket Connected');
        };
        
        ws.onclose = function() {
            appendMessage('WebSocket Disconnected', 'error');
            console.log('WebSocket Disconnected');
        };
        
        ws.onmessage = function(event) {
            console.log('Message received:', event);
            
            if (event.data instanceof Blob) {
                var reader = new FileReader();
                reader.onload = function() {
                    var data = new Uint8Array(reader.result);
                    console.log('Binary data:', Array.from(data));
                    
                    var minutes = data[0];
                    var seconds = data[1];
                    var homeScore = data[2];
                    var awayScore = data[3];
                    
                    var scoreText = (minutes < 10 ? '0' : '') + minutes + ':' +
                                  (seconds < 10 ? '0' : '') + seconds + ' ' +
                                  'Home ' + homeScore + ' - ' + 
                                  'Away ' + awayScore;
                    
                    appendMessage(scoreText, 'score');
                };
                reader.onerror = function(error) {
                    console.error('FileReader error:', error);
                    appendMessage('Error reading data', 'error');
                };
                reader.readAsArrayBuffer(event.data);
            } else {
                appendMessage(event.data, 'white');
            }
        };
        
        ws.onerror = function(error) {
            console.error('WebSocket Error:', error);
            appendMessage('Connection Error', 'error');
        };
        
        function appendMessage(message, className) {
            var timestamp = new Date().toLocaleTimeString();
            var div = document.createElement('div');
            div.className = className;
            div.textContent = '[' + timestamp + '] ' + message;
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
    </form>
    <div id="status"></div>
    <script>
        var ws = new WebSocket('ws://' + location.hostname + '/ws');
        var statusDiv = document.getElementById('status');
        
        ws.onopen = function() {
            updateStatus('Connected', 'success');
        };
        
        ws.onclose = function() {
            updateStatus('Disconnected', 'error');
        };
        
        ws.onmessage = function(event) {
            updateStatus('Settings updated: ' + event.data, 'info');
        };
        
        function updateStatus(message, className) {
            var div = document.createElement('div');
            div.className = className;
            div.textContent = message;
            statusDiv.innerHTML = '';
            statusDiv.appendChild(div);
        }
        
        document.getElementById('settingsForm').onsubmit = function(e) {
            e.preventDefault();
            var data = {
                baudRate: document.getElementById('baudRate').value
            };
            ws.send(JSON.stringify(data));
        };
    </script>
</body>
</html>
)";

#endif