# Move data

MOVE_DIR = $(DATA_ASM_SUBDIR)/move
MOVE_DATA = $(MOVE_DIR)/move_data.inc

# Headers included by tools/dungeonjson/dungeonjson.cpp:generate_move_data_text
MOVE_DATA_INC = \
	include/constants/move_range.h \
	include/constants/type.h

data_move: $(MOVE_DATA);

$(MOVE_DATA): $(MOVE_DIR)/move_data.json $(MOVE_DATA_INC)
	$(DUNGEONJSON) move pmd-red $<

# dx: Rescue Team DX Fairy moves (tools/dxtest/gen_dx_moves.py), picked by data/system_sbin.s
DX_MOVE_DATA = $(MOVE_DIR)/dx/move_data.inc
data_move: $(DX_MOVE_DATA)
$(DX_MOVE_DATA): $(MOVE_DIR)/dx/move_data.json $(MOVE_DATA_INC)
	$(DUNGEONJSON) move pmd-red $<
