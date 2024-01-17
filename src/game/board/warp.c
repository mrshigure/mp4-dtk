#include "game/board/warp.h"
#include "game/board/main.h"
#include "game/board/player.h"
#include "game/wipe.h"
#include "game/gamework_data.h"
#include "game/hsfex.h"
#include "board_unsplit.h"
#include "math.h"

static void WarpInit(s32);
static void WarpLaunch(s32);
static void WarpStartImpact(s32);
static void WarpFall(s32);
static void WarpImpact(s32);


extern void omVibrate(s16, s16, s16, s16);
extern void fn_800816CC(s32, s32);
extern void fn_80081884(f32);
extern void fn_8008181C(void);
extern s32 fn_8008186C(void);
extern f32 BoardModelMotionTimeGet(s16);
extern void BoardModelHookReset(s16);
extern s32 BoardModelHookSet(s16, char*, s16);
extern void BoardPlayerPosSet(s32, f32, f32, f32);
extern void BoardPlayerRotYSet(s32, f32);
extern void Hu3DModelObjPosGet(s16, char*, Vec*);
extern s32 BoardModelMotionEndCheck(s16);

extern s32 boardTutorialData[4];

static Vec warpPos;

static s8 warpState;
static s8 warpTarget;
static s8 warpImpactCnt;
static s16 warpSpace;
static f32 warpYFloor;
static f32 warpYVel;
static Process* warpProcess;

static s16 warpSpringMdl = -1;
static s16 warpImpactMdl = -1;
static s8 warpImpactPlayer[4] = {-1, -1, -1, -1};

static void WarpProcess(void);
static void WarpKill(void);

void BoardWarpExec(s32 player, s32 space) {
    omVibrate(player, 12, 4, 2);
    warpProcess = HuPrcChildCreate(WarpProcess, 8195, 14336, 0, boardMainProc);
    HuPrcDestructorSet2(warpProcess, WarpKill);

    while (warpProcess != NULL) {
        HuPrcVSleep();
    }
    GWPlayer[player].color = 3;
}

void WarpProcess(void) {
    s32 curr_player;
    s32 warp_sound[8] = {291, 355, 419, 483, 547, 611, 675, 739};
    
    warpState = 0;
    curr_player = GWSystem.player_curr;
    HuAudFXPlay(842);
    BoardCameraViewSet(3);
    BoardCameraMotionWait();
    BoardPlayerAnimBlendSet(curr_player, 0, 15);

    while (BoardPlayerAnimBlendCheck(curr_player) == 0) {
        HuPrcVSleep();
    }
    
    BoardRotateDiceNumbers(curr_player);
    if (_CheckFlag(0x1000B) != 0) {
        BoardTutorialHookExec(18, 0);
        boardTutorialData[0] = 0;
    }
    fn_800816CC(curr_player, 3);
    fn_80081884(18.0f);
    fn_8008181C();
    warpTarget = fn_8008186C();
    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    WarpInit(curr_player);

    while (1) {
        switch (warpState) {
            case 0:
                if (BoardModelMotionTimeGet(warpSpringMdl) < 15.0f) {
                    break;
                }
                BoardModelHookReset(warpSpringMdl);
                Hu3DModelObjPosGet(BoardModelIDGet(warpSpringMdl), "warp01", &warpPos);
                BoardSpacePosGet(0, GWPlayer[curr_player].space_curr, &warpPos);
                warpPos.y += 700.0f;
                BoardPlayerPosSetV(curr_player, &warpPos);
                warpState = 1;
                HuAudFXPlay(warp_sound[GWPlayer[curr_player].character]);
                break;
            case 1:
                WarpLaunch(curr_player);
                break;
            case 2:
                if (BoardModelMotionEndCheck(warpSpringMdl) == 0) break;
                warpState = 6;
                break;
            case 3:
                WarpStartImpact(curr_player);
                break;
            case 4:
                WarpFall(curr_player);
                break;
            case 5:
                WarpImpact(curr_player);
                break;
            case 6:
                HuPrcEnd();
                break;
        }
        HuPrcVSleep();
    }
}

void WarpKill(void) {
    if (warpSpringMdl != -1) {
        BoardModelKill(warpSpringMdl);
        warpSpringMdl = -1;
    }
    if (warpImpactMdl != -1) {
        BoardModelKill(warpImpactMdl);
        warpImpactMdl = -1;
    }
    warpProcess = NULL;
}

static void WarpInit(s32 player) {
    Vec pos;
    s16 player_mdl = BoardPlayerModelGet(player);

    warpSpringMdl = BoardModelCreate(MAKE_DATA_NUM(DATADIR_BOARD, 1), NULL, 0);
    BoardSpacePosGet(0, GWPlayer[player].space_curr, &pos);
    warpYFloor = 1500.0f + pos.y;
    BoardModelLayerSet(warpSpringMdl, 2);
    BoardCameraTargetModelSet(warpSpringMdl);
    BoardModelMotionStart(warpSpringMdl, 0, 0);
    BoardModelHookSet(warpSpringMdl, "warp01", player_mdl);
    BoardModelPosSetV(warpSpringMdl, &pos);
    HuAudFXPlay(835);
    BoardPlayerRotYSet(player, 0.0f);
    BoardPlayerPosSet(player, 0.0f, 0.0f, 0.0f);
    BoardRotateDiceNumbers(player);
}

static void WarpLaunch(s32 player) {

    warpPos.y += 20.0f;
    BoardPlayerPosSetV(player, &warpPos);
    if (!(warpPos.y < warpYFloor)) {
        WipeColorSet(0U, 0U, 0U);
        WipeCreate(2, 0, 21);
        
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        warpPos.y = warpYFloor;
        BoardModelVisibilitySet(warpSpringMdl, 0);
        warpState = 3;
    }
}

static void WarpStartImpact(s32 player) {
    s32 i;

    warpSpace = GWPlayer[warpTarget].space_curr;
    GWPlayer[player].space_curr = warpSpace;
    BoardSpacePosGet(0, warpSpace, &warpPos);
    warpYFloor = warpPos.y;
    warpPos.y += 1500.0f;
    BoardPlayerPosSetV(player, &warpPos);
    BoardPlayerRotSet(player, 180.0f, 180.0f, 0.0f);
    
    for (i = 0, warpImpactCnt = i; i < 4; i++) {
        if (warpSpace == GWPlayer[i].space_curr) {
            warpImpactPlayer[warpImpactCnt] = i;
            warpImpactCnt++;
        }
    }
    warpImpactMdl = BoardModelCreate(MAKE_DATA_NUM(DATADIR_BOARD, 2), NULL, 0);
    BoardModelVisibilitySet(warpImpactMdl, 0);
    BoardModelMotionSpeedSet(warpImpactMdl, 0.0f);
    BoardCameraMoveSet(0);
    BoardCameraTargetSpaceSet(warpSpace);
    HuPrcSleep(1);
    WipeCreate(1, 0, 0x15);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardCameraMoveSet(1);
    warpYVel = -10.0f;
    warpState = 4;
}

static void WarpFall(s32 player) {
    warpYVel += -2.0f;
    warpPos.y += warpYVel;
    if (warpPos.y <= (150.0f + warpYFloor)) {
        warpPos.y = warpYFloor;
        warpState = 5;
        HuAudFXPlay(837);
        HuAudFXPlay(829);
    }
    BoardPlayerPosSetV(player, &warpPos);
}

static void WarpImpact(s32 player) {
    Vec pos;
    Vec pos_player;
    s16 temp;
    f32 speed;
    f32 temp_f30;
    f32 angle;
    s16 i;

    pos.x = 288.0f;
    pos.y = 240.0f;
    pos.z = 700.0f;
    Hu3D2Dto3D(&pos, 1, &pos);
    BoardModelPosSetV(warpImpactMdl, &pos);
    BoardModelVisibilitySet(warpImpactMdl, 1);
    BoardModelMotionStart(warpImpactMdl, 0, 0x40000001);
    
    for (i = 0; i < warpImpactCnt; i++) {
        s32 player = warpImpactPlayer[i];
        omVibrate(player, 0xC, 4, 2);
        BoardPlayerMotionStart(player, 6, 0x40000001);
    }
    BoardPlayerRotSet(player, 0.0f, 0.0f, 0.0f);
    BoardPlayerPosGet(player, &pos_player);
    pos_player.y = warpYFloor;
    BoardPlayerPosSetV(player, &pos_player);
    speed = 4.0f;
    
    for (angle = 0.0f, temp = angle; angle < 180.0f; angle += speed) {
        temp_f30 = sin((M_PI * angle) / 180.0);
        BoardModelScaleSet(warpImpactMdl, 0.5f + temp_f30, 0.5f + temp_f30, 0.5f + temp_f30);
        HuPrcVSleep();
    }
    BoardModelVisibilitySet(warpImpactMdl, 0);
    HuPrcSleep(60);
    
    for (i = 0; i < warpImpactCnt; i++) {
        BoardRotateDiceNumbers(warpImpactPlayer[i]);
    }
    warpState = 6;
}