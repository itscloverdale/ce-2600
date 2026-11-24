RACK_DIR ?= $(HOME)/dev/Rack-SDK
SOURCES += $(wildcard src/*.cpp)
DISTRIBUTABLES += res README.md CHANGELOG.md LICENSE
include $(RACK_DIR)/plugin.mk

test:
	mkdir -p build
	clang++ -std=c++17 -O2 -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include tests/dsp.cpp -L$(RACK_DIR) -lRack -Wl,-rpath,$(RACK_DIR) -o build/dsp-test
	DYLD_LIBRARY_PATH="$(RACK_DIR)" ./build/dsp-test

# Also make the complete instrument available through File > Import selection.
install: install-selection
install-selection:
	mkdir -p "$(RACK_USER_DIR)/selections"
	cp "res/CE2600 Orange - Three panels.vcvs" "$(RACK_USER_DIR)/selections/CE2600 Orange - Three panels.vcvs"

.PHONY: install-selection
