#nasm -f elf test.asm -o test.elf
#objcopy -j .text -O binary test.elf test.bin
#xc8 -V --chip=16f877a --output=inhx32 ../test.c
gpasm -a inhx16 test.asm
#sdcc --use-non-free  -p16f877a ../test.c
#--output=default,-inhx032
##16F877A
#objcopy -I ihex -O binary test.hex ../test.bin
#objcopy -j .text -O binary test.obj test.bin

