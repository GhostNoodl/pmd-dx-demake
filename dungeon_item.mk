
DUNGEONS_DIR = $(DATA_ASM_SUBDIR)/dungeon

DUNGEON_ITEM = $(DUNGEONS_DIR)/dungeon_item_data.inc

dungeon_item: $(DUNGEON_ITEM);

$(DUNGEON_ITEM): $(DUNGEONS_DIR)/dungeon_item_data.json
	$(DUNGEONJSON) dungeon_item pmd-red $<

# dx: Rescue Team DX dungeon loot (tools/dxtest/gen_dx_items.py), picked by data/dungeon/items_found.inc
DX_DUNGEON_ITEM = $(DUNGEONS_DIR)/dx/dungeon_item_data.inc
dungeon_item: $(DX_DUNGEON_ITEM)
$(DX_DUNGEON_ITEM): $(DUNGEONS_DIR)/dx/dungeon_item_data.json
	$(DUNGEONJSON) dungeon_item pmd-red $<
