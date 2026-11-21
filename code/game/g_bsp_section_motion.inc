// Included after the portable bobbing implementation. GPL-2.0-only.
#include "g_bsp_section_motion.h"
#include "g_mover_motion.h"
#include "g_subbsp_ownership.h"
#include "g_bsp_section_wasteland_movers.h"
#include <algorithm>

// pos1/pos2 identify the frame where the children currently live. A force
// may take several substeps before setupMoved publishes the new frame.
static bool BSPSectionContains(const gentity_t& root, const gentity_t& child,
                               const vec3_t origin, const vec3_t excursion) {
    vec3_t section[3], body[3], offset;
    AnglesToAxis(root.pos2, section);
    AnglesToAxis(child.currentAngles, body);
    VectorSubtract(origin, root.pos1, offset);
    for (int i = 0; i < 3; ++i) {
        const float center = DotProduct(section[i], offset);
        const float travel = std::fabs(DotProduct(section[i], excursion));
        float low = center - travel, high = center + travel;
        for (int a = 0; a < 3; ++a) {
            const float basis = DotProduct(section[i], body[a]);
            low += basis * (basis >= 0 ? child.mins[a] : child.maxs[a]);
            high += basis * (basis >= 0 ? child.maxs[a] : child.mins[a]);
        }
        if (!(low >= root.mins[i] && high <= root.maxs[i])) return false;
    }
    return true;
}

static bool BSPSectionUncontrolled(const gentity_t& child) {
    if (child.s.eFlags & EF_FORCE_GRIPPED || child.next_roff_time ||
        !jar::angular::Stationary(child.s.apos.trType) ||
        G_JAR_MoverTranslationActive(child)) return false;
    if((child.flags&FL_TEAMSLAVE)||child.teamchain){
        const auto* room=jar::bspSection::SectionLiftRoot(child);
        if(!room||jar::bspSection::Resource(room->model)->mode!=jar::bspSection::Mode::Wasteland||
           !jar::wasteland::DoorTeamMember(*room,child))return false;
    }

    JarMoverMotion motion;
    return !G_JAR_MoverMotionRead(child, motion) || VectorLengthSquared(motion.angular) == 0;
}

static bool BSPSectionBobbingFits(const gentity_t& root, const gentity_t& child) {
    if (!jar::bspSection::Bobbing(child) ||
        (child.s.pos.trType != TR_SINE && child.s.pos.trType != TR_INTERPOLATE) ||
        child.s.pos.trDuration <= 0 || !std::isfinite(child.radius) ||
        !BSPSectionUncontrolled(child)) return false;

    const float* excursion = child.s.pos.trType == TR_SINE ? child.s.pos.trDelta : vec3_origin;
    return BSPSectionContains(root, child, child.s.origin, child.s.pos.trDelta) &&
           BSPSectionContains(root, child, child.s.pos.trBase, excursion) &&
           BSPSectionContains(root, child, child.currentOrigin, vec3_origin);
}

static bool BSPSectionBreakableFits(const gentity_t& root, const gentity_t& child) {
    return G_BSPSectionBreakableRoot(child) == &root && child.bmodel &&
           child.s.eType == ET_MOVER && (child.e_DieFunc == dieF_funcBBrushDie ||
                                        (child.e_DieFunc == dieF_funcGlassDie && jar::bspSection::SectionGlass(child))) &&
           child.s.pos.trType == TR_STATIONARY && BSPSectionUncontrolled(child) &&
           BSPSectionContains(root, child, child.currentOrigin, vec3_origin) &&
           BSPSectionContains(root, child, child.s.pos.trBase, vec3_origin);
}

static bool BSPSectionLiftFits(const gentity_t& root, const gentity_t& child) {
    return jar::bspSection::SectionLift(child) && child.s.eType == ET_MOVER &&
           (child.s.pos.trType == TR_STATIONARY || child.s.pos.trType == TR_LINEAR_STOP ||
            child.s.pos.trType == TR_NONLINEAR_STOP) &&
           BSPSectionUncontrolled(child) &&
           BSPSectionContains(root, child, child.currentOrigin, vec3_origin) &&
           BSPSectionContains(root, child, child.pos1, vec3_origin) &&
           BSPSectionContains(root, child, child.pos2, vec3_origin);
}

static bool BSPSectionGunFits(const gentity_t& root,const gentity_t& child){
    return jar::wasteland::GunEntity(child)&&G_SubBSPOwner(&child)==&root&&!child.activator&&!child.nextTrain&&
           child.s.pos.trType==TR_STATIONARY&&BSPSectionUncontrolled(child)&&
           BSPSectionContains(root,child,child.currentOrigin,vec3_origin);
}

static bool BSPSectionTrainFits(const gentity_t& root,const gentity_t& child) {
    return jar::bspSection::SectionTrainRoot(child)==&root&&child.s.eType==ET_MOVER&&
           (child.s.pos.trType==TR_STATIONARY||child.s.pos.trType==TR_LINEAR_STOP||child.s.pos.trType==TR_NONLINEAR_STOP)&&
           BSPSectionUncontrolled(child)&&child.nextTrain&&jar::bspSection::SectionTrainCorner(*child.nextTrain,root)&&
           BSPSectionContains(root,child,child.currentOrigin,vec3_origin)&&
           BSPSectionContains(root,child,child.pos1,vec3_origin)&&BSPSectionContains(root,child,child.pos2,vec3_origin);
}

BSPSectionMotionScope::BSPSectionMotionScope(const gentity_t& root) {
    if (!jar::bspSection::Portable(root)) return;
    std::vector<gentity_t*> members, solids;
    if (!G_SubBSPMembers(&root, members)) { allowed = false; return; }
    for (auto* child : members) {
        if (!(child->contents & (MASK_SOLID | CONTENTS_BODY))) continue;
        if (!BSPSectionBobbingFits(root, *child) && !BSPSectionBreakableFits(root, *child) &&
            !BSPSectionLiftFits(root, *child) && !BSPSectionTrainFits(root,*child)&&
            !BSPSectionGunFits(root,*child)) { allowed = false; return; }
        solids.push_back(child);
    }
    if (solids.empty()) return;
    const auto riderOnChild = [&](int ground) {
        return ground >= 0 && ground < ENTITYNUM_WORLD &&
               std::find(solids.begin(), solids.end(), &g_entities[ground]) != solids.end();
    };
    for (int n = 0; n < globals.num_entities; ++n) {
        const auto& entity = g_entities[n];
        if (entity.inuse && (riderOnChild(entity.s.groundEntityNum) ||
            (entity.client && riderOnChild(entity.client->ps.groundEntityNum)))) {
            allowed = false;
            return;
        }
    }
    // Finish all refusal checks before changing any link state.
    for (auto* child : solids) if (child->linked) unlinked.push_back(child);
    for (auto* child : unlinked) gi.unlinkentity(child);
}

BSPSectionMotionScope::~BSPSectionMotionScope() {
    for (auto* child : unlinked) if (child->inuse && !child->linked) gi.linkentity(child);
}
