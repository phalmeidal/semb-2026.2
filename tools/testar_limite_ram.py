#!/usr/bin/env python3
"""Experimento controlado: exigir 1024 itens deve ser recusado pelo linker."""
import argparse
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser()
p.add_argument("--prefix", default="arm-none-eabi-")
args = p.parse_args()
raiz = Path(__file__).resolve().parent.parent
fonte = (raiz / "entrega/t2.c").read_text()
assert fonte.count("#define HEAP_CAPACIDADE 512u") == 1
with tempfile.TemporaryDirectory() as pasta:
    src = Path(pasta) / "limite.c"
    src.write_text(fonte.replace("#define HEAP_CAPACIDADE 512u", "#define HEAP_CAPACIDADE 1024u"))
    processo = subprocess.run([
        args.prefix + "gcc", "-std=c11", "-Os", "-mcpu=cortex-m0", "-mthumb",
        "-ffreestanding", "-fno-builtin", "-nostdlib", "-ffunction-sections", "-fdata-sections",
        "-Wl,--gc-sections", "-T", str(raiz / "src/t2/stm32f030r8.ld"),
        str(src), "-lgcc", "-o", str(Path(pasta) / "limite.elf")], capture_output=True, text=True)
    if processo.returncode == 0 or "RAM insuficiente" not in processo.stderr:
        raise SystemExit("FALHOU: linker nao rejeitou o excesso de RAM como esperado\n" + processo.stderr)
print("OK: capacidade experimental de 1024 recusada: RAM insuficiente; reserva de pilha protegida.")
