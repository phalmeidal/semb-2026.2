# T2 — max-heap na NUCLEO-F030R8

Porta o T1 para o **STM32F030R8T6: Cortex-M0, 64 KiB de Flash, 8 KiB de RAM**. Preserva a max-heap auxiliar separada, a saída crescente e todo o intervalo de `int32_t`. A RAM comporta **512 elementos por lote**. Para ordenar **8.000 ou mais globalmente**, `tools/demo_placa.py` usa o PC como armazenamento externo: o MCU ordena os blocos com max-heap e intercala janelas de até 256 + 256 inteiros. O PC apenas transporta e guarda os runs; `sorted()` é usado exclusivamente como referência depois do resultado final. Isso exige PC conectado durante a demonstração; não é uma ordenação autônoma de 8.000 valores em RAM interna.

**Comece por [docs/demonstracao-8000.md](docs/demonstracao-8000.md)** para gravar e executar. Este fork usa protocolo **textual a 115200 baud**, incompatível com `ClientUART` binário a 38400 do repositório original. Grave o firmware deste fork e use o cliente correspondente. O erro `len(None)` da foto é consequência de tentar interpretar uma resposta ausente; este cliente interrompe com diagnóstico de timeout/protocolo.

**Validação:** testes no PC e compilação ARM concluídos; execução e medições **na placa pendentes**. A suite serial está pronta. Compilação não comprova periféricos ou funcionamento físico.

## Executar no PC

Requer C11, Make e Python 3. Na raiz:

```sh
make test
printf '7 4 -2 7 4 0 -8 3\n' | build/t2_host
# -8 -2 0 3 4 4 7
make sanitize
```

São **57 casos T2**: 41 lotes comparados integralmente com o T1 e com `sorted()`; 14 entradas inválidas, cada uma seguida de um lote válido no mesmo processo; CRLF/sinais e autoteste. A extensão v2 acrescenta oito cenários de ordenação global (incluindo 8.000, 8.001 e 16.000 elementos) e nove casos inválidos de MERGE com recuperação. Também executa os testes originais do T1, testes da propriedade da heap com capacidade 512 e testes C de timeout, wrap do relógio e recuperação de RX. `make sanitize` recompila e executa com ASan/UBSan no PC. O T1 continua com capacidade 8.000; seus caminhos de build foram corrigidos.

## Compilar e gravar na placa

Instale Arm GNU Toolchain e OpenOCD. Em Debian/Ubuntu: pacotes `gcc-arm-none-eabi`, `binutils-arm-none-eabi` e `openocd`.

```sh
make firmware
# Compilador fora do PATH: make firmware ARM_PREFIX=/caminho/bin/arm-none-eabi-
make test-limite
make flash
```

São gerados `build/t2.elf`, `.bin`, `.hex`, `.map` e `.su`. O linker reserva 1.536 bytes para pilha, incluindo 64 de guarda, e recusa dados estáticos que invadam a reserva. `make test-limite` verifica isso em uma cópia temporária com capacidade experimental 1.024, que deve falhar na linkedição; não altera a entrega de 512. O MCU não usa `malloc`, libc, semihosting ou RTOS. É um projeto Make independente: **startup e tabela de vetores já estão incluídos; não acrescente outro startup/main do CubeIDE**.

Conecte o USB do ST-LINK com cabo de dados e mantenha os jumpers SWD. A porta virtual usa USART2 em PA2/PA3, AF1, conectada ao ST-LINK por SB13/SB14 na configuração padrão. Confira a revisão da placa antes de alterar pontes. O firmware usa HSI interno a **8 MHz**, independente do MCO/cristal da revisão.

```sh
python3 -m venv .venv
# Linux/macOS: . .venv/bin/activate
# Windows: .venv\Scripts\activate
python3 -m pip install -r tests/requirements-serial.txt
python3 tests/validar.py --porta /dev/ttyACM0 \
  --referencia src/t1/bin/heapsort --json evidencias/placa.json
# Windows: use COM3 ou a porta identificada no Gerenciador.
```

Feche outros terminais usando a porta. A suite UART repete os 57 casos e acrescenta o timeout físico de 5 s: **58 casos previstos na placa**. O relatório registra `meio=placa_uart`; só a execução com hardware pode concluir essa etapa. A referência T1 deve ser compilada no PC da demonstração.

## Protocolo

**115200 baud, 8N1, sem controle de fluxo.** Uma requisição por linha; aguarde a resposta completa antes de enviar outra. `n` e seus valores devem estar na mesma linha. Aceita espaços, tabs e CRLF; cada token tem no máximo 11 caracteres. No PC, EOF também finaliza um lote; na UART exige-se LF.

| Enviar (terminado em LF) | Resposta |
| --- | --- |
| `7 4 -2 7 4 0 -8 3` | `-8 -2 0 3 4 4 7` |
| `0` | Linha vazia |
| `INFO` | Capacidade, versão do protocolo (`versao=2`), tamanhos, contadores, guarda e tempo do último sort |
| `MERGE a b ...` | `MERGE consumidos_a consumidos_b prefixo...`; duas janelas crescentes, 1–256 valores cada |
| `TEST` | `TEST OK` se extremos e lote máximo passarem |
| `513` ou `-1` | `ERR N` |
| `1 2147483648` | `ERR VALOR` |
| `3 1 2` ou `1 2 3` | `ERR QUANTIDADE` |

Erros de sintaxe são respondidos no fim da linha. Após 5 s sem bytes em uma transação, retorna `ERR TIMEOUT`; ruído, overrun e estouro do buffer RX geram `ERR RX`. Nesses dois casos, envie LF para descartar a cauda antes da próxima transação. O protocolo não tem CRC: uma alteração que continue sendo um número válido pode escapar da detecção.

LD2 alterna a cada 500 ms no main; uma transmissão longa pode atrasar a alternância. Aceso fixo indica falha fatal inspecionável por SWD. `sort_us` mede somente a ordenação, excluindo E/S, e depende da tolerância do HSI. No host, `ram_estatica=0` significa não medido nesse backend; `sizeof(Heap)` pode diferir do MCU por `size_t` e alinhamento.

## Memória medida no ELF

GCC Arm 13.2.1, `-Os`, Cortex-M0, sem libc (protocolo v2):

| Região | Bytes |
| --- | ---: |
| Heap: 512 inteiros e tamanho | 2.052 |
| Vetor de entrada/saída | 2.048 |
| RX, parser, diagnóstico e demais dados | 340 |
| **RAM estática total** | **4.440** |
| Reserva de pilha, incluindo guarda | 1.536 |
| Margem não comprometida | 2.216 |
| Flash utilizada | 3.256 |

Fonte: [`evidencias/memoria.json`](evidencias/memoria.json). Outro compilador pode mudar os valores. Reserva de pilha não é medição do uso máximo; a guarda detecta algumas invasões e não prova ausência de overflow. O algoritmo continua `O(n log n)` em tempo e `O(n)` em memória auxiliar, sem recursão. A heap estática interna torna a ordenação não reentrante; somente o main a chama.

## Arquivos e entrega

| Caminho | Responsabilidade |
| --- | --- |
| `src/heapsort/`, `src/include/` | Núcleo original, agora com capacidade configurável |
| `src/t1/` | Referência PC, com capacidade 8.000 |
| `src/t2/app.c` | Parser incremental, validação, comandos e diagnóstico |
| `src/t2/platform.c` | PC/MCU, startup, UART, interrupções, clock e GPIO |
| `src/t2/stm32f030r8.ld` | Mapa de memória e proteção da reserva de pilha |
| `tests/` | Testes unitários e suite comparativa PC/UART |
| `entrega/t2.c` | **Único arquivo C comentado para entrega** |
| `docs/apresentacao.pdf` | Slides de 8 minutos, com pendência física explícita |
| `docs/roteiro.md` | Fala, divisão de tempo e perguntas individuais |
| `docs/demonstracao.md` | Gravação, testes físicos e diagnóstico |

Edite os módulos e execute `make entrega`; não edite a cópia gerada. Os dois builds compilam **o próprio `entrega/t2.c`**. O autoteste reutiliza o vetor e a heap existentes, sem uma terceira área grande. A demonstração global também exige o cliente Python; o arquivo C sozinho processa apenas as janelas. Os slides existentes descrevem a versão de lotes e precisam refletir esta extensão antes da apresentação.

Use o PDF na entrega de slides e **apenas `entrega/t2.c` na entrega de código**. O pacote completo permite reproduzir o projeto. Após testar na placa, preencha as observações reais e gere novamente o PDF com `make slides` (dependência opcional em `tools/requirements-slides.txt`).

Prazos do enunciado: demonstrações **19–23/10**, apresentações **26 e 30/10**, slides **30/10**, código **31/10**. O roteiro dá foco a migração, testes e aprendizado, incluindo a avaliação individual.

## Referências

- [ST — DS9773, STM32F030](https://www.st.com/resource/en/datasheet/stm32f030r8.pdf): memórias, características e funções alternativas.
- [ST — UM1724, Nucleo-64 MB1136](https://www.st.com/resource/en/user_manual/dm00105823.pdf): placa, ST-LINK, USART e pontes.
- [ST — RM0360](https://www.st.com/resource/en/reference_manual/dm00091010.pdf): registradores.
- [ST — CMSIS STM32F030x8](https://github.com/STMicroelectronics/cmsis-device-f0/blob/master/Include/stm32f030x8.h) e [startup oficial](https://github.com/STMicroelectronics/cmsis-device-f0/blob/master/Source/Templates/gcc/startup_stm32f030x8.s): conferência de endereços, bits e vetores. O projeto escreve seu suporte mínimo próprio.
- [MIT — Heaps and Heap Sort](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-fall-2011/resources/lecture-4-heaps-and-heap-sort/): algoritmo do T1.
- [Python — sorted()](https://docs.python.org/3/library/functions.html#sorted): referência independente.

