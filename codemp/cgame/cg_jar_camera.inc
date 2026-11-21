// MP adapter to the same view presets as SP. Local presentation only.
// Native vehicle, zoom, spectator, emplaced and special views retain their path.
// GPL-2.0-only.
#include "qcommon/jar_camera_presets.h"
static int CG_JARCameraPreset(void){
 if(!cg.snap)return 0;
 const playerState_t*p=&cg.predictedPlayerState,*s=&cg.snap->ps;
 int protectedView=com_cameraMode.integer||p->zoomMode||s->zoomMode||p->m_iVehicleNum||s->m_iVehicleNum||p->emplacedIndex||s->emplacedIndex||
  p->pm_type==PM_INTERMISSION||s->pm_type==PM_INTERMISSION||p->pm_type==PM_SPECTATOR||s->pm_type==PM_SPECTATOR||((p->pm_flags|s->pm_flags)&PMF_FOLLOW)||
  ((p->eFlags2|s->eFlags2)&EF2_HELD_BY_MONSTER)||p->fallingToDeath||s->fallingToDeath||
  (cg_thirdPersonSpecialCam.integer&&BG_SaberInSpecial(s->saberMove));
 return JAR_CameraPreset(jar_cameraPreset.integer,protectedView);
}
