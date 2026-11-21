// Typed transient-field fixups shared by native entity saving and regressions.
// GPL-2.0-only. Include after g_local.h.
#pragma once
#include "fields.h"
constexpr save_field_t icarusSaveField={strFOFS(m_iIcarusID),F_INTZERO};
inline bool G_ClearTransientSaveField(const save_field_t *field,void *base){
    auto p=static_cast<byte *>(base)+field->iOffset;
    if(field->eFieldType==F_INTZERO){*reinterpret_cast<int *>(p)=0;return true;}
    if(field->eFieldType==F_NULL){*reinterpret_cast<void **>(p)=nullptr;return true;}
    return false;
}

constexpr save_field_t savefields_gEntity[] =
{
	{strFOFS(client),			F_GCLIENT},
	{strFOFS(owner),			F_GENTITY},
	{strFOFS(classname),		F_STRING},
	{strFOFS(model),			F_STRING},
	{strFOFS(model2),			F_STRING},
//	{strFOFS(model3),			F_STRING}, - MCG
	{strFOFS(nextTrain),		F_GENTITY},
	{strFOFS(prevTrain),		F_GENTITY},
	{strFOFS(message),			F_STRING},
	{strFOFS(target),			F_STRING},
	{strFOFS(target2),			F_STRING},
	{strFOFS(target3),			F_STRING},
	{strFOFS(target4),			F_STRING},
	{strFOFS(targetJump),		F_STRING},
	{strFOFS(targetname),		F_STRING},
	{strFOFS(team),				F_STRING},
	{strFOFS(roff),				F_STRING},
//	{strFOFS(target_ent),		F_GENTITY}, - MCG
	{strFOFS(chain),			F_GENTITY},
	{strFOFS(enemy),			F_GENTITY},
	{strFOFS(activator),		F_GENTITY},
	{strFOFS(teamchain),		F_GENTITY},
	{strFOFS(teammaster),		F_GENTITY},
	{strFOFS(item),				F_ITEM},
	{strFOFS(NPC_type),			F_STRING},
	{strFOFS(closetarget),		F_STRING},
	{strFOFS(opentarget),		F_STRING},
	{strFOFS(paintarget),		F_STRING},
	{strFOFS(NPC_targetname),	F_STRING},
	{strFOFS(NPC_target),		F_STRING},
	{strFOFS(ownername),		F_STRING},
	{strFOFS(lastEnemy),		F_GENTITY},
	{strFOFS(behaviorSet),		F_BEHAVIORSET},
	{strFOFS(script_targetname),F_STRING},
	icarusSaveField,
	{strFOFS(NPC),				F_BOOLPTR},
	{strFOFS(soundSet),			F_STRING},
	{strFOFS(cameraGroup),		F_STRING},
	{strFOFS(parms),			F_BOOLPTR},
	{strFOFS(m_pVehicle),		F_BOOLPTR},

	{NULL, 0, F_IGNORE}
};
