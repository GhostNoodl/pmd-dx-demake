# Item data

ITEM_DIR = $(DATA_ASM_SUBDIR)/item
ITEM_DATA = $(ITEM_DIR)/item_data.inc

# Headers included by tools/dungeonjson/dungeonjson.cpp:generate_item_data_text
ITEM_DATA_INC = \
	include/constants/item.h \
	include/constants/move_id.h

data_item: $(ITEM_DATA);

$(ITEM_DATA): $(ITEM_DIR)/item_data.json $(ITEM_DATA_INC)
	$(DUNGEONJSON) item pmd-red $<

# dx: Rescue Team DX item table (tools/dxtest/gen_dx_items.py), picked by data/system_sbin.s
DX_ITEM_DATA = $(ITEM_DIR)/dx/item_data.inc
data_item: $(DX_ITEM_DATA)
$(DX_ITEM_DATA): $(ITEM_DIR)/dx/item_data.json $(ITEM_DATA_INC)
	$(DUNGEONJSON) item pmd-red $<
