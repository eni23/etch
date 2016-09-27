MONITOR_BAUD		= 115200
SERIAL_DEVICE		= /dev/ttyUSB*

.PHONY: all inotify-watch-upload

all:
		platformio -f -c vim run

upload:
		platformio -f -c vim run --target upload --verbose

clean:
		platformio -f -c vim run --target clean --verbose

program:
		platformio -f -c vim run --target program --verbose

uploadfs:
		platformio -f -c vim run --target uploadfs --verbose

update: --verbose
		platformio -f -c vim update

install-pio-libs:
		platformio lib install 419

inotify-watch-upload:
	+@tools/inotify-upload.sh lib/TFT_ST7735/_usr/ src/

find-serial:
	$(eval USBTTY=$(shell ls -1 $(SERIAL_DEVICE) 2>/dev/null | head -n1 ))

monitor: find-serial
	screen $(USBTTY) $(MONITOR_BAUD)
