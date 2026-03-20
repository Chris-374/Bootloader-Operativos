ASM := nasm
RM  := rm -f

.PHONY: all clean rebuild

all: image.bin

first_stage.bin: first_stage.asm
	$(ASM) -f bin first_stage.asm -o first_stage.bin

second_stage.bin: second_stage.asm
	$(ASM) -f bin second_stage.asm -o second_stage.bin

image.bin: first_stage.bin second_stage.bin
	cat first_stage.bin second_stage.bin > image.bin

clean:
	$(RM) first_stage.bin second_stage.bin image.bin

rebuild: clean all