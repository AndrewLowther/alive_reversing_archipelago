#pragma once

#include "../AliveLibCommon/FunctionFwd.hpp"
#include "../AliveLibCommon/relive_config.h"

#if ARCHIPELAGO_CLIENT

enum class LevelIds : s16;

namespace Archipelago {

  // Public methods
  void Tick();
  int ItemCount(const char* name);
  void OnMudokonRescued(LevelIds level, s16 path, s8 zulag, s16 total, s32 id);
  void OnAbeEnteredPortal(LevelIds level, s16 path, s16 camera);
  void GoalComplete(LevelIds level, s16 path, s16 camera);
  void OnDoorUse(LevelIds level, s16 path, s8 zulag, s16 total, s32 id);
  bool IsLevelGateUnlocked(const char* name);
  bool FinalDoorOpen();

}

#endif