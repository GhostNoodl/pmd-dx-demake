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

#endif // GUARD_DX_TUNING_H
