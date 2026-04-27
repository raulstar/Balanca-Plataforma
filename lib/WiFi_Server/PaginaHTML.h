#pragma once

const char MAIN_page[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>ESP32 WebServer</title>
    <style>
        body {
            font-family: Arial;
            text-align: center;
            background-color: #121212;
            color: #ffffff;
        }
        h1 {
            color: #00ffcc;
        }
        button {
            padding: 15px;
            margin: 10px;
            font-size: 16px;
            border: none;
            border-radius: 8px;
            background-color: #00ffcc;
            cursor: pointer;
        }
        .card {
            background: #1e1e1e;
            padding: 20px;
            margin: 20px;
            border-radius: 10px;
        }
    </style>
</head>

<body>

    <h1>ESP32 WebServer</h1>

    <div class="card">
        <h2>Status</h2>
        <p id="wifi">WiFi: --</p>
        <p id="ip">IP: --</p>
    </div>

    <div class="card">
        <h2>Controle</h2>
        <button onclick="sendCmd('led_on')">LED ON</button>
        <button onclick="sendCmd('led_off')">LED OFF</button>
    </div>

    <div class="card">
        <h2>Sensor HX711</h2>
        <p id="peso">Peso: --</p>
        <button onclick="getPeso()">Atualizar</button>
    </div>

<script>

function updateStatus() {
    fetch("/status")
    .then(res => res.json())
    .then(data => {
        document.getElementById("wifi").innerText = "WiFi: " + data.wifi;
        document.getElementById("ip").innerText = "IP: " + data.ip;
    });
}

function sendCmd(cmd) {
    fetch("/cmd?c=" + cmd);
}

function getPeso() {
    fetch("/peso")
    .then(res => res.text())
    .then(data => {
        document.getElementById("peso").innerText = "Peso: " + data + " g";
    });
}

// Atualiza automaticamente
setInterval(updateStatus, 2000);

</script>

</body>
</html>

)=====";