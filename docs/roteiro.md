# Apresentação — 8 minutos

O PDF distingue PC, compilação ARM e validação física pendente. Depois da demonstração, complete as observações e gere-o novamente. Cada integrante deve saber justificar decisões e repetir os testes: 30% da avaliação depende do conhecimento individual.

| Slide | Tempo | Fala |
| --- | --- | --- |
| 1 | 0:00–0:30 | Objetivo: portar o T1 à Nucleo de 8 KiB. Preservar inteiros/algoritmo; adaptar capacidade e E/S. |
| 2 | 0:30–1:10 | Max-heap em vetor, inserção sobe, remoção desce; preencher a saída de trás para frente. `O(n log n)` e heap auxiliar `O(n)`. |
| 3 | 1:10–2:10 | 8.000 inteiros ocupam 32.000 bytes por vetor. Dois não cabem. Mostrar escolha de 512, dados estáticos, reserva de pilha e margem. |
| 4 | 2:10–3:00 | UART substitui stdin/stdout; parser incremental; ISR de RX e relógio, sort somente no main. Mostrar fluxo. |
| 5 | 3:00–4:20 | Referências T1 e sorted(), comparação de todos os elementos, semente fixa, limites, inválidos e recuperação. Mesmo C nos dois alvos, mesma suite no pipe/UART. |
| 6 | 4:20–5:20 | Resultados reais do PC e do ELF. Após executar na placa, apresentar JSON, foto e tempos medidos. Enquanto não houver, informar pendência. |
| 7 | 5:20–7:00 | Dificuldades: RAM além dos vetores, bibliotecas, tamanho/alinhamento, INT32_MIN, main/ISR, delimitação e recuperação. Relacionar cada solução ao teste. |
| 8 | 7:00–8:00 | Aprendizados: orçamento de memória, E/S como parte do porte, compilação versus hardware, medir o trecho certo. Limitação dos 8.000 itens. |

Divisão sugerida: A apresenta 1–4 (3 min), B apresenta 5–6 (2 min); ambos dividem 7–8 (3 min). Ensaiar com papéis trocados também.

## Perguntas individuais

- **Por que reduzir para 512?** Dois vetores usam 4.100 bytes no MCU e deixam orçamento para RX, parser, diagnóstico e pilha. Não é o máximo teórico; é uma escolha conservadora.
- **Por que não in-place ou int16_t?** In-place ainda precisaria de 32 KB para 8.000 int32_t. Int16_t perderia valores. Mantivemos a proposta de heap separada aceita no T1.
- **Static resolve RAM?** Não; evita grandes vetores na pilha, mas consome SRAM pelo mesmo tamanho.
- **É in-place?** A saída volta ao vetor original, porém há heap separada: memória auxiliar O(n).
- **Por que não ordenar na ISR?** Atrasaria recepção/relógio e a função não é reentrante. ISR só recebe bytes ou atualiza o relógio.
- **Volatile torna tudo atômico?** Não. Acessos simples de 32 bits alinhados são atômicos neste MCU; flush/flag do ring buffer usam seção crítica.
- **Como evita overflow numérico?** Verifica capacidade antes de escrever e limites antes de acumular dígitos; não nega diretamente INT32_MIN.
- **Como recupera erros?** Códigos UART, contadores, timeout e descarte até LF. Limites sintáticos não detectam toda corrupção: o protocolo não tem CRC.
- **Como comprova os resultados?** Comparação de todos os elementos com duas referências; para hardware, executar UART e guardar evidência física.
- **Guarda comprova ausência de estouro?** Não. É uma sentinela, sem MPU e sem medição de pior caso.
- **O que mede sort_us?** Somente ordenação. UART pode dominar tempo total; HSI tem tolerância.

Antes de entregar o PDF em 30/10, registre a demonstração real, revise as evidências e execute `make slides`. Código comentado: apenas `entrega/t2.c`, em 31/10.
