
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

