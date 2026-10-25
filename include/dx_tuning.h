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

#endif // GUARD_DX_TUNING_H
