/* T2 de Sistemas Embarcados -- NUCLEO-F030R8 (STM32F030R8T6).
 * Autores do projeto: Anilton Magalhaes de Castro e Pedro Henrique Almeida Lima.
 * Licenca MIT; veja LICENSE no repositorio de origem.
 * GERADO por tools/gerar_entrega.py. Edite os fontes modulares, nao esta copia.
 * Este e o UNICO arquivo de codigo a submeter na entrega do T2.
 * Host: cc -std=c11 -O2 -DT2_HOST entrega/t2.c -o build/t2_host
 * MCU: make firmware (usa src/t2/stm32f030r8.ld, startup ja incluido aqui).
 * UART: 115200 8N1, uma linha por lote, limite 512 int32_t, comandos INFO/TEST.
 * Nao juntar este startup a outro startup/main gerado pelo CubeIDE.
 */
#define HEAP_CAPACIDADE 512u

/*
 * MIT License
 *
 * Copyright (c) 2026 Anilton Magalhães
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* ===== src/include/heap.h ===== */
#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* O T1 usa 8000; o T2 seleciona 512 antes de incluir este cabecalho. */
#ifndef HEAP_CAPACIDADE
#define HEAP_CAPACIDADE 8000u
#endif

/* Max-heap binaria, com capacidade fixa e sem alocacao dinamica. */
typedef struct {
    int32_t dados[HEAP_CAPACIDADE];
    size_t tamanho;
} Heap;

/* Todas as funcoes exigem um ponteiro Heap valido. */
void heap_inicializar(Heap *heap);
bool heap_esta_vazia(const Heap *heap);
bool heap_inserir(Heap *heap, int32_t valor); /* false se cheia */
bool heap_remover_maximo(Heap *heap, int32_t *maior); /* false se vazia */

#endif

/* ===== src/include/heapsort.h ===== */
#ifndef HEAPSORT_H
#define HEAPSORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Ordena em ordem crescente. Aceita NULL somente quando quantidade == 0.
 * Retorna false, sem alterar o vetor, para quantidade acima da capacidade.
 * Usa uma unica heap estatica interna; nao é reentrante.
 */
bool heapsort_ordenar(int32_t dados[], size_t quantidade);

#endif

/* ===== src/t2/t2.h ===== */
#ifndef T2_H
#define T2_H

#include <stdbool.h>
#include <stdint.h>

#define T2_TIMEOUT_MS 5000u

/* Visivel por SWD/GDB. A ISR de RX atualiza somente rx_erros. */
typedef struct {
    uint32_t ordenacoes;
    uint32_t erros;
    uint32_t ultimo_us;
    uint32_t rx_erros;
    uint32_t falha;
} Diagnostico;
extern volatile Diagnostico diagnostico;

/* Camada de plataforma: nenhuma funcao do algoritmo acessa registradores. */
void plataforma_enviar(char byte);
uint32_t plataforma_us(void);
uint32_t plataforma_ram_estatica(void);
bool plataforma_pilha_ok(void);

void t2_receber(char byte, uint32_t agora_ms);
void t2_verificar_timeout(uint32_t agora_ms);
void t2_erro_rx(void);

#endif

/* ===== src/heapsort/heap.c ===== */

void heap_inicializar(Heap *heap)
{
    heap->tamanho = 0;
}

bool heap_esta_vazia(const Heap *heap)
{
    return heap->tamanho == 0;
}

/** Insere elemento na Heap
 * @param heap ponteiro para heap válida
 * @param valor valor a ser inserido na heap
 * @return retorna `true` caso operação tenha sido concluída com éxito.
 *
 * Começa da ultima posição, checa se o pai é maior ou igual a valor, caso não: desce
 * ele e repete o processo com a posição dele.
 * No fim, insere valor na posição vaga.
 */
bool heap_inserir(Heap *heap, int32_t valor)
{
    if (heap->tamanho == HEAP_CAPACIDADE) {
        return false;
    }

    size_t indice = heap->tamanho++;
    while (indice > 0) {
        size_t pai = (indice - 1u) / 2u;
        if (heap->dados[pai] >= valor) {
            break;
        }
        heap->dados[indice] = heap->dados[pai];
        indice = pai;
    }
    heap->dados[indice] = valor;
    return true;
}

/** Remove elemento da Heap
 * @param heap ponteiro para heap válida
 * @param maior ponteiro para variável externa que guarde o valor removido da heap
 * @return retorna `true` caso operação tenha sido concluída com éxito.
 *
 * Guarda o valor a ser removido na variável `maior`.
 * Começando da posição inicial da heap, escolhe o maior filho para ocupar o
 * lugar do pai removido e repete o processo para a posição vaga deixada pelo
 * filho movido enquanto o filho for maior que o ultimo elemento, e caso não
 * seja, o ultimo elemento toma o lugar vago do pai.
 */
bool heap_remover_maximo(Heap *heap, int32_t *maior)
{
    if (heap_esta_vazia(heap)) {
        return false;
    }

    *maior = heap->dados[0];
    size_t restantes = --heap->tamanho;
    if (restantes == 0) {
        return true;
    }

    int32_t ultimo = heap->dados[restantes];
    size_t pai = 0;

    /* As capacidades do T1 (8000) e T2 (512) evitam overflow em 2*pai+1. */
    while (2u * pai + 1u < restantes) {
        size_t filho = 2u * pai + 1u;
        if (filho + 1u < restantes &&
            heap->dados[filho + 1u] > heap->dados[filho]) {
            ++filho;
        }
        if (ultimo >= heap->dados[filho]) {
            break;
        }
        heap->dados[pai] = heap->dados[filho];
        pai = filho;
    }

    heap->dados[pai] = ultimo;
    return true;
}

/* ===== src/heapsort/heapsort.c ===== */


/* Reserva estatica: nao ocupa a pilha e dispensa malloc. */
static Heap area_de_trabalho;

/** Heapsort
 * @param dados array de entrada
 * @param quantidade tamanho do array
 * @return Retorna `true` caso operação tenha sido concluída com éxito.
 * @note Altera o vetor de entrada, mas usa O(n) de memoria auxiliar;
 *       nao e a variante classica in-place de memoria auxiliar O(1).
 *
 * Utiliza uma Heap estática para inserir os elementos e depois removê-los de
 * forma ordenada para o array
 */
bool heapsort_ordenar(int32_t dados[], size_t quantidade)
{
    if (quantidade > HEAP_CAPACIDADE ||
        (quantidade != 0 && dados == NULL)) {
        return false;
    }

    heap_inicializar(&area_de_trabalho);

    for (size_t i = 0; i < quantidade; ++i) {
        if (!heap_inserir(&area_de_trabalho, dados[i])) {
            return false; /* impossivel com o limite ja verificado */
        }
    }

    for (size_t i = quantidade; i > 0; --i) {
        int32_t maior;
        if (!heap_remover_maximo(&area_de_trabalho, &maior)) {
            return false; /* impossivel apos quantidade insercoes */
        }
        dados[i - 1u] = maior;
    }
    return true;
}

/* ===== src/t2/app.c ===== */
/* Protocolo incremental: uma transacao por linha, sem guardar a linha inteira.
 * Entrada: n v0 ... v(n-1)\n, INFO\n ou TEST\n.
 * Saida de ordenacao igual ao T1; mensagens de controle com prefixo textual.
 * O cliente espera a resposta antes de enviar a proxima transacao.
 */



volatile Diagnostico diagnostico;
static int32_t vetor[HEAP_CAPACIDADE];
static struct {
    char token[12]; /* -2147483648 + terminador; nenhum buffer de linha. */
    size_t usados;
    size_t esperados;
    size_t recebidos;
    uint32_t ultimo_ms;
    const char *erro;
    char comando;
    bool tem_n;
    bool ativa;
    bool descartar;
} entrada;

static void texto(const char *s)
{
    while (*s != '\0') {
        plataforma_enviar(*s++);
    }
}

/* Conversao limitada, sem printf/strtol. Funciona inclusive para INT32_MIN. */
static void natural(uint32_t valor)
{
    char digitos[10];
    size_t n = 0;
    do {
        digitos[n++] = (char)('0' + valor % 10u);
        valor /= 10u;
    } while (valor != 0u);
    while (n != 0u) {
        plataforma_enviar(digitos[--n]);
    }
}

static void inteiro(int32_t valor)
{
    uint32_t modulo;
    if (valor < 0) {
        plataforma_enviar('-');
        modulo = (uint32_t)(-(valor + 1)) + 1u;
    } else {
        modulo = (uint32_t)valor;
    }
    natural(modulo);
}

static bool igual(const char *a, const char *b)
{
    while (*a != '\0' && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}

static bool converter(const char *s, int32_t *valor)
{
    bool negativo = *s == '-';
    if (*s == '-' || *s == '+') {
        ++s;
    }
    if (*s == '\0') {
        return false;
    }
    uint32_t acumulado = 0;
    uint32_t limite = negativo ? UINT32_C(2147483648) : UINT32_C(2147483647);
    while (*s != '\0') {
        if (*s < '0' || *s > '9') {
            return false;
        }
        uint32_t digito = (uint32_t)(*s++ - '0');
        if (acumulado > (limite - digito) / 10u) {
            return false; /* Verifica ANTES de multiplicar, evitando overflow. */
        }
        acumulado = acumulado * 10u + digito;
    }
    *valor = negativo && acumulado != 0u
        ? -(int32_t)(acumulado - 1u) - 1 : (int32_t)acumulado;
    return true;
}

static void reiniciar(void)
{
    entrada.usados = 0;
    entrada.esperados = 0;
    entrada.recebidos = 0;
    entrada.erro = NULL;
    entrada.comando = '\0';
    entrada.tem_n = false;
    entrada.ativa = false;
    entrada.descartar = false;
}

static void erro(const char *codigo)
{
    ++diagnostico.erros;
    texto("ERR ");
    texto(codigo);
    plataforma_enviar('\n');
}

static void campo(const char *nome, uint32_t valor)
{
    plataforma_enviar(' ');
    texto(nome);
    plataforma_enviar('=');
    natural(valor);
}

static void info(void)
{
    texto("INFO");
    campo("cap", HEAP_CAPACIDADE);
    campo("heap_bytes", (uint32_t)sizeof(Heap));
    campo("vetor_bytes", (uint32_t)sizeof(vetor));
    campo("ram_estatica", plataforma_ram_estatica());
    campo("pilha_ok", plataforma_pilha_ok() ? 1u : 0u);
    campo("ok", diagnostico.ordenacoes);
    campo("erros", diagnostico.erros);
    campo("rx_erros", diagnostico.rx_erros);
    campo("sort_us", diagnostico.ultimo_us);
    plataforma_enviar('\n');
}

/* Exercita os extremos e todas as posicoes no limite de capacidade.
 * O host tambem testa contra sorted() e contra o T1: este autoteste nao
 * substitui a referencia independente nem os testes via UART na placa.
 */
static bool autoteste(void)
{
    static const int32_t valores[] = {4, -2, 7, 4, 0, INT32_MIN, INT32_MAX};
    static const int32_t esperado[] = {INT32_MIN, -2, 0, 4, 4, 7, INT32_MAX};
    for (size_t i = 0; i < 7u; ++i) {
        vetor[i] = valores[i];
    }
    if (!heapsort_ordenar(vetor, 7u)) {
        return false;
    }
    for (size_t i = 0; i < 7u; ++i) {
        if (vetor[i] != esperado[i]) {
            return false;
        }
    }
    for (size_t i = 0; i < HEAP_CAPACIDADE; ++i) {
        vetor[i] = (int32_t)(HEAP_CAPACIDADE - i - 1u);
    }
    if (!heapsort_ordenar(vetor, HEAP_CAPACIDADE)) {
        return false;
    }
    for (size_t i = 0; i < HEAP_CAPACIDADE; ++i) {
        if (vetor[i] != (int32_t)i) {
            return false;
        }
    }
    return !heapsort_ordenar(vetor, HEAP_CAPACIDADE + 1u) &&
        heapsort_ordenar(NULL, 0) && !heapsort_ordenar(NULL, 1);
}

static void token_pronto(void)
{
    if (entrada.usados == 0u || entrada.erro != NULL) {
        entrada.usados = 0;
        return;
    }
    entrada.token[entrada.usados] = '\0';
    entrada.usados = 0;
    if (!entrada.tem_n && entrada.comando == '\0') {
        if (igual(entrada.token, "INFO") || igual(entrada.token, "TEST")) {
            entrada.comando = entrada.token[0];
            return;
        }
        int32_t n;
        if (!converter(entrada.token, &n) || n < 0 ||
            (uint32_t)n > HEAP_CAPACIDADE) {
            entrada.erro = "N";
            return;
        }
        entrada.esperados = (size_t)n;
        entrada.tem_n = true;
        return;
    }
    if (entrada.comando != '\0') {
        entrada.erro = "COMANDO";
    } else if (entrada.recebidos >= entrada.esperados) {
        entrada.erro = "QUANTIDADE";
    } else {
        int32_t valor;
        if (!converter(entrada.token, &valor)) {
            entrada.erro = "VALOR";
        } else {
            vetor[entrada.recebidos++] = valor;
        }
    }
}

static void finalizar(void)
{
    token_pronto();
    if (entrada.erro != NULL) {
        erro(entrada.erro);
    } else if (entrada.comando == 'I') {
        info();
    } else if (entrada.comando == 'T') {
        if (autoteste()) {
            texto("TEST OK\n");
        } else {
            erro("TEST");
        }
    } else if (entrada.tem_n) {
        if (entrada.recebidos != entrada.esperados) {
            erro("QUANTIDADE");
        } else {
            uint32_t inicio = plataforma_us();
            if (!heapsort_ordenar(vetor, entrada.esperados)) {
                erro("SORT");
            } else {
                diagnostico.ultimo_us = plataforma_us() - inicio;
                ++diagnostico.ordenacoes;
                for (size_t i = 0; i < entrada.esperados; ++i) {
                    if (i != 0u) {
                        plataforma_enviar(' ');
                    }
                    inteiro(vetor[i]);
                }
                plataforma_enviar('\n');
            }
        }
    }
    reiniciar();
}

void t2_receber(char byte, uint32_t agora_ms)
{
    if (entrada.descartar) {
        if (byte == '\n') {
            reiniciar();
        }
        return;
    }
    if (byte == '\n') {
        finalizar();
        return;
    }
    if (byte == ' ' || byte == '\t' || byte == '\r') {
        token_pronto();
        if (entrada.ativa) {
            entrada.ultimo_ms = agora_ms;
        }
        return;
    }
    entrada.ativa = true;
    entrada.ultimo_ms = agora_ms;
    if (entrada.erro == NULL) {
        if (entrada.usados + 1u >= sizeof(entrada.token)) {
            entrada.erro = entrada.tem_n ? "VALOR" : "N";
        } else if (byte == '\0') {
            entrada.erro = "VALOR";
        } else {
            entrada.token[entrada.usados++] = byte;
        }
    }
}

void t2_verificar_timeout(uint32_t agora_ms)
{
    if (entrada.ativa &&
        (uint32_t)(agora_ms - entrada.ultimo_ms) >= T2_TIMEOUT_MS) {
        reiniciar();
        entrada.descartar = true; /* So uma nova linha restabelece a fronteira. */
        erro("TIMEOUT");
    }
}

void t2_erro_rx(void)
{
    reiniciar();
    entrada.descartar = true;
    erro("RX"); /* Nunca ordenar silenciosamente uma mensagem corrompida. */
}

/* ===== src/t2/platform.c ===== */
/* Duas plataformas para o MESMO aplicativo. O arquivo gerado entrega/t2.c
 * seleciona o host com -DT2_HOST. Sem essa macro, inclui startup e vetores
 * do STM32F030R8, dispensando HAL, CubeMX, libc, semihosting e heap dinamica.
 */

#if defined(T2_HOST)
#include <stdio.h>
#include <time.h>

void plataforma_enviar(char byte)
{
    (void)putchar((unsigned char)byte);
    if (byte == '\n') {
        (void)fflush(stdout); /* Resposta disponivel tambem em pipes persistentes. */
    }
}

uint32_t plataforma_us(void)
{
    return (uint32_t)((double)clock() * 1000000.0 / (double)CLOCKS_PER_SEC);
}

uint32_t plataforma_ram_estatica(void)
{
    /* Modelo de memoria diferente do MCU; nao reportar sizeof(host) como MCU. */
    return 0;
}

bool plataforma_pilha_ok(void)
{
    return true;
}

int main(void)
{
    int byte;
    while ((byte = getchar()) != EOF) {
        t2_receber((char)byte, 0);
    }
    /* EOF encerra a ultima transacao no PC. Na UART exige-se '\n'. */
    t2_receber('\n', 0);
    return ferror(stdin) || ferror(stdout) ? 1 : 0;
}

#else

/* Mapa e bits conferidos em RM0360 e no header CMSIS stm32f030x8.h da ST.
 * Acesso de 32 bits e volatile: registradores podem mudar fora do fluxo C.
 */
#define REG32(endereco) (*(volatile uint32_t *)(uintptr_t)(endereco))
#define RCC_CR        REG32(0x40021000u)
#define RCC_CFGR      REG32(0x40021004u)
#define RCC_AHBENR    REG32(0x40021014u)
#define RCC_APB1ENR   REG32(0x4002101Cu)
#define GPIOA_MODER   REG32(0x48000000u)
#define GPIOA_OSPEEDR REG32(0x48000008u)
#define GPIOA_PUPDR   REG32(0x4800000Cu)
#define GPIOA_BSRR    REG32(0x48000018u)
#define GPIOA_AFRL    REG32(0x48000020u)
#define USART2_CR1    REG32(0x40004400u)
#define USART2_CR3    REG32(0x40004408u)
#define USART2_BRR    REG32(0x4000440Cu)
#define USART2_ISR    REG32(0x4000441Cu)
#define USART2_ICR    REG32(0x40004420u)
#define USART2_RDR    REG32(0x40004424u)
#define USART2_TDR    REG32(0x40004428u)
#define SYST_CSR      REG32(0xE000E010u)
#define SYST_RVR      REG32(0xE000E014u)
#define SYST_CVR      REG32(0xE000E018u)
#define SCB_ICSR      REG32(0xE000ED04u)
#define NVIC_ISER     REG32(0xE000E100u)
#define SCB_SHPR3     REG32(0xE000ED20u)

#define HSI_HZ 8000000u
#define RX_TAMANHO 256u
#define GUARDA UINT32_C(0xA5A5A5A5)
static volatile uint32_t milissegundos;
/* ISR produz, main consome. Indices alinhados e de 32 bits: acesso atomico
 * neste Cortex-M0. volatile nao e um mecanismo de exclusao mutua generico.
 */
static volatile char rx[RX_TAMANHO];
static volatile uint32_t rx_escrita;
static volatile uint32_t rx_leitura;
static volatile bool rx_corrompido;

/* Simbolos definidos pelo linker; nao sao variaveis C alocadas aqui. */
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern uint32_t _stack_limit, _stack_guard_end;

void Reset_Handler(void);
void SysTick_Handler(void);
void USART2_IRQHandler(void);
void HardFault_Handler(void);
void Default_Handler(void);
int main(void);

/* O primeiro vetor e o SP inicial; os demais sao enderecos Thumb.
 * USART2 e IRQ28 (indice 44), segundo a tabela oficial do STM32F030x8.
 */
typedef union {
    const void *pilha;
    void (*handler)(void);
} VetorInterrupcao;
__attribute__((section(".isr_vector"), used, aligned(256)))
const VetorInterrupcao vetores[48] = {
    [0] = {.pilha = &_estack},
    [1] = {.handler = Reset_Handler},
    [2] = {.handler = Default_Handler},
    [3] = {.handler = HardFault_Handler},
    [11] = {.handler = Default_Handler},
    [14] = {.handler = Default_Handler},
    [15] = {.handler = SysTick_Handler},
    [16] = {.handler = Default_Handler},
    [18] = {.handler = Default_Handler},
    [19] = {.handler = Default_Handler},
    [20] = {.handler = Default_Handler},
    [21] = {.handler = Default_Handler},
    [22] = {.handler = Default_Handler},
    [23] = {.handler = Default_Handler},
    [25] = {.handler = Default_Handler},
    [26] = {.handler = Default_Handler},
    [27] = {.handler = Default_Handler},
    [28] = {.handler = Default_Handler},
    [29] = {.handler = Default_Handler},
    [30] = {.handler = Default_Handler},
    [32] = {.handler = Default_Handler},
    [33] = {.handler = Default_Handler},
    [35] = {.handler = Default_Handler},
    [36] = {.handler = Default_Handler},
    [37] = {.handler = Default_Handler},
    [38] = {.handler = Default_Handler},
    [39] = {.handler = Default_Handler},
    [40] = {.handler = Default_Handler},
    [41] = {.handler = Default_Handler},
    [42] = {.handler = Default_Handler},
    [43] = {.handler = Default_Handler},
    [44] = {.handler = USART2_IRQHandler}
};

/* Startup proprio: inicializa .data/.bss antes de usar qualquer global.
 * -fno-builtin impede que os lacos sejam substituidos por memcpy/memset.
 * O hardware carrega o MSP do primeiro vetor ao sair de reset.
 */
void Reset_Handler(void)
{
    uint32_t *origem = &_sidata;
    for (uint32_t *p = &_sdata; p < &_edata; ++p) {
        *p = *origem++;
    }
    for (uint32_t *p = &_sbss; p < &_ebss; ++p) {
        *p = 0;
    }
    /* Guarda na base da reserva, distante dos frames ativos do startup. */
    for (uint32_t *p = &_stack_limit; p < &_stack_guard_end; ++p) {
        *p = GUARDA;
    }
    (void)main();
    for (;;) { }
}

static void falha_fatal(uint32_t codigo)
{
    __asm__ volatile ("cpsid i" ::: "memory");
    diagnostico.falha = codigo;
    RCC_AHBENR |= 1u << 17;
    GPIOA_MODER = (GPIOA_MODER & ~(3u << 10)) | (1u << 10);
    /* LD2 aceso fixo: falha fatal. Inspecionar diagnostico por SWD. */
    GPIOA_BSRR = 1u << 5;
    for (;;) { }
}

void HardFault_Handler(void)
{
    falha_fatal(1);
}

void Default_Handler(void)
{
    falha_fatal(2);
}

void SysTick_Handler(void)
{
    ++milissegundos;
}

void USART2_IRQHandler(void)
{
    uint32_t status = USART2_ISR;
    if ((status & 15u) != 0u) { /* PE, FE, NE ou ORE: descartar o frame. */
        USART2_ICR = status & 15u;
        rx_corrompido = true;
        ++diagnostico.rx_erros;
    }
    if ((status & (1u << 5)) != 0u) {
        char byte = (char)(USART2_RDR & 255u); /* Leitura limpa RXNE. */
        uint32_t proxima = (rx_escrita + 1u) % RX_TAMANHO;
        if (proxima == rx_leitura) {
            rx_corrompido = true;
            ++diagnostico.rx_erros;
        } else if (!rx_corrompido) {
            rx[rx_escrita] = byte;
            rx_escrita = proxima;
        }
    }
}

void plataforma_enviar(char byte)
{
    uint32_t inicio = milissegundos;
    while ((USART2_ISR & (1u << 7)) == 0u) {
        if ((uint32_t)(milissegundos - inicio) >= 100u) {
            falha_fatal(3); /* TX travado nao deixa o main preso sem diagnostico. */
        }
    }
    USART2_TDR = (uint32_t)(unsigned char)byte;
}

uint32_t plataforma_us(void)
{
    /* SysTick de 1 ms; amostra atomica com compensacao de tick pendente.
     * A secao critica dura poucos ciclos, menor que um periodo de 1 ms.
     * Soma unsigned suporta o wrap de microssegundos (~71 minutos).
     */
    __asm__ volatile ("cpsid i" ::: "memory");
    uint32_t ms = milissegundos;
    uint32_t ciclos = SYST_CVR;
    if ((SCB_ICSR & (1u << 26)) != 0u) {
        ++ms;
        ciclos = SYST_CVR;
    }
    __asm__ volatile ("cpsie i" ::: "memory");
    return ms * 1000u + (7999u - ciclos) / (HSI_HZ / 1000000u);
}

uint32_t plataforma_ram_estatica(void)
{
    return (uint32_t)((uintptr_t)&_ebss - (uintptr_t)&_sdata);
}

bool plataforma_pilha_ok(void)
{
    for (const volatile uint32_t *p = &_stack_limit; p < &_stack_guard_end; ++p) {
        if (*p != GUARDA) {
            return false;
        }
    }
    return true; /* Guarda intacta nao prova ausencia de qualquer overflow. */
}

static void inicializar_hardware(void)
{
    /* HSI interno 8 MHz: independente do MCO/cristal da revisao da Nucleo. */
    RCC_CR |= 1u;
    while ((RCC_CR & 2u) == 0u) { }
    RCC_CFGR &= ~3u;
    while ((RCC_CFGR & 12u) != 0u) { }
    RCC_CFGR &= ~((15u << 4) | (7u << 8)); /* AHB/APB sem divisao. */
    RCC_AHBENR |= 1u << 17; /* GPIOA */
    RCC_APB1ENR |= 1u << 17; /* USART2 */
    (void)RCC_APB1ENR; /* Sincroniza a habilitacao do clock antes do acesso. */
    /* PA2/PA3 AF1 USART2, PA5 saida LD2. Pull-up somente no RX. */
    GPIOA_MODER = (GPIOA_MODER & ~((15u << 4) | (3u << 10))) |
        (10u << 4) | (1u << 10);
    GPIOA_AFRL = (GPIOA_AFRL & ~(255u << 8)) | (17u << 8);
    GPIOA_PUPDR = (GPIOA_PUPDR & ~(3u << 6)) | (1u << 6);
    GPIOA_OSPEEDR |= 15u << 4;
    USART2_CR1 = 0;
    USART2_CR3 = 1u; /* EIE: interrupcoes de erro de recepcao. */
    USART2_BRR = (HSI_HZ + 115200u / 2u) / 115200u; /* BRR 69, ~115942 baud. */
    USART2_ICR = 15u;
    USART2_CR1 = 1u | (1u << 2) | (1u << 3) | (1u << 5);
    /* UE, RE, TE e RXNEIE; reset mantem 8 bits, sem paridade, 1 stop. */
    SYST_RVR = HSI_HZ / 1000u - 1u;
    SYST_CVR = 0;
    SCB_SHPR3 |= 3u << 30; /* SysTick abaixo da USART na prioridade. */
    SYST_CSR = 7u; /* ENABLE, TICKINT, CLKSOURCE=CPU */
    NVIC_ISER = 1u << 28;
    __asm__ volatile ("cpsie i" ::: "memory");
}

int main(void)
{
    inicializar_hardware();
    uint32_t led_ms = 0;
    bool led = false;
    for (;;) {
        /* Flush e flag devem mudar juntos; nunca imprimir dentro da ISR. */
        __asm__ volatile ("cpsid i" ::: "memory");
        bool corrompido = rx_corrompido;
        if (corrompido) {
            rx_leitura = rx_escrita;
            rx_corrompido = false;
        }
        __asm__ volatile ("cpsie i" ::: "memory");
        if (corrompido) {
            t2_erro_rx();
        }
        if (rx_leitura != rx_escrita) {
            char byte = rx[rx_leitura];
            rx_leitura = (rx_leitura + 1u) % RX_TAMANHO;
            t2_receber(byte, milissegundos);
        } else {
            /* Verifica timeout somente depois de consumir os bytes pendentes. */
            t2_verificar_timeout(milissegundos);
        }
        if (!plataforma_pilha_ok()) {
            falha_fatal(4);
        }
        if ((uint32_t)(milissegundos - led_ms) >= 500u) {
            led_ms = milissegundos;
            led = !led;
            GPIOA_BSRR = led ? (1u << 5) : (1u << 21);
        }
    }
}
#endif
