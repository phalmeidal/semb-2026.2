#!/usr/bin/env python3
"""Gera a unidade C submetida e compilada nos dois alvos. Sem dependencias."""
from pathlib import Path
import re

RAIZ = Path(__file__).resolve().parent.parent
FONTES = (
    "src/include/heap.h", "src/include/heapsort.h", "src/t2/t2.h",
    "src/heapsort/heap.c", "src/heapsort/heapsort.c",
    "src/t2/app.c", "src/t2/platform.c",
)


def gerar():
    cabecalho = """/* T2 de Sistemas Embarcados -- NUCLEO-F030R8 (STM32F030R8T6).
 * Autores do projeto: Anilton Magalhaes de Castro e Pedro Henrique Almeida Lima.
 * Licenca MIT; veja LICENSE no repositorio de origem.
 * GERADO por tools/gerar_entrega.py. Edite os fontes modulares, nao esta copia.
 * Este e o UNICO arquivo de codigo a submeter na entrega do T2.
 * Host: cc -std=c11 -O2 -DT2_HOST entrega/t2.c -o build/t2_host
 * MCU: make firmware (usa src/t2/stm32f030r8.ld, startup ja incluido aqui).
 * UART: 115200 8N1, uma linha por lote, 512 int32_t; INFO/TEST/MERGE (v2).
 * 8000+ globais: tools/demo_placa.py usa PC como armazenamento de runs.
 * Nao juntar este startup a outro startup/main gerado pelo CubeIDE.
 */
#define HEAP_CAPACIDADE 512u
"""
    licenca = (RAIZ / "LICENSE").read_text()
    partes = [cabecalho, "\n/*\n" + "\n".join(" * " + linha for linha in licenca.splitlines()) + "\n */\n"]
    for fonte in FONTES:
        conteudo = (RAIZ / fonte).read_text()
        conteudo = re.sub(r'^#include "(?:heap|heapsort|t2)\.h"\s*$', '',
                          conteudo, flags=re.MULTILINE)
        partes.append(f"\n/* ===== {fonte} ===== */\n{conteudo}")
    destino = RAIZ / "entrega/t2.c"
    destino.parent.mkdir(exist_ok=True)
    destino.write_text("\n".join(linha.rstrip() for linha in "".join(partes).splitlines()) + "\n")


if __name__ == "__main__":
    gerar()

