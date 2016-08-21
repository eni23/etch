MONITOR_BAUD		 = 115200
SERIAL_DEVICE		 = /dev/ttyUSB*
FAST_UPLOAD_BAUD = 3000000

all:
		platformio -f -c vim run

upload:
		platformio -f -c vim run --target upload

clean:
		platformio -f -c vim run --target clean

program:
		platformio -f -c vim run --target program

uploadfs:
		platformio -f -c vim run --target uploadfs

update:
		platformio -f -c vim update

install-pio-libs:
		platformio lib install 419

find-serial:
	$(eval USBTTY=$(shell ls -1 $(SERIAL_DEVICE) 2>/dev/null | head -n1 ))

monitor: find-serial
	screen $(USBTTY) $(MONITOR_BAUD)
