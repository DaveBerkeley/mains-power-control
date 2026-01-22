
#PROJECT=PANGLOS

PROJECT=MAINS_POWER_CONTROL

TARGET=seeed_xiao_esp32c3

MODE = run
#MODE = debug

.PHONY: all flash

NAME=$(shell git config user.name)
COPYRIGHT='" (C) ${NAME} "'
BANNER=src/banner.cpp

all: src/lut.h 
	echo "//    ${PROJECT} banner. Auto generated, do not edit" > ${BANNER}
	echo "const char *banner = {" >> ${BANNER}
	echo ${PROJECT} | sed 's/_/ /g' | figlet | sed 's/\\/\\\\/g' | awk '//{printf "    \"%s\\r\\n\"\n", $$0}' >> ${BANNER}
	echo '"\\r\\n"' >> ${BANNER}
	date +"\"built %Y/%m/%d %H:%M:%S git=$$(git rev-parse --short HEAD) \"" >> ${BANNER}
	echo "\"esp-idf=\" IDF_VER" >> ${BANNER}
	echo ${COPYRIGHT} >> ${BANNER}
	date +"\"%Y\r\n\"" >> ${BANNER}
	echo "};" >> ${BANNER}
	PLATFORMIO_BUILD_FLAGS="-DPROJECT=${PROJECT} -D${PROJECT}" pio $(MODE) -e $(TARGET) 

src/lut.h: sin2_power.py
	./sin2_power.py > src/lut.h

flash:
	PLATFORMIO_BUILD_FLAGS="-DPROJECT=${PROJECT} -D${PROJECT}" pio $(MODE) -e $(TARGET) --target upload --verbose

debug:
	PLATFORMIO_BUILD_FLAGS="-DPROJECT=${PROJECT} -D${PROJECT}" pio debug -e $(TARGET)

ctags:
	ctags -R . \
		/home/dave/.platformio/packages/framework-espidf/components/freertos/ \
		/home/dave/.platformio/packages/framework-espidf/components/hal/include \
		/home/dave/.platformio/packages/framework-espidf/components/driver \
		/home/dave/.platformio/packages/framework-espidf/components/esp_common \
		/home/dave/.platformio/packages/framework-espidf/components/esp_wifi \
		/home/dave/.platformio/packages/framework-espidf/components/mqtt

clean:
	rm -rf .pio managed_components
	find . -name "*~" | xargs rm -f
	scons -c 

cleanx:
	find .pio -name "*.o" | grep panglos | xargs rm

dump:
	#~/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-objdump .pio/build/$(TARGET)/firmware.elf -d -S
	#~/.platformio/packages/toolchain-xtensa-esp32s2/bin/xtensa-esp32s2-elf-objdump .pio/build/$(TARGET)/firmware.elf -d -S
	~/.platformio/packages/toolchain-riscv32-esp/bin/riscv32-esp-elf-objdump .pio/build/$(TARGET)/firmware.elf -d -S

tdd:
	scons

test: tdd
	./tdd

clang: export CC=clang
clang: export CXX=clang++
clang: $(APP)
	scons

gdb:
	/opt/espressif/bin/openocd -f board/esp32c3-builtin.cfg

graph:
	#scons && ./tdd --gtest_filter="MainsControl.Solar" | sed -n 's/xsim/0/gp' > /tmp/a.txt
	#gnuplot -e "plot '/tmp/a.txt' u 3 w l title 'meter', '' u 6 w l title 'burn', '' u 4 w l title 'net'" --persist
	scons && ./tdd --gtest_filter="MainsControl.Solar" | sed -n 's/ysim/0/gp' > /tmp/a.txt
	gnuplot -e "plot '/tmp/a.txt' u 2 w l title 'net', '' u 3 w p title 'error', '' u 4 w l title 'avail', '' u 5 w l title 'delta', '' u 6 w l title 'percent', '' u 7 w l title 'burn'" --persist

#	FIN
