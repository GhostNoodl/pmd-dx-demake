static const WindowTemplate sUnknown_80E7D40 = {
    0,
    3,
    0, 0,
    0, 0,
    0, 0,
    NULL
};
#ifdef DX_DEBUG
// dx: the retail game dropped the screens behind Field/Field Map/Field Script/
// Debug Menu (choosing them soft-locks), so only our own tools are listed.
// Their slots now open the dx tools (see main_menu1.c / dx_tools.c).
static const WindowTemplate sUnknown_80E7D58 = {
    0,
    3,
    2, 2,
    9, 15,
    15, 0,
    NULL
};

static const MenuItem sDebugMenuItems[] = {
    {_("Warp"), MENU_DEBUG_MENU_DUNGEONS},
    {_("Give Item"), MENU_DEBUG_MENU_FIELD},
    {_("Recruit"), MENU_DEBUG_MENU_FIELD_MAP},
    {_("Set Level"), MENU_DEBUG_MENU_FIELD_SCRIPT},
    {_("Story"), MENU_DEBUG_MENU_DEBUG_MENU},
    {_("Storage"), MENU_DEBUG_MENU_STORAGE},
    {_("Evolve"), MENU_DEBUG_MENU_UNKNOWN_1},
    {_("Scenarios"), 7},
    {_("h-open"), MENU_DEBUG_MENU_H_OPEN},
    {NULL, 10},
};
#else
static const WindowTemplate sUnknown_80E7D58 = {
    0,
    3,
    2, 2,
    9, 11,
    11, 0,
    NULL
};

static const MenuItem sDebugMenuItems[] = {
    {_("Dungeons"), MENU_DEBUG_MENU_DUNGEONS},
    {_("Field"), MENU_DEBUG_MENU_FIELD},
    {_("Field Map"), MENU_DEBUG_MENU_FIELD_MAP},
    {_("Field Script"), MENU_DEBUG_MENU_FIELD_SCRIPT},
    {_("Debug Menu"), MENU_DEBUG_MENU_DEBUG_MENU},
    {_("Storage"), MENU_DEBUG_MENU_STORAGE},
    {_("h-open"), MENU_DEBUG_MENU_H_OPEN},
    {NULL, 10},
};
#endif // DX_DEBUG
