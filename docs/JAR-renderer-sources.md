# JA:R 0.2.0 renderer sources and integration

The original OpenJK engine, game and Classic renderer remain based on
[JACoders/OpenJK](https://github.com/JACoders/OpenJK) commit
`1a6a643427aa347553e9073dac5570b33337c4d9`.

The optional Rend2 renderer and supporting SP API changes are taken from
[SomaZ/OpenJK, rend2-unified-wip](https://github.com/SomaZ/OpenJK/tree/788c19e97f02c0b68db29d63060305b6ad1580e5),
commit `788c19e97f02c0b68db29d63060305b6ad1580e5` (August 20, 2026).
The upstream [single-player port proposal](https://github.com/JACoders/OpenJK/pull/1179)
credits AntiAnti for the initial SP port. Rend2 builds on the OpenJK/ioquake3
renderer and the work of its contributors. This is an experimental upstream
branch, adapted for JA:R; it is not a claim that this port has been accepted into
the main OpenJK branch.

Imported source lives in `code/rd-rend2`, `shared/rd-rend2`, and `shared/KHR`.
Supporting changes adapt SP refentities, renderer imports/exports, UI polygon
submission, model definitions and the SDL framebuffer setup. Existing source
copyright and license notices have been retained. The engine's GPL v2 license
is included in the release as `JediAcademyRemixed/LICENSE.txt`.

The imported renderer includes MikkTSpace (Morten S. Mikkelsen; license in
`shared/rd-rend2/MikkTSpace/mikktspace.h`), SMAA (Jorge Jimenez, Jose I.
Echevarria, Belen Masia, Fernando Navarro and Diego Gutierrez; notices in
`shared/rd-rend2/tr_smaa.h` and GLSL source), stb_image (Sean Barrett and
contributors; dual MIT/public-domain license in `shared/rd-rend2/stb_image.h`),
and Khronos OpenGL headers (notices in `shared/KHR/khrplatform.h` and
`shared/rd-rend2/glext.h`). These library/shader license notices remain in their
original source files.

JA:R's changes to the donor include:

- 64-bit draw keys with 15-bit renderer entity IDs and sorting across all bytes.
- 1,048,576 draw surfaces, 32,768 bounded skeletal surface records and 32,768
  model handles, with checked attachment packing and safe handle rollover.
- Larger polygon and GPU constant buffers, checked writes, and an aligned
  frame allocator that reports exhaustion instead of returning a pointer that
  a caller would dereference. Model/draw overflow drops new cosmetic submissions;
  indispensable allocator exhaustion raises a recoverable engine error.
- Unique `rdjar-rend2_x86_64.dll` naming and private installation. Generic
  OpenJK/EternalJK renderer DLLs cannot substitute for the JA:R API.
- A JA:R menu with 32 controls, three staged presets, Classic recovery selection
  and optional distant idle NPC decision scheduling. These are JA:R additions.

The menu's new controls use original code and existing registered fonts; no
retail menu files are replaced or redistributed. Renderer API 1019, game API
1011 and cgame import API 5 require matching JA:R binaries. Save version 101 and
local protocol 1040 are unchanged from 0.1.1.

[TaystJK](https://taysta.github.io/TaystJK/) and
[EternalJK](https://github.com/JKSunny/EternalJK) were examined as other renderer
donors. Their multiplayer-focused Vulkan paths would require a separate SP
port. No Vulkan renderer or code from these projects is included in this release.

The release retains Classic as the default. Rend2 requires a compatible modern
OpenGL driver (the donor requests a 3.2 core context plus required extensions).
Both DLLs pass native loading/API/handle-allocation tests. GPU rendering, campaign
behavior and frame-rate improvements have not been measured by those tests.
