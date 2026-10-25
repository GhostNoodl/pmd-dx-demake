#include "global.h"

#ifdef DX_RULES

#include "globaldata.h"
#include "constants/rare_quality.h"
#include "dx_rare_quality.h"
#include "dx_tuning.h"
#include "dungeon_message.h"
#include "dungeon_random.h"
#include "dungeon_util.h"
#include "dungeon_logic.h"
#include "move_orb_effects_2.h"
#include "pokemon.h"
#include "string_format.h"
#include "structs/str_dungeon.h"

// Rescue Team DX Rare Qualities: each Pokemon holds at most one (Pokemon.dxQuality, saved in the
// DX save block), and a quality held by anyone on the floor works for the whole team.

static const u8 *const sDxQualityNames[RQ_COUNT] = {
    [RQ_NONE] = _("None"),
    [RQ_BARGAIN] = _("Bargain"),
    [RQ_BLAST_CONTROL] = _("Blast Control"),
    [RQ_BRAWL] = _("Brawl"),
    [RQ_DEFENSIVE_RHYTHM] = _("Defensive Rhythm"),
    [RQ_EVASIVE_RHYTHM] = _("Evasive Rhythm"),
    [RQ_FOOD_FINDER] = _("Food Finder"),
    [RQ_FORGE_A_PATH] = _("Forge a Path"),
    [RQ_FRIENDLY] = _("Friendly"),
    [RQ_FUNNEL_FUN] = _("Funnel Fun"),
    [RQ_LEAP_AHEAD] = _("Leap Ahead"),
    [RQ_LEAVE_HALF] = _("Leave Half"),
    [RQ_LINK_BOOST] = _("Link Boost"),
    [RQ_LONELY_COURAGE] = _("Lonely Courage"),
    [RQ_MIND_OVER_MATTER] = _("Mind Over Matter"),
    [RQ_MOVING_UP] = _("Moving Up"),
    [RQ_NARROW_FOCUS] = _("Narrow Focus"),
    [RQ_NOTORIOUS_FASTING] = _("Notorious Fasting"),
    [RQ_NOTORIOUS_HEALING] = _("Notorious Healing"),
    [RQ_NOTORIOUS_RESTORATION] = _("Notorious Restoration"),
    [RQ_PP_POUCH] = _("PP Pouch"),
    [RQ_RAPID_BULLS_EYES] = _("Rapid Bull's-Eyes"),
    [RQ_RECOIL_BOOST] = _("Recoil Boost"),
    [RQ_RILED_UP] = _("Riled Up"),
    [RQ_SALES_PITCH] = _("Sales Pitch"),
    [RQ_SHARED_HAPPINESS] = _("Shared Happiness"),
    [RQ_SMALL_STOMACH] = _("Small Stomach"),
    [RQ_SQUAD_UP] = _("Squad Up"),
    [RQ_SQUEEZE_OUT] = _("Squeeze Out"),
    [RQ_STEAMROLL] = _("Steamroll"),
    [RQ_STRIKE_BACK] = _("Strike Back"),
    [RQ_THROWN_ITEM_BOOST] = _("Thrown Item Boost"),
    [RQ_TIGHT_FORMATION] = _("Tight Formation"),
    [RQ_WARY_WALK] = _("Wary Walk"),
    [RQ_XP_BOOST] = _("XP Boost"),
};

// qualities whose effects are built so far (the rest can't be granted yet)
static const u8 sDxQualitiesInPlay[] = {
    RQ_FRIENDLY, RQ_FUNNEL_FUN, RQ_MOVING_UP, RQ_NOTORIOUS_FASTING, RQ_NOTORIOUS_HEALING,
    RQ_SMALL_STOMACH, RQ_SQUAD_UP, RQ_XP_BOOST,
};

static const u8 sDxGainedQuality[] = _("{POKEMON_0} gained the Rare Quality\n{COLOR CYAN}%s{RESET}!");

const u8 *DxQualityName(u32 quality)
{
    return quality < RQ_COUNT ? sDxQualityNames[quality] : sDxQualityNames[RQ_NONE];
}

// the recruited Pokemon behind a team member in a dungeon (NULL for wild Pokemon)
Pokemon *DxEntityPokemon(Entity *entity)
{
    EntityInfo *info;
    s32 id;

    if (!EntityIsValid(entity))
        return NULL;
    info = GetEntInfo(entity);
    if (info->isNotTeamMember || info->teamIndex >= MAX_TEAM_MEMBERS)
        return NULL;
    id = gRecruitedPokemonRef->dungeonTeam[info->teamIndex].recruitedPokemonId;
    if (id < 0 || id >= NUM_MONSTERS)
        return NULL;
    return &gRecruitedPokemonRef->pokemon[id];
}

// does anyone on the team hold this quality? (DX: one member's quality works for everyone)
bool8 DxTeamHasQuality(u32 quality)
{
    s32 i;

    if (gDungeon == NULL)
        return FALSE;
    for (i = 0; i < MAX_TEAM_MEMBERS; i++) {
        Pokemon *mon = DxEntityPokemon(gDungeon->teamPokemon[i]);
        if (mon != NULL && mon->dxQuality == quality)
            return TRUE;
    }
    return FALSE;
}

s32 DxTeamSize(void)
{
    s32 i, count = 0;

    for (i = 0; i < MAX_TEAM_MEMBERS; i++) {
        if (EntityIsValid(gDungeon->teamPokemon[i]))
            count++;
    }
    return count;
}

u32 DxRandomQuality(void)
{
    return sDxQualitiesInPlay[DungeonRandInt(ARRAY_COUNT(sDxQualitiesInPlay))];
}

void DxGrantQuality(Entity *user, Entity *target, u32 quality)
{
    Pokemon *mon = DxEntityPokemon(target);
    u8 text[96];

    if (mon == NULL || quality == RQ_NONE || mon->dxQuality == quality)
        return;
    mon->dxQuality = quality;
    SetMessageArgument_2(gFormatBuffer_Monsters[0], GetEntInfo(target), 0);
    sprintfStatic(text, sDxGainedQuality, DxQualityName(quality));
    TryDisplayDungeonLoggableMessage3_Async(user, target, text);
}

// DX Gummis: a Rainbow Gummi (Red's type Gummis) raises a random stat and sometimes grants a Rare
// Quality; a DX Gummi raises a stat and always grants one. They no longer raise IQ.
void DxGummiItemAction(Entity *user, Entity *target, bool8 alwaysQuality)
{
    switch (DungeonRandInt(4)) {
        case 0: RaiseAtkStatTarget(user, target, DX_GUMMI_STAT_RAISE); break;
        case 1: RaiseSpAtkStatTarget(user, target, DX_GUMMI_STAT_RAISE); break;
        case 2: RaiseDefStatTarget(user, target, DX_GUMMI_STAT_RAISE); break;
        default: RaiseSpDefStatTarget(user, target, DX_GUMMI_STAT_RAISE); break;
    }
    if (alwaysQuality || DungeonRandInt(100) < DX_RAINBOW_GUMMI_QUALITY_CHANCE)
        DxGrantQuality(user, target, DxRandomQuality());
}

#endif // DX_RULES
