BUILD_DIR ?= /tmp/test
CFLAGS += $(DEBUG_CFLAGS) -std=c99 -Wformat -Werror -O3 -D_GNU_SOURCE
SRC_DIR := $(shell pwd)
BUILD_PROGRAMS := $(patsubst %,$(BUILD_DIR)/%,$(PROGRAMS))
VALGRIND ?= valgrind -q --leak-check=full

.PHONY: all
all: $(BUILD_PROGRAMS)

define PROGRAM_template
$(BUILD_DIR)/$(1): $$($(1)_BUILD_OBJS)
ALL_OBJS += $$($(1)_BUILD_OBJS)
endef

define BUILD_OBJS_template
$(1)_BUILD_OBJS = $$(patsubst %,$$(BUILD_DIR)/%,$$($(1)_OBJS))
endef

$(BUILD_PROGRAMS):
	@echo $@
	$(CC) $^ -o $@
	$(if $(findstring $(notdir $@),$(VALGRIND_AUTORUN)),$(VALGRIND) $@)

-include $(wildcard $(BUILD_DIR)/*.d)

$(BUILD_DIR)/%.o: %.c
	@echo $@
	$(CC) -c $(CFLAGS) $*.c -o $@ #$*.o
	$(CC) -MM -MT $@ $(CFLAGS) $*.c > $(BUILD_DIR)/$*.d

clean:
	rm -f $(ALL_OBJS) $(ALL_OBJS:.o=.d) $(BUILD_PROGRAMS)

$(foreach prog,$(PROGRAMS),$(eval $(call BUILD_OBJS_template,$(prog))))
$(foreach prog,$(PROGRAMS),$(eval $(call PROGRAM_template,$(prog))))
