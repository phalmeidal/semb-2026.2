# Demonstracao na NUCLEO-F030R8: 8.000 elementos

## Preparar no Linux do PC conectado

Feche o monitor serial do CubeIDE e qualquer `run.py` antigo. Na raiz deste fork:

```sh
git clone https://github.com/phalmeidal/semb-2026.2.git semb-demo
cd semb-demo
sudo apt-get update
sudo apt-get install -y make gcc gcc-arm-none-eabi binutils-arm-none-eabi openocd python3-venv
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -r tests/requirements-serial.txt
make test
make flash
python3 tools/demo_placa.py --listar-portas
python3 tools/demo_placa.py --porta /dev/ttyACM0 --n 8000 --json evidencias/placa-8000.json
```

Use a porta exibida na listagem, caso seja diferente de `/dev/ttyACM0`.
Se ja estiver neste fork, use `git pull --ff-only` na main antes dos comandos.
O clone em uma pasta nova preserva o projeto local da foto e evita misturar fontes.

`make flash` usa SWD/ST-LINK, grava, verifica e reseta. Nao use o build CubeIDE
do upstream: este projeto inclui seu proprio startup, linker e main. O USB deve
estar no conector ST-LINK, com cabo de dados; USART2 PA2/PA3 usa a porta virtual.

Se houver permissao negada na serial, confira `ls -l /dev/ttyACM0`. Em Ubuntu,
adicione seu usuario ao grupo `dialout` com `sudo usermod -aG dialout "$USER"`,
encerre e reabra a sessao. Para ST-LINK, instale as regras udev do OpenOCD
fornecidas pelo pacote e reconecte o USB. Nao execute pip com sudo.

## O que mostrar

1. `INFO` confirma firmware/protocolo v2; `TEST` executa autoteste na placa.
2. O cliente gera 8.000 inteiros int32, incluindo negativos e extremos.
3. A placa ordena 16 blocos com max-heap; depois intercala os runs em quatro
   passagens. Cada decisao entre as duas janelas ocorre no C do MCU.
4. O PC copia o prefixo informado pelo MCU, preserva o resto de cada janela e
   envia as proximas. Uma cauda e copiada diretamente somente quando o outro
   run inteiro se esgota; nao ha comparacao de ordenacao no PC.
5. `PASSOU ... (placa_uart)` significa igualdade integral com `sorted()`.
   O JSON registra meio, porta, n, tempo total e hash do resultado. Apresente
   o armazenamento externo como ajuste de arquitetura, sem afirmar que os
   8.000 elementos ficam simultaneamente na RAM interna do STM32.

O tempo mostrado inclui UART, espera e transporte; `sort_us` do INFO mede
somente o ultimo heapsort de bloco, nao a ordenacao global. Reserve alguns
minutos para a demonstracao serial. O protocolo textual nao possui CRC.

## Testes completos e reproducao sem hardware

```sh
python3 tests/validar.py --porta /dev/ttyACM0 --referencia src/t1/bin/heapsort --json evidencias/placa.json
python3 tools/demo_placa.py --host build/t2_host --n 8000 --json evidencias/demo-8000-host.json
make sanitize
make firmware test-limite
```

A suite fisica inclui timeout de 5 segundos. `host` nunca comprova execucao na
placa. O relatorio fisico somente existe apos executar no PC conectado.

## Erro da foto

`Timeout or incomplete header` vem antes de `TypeError: ... NoneType ... len()`.
O script antigo recebe `None` e chama `len()`; alterar apenas essa linha esconde
a falta de resposta. O upstream tambem usa cabecalho de tamanho de um byte,
buffer local grande e passa tamanho em bytes como quantidade de elementos.
No fork o caminho de demonstracao e firmware textual + cliente textual.

- Timeout: confira gravacao com `make flash`, porta, cabo e monitor serial fechado.
- Firmware incompativel: grave novamente este fork; nao misture baud/protocolos.
- `ERR RX`: pressione RESET e execute novamente; verifique cabo e programas
  concorrentes. O cliente envia rajadas curtas para aliviar o parser a 8 MHz.
- `ERR N` no protocolo de lotes: maximo 512 por linha; use `demo_placa.py`
  para a ordenacao global de 8.000 ou mais.
- O antigo caminho `python3 src/t2/tests/run.py --porta /dev/ttyACM0` agora
  encaminha para a mesma demonstracao v2, sem `struct.unpack` sobre `None`.

## Limite de verificacao

Compilacao ARM, testes de host e sanitizadores sao verificaveis remotamente.
Gravacao SWD, clock/baud real, cabos, pontes e o resultado fisico dependem do
PC conectado. Nao marque evidencia de placa como concluida antes de obter
`PASSOU` com `meio=placa_uart` nesse PC.
