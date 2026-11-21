// Draw a room-owned backdrop without publishing a world sky camera. GPL-2.0-only.
qboolean CG_DrawSectionSky() {
    if (cg.hyperspace || !cg.snap)
        return qfalse;
    const int delta = cg.nextSnap ? cg.nextSnap->serverTime - cg.snap->serverTime : 0;
    cg.frameInterpolation = delta ? float(cg.time - cg.snap->serverTime) / delta : 0;

    for (int i = 0; i < cg.snap->numEntities; ++i) {
        auto& cent = cg_entities[cg.snap->entities[i].number];
        const auto& state = cent.currentState;
        const int modelIndex=state.modelindex;
        if (modelIndex<=0 || modelIndex>=int(ARRAY_LEN(cgs.inlineDrawModel)) ||
            !cgs.inlineDrawModel[modelIndex] || (state.eFlags & EF_NODRAW) || !CG_SectionSky(cent))
            continue;

        refEntity_t scenery{};
        scenery.hModel = cgs.inlineDrawModel[modelIndex];
        CG_CalcEntityLerpPositions(&cent);
        vec3_t roomAxis[3], offset, localEye, localSky;
        AnglesToAxis(cent.lerpAngles, roomAxis);
        VectorSubtract(cg.refdef.vieworg, cent.lerpOrigin, offset);
        for (int axis = 0; axis < 3; ++axis)
            localEye[axis] = DotProduct(offset, roomAxis[axis]);
        if (!cgi_CM_SectionSky(modelIndex, localEye) ||
            !G_BSPSectionSkyOrigin(*cent.gent, localSky))
            continue;

        refdef_t view = cg.refdef;
        // Draw the sky in the BSP's authored frame. Sky shaders project
        // local brush vertices about this view origin.
        VectorCopy(localSky, view.vieworg);
        for (int row = 0; row < 3; ++row) {
            for (int axis = 0; axis < 3; ++axis)
                view.viewaxis[row][axis] = DotProduct(cg.refdef.viewaxis[row], roomAxis[axis]);
        }
        AxisClear(scenery.axis);
        scenery.renderfx = RF_NOSHADOW | RF_JAR_SECTION_BACKDROP;
        cgi_R_AddRefEntityToScene(&scenery);
        view.rdflags = RDF_NOWORLDMODEL | RDF_SKYBOXPORTAL | RDF_DRAWSKYBOX | RDF_JAR_SECTION_SKY;
        cgi_R_RenderScene(&view);
        cg.refdef.rdflags |= RDF_JAR_SECTION_SKY;
        return qtrue;
    }
    return qfalse;
}

