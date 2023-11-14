#pragma once

struct AimBot {
    ConfigLoader* cl;
    XDisplay* display;
    Level* level;
    LocalPlayer* localPlayer;
    std::vector<Player*>* players;
    Player* target = nullptr;

    AimBot(ConfigLoader* cl, XDisplay* display, Level* level, LocalPlayer* localPlayer, std::vector<Player*>* players) {
        this->cl = cl;
        this->display = display;
        this->level = level;
        this->localPlayer = localPlayer;
        this->players = players;
    }

    void aimAssist() {
        if (!active()) { releaseTarget(); return; }
        if (target == nullptr) assignTarget();
        if (target == nullptr) return;
        if (!target->visible) return;
        if (target->distance2DToLocalPlayer < util::metersToGameUnits(cl->AIMBOT_MIN_DISTANCE)) return;
        if (target->distance2DToLocalPlayer > util::metersToGameUnits(cl->AIMBOT_MAX_DISTANCE)) return;
        moveMouse();
    }

    void moveMouse() const {
        //calculate smoothing    
        float EXTRA_SMOOTH = cl->AIMBOT_SMOOTH_EXTRA_BY_DISTANCE / target->distanceToLocalPlayer;
        float TOTAL_SMOOTH = cl->AIMBOT_SMOOTH + EXTRA_SMOOTH;
        // //No recoil calcs
        // const FloatVector2D punchAnglesDiff = localPlayer->punchAnglesDiff
        //     .multiply(100)
        //     .divide(TOTAL_SMOOTH);
        // const double nrPitchIncrement = punchAnglesDiff.x;
        // const double nrYawIncrement = -punchAnglesDiff.y;
        //Aimbot calcs
        const FloatVector2D aimbotDelta = target->aimbotDesiredAnglesIncrement
            .multiply(100)
            .divide(TOTAL_SMOOTH);

        const float aimYawIncrement = aimbotDelta.y * -1;
        const float aimPitchIncrement = aimbotDelta.x;
        //combine
        const float totalPitchIncrement = aimPitchIncrement;// + nrPitchIncrement;
        const float totalYawIncrement = aimYawIncrement;// + nrYawIncrement;
        //turn into integers
        int totalPitchIncrementInt = roundHalfEven(atLeast_1_AwayFromZero(totalPitchIncrement));
        int totalYawIncrementInt = roundHalfEven(atLeast_1_AwayFromZero(totalYawIncrement));
        //deadzone - are we close enough yet?
        if (fabs(target->aimbotDesiredAnglesIncrement.x) < cl->AIMBOT_DEADZONE) totalPitchIncrementInt = 0;
        if (fabs(target->aimbotDesiredAnglesIncrement.y) < cl->AIMBOT_DEADZONE) totalYawIncrementInt = 0;
        if (totalPitchIncrementInt == 0 && totalYawIncrementInt == 0)return;
        //move mouse
        display->moveMouseRelative(totalPitchIncrementInt, totalYawIncrementInt);
    }

    [[nodiscard]] bool active() const {
        return cl->FEATURE_AIMBOT_ON
            && localPlayer->isCombatReady()
            && (!cl->AIMBOT_ACTIVATED_BY_BUTTON.empty() && display->keyDown(cl->AIMBOT_ACTIVATED_BY_BUTTON)
            || cl->AIMBOT_ACTIVATED_BY_ADS && localPlayer->inZoom
            || cl->AIMBOT_ACTIVATED_BY_ATTACK && localPlayer->inAttack);
    }

    void assignTarget() {
        for (auto p : *players) {
            if (!p->isCombatReady()) continue;
            if (!p->enemy) continue;
            if (!p->visible) continue;
            if (p->aimedAt) continue;
            if (fabs(p->aimbotDesiredAnglesIncrement.x) > cl->AIMBOT_FOV) continue;
            if (fabs(p->aimbotDesiredAnglesIncrement.y) > cl->AIMBOT_FOV) continue;
            if (target == nullptr || p->aimbotScore > target->aimbotScore) {
                target = p;
                //                target->aimbotLocked = true;
            }
        }
    }

    void releaseTarget() {
        if (target != nullptr && target->isValid())
            target->aimbotLocked = false;
        target = nullptr;
    }

    [[maybe_unused]] void resetLockFlag() const {
        for (auto p : *players) {
            if (!p->isCombatReady()) continue;
            p->aimbotLocked = false;
        }
        if (target != nullptr)
            target->aimbotLocked = true;
    }

    static int roundHalfEven(float x) {
        return (x >= 0.0)
            ? static_cast<int>(std::round(x))
            : static_cast<int>(std::round(-x)) * -1;
    }

    static float atLeast_1_AwayFromZero(float num) {
        if (num > 0) return std::max(num, 1.0f);
        return std::min(num, -1.0f);
    }
};
