#include "includes.cpp"

ConfigLoader* cl;

XDisplay* display;
Level* level;
LocalPlayer* localPlayer;
std::vector<Player*>* humanPlayers;
std::vector<Player*>* dummyPlayers;
std::vector<Player*>* players;

NoRecoil* noRecoil;
AimBot* aimBot;
TriggerBot* triggerBot;
Sense* sense;

bool global_pause = false;

bool no_recoil_t = false;
bool triggerbot_t = false;
bool aimbot_t = false;
bool players_t = false;
bool local_player_t = false;
bool glow_t = false;

int player_process_counter = 0;

void NoRecoilLoop()
{
    no_recoil_t = true;
    while (no_recoil_t) {
        if (global_pause)
            continue;
        try
        {
            noRecoil->controlWeapon();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        catch (...) {
            printf("!!!Unknown error in NoRecoilLoop!!! SLEEPING 10 SECONDS AND TRYING AGAIN! \n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

void TriggerBotLoop()
{
    triggerbot_t = true;
    while (triggerbot_t) {
        try
        {
            if (global_pause)
                continue;
            triggerBot->shootAtEnemy();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        catch (...) {
            printf("!!!Unknown error in TriggerBotLoop!!! SLEEPING 10 SECONDS AND TRYING AGAIN! \n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

void AimBotLoop()
{
    aimbot_t = true;
    int loop_process_counter = 0;
    while (aimbot_t) {
        try
        {
            if (global_pause)
                continue;
            aimBot->aimAssist();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        catch (...) {
            printf("!!!Unknown error in AimBotLoop!!! SLEEPING 10 SECONDS AND TRYING AGAIN! \n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

void PlayersLoop()
{
    players_t = true;
    while (players_t) {
        if (global_pause)
            continue;
        try
        {
            //record time so we know how long a single loop iteration takes
            long startTime = util::currentEpochMillis();
            if (player_process_counter % 20 == 0) cl->reloadFile();

            player_process_counter++;

            players->clear();
            if (level->trainingArea)
                for (auto p: *dummyPlayers) {
                    p->readFromMemory();
                    if (p->isValid()) players->push_back(p);
                }
            else
                for (auto p : *humanPlayers) {
                    p->readFromMemory();
                    if (p->isValid()) players->push_back(p);
                }

            //check how fast we completed all the processing and if we still have time left to sleep
            int processingTime = static_cast<int>(util::currentEpochMillis() - startTime);
            int goalSleepTime = 6; // 16.67ms=60HZ | 6.97ms=144HZ
            int timeLeftToSleep = std::max(0, goalSleepTime - processingTime);
            std::this_thread::sleep_for(std::chrono::milliseconds(timeLeftToSleep));
            if (player_process_counter > 999) player_process_counter = 0;

            if (player_process_counter % 500 == 0)
                printf("| LOOP[%04d] OK | Processing time: %02dms | Time left to sleep: %02dms | Level: %s |\n",
                       player_process_counter, processingTime, timeLeftToSleep, level->name.c_str());
        }
        catch (std::invalid_argument& e) {
            printf("!!!ERROR in PlayersLoop!!! %s SLEEPING 10 SECONDS AND TRYING AGAIN! \n", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        catch (...) {
            printf("!!!Unknown error in PlayersLoop!!! SLEEPING 10 SECONDS AND TRYING AGAIN! \n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}


void LocalPlayerLoop()
{
    local_player_t = true;
    while (local_player_t) {
        try
        {
            if (global_pause)
                continue;
            localPlayer->readFromMemory();
            if (!localPlayer->isValid()) throw std::invalid_argument("LocalPlayer invalid!");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        catch (std::invalid_argument& e) {
            printf("!!!ERROR in PlayersLoop!!! %s SLEEPING 10 SECONDS AND TRYING AGAIN! \n", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        catch (...) {
            printf("!!!Unknown error in AimBotLoop!!! SLEEPING 10 SECONDS AND TRYING AGAIN! \n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

void GlowLoop()
{
    glow_t = true;
    while (glow_t) {
        try
        {
            if (global_pause)
                continue;
            sense->modifyHighlights();
            sense->glowPlayers();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        catch (...) {
            printf("!!!Unknown error in AimBotLoop!!! SLEEPING 10 SECONDS AND TRYING AGAIN! \n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

int main() {
    //load config
    cl = new ConfigLoader();
    cl->reloadFile();

    //basic checks
    if (getuid()) { std::cout << "RUN AS SUDO!\n"; return -1; }
    if (mem::GetPID() == 0) { std::cout << "OPEN THE GAME FIRST!\n"; return -1; }

    //create basic objects
    display = new XDisplay();
    level = new Level();
    localPlayer = new LocalPlayer();
    humanPlayers = new std::vector<Player*>;
    dummyPlayers = new std::vector<Player*>;
    players = new std::vector<Player*>;

    //fill in slots for players, dummies and items
    for (int i = 0; i < 70; i++) humanPlayers->push_back(new Player(i, localPlayer));
    for (int i = 0; i < 15000; i++) dummyPlayers->push_back(new Player(i, localPlayer));

    //create features     
    noRecoil = new NoRecoil(cl, display, level, localPlayer);
    aimBot = new AimBot(cl, display, level, localPlayer, players);
    triggerBot = new TriggerBot(cl, display, localPlayer, players);
    sense = new Sense(cl, players);

    //create threads for features
    std::thread local_player_thr;
    std::thread aimbot_thr;
    std::thread glow_thr;
    std::thread triggerbot_thr;
    std::thread norecoil_thr;
    std::thread players_thr;

    //begin main loop
    int counter = 0;
    while (1) {
        try {
            //read level and make sure it is playable
            level->readFromMemory();
            if (!level->playable) {
                global_pause = true; // global pause for features in threads
                printf("Waiting for a playable level! Sleeping 10 seconds... \n");
                std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                continue;
            }

            if (global_pause) global_pause = false;

            //run features threads
            if (!local_player_t) {
                local_player_thr = std::thread(LocalPlayerLoop);
                local_player_thr.detach();
            }
            if (!players_t) {
                players_thr = std::thread(PlayersLoop);
                players_thr.detach();
            }
            if (!no_recoil_t) {
                norecoil_thr = std::thread(NoRecoilLoop);
                norecoil_thr.detach();
            }
            if (!aimbot_t) {
                aimbot_thr = std::thread(AimBotLoop);
                aimbot_thr.detach();
            }
            if (!triggerbot_t) {
                triggerbot_thr = std::thread(TriggerBotLoop);
                triggerbot_thr.detach();
            }
            if (!glow_t) {
                glow_thr = std::thread(GlowLoop);
                glow_thr.detach();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (std::invalid_argument& e) {
            printf("!!!ERROR!!! %s SLEEPING 10 SECONDS AND TRYING AGAIN! \n", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        catch (...) {
            printf("!!!UNKNOWN ERROR!!! SLEEPING 10 SECONDS AND TRYING AGAIN! \n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}



