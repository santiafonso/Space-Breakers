#pragma once

#include <filesystem>

namespace sb {

// Directory that holds the running executable. Lets the game find `assets/` and
// write `saves/` next to the binary, so a packaged build works when launched
// from anywhere (double-click on Windows, a menu entry on Linux) instead of
// only from the project root. Falls back to the current directory if the OS
// query fails.
std::filesystem::path exeDir();

}  // namespace sb
