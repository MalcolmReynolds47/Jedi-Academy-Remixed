#include "tr_portal_geometry.inc"

void RE_AddPortalToScene(const jar::portal::View* view) {
 R_SubmitPortablePortal(view, [](const jar::portal::View& v){return jar::portal::Valid(v);});
}
