// Detached BSP visibility in the section's local coordinates. GPL-2.0-only.
qboolean CM_SectionPVS(int modelIndex,const vec3_t localView,const vec3_t localPoint){
    const int slot=CM_FindSubBSP(modelIndex);
    if(slot<0||!localView||!localPoint)return qtrue;
    auto* map=&SubBSP[slot];
    if(!map->numSubModels||!map->numNodes||!map->vised||!map->visibility)return qtrue;
    const auto& world=map->cmodels[0];
    const auto cluster=[&](const vec3_t point){
        for(int axis=0;axis<3;++axis)
            if(!(point[axis]>=world.mins[axis]&&point[axis]<=world.maxs[axis]))return -1;
        const int leaf=CM_PointLeafnum_r(point,0,map);
        return leaf>=0&&leaf<map->numLeafs?map->leafs[leaf].cluster:-1;
    };
    const int view=cluster(localView),target=cluster(localPoint);
    // The player's view or a model's authored sample can lie in the host
    // map or in solid space. Leave those cases to the renderer's frustum.
    if(view<0||target<0||view>=map->numClusters||target>=map->numClusters||
       (target>>3)>=map->clusterBytes)return qtrue;
    const auto* row=map->visibility+size_t(view)*map->clusterBytes;
    return row[target>>3]&(1u<<(target&7))?qtrue:qfalse;
}
