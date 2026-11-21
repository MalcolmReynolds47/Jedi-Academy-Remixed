// POD storage belongs to tr and is cleared by the native map/renderer reset.
// Images belong to the renderer image pool and are reused, never freed while
// a submitted draw command could still reference them. GPL-2.0-only.
struct jarProbeSlot_t {
    jar::reflection::Probe desired;
    unsigned changedAt;
    bool ready;
};
struct jarProbeState_t {
    jar::reflection::Probe pending[jar::reflection::limit];
    jarProbeSlot_t slots[jar::reflection::limit];
    int pendingCount, nativeCount, cursor, activeCount;
    bool pendingSet, initialized, capturing;
    int captureTime;
};
