// The same marker identifies a City brush and its oriented EV_DEBRIS payload.
// GPL-2.0-only.
#pragma once
#define JAR_MP_CITY_BRUSH_MARKER 249

typedef struct {
    vec3_t origin, axis[3];
} jarCityDebrisFrame_t;

static inline void JAR_MP_CityDebrisFrame(const vec3_t midpoint, const vec3_t angles,
                                         const vec3_t mins, const vec3_t maxs, jarCityDebrisFrame_t *frame) {
    AnglesToAxis(angles, frame->axis);
    VectorCopy(midpoint, frame->origin);
    for (int i = 0; i < 3; ++i)
        VectorMA(frame->origin, -.5f * (mins[i] + maxs[i]), frame->axis[i], frame->origin);
}

static inline void JAR_MP_CityDebrisPoint(const jarCityDebrisFrame_t *frame, vec3_t point) {
    vec3_t local;
    VectorCopy(point, local);
    VectorCopy(frame->origin, point);
    for (int i = 0; i < 3; ++i) VectorMA(point, local[i], frame->axis[i], point);
}
