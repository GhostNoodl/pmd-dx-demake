#ifndef GUARD_DX_TOOLS_H
#define GUARD_DX_TOOLS_H

#ifdef DX_DEBUG

// dx test tools on the debug menu, all built on one "pick numbers, see what they mean" screen
enum DxToolId
{
    DX_TOOL_GIVE_ITEM,
    DX_TOOL_RECRUIT,
    DX_TOOL_SET_LEVEL,
    DX_TOOL_STORY,
    DX_TOOL_SCENARIO,
    DX_TOOL_COUNT,
};

extern u8 gDxScenario; // the Scenarios tool's last pick (0 = none)

void CreateDxToolMenu(u32 tool);
u32 UpdateDxToolMenu(void);
void DeleteDxToolMenu(void);

#endif // DX_DEBUG

#endif // GUARD_DX_TOOLS_H
