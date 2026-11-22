// Both multiplayer renderers use the same dimensions as the replicated controller.
#include "../qcommon/jar_portal_size.h"
#include "../../code/rd-common/tr_portal_geometry.inc"

void RE_AddPortalToScene(const jar::portal::View* view) {
 R_SubmitPortablePortal(view, [](const jar::portal::View& v) {
  return JAR_MP_PortalSizeValid(v.width,v.height) && jar::portal::ValidPose(v);
 });
}
