#pragma once

#include <string>

#include "progression/GameData.hpp"

namespace sb {

// Plain-text "key value" save file (see Save.cpp). Tolerant of missing and
// unknown keys so old and new builds interoperate.
bool hasSavedGame(const std::string& path);
bool saveGame(const std::string& path, const GameData& data);
bool loadGame(const std::string& path, GameData& data);

// The run in progress, in its own file (2026-10-01): written whenever the map
// opens, so quitting resumes on the map. `run.coreHp / coreMaxHp` carry the
// core. loadRun returns false (and leaves `run` untouched) on a missing,
// unreadable or older-format file.
bool saveRun(const std::string& path, const RunState& run);
bool loadRun(const std::string& path, RunState& run);
void clearRun(const std::string& path);

}  // namespace sb
