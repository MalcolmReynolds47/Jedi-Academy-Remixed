// Draw the native server's clipped searchlight with the stock cone material.
static void SearchlightDraw(const centity_t *cent) {
    const entityState_t *state = &cent->currentState;
    if ((state->eFlags & EF_NODRAW) || !state->time) return;
    const unsigned packed = (unsigned)state->time;
    const unsigned angle = packed >> 24;
    if (angle < 0 || angle > JAR_MP_CONE_MAX ||
        state->eventParm < 0 || state->eventParm > USHRT_MAX) return;
    vec3_t forward, start, right, up;
    VectorSubtract(state->origin, cent->lerpOrigin, forward);
    const float distance = VectorNormalize(forward);
    if (distance <= 0 || distance > WORLD_SIZE + 128) return;
    VectorMA(cent->lerpOrigin, 12, forward, start);
    PerpendicularVector(right, forward);
    CrossProduct(forward, right, up);
    const float width = (distance - 12) * tanf(angle * (M_PI / 180.f));
    const qhandle_t shader = trap->R_RegisterShader(JAR_SEARCHLIGHT_SHADER);
    enum { CONE_SIDES = 12 };
    vec3_t ring[CONE_SIDES];
    for (int i = 0; i < CONE_SIDES; ++i) {
        const float radians = i * (2.f * M_PI / CONE_SIDES);
        VectorScale(right, cosf(radians), ring[i]);
        VectorMA(ring[i], sinf(radians), up, ring[i]);
    }
    polyVert_t quads[CONE_SIDES][4] = {0};
    for (int i = 0; i < CONE_SIDES; ++i) {
        polyVert_t *quad = quads[i];
        for (int v = 0; v < 4; ++v) {
            const int far = v == 1 || v == 2;
            const int j = (i + (v >= 2)) % CONE_SIDES;
            VectorMA(far ? state->origin : start, far ? width : 2.f, ring[j], quad[v].xyz);
            quad[v].st[0] = far ? 1.f : 0.f;
            quad[v].st[1] = (i + (v >= 2)) / (float)CONE_SIDES;
            for (int c = 0; c < 3; ++c) quad[v].modulate[c] = (((packed >> (8 * (2 - c))) & 255) * 46) / 255;
            quad[v].modulate[3] = 255;
        }
    }
    if (shader) trap->R_AddPolysToScene(shader, 4, &quads[0][0], CONE_SIDES);
    if (state->eventParm) {
        vec3_t light;
        VectorMA(state->origin, -fminf(5.f, (distance - 12) * .5f), forward, light);
        trap->R_AddLightToScene(light, state->eventParm,
                               ((packed >> 16) & 255) / 255.f, ((packed >> 8) & 255) / 255.f,
                               (packed & 255) / 255.f);
    }
}
