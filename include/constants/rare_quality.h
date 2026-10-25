#ifndef GUARD_CONSTANTS_RARE_QUALITY_H
#define GUARD_CONSTANTS_RARE_QUALITY_H

// Rescue Team DX Rare Qualities (Mystery Dungeon Wiki, "Rescue Team DX: List of Rare Qualities").
// A Pokemon holds at most one; it works for the whole team on the floor (dx_rare_quality.c).
enum RareQuality
{
    RQ_NONE,
    RQ_BARGAIN,
    RQ_BLAST_CONTROL,
    RQ_BRAWL,
    RQ_DEFENSIVE_RHYTHM,
    RQ_EVASIVE_RHYTHM,
    RQ_FOOD_FINDER,
    RQ_FORGE_A_PATH,
    RQ_FRIENDLY,
    RQ_FUNNEL_FUN,
    RQ_LEAP_AHEAD,
    RQ_LEAVE_HALF,
    RQ_LINK_BOOST,
    RQ_LONELY_COURAGE,
    RQ_MIND_OVER_MATTER,
    RQ_MOVING_UP,
    RQ_NARROW_FOCUS,
    RQ_NOTORIOUS_FASTING,
    RQ_NOTORIOUS_HEALING,
    RQ_NOTORIOUS_RESTORATION,
    RQ_PP_POUCH,
    RQ_RAPID_BULLS_EYES,
    RQ_RECOIL_BOOST,
    RQ_RILED_UP,
    RQ_SALES_PITCH,
    RQ_SHARED_HAPPINESS,
    RQ_SMALL_STOMACH,
    RQ_SQUAD_UP,
    RQ_SQUEEZE_OUT,
    RQ_STEAMROLL,
    RQ_STRIKE_BACK,
    RQ_THROWN_ITEM_BOOST,
    RQ_TIGHT_FORMATION,
    RQ_WARY_WALK,
    RQ_XP_BOOST,
    RQ_COUNT
};

// Pokemon.dxFlags / DungeonMon.dxFlags
#define DX_FLAG_SHINY (1 << 0)
#define DX_FLAG_VISITOR (1 << 1) // joined in a dungeon without its camp: leaves (with Poke) at the end

#endif // GUARD_CONSTANTS_RARE_QUALITY_H
