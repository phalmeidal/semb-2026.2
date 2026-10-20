# Demonstração na placa

Esta etapa requer a NUCLEO-F030R8 física. Os resultados do PC e do ELF não comprovam o funcionamento no hardware.

## Preparar e gravar

1. Confira o modelo STM32F030R8T6, cabo USB de dados, alimentação, jumpers SWD e a porta VCP. Na configuração padrão, SB13/SB14 ligam USART2 ao ST-LINK. Consulte UM1724 se a placa já foi modificada.
2. Execute `make clean`, `make test`, `make firmware` e registre as versões do compilador e OpenOCD. Guarde o `.map`, `.su` e o SHA-256 do `build/t2.bin`.
3. Grave com `make flash`. Alternativa: STM32CubeProgrammer, conexão ST-LINK/SWD, arquivo `build/t2.hex`, verificação e reset. Para o `.bin`, endereço **0x08000000**. Não junte o firmware a outro startup/main gerado pelo CubeIDE.
4. Observe LD2 alternar. Abra a VCP em **115200 8N1**, sem controle de fluxo, enviando LF ou CRLF. Feche o terminal antes de usar a suite Python.

## Mostrar ao professor

| Etapa | Ação | Resultado esperado |
| --- | --- | --- |
| Identificar | Mostrar a placa e enviar `INFO` | `cap=512 heap_bytes=2052 vetor_bytes=2048`, guarda íntegra |
| Ordenar | `7 4 -2 7 4 0 -8 3` | `-8 -2 0 3 4 4 7` |
| Extremos | `5 -2147483648 2147483647 0 -2147483648 2147483647` | Dois mínimos, zero, dois máximos |
| Recuperar | `513`, depois `3 9 -1 0` | `ERR N`, depois `-1 0 9`, sem reset |
| Autoteste | `TEST` | `TEST OK`, incluindo lote de 512 |
| Comparar | Suite UART abaixo | 58 casos aprovados, se o hardware estiver correto |
| Medir | `INFO` após lote de interesse | Contadores e `sort_us` reais |

```sh
python3 -m pip install -r tests/requirements-serial.txt
python3 tests/validar.py --porta /dev/ttyACM0 \
  --referencia src/t1/bin/heapsort --json evidencias/placa.json
```

A suite usa semente fixa 20262, compara todos os elementos com T1/Python e verifica recuperação no mesmo MCU. O timeout real exige esperar 5 s. Respeite uma resposta por requisição; o buffer RX tem 255 bytes úteis e não comporta envio contínuo durante respostas extensas.

## Registrar a evidência física

Preencha `evidencias/placa-observacoes.md`: data, modelo/revisão, PC, porta, ferramentas, hash do binário gravado e resultado da gravação. Guarde o JSON, print do terminal e foto da placa conectada. O script registra o código-fonte usado; não verifica automaticamente qual binário está na Flash.

Meça `sort_us` após lotes crescentes, decrescentes e aleatórios de 512 elementos. `INFO` não altera esse tempo. Separe ordenação de E/S: aproximadamente 5.600 caracteres levam 0,49 s para transmitir a 115200 8N1. Não apresente tempos do host como tempos do MCU. Depois preencha as observações e execute `make slides`.

## Diagnóstico

| Sintoma | Verificar |
| --- | --- |
| Não grava | Cabo de dados, alimentação, jumpers CN2, ST-LINK/SWD, permissões e outro depurador aberto |
| Sem VCP | Driver/firmware do ST-LINK e enumeração USB |
| Sem resposta | LF no fim, 115200 8N1, terminal exclusivo, SB13/SB14, modelo correto |
| Texto corrompido | Configuração, clock, ruído; BRR nominal 69 com HSI 8 MHz |
| `ERR RX` | Parar envio contínuo, enviar LF, aguardar resposta; consultar `rx_erros` |
| `ERR TIMEOUT` | Evitar pausa de 5 s durante o lote; enviar LF antes do próximo |
| LD2 fixo | SWD: `diagnostico.falha`: 1 HardFault, 2 vetor inesperado, 3 TX travado, 4 guarda da pilha |

Para depurar, rode OpenOCD e, em outro terminal na raiz, `arm-none-eabi-gdb -x docs/debug.gdb`. A UART não é chamada nas ISRs. A guarda é uma sentinela: não mede pior caso de pilha nem impede a corrupção.
