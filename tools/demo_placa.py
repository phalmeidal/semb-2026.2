#!/usr/bin/env python3
"""Ordenacao global: MCU ordena blocos e intercala; PC armazena os runs.

python3 tools/demo_placa.py --porta /dev/ttyACM0 --n 8000
python3 tools/demo_placa.py --host build/t2_host --n 8000
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import random
import sys
import time

RAIZ = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(RAIZ / "tests"))
from validar import Host, Placa


def requisitar(cliente, mensagem):
    resposta = cliente.enviar(mensagem)
    if resposta.startswith("ERR"):
        raise RuntimeError(f"Firmware respondeu {resposta}: {mensagem[:60]!r}")
    return resposta


def ordenar(cliente, valores, progresso=None):
    """PC nao usa sort/merge para construir o resultado: apenas move dados."""
    runs = []
    transacoes = 0
    for inicio in range(0, len(valores), 512):
        bloco = valores[inicio:inicio + 512]
        linha = requisitar(cliente, f"{len(bloco)} " + " ".join(map(str, bloco)) + "\n")
        saida = list(map(int, linha.split()))
        if len(saida) != len(bloco):
            raise RuntimeError("Lote com tamanho incorreto")
        runs.append(saida)
        transacoes += 1
    if progresso:
        progresso(f"{len(runs)} blocos ordenados; iniciando intercalacao no MCU")
    while len(runs) > 1:
        novos = []
        for k in range(0, len(runs), 2):
            if k + 1 == len(runs):
                novos.append(runs[k])
                continue
            a, b = runs[k:k + 2]
            i = j = 0
            saida = []
            while i < len(a) and j < len(b):
                x, y = a[i:i + 256], b[j:j + 256]
                mensagem = f"MERGE {len(x)} {len(y)} " + " ".join(map(str, x + y)) + "\n"
                campos = requisitar(cliente, mensagem).split()
                if len(campos) < 4 or campos[0] != "MERGE":
                    raise RuntimeError(f"Resposta MERGE invalida: {campos[:6]}")
                ca, cb = map(int, campos[1:3])
                if not (0 <= ca <= len(x) and 0 <= cb <= len(y)):
                    raise RuntimeError("Contadores MERGE fora dos limites")
                if ca != len(x) and cb != len(y):
                    raise RuntimeError("MERGE sem progresso ate o fim de uma janela")
                prefixo = list(map(int, campos[3:]))
                if len(prefixo) != ca + cb:
                    raise RuntimeError("Prefixo MERGE com tamanho incorreto")
                saida.extend(prefixo)
                i += ca
                j += cb
                transacoes += 1
            # Run esgotado: cauda ja ordenada, nenhuma comparacao necessaria.
            saida.extend(a[i:])
            saida.extend(b[j:])
            novos.append(saida)
        runs = novos
        if progresso:
            progresso(f"Intercalacao: {len(runs)} runs restantes")
    return (runs[0] if runs else []), transacoes


def portas():
    try:
        from serial.tools import list_ports
    except ImportError as exc:
        raise RuntimeError("Instale: python3 -m pip install -r tests/requirements-serial.txt") from exc
    return list(list_ports.comports())


def executar(args):
    if args.listar_portas:
        for p in portas():
            print(f"{p.device}: {p.description}")
        return
    if args.n < 8000:
        raise RuntimeError("A demonstracao exige pelo menos 8000 elementos")
    if not args.host and not args.porta:
        candidatos = [p.device for p in portas() if p.vid == 0x0483]
        if len(candidatos) != 1:
            raise RuntimeError("Informe --porta; use --listar-portas para identificar a placa")
        args.porta = candidatos[0]
    relatorio = {"data_utc": datetime.now(timezone.utc).isoformat(),
                 "meio": "host" if args.host else "placa_uart", "porta": args.porta,
                 "n": args.n, "seed": args.seed, "status": "FALHOU",
                 "armazenamento": "RAM do PC", "ordenacao_e_intercalacao": "MCU ou backend host explicitado"}
    cliente = None
    try:
        cliente = Host(args.host) if args.host else Placa(args.porta)
        info = requisitar(cliente, "INFO\n")
        if not info.startswith("INFO cap=512 versao=2 "):
            raise RuntimeError(f"Firmware incompatível: {info!r}. Grave make flash deste fork; nao use ClientUART binario")
        relatorio["info_inicial"] = info
        if requisitar(cliente, "TEST\n") != "TEST OK":
            raise RuntimeError("Autoteste falhou")
        rng = random.Random(args.seed)
        valores = [rng.randint(-2**31, 2**31-1) for _ in range(args.n)]
        valores[:5] = [-2**31, 2**31-1, 0, -2**31, 2**31-1]
        inicio = time.monotonic()
        resultado, transacoes = ordenar(cliente, valores, print)
        # sorted e apenas o oraculo independente, depois de receber o resultado.
        if resultado != sorted(valores):
            raise RuntimeError("Resultado global difere da referencia Python")
        relatorio.update(status="PASSOU", transacoes=transacoes,
                         segundos=time.monotonic() - inicio,
                         resultado_sha256=hashlib.sha256(json.dumps(resultado).encode()).hexdigest(),
                         info_final=requisitar(cliente, "INFO\n"))
        print(f"PASSOU: {args.n} elementos globalmente ordenados ({relatorio['meio']}); {transacoes} transacoes")
    except Exception as exc:
        relatorio["erro"] = str(exc)
        raise
    finally:
        try:
            if cliente is not None:
                cliente.fechar()
        except Exception as exc:
            relatorio["status"] = "FALHOU"
            relatorio["erro"] = str(exc)
            raise
        finally:
            args.json.parent.mkdir(parents=True, exist_ok=True)
            args.json.write_text(json.dumps(relatorio, ensure_ascii=False, indent=2) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    modo = parser.add_mutually_exclusive_group()
    modo.add_argument("--porta")
    modo.add_argument("--host", type=Path)
    parser.add_argument("--listar-portas", action="store_true")
    parser.add_argument("--n", type=int, default=8000)
    parser.add_argument("--seed", type=int, default=20262)
    parser.add_argument("--json", type=Path, default=Path("evidencias/demo-atual.json"))
    try:
        executar(parser.parse_args())
    except Exception as exc:
        print(f"FALHOU: {exc}\nConfira gravacao, porta, cabo de dados e feche o monitor serial do IDE.", file=sys.stderr)
        sys.exit(1)
