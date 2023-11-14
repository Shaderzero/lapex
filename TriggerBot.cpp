#pragma once

struct TriggerBot {
    ConfigLoader* cl;
    XDisplay* display;
    LocalPlayer* localPlayer;
    std::vector<Player*>* players;
    const float TB_MAX_RANGE_ZOOMED = util::metersToGameUnits(100);
    const float TB_MAX_RANGE_HIPFRE = util::metersToGameUnits(10);

    TriggerBot(ConfigLoader* cl, XDisplay* display, LocalPlayer* localPlayer, std::vector<Player*>* players) {
        this->cl = cl;
        this->display = display;
        this->localPlayer = localPlayer;
        this->players = players;
    }

    [[nodiscard]] bool active() const {
        if (!cl->FEATURE_TRIGGERBOT_ON) return false;
        if (!localPlayer->isCombatReady()) return false;

        int weaponId = localPlayer->weaponIndex;
        if (
                weaponId != WEAPON_KRABER &&
                weaponId != WEAPON_P2020 &&
                weaponId != WEAPON_MOZAMBIQUE &&
                weaponId != WEAPON_EVA8 &&
                weaponId != WEAPON_PEACEKEEPER &&
                weaponId != WEAPON_MASTIFF &&
                weaponId != WEAPON_WINGMAN &&
                weaponId != WEAPON_LONGBOW &&
                weaponId != WEAPON_SENTINEL &&
                weaponId != WEAPON_G7 &&
                weaponId != WEAPON_HEMLOCK &&
                weaponId != WEAPON_3030 &&
                weaponId != WEAPON_TRIPLE_TAKE &&
                weaponId != WEAPON_NEMESIS
                ) return false;

        return (!cl->AIMBOT_ACTIVATED_BY_BUTTON.empty() && display->keyDown(cl->TRIGGERBOT_ACTIVATED_BY_BUTTON));
    }

    void shootAtEnemy() {
        if (!active()) return;

        //max range changes based on if we are zoomed in or not
        const float RANGE_MAX = (localPlayer->inZoom) ? TB_MAX_RANGE_ZOOMED : TB_MAX_RANGE_HIPFRE;

        for (auto player : *players) {
            if (!player->isCombatReady()) continue;
            if (!player->enemy) continue;
            if (!player->aimedAt) continue;
            if (player->distanceToLocalPlayer < RANGE_MAX) {
                display->mouseClickLeft();
                //pause to avoid a lot of shots
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                break;
            }
        }
    }
};
