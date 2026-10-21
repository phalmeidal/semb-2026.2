"""Regressoes da intercalacao entre janelas e do protocolo v2 real em C."""
from pathlib import Path
import random
import sys
import unittest

RAIZ = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(RAIZ / "tools"))
from demo_placa import Host, ordenar


class Demo(unittest.TestCase):
    def setUp(self):
        self.cliente = Host(RAIZ / "build/t2_host")

    def tearDown(self):
        self.cliente.fechar()

    def test_global(self):
        rng = random.Random(871)
        for valores in [[], [1], [4] * 8000, list(range(8000)),
                        list(range(8000, 0, -1)),
                        [-2**31, 2**31-1] * 4000,
                        [rng.randint(-2**31, 2**31-1) for _ in range(8001)],
                        [rng.randrange(-20, 20) for _ in range(16000)]]:
            with self.subTest(n=len(valores), prefixo=valores[:3]):
                resultado, _ = ordenar(self.cliente, valores)
                self.assertEqual(resultado, sorted(valores))

    def test_merge_erros_e_recuperacao(self):
        casos = [("MERGE", "ERR QUANTIDADE"),
                 ("MERGE 1", "ERR QUANTIDADE"),
                 ("MERGE 0 1 2", "ERR N"),
                 ("MERGE 257 1", "ERR N"),
                 ("MERGE 1 1 3", "ERR QUANTIDADE"),
                 ("MERGE 1 1 3 4 5", "ERR QUANTIDADE"),
                 ("MERGE 2 1 3 1 2", "ERR ORDEM"),
                 ("MERGE 1 2 1 3 2", "ERR ORDEM"),
                 ("MERGE 1 1 2147483648 0", "ERR VALOR")]
        for comando, esperado in casos:
            self.assertEqual(self.cliente.enviar(comando + "\n"), esperado)
            self.assertEqual(self.cliente.enviar("MERGE 2 2 1 4 2 3\n"),
                             "MERGE 1 2 1 2 3")


if __name__ == "__main__":
    unittest.main()
