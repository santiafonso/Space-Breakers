#pragma once

#include <string>

#include "progression/GameData.hpp"

namespace sb {

// Plain-text "key value" save file (see Save.cpp). Tolerant of missing and
// unknown keys so old and new builds interoperate.
bool hasSavedGame(const std::string& path);
bool saveGame(const std::string& path, const GameData& data);
bool loadGame(const std::string& path, GameData& data);

}  // namespace sb
