#pragma once
#include <Arduino.h>

const char pagina_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 - Balança Revlo</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            background-color: #0a0e27;
            color: #ffffff;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
        }

        .header {
            display: flex;
            justify-content: space-between;
            align-items: flex-start;
            padding: 18px 24px;
            border-bottom: 1px solid #1f2937;
        }

        .logo {
            font-size: 24px;
            font-weight: bold;
            color: #ff3333;
            letter-spacing: 1px;
        }

        .logo-sub {
            font-size: 10px;
            color: #888;
            text-transform: uppercase;
        }

        .top-info {
            display: flex;
            flex-direction: column;
            gap: 4px;
            text-align: right;
            color: #d1d5db;
            font-size: 12px;
            font-family: 'Courier New', Courier, monospace;
        }

        .top-info span {
            color: #3b82f6;
            font-weight: bold;
        }

        .header-center-info {
            display: flex;
            gap: 18px;
            justify-content: center;
            align-items: center;
            flex: 1;
            color: #d1d5db;
            font-size: 13px;
            font-family: 'Courier New', Courier, monospace;
            text-align: center;
        }

        .header-center-info span {
            color: #10b981;
            font-size: 18px;
            font-weight: bold;
        }

        .container {
            padding: 20px;
            max-width: 650px;
            margin: 0 auto;
            width: 100%;
        }

        /* Painel Lado a Lado para Exibição dos Pesos */
        .weight-grid {
            display: flex;
            gap: 16px;
            margin-bottom: 20px;
        }

        .weight-box {
            flex: 1;
            background: #111827;
            padding: 16px;
            border-radius: 8px;
            border: 1px solid #1f2937;
            text-align: center;
        }

        .weight-label {
            font-size: 11px;
            color: #9ca3af;
            text-transform: uppercase;
            margin-bottom: 6px;
            display: block;
            letter-spacing: 0.5px;
        }

        .weight-value {
            font-size: 28px;
            font-weight: bold;
            font-family: 'Courier New', Courier, monospace;
        }

        /* Containers de Alinhamento Horizontal para os Botões Superiores */
        .btn-row {
            display: flex;
            gap: 14px;
            margin-bottom: 14px;
        }

        .btn {
            flex: 1;
            color: white;
            border: none;
            padding: 16px;
            border-radius: 6px;
            font-weight: bold;
            cursor: pointer;
            font-size: 16px;
            text-transform: uppercase;
            transition: background 0.2s, opacity 0.2s;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }

        .btn:hover {
            opacity: 0.9;
        }

        .btn-somar {
            background-color: #e11d48; /* Ajustado para um vermelho similar ao da imagem */
        }

        .btn-zerar {
            background-color: #4b5563;
        }

        .btn-calibrar {
            background-color: #3b82f6;
        }

        /* NOVO: Container Inferior Horizontal (Estilo da Imagem) */
        .action-bar {
            display: flex;
            align-items: center;
            background: #111827;
            border: 1px solid #1f2937;
            padding: 16px;
            border-radius: 8px;
            margin-top: 15px;
            gap: 16px;
            justify-content: space-between;
        }

        .btn-acao-lateral {
            background-color: #e11d48;
            color: white;
            border: none;
            padding: 16px 24px;
            border-radius: 6px;
            font-weight: bold;
            cursor: pointer;
            font-size: 15px;
            text-transform: uppercase;
            transition: background 0.2s;
            min-width: 120px;
        }

        .btn-acao-lateral:hover {
            background-color: #be123c;
        }

        .info-group {
            flex: 0 0 auto;
            text-align: center;
        }

        .info-group .reg-label {
            color: #9ca3af;
            font-size: 11px;
            display: block;
            text-transform: uppercase;
            margin-bottom: 2px;
        }

        .info-group .reg-value {
            font-size: 26px;
            font-weight: bold;
            color: #10b981;
            font-family: monospace;
        }

        .input-group {
            flex: 1;
        }

        .input-group input {
            width: 100%;
            padding: 14px;
            background: #1f2937;
            border: 1px solid #374151;
            border-radius: 6px;
            color: #ffffff;
            font-size: 16px;
            font-weight: bold;
            text-transform: uppercase;
            outline: none;
            text-align: center;
            letter-spacing: 2px;
        }

        .input-group input:focus {
            border-color: #e11d48;
        }

        .status-bar {
            text-align: center;
            color: #888;
            margin-top: 20px;
            min-height: 20px;
            font-size: 14px;
            font-style: italic;
        }
    </style>
</head>
<body>
    <div class="header">
        <div>
            <div class="logo">REVLO</div>
            <div class="logo-sub">Sistema de Balança Rodoviária</div>
        </div>
        <div class="header-center-info">
            <div>Peso 1: <span id="tpeso1">--</span></div>
            <div>Peso 2: <span id="tpeso2">--</span></div>
        </div>
        <div class="top-info">
            <div>Bateria: <span id="tbateria">--</span></div>
            <div>Hora: <span id="last_thora">--</span></div>
            <div>Data: <span id="last_tdata">--</span></div>
        </div>
    </div>

    <div class="container">
        <div class="weight-grid">
            <div class="weight-box">
                <span class="weight-label">Peso Atual</span>
                <div class="weight-value" id="peso" style="color: #ff3333;">--</div>
            </div>
            <div class="weight-box">
                <span class="weight-label">Peso Acumulado</span>
                <div class="weight-value" id="pesoAcumulado" style="color: #3b82f6;">--</div>
            </div>
        </div>

        <div class="btn-row">
            <button onclick="zerar()" class="btn btn-zerar">Zerar</button>
            <button onclick="somar()" class="btn btn-somar">Somar</button>
        </div>

        <div class="action-bar">
            <button onclick="salvar()" class="btn-acao-lateral">Salvar</button>
            
            <div class="info-group">
                <span class="reg-label">Registro</span>
                <span id="contadorRegistro" class="reg-value">0</span>
            </div>
            
            <div class="input-group">
                <input type="text" id="placaInput" placeholder="PLACA DO VEÍCULO" maxlength="7" autocomplete="off">
            </div>
            
            <button onclick="limpar()" class="btn-acao-lateral">Limpar</button>
        </div>

        <div id="status" class="status-bar"></div>
    </div>

    <script>
        setInterval(buscarDados, 500);

        function setStatus(msg) {
            document.getElementById('status').innerText = msg;
        }

        function buscarDados() {
            fetch('/dados')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('peso').innerText = Number.isFinite(Number(d.pesoAtual)) ? Math.max(0, Number(d.pesoAtual)).toFixed(2) + " kg" : '--';
                    document.getElementById('pesoAcumulado').innerText = Number.isFinite(Number(d.peixo)) ? Number(d.peixo).toFixed(2) + " kg" : '--';
                    document.getElementById('contadorRegistro').innerText = d.contadorRegistro !== undefined ? d.contadorRegistro : '0';
                    document.getElementById('tbateria').innerText = d.tbateria || '--';
                    document.getElementById('last_thora').innerText = d.last_thora || '--';
                    document.getElementById('last_tdata').innerText = d.last_tdata || '--';
                    document.getElementById('tpeso1').innerText = d.tpeso1 ? d.tpeso1 + " kg" : '--';
                    document.getElementById('tpeso2').innerText = d.tpeso2 ? d.tpeso2 + " kg" : '--';
                    
                    if (d.tplaca !== undefined && document.activeElement !== document.getElementById('placaInput')) {
                        document.getElementById('placaInput').value = d.tplaca;
                    }
                })
                .catch(() => setStatus('Erro de comunicação com o ESP32...'));
        }

        function somar() {
            setStatus('Executando comando [Somar]...');
            fetch('/somar')
                .then(r => r.text())
                .then(msg => { setStatus(msg); setTimeout(() => setStatus(''), 2000); });
        }

        function salvar() {
            const placa = document.getElementById('placaInput').value.toUpperCase();
            setStatus('Salvando registro...');
            fetch(`/salvar?placa=${encodeURIComponent(placa)}`)
                .then(r => r.text())
                .then(msg => { setStatus(msg); setTimeout(() => setStatus(''), 2000); });
        }

        function zerar() {
            setStatus('Executando Zero...');
            fetch('/zero')
                .then(r => r.text())
                .then(msg => { setStatus(msg); setTimeout(() => setStatus(''), 2000); });
        }

        function calibrar() {
            setStatus('Acessando modo de calibração...');
            fetch('/calibrar')
                .then(r => r.text())
                .then(msg => setStatus(msg));
        }

        function limpar() {
            setStatus('Limpando dados ativos...');
            fetch('/limpar')
                .then(r => r.text())
                .then(msg => { 
                    setStatus(msg); 
                    document.getElementById('placaInput').value = '';
                    setTimeout(() => setStatus(''), 2000); 
                });
        }
    </script>
</body>
</html>
)rawliteral";
