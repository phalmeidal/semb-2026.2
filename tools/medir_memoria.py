#!/usr/bin/env python3
"""Verifica o ELF real e registra dados estaticos, reserva de pilha e Flash."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import struct
import tempfile

p = argparse.ArgumentParser()
p.add_argument("--prefix", default="arm-none-eabi-")
p.add_argument("--elf", type=Path, required=True)
p.add_argument("--json", type=Path)
args = p.parse_args()
texto = subprocess.check_output([args.prefix + "size", "-A", str(args.elf)], text=True)
secoes = {}
for linha in texto.splitlines():
    campos = linha.split()
    if len(campos) >= 3 and campos[0].startswith("."):
        secoes[campos[0]] = int(campos[1])
ram = secoes.get(".data", 0) + secoes.get(".bss", 0)
pilha = secoes.get(".stack", 0)
flash = sum(secoes.get(s, 0) for s in (".isr_vector", ".text", ".data"))
if ram + pilha > 8192 or flash > 65536 or pilha != 1536:
    raise SystemExit("ERRO: orçamento de memoria ou reserva de pilha invalido")
# Simbolos nao resolvidos ou uso acidental de malloc/printf devem falhar o build.
indefinidos = subprocess.check_output([args.prefix + "nm", "-u", str(args.elf)], text=True)
if indefinidos.strip():
    raise SystemExit("ERRO: simbolos nao resolvidos: " + indefinidos)
simbolos = {}
for linha in subprocess.check_output([args.prefix + "nm", str(args.elf)], text=True).splitlines():
    partes = linha.split()
    if len(partes) == 3:
        simbolos[partes[2]] = int(partes[0], 16)
with tempfile.TemporaryDirectory() as pasta:
    arquivo = Path(pasta) / "vetores.bin"
    subprocess.run([args.prefix + "objcopy", "-O", "binary", "--only-section=.isr_vector",
                    str(args.elf), str(arquivo)], check=True)
    dados = arquivo.read_bytes()
if len(dados) != 48 * 4:
    raise SystemExit("ERRO: tabela de vetores fora do tamanho esperado")
vetores = struct.unpack("<48I", dados)
if (vetores[0] != 0x20002000 or
        vetores[1] != (simbolos["Reset_Handler"] | 1) or
        vetores[15] != (simbolos["SysTick_Handler"] | 1) or
        vetores[44] != (simbolos["USART2_IRQHandler"] | 1)):
    raise SystemExit("ERRO: SP inicial, reset ou vetores de IRQ incorretos")
relatorio = {"elf": str(args.elf), "flash_bytes": flash, "flash_limite": 65536,
             "ram_estatica_bytes": ram, "pilha_reservada_bytes": pilha,
             "ram_folga_bytes": 8192 - ram - pilha, "ram_limite": 8192,
             "vetores_reset_systick_usart2": "OK", "simbolos_indefinidos": 0,
             "validacao": "ELF compilado; nao e medicao de execucao na placa"}
fonte = Path(__file__).resolve().parent.parent / "entrega/t2.c"
relatorio["fonte_sha256"] = hashlib.sha256(fonte.read_bytes()).hexdigest()
relatorio["elf_sha256"] = hashlib.sha256(args.elf.read_bytes()).hexdigest()
relatorio["compilador"] = subprocess.check_output([args.prefix + "gcc", "--version"], text=True).splitlines()[0]
print(json.dumps(relatorio, indent=2, ensure_ascii=False))
if args.json:
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(relatorio, indent=2, ensure_ascii=False) + "\n")
