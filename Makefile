
all:
	pio run

flash:
	pio run --target upload 

clean:
	pio run -t clean
	rm -rf .pio
	find . -name "*~" | xargs rm

ctags:
	ctags -R . /home/dave/.platformio/packages/framework-cmsis-stm32f1

TOOLCHAIN=${HOME}/.platformio/packages/toolchain-gccarmnoneeabi
BUILD_DIR=.pio/build/genericSTM32F103C8/

dump:
	${TOOLCHAIN}/bin/arm-none-eabi-objdump ${BUILD_DIR}/firmware.elf -d -S

openocd:
	openocd -f /usr/share/openocd/scripts/interface/stlink-v2-1.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

gdb:
	gdb-multiarch -x debug.gdb

#	FIN
