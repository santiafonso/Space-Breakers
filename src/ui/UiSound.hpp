#pragma once

namespace sb {

class Audio;

// Shared UI feedback. Widgets report what is under the pointer once a frame;
// a soft tick plays when it moves onto something new, and the App plays the
// click when a press lands while something clickable is hot - so screens never
// call the audio for plain hover / click themselves.
namespace uisound {

void attach(Audio* audio);
// Start of a frame's screen update: nothing is hot until a widget says so.
void beginFrame();
// `owner` = the widget group (a menu, a screen), `id` = the item under the
// pointer (-1 none). `clicks` = false when the item plays its own sound on
// press (the Sound screen's previews), so the generic click stays out.
void hover(const void* owner, int id, bool clicks = true);
// Something that takes a click is under the pointer (set by hover this frame).
bool hot();

}  // namespace uisound
}  // namespace sb
