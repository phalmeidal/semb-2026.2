/* Protocolo incremental: uma transacao por linha, sem guardar a linha inteira.
 * Entrada: n v0 ... v(n-1)\n, INFO\n ou TEST\n.
 * Saida de ordenacao igual ao T1; mensagens de controle com prefixo textual.
 * O cliente espera a resposta antes de enviar a proxima transacao.
 */
#include "heap.h"
#include "heapsort.h"
#include "t2.h"

volatile Diagnostico diagnostico;
static int32_t vetor[HEAP_CAPACIDADE];
static struct {
    char token[12]; /* -2147483648 + terminador; nenhum buffer de linha. */
    size_t usados;
    size_t esperados;
    size_t recebidos;
    size_t esquerda;
    unsigned fase_merge;
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
    entrada.esquerda = 0;
    entrada.fase_merge = 0;
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
    campo("versao", 2u);
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
    if (entrada.comando == 'M' && entrada.fase_merge < 2u) {
        int32_t n;
        if (!converter(entrada.token, &n) || n <= 0 ||
            (uint32_t)n > HEAP_CAPACIDADE / 2u) {
            entrada.erro = "N";
        } else if (entrada.fase_merge++ == 0u) {
            entrada.esquerda = (size_t)n;
        } else {
            entrada.esperados = entrada.esquerda + (size_t)n;
            entrada.tem_n = true;
        }
        return;
    }
    if (!entrada.tem_n && entrada.comando == '\0') {
        if (igual(entrada.token, "MERGE")) {
            entrada.comando = 'M';
            return;
        }
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
    if (entrada.comando != '\0' && entrada.comando != 'M') {
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

/* Intercala duas janelas ordenadas. Para assim que uma janela se esgota:
 * somente esse prefixo e seguro sem conhecer a proxima janela do PC.
 * Reutiliza vetor; nao aloca outro buffer nem usa comparacoes no cliente.
 */
static void intercalar(void)
{
    size_t corte = entrada.esquerda;
    size_t fim = entrada.esperados;
    for (size_t k = 1; k < fim; ++k) {
        if (k != corte && vetor[k] < vetor[k - 1u]) {
            erro("ORDEM");
            return;
        }
    }
    size_t i = 0, j = corte;
    while (i < corte && j < fim) {
        if (vetor[i] <= vetor[j]) { ++i; } else { ++j; }
    }
    texto("MERGE ");
    natural((uint32_t)i);
    plataforma_enviar(' ');
    natural((uint32_t)(j - corte));
    i = 0;
    j = corte;
    while (i < corte && j < fim) {
        plataforma_enviar(' ');
        if (vetor[i] <= vetor[j]) { inteiro(vetor[i++]); }
        else { inteiro(vetor[j++]); }
    }
    plataforma_enviar('\n');
}

static void finalizar(void)
{
    token_pronto();
    if (entrada.erro != NULL) {
        erro(entrada.erro);
    } else if (entrada.comando == 'M') {
        if (!entrada.tem_n || entrada.recebidos != entrada.esperados) {
            erro("QUANTIDADE");
        } else {
            intercalar();
        }
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

