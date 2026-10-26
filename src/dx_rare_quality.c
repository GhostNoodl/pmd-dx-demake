#include "global.h"

#ifdef DX_RULES

#include "globaldata.h"
#include "constants/rare_quality.h"
#include "dx_rare_quality.h"
#include "dungeon_vram.h"
#include "dungeon_move_util.h"
#include "dungeon_misc.h"
#include "dungeon_pos_data.h"
#include "structs/map.h"
#include "constants/direction.h"
#include "dungeon_engine.h"
#include "dungeon_move.h"
#include "text_util.h"
#include "dx_rules.h"
#include "structs/str_moves.h"
#include "items.h"
#include "move_orb_effects_5.h"
#include "number_util.h"
#include "dungeon_items.h"
#include "constants/status.h"
#include "constants/item.h"
#include "constants/move.h"
#include "dx_tuning.h"
#include "dungeon_message.h"
#include "dungeon_random.h"
#include "dungeon_util.h"
#include "dungeon_logic.h"
#include "move_orb_effects_2.h"
#include "pokemon.h"
#include "string_format.h"
#include "structs/str_dungeon.h"
#include "constants/move_id.h"
#include "constants/dungeon_exit.h"
#include "dungeon_map_access.h"
#include "dungeon_logic.h"
#include "move_orb_effects_1.h"
#include "moves.h"
#include "position_util.h"

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
    RQ_NARROW_FOCUS, RQ_RAPID_BULLS_EYES, RQ_STEAMROLL, RQ_LONELY_COURAGE, RQ_BRAWL, RQ_LINK_BOOST,
    RQ_RECOIL_BOOST, RQ_RILED_UP, RQ_STRIKE_BACK, RQ_DEFENSIVE_RHYTHM, RQ_EVASIVE_RHYTHM,
    RQ_BLAST_CONTROL, RQ_PP_POUCH,
    RQ_WARY_WALK, RQ_SQUEEZE_OUT, RQ_NOTORIOUS_RESTORATION, RQ_MIND_OVER_MATTER, RQ_BARGAIN, RQ_SALES_PITCH,
    RQ_THROWN_ITEM_BOOST, RQ_FOOD_FINDER,
};

static const u16 sDxRecoilMoves[] = {
    MOVE_DOUBLE_EDGE, MOVE_TAKE_DOWN, MOVE_SUBMISSION, MOVE_VOLT_TACKLE, MOVE_JUMP_KICK, MOVE_HI_JUMP_KICK,
};
static const u16 sDxMultiHitMoves[] = {
    MOVE_DOUBLESLAP, MOVE_COMET_PUNCH, MOVE_FURY_ATTACK, MOVE_FURY_SWIPES, MOVE_PIN_MISSILE, MOVE_SPIKE_CANNON,
    MOVE_BARRAGE, MOVE_DOUBLE_KICK, MOVE_TWINEEDLE, MOVE_BONEMERANG, MOVE_BONE_RUSH, MOVE_ARM_THRUST,
    MOVE_BULLET_SEED, MOVE_ICICLE_SPEAR, MOVE_ROCK_BLAST, MOVE_TRIPLE_KICK,
};

static bool8 DxMoveIn(const u16 *list, s32 count, u16 moveId)
{
    s32 i;
    for (i = 0; i < count; i++) {
        if (list[i] == moveId)
            return TRUE;
    }
    return FALSE;
}

static bool8 DxOnTeam(Entity *entity)
{
    return EntityIsValid(entity) && !GetEntInfo(entity)->isNotTeamMember;
}

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

// a team member's dungeon slot (a new recruit has no Pokemon record until the dungeon ends)
static DungeonMon *DxEntityDungeonMon(Entity *entity)
{
    EntityInfo *info;

    if (!EntityIsValid(entity))
        return NULL;
    info = GetEntInfo(entity);
    if (info->isNotTeamMember || info->teamIndex >= MAX_TEAM_MEMBERS)
        return NULL;
    return &gRecruitedPokemonRef->dungeonTeam[info->teamIndex];
}

u8 DxEntityQuality(Entity *entity)
{
    Pokemon *mon = DxEntityPokemon(entity);
    DungeonMon *dmon;

    if (mon != NULL)
        return mon->dxQuality;
    dmon = DxEntityDungeonMon(entity);
    return dmon != NULL ? dmon->dxQuality : RQ_NONE;
}

// does anyone on the team hold this quality? (DX: one member's quality works for everyone)
bool8 DxTeamHasQuality(u32 quality)
{
    s32 i;

    if (gDungeon == NULL)
        return FALSE;
    for (i = 0; i < MAX_TEAM_MEMBERS; i++) {
        Entity *mate = gDungeon->teamPokemon[i];
        if (EntityIsValid(mate) && DxEntityQuality(mate) == quality)
            return TRUE;
    }
    return FALSE;
}

static const u8 sDxJoinedWithQuality[] = _("{POKEMON_0} has the Rare Quality\n{COLOR CYAN}%s{RESET}!");

// DX: rare recruits (a Mystery House's newcomer, a shiny) join with a Rare Quality
u8 DxRecruitQuality(s16 species, bool8 shiny, Entity *leader)
{
    u8 text[96];
    u8 quality;

    if (!shiny && (gDxMysteryHouseSpawned == 0 || species != gDxMysteryHouseSpawned))
        return RQ_NONE;
    quality = DxRandomQuality();
    CopyCyanMonsterNametoBuffer(gFormatBuffer_Monsters[0], species);
    sprintfStatic(text, sDxJoinedWithQuality, DxQualityName(quality));
    LogMessageByIdWithPopupCheckUser_Async(leader, text);
    return quality;
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
    DungeonMon *dmon = DxEntityDungeonMon(target);
    u8 text[96];

    if (dmon == NULL || quality == RQ_NONE || DxEntityQuality(target) == quality)
        return;
    // both copies: the dungeon slot is what goes back to the Pokemon when the dungeon ends
    if (mon != NULL)
        mon->dxQuality = quality;
    dmon->dxQuality = quality;
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

// Narrow Focus (corridors) and Rapid Bull's-Eyes (multi-hit moves): the move never misses
bool8 DxQualityAlwaysHits(Entity *attacker, u16 moveId)
{
    if (!DxOnTeam(attacker))
        return FALSE;
    if (DxTeamHasQuality(RQ_NARROW_FOCUS) && GetTile(attacker->pos.x, attacker->pos.y)->room == CORRIDOR_ROOM)
        return TRUE;
    if (DxTeamHasQuality(RQ_RAPID_BULLS_EYES) && DxMoveIn(sDxMultiHitMoves, ARRAY_COUNT(sDxMultiHitMoves), moveId))
        return TRUE;
    return FALSE;
}

// Steamroll: a team move is never resisted (immune / not very effective count as neutral)
bool8 DxSteamroll(Entity *attacker)
{
    return DxOnTeam(attacker) && DxTeamHasQuality(RQ_STEAMROLL);
}

// damage from a team member's move, in percent: Lonely Courage, Brawl, Link Boost, Recoil Boost
s32 DxQualityDamagePercent(Entity *attacker, u16 moveId)
{
    s32 i, percent = 100;
    EntityInfo *info;

    if (!DxOnTeam(attacker))
        return 100;
    info = GetEntInfo(attacker);
    if (DxTeamHasQuality(RQ_LONELY_COURAGE)) {
        bool8 alone = TRUE;
        for (i = 0; i < MAX_TEAM_MEMBERS; i++) {
            Entity *mate = gDungeon->teamPokemon[i];
            if (EntityIsValid(mate) && mate != attacker && GetDistance(&mate->pos, &attacker->pos) <= 2)
                alone = FALSE;
        }
        if (alone)
            percent += DX_RQ_LONELY_COURAGE_BONUS;
    }
    if (DxTeamHasQuality(RQ_BRAWL)) {
        u8 room = GetTile(attacker->pos.x, attacker->pos.y)->room;
        s32 foes = 0;
        for (i = 0; i < DUNGEON_MAX_WILD_POKEMON; i++) {
            Entity *foe = gDungeon->wildPokemon[i];
            if (EntityIsValid(foe) && room != CORRIDOR_ROOM && GetTile(foe->pos.x, foe->pos.y)->room == room)
                foes++;
        }
        if (foes > DX_RQ_BRAWL_MAX_FOES)
            foes = DX_RQ_BRAWL_MAX_FOES;
        percent += foes * DX_RQ_BRAWL_PER_FOE;
    }
    if (DxTeamHasQuality(RQ_LINK_BOOST) && info->usedLinkedMovesCounter > 1)
        percent += info->usedLinkedMovesCounter * DX_RQ_LINK_BOOST_PER_MOVE;
    if (DxTeamHasQuality(RQ_RECOIL_BOOST) && DxMoveIn(sDxRecoilMoves, ARRAY_COUNT(sDxRecoilMoves), moveId))
        percent += DX_RQ_RECOIL_BOOST_BONUS;
    if (DxTeamHasQuality(RQ_THROWN_ITEM_BOOST) && moveId == MOVE_PROJECTILE) // thrown spikes
        percent = percent * DX_RQ_THROWN_PERCENT / 100;
    return percent;
}

// Blast Control: explosions hurt the team less
s32 DxBlastDamage(Entity *target, s32 dmg, s32 dungeonExitReason)
{
    if (DxOnTeam(target) && DxTeamHasQuality(RQ_BLAST_CONTROL)
        && (dungeonExitReason == DUNGEON_EXIT_DISAPPEARED_IN_EXPLOSION || dungeonExitReason == DUNGEON_EXIT_DEFEATED_BLAST_SEED))
        return dmg / 2;
    return dmg;
}

// a team member was hit by an enemy: Riled Up, Strike Back, Defensive Rhythm, Evasive Rhythm
void DxQualityOnTeamHit(Entity *attacker, Entity *target)
{
    StatIndex physical = {STAT_INDEX_PHYSICAL}, special = {STAT_INDEX_SPECIAL};
    EntityInfo *info;

    if (!DxOnTeam(target) || !EntityIsValid(attacker) || !GetEntInfo(attacker)->isNotTeamMember)
        return;
    info = GetEntInfo(target);
    if (DxTeamHasQuality(RQ_RILED_UP)) {
        BoostOffensiveStat(target, target, physical, 1);
        BoostOffensiveStat(target, target, special, 1);
    }
    if (DxTeamHasQuality(RQ_STRIKE_BACK) && GetDistance(&attacker->pos, &target->pos) <= 1) {
        LowerOffensiveStat(target, attacker, physical, 1, TRUE, FALSE);
        LowerOffensiveStat(target, attacker, special, 1, TRUE, FALSE);
    }
    // DX's Rhythms last one turn; here they're a +1 stage that doesn't stack
    if (DxTeamHasQuality(RQ_DEFENSIVE_RHYTHM) && info->defensiveStages[0] <= DEFAULT_STAT_STAGE) {
        BoostDefensiveStat(target, target, physical, 1);
        BoostDefensiveStat(target, target, special, 1);
    }
    if (DxTeamHasQuality(RQ_EVASIVE_RHYTHM) && info->hitChanceStages[1] <= DEFAULT_STAT_STAGE)
        info->hitChanceStages[1]++;
}

// PP Pouch: defeating an enemy restores a little PP to the victor's moves
void DxQualityOnDefeat(Entity *attacker)
{
    s32 i;
    EntityInfo *info;

    if (!DxOnTeam(attacker) || !DxTeamHasQuality(RQ_PP_POUCH))
        return;
    info = GetEntInfo(attacker);
    for (i = 0; i < MAX_MON_MOVES; i++) {
        Move *move = &info->moves.moves[i];
        if ((move->moveFlags & MOVE_FLAG_EXISTS) && move->PP < GetMoveBasePP(move))
            move->PP++;
    }
}

// a team member holds (or the team shares) this quality and the entity is on the team
bool8 DxTeamMemberHas(Entity *entity, u32 quality)
{
    return DxOnTeam(entity) && DxTeamHasQuality(quality);
}

// Squeeze Out: now and then a move with no PP left gets 1 back (each turn)
void DxSqueezeOut(Entity *entity)
{
    s32 i;
    EntityInfo *info;

    if (!DxTeamMemberHas(entity, RQ_SQUEEZE_OUT))
        return;
    info = GetEntInfo(entity);
    for (i = 0; i < MAX_MON_MOVES; i++) {
        Move *move = &info->moves.moves[i];
        if ((move->moveFlags & MOVE_FLAG_EXISTS) && move->PP == 0 && DungeonRandInt(100) < DX_RQ_SQUEEZE_OUT_CHANCE)
            move->PP = 1;
    }
}

// Bargain / Sales Pitch: shop prices in dungeons (Kecleon)
s32 DxShopPrice(s32 price, bool8 selling)
{
    if (gDungeon == NULL)
        return price;
    if (!selling && DxTeamHasQuality(RQ_BARGAIN))
        return price * DX_RQ_BARGAIN_PERCENT / 100;
    if (selling && DxTeamHasQuality(RQ_SALES_PITCH))
        return price * DX_RQ_SALES_PITCH_PERCENT / 100;
    return price;
}

#endif // DX_RULES

// Tight Formation: a teammate hit by a wide-ranging move (around the user, the room, the floor)
// takes less damage for each teammate next to it
s32 DxTightFormationPercent(Entity *target, u16 moveId)
{
    Move move;
    s32 i, range, cut = 0;

    if (!DxTeamMemberHas(target, RQ_TIGHT_FORMATION))
        return 100;
    move.id = moveId;
    range = GetMoveTargetAndRange(&move, FALSE) & 0x70;
    if (range != TARGETING_FLAG_TARGET_AROUND && range != TARGETING_FLAG_TARGET_ROOM && range != TARGETING_FLAG_TARGET_FLOOR
        && range != TARGETING_FLAG_TARGET_FRONTAL_CONE)
        return 100;
    for (i = 0; i < MAX_TEAM_MEMBERS; i++) {
        Entity *mate = gDungeon->teamPokemon[i];
        if (EntityIsValid(mate) && mate != target && GetDistance(&mate->pos, &target->pos) <= 1)
            cut += DX_RQ_TIGHT_FORMATION_PER_MATE;
    }
    if (cut > DX_RQ_TIGHT_FORMATION_MAX)
        cut = DX_RQ_TIGHT_FORMATION_MAX;
    return 100 - cut;
}

static const u8 sDxLeftHalf[] = _("An {COLOR YELLOW}Apple{RESET} was left over!");

// Leave Half: eating a Big Apple or Huge Apple leaves an Apple (in the toolbox, or on the ground
// when it is full). Red has no Max Ether or Tiny Reviver Seed, DX's other two leftovers.
void DxLeaveHalf(Entity *target, u8 itemId)
{
    Item apple;

    if ((itemId != ITEM_BIG_APPLE && itemId != ITEM_HUGE_APPLE) || !DxTeamMemberHas(target, RQ_LEAVE_HALF))
        return;
    ItemIdToItem(&apple, ITEM_APPLE, FALSE);
    if (AddItemToInventory(&apple))
        SpawnDroppedItemWrapper(target, &target->pos, &apple);
    LogMessageByIdWithPopupCheckUser_Async(target, sDxLeftHalf);
}

static bool8 DxHasCondition(Entity *mon, u8 kind)
{
    EntityInfo *info = GetEntInfo(mon);

    switch (kind) {
        case DX_CURE_PARALYSIS: return info->burnClassStatus.status == STATUS_PARALYSIS;
        case DX_CURE_POISON: return ENTITY_POISONED(info);
        case DX_CURE_BURN: return info->burnClassStatus.status == STATUS_BURN;
    }
    return MonsterHasNegativeStatus(mon);
}

// Shared Happiness: curing a teammate's bad condition also cures another teammate's (the nearest one)
void DxSharedHappiness(Entity *user, Entity *target, u8 kind)
{
    s32 i, best = 999;
    Entity *other = NULL;

    if (!DxTeamMemberHas(target, RQ_SHARED_HAPPINESS))
        return;
    for (i = 0; i < MAX_TEAM_MEMBERS; i++) {
        Entity *mate = gDungeon->teamPokemon[i];
        if (EntityIsValid(mate) && mate != target && DxHasCondition(mate, kind) && GetDistance(&mate->pos, &target->pos) < best) {
            best = GetDistance(&mate->pos, &target->pos);
            other = mate;
        }
    }
    if (other == NULL)
        return;
    if (kind == DX_CURE_ALL)
        sub_8079F20(user, other, 1, 0);
    else
        EndBurnClassStatus(user, other);
}

// Forge a Path: the leader walks through walls (not diagonally, not in boss rooms) while its Belly lasts
bool8 DxCanForgeAPath(Entity *entity)
{
    return DxTeamMemberHas(entity, RQ_FORGE_A_PATH) && GetEntInfo(entity)->isTeamLeader
        && FixedPointToInt(GetEntInfo(entity)->belly) > 0;
}

void DxForgeAPathCost(Entity *entity)
{
    EntityInfo *info = GetEntInfo(entity);

    if (FixedPointToInt(info->belly) <= DX_RQ_FORGE_A_PATH_BELLY)
        info->belly = IntToFixedPoint(0);
    else
        info->belly = FixedPoint_Subtract(info->belly, IntToFixedPoint(DX_RQ_FORGE_A_PATH_BELLY));
}

static const u8 sDxLeaptAhead[] = _("{POKEMON_0} leapt ahead!");
// tests: how many times a teammate leapt ahead
EWRAM_DATA u8 gDxLeapAheadCount = 0;
#ifdef DX_DEBUG
EWRAM_DATA bool8 gDxTestFoeInFront = FALSE;
#endif

// Leap Ahead: when the leader attacks a foe in front of it in a corridor, the teammate right behind
// it leaps in and uses a 1-tile move on that foe too (its first damaging one with PP, or a regular
// attack), then is back in its place
void DxLeapAhead(Entity *leader)
{
    EntityInfo *info, *mateInfo;
    Entity *foe, *mate;
    Entity *targets[MAX_MOVE_TARGETS + 1];
    Move move;
    s32 i, dir;

    if (!EntityIsValid(leader) || IsFloorOver() || !DxTeamMemberHas(leader, RQ_LEAP_AHEAD))
        return;
    info = GetEntInfo(leader);
    if (!info->isTeamLeader || GetTile(leader->pos.x, leader->pos.y)->room != CORRIDOR_ROOM)
        return;
    dir = info->action.direction & DIRECTION_MASK;
#ifdef DX_DEBUG
    // tests: set the scene. Face the leader along the corridor (the teammate behind it, an open
    // tile in front) and bring a wild Pokemon onto that tile.
    if (gDxTestFoeInFront) {
        s32 d;
        gDxTestFoeInFront = FALSE;
        for (d = 0; d < NUM_DIRECTIONS; d += 2) {
            const Tile *front = GetTile(leader->pos.x + gAdjacentTileOffsets[d].x, leader->pos.y + gAdjacentTileOffsets[d].y);
            Entity *back = GetTile(leader->pos.x - gAdjacentTileOffsets[d].x, leader->pos.y - gAdjacentTileOffsets[d].y)->monster;
            if (front->monster == NULL && (front->terrainFlags & TERRAIN_TYPE_NORMAL) && EntityIsValid(back)
                && GetEntityType(back) == ENTITY_MONSTER && !GetEntInfo(back)->isNotTeamMember) {
                for (i = 0; i < DUNGEON_MAX_WILD_POKEMON; i++) {
                    if (EntityIsValid(gDungeon->wildPokemon[i])) {
                        sub_80694C0(gDungeon->wildPokemon[i], leader->pos.x + gAdjacentTileOffsets[d].x, leader->pos.y + gAdjacentTileOffsets[d].y, 0);
                        info->action.direction = d;
                        dir = d;
                        break;
                    }
                }
                break;
            }
        }
    }
#endif
    foe = GetTile(leader->pos.x + gAdjacentTileOffsets[dir].x, leader->pos.y + gAdjacentTileOffsets[dir].y)->monster;
    if (!EntityIsValid(foe) || GetEntityType(foe) != ENTITY_MONSTER || !GetEntInfo(foe)->isNotTeamMember)
        return;
    mate = GetTile(leader->pos.x - gAdjacentTileOffsets[dir].x, leader->pos.y - gAdjacentTileOffsets[dir].y)->monster;
    if (!EntityIsValid(mate) || GetEntityType(mate) != ENTITY_MONSTER || GetEntInfo(mate)->isNotTeamMember)
        return;
    mateInfo = GetEntInfo(mate);
    if (CheckVariousStatuses(mate) || IsChargingAnyTwoTurnMove(mate, FALSE))
        return; // asleep, frozen, mid-Fly and the like

    InitPokemonMove(&move, MOVE_REGULAR_ATTACK);
    for (i = 0; i < MAX_MON_MOVES; i++) {
        Move *m = &mateInfo->moves.moves[i];
        if ((m->moveFlags & MOVE_FLAG_EXISTS) && m->PP != 0 && DxMoveIsDamaging(m)
            && (GetMoveTargetAndRange(m, FALSE) & 0xFF) == (TARGETING_FLAG_TARGET_OTHER)) {
            move = *m;
            m->PP--;
            break;
        }
    }
#if DX_NO_REGULAR_ATTACK
    if (move.id == MOVE_REGULAR_ATTACK)
        return; // no move of its own to leap in with
#endif
    gDxLeapAheadCount++;
    SubstitutePlaceholderStringTags(gFormatBuffer_Monsters[0], mate, 0);
    LogMessageByIdWithPopupCheckUser_Async(mate, sDxLeaptAhead);
    mateInfo->action.direction = dir;
    for (i = 0; i <= MAX_MOVE_TARGETS; i++)
        targets[i] = NULL;
    targets[0] = foe;
    // the leap: over the leader to the front, the strike, and back (a tile is 24 pixels)
    for (i = 0; i < 12; i++) {
        IncreaseEntityPixelPos(mate, gAdjacentTileOffsets[dir].x * 0x200, gAdjacentTileOffsets[dir].y * 0x200);
        DungeonRunFrameActions(0x46);
    }
    UseMoveAgainstTargets(targets, mate, &move, 0, FALSE);
    if (EntityIsValid(mate)) {
        for (i = 0; i < 12; i++) {
            IncreaseEntityPixelPos(mate, -gAdjacentTileOffsets[dir].x * 0x200, -gAdjacentTileOffsets[dir].y * 0x200);
            DungeonRunFrameActions(0x46);
        }
    }
}
