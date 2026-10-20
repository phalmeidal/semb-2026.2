/* Injecao de tempo e falhas sem hardware. Verifica recuperacao e wrap do
 * relogio; a validacao da UART e do clock reais deve ser feita na placa. */
#include "t2.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static char saida[256];
static size_t usados;

void plataforma_enviar(char byte)
{
    assert(usados + 1u < sizeof(saida));
    saida[usados++] = byte;
    saida[usados] = '\0';
}

uint32_t plataforma_us(void) { return 0; }
uint32_t plataforma_ram_estatica(void) { return 0; }
bool plataforma_pilha_ok(void) { return true; }

static void enviar(const char *s, uint32_t ms)
{
    for (; *s != '\0'; ++s) {
        t2_receber(*s, ms);
    }
}

static void esperar(const char *s)
{
    assert(strcmp(saida, s) == 0);
    usados = 0;
    saida[0] = '\0';
}

int main(void)
{
    enviar("3 9", 100);
    t2_verificar_timeout(5099);
    esperar("");
    t2_verificar_timeout(5100);
    esperar("ERR TIMEOUT\n");
    /* A cauda da mensagem interrompida nao vira uma transacao nova. */
    enviar(" 1 2\n3 3 2 1\n", 5101);
    esperar("1 2 3\n");

    enviar("2 8", UINT32_MAX - 100u);
    t2_verificar_timeout(4898u);
    esperar("");
    t2_verificar_timeout(4899u);
    esperar("ERR TIMEOUT\n");
    enviar("\n", 4900);

    enviar("2 7", 5000);
    t2_erro_rx();
    esperar("ERR RX\n");
    enviar(" 1\n2 -7 9\n", 5001);
    esperar("-7 9\n");

    enviar("\r\n \t\r\n", 6000);
    esperar("");
    enviar("0\r\n", 6000);
    esperar("\n");
    enviar("1 2", 6000);
    t2_receber('\0', 6000);
    enviar("\n1 5\n", 6000);
    esperar("ERR VALOR\n5\n");
    assert(diagnostico.erros == 4u);
    puts("Protocolo: timeout, wrap uint32_t, erro RX e recuperacao OK");
    return 0;
}
