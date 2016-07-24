# this is may the ugliest makefile you've ever seen

PROJECT_DIR			= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
FQBN 						= esp8266:esp8266:nodemcuv2:CpuFrequency=80,UploadSpeed=921600,FlashSize=4M3M
SRCDIR					= $(PROJECT_DIR)src
OBJDIR					= $(PROJECT_DIR)build
MAINFILE				= main.ino
SERIAL_DEVICE		= /dev/ttyUSB*
UPLOAD_BAUD			= 921600
MONITOR_BAUD		= 115200



# non-config
SKETCH_NAME = $(basename $(MAINFILE))
SRCTMP = $(join $(OBJDIR)/sketch/,$(SKETCH_NAME))
MAIN = $(join $(SRCTMP)/,$(MAINFILE))

ARDUINO_BUILDER = $(ARDUINO_DIR)/arduino-builder
ABFLAGS  = -hardware "$(ARDUINO_DIR)/hardware" -hardware "$(ARDUINO_PACKAGES_DIR)/packages"
ABFLAGS += -tools "$(ARDUINO_DIR)/tools-builder" -tools "$(ARDUINO_DIR)/hardware/tools/avr" -tools "$(ARDUINO_PACKAGES_DIR)/packages"
ABFLAGS += -built-in-libraries "$(ARDUINO_DIR)/libraries" -libraries "$(ARDUINO_LIBRARY_DIR)" -fqbn=$(FQBN)
ABFLAGS += -build-path "$(OBJDIR)/hex"
ABFLAGS += -prefs=build.warn_data_percentage=99 -warnings=all -verbose

.PHONY: build find-serial update-temp upload find-monitor-baud

default: build

build: update-temp
	$(ARDUINO_BUILDER) -compile $(ABFLAGS) $(PROJECT_DIR)build/sketch/$(SKETCH_NAME)/$(SKETCH_NAME).ino

find-serial:
	$(eval USBTTY=$(shell ls -1 $(SERIAL_DEVICE) 2>/dev/null | head -n1 ))
	# Serial device: $(USBTTY)

monitor: find-serial
	screen $(USBTTY) $(MONITOR_BAUD)

upload: find-serial
	$(ARDUINO_PACKAGES_DIR)/packages/esp8266/tools/esptool/0.4.8/esptool -vv -cd nodemcu -cb $(UPLOAD_BAUD) -cp $(USBTTY) -ca 0x00000 -cf $(OBJDIR)/hex/$(SKETCH_NAME).ino.bin

all: build upload

clean:
	-rm -r $(OBJDIR)/*
	-touch $(OBJDIR)/.gitkeep

update-temp:
	mkdir -p $(OBJDIR)/sketch/$(SKETCH_NAME)
	mkdir -p $(OBJDIR)/hex
	cp -r $(SRCDIR)/* $(OBJDIR)/sketch/$(SKETCH_NAME)
