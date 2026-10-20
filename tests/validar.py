#!/usr/bin/env python3
"""Mesma suite no host e na UART. Relatorio registra o meio efetivamente usado.

PC:    python3 tests/validar.py --host build/t2_host --referencia src/t1/bin/heapsort
Placa: python3 tests/validar.py --porta /dev/ttyACM0 --referencia src/t1/bin/heapsort
"""
import argparse
from datetime import datetime, timezone
import json
import hashlib
from pathlib import Path
import random
import queue
import subprocess
import sys
import threading

CAP = 512


def casos_validos():
    rng = random.Random(20262)
    casos = [
        ("vazio", []), ("unitario", [42]),
        ("exemplo", [4, -2, 7, 4, 0, -8, 3]),
        ("extremos", [-2**31, 2**31-1, 0, -2**31, 2**31-1]),
        ("crescente_512", list(range(CAP))),
        ("decrescente_512", list(range(CAP, 0, -1))),
        ("iguais_512", [7] * CAP),
        ("duplicados_512", [i % 11 - 5 for i in range(CAP)]),
        ("extremos_512", [-2**31, 2**31-1] * (CAP // 2)),
    ]
    for n in [2, 3, 7, 31, 127, 255, 511, 512] * 4:
        casos.append((f"aleatorio_{len(casos):02d}_n{n}",
                      [rng.randint(-2**31, 2**31-1) for _ in range(n)]))
    return casos


INVALIDOS = [
    ("capacidade_excedida", "513\n", "ERR N"),
    ("tamanho_negativo", "-1\n", "ERR N"),
    ("tamanho_overflow", "4294967296\n", "ERR N"),
    ("tamanho_texto", "abc\n", "ERR N"),
    ("faltando", "3 1 2\n", "ERR QUANTIDADE"),
    ("sobrando", "1 2 3\n", "ERR QUANTIDADE"),
    ("zero_com_valor", "0 2\n", "ERR QUANTIDADE"),
    ("positivo_fora_int32", "1 2147483648\n", "ERR VALOR"),
    ("negativo_fora_int32", "1 -2147483649\n", "ERR VALOR"),
    ("token_longo", "1 " + "9" * 2048 + "\n", "ERR VALOR"),
    ("token_invalido", "1 2a\n", "ERR VALOR"),
    ("sinal_sem_numero", "1 -\n", "ERR VALOR"),
    ("comando_com_argumento", "INFO 1\n", "ERR COMANDO"),
    ("byte_nulo", "1 2\x00\n", "ERR VALOR"),
]


class Host:
    def __init__(self, executavel):
        self.processo = subprocess.Popen([str(executavel.resolve())],
                                         stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.linhas = queue.Queue()

        def ler():
            for linha in self.processo.stdout:
                self.linhas.put(linha)
            self.linhas.put(None)

        threading.Thread(target=ler, daemon=True).start()

    def enviar(self, mensagem):
        self.processo.stdin.write(mensagem.encode("ascii"))
        self.processo.stdin.flush()
        # Thread permite timeout em pipes tanto no POSIX quanto no Windows.
        try:
            resposta = self.linhas.get(timeout=5)
        except queue.Empty as exc:
            raise AssertionError("Timeout esperando resposta do host") from exc
        if resposta is None:
            raise AssertionError(f"Host encerrou com {self.processo.poll()}")
        return resposta.decode("ascii").rstrip("\r\n")

    def fechar(self):
        self.processo.stdin.close()
        try:
            self.processo.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.processo.kill()
            self.processo.wait()
        if self.processo.returncode != 0:
            raise AssertionError(f"Host encerrou com {self.processo.returncode}")


class Placa:
    def __init__(self, porta):
        try:
            import serial
        except ImportError as exc:
            raise SystemExit("Instale a dependencia: python3 -m pip install pyserial==3.5") from exc
        self.serial = serial.Serial(porta, 115200, timeout=8, write_timeout=8)
        self.serial.reset_input_buffer()
        self.serial.write(b"\n")  # Fronteira para recuperar eventual frame anterior.
        self.serial.flush()
        # Descarta residuos anteriores; sincroniza com INFO, sem resetar a placa.
        import time
        time.sleep(0.1)
        self.serial.reset_input_buffer()

    def enviar(self, mensagem):
        self.serial.write(mensagem.encode("ascii"))
        self.serial.flush()
        linha = self.serial.readline()
        if not linha.endswith(b"\n"):
            raise AssertionError(f"Resposta incompleta/timeout: {linha!r}")
        return linha.decode("ascii").rstrip("\r\n")

    def timeout_real(self):
        self.serial.write(b"2 7")
        self.serial.flush()
        linha = self.serial.readline()
        if linha != b"ERR TIMEOUT\n":
            raise AssertionError(f"Timeout real inesperado: {linha!r}")
        return self.enviar(" 1\n2 9 -3\n")

    def fechar(self):
        self.serial.close()


def executar(args):
    meio = "placa_uart" if args.porta else "host"
    cliente = Placa(args.porta) if args.porta else Host(args.host)
    relatorio = {"data_utc": datetime.now(timezone.utc).isoformat(), "meio": meio,
                 "porta": args.porta, "capacidade": CAP, "casos": []}
    fonte = Path(__file__).resolve().parent.parent / "entrega/t2.c"
    relatorio["fonte_sha256"] = hashlib.sha256(fonte.read_bytes()).hexdigest()
    try:
        resposta = cliente.enviar("INFO\n")
        if not resposta.startswith("INFO cap=512 "):
            raise AssertionError(f"Firmware/protocolo incorreto: {resposta!r}")
        relatorio["info_inicial"] = resposta

        for nome, valores in casos_validos():
            mensagem = f"{len(valores)} " + " ".join(map(str, valores)) + "\n"
            resposta = cliente.enviar(mensagem)
            esperado = " ".join(map(str, sorted(valores)))
            if resposta != esperado:
                raise AssertionError(f"{nome}: resultado difere de sorted()")
            referencia = False
            if args.referencia:
                proc = subprocess.run([str(args.referencia.resolve())], input=mensagem,
                                      text=True, capture_output=True, check=True, timeout=5)
                if resposta != proc.stdout.rstrip("\r\n"):
                    raise AssertionError(f"{nome}: resultado difere do T1")
                referencia = True
            relatorio["casos"].append({"nome": nome, "n": len(valores), "ok": True,
                                       "comparado_t1": referencia})
            print(f"OK {meio} {nome}")

        for nome, mensagem, esperado in INVALIDOS:
            resposta = cliente.enviar(mensagem)
            if resposta != esperado:
                raise AssertionError(f"{nome}: esperado {esperado!r}, recebido {resposta!r}")
            # Verifica recuperacao no MESMO processo/placa depois de cada erro.
            if cliente.enviar("3 9 -1 0\n") != "-1 0 9":
                raise AssertionError(f"{nome}: nao recuperou apos erro")
            relatorio["casos"].append({"nome": nome, "ok": True, "recuperou": True})
            print(f"OK {meio} {nome} + recuperacao")

        if cliente.enviar("2\t+4 -0\r\n") != "0 4":
            raise AssertionError("Espacos, sinais e CRLF")
        relatorio["casos"].append({"nome": "sinais_tabs_crlf", "ok": True})
        if cliente.enviar("TEST\n") != "TEST OK":
            raise AssertionError("Autoteste")
        relatorio["casos"].append({"nome": "autoteste", "ok": True})
        if args.porta:
            if cliente.timeout_real() != "-3 9":
                raise AssertionError("Recuperacao apos timeout real")
            relatorio["casos"].append({"nome": "timeout_real_5s", "ok": True})
        relatorio["info_final"] = cliente.enviar("INFO\n")
        relatorio["status"] = "PASSOU"
    except Exception as exc:
        relatorio["status"] = "FALHOU"
        relatorio["erro"] = str(exc)
        raise
    finally:
        try:
            cliente.fechar()
        finally:
            if args.json:
                args.json.parent.mkdir(parents=True, exist_ok=True)
                args.json.write_text(json.dumps(relatorio, indent=2, ensure_ascii=False) + "\n")
    print(f"{len(relatorio['casos'])} casos OK ({meio}); 41 comparacoes com sorted().")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    modo = parser.add_mutually_exclusive_group(required=True)
    modo.add_argument("--host", type=Path)
    modo.add_argument("--porta", help="/dev/ttyACM0, /dev/cu.usbmodem... ou COM3")
    parser.add_argument("--referencia", type=Path)
    parser.add_argument("--json", type=Path)
    try:
        executar(parser.parse_args())
    except (AssertionError, OSError, subprocess.SubprocessError) as exc:
        print(f"FALHOU: {exc}", file=sys.stderr)
        sys.exit(1)
