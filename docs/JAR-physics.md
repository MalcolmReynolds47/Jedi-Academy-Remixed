# Prop physics and tools (0.48.0)

## Single-player Box3D under load

JA:R 1.148 keeps the 120 Hz fixed step and runs Box3D on up to eight
workers, depending on CPU availability and how many bodies are awake.
Small scenes use one worker. The native bridge reuses its current frame's
prop set for proxy selection and avoids repeated ownership scans for
prop rotation and actor response.

On the development machine, the median of three 95th-percentile step times
for 4,096 initially awake 47-vertex props fell from 10.66 ms to 2.30 ms.
Each run simulated 180 steps with the same geometry. The final poses and
velocities matched across all runs. The benchmark measures the solver
in isolation. In-game frame timing and stress acceptance remain open.

## Rope collision with moving bodies

In 1.89.0, SP and MP route taut ropes around collision hulls, including
moving props and characters. The route cache queries the bounds of each rope
so motion elsewhere can leave it intact. If a body overlaps an anchor and
the solver cannot find a clear path, tension waits until the path clears.
Its visible slack clips to nearby collision, and MP draws the server chord
if local interpolation would otherwise hide that span.

In 1.90.0, a taut rope also pulls at its bends when they press against a
movable Sandbox prop. Box3D uses the prop's mass and inertia to apply the
load at each contact. Frozen or held props and characters can guide the
route while remaining under their original controllers. MP uses the server's
edit policy for the contacted prop. Contact points are recalculated from
the route and current geometry after reload.

## Latest installed release: 1.43.0 (2026-10-12)

Build `20261012-081332-d0df7779` installed at `2026-10-12T08:17:36.3584076-04:00` through
scripts/build-jar.ps1. All 145 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
70 installed files, 20 replacement backups and 2059 archived sources.
All five engine/launcher executable resources report version 1.43.0.0.
ZIP SHA-256: `C46D244051F82E569854E008A1FDA30CE6B67F43F4B65FEE377EBC0F059FBFEA`.
Receipt: `build/deploy/20261012-081727-359a56a4/receipt.json`.
Qualification: `build/mp-ropes-1.43.0/qualification.json`.

MP ropes now share the server's scenery bends with client rendering and use
complete-path tension. JRW18 saves retain bends; copies reroute with complete
rollback on failure. Blocked saves preserve the previous journal. Protocol
1051/game API 6/cgame API 8/feature 54 require matching updated clients and
servers. The native loopback protocol/mismatch/two-bot check also passed.
The first full attempt exposed three old-header assertions in smoke readers;
they were updated and the full pipeline rerun successfully. The shared journal
inspection helper now reads JRW18 while retaining the older stable prefix.

Terrain is hidden with explicit owner approval. SP has 1,174 cards, zero listed
unavailable cards and 29 approved exclusions; MP retains its 1,039 choices.
Other catalog rows and asset packs are byte-identical to 1.42.0. SP save/API
contracts are unchanged. Settings, saves and other mods were preserved.

No graphical game or human remote client was launched. Dynamic cable contacts,
distributed rope mass, complex concave routes and rendered/performance/remote
acceptance remain open. MP Rend2, controller/teleport prediction parity and
three legacy pickup resource adapters also remain open. The goal stays active.
Next public release: 1.44.0. Receipt notes were added after packaging; runtime
sources remain exact.

## 1.43.0: MP scenery ropes and approved terrain exclusion (2026-10-12)

Multiplayer ropes now use the shared scenery path solver on the server. Up to
eight bends determine complete-path tension and the visible client mesh. The
server sends bends and endpoints in one snapshot; clients do not independently
choose routes. Wrapped ropes release the straight Box3D joint, and clear paths
become eligible for that joint again. Both attached bodies are excluded from
route sweeps. World anchors retain surface normals and account for clearance.

Unchanged routes are cached. Each solve allows 512 hull sweeps, with a shared
4096-sweep frame budget. Blocked or deferred routes stop eligible motion,
attempt a swept recovery and suppress invalid force/geometry with a status
message. Occupied vehicles stop without relocating their riders independently.
Suspended routes also release their straight rigid constraint.

JRW18 persists bends and reads JRW1-17. Saves reject obstructed routes before
writing a journal. Copies reroute at their destination and roll back the whole
assembly if admission fails. Snapshot fixtures cover late-join baselines,
changed bends, unwrapping, invalid states and reused slots; all 128 eight-bend
attachments fit one 32768-byte native message. Production server fixtures
cover actual Box3D impulses, cache invalidation, recovery, persistence and copy
rollback. Client mesh fixtures cover replicated bends and invalid input.

MP protocol is now 1051, game API 6, cgame API 8 and feature 54. Matching updated
clients and servers are required. Renderer API 12, SP APIs 1024/1023 and SP
save V44/workshop wire 8 are unchanged. The shared geometry helper accepts the
existing MP 131072-unit maximum; SP record/admission limits remain unchanged.

The owner explicitly approved hiding the disabled terrain entry on 2026-10-12.
SP now has 1,174 cards, zero listed unavailable cards and 29 approved exclusions
(21 empty helpers, seven missing-content entries and terrain). Of the original
182 unavailable entries, 154 have implemented routes and 28 were excluded by
the owner; all 211 entity classes are accounted for as 204 routed and seven
excluded. Existing card IDs are preserved. MP stays at 1,039 choices.

These are implementation and automated-test claims. Rendered SP/MP wrapping,
real remote synchronization, save/load feel and frame-time acceptance remain
open. Dynamic prop/actor cable contacts, distributed rope mass and complex
concave routing remain incomplete. MP Rend2, remaining controllers/teleport
prediction and broader multiplayer parity also remain open. Three MP legacy
pickup resources remain open and have not been excluded. The goal stays active.
Build/package/install: scripts/build-jar.ps1. Evidence: build/mp-ropes-1.43.0/.

## Latest installed release: 1.42.0 (2026-10-12)

Build `20261012-074755-f817edfd` installed at `2026-10-12T07:52:15.3544420-04:00` through
scripts/build-jar.ps1. All 145 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
70 installed files, 16 replacement backups and 2057 archived sources.
All five engine/launcher executable resources report version 1.42.0.0.
ZIP SHA-256: `46B870141FAF4CE0FF782BBA904B094ABDDDAA434C64EE5D3530E01BFD65CD24`.
Receipt: `build/deploy/20261012-075205-b71c5bd2/receipt.json`.
Qualification: `build/rope-integration-1.42.0/qualification.json`.

SP ropes now route around solid scenery and use those bends for both tension
and visible geometry. Full saves retain bends; copies reroute at the destination.
Blocked routes stop eligible motion, attempt a swept recovery and report the
obstruction. New save V44/workshop wire 8 retain legacy reads. The full suite
caught and verified the required V44 vehicle-registry writer/reader update.

MP gameplay and assets remain unchanged: 1,039 entries, feature 53, protocol
1050 and JRW17. Authoritative MP bends, world persistence and replicated rope
rendering are still required. Shared native SP/MP brush fixtures are geometry
evidence only. Dynamic prop/actor cable contacts, distributed rope mass and
complex concave routes remain open. Settings, saves and other mods were preserved.

No graphical game or human remote client was launched. SP wrapping, quicksave/
load and frame-time acceptance still need gameplay evidence. MP Rend2, remaining
controllers/teleport prediction and remote/load/rendered acceptance remain open.
The goal stays active. Next public release: 1.43.0. Receipt notes were added
after packaging; runtime sources remain exact.

## 1.42.0: SP ropes route around solid scenery (2026-10-12)

Single-player Rope now shares one authoritative scenery path between its force
solver and visible mesh. Up to eight saved bends route around solid brush
geometry; clear shortcuts unwrap. Tension uses the complete path length and
the first/last segment directions, including mixed native/Box3D endpoints.
Wrapped ropes disable the straight Box3D distance constraint, which becomes
eligible again after unwrapping. World anchors account for hull clearance.

Each solve is bounded to 512 hull sweeps, with a shared 4096-sweep budget per
game frame and cached unchanged routes. Scenery changes invalidate the cache.
Blocked/busy routes stop eligible endpoint motion and attempt a swept return
to the previous accepted pose. An invalid route supplies no force or misleading
straight mesh; a status message explains how to release it. Attached bodies are
temporarily excluded from each route query and their link state is restored.

Full saves retain which side of an obstacle the rope uses. Workshop wire 8 and
save V44 retain reads of older versions; autosaves omit world attachments as
before. Invalid/truncated records are rejected transactionally. The maximum
workshop payload, including all eight bends on all 128 links, is 115744 bytes
within the existing 131072-byte cap. Assembly copies reroute at the destination
and roll back if the new route is blocked; old world-space bends are not copied.

The 384-vertex mesh follows every physical bend and sweeps both its sag samples
and spans. Clipped sag falls back to a validated straight leg instead of cutting
through scenery. Shared C/C++ fixtures and the actual SP/MP brush-trace routines
cover geometric clearance, moving endpoints and bounded failure. Production SP
attachment fixtures cover forces, saved topology, motion recovery, endpoint
exclusions, removal and real Box3D switching. Save and duplicate fixtures cover
the version transition, maximum payload, autosaves and complete-copy rollback.

This implements SP integration of the previously source-only path foundation.
MP gameplay still uses its existing ropes: authoritative MP bend replication,
world persistence and client rendering integration remain required. Native MP
brush fixtures validate shared geometry code, not an installed MP rope port.
The bounded detour search can reject complex concave routes. Dynamic prop/actor
contacts and distributed cable mass are not implemented. Rendered gameplay and
performance acceptance remain open; no graphical game or human remote client
was launched. TOOL-ROPE and the overhaul goal remain partial/active.

Catalog, assets, APIs, MP feature 53, protocol 1050 and JRW17 are unchanged.
Full build/install and independent exact-release verification follow through
scripts/build-jar.ps1. Evidence: build/rope-integration-1.42.0/.

## Latest installed release: 1.8.0 (2026-10-11)

Build `20261011-211023-07e427c2` installed at 21:12 EDT through scripts/build-jar.ps1.
All 110 automated suites, three native retail-resource loaders, the seven-card
droid spawn/restore fixture and native YT-1300 rig/persistence checks passed.
SP/MP collision-export fixtures cover distinct world, inline and sub-BSP models,
brush/patch masks and cleanup. The real solver checks hollow rooms, rotation,
moving support, momentum, shared geometry, slot reuse and budgets. The MP frame
fixture also checks geometry caching, replacement, removal and failure fallback.

A separate hidden loopback server passed retail-map prop floor/stack contact,
saved angular motion and disable/re-enable, plus world persistence and two bot
clients. It exported 51,516 world triangles on mp/ffa3. No graphical client or
human remote client was launched. Visual/gameplay acceptance remains open.
The rigid helper's historical protocol label is distinguished in qualification
from the actual network assertion of protocol 1050; raw evidence is preserved.

Independent readback verified 68 installed files, 16 replacement backups
and 1884 archived sources. All five engine/launcher executables report
Windows version 1.8.0.0. ZIP SHA-256:
`CEC2AF5C45121B19F525BB99A6CB4E2A421E09152F4EDEB25EA161AD4F86EA73`.
Receipt: `build/deploy/20261011-211241-1fa445a4/receipt.json`.
Qualification: `build/brush-collision-1.8.0/qualification.json`.
All catalog data, asset packs, configuration/launcher scripts and save/API
headers match 1.7.0. No settings, saves or unrelated mods were replaced.

The collision foundation is installed in SP/MP. Portable misc_bsp spawning,
rendering, child ownership and save integration remain unfinished; terrain also
remains unavailable. SP stays at 1,175 cards, two unavailable and 28 approved
exclusions; MP retains 1,000 choices, feature 34, protocol 1050, APIs 5/5/11 and
JRW12. SP save V38 and renderer API 1023 are unchanged. The overhaul goal and
outstanding parity/acceptance work remain active. Next public release: 1.9.0.
These receipt notes were added after packaging; all runtime sources remain exact.

## 1.8.0: native brush collision for SP and MP (2026-10-11)

Simulated props now collide with the actual brush and curved-patch surfaces of
native brush entities. Rooms and concave movers keep their empty interiors;
their broad-phase bounding boxes no longer become filled boxes in Box3D.
Both SP and MP use the native entity's position and rotation. Native movement
controllers retain authority, while moving surfaces impart momentum to props.
Ordinary non-brush model/actor proxies keep their existing hull behavior.

The collision exporter now resolves global inline-model indices across loaded
sub-BSPs, keeping each root and child model separate. Loading a sub-map no
longer prevents export of the main world's collision geometry. Terrain remains
unsupported by this mesh path. Immutable meshes are shared between instances,
bounded to 256 meshes and 262,144 aggregate proxy triangles, and discarded when
no proxy uses them or the physics world resets. Missing/oversized geometry
freezes props and returns to native mode instead of inventing a solid box.

Production-code fixtures exercise SP/MP brush/patch export, hollow interiors,
rotated and moving surfaces, momentum, shared-mesh lifetime, model/entity reuse,
budget/failure handling and the actual MP frame integration. Loaded map data,
engine imports and native traces are explicit fixture boundaries. These checks
are not graphical gameplay acceptance. Full release qualification and receipt
evidence follow after build/install.

This is collision groundwork for portable map sections. The misc_bsp menu
controller still needs rendering, child ownership and save/restore integration;
terrain also remains unavailable. Catalogs remain 1,175 SP cards (two unavailable,
28 approved exclusions) and 1,000 MP choices. No protocol, game/renderer API or
save-format changes. The overhaul goal remains active; next release is 1.9.0.

## 0.48.0 native Thruster torque (2026-10-10)

Off-center Thrusters now rotate non-actor native entities, including movers,
ordinary native objects and scripted controllers. Centered force does not add
torque. The response uses the force application point, entity mass, orientation
and approximate box inertia about the native entity pivot. Box3D retains its
existing physical impulse path. Native actors/players keep upright/view controls
and receive linear force. Owned props also receive torque with Box3D disabled.

External native spin advances with quaternion rotation while retaining native
trajectory clocks and callbacks. It coasts after the Thruster is off/removed;
holding, Freeze, disabled/occupied controls and Sandbox off stop external spin.
Rotation is capped at 8 radians/second and reduced for large bounds to limit
surface speed to 2,000 units/second, in steps no longer than 4 ms. Linear and
angular blocking stop independently. Engine script/AI control still has its
normal authority; this does not rewrite scripted behaviors.

BSP rotation checks conservatively enclose swept corner arcs against scenery.
Native riders use the actual old-to-new orientation transform, including pitch
singularities, rather than interpreting a large Euler-angle jump as physical
rotation. Blocked pushes restore rider positions, view yaw and ground references;
added force still suppresses crushing/touch callbacks. Ordinary native models
retain their engine collision hulls; this is not swept mesh collision. Props
without Box3D now check swept rotated bounds before advancing their spin.

Rope anchor velocities include angular motion. Native Weld/Hinge projection
consumes external spin when committing the projected angular velocity, preventing
it from being applied twice. Path movers remain kinematic constraint anchors.
Context Undo restores external spin; Physgun/Freeze cancels it. Duplicate retains
the current orientation/path offset and starts the copy with zero external spin,
consistent with its existing native-controller restart policy.

Full saves use **V31 `0x4a415220`**; `JMFR` now contains angular velocity.
The V30 linear-only chunk remains readable, and older save readers are retained.
Autosaves omit external motion. Workshop wire 7 and game/renderer APIs 1022 are
unchanged, as are saved callback IDs. Motion records retain the existing deletion,
initialization, shutdown and transition lifecycle.

Six focused suites and the game DLL passed. Checks execute production torque,
motion, pusher, attachment, editor and native hold code: mass/size/orientation
scaling; centered/off-center force; 20/60/120 Hz integration; coast/freeze; rider
rotation and blocked rollback; a 90-degree pitch crossing; swept scenery bounds;
separate linear/angular stops; speed limits; native/fallback Thrusters; Weld
projection; Undo; V30/V31 motion and invalid state. The full workflow also checks
duplicate spin restart and explicit V30 save-header acceptance.

Native gameplay, visual, full-save, campaign and load/performance acceptance remain
open, including embedded/rotating movers and mixed assemblies. Approximate inertia
and native hulls are not exact mesh physics. Rope scenery collision/wrapping,
remaining catalog routes and selectable poses are still unfinished. Catalog
counts are unchanged: 1,197 cards, 98 presets, 101 original unavailable entries
routed, 82 unavailable total and 181/211 entity routes. Walking/Noghri/NPC fixes
still require gameplay confirmation. The original completion goal remains active.

The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.48.0 is installed and independently verified**: **69/69 automated suites**,
**37 installed files** matched to the exact ZIP, **8 changed files**
backed up, and **1011 implementation/build/test snapshot files** matched.
Saved callback source files match the preceding 0.47.0 snapshot byte for byte.

- Build: `20261010-165612-66c7da80`
- ZIP: `build/JediAcademyRemixed-0.48.0-win64.zip`
- SHA-256: `AB480095818802B2D66556F8D2ACF00907FE0AE92725E59352B5B107C0B7677B`
- Receipt/backups: `build/deploy/20261010-165700-057254ad/receipt.json`
- Controller tests/logs/result: `build/angular-thrusters-20261010/`
- Symbols/source: `build/symbols/20261010-165612-66c7da80.zip`

Initial checks exposed decayed-array range iteration in new code, a fixture that
did not actually block a rider, and engine angle quantization in the new rotation
path. These were corrected before the six-suite focused pass and full release
run. All initial and corrected logs remain in the evidence directory.

This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; native visual/full-save/campaign/performance checks and the
remaining overhaul checklist are open. The 0.47.0 ZIP remains available.

## 0.47.0 native mover Thruster translation (2026-10-10)

Thruster now translates native brush/path movers and non-actor scripted entities
while retaining their trajectory clocks, callbacks and native motion. Tool input
is unchanged: left attaches/toggles, right removes; C / Thruster uses an initially
upward direction. Force follows the entity's orientation and uses its mass.
External momentum coasts after disabling/removing the Thruster, until blocked.
Holding, Freeze, inactive/occupied controllers and Sandbox off stop that momentum.
Scripted actors retain their existing control protections.

Translation uses swept scenery bounds and the native rider/pusher path, with
additional force split into at most 4 ms steps and a 2,000-unit/second speed cap.
Blocked riders roll back together with the mover. Added force does not invoke
crush/touch/sticky-missile damage callbacks; the original native mover path keeps
its ordinary behavior. Rotated BSP sweeps use conservative enclosing bounds.
This is not a new independent 250 Hz AI or game-frame simulation.

Train arrivals include accumulated translation in each new waypoint leg. Physgun
and context movement also update that offset, and Undo restores the prior offset
and external momentum. ROFF translation updates its position accumulator without
mistaking its angular accumulator for an endpoint. Added mover motion precedes
Rope/Weld/Hinge projection, so attached bodies see its current pose and velocity.
Path movers remain kinematic anchors for those constraints. Duplicate retains
the offset but restarts native execution and external velocity as before.

Full saves use **V30 `0x4a41521f`**, with the new `JMFR` motion chunk. Autosaves
omit external motion. Earlier saves remain readable; older releases reject V30.
Workshop wire 7 and game/renderer APIs 1022 are unchanged. Entity deletion, level
initialization and shutdown clear motion records; Sandbox off retains path offsets
for surviving map entities so subsequent train legs do not snap back.

Five focused suites and the game DLL passed. Tests execute production force,
native push, train arrival/state, hold/editor and attachment code against bounded
engine fixtures: 20/60/120 Hz force integration, rider carry and blocked rollback,
world stops, speed/sweep caps, controller clocks, train waits, Freeze/Physgun/Undo,
rope response, duplicate offset, saved motion, corrupt input and slot cleanup.
Native callback source IDs are unchanged from the 0.46.0 snapshot.

Gameplay checks remain open for doors embedded in scenery, rotating trains,
scripted paths, player riders, mixed constraints and full native save/load.
Native angular-force response, rope scenery collision/wrapping, remaining catalog
routes and selectable model poses are still open. Catalog counts are unchanged:
1,197 cards, 98 presets, 101 original unavailable entries routed, 82 unavailable
total and 181/211 entity routes. Existing walking/Noghri/NPC fixes still require
owner gameplay confirmation. No game is launched by this release workflow.

The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.47.0 is installed and independently verified**: **69/69 automated suites**,
**37 installed files** matched to the exact ZIP, **9 changed files**
backed up, and **1011 implementation/build/test snapshot files** matched.
Saved callback source files match the preceding 0.46.0 snapshot byte for byte.

- Build: `20261010-164539-cb1045c9`
- ZIP: `build/JediAcademyRemixed-0.47.0-win64.zip`
- SHA-256: `EFA06D990F8C0264BA102DF2F608A7F83DB5CC66C12E00E54E90BD9C1A5BA8A3`
- Receipt/backups: `build/deploy/20261010-164626-a3bd06ec/receipt.json`
- Controller tests/logs/result: `build/mover-thrusters-20261010/`
- Symbols/source: `build/symbols/20261010-164539-cb1045c9.zip`

Initial focused compilation exposed duplicate fixture linkage and an include-order
dependency; both were corrected before the five-suite focused pass and this full
release run. Initial and successful logs remain in the evidence directory.

This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; native visual/full-save/campaign/performance checks and the
remaining overhaul checklist are open. The 0.46.0 ZIP remains available.

## 0.33.0 native and mixed Weld/Hinge (2026-10-10)

Weld and Hinge now connect native NPCs, the player, pickups, ordinary native
models, owned props and world surfaces. Box3D can be off. In Q choose the tool,
then click two different entities or an entity and scenery. The first clicked
face supplies the hinge pivot and axis; the second click retains the entities'
current relative poses. Right click cancels the first anchor. Frozen entities,
world surfaces, occupied controllers and scripted/path movers act as anchors.
Path movers retain their native motion; this does not add force-driven movers.

In single-player, when either click hits bare map scenery, that surface is the
fixed pivot. The first click still sets a hinge's axis. Duplicate can carry a
qualified world anchor or mounted thruster on a copyable map model. Pasting a
fixed anchor requires matching scenery at its new location, and Undo removes
the pasted object with its attachments.

Hold C to use the new **Weld** and **Hinge** actions, including **Select player**.
Context anchors use the entity origin and an initially vertical hinge axis. To
connect yourself to scenery, first set a world anchor with the equipped tool,
then C -> Select player -> Weld/Hinge. Release permits motion; Unlink removes
the selected entity's attachments and Undo removes a newly created link. Exact
position/angle edits require Unlink. Physgun USE rotation retains attached joints.

Native/mixed graphs project position and velocity after native movement, using
mass and approximate box inertia. Welds restrict all relative motion; hinges
retain axis spin. The graph solve exchanges impulses across all connected bodies
and adds no positional spring velocity. It handles moving/rotating anchors without
advancing native movement twice. All-Box3D components retain their existing solver;
an entire mixed component uses the native projection, preventing duplicate joints.
Native angular momentum on ordinary props continues after Unlink. AI, gravity and
ordinary actor input remain native; constraints do not inflict Force-grip damage.

Corrections use native collision sweeps. Direct neighbours are temporarily excluded
from that correction trace; other scenery/entities still block it. This is an
approximate rotated box sweep, not full swept angular mesh contact. Native actors
and native attachments remain game-frame-rate systems, not 120 Hz Pmove/AI. Large
or obstructed assemblies, player/camera feel and collision interaction still need
gameplay testing. Scripted/path mover thrusters and rope wrapping remain open.

Full saves now use **V16 `0x4a415211`**, with **workshop wire 7**; game/renderer
APIs remain 1021. Earlier saves are readable; older releases reject V16. Internal
owned native/prop Weld/Hinge links now survive assembly Duplicate with remapped
identities. External map/player/world anchors are excluded from copying and from
map transitions. Death, deletion, slot reuse, cleanup and Sandbox off remove links;
transient anchor-motion history resets on loading and checks entity generations.

Eight focused suites and the native game DLL build passed. The production solver
fixture covers 20/60/120 Hz steps, moving/rotating anchors, long lever arms, unequal
mass, hinge spin/flipped axes, closed loops, a 128-link chain and 128 independent
pairs. In one bounded headless run the chain took about **18 ms per solve** and the
independent pairs about **0.7 ms**; these are not gameplay performance results or
an active-load qualification. Engine-service fixtures cover collision vetoes,
real Box3D pose writeback, mixed/pure graph transitions, lifecycle, saves, copy,
tool dispatch and C menu transport. The full release workflow contains **52 suites**.
No game launched; native visuals/input/campaign and full engine save/load remain open.

The owner answered that the installed walking and Noghri attack fixes have **not
been tested yet**. Both acceptance items remain open; no gameplay success is
inferred. Catalog totals are unchanged: **1,197 cards**, **80 portable definitions**,
**83 of the original 182 unavailable entries routed**, **99 still unavailable from
that set**, plus one empty model placeholder (**100 unavailable cards**). Remaining
controllers/properties, scripted mover forces, skeletal poses and broad gameplay,
save/campaign/physics acceptance still need completion. The overall goal is active.

### 0.33.0 installation evidence (workspace post-package record)

The normal `scripts/build-jar.ps1` workflow completed: **52/52 suites**, retail
catalog/asset checks, packaging and local Steam GameData installation. Build
**20261010-130203-6cd52d6a**; **37 files verified**, **9 replaced with backups**.
Independent readback matched the ZIP, installed identity/hashes and **931
source/build/test/runtime snapshot files**. Save V16, wire 7, APIs 1021.

- ZIP: `build/JediAcademyRemixed-0.33.0-win64.zip`
- SHA-256: `DE1AE448CB1CB5FC28C93BB9E7CA84BAC4A1D5FEB438E08635BE42009D041FB9`
- Receipt/backups: `build/deploy/20261010-130257-21980282/receipt.json`
- Workflow: `build/native-joints-20261010/release-install.log`
- Independent verification: `build/native-joints-20261010/release-result.json`
- Symbols/source: `build/symbols/20261010-130203-6cd52d6a.zip`

This post-package record is workspace-only; installed docs contain the feature
and test notes above. **No game launched.** The owner confirmed walking and Noghri
attacks are not tested yet. Those fixes, native joint input/collision/save behavior
and large active-assembly performance remain unaccepted. The all-objectives goal
is active. Remaining catalog controllers/properties, scripted mover forces, rope
wrapping, skeletal poses and broader campaign/save/physics acceptance remain open.


## 0.27.0 posed decorative model collision (2026-10-10)

Decorative Ghoul2 model copies now get collision bounds from the visible vertices
in their native default skeletal pose. Both Classic and Rend2 use the same sampler;
hidden surfaces and bolt/tag helpers do not inflate the box. The result is cached
once per model per map. Placement, rotated native collision hulls and Duplicate use
these bounds, including models whose local origin is away from their center.

With Box3D enabled, models inside the existing rigid-body size limits now receive
volume-derived mass, rotation, contacts and the existing prop tools, including
Weld/Hinge/Rope/Thruster. Native actor contact searches cover the full rotated
body bounds, including large off-center shapes. Failed bounds queries are cached
for the map so empty models in old saves do not rebuild every frame.
Larger models retain their measured native collision box
and ordinary native motion. This is a box around the default pose, not articulated
ragdoll or per-triangle dynamic collision; selectable skeletal poses remain open.
Existing NPC/vehicle controllers are unchanged. Native-mode saved props refresh
their hull on the first active Sandbox frame, including saves from older releases.

The retail check covers **126 visible Ghoul2 candidates** and their mesh/skeleton
metadata. One empty `noweap.glm` placeholder is now explicitly unavailable instead
of pretending to spawn a visible model. The catalog remains **1,197 cards** with
**105 unavailable entries**: the previous 104 plus that newly identified empty
placeholder. The 78 earlier map-entity/NPC routes remain implemented.

New full saves use **V10 `0x4a41520b`**, with workshop wire 6 unchanged. Earlier
saves remain readable; older releases reject new saves before loading. The new
bounds callback changes both **game and renderer APIs to 1021**; install all runtime
modules together through the normal release workflow.

Headless fixtures cover posed weighted vertices, hidden/bolt surfaces, traversal
and geometry bounds, cache cleanup, rotated placement, real Box3D hull/mass parity,
context/attachment/duplication regression and save-version routing. Retail checks
verify candidate asset structure; they do not render all models. Native visual,
contact, save/load and active-load acceptance remain open. **No game launched.**
The all-objectives completion goal remains active.

## 0.26.0 native Rope and Thruster attachments (2026-10-10)

**Rope** connects native NPCs, the player, pickups, native movable models, owned
props and solid world surfaces. Click its two anchors with the Toolgun. Frozen
objects, native path movers and occupied controllers act as anchors. Native/mixed
ropes work with Box3D off; two simulated props use the existing Box3D distance
constraint. The line follows both saved local anchor points. Rope slack in Q
sets the starting length for new ropes. In single-player, select an attached
entity in C and open **Ropes** to see the other endpoint and change an existing
rope's length. **Next attached rope** cycles through ties on the selected entity.
Shortening a rope can pull released endpoints together. **Revert property**
restores the previous length. Full saves and eligible Duplicate copies carry
the edited length.

**Thruster** now applies force to native actors, pickups and models with ordinary
object motion, as well as owned props. Left attaches/toggles; right removes. The
clicked face selects direction. Native actor input/AI and gravity remain active;
forces do not replace their controllers or cause Force-grip damage. Frozen native
entities wait for Release. Eligible native path movers translate under thrust.
In single-player, select an entity with a mounted thruster and open **Thruster**
beside Refresh in Context to edit its force and power or reverse its direction.
Applying the edit keeps the mount point. Full saves and eligible Duplicate
copies retain the settings, and **Revert property** restores the previous values.

The **C context menu** adds Rope, Thruster and Unlink. Select player to use them on
yourself. Context Rope places its anchor at the selected entity's origin; select
another entity and press Rope again. To tie the player to scenery, first place a
world anchor with the equipped Rope tool, then C -> Select player -> Rope. Context
Thruster initially points upward; its direction then follows the entity's rotation.
Unlink removes the selected entity's attachments. Undo removes newly created links.

Native rope impulses exchange momentum according to mass, with swept positional
correction and no force while slack. Native velocity is bounded to 2,000 units/s.
Player force pauses during protected native controls/cameras, and occupied or
ROFF-controlled endpoints act as anchors. Death/freeing removes affected links;
cleanup/Sandbox off removes attachments. Actor and native pickup integration are
still native-frame-rate systems; this does not promise 120 Hz native AI or Pmove.

Full saves use **V9 `0x4a41520a`, workshop wire 6**. Earlier saves remain readable;
older releases reject these new saves. Game/renderer APIs stay 1020. Native entity
references and world anchors do not transfer through autosaves/map transitions.
Duplication copies internal owned native/prop attachments; links to existing map,
player or world endpoints are excluded and the copy message reports that exclusion.

The production attachment fixture checks native/world/player references, forces,
unequal mass, gravity at multiple time steps, collision vetoes, mixed real-Box3D
response, local directions, toggles, frozen/protected states, death/free cleanup,
save records and old-wire rejection. Native UI, collection, save/load and campaign
acceptance still require gameplay. **No game launched.** Native Weld/Hinge and
scripted-mover thrusters, rope wrapping/collision with scenery, decorative GLM
collision/poses, walking/crash confirmation and active-load qualification remain
open. Catalog unchanged: 1,197 cards, 104 blocked. The completion goal stays active.

## 0.24.0 assembly duplication

Duplicate retains each member's frozen state and the original local joint frames,
rope length, thruster force and enabled state. IDs are remapped to the copied
members; a translation does not change local frames. Bodies restart with zero
linear/angular velocity. A real solver fixture compares a source welded pair and
its translated copy, while save fixtures verify the resulting ownership/joints.
This adds no arbitrary native-entity constraint attachments; those remain open.


## 0.23.0 held rotation

USE + mouse while holding Physgun primary sets a bounded yaw/local-pitch angular
target. Box3D applies angular velocity, capped at 8 rad/s, while retaining collision
and joint solving. Release USE resumes ordinary dragging; release primary restores
gravity and retains fling velocity. Welded/frozen neighbours can resist rotation.
Context exact pose edits require unlinking; native entities keep native controllers.
Both free rotation and constrained weld/hinge behavior are exercised by the real
solver fixture. These tests do not qualify native feel or large active-body loads.


> **0.22 update (2026-10-10):** 1,197 searchable entries, 74 newly spawnable
> from the previously blocked set, 108 remaining blocked. [Catalog controls](JAR-spawn-catalog.md)
> cover portable map controllers, automatic helpers, Activate and Link. The fixed
> limits are 4,096 owned objects and 128 actors. Default `g_noFootSlide 1` now
> adapts NPCs only; the player's walking fix still needs visual acceptance.
> Beams are owner accepted and unchanged. New saves use V8 `0x4a415209`, wire 5.
> Older release sections below are historical where they conflict with this update.

Enable Sandbox, hold Q and toggle Box3D. It remains Off by default and preserves
the installed setting. `jar_sb physics on|off|status` provides the same control.
The pinned Box3D solver operates on qualified spawned box/crate/barrel hulls.
Native entities use their game controllers, with a separate suspended-controller
path for Physgun/freeze. Unsupported terrain/sub-BSP or oversized collision maps
disable Box3D with a reason and preserve props in native mode.

## Motion and collision

The solver advances at **120 Hz**, four substeps per step, at most six steps per
50 ms native frame. Props use interpolated render trajectories. This smooths
presentation without increasing the native AI/script tick or guaranteeing 120 FPS.
One metre equals 32 game units. Hull volume at density 120 kg/m3 defines mass;
larger props respond less to the same impulse. Gravity, friction, low restitution,
continuous collision, sleep and angular damping remain enabled.

Native actor movement applies time-bounded push impulses, instead of treating an
NPC/player as an infinitely strong moving proxy. Swept hull contacts exchange
momentum by prop/actor mass, push actors with world collision checks, and correct
the prop if the actor is blocked. Players/NPCs standing on a moving prop are carried
with it and apply downward weight. Native axis-aligned hulls remain conservative
around rotated boxes. Movers and held/frozen actors retain kinematic proxies.
Vehicles retain native handling; this is not a replacement vehicle simulation.

Collision events use stock metal-impact sounds, at most eight per native frame
and one per prop per 150 ms. Quiet contacts do not play. Player/NPC impacts and
the native fallback also use this bounded sound path. This release does not add
collision damage or material-specific sound classification.

Physgun drives a held prop toward the aim point using a bounded velocity servo.
Wheel input changes hold distance; primary release restores gravity/rotation and
preserves current momentum. Right-click freezes and cancels the throw. Non-Box3D
entities use a swept, harmless native grip with original trajectories/controllers
saved for release. They are not converted into arbitrary rigid meshes.

The fixed capacity is 4,096 props inside a shared 4,096-object registry, plus
256 nearby native proxies and 262,144 collision triangles. Native allocation can
still fail cleanly if a map exhausts entity slots. Turning Box3D off freezes props,
destroys solver state and retains saved attachments for a later re-enable.

## Attachments

Choose Toolgun and a tool from the vertical Q list. Weld, Hinge and Rope require
two different frozen spawned props within 512 units. Left-click each; right-click
cancels the pending first anchor without changing tool. Use Gravity afterward on
members that should move; leave an anchor frozen.

- Weld retains relative pose. Hinge rotates around the first clicked face normal.
- Rope uses the two clicked local anchors and a maximum distance with adjustable
  slack (1-3x). A segmented hanging line visualizes it. Endpoints collide normally;
  the visual rope has no intermediate world-contact segments or wrapping.
- Thruster attaches at the clicked face and pushes along its outward normal.
  It releases the prop and turns on; left toggles, right removes. Force for new
  thrusters is 100-100,000 N. One thruster per prop is supported.
- Unlink removes all attachments on the aimed prop. Undo after attachment removes
  it. Runtime links follow saved stable ownership IDs, never reused entity slots.

There are 128 combined attachments, with one link per prop pair. Rigid linked
assemblies keep angular freedom under Physgun. Unlink before rotating a Weld/Hinge/
Rope member; a single-body thruster may rotate with its host. The iterative solver
allows small compliance. Long chains and extreme thrust still need gameplay tuning.
Attachments currently require Box3D props, not arbitrary actors/movers or world anchors.

## Saves and qualification

Header `0x4a415207`, workshop wire 5, saves native frozen-controller restoration
alongside prop motion and Rope/Thruster/Weld/Hinge records. Older native/JA:R saves
remain readable. Earlier releases reject new saves. Game API 1019 prevents mixing
the new save writer with an older game DLL. Solver caches rebuild on load, so future
physics is not promised bit-identical. Randomizer seed/state remain independent.

Tests exercise the actual solver, native hold controller and actor collision
bridge: mass ratios, release momentum/gravity, 120 Hz cadence, collision events,
rope distance, thruster toggles, player support, fast actor collisions, blocked
displacement, controller save/restore, malformed data and 4,096-body allocation.
No game was launched for 0.20. These headless checks do not establish gameplay
feel, complete campaign safety, extreme-load performance or visual acceptance.
Earlier 0.18 native timing/contact evidence is retained in the validation history;
it does not qualify the newly changed actor response or the larger active load.

## Source and reuse

Box3D is Erin Catto's MIT-licensed C17 core, pinned at commit
`e77352cd606dc1a34209094076199549a52ea0a1` (v0.1.0 alpha):
https://github.com/erincatto/box3d/tree/e77352cd606dc1a34209094076199549a52ea0a1 .
The unmodified core is vendored in `lib/box3d`; `SOURCE.json` records the exact
per-file SHA-256 provenance. It is linked statically; no external physics DLL,
SDK installation, service or runtime download is needed. The package includes
`Box3D-LICENSE.txt` and `Box3D-SOURCE.json`; the source archive includes the core.
The native bridge, geometry export, UI and original icon are JA:R GPL changes.

Facepunch/sandbox MIT source at `e280e2a667bd0d95cbd6606db687d66f05199507`
was inspected as a behavior reference. Its C# scene/weapon/network APIs require
s&box; no code or assets were copied, and its excluded Envato audio was not used.
The compatible Box3D core and existing OpenJK parsers, collision geometry, native
menus, entity ownership and save machinery were reused directly instead.

The icon is original local vector geometry, generated by
`scripts/generate-jar-icon.py`, with source SVG, PNG, nine-size Windows ICO and
SDL RGBA data retained. No third-party logo/art or cloud image generation was
used. Stock model/NPC references use the owner's installed game data; the expanded
catalog adds paths/fingerprints and does not redistribute retail model files.

