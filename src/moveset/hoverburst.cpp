#include <SMS/Player/Mario.hxx>
#include <SMS/Player/Watergun.hxx>
#include <SMS/raw_fn.hxx>
#include <SMS/MSound/MSound.hxx>
#include <SMS/MSound/MSoundSESystem.hxx>

#include <BetterSMS/module.hxx>
#include <BetterSMS/player.hxx>
#include "common.hxx"

using namespace BetterSMS;

// This patches delayed fludd usage
static void snapNozzleToReady() {
    TWaterGun *fludd;
    SMS_FROM_GPR(30, fludd);

    if (fludd->mCurrentNozzle == TWaterGun::TNozzleType::Hover) {
        ((float *)(fludd))[0x1CEC / 4] = 0.0f;
    } else {
        ((float *)(fludd))[0x1CEC / 4] -= 0.1f;
        if (((float *)(fludd))[0x1CEC / 4] < 0.0f)
            ((float *)(fludd))[0x1CEC / 4] = 0.0f;
    }
}
SMS_PATCH_BL(SMS_PORT_REGION(0x802699CC, 0, 0, 0), snapNozzleToReady);

// extern -> fluddgeneral.cpp
BETTER_SMS_FOR_CALLBACK void checkSpamHover(TMario *player, bool isMario) {
    TWaterGun *fludd = player->mFludd;
    if (!fludd)
        return;

    if (fludd->mCurrentNozzle != TWaterGun::Hover || !gHoverBurstSetting.getBool())
        return;

    TNozzleTrigger *nozzle =
        reinterpret_cast<TNozzleTrigger *>(fludd->mNozzleList[fludd->mCurrentNozzle]);
    auto &emitParams = nozzle->mEmitParams;

    emitParams.mNum.set(1.0f);
    emitParams.mDirTremble.set(0.0f);

    auto *playerData = Player::getData(player);
    bool isAlive     = playerData->getCanSprayFludd();
    isAlive |= SMS_IsMarioTouchGround4cm__Fv();
    isAlive |= (player->mState & TMario::STATE_WATERBORN);
    isAlive |= (player->mState == TMario::STATE_NPC_BOUNCE);
    isAlive |= (player->mState == 0x350 || player->mState == 0x10000357 ||
                player->mState == 0x10000358);  // Ropes
    isAlive |= (player->mState == 0x10100341);  // Pole Climb
    playerData->setCanSprayFludd(isAlive);

    if ((player->mState & TMario::STATE_WATERBORN) || !playerData->getCanSprayFludd())
        return;

    if (player->mController->mButtons.mAnalogR < 0.9f ||
        !(player->mController->mFrameMeaning & 0x80))
        return;

    if ((emitParams.mTriggerTime.get() - nozzle->mSprayQuarterFramesLeft) >= 20)
        return;

    if (nozzle->mFludd->mCurrentWater < 510)
        return;

    emitParams.mNum.set(255.0f);
    emitParams.mDirTremble.set(0.5f);
    nozzle->emit(0);
    nozzle->emit(1);

    nozzle->mSprayQuarterFramesLeft = 0;
    nozzle->mSprayState             = TNozzleTrigger::DEAD;
    nozzle->mFludd->mCurrentWater -= 255;

    if ((player->mState & 0x800000) == 0) {
        player->mState = TMario::STATE_HOVER_F;
        player->mSpeed.y += (70.0f * player->mScale.y) - player->mSpeed.y;
        player->mJumpingState &= 0xFFFFFEFF;
    } else if (player->mState == TMario::STATE_DIVE) {
        player->mSpeed.y += (35.0f * player->mScale.y) - player->mSpeed.y;
        player->mForwardSpeed += 15.0f;
    } else if (player->mState == TMario::STATE_DIVESLIDE) {
        player->mSpeed.y += (35.0f * player->mScale.y) - player->mSpeed.y;
        player->mForwardSpeed += 30.0f;
    }

    playerData->setCanSprayFludd(false);
    return;
}