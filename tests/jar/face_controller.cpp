// Exercise the production cgame controller with headless skeletal API services.
#include "cgame/cg_headers.h"
#include "test_support.h"
#include <memory>
#include <vector>
#include "../../code/cgame/cg_jar_face.inc"

cg_t cg={};
int iCGResetCount=1;
game_import_t gi={};
vmCvar_t jar_facePreset={},jar_faceSensitivity={};
static float voiceEnergy;
static float TestVoiceEnergy(int entity) { CHECK(entity==0);return voiceEnergy; }
struct TestPool : IGhoul2InfoArray {
    std::vector<CGhoul2Info> models;
    int handle=0;
    int New() override { ++handle;return handle; }
    void Delete(int) override { models.clear(); }
    bool IsValid(int h) const override { return h>0&&h==handle; }
    std::vector<CGhoul2Info>& Get(int) override { return models; }
    const std::vector<CGhoul2Info>& Get(int) const override { return models; }
};
IGhoul2InfoArray& _TheGhoul2InfoArray() { static TestPool pool;return pool; }
static int writes=0;
static const char *names[]={"face","jaw","lblip2","rblip2","ltlip2","rtlip2","ceyebrow"};

int main() {
    return RunTests("facial presets: live update, stock/save cleanup, ownership and missing rigs",[] {
        std::vector<byte> rig(sizeof(mdxaHeader_t)+7*sizeof(int)+7*sizeof(mdxaSkel_t));
        auto *header=reinterpret_cast<mdxaHeader_t *>(rig.data());
        header->numBones=7;header->ofsEnd=int(rig.size());
        auto *offsets=reinterpret_cast<int *>(rig.data()+sizeof(*header));
        for(int i=0;i<7;++i) {
            offsets[i]=7*sizeof(int)+i*sizeof(mdxaSkel_t);
            auto *bone=reinterpret_cast<mdxaSkel_t *>(reinterpret_cast<byte *>(offsets)+offsets[i]);
            std::strcpy(bone->name,names[i]);bone->parent=i?0:-1;
            bone->BasePoseMat.matrix[0][3]=i==2||i==4?0.6f:-0.5f;
            bone->BasePoseMat.matrix[1][3]=i==1?-0.5f:-3.8f;
            bone->BasePoseMat.matrix[2][3]=i==1?60.8f:60.0f;
        }
        gi.JAR_VoiceEnergy=TestVoiceEnergy;
        gi.G2API_GetBoneIndex=[](CGhoul2Info *model,const char *name,qboolean add) {
            int number=-1;for(int i=0;i<7;++i)if(!std::strcmp(name,names[i]))number=i;
            if(number<0)return -1;
            for(int i=0;i<int(model->mBlist.size());++i)if(model->mBlist[i].boneNumber==number)return i;
            if(!add)return -1;
            boneInfo_t bone={};bone.boneNumber=number;model->mBlist.push_back(bone);
            return int(model->mBlist.size())-1;
        };
        gi.G2API_SetBoneAnglesMatrixIndex=[](CGhoul2Info *model,int index,const mdxaBone_t& matrix,int flags,qhandle_t*,int,int) {
            ++writes;auto& bone=model->mBlist.at(index);
            bone.flags=(bone.flags&~BONE_ANGLES_TOTAL)|flags;
            bone.matrix=bone.newMatrix=matrix;return qtrue;
        };
        gi.G2API_StopBoneAnglesIndex=[](CGhoul2Info *model,int index) {
            auto& bone=model->mBlist.at(index);bone.flags&=~BONE_ANGLES_TOTAL;
            return qtrue;
        };
        auto entity=std::unique_ptr<gentity_t>(new gentity_t{});
        auto client=std::unique_ptr<gclient_t>(new gclient_t{});
        entity->client=client.get();entity->health=100;entity->faceBone=0;entity->playerModel=0;
        entity->ghoul2.resize(1);
        auto& model=entity->ghoul2[0];model.aHeader=header;model.currentAnimModelSize=int(rig.size());model.mModel=7;
        centity_t cent={};cent.gent=entity.get();cg.time=1000;
        jar_faceSensitivity.value=1;
        voiceEnergy=1;CG_JAR_ApplyFace(&cent);
        CHECK(writes==0);CHECK(model.mBlist.empty()); // Stock does not create overrides.
        jar_facePreset.integer=1;CG_JAR_ApplyFace(&cent);
        cg.time+=50;CG_JAR_ApplyFace(&cent);CHECK(writes>0);
        int jaw=gi.G2API_GetBoneIndex(&model,"jaw",qfalse);CHECK(jaw>=0);
        float expressive=model.mBlist[jaw].matrix.matrix[2][1];
        jar_facePreset.integer=3;cg.time+=50;CG_JAR_ApplyFace(&cent);
        CHECK(model.mBlist[jaw].matrix.matrix[2][1]>expressive);
        CHECK(model.mBlist[jaw].flags&BONE_ANGLES_JAR_FACE);
        // A pre-existing independent facial override remains untouched.
        int brow=gi.G2API_GetBoneIndex(&model,"ceyebrow",qfalse);
        model.mBlist[brow].flags=BONE_ANGLES_POSTMULT;model.mBlist[brow].matrix.matrix[2][3]=42;
        cg.time+=50;CG_JAR_ApplyFace(&cent);CHECK(model.mBlist[brow].matrix.matrix[2][3]==42);
        // A restored model may retain our flags while the client state is fresh.
        cent={};cent.gent=entity.get();++iCGResetCount;jar_facePreset.integer=0;CG_JAR_ApplyFace(&cent);
        CHECK(!(model.mBlist[jaw].flags&(BONE_ANGLES_TOTAL|BONE_ANGLES_JAR_FACE)));
        CHECK(model.mBlist[brow].flags==BONE_ANGLES_POSTMULT);
        jar_facePreset.integer=2;cg.time+=20;CG_JAR_ApplyFace(&cent);cg.time+=50;CG_JAR_ApplyFace(&cent);
        entity->health=0;CG_JAR_ApplyFace(&cent);CHECK(!(model.mBlist[jaw].flags&BONE_ANGLES_JAR_FACE));
        // If IK/ragdoll takes ownership after our accent, cleanup must not
        // remove its active transform even if the old metadata bit survives.
        model.mBlist[jaw].flags=BONE_ANGLES_JAR_FACE|BONE_ANGLES_IK|BONE_ANGLES_REPLACE;
        CG_JAR_ApplyFace(&cent);
        CHECK(model.mBlist[jaw].flags==(BONE_ANGLES_IK|BONE_ANGLES_REPLACE));
        model.mBlist[jaw].flags=0;
        entity->health=100;voiceEnergy=0;
        for(int i=0;i<50;++i){cg.time+=20;CG_JAR_ApplyFace(&cent);}
        CHECK(jarFaceStates[0].amount<0.001f);
        // Invalid/no facial skeleton must never dereference bogus offsets.
        model.aHeader=nullptr;voiceEnergy=1;cg.time+=20;CG_JAR_ApplyFace(&cent);cg.time+=50;CG_JAR_ApplyFace(&cent);
        model.aHeader=header;offsets[1]=header->ofsEnd;cg.time+=20;CG_JAR_ApplyFace(&cent);
        CHECK(!(model.mBlist[jaw].flags&BONE_ANGLES_JAR_FACE));
    });
}
