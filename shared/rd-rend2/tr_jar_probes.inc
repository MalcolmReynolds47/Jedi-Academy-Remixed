// Live Sandbox reflection probes. Capture only before a frame accepts scene data,
// using the native six-face world capture and roughness convolution path.
// GPL-2.0-only.
int RE_SetReflectionProbes(const jar::reflection::Probe* probes, int count) {
    using namespace jar::reflection;
    if (count < 0 || (count && !probes)) return Invalid;
    if (count > limit) return Capacity;
    for (int i=0; i<count; ++i) {
        if (!Valid(probes[i])) return Invalid;
        for (int j=0; j<i; ++j) if (probes[i].owner == probes[j].owner) return Invalid;
    }
    if (!tr.registered || !tr.world) return NoWorld;
    if (!r_cubeMapping->integer || !tr.renderCubeFbo[0] || !tr.filterCubeFbo) return Disabled;
    const int nativeCount = tr.jarProbes.initialized ? tr.jarProbes.nativeCount : tr.numCubemaps;
    if (nativeCount < 0 || nativeCount > QSORT_CUBEMAP_MASK-limit) return Capacity;
    auto& state = tr.jarProbes;
    if (count != state.pendingCount || (count && memcmp(state.pending, probes, count*sizeof(*probes)))) {
        if (count) memcpy(state.pending, probes, count*sizeof(*probes));
        state.pendingCount = count;
        state.pendingSet = true;
    }
    int ready = 0;
    for (int i=0; i<count; ++i) for (const auto& slot : state.slots)
        if (slot.ready && SameCapture(slot.desired, probes[i])) { ++ready; break; }
    return ready;
}

static void R_JAR_CommitProbes(unsigned now) {
    using namespace jar::reflection;
    auto& state = tr.jarProbes;
    if (!state.pendingSet) return;
    state.pendingSet = false;
    if (!state.initialized) {
        if (!state.pendingCount) return;
        state.nativeCount = tr.numCubemaps;
#ifdef REND2_SP
        auto* storage = static_cast<cubemap_t*>(Hunk_Alloc((state.nativeCount+limit)*sizeof(cubemap_t), h_low));
#else
        auto* storage = static_cast<cubemap_t*>(ri.Hunk_Alloc((state.nativeCount+limit)*sizeof(cubemap_t), h_low));
#endif
        memset(storage, 0, (state.nativeCount+limit)*sizeof(cubemap_t));
        if (state.nativeCount) memcpy(storage, tr.cubemaps, state.nativeCount*sizeof(cubemap_t));
        tr.cubemaps = storage;
        tr.numCubemaps = state.nativeCount+limit;
        state.initialized = true;
    }
    // Retire absent lifetimes before allocating new ones; list order is immaterial.
    for (auto& slot : state.slots) {
        bool found = false;
        for (int i=0; i<state.pendingCount; ++i) if (slot.desired.owner == state.pending[i].owner) { found=true; break; }
        if (!found) slot = {};
    }
    state.activeCount = state.pendingCount;
    for (int i=0; i<state.pendingCount; ++i) {
        const auto& input = state.pending[i];
        int index = -1;
        for (int j=0; j<limit; ++j) if (state.slots[j].desired.owner == input.owner) { index=j; break; }
        if (index < 0) for (int j=0; j<limit; ++j) if (!state.slots[j].desired.owner) { index=j; break; }
        assert(index >= 0); // complete validated set fits the fixed image budget
        auto& slot = state.slots[index];
        if (!SameCapture(slot.desired, input)) { slot.ready=false; slot.changedAt=now; }
        slot.desired = input;
        // Radius changes affect parallax and influence, not captured pixels.
        tr.cubemaps[state.nativeCount+index].parallaxRadius = input.radius;
    }
}

void R_JAR_ReflectionFrame(stereoFrame_t stereoFrame) {
    using namespace jar::reflection;
    auto& state = tr.jarProbes;
    if (state.capturing || !tr.registered || !tr.world || !r_cubeMapping->integer ||
        stereoFrame == STEREO_RIGHT || r_numentities || r_numpolys || r_numdlights) return;
    const unsigned now = static_cast<unsigned>(ri.Milliseconds());
    // Finish commands referencing old probe origins/images before changing them.
    if (state.pendingSet) { R_IssuePendingRenderCommands(); R_JAR_CommitProbes(now); }
    if (!state.initialized) return;
    for (int n=0; n<limit; ++n) {
        const int index = (state.cursor+n)%limit;
        auto& slot = state.slots[index];
        if (!slot.desired.owner || slot.ready || now-slot.changedAt < settleMilliseconds) continue;
        auto& cubemap = tr.cubemaps[state.nativeCount+index];
        VectorCopy(slot.desired.origin, cubemap.origin);
        cubemap.parallaxRadius = slot.desired.radius;
        const trRefdef_t savedRefdef = tr.refdef;
#ifdef REND2_SP
        matrix_t previousCamera;
        const bool hadCamera = backEndData->currentFrame != nullptr;
        const float previousTime = hadCamera ? backEndData->currentFrame->time : 0;
        if (hadCamera) memcpy(previousCamera, backEndData->currentFrame->viewProjectionMatrix, sizeof(previousCamera));
#endif
        state.capturing = true;
        state.captureTime = savedRefdef.time;
        R_IssuePendingRenderCommands();
        R_InitNextFrame();
        for (int side=0; side<6; ++side) R_RenderCubemapSide(state.nativeCount+index, side, false);
        R_AddConvolveCubemapCmd(&cubemap, state.nativeCount+index);
        R_IssuePendingRenderCommands();
        R_InitNextFrame();
        tr.refdef = savedRefdef;
#ifdef REND2_SP
        // Outer BeginFrame will make the final capture buffer previousFrame.
        // Preserve the real camera for temporal reprojection. Model/bone cache
        // offsets from before capture are deliberately not reused: those GPU
        // buffers have cycled. Six faces preserve the temporal jitter parity.
        if (hadCamera && backEndData->currentFrame) {
            memcpy(backEndData->currentFrame->viewProjectionMatrix, previousCamera, sizeof(previousCamera));
            backEndData->currentFrame->time = previousTime;
        }
#endif
        state.capturing = false;
        slot.ready = cubemap.image != nullptr;
        // A failed allocation is retried after the same quiet interval.
        slot.changedAt = now;
        state.cursor = (index+1)%limit;
        break; // Never rebuild all probes in one frame.
    }
}

int R_JAR_ProbeForPoint(const vec3_t point) {
    const auto& state = tr.jarProbes;
    if (!r_cubeMapping->integer || !state.initialized || !state.activeCount || state.capturing) return 0;
    int best = 0;
    float nearest = (float)WORLD_SIZE*(float)WORLD_SIZE;
    for (int i=0; i<jar::reflection::limit; ++i) {
        const auto& slot = state.slots[i];
        if (!slot.desired.owner || !slot.ready) continue;
        const auto& probe = tr.cubemaps[state.nativeCount+i];
        vec3_t delta; VectorSubtract(point, probe.origin, delta);
        const float distance = DotProduct(delta, delta);
        if (distance <= probe.parallaxRadius*probe.parallaxRadius && distance < nearest) {
            nearest = distance; best = state.nativeCount+i+1;
        }
    }
    return best;
}

int R_JAR_ProbeForSurface(const msurface_t* surface, const trRefEntity_t* entity, int entityNum) {
    if (!tr.jarProbes.activeCount) return surface->cubemapIndex;
    vec3_t point;
    if (surface->cullinfo.type & CULLINFO_SPHERE) VectorCopy(surface->cullinfo.localOrigin, point);
    else if (surface->cullinfo.type & CULLINFO_BOX)
        for (int i=0; i<3; ++i) point[i] = (surface->cullinfo.bounds[0][i]+surface->cullinfo.bounds[1][i])*0.5f;
    else return surface->cubemapIndex;
    if (entity && entityNum != REFENTITYNUM_WORLD) {
        vec3_t world;
        for (int i=0; i<3; ++i) world[i] = entity->e.origin[i] + point[0]*entity->e.axis[0][i] + point[1]*entity->e.axis[1][i] + point[2]*entity->e.axis[2][i];
        VectorCopy(world, point);
    }
    const int live = R_JAR_ProbeForPoint(point);
    return live ? live : surface->cubemapIndex;
}
