/* Duas plataformas para o MESMO aplicativo. O arquivo gerado entrega/t2.c
 * seleciona o host com -DT2_HOST. Sem essa macro, inclui startup e vetores
 * do STM32F030R8, dispensando HAL, CubeMX, libc, semihosting e heap dinamica.
 */
#include "t2.h"

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
