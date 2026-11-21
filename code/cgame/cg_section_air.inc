// Room-local cold breath follows the rendered instance. GPL-2.0-only.
namespace {
struct RiftAirSpace {
    int modelIndex;
    vec3_t origin;
    vec3_t axis[3];
};
std::vector<RiftAirSpace> riftAirSpaces;

void CG_CollectRiftAirSpaces() {
    riftAirSpaces.clear();
    int modelIndex, modelCount;
    if (!cg.snap || !cgi_CM_BSPModelInfo(jar::rift::resource, &modelIndex, &modelCount) ||
        !cgi_CM_RiftSectionAir(modelIndex, nullptr)) return;
    for (int i = 0; i < cg.snap->numEntities; ++i) {
        auto& cent = cg_entities[cg.snap->entities[i].number];
        const auto& state = cent.currentState;
        if (state.modelindex != modelIndex || state.eType != ET_MOVER ||
            state.solid != SOLID_BMODEL || (state.eFlags & EF_NODRAW) ||
            !cent.gent || !jar::bspSection::Portable(*cent.gent) ||
            std::strcmp(cent.gent->model, jar::rift::resource)) continue;
        CG_CalcEntityLerpPositions(&cent);
        RiftAirSpace room{};
        room.modelIndex = modelIndex;
        VectorCopy(cent.lerpOrigin, room.origin);
        AnglesToAxis(cent.lerpAngles, room.axis);
        riftAirSpaces.push_back(room);
    }
}
}

bool CG_RiftSectionAir(const vec3_t point) {
    if (cg.refdef.rdflags & RDF_SKYBOXPORTAL) return false;
    for (const auto& room : riftAirSpaces) {
        vec3_t offset, localPoint;
        VectorSubtract(point, room.origin, offset);
        for (int i = 0; i < 3; ++i) localPoint[i] = DotProduct(offset, room.axis[i]);
        if (cgi_CM_RiftSectionAir(room.modelIndex, localPoint)) return true;
    }
    return false;
}
