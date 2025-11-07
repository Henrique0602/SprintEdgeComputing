Integrantes: Eric Yang rm 563290, Pietra Satollo rm 562538, Raul Laguna , Pedro Gomes rm 562684 e Henrique Fara Brigo rm 563323

# Monitoramento inteligente do campo feminino
Criamos um sistema completo para monitorar um campo de futebol feminino, coletar clima de borda e detectar gols em tempo real. O ESP32 fica instalado proximo a trave, captura temperatura/umidade para entender o conforto das atletas e aciona um botao dedicado sempre que a bola cruza a linha, disparando uma animacao de gol no painel web.

## Passo a passo do codigo (`iot-device/sprint.ino`)
1. **Sensores e pinos**: define o DHT11 no GPIO 4 para clima e um botao no GPIO 5 (D4) com `INPUT_PULLUP`. Tambem reserva o LED onboard (GPIO 2) para feedback visual.
2. **Conexao Wi-Fi**: `setup_wifi()` coloca o ESP32 em modo `WIFI_STA`, conecta na rede (`ssid` e `password`) e so prossegue quando recebe IP.
3. **Cliente MQTT**: `PubSubClient` usa `broker.hivemq.com`. A funcao `reconnect()` garante que, se cair, o dispositivo tenta reconectar, publica um "hello" no topico `iotfrontier/mqtt` e se inscreve no mesmo topico para comandar o LED remotamente via `callback()`.
4. **Leitura do DHT11**: a cada 2 segundos (`if (now - lastMsg > 2000)`), o ESP32 le temperatura e umidade com `dhtSensor.getTempAndHumidity()`, imprime na serial e publica em `iotfrontier/temperatura` e `iotfrontier/humidade`.
5. **Debounce do botao**: as funcoes `readButtonPressedRaw()` e `edgePressed()` filtram o ruido mecanico. So quando ha transicao real para pressionado e que retornam `true`.
6. **Deteccao de gol**: sempre que `edgePressed()` dispara, o ESP32 envia `gol_detectado` no topico `iotfrontier/motion`. Esse evento alimenta a automacao que representa o gol no painel do campo feminino.
7. **Loop principal**: mantem o cliente MQTT ativo (`client.loop()`), publica clima, trata o botao e aplica um `delay(5)` curto apenas para aliviar CPU sem travar o debounce.

## Funcionalidades principais
- Monitoramento continuo de conforto termico de borda do gramado.
- Deteccao instantanea de gols por botao fisico instalado na trave.
- Feedback visual no LED onboard (via comandos MQTT recebidos).
- Dashboard web estilizado em preto e dourado (`frontend/index.html`) com animacao da bola e status de movimento.

## Fluxo de dados
1. **Campo**: ESP32 capta dados e envia para o broker MQTT publico.
2. **Node-RED**: um flow escuta `iotfrontier/temperatura`, `iotfrontier/humidade` e `iotfrontier/motion`, organiza os valores em JSON e expoe um endpoint HTTP (`http://localhost:1880/dados`).
3. **Painel**: `frontend/index.html` faz `fetch` no endpoint a cada 5 s. Atualiza temperatura/umidade com `fmtNum()` e converte o campo `motion` em animacao de gol pelo metodo `setMotion()`.

## Como executar o projeto
1. **Hardware**: conecte o DHT11 ao 3V3, GND e GPIO 4, e fixe um botao NA entre GPIO 5 e GND (usando o pull-up interno). Posicione o conjunto perto da trave do campo feminino.
2. **Configurar rede**: edite `ssid` e `password` em `iot-device/sprint.ino` para a rede disponivel no estadio. Compile e envie o sketch para o ESP32.
3. **Broker e Node-RED**:
   - Ligue o Node-RED na mesma rede e garanta acesso ao `broker.hivemq.com:1883`.
   - Crie um flow com tres `mqtt in` (temperatura, humidade, motion) ligados a um `function`/`template` que monte um objeto `{ temperatura, humidade, motion }`.
   - Finalize com um `http response` no caminho `/dados` e mantenha o servidor escutando `http://localhost:1880`.
4. **Dashboard**: com Node-RED rodando, abra `frontend/index.html` (Live Server ou `npx serve frontend`) para visualizar o painel. O script interno ja aponta para `http://localhost:1880/dados`.
5. **Operacao**: conecte o ESP32 na energia, aguarde o log de Wi-Fi/MQTT na serial e pressione o botao quando houver gol. Os valores climaticos e o alerta de gol devem surgir automaticamente no dashboard.

Com esses passos o monitoramento do campo feminino e a deteccao de gols ficam integrados de ponta a ponta: dispositivo -> MQTT -> Node-RED -> painel web..
