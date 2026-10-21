#!/usr/bin/env python3
"""Entrada de compatibilidade para quem executava o antigo run.py binario."""
from pathlib import Path
import runpy

runpy.run_path(str(Path(__file__).resolve().parents[3] / "tools/demo_placa.py"), run_name="__main__")
