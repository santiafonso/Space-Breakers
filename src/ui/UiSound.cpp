#include "ui/UiSound.hpp"

#include "platform/Audio.hpp"

namespace sb::uisound {

namespace {
Audio* gAudio = nullptr;
const void* gOwner = nullptr;   // the group whose item last lit up
int gId = -1;
bool gHot = false;
}  // namespace

void attach(Audio* audio) { gAudio = audio; }

void beginFrame() { gHot = false; }

void hover(const void* owner, int id, bool clicks) {
    if (id < 0) {
        if (owner == gOwner) gId = -1;   // left it: the same item ticks again next time
        return;
    }
    if (clicks) gHot = true;
    if (owner == gOwner && id == gId) return;
    gOwner = owner;
    gId = id;
    if (gAudio) gAudio->uiHover();
}

bool hot() { return gHot; }

}  // namespace sb::uisound
