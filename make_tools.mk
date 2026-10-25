
MAKEFLAGS += --no-print-directory

# dx: tools/dxtest (needs libmgba) and the dxtest.sh script are not build tools
TOOLDIRS := $(filter-out tools/agbcc tools/binutils tools/dxtest tools/dxtest.sh tools/spritecollab,$(wildcard tools/*))

.PHONY: all $(TOOLDIRS)

all: $(TOOLDIRS)

$(TOOLDIRS):
	@$(MAKE) -C $@
