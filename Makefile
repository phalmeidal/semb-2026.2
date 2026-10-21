CC ?= cc
PYTHON ?= python3
ARM_PREFIX ?= arm-none-eabi-
RELATORIO ?= evidencias/host.json
ARM_CC = $(ARM_PREFIX)gcc
WARN = -Wall -Wextra -Wpedantic -Wconversion -Werror
CFLAGS ?= -std=c11 -O2 $(WARN)
ARM_FLAGS = -std=c11 -Os -g3 $(WARN) -mcpu=cortex-m0 -mthumb \
	-ffreestanding -fno-builtin -fno-common -ffunction-sections -fdata-sections \
	-fstack-usage -fno-unwind-tables -fno-asynchronous-unwind-tables
FONTES = src/include/heap.h src/include/heapsort.h src/t2/t2.h \
	src/heapsort/heap.c src/heapsort/heapsort.c src/t2/app.c src/t2/platform.c

.PHONY: all host test test-t1 firmware memory flash clean entrega sanitize slides test-limite
all: host
host: build/t2_host
build:
	mkdir -p build
entrega/t2.c: $(FONTES) tools/gerar_entrega.py LICENSE
	$(PYTHON) tools/gerar_entrega.py
entrega: entrega/t2.c
build/t2_host: entrega/t2.c | build
	$(CC) $(CFLAGS) -DT2_HOST $< -o $@
build/teste_heap: src/t1/tests/teste_heap.c $(FONTES) | build
	$(CC) $(CFLAGS) -DHEAP_CAPACIDADE=512u -Isrc/include \
		src/t1/tests/teste_heap.c src/heapsort/heap.c src/heapsort/heapsort.c -o $@
build/teste_protocolo: tests/teste_protocolo.c $(FONTES) | build
	$(CC) $(CFLAGS) -DHEAP_CAPACIDADE=512u -Isrc/include -Isrc/t2 \
		tests/teste_protocolo.c src/t2/app.c src/heapsort/heap.c \
		src/heapsort/heapsort.c -o $@
test-t1:
	$(MAKE) -C src/t1 test
test: host build/teste_heap build/teste_protocolo test-t1
	./build/teste_heap
	./build/teste_protocolo
	$(PYTHON) tests/validar.py --host build/t2_host --referencia src/t1/bin/heapsort --json $(RELATORIO)
	$(PYTHON) -m unittest discover -s tests -p 'test_demo.py' -v
sanitize:
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) -B test RELATORIO=evidencias/sanitizers.json CFLAGS='-std=c11 -O1 -g $(WARN) -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie'
build/t2.elf: entrega/t2.c src/t2/stm32f030r8.ld | build
	$(ARM_CC) $(ARM_FLAGS) -nostdlib -Wl,--gc-sections -Wl,-Map,build/t2.map \
		-T src/t2/stm32f030r8.ld entrega/t2.c -lgcc -o $@
build/t2.bin: build/t2.elf
	$(ARM_PREFIX)objcopy -O binary $< $@
build/t2.hex: build/t2.elf
	$(ARM_PREFIX)objcopy -O ihex $< $@
firmware: build/t2.bin build/t2.hex memory
memory: build/t2.elf
	$(PYTHON) tools/medir_memoria.py --prefix $(ARM_PREFIX) --elf $< --json evidencias/memoria.json
test-limite: entrega/t2.c
	$(PYTHON) tools/testar_limite_ram.py --prefix $(ARM_PREFIX)
flash: firmware
	openocd -f interface/stlink.cfg -f target/stm32f0x.cfg -c 'program build/t2.elf verify reset exit'
slides:
	$(PYTHON) tools/gerar_slides.py
clean:
	rm -rf build
	$(MAKE) -C src/t1 clean

