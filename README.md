# Projeto de TCC - MGE
Programação integral do projeto de TCC Medidor de Grandezas Elétricas, desenvolvido em 2021.

Todos os arquivos do projeto estão disponibilizados neste repositório, divididos em pastas, sendo portanto:

- SRC -> "Source Code", o código em C++ no formato ".ino" utilizado através do Arduino IDE. Para o uso do software em questão com o microcontrolador utilizado, o ESP32, é necessário instalar o gerenciador de placas "Espressif IDF", disponpivel em: <https://dl.espressif.com/dl/package_esp32_index.json>. (É necessário colar o link na gua de preferências do Arduino IDE e intalar o Gerenciador de placas na aba dedicada).

- DATA -> Todos os arquivos que devem ser intalados na memória flash do microcontrolador, que consistem na programação em HTML/CSS/JavaScript e imagens/documentos de configuração.

- BIBLIOTECA DEV -> Biblioteca desenvolvida exclusivamente para este projeto, deve ser instalada na pasta "libraries" que é localizada dentro da pasta "C:\\USER\DOCUMENTS\ARDUINO\LIBRARIES" (no Windows). As demais bibliotecas, por questões de direitos de uso, estão listadas abaixo, porém, não estarão disponíveis para download neste repositório.

Para o "upload" dos arquivos contidos na pasta DATA, é necessário instalar no Arduino IDE a ferramenta "esptool.py", que está disponível a partir do link <https://github.com/me-no-dev/arduino-esp32fs-plugin/releases/> e deve ser instalada na pasta "tools", que está localizada dentro da pasta de instalação do Arduino IDE.


Tutorial de como instalar o Gerenciador de Placas para o ESP32 (Blog UsinaInfo): <https://www.usinainfo.com.br/blog/programar-esp32-com-a-ide-arduino-tutorial-completo/>

Tutorial de como instalar a ferramenta "esptool.py" (Blog Embarcados): <https://www.embarcados.com.br/spiffs-o-sistema-de-arquivos-do-esp8266-32/>
