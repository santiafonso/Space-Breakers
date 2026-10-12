#pragma once

#include <string>

namespace sb {

// Open a web page in the player's browser (the menu's "Feedback"). Returns at
// once; false if it couldn't even try. Only ever called with our own URLs.
bool openUrl(const std::string& url);

}  // namespace sb
