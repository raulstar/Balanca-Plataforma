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
            /* Mesma fonte e tamanho do rotulo "Peso Atual" (.weight-label) */
            font-size: 11px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            letter-spacing: 0.5px;
        }

        .top-info span {
            color: #3b82f6;
            font-weight: 900;
            font-family: Arial, Helvetica, sans-serif;
            color: #39ff14;
        }

        .header-center-info {
            display: flex;
            gap: 18px;
            justify-content: center;
            align-items: center;
            flex: 1;
            color: #d1d5db;
            /* Mesma fonte e tamanho do rotulo "Peso Atual" (.weight-label) */
            font-size: 11px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            letter-spacing: 0.5px;
            text-align: center;
        }

        .header-center-info span {
            color: #10b981;
            font-size: 18px;
            font-weight: 900;
            font-family: Arial, Helvetica, sans-serif;
            color: #39ff14;
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
            font-size: clamp(38px, 7vw, 64px);
            font-weight: 900;
            font-family: Arial, Helvetica, sans-serif;
            color: #39ff14 !important;
            line-height: 1;
            letter-spacing: -2px;
            white-space: nowrap;
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
            background-color: #e11d48;
        }

        .btn-zerar {
            background-color: #e11d48;
        }

        .btn-calibrar {
            background-color: #101829;
        }

        /* Container Inferior Horizontal */
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
            font-weight: 900;
            color: #39ff14;
            font-family: Arial, Helvetica, sans-serif;
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

        .page-footer {
            text-align: center;
            color: #6b7280;
            font-size: 12px;
            line-height: 1.6;
            margin-top: 24px;
            padding-top: 14px;
            border-top: 1px solid #1f2937;
        }

        .page-footer strong {
            color: #9ca3af;
            font-weight: 600;
        }

        .status-bar {
            text-align: center;
            color: #888;
            margin-top: 20px;
            min-height: 20px;
            font-size: 14px;
            font-style: italic;
        }

        .bottom-btn-row {
            display: flex;
            gap: 14px;
            margin-top: 14px;
        }

        .btn-bottom-blue {
            background-color: #293462;
        }

        .history-panel {
            background: #090d18;
            border: 1px solid #1f2937;
            border-radius: 8px;
            margin-top: 20px;
            padding: 18px 20px 20px;
            overflow: hidden;
        }

        .history-table-wrap {
            width: 100%;
            overflow-x: auto;
            padding-bottom: 2px;
        }

        .history-table {
            width: 100%;
            min-width: 610px;
            border-collapse: separate;
            border-spacing: 0 5px;
            font-family: 'Courier New', Courier, monospace;
        }

        .history-table th {
            color: #687286;
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 1px;
            text-align: left;
            padding: 0 12px 6px;
            font-weight: 900;
        }

        .history-table td {
            background: #151b2c;
            color: #e5e7eb;
            padding: 10px 12px;
            border-top: 1px solid #253047;
            border-bottom: 1px solid #253047;
            font-size: 12px;
            font-weight: 800;
            white-space: nowrap;
        }

        .history-table td:first-child {
            border-left: 1px solid #253047;
            border-radius: 4px 0 0 4px;
            color: #ff1f5b;
        }

        .history-table td:last-child {
            border-right: 1px solid #253047;
            border-radius: 0 4px 4px 0;
            color: #727b8d;
        }

        .history-table .history-date {
            color: #727b8d;
        }

        .history-table .history-weight {
            color: #00ff66;
            font-weight: 900;
        }

        @media (max-width: 768px) {
            .header {
                align-items: center;
                flex-direction: column;
                gap: 14px;
                padding: 16px;
                text-align: center;
            }

            .header-center-info {
                flex: none;
                flex-wrap: wrap;
                gap: 10px 16px;
                width: 100%;
            }

            .top-info {
                align-items: center;
                text-align: center;
                width: 100%;
            }

            .container {
                max-width: 100%;
                padding: 14px;
            }

            .weight-grid {
                flex-direction: column;
                gap: 12px;
                margin-bottom: 14px;
            }

            .weight-box {
                padding: 14px 10px;
            }

            .weight-value {
                font-size: clamp(32px, 12vw, 48px);
                letter-spacing: -1px;
            }

            .btn-row,
            .bottom-btn-row {
                gap: 10px;
            }

            .btn {
                min-height: 52px;
                padding: 14px 10px;
                font-size: 14px;
            }

            .action-bar {
                display: grid;
                grid-template-columns: 1fr 1fr;
                gap: 12px;
                padding: 12px;
            }

            .info-group,
            .input-group {
                grid-column: 1 / -1;
            }

            .input-group {
                order: 2;
            }

            .info-group {
                order: 1;
            }

            .action-bar .btn-acao-lateral:first-child {
                order: 3;
            }

            .action-bar .btn-acao-lateral:last-child {
                order: 4;
            }

            .btn-acao-lateral {
                min-width: 0;
                width: 100%;
                padding: 14px 10px;
                font-size: 14px;
            }

            .history-panel {
                padding: 16px 12px;
            }
        }

        @media (max-width: 420px) {
            .logo {
                font-size: 22px;
            }

            .logo-sub,
            .weight-label,
            .info-group .reg-label {
                font-size: 10px;
            }

            .status-bar {
                font-size: 12px;
            }

            .header-center-info,
            .top-info {
                font-size: 10px; /* acompanha .weight-label no mobile */
            }

            .header-center-info span {
                font-size: 16px;
            }

            .weight-value {
                font-size: clamp(28px, 11vw, 42px);
            }

            .bottom-btn-row {
                flex-direction: column;
            }

            .input-group input {
                padding: 12px;
                font-size: 14px;
                letter-spacing: 1px;
            }
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
            <div>Plataforma 1: <span id="tpeso1">--</span></div>
            <div>Plataforma 2: <span id="tpeso2">--</span></div>
        </div>
        <div class="top-info">
            <div>Bateria: <span id="tbateria">{{TBATERIA}}</span></div>
            <div>Hora: <span id="last_thora">{{THORA}}</span></div>
            <div>Data: <span id="last_tdata">{{TDATA}}</span></div>
        </div>
    </div>

    <div class="container">
        <div class="weight-grid">
            <div class="weight-box">
                <span class="weight-label">Peso Atual</span>
                <div class="weight-value" id="peso">0 kg</div>
            </div>
            <div class="weight-box">
                <span class="weight-label">Peso Acumulado</span>
                <div class="weight-value" id="pesoAcumulado">0 kg</div>
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

        <div class="bottom-btn-row">
            <button onclick="configuracao()" class="btn btn-bottom-blue">Configuração</button>
            <button onclick="imprimirRegistro()" class="btn btn-bottom-blue">Imprimir</button>
            <button onclick="baixarPlanilha()" class="btn btn-bottom-blue" style="background-color: #293462;">Baixar Planilha</button>
        </div>

        <div class="history-panel">
            <div class="history-table-wrap">
                <table class="history-table">
                    <thead>
                        <tr>
                            <th>Nº</th>
                            <th>Placa</th>
                            <th>Data</th>
                            <th>Peso</th>
                            <th>Tara</th>
                        </tr>
                    </thead>
                    <tbody id="tabela-historico">
                        <!-- As linhas do histórico serão inseridas aqui dinamicamente pelo JavaScript -->
                    </tbody>
                </table>
            </div>
        </div>

        <div id="status" class="status-bar"></div>

        <footer class="page-footer">
            <div>Desenvolvido por <strong>Eng. Raul</strong> &middot; 2026</div>
            <div><strong>Revlo do Brasil Com&eacute;rcio e Loca&ccedil;&atilde;o de Equipamentos LTDA</strong></div>
        </footer>
    </div>

    <script>
        // Variável global para armazenar o histórico e exportar depois
        let dadosHistoricoGlobal = [];

        setInterval(buscarDados, 500);

        function setStatus(msg) {
            document.getElementById('status').innerText = msg;
        }

        function buscarDados() {
            fetch('/dados')
                .then(r => r.json())
                .then(d => {
                    document.getElementById('peso').innerText = Number.isFinite(Number(d.pesoAtual)) ? Math.round(Math.max(0, Number(d.pesoAtual))) + " kg" : '0 kg';
                    document.getElementById('pesoAcumulado').innerText = Number.isFinite(Number(d.pesoAcumulado)) ? Math.round(Number(d.pesoAcumulado)) + " kg" : '0 kg';
                    document.getElementById('contadorRegistro').innerText = d.contadorRegistro !== undefined ? d.contadorRegistro : '0';
                    document.getElementById('tbateria').innerText = d.tbateria || '--';
                    document.getElementById('last_thora').innerText = d.last_thora || '--';
                    document.getElementById('last_tdata').innerText = d.last_tdata || '--';
                    document.getElementById('tpeso1').innerText = d.tpeso1 ? d.tpeso1 + " kg" : '--';
                    document.getElementById('tpeso2').innerText = d.tpeso2 ? d.tpeso2 + " kg" : '--';
                    
                    if (d.tplaca !== undefined && document.activeElement !== document.getElementById('placaInput')) {
                        document.getElementById('placaInput').value = d.tplaca;
                    }

                    // Construção dinâmica da tabela de histórico
                    if (d.historico && Array.isArray(d.historico)) {
                        dadosHistoricoGlobal = d.historico; // Salva para a planilha
                        const tbody = document.getElementById('tabela-historico');
                        let linhasHTML = '';
                        
                        d.historico.forEach(reg => {
                            linhasHTML += `
                                <tr>
                                    <td>${reg.n}</td>
                                    <td>${reg.placa}</td>
                                    <td class="history-date">${reg.data}</td>
                                    <td class="history-weight">${reg.total} kg</td>
                                    <td>${reg.tara} kg</td>
                                </tr>
                            `;
                        });
                        
                        tbody.innerHTML = linhasHTML;
                    }
                })
                .catch(() => setStatus('Erro de comunicação com o ESP32...'));
        }

        function baixarPlanilha() {
            if (dadosHistoricoGlobal.length === 0) {
                setStatus('Nenhum registro para baixar.');
                setTimeout(() => setStatus(''), 2000);
                return;
            }

            setStatus('Gerando planilha...');

            // Cabeçalho da planilha (Usando ponto e vírgula para abrir certinho no Excel em Português)
            let csvContent = "N;Placa;Data e Hora;Peso Total(kg);Tara(kg)\n";

            // Preenche as linhas
            dadosHistoricoGlobal.forEach(reg => {
                // Substitui eventuais pontos por vírgulas nos números para o padrão brasileiro
                let totalStr = String(reg.total).replace('.', ',');
                let taraStr = String(reg.tara).replace('.', ',');
                
                csvContent += `${reg.n};${reg.placa};${reg.data};${totalStr};${taraStr}\n`;
            });

            // Cria o arquivo virtual (Blob) usando codificação UTF-8
            const blob = new Blob(["\uFEFF" + csvContent], { type: 'text/csv;charset=utf-8;' });
            const url = URL.createObjectURL(blob);
            
            // Cria um link invisível e clica nele para forçar o download
            const link = document.createElement("a");
            link.setAttribute("href", url);
            link.setAttribute("download", "Historico_Balanca.csv");
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);

            setTimeout(() => setStatus('Planilha baixada!'), 1000);
            setTimeout(() => setStatus(''), 3000);
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

        function imprimirRegistro() {
            setStatus('Imprimindo...');
            fetch('/imprimir')
                .then(r => r.text())
                .then(msg => { setStatus(msg); setTimeout(() => setStatus(''), 2000); });
        }

        function configuracao() {
            alert('Função ainda não implementada, use o controlador da balança para configurações');
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