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
