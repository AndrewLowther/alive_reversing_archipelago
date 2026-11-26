#include "Archipelago.hpp"
#include "logger.hpp"
#include "Abe.hpp"
#include "PathData.hpp"
#include "BaseBomb.hpp"
#include "stdlib.hpp"

#include <cstdio>
#include <set>
#include <string>
#include <iostream>
#include <chrono>

#if ARCHIPELAGO_CLIENT

namespace {
    const char* kEventLogFile = "archipelago_events.jsonl";
    const char* kUnlocksFile = "archipelago_unlocks.txt";
    const char* kEventsFile = "archipelago_events.txt";

    std::map<std::string, int> unlocks;
    int frameCount = 0;
    int sMudokonsRequired = 0;
    bool sAbeWasDead = false;
    std::time_t lastDeathTime = 0;
    static long long sLastKillHandled = 0;

    bool AbeIsDead()
    {
        return sActiveHero_5C1B68 && sActiveHero_5C1B68->field_106_current_motion == eAbeMotions::Motion_57_Dead_4589A0;
    }

    bool InGameplay()
    {
        const LevelIds level = gMap_5C3030.field_0_current_level;
        return sActiveHero_5C1B68
            && level != LevelIds::eMenu_0
            && level != LevelIds::eCredits_16
            && level != LevelIds::eNone;
    }
}

namespace Archipelago {

    void WriteEventLog(const char* event)
    {
        if (FILE* file = std::fopen(kEventLogFile, "ab"))
        {
            fprintf(file, "%s\n", event);
            fclose(file);
        }
    }

    int ItemCount(const char* name)
    {
        const auto it = unlocks.find(name);
        return it == unlocks.end() ? 0 : it->second;
    }

    void BuildKeySet()
    {
        std::map<std::string, int> fresh;
        int freshRequired = 0;
        if (FILE* file = std::fopen(kUnlocksFile, "r"))
        {
            char line[256];
            while (std::fgets(line, sizeof(line), file))
            {
                std::string name = line;
                const auto first = name.find_first_not_of(" \t\r\n");
                if (first == std::string::npos) {
                    continue;
                }
                // Check for a mudokons_required line, it will have an =
                if (name.find('=') != std::string::npos)
                {
                    const auto pos = name.find('=');
                    if (pos != std::string::npos)
                    {
                        const std::string key = name.substr(0, pos);
                        const std::string value = name.substr(pos + 1);
                        if (key == "mudokons_required")
                        {
                            freshRequired = std::stoi(value);
                        }
                    }
                    continue;
                }
                name = name.substr(first, name.find_last_not_of(" \t\r\n") - first + 1);
                if (fresh.find(name) != fresh.end()) {
                    fresh[name]++;
                } else {
                    fresh[name] = 1;
                }
            }
            std::fclose(file);
        }
        if (fresh != unlocks)
        {
            unlocks.swap(fresh);
            LOG_INFO("Unlocks changed (" << unlocks.size() << ")");
        }
        if (freshRequired != sMudokonsRequired)
        {
            sMudokonsRequired = freshRequired;
            LOG_INFO("Mudokons required changed (" << sMudokonsRequired << ")");
        }
    }

    bool DeathlinkReceived()
    {
        // Implement the logic to check if a deathlink has been received in the last 15 seconds
        // 
        if (FILE* file = std::fopen(kEventsFile, "rb"))
        {
            char line[256];
            std::time_t t;
            while (std::fgets(line, sizeof(line), file))
            {
                std::sscanf(line, "kill %lld", &t);
                const long long nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                if (t > sLastKillHandled &&  nowMs - t <= 15000)
                {
                    std::fclose(file);
                    sLastKillHandled = t;
                    return true;
                }
            }
            std::fclose(file);
        }
        return false;
    }

    void Tick()
    {
        frameCount++;

        // Check to see if Abe is dead
        const bool dead = AbeIsDead();

        if (frameCount == 30)
        {
            BuildKeySet();

            // Check for a deathlink in the last 15 seconds, ensure Abe is not dead
            // and ensure we're in a level
            if (DeathlinkReceived() && !dead && InGameplay()) 
            {
                if (auto* pBomb = ae_new<BaseBomb>())
                {
                    pBomb->ctor_423E70(sActiveHero_5C1B68->field_B8_xpos, sActiveHero_5C1B68->field_BC_ypos, 0, sActiveHero_5C1B68->field_CC_sprite_scale);
                }
            }

            // Reset to zero so the frame count never gets huge during a long session
            frameCount = 0;
        }

        if (dead && !sAbeWasDead && InGameplay()) {
            char buffer[160] = {};

            std::snprintf(
                buffer,
                sizeof(buffer),
                "{\"event\": \"abe_died\"}"
            );
            WriteEventLog(buffer);
        }
        sAbeWasDead = dead;
    }

    void OnMudokonRescued(LevelIds level, s16 path, s8 zulag, s16 total, s32 id)
    {
        char buffer[160] = {};

        std::snprintf(
            buffer,
            sizeof(buffer),
            "{\"event\": \"rescue\", \"level\": %u, \"path\": %d, \"zulag\": %d, \"total\": %d, \"id\": %d}",
            static_cast<int>(level), path, zulag, total, id
        );

        WriteEventLog(buffer);
    }

    void OnAbeEnteredPortal(LevelIds level, s16 path, s16 camera)
    {
        char buffer[160] = {};

        std::snprintf(
            buffer,
            sizeof(buffer),
            "{\"event\": \"abe_entered_portal\", \"level\": %u, \"path\": %d, \"camera\": %d}",
            static_cast<int>(level), path, camera
        );

        WriteEventLog(buffer);
    }

    void GoalComplete(LevelIds level, s16 path, s16 camera)
    {
        char buffer[160] = {};

        std::snprintf(
            buffer,
            sizeof(buffer),
            "{\"event\": \"goal_completed\", \"level\": %u, \"path\": %d, \"camera\": %d}",
            static_cast<int>(level), path, camera
        );

        WriteEventLog(buffer);
    }

    bool IsLevelGateUnlocked(const char* name)
    {
        return unlocks.find(name) != unlocks.end();
    }

    bool FinalDoorOpen()
    {
        return sMudokonsRequired <= 0 || ItemCount("Mudokon Saved") >= sMudokonsRequired;
    }

}

#endif