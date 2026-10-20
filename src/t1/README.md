# T1 — ordenação com max-heap de capacidade fixa

> O programa ordena um lote de inteiros usando uma max-heap binária de capacidade fixa.

> Recebe um vetor com até 8.000 valores `int32_t` e armazena os valores em uma heap separada.

> Remove repetidamente o maior valor e preenche o vetor de saída de trás para frente, em ordem crescente.

> A construção por inserções e as remoções custam `O(n log n)` no pior caso; a heap auxiliar usa `O(n)` de memória.

> Pode organizar lotes de amostras ou prioridades inteiras em sistemas embarcados que disponham da RAM necessária.

## Estrutura e interface

| Arquivo | Responsabilidade |
| --- | --- |
| `include/heap.h`, `heapsort/heap.c` | `Heap` com `dados[8000]` e `tamanho`; inicializar, inserir, remover máximo, consultar se está vazia. |
| `include/heapsort.h`, `heapsort/heapsort.c` | `heapsort_ordenar(vetor, quantidade)`: copia para a heap e escreve o resultado crescente no mesmo vetor. |
| `main.c` | Lê a entrada textual e exibe o resultado, apenas no host. |

## Executar e verificar

Requer compilador C11, `make` e Python 3 para os testes. Na pasta do projeto:

```sh
make test
```

`make test` executa verificações da propriedade da max-heap após inserções e remoções, testa heap vazia e cheia, números extremos, duplicatas e o tamanho máximo.
Depois compara 8 casos de entrada do programa, inclusive vetores de 8000 elementos em ordem crescente, decrescente, iguais e com duplicatas, com `sorted()` do Python, usado como implementação de referência independente.
Também verifica entradas inválidas.
O resultado esperado é o mesmo vetor ordenado, sem perda ou duplicação de valores, e todos os testes marcados `OK`.

Nesta primeira versão, todos os testes passaram no host; os 26 casos também passaram com os verificadores de acesso à memória e comportamento indefinido habilitados (detecção de vazamentos desabilitada porque não funciona neste ambiente de execução).

## Fontes e referência

- [MIT OpenCourseWare — Lecture 4: Heaps and Heap Sort](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-fall-2011/resources/lecture-4-heaps-and-heap-sort/): descrição da fila de prioridade, operações de heap e ordenação.
- [MIT OpenCourseWare — Lecture 8: Binary Heaps (PDF)](https://ocw-preview.odl.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/40d4851e550507ca14dc778b9b2266cc_MIT6_006S20_lec8.pdf): construção por inserções, remoção do máximo e comparação com a versão in-place.
- [Documentação oficial do Python — `heapq`](https://docs.python.org/3/library/heapq.html): propriedade de min-heap/max-heap e operações de inserção e remoção.
- [Documentação oficial do Python — `sorted()`](https://docs.python.org/3/library/functions.html#sorted): função independente usada como resultado de referência nos testes.
