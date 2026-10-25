#ifndef GUARD_DX_TUNING_H
#define GUARD_DX_TUNING_H

// Rescue Team DX tuning (modern builds, DX_RULES). No published source gives DX's actual numbers
// for these, so they are ESTIMATES picked to match how DX plays (see DX_LOG.md, "Estimated values").
// Change them here and rebuild.

// Belly lost per turn, as a percentage of Red's 0.1 per turn (100 = same as Red)
#define DX_BELLY_DRAIN_PERCENT 100

// Natural HP regen: each turn a team member heals maxHP / regenSpeed. GetRegenSpeed() doubles the
// species value (usually 100 -> 200), so Red heals 0.5% of max HP per turn. DX heals much faster:
// this scales regenSpeed down.
#define DX_REGEN_SPEED_PERCENT 25 // 25 = four times Red's rate (2% of max HP per turn, ~50 turns to full)
// DX: no natural regen on a turn spent using a move or an item
#define DX_REGEN_PAUSES_AFTER_ACTION TRUE

// Oran Berry eaten at full HP raises max HP by this much (Red: 0)
#define DX_ORAN_BERRY_MAX_HP_RISE 10

// Speed-based double moves: a regular damaging move strikes a second time when the user's
// mainline base Speed beats the target's by at least DX_DOUBLE_MOVE_SPEED_GAP. The chance
// starts at DX_DOUBLE_MOVE_BASE_CHANCE% and grows 1% per DX_DOUBLE_MOVE_SPEED_PER_PERCENT
// more Speed, up to DX_DOUBLE_MOVE_MAX_CHANCE%. Speed-boosted users count double.
#define DX_DOUBLE_MOVE_SPEED_GAP 20
#define DX_DOUBLE_MOVE_BASE_CHANCE 10
#define DX_DOUBLE_MOVE_SPEED_PER_PERCENT 4
#define DX_DOUBLE_MOVE_MAX_CHANCE 30

// Mystery Houses: on a newcomer's home dungeon (gen_dx_species.py HOMES), each floor has this
// chance to force a Monster House with that newcomer inside, at the floor's top spawn level
#define DX_MYSTERY_HOUSE_CHANCE 8

// Move Growth: each time a team member uses a damaging move it may grow one level (+1 power,
// shown as "Tackle+1", the same field as Red's Ginseng boost, up to Red's per-move cap).
// Chance per use = BASE - FALLOFF * current level, never below MIN (percent)
#define DX_MOVE_GROWTH_BASE_CHANCE 20
#define DX_MOVE_GROWTH_FALLOFF 3
#define DX_MOVE_GROWTH_MIN_CHANCE 2

// Empowerment Seed: stat stages gained by an Empowered Pokemon (lasts the floor, like any stat change)
#define DX_EMPOWER_STAGES 2

// Rare Qualities (ESTIMATED strengths; DX's numbers aren't published)
#define DX_RAINBOW_GUMMI_QUALITY_CHANCE 25 // % a Rainbow Gummi also grants a quality
#define DX_GUMMI_STAT_RAISE 1              // a Gummi raises one random stat by this much
#define DX_RQ_XP_BOOST_PERCENT 150         // XP Boost: Exp. from defeated enemies
#define DX_RQ_HEALING_PERCENT 50           // Notorious Healing: regen speed (lower = faster)
#define DX_RQ_FUNNEL_PERCENT 33            // Funnel Fun: regen speed in corridors
#define DX_RQ_FRIENDLY_BONUS 100           // Friendly: recruit rate (out of 1000)
#define DX_RQ_SQUAD_UP_BONUS 25            // Squad Up: recruit rate per team member
#define DX_RQ_LONELY_COURAGE_BONUS 50      // Lonely Courage: % more damage with no teammate within 2 tiles
#define DX_RQ_BRAWL_PER_FOE 10             // Brawl: % more damage per enemy in the room...
#define DX_RQ_BRAWL_MAX_FOES 5             // ...counting at most this many
#define DX_RQ_LINK_BOOST_PER_MOVE 10       // Link Boost: % more damage per linked move used
#define DX_RQ_RECOIL_BOOST_BONUS 50        // Recoil Boost: % more damage for recoil/crash moves
#define DX_RQ_WARY_WALK_CHANCE 50          // Wary Walk: % a trap a team member steps on does nothing
#define DX_RQ_SQUEEZE_OUT_CHANCE 5         // Squeeze Out: % per turn a 0 PP move gets 1 PP back
#define DX_RQ_RESTORATION_PERCENT 150      // Notorious Restoration: HP / Belly from items
#define DX_RQ_BARGAIN_PERCENT 75           // Bargain: dungeon shop buying prices
#define DX_RQ_SALES_PITCH_PERCENT 125      // Sales Pitch: dungeon shop selling prices
#define DX_RQ_THROWN_PERCENT 150           // Thrown Item Boost: stone and spike damage
#define DX_RQ_FOOD_FINDER_CHANCE 15        // Food Finder: % of floor items that become an Apple

// Strong Foes (ESTIMATED): chance per floor of a cleared dungeon, level over the floor's top level,
// stat stages, and Exp. multiplier
#define DX_STRONG_FOE_CHANCE 15
#define DX_STRONG_FOE_LEVELS 5
#define DX_STRONG_FOE_STAGES 2
#define DX_STRONG_FOE_EXP_MULTIPLIER 3
#define DX_SHINY_CHANCE 10 // % a Strong Foe of a shiny species is shiny (ESTIMATED)

#define DX_BOSS_REMATCH_LEVEL 40 // level of a post-game boss rematch (ESTIMATED)

#define DX_BOSS_FALLBACK_RECRUIT_RATE 100 // per mille, a post-game story boss Red never lets join (ESTIMATED)

#define DX_RQ_TIGHT_FORMATION_PER_MATE 10 // % less damage from wide-ranging moves per adjacent teammate (ESTIMATED)
#define DX_RQ_TIGHT_FORMATION_MAX 30 // (ESTIMATED)
#define DX_RQ_FORGE_A_PATH_BELLY 5 // Belly per wall broken (ESTIMATED)

#endif // GUARD_DX_TUNING_H
