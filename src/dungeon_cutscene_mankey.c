#include "global.h"
#include "globaldata.h"
#include "dungeon_cutscene.h"
#include "effect_main.h"
#include "dungeon_vram.h"
#include "dungeon_8041AD0.h"
#include "dungeon_mon_sprite_render.h"
#include "constants/bg_music.h"
#include "constants/direction.h"
#include "constants/dungeon_exit.h"
#include "constants/friend_area.h"
#include "constants/weather.h"
#include "dungeon_items.h"
#include "dungeon_range.h"
#include "dungeon_map_access.h"
#include "dungeon_message.h"
#include "dungeon_music.h"
#include "dungeon_misc.h"
#include "dungeon_logic.h"
#include "dungeon_random.h"
#include "dungeon_util.h"
#include "exclusive_pokemon.h"
#include "friend_area.h"
#include "items.h"
#include "pokemon_3.h"
#include "pokemon.h"
#include "position_util.h"
#include "trap.h"
#include "math.h"
#include "dungeon_config.h"
#include "dungeon_boss_dialogue.h"
#include "dx_rules.h"
#include "dx_tuning.h"
#include "dungeon_leveling.h"


static void sub_80898F8(Entity *r0);
static void sub_8089908(Entity *r0);

void sub_808970C(void)
{
    Entity *leaderEntity = CutsceneGetLeader();
    DungeonStartNewBGM(MUS_TINY_WOODS);
    sub_80854D4();
    sub_8085930(DIRECTION_NORTH);
    sub_80855E4(sub_8086A3C);
    sub_808563C(sub_8089908);
    sub_8085860(leaderEntity->pos.x,leaderEntity->pos.y -3);
}

#ifdef DX_RULES
static void DxMankeyRematch(Entity *mon)
{
    LevelUpTarget(mon, mon, DX_BOSS_REMATCH_LEVEL, 0, 0);
    SetFacingDirection(mon, DIRECTION_NORTH);
}
#endif

void sub_808974C(void)
{
    Entity *leaderEntity = CutsceneGetLeader();
#ifdef DX_RULES
    if (DxPostgame()) {
        // DX: after the credits the Mankey gang is back in Uproar Forest for a rematch
        DungeonStartNewBGM(MUS_TINY_WOODS);
        sub_80854D4();
        sub_8085930(DIRECTION_NORTH);
        sub_80855E4(sub_8086A3C);
        sub_808563C(DxMankeyRematch);
        sub_8085860(leaderEntity->pos.x,leaderEntity->pos.y -3);
        return;
    }
#endif
    sub_80854D4();
    sub_8085930(DIRECTION_NORTH);
    sub_80855E4(sub_8086A3C);
    sub_808563C(sub_80898F8);
    sub_8085860(leaderEntity->pos.x,leaderEntity->pos.y + -3);
}

void HandleMankeyBossFaint(Entity *entity, u8 monsterBehavior, u8 cutscene)
{
    s32 i;
    bool8 stillAlive = FALSE;

#ifdef DX_RULES
    if (cutscene == CUTSCENE_UPROAR_FOREST_ATTEMPT1 || cutscene == CUTSCENE_UPROAR_FOREST_ATTEMPT2 || cutscene == CUTSCENE_UPROAR_FOREST_POSTSTORY) {
#else
    if (cutscene == CUTSCENE_UPROAR_FOREST_ATTEMPT1 || cutscene == CUTSCENE_UPROAR_FOREST_ATTEMPT2) {
#endif
        for (i = 0; i < DUNGEON_MAX_WILD_POKEMON; i++) {
            Entity *monEntity = gDungeon->wildPokemon[i];
            if (EntityIsValid(monEntity) && monEntity != entity && GetEntInfo(monEntity)->monsterBehavior == monsterBehavior) {
                stillAlive = TRUE;
                break;
            }
        }

        if (!stillAlive){
            SetTempCutsceneFlag(CUTSCENE_FLAG_UPROAR_FOREST_COMPLETE);
            gDungeon->unk2 = DUNGEON_UNK2_1;
        }
    }
}

void sub_80897F0(void)
{
  Entity *leaderEntity;

  leaderEntity = CutsceneGetLeader();
  sub_8086448();
  DungeonWaitFrames_Async(10,0x46);
  sub_808563C(SpriteShockEffect);
  DungeonWaitFrames_Async(0x20,0x46);
  sub_808692C();
  DisplayDungeonDialogue_Async(&gUnknown_8103BD8);
  // We don't like to brag or
  // nothing, but
  DisplayDungeonDialogue_Async(&gUnknown_8103C00);
  // We get furious real easy
  // You know that?!
  DisplayDungeonDialogue_Async(&gUnknown_8103C3C);
  // What really irritates us is
  // when we see some dim witted face
  DisplayDungeonDialogue_Async(&gUnknown_8103C74);
  // .........
  // Wrooooooar!
  // That vacant expression
  DisplayDungeonDialogue_Async(&gUnknown_8103CC4);
  // Gwoooh
  // It's ticking me off!
  // I'm seeing red!
  DisplayDungeonDialogue_Async(&gUnknown_8103D0C);
  // Mukkkeeeeeh
  // Get 'em! It's frenzy time!
  DisplayDungeonDialogue_Async(&gUnknown_8103D50);
  DungeonWaitFrames_Async(10,0x46);
  ShiftCameraToPosition(&leaderEntity->pixelPos,0x10);
}

void sub_8089878(void)
{
  Entity *leaderEntity;

  leaderEntity = CutsceneGetLeader();
  sub_8086448();
  DungeonWaitFrames_Async(10,0x46);
  sub_808563C(SpriteShockEffect);
  DungeonWaitFrames_Async(0x20,0x46);
  sub_808692C();
  DisplayDungeonDialogue_Async(&gUnknown_8103D8C);
  DisplayDungeonDialogue_Async(&gUnknown_8103D98);
  DisplayDungeonDialogue_Async(&gUnknown_8103DD8);
  DisplayDungeonDialogue_Async(&gUnknown_8103E28);
  DungeonWaitFrames_Async(10,0x46);
  ShiftCameraToPosition(&leaderEntity->pixelPos,0x10);
}

void sub_80898E4(void)
{
#ifdef DX_RULES
    if (GetEntityFromMonsterBehavior(BEHAVIOR_MANKEY) != NULL) {
        sub_8089878(); // the rematch: Red's re-fight scene
        return;
    }
#endif
    sub_8086448();
    sub_80866C4(&gUnknown_8103E34);
}

static void sub_80898F8(Entity *r0)
{
    HandleFaint_Async(r0, DUNGEON_EXIT_DELETED_FOR_EVENT, 0);
}

static void sub_8089908(Entity *r0)
{
    SetFacingDirection(r0, DIRECTION_NORTH);
}

