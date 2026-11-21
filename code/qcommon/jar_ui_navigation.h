// Menu-only joystick navigation. GPL-2.0-only.
#ifndef JAR_UI_NAVIGATION_H
#define JAR_UI_NAVIGATION_H
#include "client/keycodes.h"
namespace jar {
inline int MenuNavigationKey(int key, bool bindingPending) {
    // Binding capture must receive the actual physical key, including buttons
    // otherwise used for confirm/back. Gameplay never calls this adapter.
    if (bindingPending) return key;
    switch (key) {
        case A_JOY1: return A_ENTER;
        case A_JOY2: return A_ESCAPE;
        case A_JOY28: return A_CURSOR_UP;
        case A_JOY29: return A_CURSOR_RIGHT;
        case A_JOY30: return A_CURSOR_DOWN;
        case A_JOY31: return A_CURSOR_LEFT;
        default: return key;
    }
}
}
#endif
