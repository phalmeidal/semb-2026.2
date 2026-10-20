#!/usr/bin/env python3
"""PDF 16:9 com resultados dos JSONs reais. Requer ReportLab; sem imagens externas."""
from pathlib import Path
import json
import os
import hashlib

from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor
from reportlab.lib.utils import simpleSplit
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont

RAIZ = Path(__file__).resolve().parent.parent
W, H = 1280, 720
BG, PANEL, INK, MUTED = map(HexColor, ["#101D2B", "#1B2C3D", "#F5F3EC", "#ABBCCB"])
TEAL, GOLD, RED = map(HexColor, ["#52D6B7", "#F1BD65", "#ED8C82"])
fontes = Path(os.environ.get("T2_FONT_DIR", "/usr/share/fonts/truetype/dejavu"))
if (fontes / "DejaVuSans.ttf").exists():
    pdfmetrics.registerFont(TTFont("Normal", str(fontes / "DejaVuSans.ttf")))
    pdfmetrics.registerFont(TTFont("Forte", str(fontes / "DejaVuSans-Bold.ttf")))
    NORMAL, FORTE = "Normal", "Forte"
else:
    NORMAL, FORTE = "Helvetica", "Helvetica-Bold"

mem = json.loads((RAIZ / "evidencias/memoria.json").read_text())
host = json.loads((RAIZ / "evidencias/host.json").read_text())
san = json.loads((RAIZ / "evidencias/sanitizers.json").read_text())
hash_fonte = hashlib.sha256((RAIZ / "entrega/t2.c").read_bytes()).hexdigest()
if any(r.get("fonte_sha256") != hash_fonte for r in (host, san, mem)):
    raise SystemExit("Evidencias desatualizadas: execute make test, make sanitize e make firmware.")
placa_path = RAIZ / "evidencias/placa.json"
placa = json.loads(placa_path.read_text()) if placa_path.exists() else {}
observacoes = (RAIZ / "evidencias/placa-observacoes.md").read_text()
fisico_ok = (placa.get("meio") == "placa_uart" and placa.get("status") == "PASSOU"
             and len(placa.get("casos", [])) == 58 and "PENDENTE" not in observacoes
             and placa.get("fonte_sha256") == hash_fonte)
saida = RAIZ / "docs/apresentacao.pdf"
saida.parent.mkdir(exist_ok=True)
c = canvas.Canvas(str(saida), pagesize=(W, H), pageCompression=1)
c.setTitle("T2 — Max-heap na NUCLEO-F030R8")
c.setAuthor("Anilton Magalhães de Castro; Pedro Henrique Almeida Lima")
c.setSubject("Porte do PC ao Cortex-M0; testes, memória e demonstração")


def num(n):
    return f"{n:,}".replace(",", ".")


def txt(s, x, y, size=22, color=INK, bold=False):
    c.setFillColor(color)
    c.setFont(FORTE if bold else NORMAL, size)
    c.drawString(x, y, str(s))


def par(s, x, y, width, size=22, color=INK, leading=None, bold=False):
    font = FORTE if bold else NORMAL
    leading = leading or size * 1.45
    for bloco in s.split("\n"):
        for linha in simpleSplit(bloco, font, size, width):
            txt(linha, x, y, size, color, bold)
            y -= leading
    return y


def panel(x, y, w, h, fill=PANEL):
    c.setFillColor(fill)
    c.roundRect(x, y, w, h, 16, fill=1, stroke=0)


def base(i, titulo, tempo):
    c.setFillColor(BG)
    c.rect(0, 0, W, H, fill=1, stroke=0)
    txt("SEMB 2026.2  /  T2", 64, 670, 15, TEAL, True)
    txt(tempo, 1060, 670, 15, MUTED)
    txt(titulo, 64, 599, 37, INK, True)
    c.setStrokeColor(PANEL)
    c.line(64, 60, W - 64, 60)
    txt("MAX-HEAP  /  NUCLEO-F030R8", 64, 33, 12, MUTED)
    txt(f"{i:02d} / 08", W - 130, 33, 12, MUTED)


def arrow(x1, y1, x2, y2, color=TEAL):
    c.setStrokeColor(color)
    c.setLineWidth(2)
    c.line(x1, y1, x2, y2)
    c.line(x2, y2, x2 - 8, y2 + 5)
    c.line(x2, y2, x2 - 8, y2 - 5)


def bullet(s, x, y, width, size=22):
    c.setFillColor(TEAL)
    c.circle(x + 3, y + 7, 3, fill=1, stroke=0)
    return par(s, x + 24, y, width - 24, size) - 15


base(1, "Do PC aos 8 KiB de RAM", "0:00–0:30")
par("Ordenação com max-heap\nna NUCLEO-F030R8", 64, 500, 690, 39, leading=58, bold=True)
par("Mesmo algoritmo e inteiros de 32 bits.\nCapacidade e E/S adaptadas ao hardware.", 64, 341, 680, 25, MUTED)
panel(850, 260, 365, 260)
txt("8000", 887, 449, 56, MUTED, True)
arrow(892, 412, 1164, 412)
txt("512", 887, 328, 66, TEAL, True)
txt("elementos por lote", 887, 285, 20, INK)
txt("Anilton Magalhães de Castro", 64, 180, 20)
txt("Pedro Henrique Almeida Lima", 64, 148, 20)
txt("Resultados físicos registrados" if fisico_ok else "Validação física: pendente nesta versão", 64, 95, 18,
    TEAL if fisico_ok else GOLD)
c.showPage()

base(2, "O algoritmo em 40 segundos", "0:30–1:10")
y = 516
for s in ["Árvore binária armazenada em vetor.", "O pai é maior ou igual aos filhos.",
          "Inserir: subir. Remover máximo: descer.", "Cada máximo ocupa o fim da saída."]:
    y = bullet(s, 64, y, 530)
panel(64, 126, 530, 110)
txt("Tempo: O(n log n)", 88, 191, 23, TEAL, True)
txt("Memória auxiliar: O(n), heap separada", 88, 151, 20)
coords = [(920, 475), (785, 373), (1055, 373), (714, 267), (855, 267), (984, 267), (1126, 267)]
for pai, filho in [(0, 1), (0, 2), (1, 3), (1, 4), (2, 5), (2, 6)]:
    c.setStrokeColor(MUTED)
    c.line(*coords[pai], *coords[filho])
for (x, y), valor in zip(coords, [7, 4, 4, -2, 0, -8, 3]):
    c.setFillColor(TEAL if valor == 7 else PANEL)
    c.circle(x, y, 34, fill=1, stroke=0)
    c.setFont(FORTE, 23)
    c.setFillColor(BG if valor == 7 else INK)
    c.drawCentredString(x, y - 8, str(valor))
txt("Saída:  -8  -2  0  3  4  4  7", 700, 152, 24, TEAL, True)
c.showPage()

base(3, "A restrição que mudou o projeto", "1:10–2:10")
panel(64, 424, 1152, 120)
txt("T1: 2 × 8000 × 4 = 64.000 bytes", 88, 490, 31, RED, True)
txt("A RAM inteira da F030R8 tem 8.192 bytes.", 88, 449, 22, MUTED)
txt("T2: orçamento real do ELF", 64, 378, 25, INK, True)
segmentos = [(mem["ram_estatica_bytes"], TEAL, "Dados estáticos"),
             (mem["pilha_reservada_bytes"], GOLD, "Reserva de pilha"),
             (mem["ram_folga_bytes"], MUTED, "Margem livre")]
x = 64
for tamanho, cor, _ in segmentos:
    largura = 1152 * tamanho / 8192
    c.setFillColor(cor)
    c.rect(x, 284, largura, 51, fill=1, stroke=0)
    x += largura
for x, (tamanho, cor, nome) in zip([64, 484, 904], segmentos):
    txt(num(tamanho) + " B", x, 242, 29, cor, True)
    txt(nome, x, 209, 19, MUTED)
par("Dois vetores de 512 int32_t: 4.100 B. Mantemos o intervalo numérico e a heap separada.",
    64, 143, 1152, 22)
txt("Reserva de pilha é orçamento; não representa uso máximo medido.", 64, 94, 17, GOLD)
c.showPage()

base(4, "Da linha serial ao resultado", "2:10–3:00")
nomes = [("PC", "gera e compara"), ("ST-LINK", "VCP / USART2"), ("ISR RX", "buffer de 255 B"),
         ("MAIN", "valida tokens"), ("MAX-HEAP", "ordena o lote"), ("UART TX", "devolve saída")]
for i, (nome, detalhe) in enumerate(nomes):
    x = 64 + i * 196
    panel(x, 380, 172, 130)
    txt(nome, x + 15, 456, 22, TEAL, True)
    par(detalhe, x + 15, 415, 142, 16, MUTED)
    if i < len(nomes) - 1:
        arrow(x + 174, 447, x + 191, 447)
y = 310
for s in ["Parser incremental: não guarda a linha inteira nem usa scanf.",
          "HSI interno: 8 MHz. USART2 PA2/PA3: 115200, 8N1.",
          "ISR curta; ordenação somente no main. Uma requisição por vez.",
          "Mesmo arquivo C compilado para host e Cortex-M0."]:
    y = bullet(s, 64, y, 1152, 23)
c.showPage()

base(5, "Testar o resultado e a recuperação", "3:00–4:20")
for x, nome, detalhe in [(64, "1  Gerar", "Semente fixa 20262.\n0, 1 e 512 itens; extremos,\nrepetidos e aleatórios."),
                         (458, "2  Comparar", "Todos os elementos\ncontra T1 e sorted().\nMesma suite: PC e UART."),
                         (852, "3  Recuperar", "Após cada erro, enviar\num lote válido no mesmo\nprocesso, sem reset.")]:
    panel(x, 325, 364, 216)
    txt(nome, x + 24, 492, 27, TEAL, True)
    par(detalhe, x + 24, 444, 316, 20)
y = 270
for s in ["Heap: propriedade pai/filhos, cheia, vazia e multiplicidade dos valores.",
          "Protocolo: quantidade, overflow int32, token longo, CRLF e timeout.",
          "PC: ASan/UBSan. MCU: suite UART, guarda, contadores e SWD."]:
    y = bullet(s, 64, y, 1152, 23)
c.showPage()

base(6, "Resultados e limites da evidência", "4:20–5:20")
linhas = [
    ("PC / T2", f"{len(host['casos'])} casos", host["status"]),
    ("ASan / UBSan", f"{len(san['casos'])} casos", san["status"]),
    ("Referências", "41 lotes", "T1 e Python iguais"),
    ("ELF Cortex-M0", f"Flash: {num(mem['flash_bytes'])} B", "Compilação concluída"),
    ("RAM estática", num(mem['ram_estatica_bytes']) + " B", "Extraída do ELF"),
    ("Placa / UART", "58 casos" if fisico_ok else "58 casos previstos", "PASSOU" if fisico_ok else "PENDENTE"),
]
for i, (origem, quantidade, status) in enumerate(linhas):
    y = 489 - i * 59
    if i % 2 == 0:
        panel(64, y - 19, 1152, 52)
    txt(origem, 85, y, 21, INK, True)
    txt(quantidade, 472, y, 21, MUTED)
    txt(status, 830, y, 21, GOLD if status == "PENDENTE" else TEAL)
par("Relatórios: evidencias/host.json, sanitizers.json e memoria.json.\n"
    + ("UART: placa.json e observações físicas disponíveis." if fisico_ok else
       "Ainda não há tempo medido, foto ou resultado UART da placa nesta versão."),
    64, 114, 1152, 17, MUTED, leading=25)
c.showPage()

base(7, "Dificuldades técnicas e soluções", "5:20–7:00")
pares = [
    ("RAM insuficiente", "512 itens; sem buffer de linha; mapa e limite no linker."),
    ("Bibliotecas de E/S", "UART e conversão decimal limitada, sem libc no MCU."),
    ("Tipos e overflow", "int32_t explícito; validar antes de acumular; INT32_MIN."),
    ("Concorrência main / ISR", "Ring buffer; seção crítica no flush; sort fora da ISR."),
    ("Entrada incompleta ou ruim", "Timeout de 5 s, ERR e descarte até uma nova linha."),
    ("Falhas sem um console", "INFO, TEST, guarda de pilha, LD2 e diagnóstico por SWD."),
]
for i, (problema, solucao) in enumerate(pares):
    y = 508 - 65 * i
    txt(problema, 64, y, 21, TEAL, True)
    par(solucao, 485, y, 730, 20, INK, leading=27)
txt("static evita grandes objetos na pilha; não reduz o consumo de RAM.", 64, 95, 20, GOLD)
c.showPage()

base(8, "O que aprendemos em Embarcados", "7:00–8:00")
for x, nome, detalhe in [(64, "Orçar", "Memória inclui dados,\nbuffers, biblioteca e pilha.\nMedir o ELF e reservar margem."),
                         (458, "Isolar", "Separar algoritmo e E/S.\nLimitar interrupções e deixar\no estado compartilhado claro."),
                         (852, "Comprovar", "Referência independente,\nrecuperação de falhas e\nexecução no hardware real.")]:
    panel(x, 300, 364, 235)
    txt(nome, x + 24, 479, 31, TEAL, True)
    par(detalhe, x + 24, 426, 316, 19, INK)
par("Requisito adaptado: 512 inteiros por lote.\nPreservados: valores int32_t, ordem crescente e max-heap separada.",
    64, 244, 1152, 23)
txt("Próximo registro: demonstração física e seus resultados." if not fisico_ok else
    "Evidência física registrada; revisar tempos e observações no roteiro.", 64, 147, 21, GOLD)
txt("Fontes: ST DS9773 / RM0360 / UM1724; MIT Heaps; Python sorted().", 64, 94, 16, MUTED)
c.save()
print(saida)
