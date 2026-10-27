# Сборка на Linux/macOS (mingw-w64 + msitools) или в CI — см. README.md.
#   make test   — тесты engine.c обычным cc
#   make        — build/UzbekHarflari.exe
#   make msi    — build/UzbekHarflari-<версия>-x64.msi

VERSION := $(shell sed -n 's/^\#define APP_VERSION "\(.*\)"/\1/p' src/version.h)
BUILD := build
ICONS := ../uzb_ext/icons

CC ?= cc
WIN_CC ?= x86_64-w64-mingw32-gcc
WINDRES ?= x86_64-w64-mingw32-windres
PYTHON ?= python3
WIXL ?= wixl

CFLAGS := -std=c11 -O2 -Wall -Wextra
WIN_CFLAGS := $(CFLAGS) -municode -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00
WIN_LDFLAGS := -municode -mwindows -static -s
WIN_LIBS := -luser32 -lshell32 -ladvapi32 -lole32 -loleaut32 -loleacc -luuid

EXE := $(BUILD)/UzbekHarflari.exe
MSI := $(BUILD)/UzbekHarflari-$(VERSION)-x64.msi

.PHONY: all exe msi test clean version

all: exe
exe: $(EXE)
msi: $(MSI)

version:
	@echo $(VERSION)

$(BUILD):
	mkdir -p $@

# Заодно создаёт app-off.ico.
$(BUILD)/app.ico: tools/make_icons.py $(wildcard $(ICONS)/icon-*.png) | $(BUILD)
	$(PYTHON) tools/make_icons.py $(ICONS) $(BUILD)

$(BUILD)/app.res.o: src/app.rc src/app.manifest src/resource.h src/version.h $(BUILD)/app.ico
	$(WINDRES) -I src -I $(BUILD) -O coff -o $@ src/app.rc

$(EXE): src/main.c src/engine.c src/engine.h src/resource.h src/version.h $(BUILD)/app.res.o
	$(WIN_CC) $(WIN_CFLAGS) -o $@ src/main.c src/engine.c $(BUILD)/app.res.o $(WIN_LDFLAGS) $(WIN_LIBS)

$(MSI): installer/UzbekHarflari.wxs $(EXE) $(BUILD)/app.ico
	$(WIXL) -a x64 -D Version=$(VERSION) -D BuildDir=$(BUILD) -o $@ installer/UzbekHarflari.wxs

$(BUILD)/test_engine: tests/test_engine.c src/engine.c src/engine.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ tests/test_engine.c src/engine.c

test: $(BUILD)/test_engine
	./$(BUILD)/test_engine

clean:
	rm -rf $(BUILD)
