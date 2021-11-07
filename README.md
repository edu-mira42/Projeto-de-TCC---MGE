# Projeto de TCC - MGE
Programação integral do projeto de TCC Medidor de Grandezas Elétricas, desenvolvido em 2021.

Todos os arquivos do projeto estão disponibilizados neste repositório, divididos em pastas, sendo portanto:

- SRC -> "Source Code", o código em C++ no formato ".ino" utilizado através do Arduino IDE. Para o uso do software em questão com o microcontrolador utilizado, o ESP32, é necessário instalar o gerenciador de placas "Espressif IDF", disponpivel em: <https://dl.espressif.com/dl/package_esp32_index.json>. (É necessário colar o link na gua de preferências do Arduino IDE e intalar o Gerenciador de placas na aba dedicada).

- data -> Todos os arquivos que devem ser intalados na memória flash do microcontrolador, que consistem na programação em HTML/CSS/JavaScript e imagens/documentos de configuração.

- CrendentialsManager -> Biblioteca desenvolvida exclusivamente para este projeto, deve ser instalada na pasta "libraries" que é localizada dentro da pasta "C:\\USER\DOCUMENTS\ARDUINO\LIBRARIES" (no Windows). As demais bibliotecas, por questões de direitos de uso, estão listadas abaixo, porém, não estarão disponíveis para download neste repositório.


   - BIBLIOTECAS UTILIZADAS

As bibliotecas EmonLib, ESPAsyncWebServer, AsyncTCP, AsyncElegantOTA e CredentialsManager precisam ser instaladas separadamente. As demais já estão inclusas com o Arduino IDE ou são instaladas ao adicionar o Gerenciador de Placas do ESP32.
A biblioteca "CredentialsManager foi desenvolvida pelos próprios autores do projeto, portanto, está disponível neste repoitório, na pasta "BIBLIOTECA DEV".

Links de instalação das bibliotecas:
- EmonLib: <https://github.com/openenergymonitor/EmonLib>
- ESPAsyncWebServer: <https://github.com/me-no-dev/ESPAsyncWebServer>
- AsyncTCP: <https://github.com/me-no-dev/AsyncTCP>
- AsyncElegantOTA: <https://github.com/ayushsharma82/AsyncElegantOTA>


    - GERENCIADOR DE PLACAS E ENVIO DOS ARQUIVOS PARA A MEMÓRIA


Para o "upload" dos arquivos contidos na pasta DATA, é necessário instalar no Arduino IDE a ferramenta "esptool.py", que está disponível a partir do link <https://github.com/me-no-dev/arduino-esp32fs-plugin/releases/> e deve ser instalada na pasta "tools", que está localizada dentro da pasta de instalação do Arduino IDE.


Tutorial de como instalar o Gerenciador de Placas para o ESP32 (Blog UsinaInfo): <https://www.usinainfo.com.br/blog/programar-esp32-com-a-ide-arduino-tutorial-completo/>

Tutorial de como instalar a ferramenta "esptool.py" (Efeito Nerd): <http://www.efeitonerd.com.br/2021/06/gravar-arquivos-no-esp32-spiffs.html>
