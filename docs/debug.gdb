file build/t2.elf
target extended-remote localhost:3333
monitor reset halt
break HardFault_Handler
break Default_Handler
break falha_fatal
display diagnostico
continue
