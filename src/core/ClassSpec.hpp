#pragma once

#include "progression/Offers.hpp"

namespace sb {

// Fold one of the newer classes' items (at `level`, 1..kMaxItemLevel) into the
// ball's numbers - usually its own mods.cls.<class> block, but it may touch
// any BallMods field (damageMult, radiusMult...). Called by App::ballSpec for
// every item the older-item switch doesn't know. Returns false if `k` isn't a
// class item handled here. One section per class in core/ClassSpec.cpp.
bool foldClassItem(UpgradeKind k, int level, BallMods& m);

}  // namespace sb
