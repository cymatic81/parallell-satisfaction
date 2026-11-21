# =============================================================================
# PARALLEL SATISFACTION SUBSYSTEM: VERIFICATION & GATE-COUNT PIPELINE
# Root-level Makefile supporting make, verify, verify-count, clean, and remake.
# =============================================================================

CC            ?= clang
RM            ?= rm -f
MKDIR         ?= mkdir -p

# Search paths across root, Support_Functions, and Verify_Pipeline
STD_CFLAGS    := -std=c11 -Wall -Wextra -march=native -I. -I"Support_Functions" -I"Verify_Pipeline"
CFLAGS        ?= -O2 $(STD_CFLAGS) $(EXTRA_CFLAGS)

BUILD_DIR     ?= build
TARGET_NAME   ?= ps_verify
TARGET        := $(BUILD_DIR)/$(TARGET_NAME)

# Header dependencies
MASTER_HDR    := parallel_satisfaction.h
SUPPORT_HDR   := Support_Functions/ps_functions.h
VERIFY_HDR    := Verify_Pipeline/verify_pipetest.h

# Source mappings
SUPPORT_SRC   := Support_Functions/ps_functions.c
CHIPSET_SRC   := chipset_functions.c
VERIFY_SRC    := Verify_Pipeline/verify_pipeline.c

# Object mappings inside build/
OBJS          := $(BUILD_DIR)/ps_functions.o \
                 $(BUILD_DIR)/chipset_functions.o \
                 $(BUILD_DIR)/verify_pipeline.o

.PHONY: all verify verify-count clean remake

all: $(TARGET)

# Link verification binary
$(TARGET): $(OBJS)
	@$(MKDIR) $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)
	@echo "Verification binary built successfully: $(TARGET)"

# Compile Support_Functions/ps_functions.c
$(BUILD_DIR)/ps_functions.o: $(SUPPORT_SRC) $(SUPPORT_HDR) $(MASTER_HDR)
	@$(MKDIR) $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile chipset_functions.c
$(BUILD_DIR)/chipset_functions.o: $(CHIPSET_SRC) $(SUPPORT_HDR) $(MASTER_HDR)
	@$(MKDIR) $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Verify_Pipeline/verify_pipeline.c
$(BUILD_DIR)/verify_pipeline.o: $(VERIFY_SRC) $(VERIFY_HDR) $(SUPPORT_HDR) $(MASTER_HDR)
	@$(MKDIR) $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Standard verification build (zero gate-counting overhead)
verify: all
	@./$(TARGET)

# Gate-count / theoretical cycle telemetry build (-DPS_GATE_COUNT)
verify-count: CFLAGS += -DPS_GATE_COUNT
verify-count: remake
	@./$(TARGET)

clean:
	$(RM) $(OBJS) $(TARGET)
	@if [ -d "$(BUILD_DIR)" ]; then rmdir "$(BUILD_DIR)" 2>/dev/null || true; fi

remake: clean all
