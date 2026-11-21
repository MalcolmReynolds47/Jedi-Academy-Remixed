// Stock panel and settings used by both Sandbox game modes. GPL-2.0-only.
#pragma once

#define JAR_SLIDING_DOOR_ID 100052696u
#define JAR_SLIDING_DOOR_MODEL "models/map_objects/factory/f_door_b.md3"
#define JAR_DOOR_START_SOUND "sound/movers/doors/door1start.wav"
#define JAR_DOOR_MOVE_SOUND "sound/movers/doors/door1move.wav"
#define JAR_DOOR_STOP_SOUND "sound/movers/doors/door1stop.wav"

enum {
 JAR_SLIDE_UP, JAR_SLIDE_LEFT, JAR_SLIDE_RIGHT, JAR_SLIDE_DOWN, JAR_SLIDE_DIRECTIONS,
 JAR_SLIDE_REST = 0, JAR_SLIDE_MOVING, JAR_SLIDE_BLOCKED, JAR_SLIDE_PAUSED,
 JAR_SLIDE_MIN_TRAVEL = 16, JAR_SLIDE_MAX_TRAVEL = 384, JAR_SLIDE_TRAVEL = 176,
 JAR_SLIDE_MIN_SPEED = 25, JAR_SLIDE_MAX_SPEED = 500, JAR_SLIDE_SPEED = 100,
 JAR_SLIDE_MAX_WAIT = 30000, JAR_SLIDE_MILLI = 1000
};

#ifdef __cplusplus
namespace jar { namespace slidingDoor {
struct Settings {
 int travel=JAR_SLIDE_TRAVEL, speed=JAR_SLIDE_SPEED, direction=JAR_SLIDE_UP, waitMs=0;
};
struct State {
 Settings settings;
 int extension=0, remainingWait=0;
 bool open=false, inactive=false;
};
inline bool Valid(const State& state)
{
 const auto& s=state.settings;
 return s.travel>=JAR_SLIDE_MIN_TRAVEL && s.travel<=JAR_SLIDE_MAX_TRAVEL &&
        s.speed>=JAR_SLIDE_MIN_SPEED && s.speed<=JAR_SLIDE_MAX_SPEED &&
        s.direction>=0 && s.direction<JAR_SLIDE_DIRECTIONS &&
        s.waitMs>=0 && s.waitMs<=JAR_SLIDE_MAX_WAIT &&
        state.extension>=0 && state.extension<=s.travel*JAR_SLIDE_MILLI &&
        state.remainingWait>=0 && state.remainingWait<=s.waitMs &&
        (!state.remainingWait || (state.open && state.extension==s.travel*JAR_SLIDE_MILLI));
}
}}
#endif
