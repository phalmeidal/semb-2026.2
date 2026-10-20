# Evidências

`host.json` e `validacao.log`: execução no PC, 57 casos T2 e 41 comparações com T1/Python; o log inclui os testes C, T1 e compilação ARM.

`sanitizers.json` e `sanitizers.log`: os mesmos testes com ASan/UBSan. A detecção de vazamentos é desabilitada no alvo `sanitize`; o núcleo não usa alocação dinâmica.

`memoria.json`: tamanhos do ELF ARM, sem afirmar execução física. O `.map` e `.su` são regenerados pelo compilador.

`firmware.log`: conferência dos vetores de reset/SysTick/USART2, ausência de símbolos indefinidos e teste de rejeição de uma capacidade experimental que excede o orçamento de RAM.

**Placa: pendente.** `placa.json` não é fornecido: deve ser produzido pela suite UART no hardware real. Preencha também as observações. Não converta resultados do PC em resultados do MCU.
