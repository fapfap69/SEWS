CC = gcc
# Ottimizzazione per dimensione invece che per velocità
CFLAGS = -Wall -Wextra -Os -s -ffunction-sections -fdata-sections

# Determina il sistema operativo
UNAME_S := $(shell uname -s)

# Opzioni specifiche per il linker in base al sistema operativo
ifeq ($(UNAME_S),Linux)
    # Linux usa GNU ld che supporta --gc-sections
    LDFLAGS = -lpthread -lssl -lcrypto -Wl,--gc-sections
else ifeq ($(UNAME_S),Darwin)
    # macOS usa il linker di Apple che supporta -dead_strip
    LDFLAGS = -lpthread -lssl -lcrypto -Wl,-dead_strip
else
    # Default per altri sistemi
    LDFLAGS = -lpthread -lssl -lcrypto
endif

SRCDIR = src
OBJDIR = obj

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = swsws

.PHONY: all clean size

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Dimensione dell'eseguibile:"
	@ls -lh $(TARGET) | awk '{print $$5}'

# Target per visualizzare la dimensione dell'eseguibile
size: $(TARGET)
	size $(TARGET)
ifeq ($(UNAME_S),Linux)
	# Mostra le sezioni dell'eseguibile (solo Linux)
	readelf -S $(TARGET) | grep -E '\.text|\.data|\.bss|\.rodata'
else ifeq ($(UNAME_S),Darwin)
	# Mostra le sezioni dell'eseguibile (macOS)
	otool -l $(TARGET) | grep -E 'segname|sectname|size'
endif

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Target per comprimere l'eseguibile con UPX (se installato)
compress: $(TARGET)
	@if command -v upx >/dev/null 2>&1; then \
		echo "Compressione dell'eseguibile con UPX..."; \
		upx --best --lzma $(TARGET); \
		echo "Dimensione dopo la compressione:"; \
		ls -lh $(TARGET) | awk '{print $$5}'; \
	else \
		echo "UPX non trovato. Installalo con 'brew install upx' su macOS o 'sudo apt-get install upx' su Linux."; \
	fi

clean:
	rm -rf $(OBJDIR) $(TARGET)
