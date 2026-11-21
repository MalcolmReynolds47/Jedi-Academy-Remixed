# JA:R validation record

## Multiplayer Korriban FFA

The MP server admits the exact retail BSP and shared stock child records.
Two clients receive 47 pickups, 28 native torches and seven ambient winds.
The original eight fall volumes use the shared validated BSP extents.
JRW22 saves Korriban pickup respawn timers. Older world journals continue
to load with their original layouts.

The collision fixture checks the stock checksum and verifies that only
the Korriban root can enable local fog. Classic and Rend2 reuse playable
BSP-leaf selection for actors and the camera. Rend2 resolves fog colors
against each section's world and draws a fog cap at the room's stock
opaque distance. Rendered play and visual transitions remain open.

## Multiplayer Imperial Shaft and section timers

Imperial Shaft reuses the stock BSP, levelshot, floor pads, two independently
moving lifts, fall trigger, three speakers and six non-solid scenery models.
The server validates the retail brush layout and tests exact contact inside
the translated and rotated floor pads. The two-client tests check the lifts'
independent activation and return, scenery transforms, late join and Undo.

JRW21 records Shaft lift and pad timers and Rift pickup respawn timers.
World restore resumes those timers against the current server clock. Older
journals start those children in their stock state. Interactive visuals and
full gameplay remain open.

## Multiplayer Rift Sanctuary

The multiplayer server validates the stock Rift BSP, its two native
fall triggers, and all 38 pickups. The compound trigger checks its two
authored brushes independently after translation and rotation. The
collision fixture checks the gap and the separate fall trigger. The
client qualifies the stock room and its playable leaf before emitting
cold breath. Classic and Rend2 select the room's fog for actors and the
camera.

The dedicated-server tests exercise spawning, late join, and Undo with
two UDP clients in pure and non-pure sessions. Interactive rendering and
play remain to be checked. Restoring a saved multiplayer room starts its
pickups in their stock state.

## Multiplayer Hoth Canyon

The multiplayer catalog reuses the single-player card and stock levelshot.
The server admits only the original Hoth Canyon BSP and uses the shared
fall-volume validator. Both renderers separate the 105 backdrop surfaces
from the 522 arena surfaces. The client checks the section checksum and
the camera's local BSP leaf before selecting the backdrop.

The collision fixture verifies identity, root-versus-child handles and
inside/outside selection. The Rend2 fixture verifies both submission
passes. The dedicated-server tests check stock wind positions, brush
registration, late join, and Undo. Live rendering and fall-trigger play
remain to be checked.

## Vjun Sentinel multiplayer journal

JRW20 stores each room's ten lift phases, five floor-pad delays and
cooldowns, and 45 pickup timers in stock BSP order. The dedicated-server
test saves a room while its lift is up and its concussion rifle is
respawning, restores it into new entity slots, and checks the remaining
times. Additional fixtures load a moving lift and a queued floor pad,
reject an impossible lift time with full section rollback, and load a
JRW18 Sentinel at its original starting state. Existing JRW19 City saves
retain their encoding. Interactive saves and rendered movement remain open.

## Vjun Sentinel in multiplayer

The Map Sections tab uses Sentinel's original levelshot and catalog ID in
both modes. The multiplayer server checks the retail BSP checksum and uses
the shared single-player admission rules. The room owns ten linear lift
brushes, five floor pads, the fatal fall trigger, 45 respawning pickups
subject to server item rules, and 12 ambient sound emitters.

The dedicated-server test verifies that both clients receive the room and
its visible children, a floor pad moves its five matching lift pieces, the
pieces return to rest, the concussion rifle respawns after five seconds,
and Undo removes the room and its children. In-game rendering, lift riding,
audio and the remaining floor pads and fall trigger need gameplay checks.

## Multiplayer overlapping Sandbox placement

JA:R 1.164 allows catalog bodies to overlap when spawned, duplicated or
restored. The overlap exception is confined to placement hull checks.
Players, native actor hulls, solid map brushes and portable section
geometry still participate in those checks. The aiming ray and normal
movement retain their existing collision rules.

The Construct dedicated-server regression restores a frozen crate inside
the authored sofa, saves the exact pose, then confirms that a crate inside
the floor is rejected. It also verifies that clearing the Sandbox leaves
the four pieces of map furniture intact. Interactive placement remains open.

## Sandbox cleanup below the map

JA:R 1.163 uses the same fall boundary in both modes. A Sandbox object is
removed after its top passes 256 units below the lowest loaded world or
portable section. Crossing the engine coordinate limit also removes it.
Multiplayer uses the normal entity cleanup path for ropes, tools and
physics state. Untracked map entities and occupied vehicles remain in place.

The multiplayer fixture exercises native movement and Box3D, including
coordinate-limit cleanup while another prop continues simulating. It also
checks attached rope removal, stale selection cleanup, occupied vehicle
protection and removal of a portable section's lower boundary. The native
Construct server test compares the reported map bottom with the loaded BSP.

## Construct ladder round trip

The multiplayer wire test waits for each movement command to reach the
server's player state. It climbs through the upper opening, backs onto
the ladder and descends. Both clients receive the return to the lower
ladder. A single-player collision fixture also checks the reverse sweep
through the opening. Single-player uses the stock hanging pose when the
model lacks ladder clips, matching multiplayer. Interactive movement and
animation still need an in-game check.

## Construct upper ladder exit

The original opening provides 48 units of vertical clearance. A standing
Jedi Academy player needs 64 units. Both game modes test the normal crouch
hull when the standing hull hits an obstruction while climbing. The map
converter adds a non-solid ladder volume leading toward the original upper
dismount marker. The passage's solid brushes and rendering data are unchanged.

The pure-server loopback test sends movement commands through the upper
opening and checks that the player and an observer both receive the upper
exit. Existing Construct multiplayer worlds keep their save identity for
the verified BSP revision.

## Construct ladder in multiplayer

The same multiplayer movement code runs on the server and in client
prediction. It samples the authored ladder brush separately from water,
applies a 70% speed cap, and uses the model's ladder clips when available.
The retail humanoid has no ladder clips, so it uses its hanging pose.

The Construct loopback test uses native movement commands on the converted
BSP. It checks ascent and the observer's snapshot, stationary hold, descent,
stepping off at the base and landing back on the floor. The upper exit and
interactive client movement remain open.

## Construct furniture in multiplayer

The companion doll, table, armchair and sofa use the same BSP spawn data
as single-player. Their starting masses are 1, 40, 700 and 300 kg.
Multiplayer registers each as a server-owned physics prop. The map owns
their lifecycle. They survive `jar_world_clear` and return to their authored
positions on map reload. Their current positions and direct attachments to
them are outside the Sandbox world save. Copies use the regular prop journal.

The Construct loopback test verifies all four solid props on two clients,
the armchair's reported mass, Physgun pickup and lift on both snapshots,
and right-click freeze. It also checks that saving, clearing and loading an
empty Sandbox world leaves all four originals in place. Interactive
gameplay remains to be checked.

## Construct ambient soundscapes in multiplayer

Multiplayer clients use Construct's existing eight audio stems, four
ambient mixes and 19 zones. Zone selection runs on each client from its
player or followed player position. The shared single-player parser and
three-second crossfade run in both modes. Remote clients use the local
BSP's world collision to test walls and water. The software mixer routes
left and right stems independently and OpenAL starts new stems together.

SP and MP fixtures cover the parser, fades, rewind and stereo routing.
A native MP collision fixture on the installed Construct BSP matched all
19 zones and the outside-to-lake crossfade. In-game listening and
hardware audio validation remain open.

## Construct and Flatgrass in multiplayer

The local release stages the same converted map archive for single-player
and multiplayer. The archive check verifies both RBSP files, FFA arena
metadata and the original map previews. In multiplayer, open
**Host JA:R Sandbox → Browse maps** and choose either map, then start
the server. The map field still accepts a map name directly.

The host menu fixture verifies selection and launch commands for both
maps. Separate production Rend2 fixtures exercise the shared skybox
batching fix in both renderers. Dedicated loopback tests load each map
from the staged archive and check pure-mode admission for two clients.
Interactive multiplayer rendering remains open.

## Construct skybox crash in 1.156.0

The single-player Rend2 crash log for `gm_construct` reports
`SHADER_MAX_VERTEXES hit in DrawSkySideVBO()`. Rend2 uses the map's
sky triangles to find visible skybox faces. It now reuses that batch
storage for a separate upload per face after projecting the triangles.

The renderer fixture covers all six faces with 999 source vertices and
5,994 indices in the original batch. It verifies each face's 81 vertices
and 384 indices, followed by the cloud pass. The same fixture reproduces
the reported error with the 1.155.0 source. GPU rendering and a
single-player launch of Construct remain to be checked in-game.

## Portable Vjun Sentinel in 1.155.0

In single-player, hold Q and choose **Map Sections → Vjun Sentinel**. The
section uses the original `mp/ffa1` BSP and its stock levelshot. The section
owns ten native lifts, five floor trigger pads, one fatal fall trigger, 45
respawning pickups and 12 ambient speakers. The five pads address only their
own lift groups and retain their original delays, cooldowns and brush planes.
The pickups use the original map's multiplayer respawn intervals. Use the
spawned section to enter it through one of its 14 local starts.

The stock BSP fixture checks admitted content, ownership, movement, duplication,
native field serialization, entry and cleanup. The release also checks the
shared Map Sections tab in multiplayer, where the seven supported portable
sections retain their original catalog IDs. Rendered single-player lift and
pickup play, full interactive save/reload and an external multiplayer session
remain open.

## Portable Yavin Training Grounds in 1.154.0

The retail admission audit verifies 31 inline models, all 16 local
starts, and the nine detached sky surfaces. It rejects foreign emitter
and timer targets and changes to the saber-only brush policy. The native
section fixture checks 323 owned children, including 30 breakables,
51 scenery models, 16 torches and 74 dust emitters. It runs the native
timer, random target and FX callbacks against two room instances and a
host marker with the same name. It also checks movement, local aim,
ownership, cleanup and native timer save fields. The sky fixture covers
camera transforms and the surface selection shared by Classic and Rend2.
Interactive rendering, combat and full in-game save/load remain open.

## Physgun surface pickup in 1.153.0

The MD3 fixture checks a rotated triangle at an off-center click, an
empty corner within its bounds, a nearer obstruction, the selected
animation frame and malformed triangle indices. The MP Sandbox fixture
checks that the selected model contact sets pickup depth and becomes the
local grip. The shared physics fixture covers yaw tracking, elevation
with preserved tilt, USE rotation on a sideways prop, and the off-center
grip through Box3D substeps and release. Interactive verification of
the camera, beam and held actors remains open.

## Portable Korriban FFA in 1.152.0

The retail BSP admission audit checks the 17 starts in the playable
fog area and rejects changes to the eighth fall brush, pickup counts
and torch effects. The native controller fixture moves, rotates, copies,
enters and removes all 310 owned children. It checks the eight fall
volumes after rotation and a stock weapon's respawn and native save
fields. The retail item parser resolves every pickup model, and the
resource audit checks the stock wind and torch assets. Classic and
Rend2 fixtures select room-local character and camera fog. Interactive
rendering, gameplay and full in-game save/load remain open.

## Single-player prop mass and gravity in 1.151.0

The native menu and command fixtures cover eligible selections, input
validation, mass and gravity edits, and Undo. The motion fixtures cover
native gravity and Box3D acceleration, custom mass, and reset to the
shape-derived mass. Assembly duplication and full save serialization
retain the settings. Autosaves omit owned props as before. The codec
checks legacy defaults and all 4,096 props with settings at the previous
maximum save size. Interactive menu, physics, and full save/load checks
remain open.

## Physgun contact and portable Rift fog in 1.150.0

The MP server fixture fires the Physgun at a 45-degree prop from an
off-center height, checks the local contact against the rotated model
bounds and verifies that a nearer wall prevents the grab. The shared
solver fixture turns a held prop about its clicked point at 20 game
frames per second with six 120 Hz physics steps per frame. Existing
native and rigid fixtures cover yaw tracking, elevation, USE input,
freeze, release momentum and obstruction. Interactive confirmation is open.

Classic and Rend2 fog fixtures check actor and camera selection against
the playable leaves of translated and rotated Rift Sanctuary instances.
They cover the host fog outside the room, the Rend2 sky-portal entity
prefix and reuse of the room's existing fog buffer for actors. Cross-boundary
fog and interactive visual confirmation remain open.

## Rift Sanctuary cold breath

The native client fixture exercises two portable instances, including a
rotated room, and checks the exterior, hidden instances, resource refusal
and snapshot removal. The retail BSP audit verifies cold-breath mode 2
and all 16 player starts against the stock playable area. In-game visual
confirmation remains open.

## Local Garry's Mod map add-on (2026-11-12)

JA:R 1.146.0 selects Construct's closest visible soundscape zone
with a brush-and-water trace. It retains the current ambient mix
when no zone qualifies and crossfades to the next mix in three
seconds. All 19 positions and radii, four profiles, pitch variants,
two stereo sources and the nested lake-water loop were verified.
The x64 software mixer routes the separate stereo stems.

Native BSP tests reached all 19 zones and verified an outside-to-lake
transition. The mixer fixture checked left and right routing, and
the asset check compared the converted PCM against the Source files.
The release and local PK3 were installed and hash-verified.
In-game listening remains open.

### Decals and overlays

The decal update adds 19 attached decals and 11 compiled overlays
as 65 depth-biased draw surfaces. The package check matched original
texture RGBA, projected UVs and the receiving faces and visibility
leaves for every added surface. The unattached Construct decal
with Hammer ID 506897 is 231.71 units from the nearest Source BSP
surface. It is recorded in the conversion report.

Native checks passed for both maps' 89 starts, Construct's four
movable props and Color Room, and Classic/Rend2 lightmap ingestion.
All 154 automated suites passed. In-game decal appearance remains
open. Receipt `build/source-map-port/decals/install-receipt.json`.

### Color Room and sunlight

Construct's Color Room uses the original brush-model positions and a
native USE switch at the Source origin. The switch toggles its 174
lightmapped faces and plays the original click. The converter combines
the baked sunlight layer with base light on 2,264 faces. It retains the
room's separate light layer for toggling.

Native checks passed for six room boundaries, three standing-player
approaches to the switch, and the switch's cooldown, sound event and
off/on lightstyle publication. All 13,530 paired light samples matched
the Source samples after conversion. Classic and Rend2 loaded the
lightmaps. All 89 player starts, four movable prop placements and nine
static collision brushes passed again. Flatgrass assets matched the
prior add-on byte for byte. All 154 JA:R automated suites passed.
In-game visual, audio and save/reload confirmation remain open.

### Static scenery collision

The installed collision update converts the five solid Source scenery
placements to nine native brushes. The converter matched the installed
Source VPhysics collision mesh to 0.0001 units for the four unique
models. Native BSP checks covered 144 face approaches, all five world
placements, 242 standing-player sweeps on Construct and 100 clear
positions inside the sink's overall bounds. Flatgrass's doll was
verified with point and small-hull sweeps.

All 89 starts and four movable prop placements passed again. Draw
geometry, materials, lightmaps and the remaining package files matched
the prior build byte for byte. All 154 JA:R automated suites passed.
The updated PK3 and rollback copy were hash-verified. Receipt
`build/source-map-port/static-collision/install-receipt.json`.
Interactive gameplay remains open.

### Initial map conversion

Converted the locally installed stock `gm_flatgrass.bsp` and
`gm_construct.bsp` to native RBSP. The generated archive closes all
referenced texture, shader, scenery mesh and ambient WAV dependencies.
Construct's doll, table and two couches use converted MD3s, their
original mass values and native JA:R gravity. Native hull traces
verified floor support and lateral clearance for all four props.
The add-on includes both original Garry's Mod menu previews, cropped
to fit the JA:R map cards.
Native player-sized floor sweeps passed at all 56 Flatgrass and 33
Construct starts. Classic and Rend2 lightmap fixtures accepted the 33
Flatgrass and 111 Construct lightmaps. The package check verified
visibility from each primary spawn and sky camera.

Installed `JediAcademyRemixed/jar-gmod-maps.pk3` with SHA-256
`EA32743B406C1D6EC0D110B45FFC2ED4540EAFDD6C4F3EA588EDFD018FBFFAD2`.
The qualification, native logs and receipt are under
`build/source-map-port/`. Interactive geometry, effects, audio and
prop handling have yet to be verified.

## Latest installed release (1.146.0, 2026-11-12)

Build `20261112-164436-ae5979b3` installed at `2026-11-12T16:52:38.4685683-05:00`.
All 155 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `F02DCC89CFE15BC30831BD1DD382914BAF307AC89D2E06AF09EEE60F8090B7B1`.
Receipt `build/deploy/20261112-165213-eaae7ff4/receipt.json`.
Qualification `build/source-map-port/soundscapes/release-qualification.json`.

Construct's local map add-on uses the 19 Source soundscape zones
and four original ambient profiles. The closest visible zone sets
the mix, and the last mix continues through passages with no eligible
zone. Tracks crossfade over three seconds. The two stereo loops keep
their channels, and the Lake mix includes its nested water loop.

The updated local PK3 was installed at `2026-11-12T16:53:35.7042133-05:00`.
The package and rollback copy were hash-verified.
Receipt `build/source-map-port/soundscapes/install-receipt.json`.
Native checks covered all 19 zones, an outside-to-lake transition,
the mixer routing and the converted PCM. In-game listening remains
open. This record was added after packaging.

## 1.145.0 installation (2026-11-12)

Build `20261112-154931-62e3fd71` installed at `2026-11-12T15:57:22.7175152-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `FC15434A7AAFE4659F58D15592853653084207EB662ECE77EB3A9E82490CA086`.
Receipt `build/deploy/20261112-155658-188f53c2/receipt.json`.
Qualification `build/physgun-grip-1.145.0/qualification.json`.

The single-player Physgun keeps the clicked point attached to its aim
and beam. Held objects follow the player's yaw while preserving pitch
when looking up or down. USE rotation pivots around the grab point.
The new behavior applies to Box3D props and native entities, with
native mover and mounted-charge collision checks.

Focused fixtures cover off-center Box3D grabs, yaw tracking, manual
rotation, release momentum, NPC pivots, brush mover riders and blocked
motion, and prop arc and charge clearance. Interactive confirmation
remains open. This record was added after packaging.

## 1.144.0 installation (2026-11-12)

Build `20261112-152538-945092b3` installed at `2026-11-12T15:33:35.3303569-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `E5511A36E8AA2286DBC4433C67CAD8AD198F717F96AC17BEBFF49D02D2AC1A2B`.
Receipt `build/deploy/20261112-153311-f1266be5/receipt.json`.
Qualification `build/sp-cheats-1.144.0/qualification.json`.

Every JA:R single-player map load starts with `helpUsObi 1`, including
the Maps picker, ordinary console loading, campaign transitions and
saves. The common server startup sets the flag before initializing
the game and publishing systeminfo. Explicit development map commands
retain their resource reload modes.

The startup fixture reproduced disabled cheats on 1.143.0 and
verified the flag during game initialization, simulation and
systeminfo publication on 1.144.0. Interactive confirmation remains
open. This record was added after packaging.

## 1.143.0 installation (2026-11-12)

Build `20261112-151608-c2a583c8` installed at `2026-11-12T15:24:22.6386579-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `3A4041D5EA49DF15009B1F30C02EBB9C89AE3E14CFC9C4468ECBCFAEF7893ADB`.
Receipt `build/deploy/20261112-152401-6466b0ff/receipt.json`.
Qualification `build/prop-coverage-1.143.0/qualification.json`.

Single-player prop collision now uses the convex intersection of
26 model-derived support planes for Box3D and native traces. It checks
the resulting hull against its candidate corners before use. Degenerate
models retain their fallback. The visible-overhang regression verifies
that a model vertex the earlier sampling missed is now collidable and
that an adjacent empty corner remains clear.

The production builder covered the indexed vertices in 617
poses across 575 installed retail MD3s, including
42 alternate animation frames. The independent
audit also tested combined pitch, yaw and roll. Ghoul2 retail pose
coverage and interactive collision remain open.

A standalone Box3D floor test kept 4,096 initially awake props
with a 47-vertex hull through 180 solver steps.
The median 95th-percentile step time across three comparative trials was
9.5 ms on 1.142.0 and 11.5 ms on 1.143.0.
Live scene performance remains open.
This record was added after packaging.

## 1.142.0 installation (2026-11-12)

Build `20261112-150402-21589ad4` installed at `2026-11-12T15:12:06.3194713-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `10A5F93306E7C5B35B169451CC8F0FB022A138B92195FDABDBE2E4CF36FB20D0`.
Receipt `build/deploy/20261112-151141-c0983b46/receipt.json`.
Qualification `build/prop-pair-1.142.0/qualification.json`.

Native single-player movement now tests both qualified prop hulls.
Separation uses both sets of face normals and crossed edges. Rotation
continues to use conservative arc envelopes. Context and Undo check
the destination. Native drag uses convex penetration depth to back out
of allowed overlaps. The original fallback handles unqualified shapes.

The server fixture checks moving-hull dispatch, contact fractions and
box fallback. It agrees with independent checks for 8,192 oriented-box
cases and 4,096 convex placements. Interactive collision and large-scene
performance remain open.

An offline audit of 575 installed retail MD3s at frame zero found 519
whose sampled hull omitted at least one indexed vertex by more than
0.125 units. A prototype built from 26 supporting planes covered all
575 at that tolerance. Integration, posed-model qualification and
live testing remain open. The audit results are in
`build/prop-pair-1.142.0/md3-coverage.json` and
`build/prop-pair-1.142.0/md3-candidate-coverage.json`.
This record was added after packaging.

## 1.141.0 installation (2026-11-12)

Build `20261112-144601-dec24b63` installed at `2026-11-12T14:53:45.1143250-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `88199BBB055B1A5B797DC375694F8DA1836C592AEC136F571DFE327C4F1E8020`.
Receipt `build/deploy/20261112-145322-c5a33ffa/receipt.json`.
Qualification `build/map-picker-1.141.0/qualification.json`.

The single-player main and pause menus have a searchable **Maps**
grid. Select a card and choose **Start map**. JA:R uses a map's
levelshot where available and generates an isometric thumbnail from
its BSP geometry otherwise. The fixture covers both preview routes,
search, pagination, return to the pause menu, and the map launch command.

Offline checks covered 64 installed maps with usable single-player
starts, including the original Garry's Mod thumbnails for Flatgrass
and Construct and a generated Yavin 1 thumbnail. The separate map
add-on retains four movable Construct props. In-game appearance and
launch behavior remain to be checked.
This record was added after packaging.

## 1.140.0 installation (2026-11-12)

Build `20261112-135947-d1b41935` installed at `2026-11-12T14:07:54.2137694-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `E28DE6948E19E2695EEF5EC1862EDE98AECD984A8CA133482F618DA5C3AB9F91`.
Receipt `build/deploy/20261112-140732-06cd9dd2/receipt.json`.
Qualification `build/prop-rotation-1.140.0/qualification.json`.

The single-player Physgun and Rotate tool check the prop hull along
its rotation before committing the move. Native angular motion uses
the same check. Context edits and Undo test the hull at the destination.
Mounted charges remain part of the same transaction.

Fixtures cover a clear corner, an obstruction between clear endpoints,
the outer arc of a thin model, and rollback with planted charges.
A separate fixture checks 2,082,080 sampled vertex projections for
combined pitch, yaw and roll. Prop-pair clearance uses conservative
bounds. Interactive controls and large-scene performance remain open.
This record was added after packaging.

## 1.139.0 installation (2026-11-12)

Build `20261112-134412-606eece5` installed at `2026-11-12T13:52:48.3940374-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `95D9C72EB86329E7B0C56E06C9D3177258EA4A571F5691A553996A4415387BA1`.
Receipt `build/deploy/20261112-135226-5f37e7ff/receipt.json`.
Qualification `build/moving-prop-collision-1.139.0/qualification.json`.

Native single-player prop translation now passes the rotated model hull
to the host BSP and linked brush-model tracers. Rectangular brushes use
full separating-axis checks. Sloped brushes and curved patches use the
model's support distances along the native collision planes. Native
entity and mounted-charge collision also contribute to the final hit.

Fixtures cover clear corners, real brush contact, start-solid and exit
behavior, rotated inline models, sloped planes, and off-center patch
collision with inward-facing borders. Rotation clearance and moving
prop pairs retain conservative bounds. Interactive gameplay and
representative large-scene performance remain open.
This record was added after packaging.

## 1.138.0 installation (2026-11-12)

Build `20261112-132813-a31bdb0c` installed at `2026-11-12T13:36:47.0583586-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `9138F4886A88602B298F8D9350DEB3E0AFAB59FDB07B860E6721D3B012AF59FD`.
Receipt `build/deploy/20261112-133624-81f19d0f/receipt.json`.
Qualification `build/native-prop-collision-1.138.0/qualification.json`.

Single-player server and client traces that include Sandbox props use
the rotated model hull for narrow collision. Point-contents queries and
the Box3D-to-native actor contact bridge use the same geometry. The
fixtures cover an empty corner, standing-height contact, starting
inside, leaving a hull, a different rendered angle and a moving prop
that either misses an actor or transfers its momentum.

The hull is convex. Flat or otherwise unsuitable meshes use the
projected box. Native sweeps of a moving prop against BSP scenery
still use its bounds. Interactive movement, collision feel and
representative performance remain open.
This record was added after packaging.

## 1.137.0 installation (2026-11-12)

Build `20261112-131038-aeb789e5` installed at `2026-11-12T13:19:11.5411736-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
17 replacement backups.
ZIP SHA-256 `3CBA32BA0FC207E4F6390AFDD00B70A1231245DD22D51DB3E6868C71A9FDEB55`.
Receipt `build/deploy/20261112-131849-a730823e/receipt.json`.
Qualification `build/workshop-collision-1.137.0/qualification.json`.

Single-player Sandbox spawn placement allows overlapping object hulls
while checking solid host BSP geometry. Native props can move out of
an existing prop overlap. Owned objects that fall entirely below the
host map and placed sections are freed through normal Workshop cleanup.
Occupied or controlled actors retain the existing removal protections.

Native prop bounds use conservative vertex-based projections from
visible MD3 and posed Ghoul2 geometry. Box3D uses cached convex hulls
when the sampled vertices form a suitable volume. Native collision
still uses axis-aligned bounds, and degenerate Box3D shapes use a box.
The offline MD3 audit checked 575 models and
1177 poses. Interactive gameplay remains open.
This record was added after packaging.

## 1.136.0 installation (2026-11-12)

Build `20261112-125215-0961504d` installed at `2026-11-12T13:00:29.0304974-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `431E6B38A76687AF582D45FEBB9E06EDF5B45C6D481479E920E5635CF95FCA1A`.
Receipt `build/deploy/20261112-130007-d1796417/receipt.json`.
Qualification `build/rift-sanctuary-1.136.0/qualification.json`.

Rift Sanctuary is directly spawnable from the single-player Sandbox
Entities tab. Its 38 stock pickups use native item definitions, anchor
to the room, and respawn independently for each copy. Both fatal fall
triggers retain their authored geometry, including the gap in the
compound trigger.

The retail audit verifies the two trigger models and 16 entry points.
A stock brush audit checks all 38 pickup anchors. Native fixtures cover
the fall-trigger gap after rotation, isolated pickup state, native
serialization fields, and room cleanup. The host supplies the fog
and breath settings. In-game rendering, pickup interaction, and full
save/load remain open. This record was added after packaging.

## 1.135.0 installation (2026-11-12)

Build `20261112-123236-2b793433` installed at `2026-11-12T12:40:46.1951968-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `15873F3B71ECC1DEA2EA37BE89921A1C13C4024E9CF3F6C9583183F24736B6A2`.
Receipt `build/deploy/20261112-124024-c9a33242/receipt.json`.
Qualification `build/imperial-shaft-1.135.0/qualification.json`.

Imperial Shaft is directly spawnable in single-player Sandbox's
Entities tab. It includes the two native lifts with their own floor
triggers and stock sounds, the fatal fall volume, six scenery models
with stock scaling, and three ambient speakers. The section restores
the authored 0.2-second delays that the native integer parser truncates.

The retail audit checks the complete 35-child section, the two floor
pad bounds and the fall volume. Native fixtures cover target isolation
across the host and duplicate rooms, preserving an active lift's path
through a room rotation, native field serialization, and cleanup.
In-game rendering, lift riding, and full save/load remain open.
This record was added after packaging.

## 1.134.0 installation (2026-11-12)

Build `20261112-121041-4ac586a7` installed at `2026-11-12T12:18:45.4047362-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `7844FCCE8177131AB6970505071009311846EF2A7FC378D9A2E01201AFD1B684`.
Receipt `build/deploy/20261112-121822-e612ed62/receipt.json`.
Qualification `build/hoth-sky-1.134.0/qualification.json`.

Single-player Hoth Canyon uses its stock 3D sky scenery in Classic
and Rend2. The renderer uses the room's authored camera and only
selects it when the viewpoint is inside the canyon's playable BSP
area. The host keeps its native sky settings. The split applies
to the stock BSP fingerprint.

Retail checks match the 105 backdrop surfaces to the authored sky
camera's visibility set and the 522 playable surfaces to the
arena. Native and client fixtures cover independent copies,
rotation, and the choice to fall through to the host sky outside
the room. In-game visual acceptance and full native save/load
remain open. This record was added after packaging.

## 1.133.0 installation (2026-11-12)

Build `20261112-114833-baed9f00` installed at `2026-11-12T11:56:37.3845357-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
15 replacement backups.
ZIP SHA-256 `FD4472AA387CC9C482A8D637A62D0C6BFEB34BFD906AD969C4630F883FF2D671`.
Receipt `build/deploy/20261112-115616-965f1aca/receipt.json`.
Qualification `build/hoth-section-1.133.0/qualification.json`.

Single-player Sandbox now exposes Hoth Canyon in the Entities tab.
The eighth stock room brings the SP catalog to 1,181 cards and
preserves the canyon's eight local wind emitters and its fatal fall
trigger. Use the section to enter via a safe authored entry point.
Use open space for placement due to the canyon's footprint. The
room retains its authored sky camera as a passive marker, with
local 3D sky rendering still open.

Stock-byte checks exercise section admission, actual child creation
through the native section controller, movement, independent copies,
and cleanup. The native hurt fixture verifies that the flagless fall
uses SP falling damage. Interactive rendering, gameplay, and full
save/load remain open. This record was added after packaging.

## 1.132.0 installation (2026-11-12)

Build `20261112-113116-0c031880` installed at `2026-11-12T11:39:50.1718952-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
16 replacement backups.
ZIP SHA-256 `F0C5C2B9939D1FF3990973B979246AA4E5E93614244D1FDBEC462988B973D4B1`.
Receipt `build/deploy/20261112-113927-9518f1d2/receipt.json`.
Qualification `build/sandbox-input-1.132.0/qualification.json`.

Multiplayer Physgun rotation now consumes mouse motion while the
player holds the bound USE action on a confirmed grab. Local
prediction and the server keep the player's aim fixed during the
gesture. With the default bindings wheel up extends and wheel down
retracts, including on existing profiles. Single-player Sandbox
mouse clicks resolve the button beneath the stationary pointer.

Fixtures cover native prop turning, unchanged player aim, the
snapshot and command codec, wheel routing and repeated click-down.
Interactive client confirmation remains open. This record was added
after packaging.

## 1.131.0 installation (2026-11-12)

Build `20261112-111507-a69f9bb8` installed at `2026-11-12T11:23:06.0903453-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `5533B6CAC2D912D17F4F2533D5FBF81B52786312D77A50EBE8B0987ABF2FD19D`.
Receipt `build/deploy/20261112-112246-f66a9405/receipt.json`.
Qualification `build/sp-mounted-actor-1.131.0/qualification.json`.

Single-player Box3D actor corrections stage the prop and actor
responses before publishing them. The mounted charge hull is swept
through the correction and checked against staged actor movement.
A blocked charge restores the prior prop pose and discards the
pending actor responses. Native rotation and Context transforms
also check charges mounted on other props.

Fixtures cover actor and scenery rejection, successful correction,
fuse preservation and native reverse contact. Interactive gameplay
and save acceptance remain open. This record was added after packaging.

## 1.130.0 installation (2026-11-12)

Build `20261112-105938-18223cce` installed at `2026-11-12T11:07:28.8280156-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `1724166EA1794DAB30876E3D63F1FA7598C55D5204764D333DC3868CB471A537`.
Receipt `build/deploy/20261112-110709-2799b34d/receipt.json`.
Qualification `build/sp-mounted-native-1.130.0/qualification.json`.

Native single-player prop movement checks both the prop and its mounted
det packs and trip mines. The earliest hit clips the movement, with a
bounded slide along the surface for a charge contact. Props can also
hit charges mounted on other props. Native spin and Rope, Weld, and Hinge
corrections check the carried charge path before applying a new pose.

Fixtures cover earliest contact, overlap rejection and escape, mounted
contents isolation, slide velocity and portal ordering. Post-solver
actor displacement, reverse contacts in native spin, and interactive
gameplay/save acceptance remain open. This record was added after packaging.

## 1.129.0 installation (2026-11-12)

Build `20261112-103817-4f87310a` installed at `2026-11-12T10:46:23.8453660-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `CB21B2A999E23FAC4BE2FACC87D46D1AB51F7699534545287DE3CFE0704156F9`.
Receipt `build/deploy/20261112-104604-d66eba8c/receipt.json`.
Qualification `build/sp-mounted-solver-1.129.0/qualification.json`.

When single-player Box3D physics is enabled, mounted det packs and
trip mines participate in the prop body's collisions while falling,
dragging or rotating. Nearby on-foot players and NPCs interact with
the charge hull through a dedicated collision proxy. The shapes
retain the prop's mass and are derived from the existing V53 mount
records and current native lifetimes.

Fixtures cover the floor, a second prop, the actor collision filter,
rotation, mass, removal, and restoration with fresh entity lifetimes.
Native free-prop contacts, post-solver corrections, and interactive
gameplay/save acceptance remain open. This record was added after packaging.

## 1.128.0 installation (2026-11-12)

Build `20261112-102127-bd8a4a88` installed at `2026-11-12T10:29:27.7143564-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `214832DBE61431D5BD1BF6311E4C1CB9B13C31CCBD4446B0532E738837E040E0`.
Receipt `build/deploy/20261112-102909-1ff90a2c/receipt.json`.
Qualification `build/sp-mounted-pose-1.128.0/qualification.json`.

Single-player Context placement, rotation and Undo check the posed
prop hull and the destination of each mounted det pack or trip mine.
Rotate and native Physgun motion also sweep the mounted charges.
A blocked charge keeps the previous pose and leaves Undo available.
Successful moves preserve the mount, fuse, and V53 save fields and
refresh the tripwire after relinking.

Fixtures cover blocked Context edits, retryable Undo, preservation
of the solver pose on rejection, and native Physgun rotation and drag.
Continuous Box3D solver motion, rendered gameplay and full interactive
save/reload remain open. This record was added after packaging.

## 1.127.0 installation (2026-11-12)

Build `20261112-100626-445207d5` installed at `2026-11-12T10:14:37.0647554-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
16 replacement backups.
ZIP SHA-256 `7278E2C3AF097CABE2631BABFE3361617EB1BB9EC3D461A3462E8EBC157ECA5D`.
Receipt `build/deploy/20261112-101417-1f9088c6/receipt.json`.
Qualification `build/logic-marker-1.127.0/qualification.json`.

Invisible Sandbox entities in SP and MP use a larger, slightly raised
billboard with an amber rim, dark face and a bright stock reticle. The
billboard grows with view depth up to a fixed cap and uses world depth
testing. Both packaged icon archives contain the same updated shader.
The installation record was added after packaging. In-game visual
confirmation remains open.

## 1.126.0 installation (2026-11-12)

Build `20261112-095701-e91fd046` installed at `2026-11-12T10:04:59.8718295-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `D381EED742AC3CFF96215700938E6F23B3D36D6596A0A7D97E7CE7838688A359`.
Receipt `build/deploy/20261112-100439-12689092/receipt.json`.
Qualification `build/sp-mounted-cargo-1.126.0/qualification.json`.

Single-player spinners, pendulums, lifts and bobbing platforms now
check det packs and trip mines attached to cargo during movement.
A blocked charge prevents the entire step. Angular movement checks
the path and destination, including nearby carried hulls. Charges
retain their local mounting, fuse and damage credit. Tripwires update
after the platform and cargo are linked back into the world.

Fixtures cover angular and vertical obstruction, native and Box3D
pose rollback, preserved fuse and direction, and existing V53 mount
records. Rendered gameplay and full interactive save/reload remain
open. The installation record was added after packaging.

## 1.125.0 installation (2026-11-12)

Build `20261112-094118-74eaed76` installed at `2026-11-12T09:48:50.2926166-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `9B3FD18B1D77A574E964C55D1D558BACC0A339F9796D84C512B066781D129B7A`.
Receipt `build/deploy/20261112-094830-735da7f0/receipt.json`.
Qualification `build/sp-angular-cargo-1.125.0/qualification.json`.

Single-player spinners and pendulums now carry supported released
native and Box3D props and supported vehicle hulls with their pilot
and droid seats. Vehicles follow the platform heading while retaining
their native pitch, roll and momentum. The Box3D pose and native
collision hull move together. An obstructed hull or seat refuses the
step, including native rider rollback if a final check is blocked.

Fixtures cover a Box3D prop and occupied vehicle during yaw and tilt,
preflight and late rollback, external attachment rejection, native
seat ownership and preserved momentum. Live riding, mounted charge
clearance and full in-game save/reload remain open. The installation
record was added after packaging.

## 1.124.0 installation (2026-11-12)

Build `20261112-092354-fd3c2067` installed at `2026-11-12T09:31:23.5271634-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `6AD642D569F54BF00FC27CE0C6B7A9A3159AF544B6BAAB3E2B4B0D27A5D3C44A`.
Receipt `build/deploy/20261112-093104-380045e0/receipt.json`.
Qualification `build/sp-angular-movers-1.124.0/qualification.json`.

The single-player rotating and pendulum cards now expose local axis,
signed spin speed or swing amplitude and period, running, and enabled
state. Use, Activate, and links can start or pause motion. The stock crate's
projected collision hull follows its orientation. Blocked steps restore
native riders and wait at the current phase. The client reconstructs the
same projected hull for its rendered pose.

Context settings support Undo. Duplicate, linked teleport and V53 native
fields preserve the swing phase. The authored brush regression still
passes. Rotating Box3D cargo, occupied vehicles, rendered gameplay and
full in-game save/reload remain open. The installation record was added
after packaging.

## 1.123.0 installation (2026-11-12)

Build `20261112-090440-a409776c` installed at `2026-11-12T09:12:12.8230893-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
13 replacement backups.
ZIP SHA-256 `78BF502254342D6FBB34618B091D6FE4E98710B21CD8EE3D233E74E7827DF2AB`.
Receipt `build/deploy/20261112-091154-44c011e1/receipt.json`.
Qualification `build/sp-speaker-1.123.0/qualification.json`.

The single-player Speaker tab gives the existing speaker card eleven
stock sounds and five playback modes. Use or Activate plays a one-shot
or toggles a loop or repeat. A linked trigger can activate it. Repeat
has editable interval and variation. Disabling it pauses the deadline,
and enabling it resumes playback. New speakers start idle in Spatial
shot mode. Older owned speakers migrate with their repeat interval.

Undo restores settings and copies start idle. V53 native fields retain
active or paused state and the remaining interval. Authored speakers
retain native playback. Audible gameplay and full in-game save/reload
remain unverified. The installation record was added after packaging.

## 1.122.0 installation (2026-11-12)

Build `20261112-084537-e41f4998` installed at `2026-11-12T08:53:29.2672377-05:00`.
All 154 automated suites and 8 headless two-client checks passed.
Independent SHA-256 readback matched all 72 installed files and
16 replacement backups.
ZIP SHA-256 `97B8A80BAF7AA7F8177EB196D4EF848EF34DC2FF767945FB7C416BED1A493F23`.
Receipt `build/deploy/20261112-085309-83e3caf5/receipt.json`.
Qualification `build/logic-marker-1.122.0/qualification.json`.

Sandbox logic markers in single-player and multiplayer now use a thick
amber stock reticle over an opaque dark disc. The billboard radius is
32 world units up close and scales with camera depth up to 72 units.
The marker keeps its world position and ordinary world occlusion.
Both client fixtures and the packaged stock texture dependencies passed.
In-game appearance remains unverified.
The installation record was added after packaging.

## 1.121.0 installation (2026-11-12)

Build `20261112-083657-3f3f77f7` installed at `2026-11-12T08:44:28.4017366-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `B8E188F42B91182EA124208A1D37BA65CDCB1BDFF106D47993C3626B697F9588`.
Receipt `build/deploy/20261112-084409-4be2e05b/receipt.json`.
Qualification `build/sp-usable-1.121.0/qualification.json`.

The single-player func_usable card has an Usable tab for Toggle
or Pulse mode, solidity, health, damage and direct Use policies,
reuse timing, enabling and rearming. Link Primary fires on Off or
Pulse. Secondary fires after the body becomes solid.

The Off state leaves a selectable marker and suspends attached
forces until the body is solid again. Turning on waits for clear
space. The Context solidity edit checks clearance and supports Undo.
Duplicate remaps both outputs. V53 native fields preserve blocked
returns and pulse cooldown. Focused fixtures cover both native
Workshop and authored callbacks, Link, Undo, copy, attachments
and saves. In-game rendering and full save/reload remain open.
Evidence `build/sp-usable-1.121.0/fixtures.log`.
The installation record was added after packaging.

## 1.120.0 installation (2026-11-12)

Build `20261112-081239-ea477cae` installed at `2026-11-12T08:20:20.7555723-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `21629A0A3A57A26DDAE3A65F4E112AA1B2EFA6995A9BCB494E9FD9C3AC7E4E79`.
Receipt `build/deploy/20261112-081958-25a88838/receipt.json`.
Qualification `build/sp-breakable-1.120.0/qualification.json`.

The three generic SP breakable cards now have a Breakable tab for
health, fuse, weapon policy, Player Use, debris, explosion effect,
splash damage and radius. When a body breaks, it releases physical
attachments and leaves a selectable marker. Its output link can
fire again after a successful repair. Repair requires a clear hull
and can cancel an armed fuse.

Undo and Duplicate preserve breakable settings and phase. V53 save
fields retain the body, links and pending deadline. Focused fixtures
cover safe blast chains, blocked repairs, native brush regressions,
menu dispatch, copying and entity saves. In-game rendering and full
save/reload remain open. Evidence `build/sp-breakable-1.120.0/fixtures.log`.
The installation record was added after packaging.

## 1.119.0 installation (2026-11-12)

Build `20261112-075009-f8a333f1` installed at `2026-11-12T07:57:46.1743825-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
16 replacement backups.
ZIP SHA-256 `71B22EEC6E0E1DF0E24EF5D0F5FA1D7472DE31C94C4FD6973179F79A77A14130`.
Receipt `build/deploy/20261112-075726-1180957e/receipt.json`.
Qualification `build/sp-stations-marker-1.119.0/qualification.json`.

Sandbox logic markers in SP and MP are now 50% wider and sit higher
above their origins. The material makes the cyan mark opaque and adds
a dark stock-texture backing. Each marker uses one billboard and
ordinary world occlusion. Fixtures verify both clients' sprite
submission and the packaged material. Rendered gameplay remains open.

The four SP ammo and shield stations have a Station tab for capacity,
charge, recharge, refill, unlimited supply and enabled state. Recharge
defaults to off and native supply and difficulty defaults apply.
Fixtures cover native adapter scheduling, Undo, Duplicate, V53
entity fields and older owned-station migration. Full in-game
save and reload remain open.
Evidence `build/sp-stations-marker-1.119.0/fixtures.log`.
The installation record was added after packaging.

## 1.118.0 installation (2026-11-12)

Build `20261112-073046-49677705` installed at `2026-11-12T07:38:26.0184484-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `BD0CB5981A781D4A2E78D2CC921A37C72DBFF24517C3ECB15BB74AFA45F89CFD`.
Receipt `build/deploy/20261112-073807-fba4769a/receipt.json`.
Qualification `build/sp-laser-emitter-1.118.0/qualification.json`.

The single-player target_laser card now places a selectable gizmo with
a traced red beam. Open its Laser tab to edit damage and toggle
Firing or Enabled. Use or linked activation toggles firing. Primary
Link aims at a spawned entity. Clear aim to use the emitter's rotation.
Zero damage draws a harmless beam.

Undo restores edits, Duplicate remaps the aim link, and native save
fields retain the settings in V53. Older owned emitters migrate on load.
Fixtures cover player-slot-zero damage, world obstruction, held/frozen
think scheduling, menu commands, copying and native save fields.
Rendered gameplay and full in-game save/reload remain open.
Evidence `build/sp-laser-emitter-1.118.0/fixtures.log`.
The installation record was added after packaging.

## 1.117.0 installation (2026-11-12)

Build `20261112-071111-242bcc6a` installed at `2026-11-12T07:18:58.3865581-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `82FC88157E9D34AE967741C538F76DF91B1F547833A2364352A20FAF17C3E031`.
Receipt `build/deploy/20261112-071837-9527b971/receipt.json`.
Qualification `build/sp-world-anchor-teleport-1.117.0/qualification.json`.

Single-player teleport volumes can move linked groups with ropes,
hinges or welds attached to solid map geometry. Enable Props, Linked
groups and World anchors in the Teleport tab. The exit must provide
a solid surface at the matching position and angle for every anchor.
If a hull or rope route is blocked, the group and its anchors stay put.
Groups attached to the same map remain independent.

The option defaults to off. Existing joint frames and teleport settings
use the same V53 save format. Rigid pivots away from map surfaces still
prevent a move. Focused fixtures cover surface checks, rope clearance,
an unrelated anchor, Undo, Duplicate and native save fields.
Rendered gameplay and full in-game save/reload remain open.
Evidence `build/sp-world-anchor-teleport-1.117.0/fixtures.log`.
The installation record was added after packaging.

## 1.116.0 installation (2026-11-12)

Build `20261112-065531-dfb2338f` installed at `2026-11-12T07:03:11.3340907-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `404759BACC4E8D04D47C61C5E0192742ABAE857CB7B0E1DA47DE4C10D89DCB16`.
Receipt `build/deploy/20261112-070252-1cf0f5bf/receipt.json`.
Qualification `build/sp-section-teleport-1.116.0/qualification.json`.

Single-player teleport volumes can carry all seven qualified portable
map sections. Enable Props, Linked groups and Movers in the Teleport
tab. Link a section root or eligible child to the group. The section's
registered brushes, platforms, hazards, sound and effect sources, and
local entry points travel with it.

The enclosing section hull must clear the exit. Unlinked occupants or
grounded riders prevent departure. Linked riders stay supported, and
each child keeps its native state, including bobbing phase and damage.
The same V53 save fields store the resulting positions and ownership.

The retail fixtures exercised all seven sections. Focused tests also
covered child-led links, clearance, linked riders and a refused move.
Rendered gameplay and full in-game save/reload remain open. Evidence
`build/sp-section-teleport-1.116.0/fixtures.log`.
The installation record was added after packaging.

## 1.115.0 installation (2026-11-12)

Build `20261112-063943-276f66a6` installed at `2026-11-12T06:47:19.6844757-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `6185EE535BF34CE043BA94ABFA443A263672DD1D938FE507CA3E97DD03387386`.
Receipt `build/deploy/20261112-064659-596f0e99/receipt.json`.
Qualification `build/sp-binary-mover-teleport-1.115.0/qualification.json`.

Single-player linked teleport volumes can carry map-authored brush
doors, platforms and buttons. Enable Props, Linked groups and Movers
in the Teleport tab. Link one part of a supported mover team to bring
the whole team, including automatic door and platform triggers.

Every solid leaf must clear the exit. The move preserves native
motion, endpoints and trigger callbacks. A relocated door leaves
its original map opening visible. It stops adjusting area visibility
after the move. The V53 entity save preserves this state.

Focused tests cover door-team crossing, a blocked leaf, native
trigger activation, disabled triggers, repeated movement and save
fields. Mixed mover teams, active scripts, section-owned brushes
and full in-game save/reload remain open. Evidence
`build/sp-binary-mover-teleport-1.115.0/fixtures.log`.
The installation record was added after packaging.

## 1.114.0 installation (2026-11-12)

Build `20261112-062320-f51bf181` installed at `2026-11-12T06:31:00.7605888-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `1EA66E969BA250624A77866C6C74F751426F01106962986789B346E788E83782`.
Receipt `build/deploy/20261112-063040-45aa10b8/receipt.json`.
Qualification `build/sp-authored-movers-1.114.0/qualification.json`.

Single-player linked teleport volumes now carry single map-authored
train, bobbing, pendulum and rotating brushes. Enable Props, Linked
groups and Movers in the Teleport tab. A translating brush can lead a
crossing. Exit clearance uses the brush's enclosing rotated bounds.

A train's authored path corners stay in place. Its saved route offset
carries it through later legs, and bobbing brushes keep their restart
reference. Rotating and pendulum brushes retain angular trajectories.
The same V53 save format stores the route offset.

Team movers, brushes with active movement scripts, and brushes owned by
a placed map section remain outside this route. Focused tests cover
native brush crossing, rotated clearance, safe refusal, save import
and the real train and bobbing callbacks. Rendered gameplay and a full
in-game save/reload still need testing. Evidence
`build/sp-authored-movers-1.114.0/fixtures.log`.
The installation record was added after packaging.

## 1.113.0 installation (2026-11-12)

Build `20261112-060930-fe3eefd8` installed at `2026-11-12T06:17:10.9678707-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `84503976837DEA059592C02E7F09BFE91443E44F59AF8623E7ADA9B74026A848`.
Receipt `build/deploy/20261112-061652-842c0ac6/receipt.json`.
Qualification `build/sp-linked-movers-1.113.0/qualification.json`.

Single-player teleport volumes can carry linked portable sliding doors,
lifts, bobbing platforms and rail movers. Turn on Props, Linked groups
and Movers in the Teleport tab. A moving controller can lead the
crossing, and it resumes the same phase after arrival. Rail movers
carry their route offset to future trips.

Grounded riders must belong to the linked group. A blocked rider,
mover hull or rope cancels the whole move. Ordinary lift steps allow
links within the carried group. The setting and route offsets use
existing V53 save fields.

Focused tests cover swept crossing, linked and unlinked riders,
obstructed exits, phase and route preservation, Context Undo,
Duplicate and native save serialization. Rendered gameplay and full
in-game save/reload still need testing. Evidence
`build/sp-linked-movers-1.113.0/fixtures.log`.
The installation record was added after packaging.

## 1.112.0 installation (2026-11-12)

Build `20261112-055300-c347fd88` installed at `2026-11-12T06:01:01.1244816-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `74C223DABF28B7F789C31B394BCC434333355CBF9DE3963B13CA41DCA1936DD7`.
Receipt `build/deploy/20261112-060043-47dc6dc7/receipt.json`.
Qualification `build/sp-frozen-teleport-1.112.0/qualification.json`.

Single-player linked teleporter groups can carry frozen props. Turn on
Props, Linked groups and Frozen props in the Teleport tab. A moving
member leads the crossing, and the frozen props land in the same
relative pose and stay frozen. Mounted charges travel with their prop.
A blocked hull, charge or rope cancels the whole move.

The Frozen props switch defaults to off for existing teleporters and
is stored in their native save fields. Context Undo and Duplicate
preserve the setting. The V53 save format is unchanged.

Focused tests cover native and Box3D frozen props, momentum, mounted
charges, a blocked exit, Context Undo, Duplicate and native save
serialization. Rendered gameplay and a full in-game save and reload
still need testing. Evidence `build/sp-frozen-teleport-1.112.0/fixtures.log`.
The installation record was added after packaging.

## 1.111.0 installation (2026-11-12)

Build `20261112-054023-60d3a18f` installed at `2026-11-12T05:48:04.4907313-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `D70450B3E7E4626DBCDD5C61802F701399A33C29A9AE779B5CEC1F5256948222`.
Receipt `build/deploy/20261112-054744-a40407c4/receipt.json`.
Qualification `build/sp-linked-vehicles-1.111.0/qualification.json`.

Single-player teleport volumes can carry a linked vehicle and its
native pilot or droid with the rest of the group. Turn on Props, Linked
groups and Vehicles in the Teleport tab. The hull or an eligible linked
prop or on-foot actor can lead the crossing, including when a link runs
through an occupied seat.

The vehicle keeps its heading and the pilot keeps their view. Native
seat bolts determine where riders land. Clearance checks cover the
hulls, changed seat positions, mounted charges and ropes before
anything moves. The Velocity setting controls retained momentum.
The V53 save format is unchanged.

Focused tests cover hull-first and cargo-first arrivals, a link through
the pilot, missing bolts, boarding, blocked seats, Box3D movement and
momentum. Rendered gameplay and a full in-game save/reload still need
testing. Evidence `build/sp-linked-vehicles-1.111.0/fixtures.log`.
The installation record was added after packaging.

## 1.110.0 installation (2026-11-12)

Build `20261112-052530-917e9718` installed at `2026-11-12T05:33:30.8095379-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `016E9F6C4787987F2D058CB267EF428FCA15B4D96F1C1EFCBB9A72215F5E0BC4`.
Receipt `build/deploy/20261112-053311-3bff1468/receipt.json`.
Qualification `build/sp-mixed-teleport-1.110.0/qualification.json`.

Single-player Sandbox teleport volumes can move a linked group of
released props, on-foot players and NPCs. Turn on Props and Linked groups
in the C Context menu and choose an actor filter that includes every
actor in the group. Any eligible member can lead the crossing.

The move preserves relative poses and actor views. The Velocity setting
controls whether the group carries its momentum through the exit.
Every hull, mounted charge and rope must clear the exit before anything
moves. Solo portable teleport routes also respect existing links.
The V53 save format is unchanged.

Focused tests cover both crossing directions, actor filters, blocked
exits, rope clearance, momentum and repeat-entry guards. World anchors,
frozen members, linked vehicles and path movers remain outside the
supported group types. Rendered gameplay and a full in-game save and
reload still need testing. Evidence
`build/sp-mixed-teleport-1.110.0/fixtures.log`.
The installation record was added after packaging.

## 1.109.0 installation (2026-11-12)

Build `20261112-051050-0380e1d2` installed at `2026-11-12T05:18:30.6904189-05:00`
through the JA:R release scripts. All 154 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `A5C013E07D014AC2C1D0B49EF7B05E1A6AA0210CC3DF6CEDCA0939252AFB48C8`.
Receipt `build/deploy/20261112-051809-ec6a815f/receipt.json`.
Qualification `build/sp-mounted-charges-1.109.0/qualification.json`.

Det packs and trip mines planted on single-player Sandbox props now
follow their support through normal movement, rotation and teleporting.
The existing Props and Linked props settings also carry the mounted
charges. If a charge would clip at the exit, the whole move is blocked.
An underside charge raises the landing enough to clear the floor.

The native fuse, detonation credit, tripwire orientation and arming
state are preserved. V53 full saves restore the mount with fresh
entity identities. Removing the support detaches the charge at its
last position and leaves its native fuse running. Charges on authored
map surfaces keep their stock handling.

Focused tests exercise native planting, tripwire updates, detonation
credit, native and Box3D clearance, blocked assembly moves, removed
supports and save restoration. In-game placement, rendering and full
save/reload remain open. Evidence `build/sp-mounted-charges-1.109.0/fixtures.log`.
The installation record was added after packaging.

## 1.108.0 installation (2026-11-12)

Build `20261112-044946-8f7df3a4` installed at `2026-11-12T04:57:43.3767631-05:00`
through the JA:R release scripts. All 153 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `DEB90E6EB83E48D090D05BB05DC98BE6A440D12184EE98786052E59A47427995`.
Receipt `build/deploy/20261112-045726-d2e6b1f9/receipt.json`.
Qualification `build/sp-special-projectiles-1.108.0/qualification.json`.

Single-player Sandbox Teleport volumes with Projectiles enabled
now admit airborne det packs and trip mines, rolling thermals and
loose stationary thermal and flechette explosives. Each rolling
segment checks its portal crossing before the next impact, so a
bounce can send a grenade into a portal. Ownership, fuse callbacks
and native sticking remain intact. Redirected homing rockets keep
their target and steer from the new heading. Planted charges stay
attached to their original surface.

V52 preserves arrival guards for these projectile types. The focused
fixtures exercise throws, both wall/portal orders, a post-bounce
crossing, blocked exits, rolling velocity, rocket guidance state and
save data. In-game firing and full save/reload remain open.
Evidence `build/sp-special-projectiles-1.108.0/fixtures.log`.
The installation record was added after packaging.

## 1.107.0 installation (2026-11-12)

Build `20261112-043403-16f4b3d0` installed at `2026-11-12T04:41:42.9397375-05:00`
through the JA:R release scripts. All 153 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `D34C38CDDEA9A27FFD2BA0BBBD562CB6DA946303C61364C6AE2FA717B9AF4150`.
Receipt `build/deploy/20261112-044126-10ab098b/receipt.json`.
Qualification `build/sp-assembly-teleport-1.107.0/qualification.json`.

Single-player Sandbox Teleport volumes can move released linked
props. Enable Props and Linked props in the C Context Teleport tab.
When a member crosses, the assembly is moved only if every hull fits
and each rope can route around scenery at the exit. Welds, hinges,
thrusters, relative poses and optional momentum are preserved. The
first valid crossing wins, and arrival guards cover the whole group.

V51 preserves the setting, arrival guards and the new rope routes
through existing save fields and chunks. Focused tests exercise mixed
native and Box3D props, blocked placement, rope wrapping, crossing
order, Context, Undo, Duplicate and serialization. In-game assembly
movement and full save/reload remain open. World-anchored assemblies,
frozen members and links to actors or movers stay in place.
Evidence `build/sp-assembly-teleport-1.107.0/fixtures.log`.
The installation record was added after packaging.

## 1.106.0 installation (2026-11-12)

Build `20261112-041454-978cc8b4` installed at `2026-11-12T04:22:37.2255294-05:00`
through the JA:R release scripts. All 153 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
15 replacement backups.
ZIP SHA-256 `F7525CA058E315E918AD9CBC63AD2691EDB2BE1B01C1073D76EAFF80995FB420`.
Receipt `build/deploy/20261112-042220-09681690/receipt.json`.
Qualification `build/sp-vehicle-push-1.106.0/qualification.json`.

Single-player Sandbox Push volumes and markers have a Vehicles
setting in the C Context Push tab. Enable it to push a vehicle with
its riders still seated. The setting is independent of the actor
and prop filter. Aim at a nearby enabled Push or Teleport marker
and press Use while mounted to activate it. Other Use inputs keep
the vehicle's native controls.

Add keeps native throttle while Replace resets it. Horizontal
launch momentum uses the native hull sweep alongside driving.
Full V50 saves preserve that component and the controller settings.
Native tests cover four vehicle families, wall collision, command
rates, mounted marker activation, cooldowns, Context, Undo,
Duplicate and save chunks. In-game driving and full save/reload
remain open. Evidence `build/sp-vehicle-push-1.106.0/fixtures.log`.
The installation record was added after packaging.

## 1.105.0 installation (2026-11-12)

Build `20261112-035622-591df71d` installed at `2026-11-12T04:03:59.5239517-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `F44B84737F5D1F74291010A517A2EA125E0FC65491FDC8A9BD1BDC7A4E4C2C39`.
Receipt `build/deploy/20261112-040342-dd599613/receipt.json`.
Qualification `build/sp-teleport-vehicles-1.105.0/qualification.json`.

Single-player Teleport volumes and markers now have a Vehicles
setting in the C Context Teleport tab. An enabled route moves the
hull and native seated riders together after checking clearance at
each arrival position. It uses the vehicle's native seat bolts and
preserves rider ownership. The view and velocity settings control
exit heading and retained momentum. Existing routes default to
Vehicles disabled.

The setting supports Undo and Duplicate. V49 preserves it and the
arrival guards. Native fixtures cover speeder, walker, animal and
fighter seat relocation, blocked exits, constrained members,
boarding, momentum, and arrival save data. In-game interaction and
full native save/reload remain open, along with linked assemblies
and stationary or special projectiles.
Evidence `build/sp-teleport-vehicles-1.105.0/fixtures.log`.
The installation record was added after packaging.

## 1.104.0 installation (2026-11-12)

Build `20261112-033803-3bf415bc` installed at `2026-11-12T03:45:39.4343548-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `77B90CBAB992B932F2C402E71003946A644BBF5E03629C4726E6803374033BD7`.
Receipt `build/deploy/20261112-034522-743c5a84/receipt.json`.
Qualification `build/sp-teleport-objects-1.104.0/qualification.json`.

Single-player Teleport volumes can now admit released props and airborne
projectiles. Select the volume in C Context and enable Props or
Projectiles in the Teleport tab. The actor filter also has a No
actors choice for cargo or projectile-only routes.

Prop hulls and projectile flight paths are checked against the
rotated volume. The exit must fit the full object. Native and
Box3D props keep their orientation and can keep or reset momentum.
Projectiles keep their speed, owner and fuse. The view setting
chooses whether they keep their direction or follow the exit's
facing. The first reachable portal wins before a wall impact.
An arrival guard prevents an immediate return trip while the object
is inside a volume.

Settings support Undo and Duplicate. V48 full saves retain object
cooldowns and arrival guards. Native fixtures cover swept crossing,
blocked exits, trajectory and fuse preservation, object cooldowns,
native and Box3D movement and save chunks. In-game interaction and
full native save/reload remain open, along with vehicle transport,
linked assemblies and stationary or special projectiles.
Evidence `build/sp-teleport-objects-1.104.0/fixtures.log`.
The installation record was added after packaging.

## 1.103.0 installation (2026-11-12)

Build `20261112-032002-7deae449` installed at `2026-11-12T03:27:34.7945478-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `5EA36D829A57DE23282AA1303EB53585E5E0352855E94F4B73F04850A3C4D06A`.
Receipt `build/deploy/20261112-032719-5a75e2a3/receipt.json`.
Qualification `build/sp-teleport-controllers-1.103.0/qualification.json`.

Single-player Sandbox teleport sources include Teleport volume,
Teleporter and Teleport marker under Q > Entities. Each spawns with
an owned Teleport destination. Position and label the destination
in C Context, or use Primary Link to connect another owned anchor.
The Teleport tab controls actor filters, interval, view, velocity
and enabled state. The volume uses oriented hull contact. The marker
teleports its activating actor.

Arrivals require clearance for the actor's full hull and leave
occupants unharmed. Touch teleports are held until the actor exits
the arrival volume, so paired portals can face one another safely.
Undo restores settings and destination links. Duplicate remaps the
destination and starts with fresh cooldowns. V47 saves retain actor
cooldowns and arrival guards and migrate older owned teleporters.

Native fixtures cover obstructed and tilted contacts, safe and
authored relocation, reciprocal portals, settings, duplication and
full/autosave dispatch. Occupied vehicles, projectiles, props, in-game
interaction and full native save/reload remain open.
Evidence `build/sp-teleport-controllers-1.103.0/fixtures.log`.
The installation record was added after packaging.

## 1.102.0 installation (2026-11-12)

Build `20261112-025928-e18a2dba` installed at `2026-11-12T03:06:58.6344633-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `E15342A7FAE106401162915A74C0FA3982EB554CD8452CDFED4BCDD37AF17A6C`.
Receipt `build/deploy/20261112-030642-08d7d456/receipt.json`.
Qualification `build/sp-push-controllers-1.102.0/qualification.json`.

Single-player Sandbox has Push volume and Push marker under Q >
Entities. Select either with C. The Push tab controls speed,
per-subject interval, direction, affected actors, and whether the
push replaces or adds velocity. A volume can also push released
props. The marker launches its activating actor.

Use the Link tool with Primary selected to connect a live aim point.
An aim link follows the target as it moves. If it is removed, the
controller remains available and shows the missing link. The
Properties page edits the volume dimensions and orientation.

Undo restores settings and aim-link edits. Duplicate remaps owned
aim links and starts with fresh cooldowns. V46 saves retain live
cooldowns and migrate older owned push controllers. Fixtures cover
on-foot contacts, moving and missing aim points, swept prop contact,
native and Box3D velocity, clipboard remapping and save chunks.
Vehicle routing, in-game interaction and a full native save/reload
remain open. Evidence `build/sp-push-controllers-1.102.0/fixtures.log`.
The installation record was added after packaging.

## 1.101.0 installation (2026-11-12)

Build `20261112-023813-3459e104` installed at `2026-11-12T02:45:33.0086962-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `4A5C9990D2DECF5CB073FC167D593CD4380E40FF7DD453358E5C71C5C148C566`.
Receipt `build/deploy/20261112-024516-825746e2/receipt.json`.
Qualification `build/sp-hurt-trigger-1.101.0/qualification.json`.

In single-player Sandbox, spawn Hurt trigger from Q > Entities and select
it with C. Use the Trigger tab for damage per hit, per-actor interval,
normal, falling or electrical damage, player/NPC filters, and touch or
Hold Use. Arm or disarm damage from Context or through an incoming Link.
Enable and Disable entity markers control the native Use permission.
Properties provides local volume dimensions and orientation.

Context edits support Undo. Duplicate retains settings and armed state
and starts with fresh contact timers. Full Sandbox saves retain each
live actor's remaining cooldown. V45 adds the cooldown chunk, while
older saves and their legacy Hurt volumes retain a migration path.

Fixtures cover staggered contacts, tilted volumes, held Use, authored
damage behavior, freed-slot reuse, Context, copy, and full/autosave chunk
dispatch. In-game interaction and a full native save/reload remain open.
Evidence `build/sp-hurt-trigger-1.101.0/fixtures.log`.
The installation record was added after packaging.

## 1.100.0 installation (2026-11-12)

Build `20261112-021849-84a6a9f3` installed at `2026-11-12T02:26:21.7794239-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `E5E3CDABB573DD555DAAA1A7D00BE2D6B86D7974AAC4B3D1BE0A65414FFCAEC3`.
Receipt `build/deploy/20261112-022606-5f22bd93/receipt.json`.
Qualification `build/sp-contact-triggers-1.100.0/qualification.json`.

In single-player Sandbox, find Repeat trigger and Once trigger under
Q > Entities. Select a volume with C. The Trigger tab controls delay,
player/NPC filters, touch or Hold Use, enable state, and Repeat interval
and variation. A spent Once trigger can be rearmed. Resize the oriented
contact volume under Properties. Use Link to connect an output, or
Activate to fire it manually. The contact filters apply to physical
entry and held Use.

Undo restores trigger settings and rearm edits. Duplicate keeps the
settings and spent state, remaps internal links, and clears scheduled
output and cooldown in the copy. Native save fields retain phase and
settings and older Sandbox triggers are migrated on load.

Fixtures cover actor hull contact during a movement sample, oriented
corner rejection, filters, delay, cooldown, Undo, clipboard remapping,
and native field serialization. Rendered gameplay and a full native
save/reload remain open. Evidence `build/sp-contact-triggers-1.100.0/fixtures.log`.
The installation record was added after packaging.

## 1.99.0 installation (2026-11-12)

Build `20261112-015814-7e9b05ec` installed at `2026-11-12T02:05:47.7080076-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `CBE73930832E3E898C4A8A1AA389380102E87579C94D395D5CF4D3CAF94F80D3`.
Receipt `build/deploy/20261112-020531-c37f750c/receipt.json`.
Qualification `build/sp-use-gates-1.99.0/qualification.json`.

In single-player Sandbox, spawn Enable entity or Disable entity from
Q > Entities. Select it with C to choose a destination slot, inspect
its current Use state, or clear the link. Connect up to four owned
Use destinations through the Link tool. Supported contact triggers
use the same native inactive flag. A trigger can link to a marker
that disables that trigger after it fires.

Direct activation records one Undo for the destinations it changes.
Timed and linked activation can change state without adding to the
player's Undo history. Duplicate preserves supported Use states
and remaps the links. Native save fields retain the links and flag.

Fixtures cover authored multi-target behavior, ownership, touch-only
destinations, graph admission, direct and linked activation, reused
slots during Undo, clipboard remapping, and native save fields.
Interactive gameplay and a full native save/reload remain open.
Evidence `build/sp-use-gates-1.99.0/fixtures.log`.
The installation record was added after packaging.

## 1.98.0 installation (2026-11-12)

Build `20261112-014220-648b6dec` installed at `2026-11-12T01:50:02.9008141-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `74A2C89F8D9FC831C0E7655A57E9D3F0A64CF3EC4BF6CCBA47A78D6E13C04591`.
Receipt `build/deploy/20261112-014947-799f7f39/receipt.json`.
Qualification `build/sp-give-1.98.0/qualification.json`.

In single-player Sandbox, spawn Give from Q > Entities. Select the
marker with C to choose or clear one of four pickup links, inspect
its destination, and enable or disable the marker. Link connects
the chosen slot to an owned native pickup. Activate as a living
character or connect a trigger that forwards that character.

Give applies native pickup rules to each linked item. Rejected items
stay available. Linked dropped pickups stay collectible and suspend
their expiration. Removing the final Give link restarts the normal
30-second expiration. Undo can restore a cleared link, and Duplicate
rewires to the copied pickups. Native saves retain the reservation
and can recover an older rejected pickup with an unlinked world model.

Fixtures cover routing, rejection, source removal during a grant,
shared reservations, link Undo, clipboard remapping and native
save fields. Interactive gameplay and a full native save/reload
remain open. Evidence `build/sp-give-1.98.0/fixtures.log`.
The installation record was added after packaging.

## 1.97.0 installation (2026-11-12)

Build `20261112-012621-17861735` installed at `2026-11-12T01:33:52.1822403-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `DCF71C90A890BA20224F3294A75E576E27C249E6EC1D857C33A8DBEFD9DE8CC4`.
Receipt `build/deploy/20261112-013334-cba942ec/receipt.json`.
Qualification `build/sp-relay-score-kill-1.97.0/qualification.json`.

In single-player Sandbox, spawn Relay, Score or Kill from Q > Entities.
Select a marker with C. Relay lets you set delay and cooldown, choose
Once or Repeat, and rearm it. The linked output fires once per accepted
activation, and cooldown starts when that output fires. Delay changes
apply to future activations. A native save retains an existing deadline
and cooldown, while a pasted Relay starts without a pending activation
or cooldown and keeps its armed or spent state.

Score adds or subtracts signed points from the activating character
and shows the new total for the player. Kill exposes the stock Normal,
Falling and Electrical modes. Both require a living activating
character. Use Link to connect a trigger or Relay to either receiver.

Fixtures cover typed settings and Undo, due-time and cooldown gating,
stock Kill dispatch arguments, score bounds, native saved fields and
clipboard remapping. Rendered interaction and a full native save/reload
remain open. Evidence `build/sp-relay-score-kill-1.97.0/fixtures.log`.
The installation record was added after packaging.

## 1.96.0 installation (2026-11-12)

Build `20261112-010929-d254f127` installed at `2026-11-12T01:17:11.9480498-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
13 replacement backups.
ZIP SHA-256 `3EED5E034A7B3D9F3BD9679CAC50D14445E1C84F8889304336FD8D1134C9E852`.
Receipt `build/deploy/20261112-011657-a636304c/receipt.json`.
Qualification `build/sp-timer-random-1.96.0/qualification.json`.

In single-player Sandbox, spawn Timer or Random from Q > Entities.
Select the marker with C. Timer exposes the interval and variation,
start/stop, and a mute toggle. Muting preserves the scheduled cadence.
A native save retains the next tick. A pasted Timer starts stopped.

Random selects one eligible destination from up to four independently
linked outputs. The context menu selects or clears an output and shows
its destination. Once mode can be rearmed, and Duplicate preserves the
armed/spent state and links to the pasted assembly. Undo restores
settings and output edits when their destinations still exist.

The native fixtures cover restored identities, safe fallback activators,
live selection, timing and muting, cycle rejection, clipboard remapping,
and save-field serialization. Rendered interaction and a full native
save/reload remain open. Evidence `build/sp-timer-random-1.96.0/fixtures.log`.
The installation record was added after packaging.

## 1.95.0 installation (2026-11-12)

Build `20261112-005156-2dc8d5f5` installed at `2026-11-12T00:59:25.9659757-05:00`
through the JA:R release scripts. All 152 automated
suites and 8 headless two-client
checks passed. Independent SHA-256 readback matched all
72 installed files and
15 replacement backups.
ZIP SHA-256 `710706B7F71A11B6271AE8C86E375382B1B071599566424EAFE9CA246F8B28D5`.
Receipt `build/deploy/20261112-005910-da27854b/receipt.json`.
Qualification `build/sp-logic-1.95.0/qualification.json`.

In single-player Sandbox, the Delay and Counter cards are available in
Q > Entities. Select either in C Context to edit it. Delay sets the time
and optional random variation before the linked output fires. Changes
apply to the next activation. Counter sets the required hits, supports
Once and Repeat modes, and can be reset or disabled. Link connects its
Complete and Progress outputs. Duplicate keeps the settings and counter
progress. A pending delay resumes from a native save. Pasting a copy
starts with an idle Delay.

Native activation, menu and typed-input routing, links and cycle rejection,
Undo, Duplicate and native serialization checks passed. Rendered Sandbox
interaction and a full interactive save/reload remain open.
Evidence `build/sp-logic-1.95.0/fixtures.log`.
The installation record was added after packaging.

## 1.94.0 installation (2026-11-12)

Build `20261112-003238-c002b144` installed at `2026-11-12T00:40:01.8191294-05:00` through the JA:R release scripts.
All 152 automated suites passed. Independent readback verified
72 installed files, 15 replacement backups and
2197 archived source files. ZIP SHA-256 `52B5B111251871ABA1CDD6A8EFFECC7E8DD77F85F24E6718A1ACC2D0855C1BB6`.
Receipt `build/deploy/20261112-003946-229e070b/receipt.json`.
Qualification `build/sp-movement-1.94.0/qualification.json`.

SP Context now has a Movement tab for eligible on-foot actors, including
the player. Set gravity or ground friction, return to map gravity, or Undo
a property edit. The Gravity change and Friction change cards apply their
editable values to the actor activating them. Their settings and enabled
state survive Duplicate and native save/load. Legacy owned markers gain
the same controls on load. Fixtures verify native target activation, typed
edits, undo conflicts, clipboard state and native save fields.
In-game movement and a full interactive save/reload remain open.
Evidence `build/sp-movement-1.94.0/fixtures.log`.
The installation record was added after packaging.

## 1.93.0 installation (2026-11-12)

Build `20261112-001401-f878f937` installed at `2026-11-12T00:21:44.6052281-05:00` through the JA:R release scripts.
All 152 automated suites passed. Independent readback verified
72 installed files, 18 replacement backups and
2194 archived source files. ZIP SHA-256 `41679865C6AFA991160D0CA5B77266C7AB135A2D8522EE6C53C87ADA83D74F42`.
Receipt `build/deploy/20261112-002130-7b90aaae/receipt.json`.
Qualification `build/sp-bobbing-1.93.0/qualification.json`.

SP Sandbox bobbing platforms now have a Platform tab in C Context for
amplitude, cycle length, pause/resume and enabled state. They use the
same motion math as MP, the stock crate hull, and the lift's swept
rider/cargo movement. Property Undo preserves the current phase.
The native save fields and clipboard preserve the pose and phase.
Legacy owned Sandbox bobbers migrate at their saved pose.
Fixtures covered spawn, Context and native USE, rider and cargo movement,
obstruction rollback, floor recovery, native save fields, and Duplicate.
Two headless clients retested MP motion, late join and journal restore
against the packaged server and module.
Rendered riding and full in-game save/reload remain open. Evidence
`build/sp-bobbing-1.93.0/fixtures.log` and `build/sp-bobbing-1.93.0/native-wire/run-20261112-001627/bobbing-result.json`.
The installation record was added after packaging.

## 1.92.0 installation (2026-11-12)

Build `20261111-235426-8017ff00` installed at `2026-11-12T00:01:51.8795199-05:00` through the JA:R release scripts.
All 152 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2191 archived source files. ZIP SHA-256 `73E69D085EDD5C977D1C600B6669EAB645722A7444F14C967216CF0B421AD989`.
Receipt `build/deploy/20261112-000135-f4c901fc/receipt.json`.
Qualification `build/bobbing-platform-1.92.0/qualification.json`.

MP now has the Bobbing platform in the Entities tab. It uses the SP
stock crate model and starts at the low point with a 12-unit amplitude
and a four-second vertical cycle. Use or Context can pause and resume it.
Context exposes amplitude, cycle length, and enabled state. The platform
carries riders and cargo using the lift's swept transaction. Duplicate
and world save/restore preserve the position and phase.
Two dedicated-server clients observed a grounded rider on the ascent and
descent while the rider kept sending commands. Consent changes paused and
resumed the cycle. Context Undo, journal resume and recovery when the
saved cycle reaches the floor passed on the packaged server and module
in pure and non-pure modes. The pure run included late join.
Rendered movement remains open. Evidence
`build/bobbing-platform-1.92.0/native-wire/run-20261111-235535/bobbing-result.json` and
`build/bobbing-platform-1.92.0/native-wire/run-20261111-235426/bobbing-result.json`.
The installation record was added after packaging.

## 1.91.0 installation (2026-11-11)

Build `20261111-233217-3d946df7` installed at `2026-11-11T23:39:54.5618712-05:00` through the JA:R release scripts.
All 152 automated suites passed. Independent readback verified
72 installed files, 20 replacement backups and
2189 archived source files. ZIP SHA-256 `0F7B1215E591C314FE1FE6391D6A8E99780556A5D790763AAFE6798B0E8A31BF`.
Receipt `build/deploy/20261111-233940-3bd4450b/receipt.json`.
Qualification `build/mp-probes-1.91.0/qualification.json`.

MP now has the Reflection probe in the Entities tab. Select it in Context
to change Radius, toggle reflections or choose Recapture map. Use
`Play-JA-R-Multiplayer-Rend2.cmd` and enable Cubemap reflections in Graphics for the native
six-face static map capture. The marker uses a stock billboard and stays
selectable in Classic. MP has 1116 catalog entries and SP has 1180.
The wire protocol and journal format stay at their prior versions.
Two native clients verified late join, radius edits, recapture requests,
Undo, journal restore and cleanup on the packaged server and module
with pure mode on and off. Evidence
`build/mp-probes-1.91.0/native-wire/run-20261111-233536/probe-result.json` and
`build/mp-probes-1.91.0/native-wire/run-20261111-233217/probe-result.json`.
Rendered reflections and the reported SP cubemap restart still need in-game
confirmation.
The installation record was added after packaging.

## 1.90.0 installation (2026-11-11)

Build `20261111-230838-158428fd` installed at `2026-11-11T23:16:21.1362095-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 17 replacement backups and
2188 archived source files. ZIP SHA-256 `787D1F6DCC4F67672D9661412DCD9A772F1C51DCB4B5BCFCC12BB6F005BD54C8`.
Receipt `build/deploy/20261111-231607-c3edba45/receipt.json`.
Qualification `build/rope-tension-1.90.0/qualification.json`.

A taut rope now transfers tension at its bends to eligible movable Sandbox
props in SP and MP. Box3D applies the load at the contact point using the
prop's mass and inertia. Held and frozen props stay under their existing
controls, and MP checks edit permission before applying a load. The wire
and save formats are unchanged.
Two native clients observed a stock crate move and turn under tension
between fixed anchors, including a shallow wrap. The same crate stayed
stationary in slack and frozen controls. The previous deflection, overlap
and journal tests also passed on the same binaries.
Evidence `build/rope-tension-1.90.0/rope-tests.log`, `build/rope-tension-1.90.0/native-wire/run-20261111-230855/rope-contact-result.json`
and `build/rope-tension-1.90.0/native-wire/run-20261111-230838/rope-load-result.json`.
Rendered gameplay, distributed rope mass and complex concave routing remain open.
The installation record was added after packaging.

## 1.89.0 installation (2026-11-11)

Build `20261111-224650-5441243c` installed at `2026-11-11T22:54:25.9164661-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 17 replacement backups and
2188 archived source files. ZIP SHA-256 `F5E79174ACEFD473F08D7E533AA2F7E7F8F6561D08DDE43F62FE6D6112BA315B`.
Receipt `build/deploy/20261111-225412-eecbdde2/receipt.json`.
Qualification `build/rope-contacts-1.89.0/qualification.json`.

SP and MP ropes now route around moving collision hulls. The cache tracks
nearby bodies. When a collision body overlaps an anchor and no path clears,
the rope suspends tension until it can route again. MP keeps the server's
chord when client-side slack clipping would obscure it.
Two native clients observed a player and a falling physics prop deflect the
rope, anchor overlap suspend it, world save and restore succeed during
that overlap, and the rope return to its straight path on both clients.
Evidence `build/rope-contacts-1.89.0/rope-tests.log` and `build/rope-contacts-1.89.0/native-wire/run-20261111-225221/rope-contact-result.json`.
Rendered gameplay, forces on obstructing bodies and distributed cable mass remain open.
The installation record was added after packaging.

## 1.88.0 installation (2026-11-11)

Build `20261111-223007-793cc4d4` installed at `2026-11-11T22:37:27.6801545-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 15 replacement backups and
2188 archived source files. ZIP SHA-256 `476661A97F32336A7AC6552ED413B0E6B988E5D2DD7D38EE9D550693E9C076C6`.
Receipt `build/deploy/20261111-223714-307c4c92/receipt.json`.
Qualification `build/lift-vehicles-1.88.0/qualification.json`.

SP and MP lifts can carry supported vehicles and their riders. Fixtures passed
for seat integrity, consent, rider obstruction, and grounded, hovered and
landed hulls. On an isolated dedicated server, a pilot boarded a stock Swoop
through native USE input and stayed mounted for a 36-unit lift ascent and
return. A second client received the raised pilot position. The two-client
on-foot carry and saved auto-return check also passed.
Evidence `build/lift-vehicles-1.88.0/lift-tests.log`, `build/lift-vehicles-1.88.0/native-wire/run-20261111-223410/lift-result.json`
and `build/lift-vehicles-1.88.0/native-wire/run-20261111-223345/vehicle-lift-result.json`.
Rendered riding and interactive SP saves remain open.
The installation record was added after packaging.

## 1.87.0 installation (2026-11-11)

Build `20261111-221308-c49c0f92` installed at `2026-11-11T22:20:35.5410903-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 17 replacement backups and
2188 archived source files. ZIP SHA-256 `0B62AE1AC0AE610045986C00711160F3906E5160936D0043FF9C820BD68E3A7B`.
Receipt `build/deploy/20261111-222021-afa56ef7/receipt.json`.
Qualification `build/stock-lift-1.87.0/qualification.json`.

The stock floor grate replaced the single-player platform placeholder
and added a usable multiplayer lift. Fixtures passed for lift and rider
sweeps, blocked rider rollback, loose cargo, settings, both endpoint links,
and persistence. Two native MP clients received the stock hull and the
40-unit rider ascent. Server world reload retained the raised deck and
auto-return timer, and an on-foot rider descended with it.
Evidence `build/stock-lift-1.87.0/lift-tests.log` and `build/stock-lift-1.87.0/native-wire/run-20261111-221645/lift-result.json`.
Rendered riding, vehicle carriage and interactive SP saves remain open.
The installation record was added after packaging.

## 1.86.0 installation (2026-11-11)

Build `20261111-215220-7aa73992` installed at `2026-11-11T22:00:07.0543235-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 19 replacement backups and
2184 archived source files. ZIP SHA-256 `04125B4E8AB86D4FC953CF5D9D4018151FED9EBC41C3710355D9CA061D6FAF74`.
Receipt `build/deploy/20261111-215953-b07c0e8f/receipt.json`.
Qualification `build/imperial-button-1.86.0/qualification.json`.

The stock Imperial switch replaced the single-player func_button
placeholder and is now available in multiplayer. Fixtures exercised both
links, hold time and enable settings, Undo, copying and save restoration.
Both native MP clients received the same phase and fixed hull, plus the
stock press and return sound events. Reloading the server world while the
button was held retained the timer and completed the return.
Evidence `build/imperial-button-1.86.0/button-tests.log` and `build/imperial-button-1.86.0/native-wire/run-20261111-215413/button-result.json`.
Graphical gameplay and full interactive SP save/reload remain open.
The installation record was added after packaging.

## 1.85.0 installation (2026-11-11)

Build `20261111-213252-56205cb4` installed at `2026-11-11T21:40:55.3020292-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 19 replacement backups and
2180 archived source files. ZIP SHA-256 `B20004D3B6C0214C4C504380DE218B426AE3E048749AB6C974553208137A22D6`.
Receipt `build/deploy/20261111-214041-72653065/receipt.json`.
Qualification `build/sliding-door-parity-1.85.0/qualification.json`.

SP fixtures passed for the stock panel hull, Context edits and Undo,
obstruction and resume, the serialized auto-close timer, and clipboard
link remapping. The MP exact-travel edit now clears the movement sound.
A two-client test with the packaged MP module also passed the stock
bounds, player obstruction and server save/reload checks.
Evidence `build/sliding-door-parity-1.85.0/door-tests.log` and `build/sliding-door-parity-1.85.0/native-wire/run-20261111-213344/door-result.json`.
Graphical gameplay and full interactive SP save/reload remain open.
The installation record was added after packaging.

## 1.84.0 installation (2026-11-11)

Build `20261111-211345-e0fc16da` installed at `2026-11-11T21:21:09.5177126-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2176 archived source files. ZIP SHA-256 `39595A95AD49B915C9DE8B8F5ACB66AEA4C292351702227E6FBB9DA422AB42D5`.
Receipt `build/deploy/20261111-212055-eda8bec4/receipt.json`.
Qualification `build/sliding-door-1.84.0/qualification.json`.

Two clients received the stock door with its solid panel bounds. The
door stopped for a player, preserved its position through server save
and reload, and finished closing after the player moved.
Evidence `build/sliding-door-1.84.0/native-wire/run-20261111-211728/door-result.json`.
Matching SP panel appearance and Context controls, and graphical
gameplay confirmation remain open.
The installation record was added after packaging.

## 1.83.0 installation (2026-11-11)

Build `20261111-205856-2dbe7b16` installed at `2026-11-11T21:06:22.1502953-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 17 replacement backups and
2174 archived source files. ZIP SHA-256 `28F6FDAF7760B5C1BE9C003EA5A2D70B8990BA08799243E1275755C52B325CD8`.
Receipt `build/deploy/20261111-210610-d82db2a1/receipt.json`.
Qualification `build/portable-wall-1.83.0/qualification.json`.

Two clients received the stock solid wall and the selectable off gizmo.
The server refused Undo while a player occupied the wall's hull and
restored solidity after the player moved. The off state survived world
reload and could be activated again. Evidence `build/portable-wall-1.83.0/native-wire/run-20261111-210023/wall-result.json`.
Graphical and SP full-save gameplay confirmation remain open.
The installation record was added after packaging.

## 1.82.0 installation (2026-11-11)

Build `20261111-204002-df2f1e94` installed at `2026-11-11T20:47:30.2168151-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2171 archived source files. ZIP SHA-256 `41347F57C5D90C91A387E2580CB86B1ECF5EF5CB38D6AF55D1E0E1F588326388`.
Receipt `build/deploy/20261111-204717-6e62a8cc/receipt.json`.
Qualification `build/mp-distance-1.82.0/qualification.json`.

The exact dedicated build published both feedback branches to two clients,
accepted a movable reference through the Link tool, and fired when a
different player was in range. The spent state, range and reference were
restored from the world journal. Evidence `build/mp-distance-1.82.0/native-wire/run-20261111-204420/distance-result.json`.
Graphical and SP full-save gameplay confirmation remain open.
The installation record was added after packaging.

## 1.81.0 installation (2026-11-11)

Build `20261111-202453-335dfc9d` installed at `2026-11-11T20:32:32.5390480-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 15 replacement backups and
2170 archived source files. ZIP SHA-256 `8308CD9A61F0CCD8B3C73F51CE91D2C54FA923A3BC992A0C729242747F58C6A4`.
Receipt `build/deploy/20261111-203218-f7c87d44/receipt.json`.
Qualification `build/distance-trigger-1.81.0/qualification.json`.

Five focused SP suites cover the entity-distance controller, Context,
Link, and duplication. Their log is
`build/distance-trigger-1.81.0/focused-ctest.log`.
Graphical and full-save gameplay confirmation and MP distance parity remain open.
The installation record was added after packaging.

## 1.80.0 installation (2026-11-11)

Build `20261111-200649-6dfc141e` installed at `2026-11-11T20:14:02.8723351-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 18 replacement backups and
2168 archived source files. ZIP SHA-256 `DEB188A7DAC5A4B30121F36211B05C663E1F3CEDAF20572EF1A76DBE0024A179`.
Receipt `build/deploy/20261111-201347-93c5ba7c/receipt.json`.
Qualification `build/visible-trigger-1.80.0/qualification.json`.

On the exact dedicated build, two clients saw the gizmo and received one
Primary pulse per activation. The rearmed marker waited while both looked
away, fired when the second client looked at it, and restored its spent
state and edited range and angle. Evidence `build/visible-trigger-1.80.0/native-wire/run-20261111-200825/visibility-result.json`.
Graphical and SP full-save gameplay confirmation remain open.
The installation record was added after packaging.

## 1.79.0 installation (2026-11-11)

Build `20261111-194601-62c81289` installed at `2026-11-11T19:53:26.4147308-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 18 replacement backups and
2164 archived source files. ZIP SHA-256 `AF15E0E8399B5018FA2FF93A36AEC65433A793FFEDC73DA508DF60E146EC1DC2`.
Receipt `build/deploy/20261111-195314-2cbb44b6/receipt.json`.
Qualification `build/start-markers-1.79.0/qualification.json`.

The exact dedicated server used restored player-start markers for late
join and death respawn in both FFA and Team Deathmatch. An occupied
respawn marker fell back to the map's start selection. Evidence
`build/start-markers-1.79.0/native-wire/run-20261111-194912/start-result.json` and
`build/start-markers-1.79.0/native-wire/run-20261111-195003/start-result.json`.

SP fixtures exercised both start cards, clear and obstructed arrival,
mounted-player refusal, and label editing. Graphical and SP full-save
gameplay confirmation remain open.
The installation record was added after packaging.

## 1.87.0 Sandbox lift (2026-11-11)

Lift checks cover the retail bounds and audio assets, editable Context
properties, both endpoints, carriage and atomic obstruction refusal, saved
progress and the placement of an elevated copy. Graphical and full interactive
single-player save and reload remain open.

## 1.86.0 Sandbox Imperial button (2026-11-11)

Button checks cover stock assets and aggregate bounds, native activation,
Context limits and Undo, phase and delay restoration, linked outcomes and
adjacent client snapshot interpolation. Interactive graphical and full SP save
verification remain open.

## 1.85.0 Sandbox sliding door parity (2026-11-11)

The single-player fixtures cover the stock hull, obstruction retry, validated
Context settings and Undo, native save serialization of auto-close time, and
duplicate remapping of the Closed output. The multiplayer fixture covers
stopping the sound when the travel edit lands at the current extension.

## 1.84.0 Multiplayer sliding door (2026-11-11)

The server fixture checks the stock-model hull path, both arrival
outputs, obstruction retry, property Undo during motion, and an
auto-close timer across world restore. The Context fixture checks
range, speed, delay, direction, activation and enable controls.

## 1.83.0 Portable wall (2026-11-11)

SP and MP fixtures cover wall state, blocked reactivation, retryable
Context Undo, activation controls and duplication. The MP fixtures also
check the existing world journal codec, the dormant body state and the
Context controls.

## 1.82.0 Multiplayer distance trigger (2026-11-11)

Multiplayer suites cover helper allocation rollback, the exact distance
boundary, both feedback outputs, rearming, linked NPC movement, subject
validation, input lifetime, Undo, copy translation and journal
validation. The Context fixture checks bounded range and all Link roles.

## 1.81.0 Entity-distance trigger (2026-11-11)

SP fixtures check the two feedback outputs, the strict range boundary,
rearming, a relocated reference, a missing reference, the authored spawn
path, native entity serialization, Link cycle and subject validation,
Context Undo and independent assembly duplication.

## 1.80.0 Visibility trigger (2026-11-11)

SP fixtures check spawn, view direction, visibility set, obstruction,
Force Sight, rearming and native entity serialization. MP fixtures also
check nonblocking tool selection, spectators, the actual viewing
activator, property validation, Undo, duplication and world restore.
The MP menu fixture checks the full range and half-angle inputs.

## 1.79.0 Player-start markers (2026-11-11)

SP fixtures check authored and owned start identity, label editing and Undo,
the actual on-foot arrival hull, obstruction and mounted-player refusal.
MP fixtures cover opt-in entry and respawn selection, clear and obstructed
positions, gametype checks, tool selection, visit and teleport links, and
restored labels and eligibility. The menu fixture exercises the corresponding
commands. SP full-save gameplay and on-screen confirmation remain open.

## 1.78.0 installation (2026-11-11)

Build `20261111-192606-15b46190` installed at `2026-11-11T19:34:21.3558212-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 17 replacement backups and
2161 archived source files. ZIP SHA-256 `65CBF725D6E1FA0B41F1289B146284166B6CD1EC1432F533A1DE7CF0A2BE03F8`.
Receipt `build/deploy/20261111-193410-7f004052/receipt.json`.
Qualification `build/logic-gizmos-1.78.0/qualification.json`.

The exact dedicated server published gizmo tags for the one-shot and its
feedback helper, and an ordinary model tag for the same green crate as a
physical prop. Both peers and a late joiner saw the correct saved/restored
tags. Restoring the pending one-shot fired exactly once for both peers.
Evidence `build/logic-gizmos-1.78.0/native-wire/run-20261111-192810/marker-result.json`.

Native tests exercised the marker classifier and sprite submission. Both
clients use a stock texture through the same packaged shader. Graphical
confirmation remains open.
The installation record was added after packaging.

## 1.77.0 installation (2026-11-11)

Build `20261111-191734-0218c951` installed at `2026-11-11T19:24:40.6977026-05:00`.
All 151 suites passed and 72 installed files matched the package manifest.
ZIP SHA-256 `42089499ADCBBA2AEED48DDB56176C5F29F025BDFC70F1B41465C93F805EDDCA`.
Receipt `build/deploy/20261111-192430-bdd5688f/receipt.json`.
Two clients received the automatic spawn pulse and one additional pulse
after the pending save was restored. Evidence
`build/always-trigger-1.77.0/native-wire/run-20261111-192418/always-result.json`.

## 1.77.0 Portable one-shot trigger (2026-11-11)

Native tests cover automatic and rearmed output, preserved authored behavior,
bounded delay editing and Undo, and pending or spent states through clipboard
and saves. Two native UDP clients received the spawn pulse and exactly one
more pulse after a pending journal was restored. Evidence
`build/always-trigger-1.77.0/native-wire/run-20261111-192418/always-result.json`.
SP full-save gameplay and graphical confirmation remain open.

## 1.78.0 Map logic gizmos (2026-11-11)

Fixtures exercise SP crate-placeholder classification, MP snapshot tags on
spawn and restore, and native sprite submission with the ordinary model
fallback. The package check verifies the shared shader resolves to a stock
texture. On-screen confirmation remains open.

## 1.76.0 installation (2026-11-11)

Build `20261111-190008-c669b0e9` installed at `2026-11-11T19:07:28.2038833-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2155 archived source files. ZIP SHA-256 `76231E876FA1D5316753E4A0AF73CF6749F8D5EA2E6439DB780AF3A34DF52512`.
Receipt `build/deploy/20261111-190716-7c52fefb/receipt.json`.
Qualification `build/location-volume-1.76.0/qualification.json`.

The exact dedicated server reported the volume label in team chat and the
team overlay when the player's hull overlapped a rotated volume with the
player's center five units past the edge. A nearby miss used the point label. The run
also verified late join, cleanup and the three catalog IDs in the world save.
Evidence `build/location-volume-1.76.0/native-wire/run-20261111-190210/location-result.json`.

Single-player fixtures exercised label and dimensions together, Undo,
Duplicate, legacy native save defaults and restoration of edited dimensions.
Graphical and full-save gameplay confirmation remains open.
The installation record was added after packaging.

## 1.76.0 Portable location volumes (2026-11-11)

Native fixtures cover rotated edge contacts, a nearby rotated miss, stock
brush lookup priority, old save defaults, and restoration of edited local
bounds from native save fields. Context and Duplicate fixtures cover label
and dimensions together, independent Undo, rotation, and preserved marker
contents. The menu fixture verifies that the simultaneous controls do not
overlap. In-game graphical and save/load acceptance remain open.

## 1.75.0 installation (2026-11-11)

Build `20261111-184121-6a144329` installed at `2026-11-11T18:48:17.2468666-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2154 archived source files. ZIP SHA-256 `B6FEFD41CAC205913D5D12ECE1A918FF6203AF363E0BA61621F5E0F4D5E4C7AC`.
Receipt `build/deploy/20261111-184807-8e2b9142/receipt.json`.
Qualification `build/mp-location-1.75.0/qualification.json`.

The exact dedicated server sent both portable labels through native team
chat and team overlay messages. A late joiner received the current
location strings. Cleanup removed the portable labels and retained the
original map label. The same run spawned the skeletal prop and saved all
three original catalog IDs. Evidence `build/mp-location-1.75.0/native-wire/run-20261111-184444/location-result.json`.

The two focused suites also exercised rotated volume bounds, property Undo,
and journal restoration. The installation record was added after packaging.

## 1.74.0 installation (2026-11-11)

Build `20261111-182502-0ecbecbe` installed at `2026-11-11T18:39:28.2154292-05:00` through the JA:R release scripts.
All 151 automated suites passed. Independent readback verified
72 installed files, 21 replacement backups and
2152 archived source files. ZIP SHA-256 `E97B4D0DB8C974FD926E26454CDC472A55D4C90336FE0DEFE3FDC31284444445`.
Receipt `build/deploy/20261111-183917-3346faa9/receipt.json`.
Qualification `build/sp-cubemaps-1.74.0/qualification.json`.

Two single-player dumps tied the Cubemaps crash to a video restart that
unloaded the renderer before client-game reflection cleanup. The restart
regression reproduced the null callback with the archived code. In this build,
reflection and weather cleanup finish before renderer unload. Evidence is in
`build/sp-cubemaps-1.74.0/dump/`.

The exact multiplayer module and server replicated both E-11 firing modes,
their stock muzzle and sound events, and both model cards to two clients.
All three catalog IDs survived world save. Evidence `build/mp-native-catalog-1.73.0/native-wire/run-20261111-182539/e11-catalog-result.json`.
The earlier room fixes are also included. On-screen and audible playback
confirmation remains open. The installation record was added after packaging.

## 1.69.0 installation (2026-11-11)

Build `20261111-172043-b9432c68` installed at `2026-11-11T17:27:44.2831496-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2149 archived source files. ZIP SHA-256
`768F21AF2072DA58863A7C01D6CB63AFF2A80EB4A6EB53E553272010AB03B721`.
Receipt `build/deploy/20261111-172736-e02917aa/receipt.json`.
Qualification `build/mp-crystal-1.69.0/qualification.json`.

Single-player fixtures passed for live room publication, card identity,
legacy ownership recovery, and inline model registration. The same room
controller fixtures passed on all seven exact retail BSPs.
Evidence `build/jar/Release/retail-bsp-admission-20261111-172135-399566/result.json`.
The multiplayer crystal crate and teleporter passed server, menu, and
two-client headless tests. The crate retained weapon hits in shot-only
mode, reconstructed its attached crystal on copy and restore, and fired
Primary once per destruction. The teleporter card routed the walking
player to the same destination on both clients.
Evidence `build/mp-crystal-1.69.0/native-wire/run-20261111-171713/crystal-crate-result.json` and `build/mp-crystal-1.69.0/native-wire/teleporter-run-20261111-171713/misc-teleporter-result.json`.
The owner also reported invisible rooms on the regular multiplayer
launcher. The server snapshots carry the room shells and their BSP keys.
The multiplayer graphical issue remains open.

Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, audio playback, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.68.0 installation (2026-11-11)

Build `20261111-143348-28f8aa5a` installed at `2026-11-11T14:42:02.5360719-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2145 archived source files. ZIP SHA-256
`97FB7E5F543706EBFF2CCBDBE2B6E7A65BCB2F64E76C0F0FA2B8692DD8859D51`.
Receipt `build/deploy/20261111-144121-f6e471c3/receipt.json`.
Qualification `build/mp-welder-1.68.0/qualification.json`.

Focused Sandbox and menu fixtures covered Context bounds, rotated pulses,
clipping, consent, zero-damage sparks, duplicate, JRW18 and safe entity reuse.
Two native clients observed stock sparks and the electrical loop, both
stock and edited pulse intervals, muted states, fresh duplicate and saved
deadline restore. Evidence `build/mp-welder-1.68.0/native-wire/run-20261111-143556/welder-result.json`.
A second two-client test verified clipping against a solid crate,
non-owner selection denial and native damage only after the target granted
tool consent. Evidence `build/mp-welder-1.68.0/native-wire/wall-damage-run-20261111-143505/welder-wall-damage-result.json`.
Native USE switched the welder off and on for the owner. With shared
editing disabled, another player's USE registered without switching it.
Evidence `build/mp-welder-1.68.0/native-wire/use-run-20261111-143811/welder-use-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, audio playback, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.67.0 installation (2026-11-11)

Build `20261111-141702-20321b41` installed at `2026-11-11T14:25:06.0479605-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2141 archived source files. ZIP SHA-256
`29E61AEECDA8BBFAF7D3D31051B54DE51719F5405C73522754E09B049FF7E119`.
Receipt `build/deploy/20261111-142426-68bd921f/receipt.json`.
Qualification `build/mp-door-camera-1.67.0/qualification.json`.

The Sandbox, menu and client fixtures covered scoped part selection,
Physgun rotation, stock shaft scale, obstruction, Context settings, copy,
Undo and journal restore. Two native clients saw the stock motion and servo
events, and restored a moving camera without resetting its extension.
Evidence `build/mp-door-camera-1.67.0/native-wire/run-20261111-141541/door-camera-result.json`.
A separate two-client test verified that a real crate blocked the head
and that the camera automatically resumed after Undo removed the crate.
Evidence `build/mp-door-camera-1.67.0/native-wire/obstruction-run-20261111-141627/door-camera-obstruction-result.json`.
Native USE on the shaft and head also passed with shared editing off.
The owner could actuate both parts, and another player could not.
Evidence `build/mp-door-camera-1.67.0/native-wire/use-run-20261111-141914/door-camera-use-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.66.0 installation (2026-11-11)

Build `20261111-135801-2d62430c` installed at `2026-11-11T14:06:27.7512509-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2135 archived source files. ZIP SHA-256
`06AFF6B4535EC8C87E19FCF7A6A00DC0EF907263B3B45D3D264570AC987596DE`.
Receipt `build/deploy/20261111-140545-0b4ef4c1/receipt.json`.
Qualification `build/mp-ammo-rack-1.66.0/qualification.json`.

The Sandbox, menu and client fixtures covered the five native pickups,
shelf poses, clip policy, saved shelf state, Undo and spawn rollback.
Two native network clients verified the rack through pickup, copy, restore
and late join. Evidence `build/mp-ammo-rack-1.66.0/native-wire/run-20261111-140103/ammo-rack-result.json`.
A separate two-client run exercised Force Push, Pull and Undo. Evidence
`build/mp-ammo-rack-1.66.0/native-wire/force-run-20261111-135808/ammo-rack-force-result.json`.
The trace test verified a native E-11 shot hit the rack and native USE
reached the usable entity behind it. Evidence
`build/mp-ammo-rack-1.66.0/native-wire/trace-run-20261111-140104/ammo-rack-trace-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.65.0 installation (2026-11-11)

Build `20261111-134100-f14f6950` installed at `2026-11-11T13:49:23.7945525-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2134 archived source files. ZIP SHA-256
`FE85325072CD2F2EA31050E1CBDF16A8EECFC100373FD4545CC54473D781B989`.
Receipt `build/deploy/20261111-134841-80fa4e8d/receipt.json`.
Qualification `build/mp-gun-rack-1.65.0/qualification.json`.

The Sandbox, menu and client fixtures covered stock poses, one-shot
pickups, saved shelf state, Undo and spawn rollback. Two native network
clients verified the same rack through pickup, copy, restore and late join.
Evidence `build/mp-gun-rack-1.65.0/native-wire/run-20261111-134548/gun-rack-result.json`.
A separate two-client run exercised native Force Push and Pull and
removed the loose stock with Undo. Evidence `build/mp-gun-rack-1.65.0/native-wire/force-run-20261111-134405/gun-rack-force-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.64.0 installation (2026-11-11)

Build `20261111-132240-d1ed3074` installed at `2026-11-11T13:30:47.3094759-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2129 archived source files. ZIP SHA-256
`1FB5118A9E7E711A4E65174FD2D1234BEF39A81DB36015C6A40F6C37FD84A827`.
Receipt `build/deploy/20261111-133005-4c49932e/receipt.json`.
Qualification `build/mp-small-cargo-1.64.0/qualification.json`.

The Sandbox and menu fixtures covered heavy-only damage, pending restore,
linked destruction, native pickup ownership, cleanup, exhaustion and the
small cargo card's Context actions. The exact packaged server ran two
network clients through the damage, pickup and restore paths. Evidence
`build/mp-small-cargo-1.64.0/native-wire/run-20261111-132248/small-cargo-result.json`.
Both clients also saw the stock drop expire at 30 seconds and saw Undo
remove a live drop while preserving the earlier E-Web. Evidence
`build/mp-small-cargo-1.64.0/native-wire/drop-lifecycle-run-20261111-132248/small-cargo-drop-lifecycle-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.63.0 installation (2026-11-11)

Build `20261111-125722-8f479de6` installed at `2026-11-11T13:05:45.5535304-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2127 archived source files. ZIP SHA-256
`D7E121FAD5E44E0DC5FD1F56C77A6077F676C93F9A1E1DE86B82534147F12393`.
Receipt `build/deploy/20261111-130504-05281857/receipt.json`.
Qualification `build/mp-animated-1.63.0/qualification.json`.

The Sandbox, menu and presentation fixtures covered full-frame collision,
linked disarm, blocked deployment, pose and shader timing, saved phases and
hidden selection. The exact packaged server ran both devices with two network
clients, including pause, hide and reload. Evidence
`build/mp-animated-1.63.0/native-wire/run-20261111-125757/animated-result.json`.
A separate two-client run verified hidden selection and player clearance
before deploying. Evidence `build/mp-animated-1.63.0/native-wire/run-20261111-125759/beacon-hidden-reveal-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.62.0 installation (2026-11-11)

Build `20261111-123413-32335b3f` installed at `2026-11-11T12:42:49.5374543-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2121 archived source files. ZIP SHA-256
`A9D996D604D57F49C3988F297CD5BF9FA00E9D42E09D797F19E603CE93257E4B`.
Receipt `build/deploy/20261111-124206-ba2c8773/receipt.json`.
Qualification `build/mp-shooters-1.62.0/qualification.json`.

The Sandbox and menu fixtures covered aim, both fire modes, repeat timing,
permissions, save and duplication. The exact packaged server ran all three
shooters in a private two-client test with stock effects, native trajectories,
and linked repeat save, reload and stop. Evidence
`build/mp-shooters-1.62.0/native-wire/run-20261111-123738/shooters-result.json`.
The same native rocket impact damaged a nearby player only when their
personal tool consent was enabled. Evidence `build/mp-shooters-1.62.0/native-wire/run-20261111-123741/shooter-rocket-consent-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.61.0 installation (2026-11-11)

Build `20261111-121104-d4a13b88` installed at `2026-11-11T12:19:23.2746544-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2117 archived source files. ZIP SHA-256
`99674D0580699F002C224627D8EE46FA17376C8B3142AAD6E9149EE0F0F432F0`.
Receipt `build/deploy/20261111-121844-e81ff24c/receipt.json`.
Qualification `build/mp-curated-catalog-1.61.0/qualification.json`.

The native dedicated-server gate spawned, saved, restored and cleared all
34 new multiplayer cards. Two network clients verified representative menu
spawns and persistence. The stock Kejim mesh and texture match the original.
Evidence `build/mp-smoke-20261111-121407/curated-catalog-result.json`,
`build/mp-curated-catalog-1.61.0/native-wire/run-20261111-121156/curated-catalog-result.json` and
`build/mp-curated-catalog-1.61.0/kejim-equivalence.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.60.0 installation (2026-11-11)

Build `20261111-115403-3c1a76ce` installed at `2026-11-11T12:01:34.4229154-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2116 archived source files. ZIP SHA-256
`2BE07EBA090E0955F7DC53D9F3B4576896451AAA06BEF8E0CD5A712EEB96D294`.
Receipt `build/deploy/20261111-120053-fe53b392/receipt.json`.
Qualification `build/mp-explosives-1.60.0/qualification.json`.

The Sandbox and menu fixtures exercised stock defaults, bounded property
edits, linked actions, consent, gas pulses and journal restoration.
The packaged dedicated-server test exercised both objects using two clients.
Evidence `build/mp-explosives-1.60.0/native-wire/run-20261111-115504/explosives-result.json`.
At the same position, a player's crate blast damage depended on their
personal tool consent. Evidence `build/mp-explosives-1.60.0/native-wire/run-20261111-115934/explosives-blast-ownership-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.59.0 installation (2026-11-11)

Build `20261111-113916-c5a9251c` installed at `2026-11-11T11:46:48.1928314-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2115 archived source files. ZIP SHA-256
`5C3CD38F97373754AE3D8725BECC815E44065BC4AFD31E36CBB79858D7BFBAB9`.
Receipt `build/deploy/20261111-114607-c8a87f46/receipt.json`.
Qualification `build/mp-eweb-1.59.0/qualification.json`.

The fixtures checked stock bounds, ammo and yaw edits, mounted protections,
pending destruction and wreck repair. On the packaged dedicated server,
two clients saw native E-Web fire from both attack buttons and the ammo
decrement. A second player mounted the restored gun. The stock inventory
E-Web also deployed and packed away in the same test. Evidence
`build/mp-eweb-1.59.0/native-wire/run-20261111-114105/eweb-result.json`.
Both clients also saw an E-11 hit destroy the gun and Context Repair
restore it with the same ammo. Evidence `build/mp-eweb-1.59.0/native-wire/run-20261111-114312/eweb-wreck-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.58.0 installation (2026-11-11)

Build `20261111-111935-edb0cc95` installed at `2026-11-11T11:27:10.0881764-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2113 archived source files. ZIP SHA-256
`4D8BCA6F40156E05D1B2A3593151218CE82657E3A998EA88122ADFC9CDDF9A91`.
Receipt `build/deploy/20261111-112629-cf6cd248/receipt.json`.
Qualification `build/mp-emplaced-gun-1.58.0/qualification.json`.

The fixtures checked gun state and collision bounds, typed context settings,
occupied protections, parked autosave, pending destruction and wreck repair.
On the packaged dedicated server, two clients observed the same native
mount, projectile and dismount. A second player mounted the restored gun,
and their disconnect left it vacant for the remaining client. A separate
test confirmed that operator death clears the seat. Evidence
`build/mp-emplaced-gun-1.58.0/native-wire/run-20261111-112019/emplaced-gun-result.json` and `build/mp-emplaced-gun-1.58.0/native-wire/run-20261111-112150/emplaced-death-result.json`.
Eight two-client loopback cases passed across pure and non-pure servers.
Graphical, remote-player and load validation remain open. The installation
record was added after packaging.

## 1.57.0 installation (2026-11-11)

Build `20261111-105543-8127b664` installed at `2026-11-11T11:03:17.0077058-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2107 archived source files. ZIP SHA-256
`545DC8CAF92FD73C2BD4017058DE39A4A6CC2B9D328A57E870D5F339F2CDA5FC`.
Receipt `build/deploy/20261111-110235-ebbcc5b9/receipt.json`.
Qualification `build/mp-cargo-barrel-1.57.0/qualification.json`.

The native fixtures checked stock asset loading, immediate and timed
detonation, linked output, damage permissions, chain safety, and pending-fuse
copy and journal restore. On the packaged dedicated server, an E-11 armed a
barrel and two clients received its thermal effect and removal. Reloading the
pending journal reproduced the blast. Evidence `build/mp-cargo-barrel-1.57.0/native-wire/run-20261111-105624/cargo-barrel-result.json`.
Eight two-client loopback cases passed across pure
and non-pure servers. Graphical, remote-player and load validation remain
open. The installation record was added after packaging.

## 1.56.0 installation (2026-11-11)

Build `20261111-103912-00c62bc5` installed at `2026-11-11T10:46:27.0947552-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2106 archived source files. ZIP SHA-256
`4E5D32E9A94A855CDD6EACBACAE24A8804F5187865ABF8ADCA035544F675C500`.
Receipt `build/deploy/20261111-104546-d1f16fbe/receipt.json`.
Qualification `build/mp-vehicle-teleport-1.56.0/qualification.json`.

The native server, client prediction and snapshot codec fixtures cover four
vehicle families, exact hull and seat arrivals, grant denial and rollback.
An isolated dedicated server using the packaged DLL verified native swoop
boarding, the UDP pilot grant, the exact arrival, and seat retention.
Evidence `build/mp-vehicle-teleport-1.56.0/native-wire/run-20261111-104127/pilot-grant-result.json`.
Eight two-client loopback cases also passed across pure and non-pure server
configurations. Graphical driving, remote-player and load validation remain
open. The installation record was added after packaging.

## 1.55.0 installation (2026-11-11)

Build `20261111-102045-c8ee0efa` installed at `2026-11-11T10:28:09.6771463-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2106 archived source files. ZIP SHA-256: `880A34EE91FED65D5BFC1F81B5793BFC0F00A4EC4AE5D9BCA9AF1F889CB2F05D`.
Receipt: `build/deploy/20261111-102730-b5dd724a/receipt.json`. Qualification: `build/mp-city-1.55.0/qualification.json`.

The City and Vjun test verified 69 independent brush entities, six speakers,
cumulative model indices 1 and 71, late join and independent Undo on pure
and non-pure servers. The server fixture verified rotated native breaks,
pristine copies, malformed JRW19 rejection and independent damage restore.
Evidence: `build/mp-smoke-20261111-102547/network-client-result.json`,
`build/mp-smoke-20261111-102424/network-client-result.json` and
`build/mp-city-1.55.0/lifecycle.log`.
A headless test using the exact game DLL destroyed one brush with an E-11
shot, wrote a native JRW19 save, cleared the server, and reloaded both rooms.
Both clients received 68 surviving City brushes. Evidence:
`build/city-native-damage/run-20261111-102336/network-client-result.json` and `build/city-native-damage/run-20261111-102336/JediAcademyRemixedMP/jar/worlds/city_wire.0.jrw`.
The MP Box3D fixture also checked precise City brush proximity and stale
proxy turnover at its 1,024-body cap.
Graphical and audio playback, remote-human and load acceptance remain open.
These receipt notes were added after packaging.

## 1.75.0 Multiplayer location labels and skeletal map prop (2026-11-11)

The Entities tab adds **Location marker** (`target_location`), **Location
volume** (`trigger_location`) and **Skeletal map prop** (`misc_model_ghoul`).
The skeletal prop uses the stock R2-D2 model with posed collision, Context
and the existing Physgun and Duplicate controls.

Markers name an area for native team chat and the team overlay. A containing
volume takes precedence, then the nearest visible marker. A portable point
can serve as a fallback when no point is visible. Use Context to edit the
name, chat color, Enabled state and the volume half-size. Context outlines
the rotated volume. Rename, size, color and Enabled support property Undo.
Copy and world saves preserve settings and placement. Removing a location
frees its native index without renumbering authored locations.

Feature 79 brings the multiplayer catalog to 1,106 choices. Protocol 1051
and the existing journal formats remain in use.

## 1.74.0 Single-player graphics restart crash (2026-11-11)

Single-player video restarts now shut down the UI and client game while the
renderer is still available. That lets the client game clear its reflection
probes and weather before the renderer unloads. Two dumps from enabling
Cubemaps showed the same call through the cleared reflection export.
The Cubemaps option continues to use Rend2's reflection capture path.

This release includes the 1.72.0 room fixes and the 1.73.0 multiplayer
E-11 and map-model cards.

## 1.73.0 Multiplayer E-11 shooter and map-model cards (2026-11-11)

The multiplayer Entities tab adds **E-11 shooter** (`misc_weapon_shooter`),
**Map model** (`misc_model`), and **Static map model** (`misc_model_static`).
Both model cards use the stock Imperial crate and start frozen. Context,
Physgun and Duplicate work through the existing physical prop controls.
The original class names remain searchable.

The E-11 uses native blaster bolts with the single-player Normal damage and
spread. Use Context to fire once, start or stop repeating, select primary or
alternate fire, and set the repeat interval and random delay. Link Primary
to aim at a spawned entity. Copy and world saves preserve the aim link and
firing settings. The muzzle uses the retail effect with the embedded firing
cue and unsupported force-feedback reference removed. The server sends one
stock audio cue for the selected mode.

The multiplayer catalog has 1,103 cards. Feature 78 keeps protocol 1051 and
the existing journal formats. The portable room fixes from 1.72.0 are included.

## 1.72.0 Rend2 portable room visibility (2026-11-11)

Single-player and multiplayer Rend2 now include visible portable brush
models when calculating the view's far plane. Host map distance limits still
apply. This also applies to mirror views and leaves the menu projection
unchanged. A probe using the installed single-player Rend2 DLL reproduced
full clipping of the stock Generator Room by running the native projection
functions. It recorded both room depth endpoints beyond the clip plane.
Evidence is under `build/sp-room-visibility-1.71.0/native-sp-rend2-probe/`.

This package includes the single-player Classic fix from 1.71.0 and the
multiplayer placement and renderer updates from 1.70.0. Confirmation on
screen in the installed game remains open.

## 1.71.0 Single-player Classic room visibility (2026-11-11)

Single-player Classic now includes visible portable brush models when
computing its far plane, using the same bounds calculation and distance cap
as multiplayer Classic. The native renderer reproduced a fully clipped
Generator Room on `mp/ffa3`. The host view ended at 2,048 units, while the
room began 3,240 units ahead. This release includes the multiplayer room
placement and renderer changes from 1.70.0.

An exact 1.70.0 multiplayer CGame and Classic renderer probe registered and
submitted the stock Generator Room for a listen host, a connected remote
client and a late joiner. It used the captured Flatgrass server snapshot and
recorded indexed draw calls without an OpenGL context. Evidence is under
`build/mp-room-visibility-1.70.0/native-cgame-probe/`. On-screen confirmation
for the reported maps and launchers remains open.


## 1.70.0 Multiplayer room placement and rendering (2026-11-11)

Q-menu rooms now center their rotated bounds on the aimed surface, as in
single-player. A room's internal BSP origin can sit thousands of units from
its walls. The new placement keeps the full hull clear of the clicked
surface and still rejects collisions with existing objects or players.
Previously placed rooms retain the positions recorded in the world journal.

Classic now includes visible room brush models when setting the view's far
plane, within the map's existing distance limit. This lets a room extend
beyond the host map's visible leaves. Rend2 submits each copy of a brush
model with its own transform and lighting. A culled copy cannot suppress
another copy's surfaces.

This release addresses the misplaced-room and renderer cases above.
Graphical confirmation on the reported regular multiplayer launcher remains
open. The multiplayer feature number is 77. The catalog has 1,100 choices,
with protocol 1051 and the existing journal formats.

## 1.69.0 Single-player room visibility and multiplayer entities (2026-11-11)

Portable rooms in single-player now send the shell and its visible brush
children in live snapshots. The previous map-start baseline flag omitted
rooms spawned during play, even while their collision and hazards ran.
Restoring an older Sandbox save updates the same flags after room ownership
has been rebuilt. Named room cards match their native BSP resource, and
older saves can recover the original room ownership ID from the validated
assembly. Authored map baselines keep their native rules.

Multiplayer adds **Crystal crate** under **Q / Entities**. The retail
crystal follows the crate's full rotation. Hits spark and pulse damage,
while destruction or Activate fires Primary before the terminal blast.
Use **C** to edit health, hit and blast damage and radii, Enabled, and
solid or shot-only collision. Shot-only permits walking through the crate
and retains weapon hits. Duplicate and the JRW18/JRW19 world journal
preserve the crate's health, settings and links and rebuild its crystal.

The original **misc_teleporter** card now uses the same native multiplayer
teleport volume and destination controller as **trigger_teleport**.
Feature 76 brings the MP catalog to 1,100 choices. Protocol 1051 and the
journal layouts are unchanged.

## 1.68.0 Multiplayer welder (2026-11-11)

Spawn **Welder** from **Q / Entities**. The housing, sparks and electrical
loop use stock retail assets. The pulse follows the welder's final pose and
clips against scenery. Damage uses native falloff and line of sight and honors
server ownership rules and player tool consent. A value of zero keeps the
sparks and sound.

Use **C / Welder** to edit reach, damage radius, pulse interval and damage.
Activate or press USE on an editable welder to toggle its output. Enabled
controls whether the welder can run or be activated. Both states retain the
current pulse deadline. Reach and radius accept 1 to 256 units, interval
accepts 0.1 to 10 seconds and damage accepts whole values from 0 to 1,000.

Duplicate keeps the settings and starts a fresh cycle. The JRW18/JRW19
journal restores the remaining time, including a pending pulse from before
an interval edit. The multiplayer feature number is 75 and the catalog has
1,098 choices. Protocol 1051 and journal layouts are unchanged.

## 1.67.0 Multiplayer Jabba door camera (2026-11-11)

Q > Entities includes Jabba door camera (`misc_model_jabba_cam`, ID
`86873624`). The stock Kejim mount, telescoping shaft and camera head
form one selectable assembly. The shaft's stock local axis scales to the
current extension. The camera starts with a 20-unit closed length,
48 units of travel and a speed of 120 units per second.

The C menu exposes fractional travel and speed, Enable and Extend/Retract.
Native USE works on the parts. The head sweeps against scenery and actors,
stops and mutes the stock servo when blocked, then resumes when clear.
The assembly lets players and shots pass through it. The Physgun and tools
can select an editable part up to the first solid obstruction. The
assembly has a fixed mass of 20 and uses the native movement path.

World saves and copies preserve the setting and current extension. The
shaft and head are recreated from the root and follow the full root pose.
Removal and Undo clean up the root and its two parts. Multiplayer feature
74 adds one entry for 1,097 choices. Protocol 1051 and JRW18/JRW19 layouts
are unchanged. Graphical and audio playback, remote players and server
load remain open.

## 1.66.0 Multiplayer ammo rack (2026-11-11)

Q > Entities includes Ammo rack (`misc_model_ammo_rack`, ID `98056028`).
The stock Kejim rack carries 25 blaster rounds, 100 metallic bolts, 100
power cells, an E-11 with 15 rounds and a 20-health medpack. The five pickups
use the stock SP artwork and shelf poses. The medpack keeps its original
model when pulled off the rack. Positions use the centers of the stock
placement ranges so copies and reloads preserve the same layout.

The body clips players, NPCs, bots and normal weapon traces, and native USE
traces pass through it. The client uses the same clip policy for prediction.
Physgun, freeze and attachments use the existing prop body. Context lists
the five shelf statuses. Force Pull, pickup, Undo and cleanup use the
shared rack lifecycle. A saved or copied rack restores only the remaining
slots. Required stock and capacity are checked before publishing items.

Multiplayer feature 73 adds one entry for 1,096 choices. Protocol 1051 and
JRW18/JRW19 layouts are unchanged. The SP catalog and packaged stock assets
are reused. Graphical gameplay, remote players and server load remain open.

## 1.65.0 Multiplayer gun rack (2026-11-11)

Q > Entities includes Gun rack (`misc_model_gun_rack`, ID `96929416`).
It uses the stock Kejim rack and the stock E-11, repeater and rocket
launcher models. The three native one-shot pickups grant 15, 100 and 4
rounds respectively. Each weapon follows the rack's full transform at its
stock shelf position. Context shows which slots are stocked or collected.
Ordinary Physgun, freeze and attachment actions remain available.

Collected slots stay empty in copies and world saves. Undo and cleanup
remove the rack with its mounted stock. Missing stock assets, server-disabled
weapons or insufficient entity space reject placement as one operation.
Force Pull can dislodge stock weapons as temporary pickups. A dislodged slot
stays empty and its temporary item expires through the native drop lifetime.

Multiplayer feature 72 adds one entry for 1,095 choices. Protocol 1051 and
JRW18/JRW19 layouts are unchanged. The SP catalog and stock assets are reused.
The separate ammo rack and rendered gameplay, remote players and server
load remain open.

## 1.64.0 Multiplayer small cargo crate (2026-11-11)

Q > Entities includes Small cargo crate (`misc_model_cargo_small`, ID
`87961440`). It uses the stock Kejim MD3 and stock menu art. The crate starts
with 40 health, 1 splash damage and a 96-unit blast radius, matching the SP
Sandbox card. Direct heavy fire from turrets, emplaced guns and walker weapons
can break it. Its C panel has health, blast damage, radius, Enabled and the
Primary destruction output. There is no manual activation action.

Destruction removes the crate, invokes its optional Primary output once and
drops one native instant medpack. It uses the stock SP model, bounds and
20-health award, with the native 30-second timeout. It belongs to the crate's spawn group for Undo and
cleanup. World saves record intact or pending crate state and omit temporary
drops. The MP client loads stock pickup art on first visible use, including
items first registered after a client joins.

Multiplayer feature 71 adds one entry for 1,094 choices. Protocol 1051 and
JRW18/JRW19 layouts are unchanged. Existing SP cards and stock assets are
reused. Graphical and audio playback, remote players and server load remain
open.

## 1.63.0 Multiplayer planted bomb and deployable beacon (2026-11-11)

Open Q > Entities and search for `misc_model_bomb_planted` or
`misc_model_beacon`. The bomb starts armed and humming. Disarm it with
USE, the C menu or Toolgun Activate. It plays the stock disarm sound and
frames 0–8 once, freezes its shader animation, and triggers an optional
Primary link. The disarmed prop stays in the world.

The beacon runs its deploy animation through frames 0–30, then loops its stock
signal animation and sound. Activate to hide or deploy it. A hidden
beacon is silent and has no collider. The C menu can reselect it at its
location, and deployment checks the full hull for obstruction. The C
Enabled control pauses animation and audio on both devices.

World saves and duplicates preserve phase, frame, remaining step and
included links. Restore resumes playback without replaying one-shot
sounds or links. Hidden beacon attachments stay dormant until deployment.
Both devices use stock MD3s, sounds and collision bounds that cover all
frames. Multiplayer feature 70 adds two entries for 1,093 choices and
retains protocol 1051 and JRW18/JRW19.

## 1.62.0 Multiplayer projectile shooters (2026-11-11)

Open Q > Entities and search for `shooter_plasma`, `shooter_grenade`, or
`shooter_rocket`. Each starts as a single-shot primary launcher facing
the player’s placement yaw. Activate it with the C menu or Toolgun.
Rotate it to change its direction, or Link Primary to an editable entity
or marker for live aim. The grenade calculates a lob for a linked target.

The C menu sets Primary or Alternate, Repeat, interval and added random
delay. Repeat starts with an immediate shot and defaults to 500 ms
between shots. Activate again to stop. Disabling the shooter stops repeat.
Duplicates and world saves retain the configuration, included aim link,
and remaining repeat delay. Launched projectiles are transient.

Plasma, thermal detonators, and rockets use stock projectile types, muzzle
and impact effects, and sounds. The rocket uses normal SP difficulty for
its direct-hit damage. Direct and splash damage respect current server
ownership and player consent. A projectile loses its firing authority if
the shooter is removed or disabled. Feature 69 adds three entries for a
total of 1,091 MP choices and retains protocol 1051 and JRW18/JRW19.

## 1.61.0 Curated prop and actor cards (2026-11-11)

Multiplayer Q now has all 20 curated SP prop cards under Props and all 12
curated NPCs under NPCs. Swoop and Tauntaun appear under Vehicles. The
cards retain their SP IDs, names and stock icons. Existing map-spawner
aliases retain their Entities tab, IDs and native spawn routes. Props
use the normal MP MD3 collision path and actors use the native NPC and
vehicle loaders. The shared MP vehicle definition uses the stock default
skin for base Swoop so server and client resolve one valid skin path.
SP selects from the retail palette per spawn, and MP offers seven named
color cards. Multiplayer now has 1,088 spawn choices.

Kejim crate (ID 1013) now spawns in both modes. Jedi Academy's Imperial
`crate_01.md3` and Jedi Outcast's Kejim crate differ only in two internal
path strings. Geometry, frame bounds and the resolved texture match.
New spawns use the installed Jedi Academy model and texture. SP ownership
validation still recognizes the old model path in saves made with that
asset installed. No Jedi Outcast assets are required. Multiplayer feature
68 retains protocol 1051 and the existing world journal formats.

## 1.60.0 Multiplayer exploding crate and gas tank (2026-11-11)

Open Q > Entities and search for `misc_exploding_crate` or `misc_gas_tank`.
The Nar Shaddaa crate starts with 40 health and a 50-damage blast over
128 units. The Imperial Mine gas tank starts with 20 health and a
32-damage blast over 48 units. The tank releases occasional gas jets and
fires one damaging flame pulse on the first hit it survives. Jets follow
the tank's orientation. Both objects use stock models, debris and sounds.

Use the C menu to change maximum health, blast damage, radius and Enabled,
or detonate an intact object. Link Primary to run a controller when the
object is destroyed, before the blast. Splash and flame damage respect
server ownership and player consent. The blast removes the object.
Duplicates and world saves keep current health, the pending explosion
or the remaining idle-jet delay, and whether the flame pulse was spent.
There are 1,054 MP choices. Feature 67 retains protocol 1051 and the
existing JRW18 and JRW19 record layouts.

## 1.59.0 Multiplayer persistent E-Web (2026-11-11)

Search for `emplaced_eweb` under Q > Entities. Freeze the gun before use,
stand behind it and press USE to mount. Either attack button fires the same
stock E-Web shot. Release and press USE again to dismount. The gun stays in
the world and the operator regains their previous weapon.

The stock Hoth E-Web uses the SP Sandbox defaults of 250 health, 999
rounds and full traverse. The C controller panel exposes health, yaw
limit, ammunition, Enabled, Mount and Repair. Duplication and world saves
preserve ammunition, health and wreck state. Manual save and removal
require a vacant gun. Autosave records a parked gun without its operator.
There are 1,052 MP choices. Feature 66 uses protocol 1051 and the existing
JRW18 and JRW19 record layouts.

## 1.58.0 Multiplayer emplaced gun (2026-11-11)

Search for `emplaced_gun` under Q > Entities. The gun starts frozen and
faces the player’s spawn aim. Stand behind it, face forward and press USE
to mount. Fire with the regular attack control. Release and press USE again
to dismount. The card uses the stock MP chair and its native aim, muzzle
and operator behavior. The MP model uses Bone02 and has no SP seat tag.

When parked, the gun supports the Physgun, rotation, attachments and
placement. The C controller panel exposes maximum health, yaw limit,
Enabled, Mount and Repair. Occupied guns reject movement, duplication,
manual world save and removal. An autosave records a parked gun without
the operator. The JRW18 and JRW19 layouts retain health, yaw, and pending
destruction or wreck state. A gun can be mounted again after restoration.
There are 1,051 MP choices. Multiplayer feature 65 uses protocol 1051.

## 1.57.0 Multiplayer explosive cargo barrel (2026-11-11)

Multiplayer Sandbox now has the stock explosive Cargo Barrel
(`object_cargo_barrel1`). Search for `cargo_barrel` under Q > Entities.
It uses the single-player card's
Imperial model, 40 health, 100 splash damage and 200-unit blast radius.
Lethal damage starts a random 100–500 ms fuse. Use the Toolgun's Activate
mode or C > Controller > Detonate for an immediate explosion. The blast
uses the stock thermal effect, fires the barrel's Primary output and
removes the barrel. Splash damage uses the server's current damage
permissions. Damage-triggered chain reactions use separate fuse timers.

The C menu exposes health, splash damage, radius and Enabled. Duplicate and
world journals preserve an armed or pending barrel and the remaining fuse.
There are now 1,050 MP choices. Multiplayer feature 64 uses protocol 1051
and the existing JRW18 and JRW19 record layouts.

Vehicle teleport prediction now defers for the rest of a command replay
if the rider reaches a trigger before the hull.

## 1.56.0 Multiplayer vehicle teleporter prediction (2026-11-11)

A multiplayer client can predict a lone pilot and settled native vehicle
entering a frozen Sandbox Teleporter linked to a frozen destination. The
server publishes a short-lived grant with the exact native arrival positions
and collision bounds for both the hull and pilot. The client checks the
current seat, trigger contact, visibility and clearance before moving the
pair in the current prediction replay. The server commits the movement and
emits the effects.

The server handles rider-first trigger contacts, competing volumes, extra
occupants, droid seats, attachments, dynamic endpoints and authored routes.
The grant uses multiplayer feature 63. Protocol 1051, module APIs and the
JRW18 and JRW19 save layouts are unchanged.

## 1.55.0: Multiplayer Abandoned City (2026-11-11)

Spawn Abandoned City in multiplayer Sandbox from Q > Entities. It uses the
stock `mp/duel8` BSP with 13 local entries, 21 markers, an intermission point,
two wind loops and 69 independent stone brushes. Select the room in C and
choose Controller > Enter section, or use the Toolgun's Activate mode.
Aiming a Sandbox tool at a stone brush selects the room.

Standard weapon damage breaks each stone brush according to its stock
health. A break removes that brush's collision and model, with local stone
debris. It leaves host-map visibility portals alone. The room's live brushes
follow the room when moved. Movement is refused while an entity stands on
the room or one of its brushes. Duplicate creates a new intact city. A
server world restore preserves missing brushes and partial health.

The MP catalog now has 1,049 choices and all seven sections available in
SP. The 70 City inline models share one resource slot. City worlds use
JRW19 for a per-room brush-damage record. Worlds without City still use
JRW18. Game API 7 and protocol 1051 are retained. The server fixture covers
breaks, moved geometry, pristine copies, validation and reconstruction.
Pure and non-pure dedicated-server checks exercise City alongside Vjun
through late join and independent Undo.

Rendered and audible playback, remote players and server load remain open.

## Latest installed release: 1.54.0 (2026-11-11)

Build `20261111-100327-b0d9ce9d` installed at `2026-11-11T10:10:37.0022389-05:00` through `scripts/build-jar.ps1`.
All 151 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2104 archived source files. ZIP SHA-256: `70600DACF5094A2F0940C79E4B5229E63D7A6BBE77E6C152AC7FADE7C916B3F1`.
Receipt: `build/deploy/20261111-100956-b694503e/receipt.json`. Qualification: `build/mp-bespin-1.54.0/qualification.json`.

Both headless clients received the native cloud car, the original MD3 and its
three-second trajectory on pure and non-pure servers. Undo removed the
courtyard and its car while retaining the resource slots. The native
mover fixture verified rider motion, protected rollback and stock crush.
Evidence: `build/mp-smoke-20261111-100812/network-client-result.json`,
`build/mp-smoke-20261111-100658/network-client-result.json` and
`build/mp-bespin-1.54.0/mover.log`. A headless run using the exact
packaged game DLL verified that a grounded rider followed the platform's
sine motion. Evidence: `build/bespin-native-rider/run-20261111-100412/network-client-result.json`.
Rendered, audible, remote-player, load and in-game restore acceptance
remain open. These receipt notes were added after packaging.

## 1.54.0: Multiplayer Bespin Courtyard (2026-11-11)

Spawn Bespin Courtyard in multiplayer Sandbox from Q > Entities. It uses
the stock `mp/duel1` courtyard. Select the room in C and choose Controller >
Enter section, or use the Toolgun's Activate mode. Aiming at the cloud car
with the Physgun, Toolgun or context menu selects its parent room.

The cloud car uses the original MD3 and native player/NPC clip brush. Its
three-second cycle moves 20 units in either direction along the room's
local vertical axis. Native mover collision carries riders. A blockage
that would crush a protected target backs out the attempted push. Moving
the room is refused while an entity stands on the courtyard or the car.
A successful move keeps the car's phase, and copies and JRW18 world
restores reconstruct that phase. Undo removes the room and the car.

The MP catalog now has 1,048 choices. The courtyard root and cloud car
use consecutive inline model indices, and the visible car model is
registered separately. Game API 7, protocol 1051 and JRW18 are retained.
The lifecycle and native mover tests cover rotation, blocked movement,
riders, rollback, phase persistence and cleanup. The pure and non-pure
dedicated-server tests check the car's brush and model indices, trajectory,
late join and independent Undo alongside the existing sections.

Abandoned City remains to be ported from SP. Graphical appearance, audio,
in-game world restore, remote play and server load remain open.

## Latest installed release: 1.53.0 (2026-11-11)

Build `20261111-094742-06b31770` installed at `2026-11-11T09:54:39.0478811-05:00` through `scripts/build-jar.ps1`.
All 150 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2101 archived source files. ZIP SHA-256: `43CEE12F85DA8CC2F4B28EEC2C2A2B6DE141ECCD400A62C6D51817A27877CDB6`.
Receipt: `build/deploy/20261111-095357-0b92bce7/receipt.json`. Qualification: `build/mp-hazards-1.53.0/qualification.json`.

The pure and non-pure dedicated servers each exercised the two-model hazard
sections, late join, retention of all resource slots and independent Undo.
The native fixture also checked rotated contact, consent, safe arrival and
JRW18 cooldown reconstruction.
Evidence: `build/mp-smoke-20261111-095213/network-client-result.json` and
`build/mp-smoke-20261111-095102/network-client-result.json`.
A separate headless test used the exact packaged game DLL. Native movement
triggered Generator's falling state and Taspir's 50 combined health and armor
loss, opt-out and cooldown. Evidence: `build/hazard-native-contact/run-20261111-095354/network-client-result.json`
and `build/hazard-native-contact/run-20261111-095313/network-client-result.json`.
Rendered, audible, remote-player, load and in-game restore acceptance remain
open. These receipt notes were added after packaging.

## 1.53.0: Multiplayer Generator Room and Taspir Landing (2026-11-11)

Spawn Generator Room or Taspir Landing in multiplayer Sandbox from Q >
Entities. Each uses its original retail BSP. To enter, select the room in C
and choose Controller > Enter section, or use the Toolgun's Activate mode.
The server checks the arrival hull against the active hazard.

Generator Room retains its native fatal-fall volume and positional machinery
loop. Taspir Landing retains its 50-damage volume with a one-second cooldown.
The hazard moves and rotates with each room, and the server tests against its
rotated shape. Damage follows existing player-tool settings, player opt-out
and Sandbox edit permissions. Deactivate links suspend the hazard. Duplicate
and world restore reconstruct the volume and preserve Taspir's remaining
cooldown. Undo removes the room and its local entities.

The MP catalog now has 1,047 choices. Each new room uses two consecutive
inline model indices under one BSP resource slot. The game API remains 7,
wire protocol 1051 and world format JRW18. The lifecycle test covers native
fall and damage dispatch, cooldown, rotated contact, blocked entry,
permissions, mixed model counts and restore. Dedicated-server release
checks exercise separate mixed-room and hazard-room scenarios with two
headless clients on pure and non-pure servers.

Bespin Courtyard and Abandoned City remain to be ported from SP. Graphical
and audio playback, in-game world restore, remote players and server load
remain open.

## Latest installed release: 1.52.0 (2026-11-11)

Build `20261111-093212-32ffc39c` installed at `2026-11-11T09:38:43.2632949-05:00` through `scripts/build-jar.ps1`.
All 150 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2100 archived source files. ZIP SHA-256: `C34DD3764D2FC4C66A7DEDC5AEFE16929497FC07315FD190B7C42D381C74420F`.
Receipt: `build/deploy/20261111-093802-7fac7f92/receipt.json`. Qualification: `build/mp-rancor-1.52.0/qualification.json`.

The pure and non-pure dedicated servers each published three stock BSP resources,
four room roots, twelve speakers and six native Rancor Pit torches. Both
clients saw each Undo. The native lifecycle fixture verified the damage
dispatch, consent policy, transformed torch direction and disabled restore.
Evidence: `build/mp-smoke-20261111-093617/network-client-result.json` and
`build/mp-smoke-20261111-093518/network-client-result.json`.
Rendered, audible, remote-player, load and in-game restore acceptance remain
open. These receipt notes were added after packaging.

## 1.52.0: Multiplayer Rancor Pit (2026-11-11)

Spawn Rancor Pit in multiplayer Sandbox from Q > Entities. It uses the stock
`mp/duel7` BSP and retains 15 local entry points, the intermission point,
18 reference markers and six native torches. Select the room in C and choose
Controller > Enter section, or use the Toolgun's Activate mode.

The torches use the original flame effect and burning loop. They follow the
section's pose and dispatch the native 5-point splash damage with 16-unit
falloff every 200 ms. Damage follows the multiplayer server's player-tool
setting, per-player opt-out and applicable Sandbox edit permissions.
Deactivate links suspend the flame, loop and damage, and the disabled state
survives duplication and JRW18 world restore. Undo removes the room and all
six torches.

The MP catalog now has 1,045 choices. Imperial Control Room, Vjun Fuel
Processing and Rancor Pit use separate BSP registrations. Game API 7, wire
protocol 1051 and JRW18 are retained. The native lifecycle test covers exact
effect-table capacity, rotated torch position and direction, damage dispatch,
consent, disabled state reconstruction and cleanup. The dedicated-server
release checks cover all three BSP resources, four room roots, twelve
speakers, six torch snapshots and four independent Undo events on pure and
non-pure servers.

Four SP sections still need MP implementations. Graphical flame rendering,
audible playback, in-game world restore, remote players and server load
remain open.

## Latest installed release: 1.51.0 (2026-11-11)

Build `20261111-091953-68d2f130` installed at `2026-11-11T09:26:15.5635137-05:00` through `scripts/build-jar.ps1`.
All 150 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2099 archived source files. ZIP SHA-256: `CFC69E48E80C75B4DC30DDBE6BB220EB45980406F3CE8EEF8B92EDB0CB32DDC9`.
Receipt: `build/deploy/20261111-092533-46edb8b1/receipt.json`. Qualification: `build/mp-vjun-1.51.0/qualification.json`.

The pure and non-pure dedicated servers each published two stock BSP resources,
three room roots and twelve speakers. Both peers saw each Undo, and the pure
late joiner's archive proof included the stock archive containing both rooms.
Evidence: `build/mp-smoke-20261111-092356/network-client-result.json` and
`build/mp-smoke-20261111-092259/network-client-result.json`.
Rendered, audible, remote-player, load and in-game restore acceptance remain
open. These receipt notes were added after packaging.

## 1.51.0: Multiplayer Vjun Fuel Processing (2026-11-11)

Spawn Vjun Fuel Processing in multiplayer Sandbox from Q > Entities. The room
uses the stock `mp/duel10` BSP and retains 17 local entry points, the
intermission point, 38 reference markers and four positional sound loops.
Select the room in C and choose Controller > Enter section, or use the
Toolgun's Activate mode. Allow clearance for its full hull.

Vjun and Imperial Control Room can coexist and each keeps its own BSP model
registration. Copies and server world restores reconstruct the correct section
and its children from the saved catalog ID and pose. Undo removes only the
selected spawn and its children. A Physgun grab on either room now uses the
point hit by the beam. Wheel distance moves that point and USE rotation pivots
around it. The room parks on release. Room movement still needs clearance and
is refused when an entity stands on it.

The MP catalog now has 1,044 choices. Vjun uses the SP catalog's original
`misc_bsp` ID with its retail display name. Game API 7, protocol 1051 and
JRW18 are retained. Mixed-room fixtures cover blocked entry, grab and
rotation, duplicate reconstruction, save reconstruction and cleanup. Native
loopback clients verify both BSP slots, three roots and twelve retail speaker
positions through late join and three independent Undo events. The release
runs that test on both pure and non-pure dedicated servers.

Five SP sections still need MP implementations. Graphical and audio playback,
native in-game world restore, remote players and server load remain open.

## Latest installed release: 1.50.0 (2026-11-11)

Build `20261111-090451-64c3ee0c` installed at `2026-11-11T09:11:23.0112252-05:00` through `scripts/build-jar.ps1`.
All 150 automated suites passed. Independent readback verified
72 installed files, 18 replacement backups and
2099 archived source files. ZIP SHA-256: `F0A001D29E719751223F4E2E87C589296FECAB438EDD93117D9FBB52638DAE59`.
Receipt: `build/deploy/20261111-091041-0c5c9898/receipt.json`. Qualification: `build/mp-section-1.50.0/qualification.json`.

The dedicated-server section probe used the exact retail Imperial Control Room
and two independent loopback clients. It verified the reliable BSP resource
before the first root snapshot, the late-join gamestate, eight positioned
native speakers and separate Undo events for the two copies.
Evidence: `build/mp-smoke-20261111-090814/network-client-result.json`.
The pure and non-pure archive/light/music checks also passed.
Rendered, audible, remote-player, load and in-game restore acceptance remain
open. These receipt notes were added after packaging.

## 1.50.0: Multiplayer Imperial Control Room (2026-11-11)

On a multiplayer Sandbox server, open Q > Entities and choose Imperial Control
Room. Aim at an open surface within 4,096 units. The full rotated room needs
clearance. Select the room in C and use Controller > Enter section, or use the
Toolgun's Activate mode. Entry checks the player's destination. The section
contains 13 local entry points, an intermission point, nine reference markers
and four stock positional sound loops.

The room uses native collision from the stock `mp/duel4` BSP. The Physgun,
Rotate tool and C transform controls can reposition it within collision bounds.
Movement is refused while an entity is standing on it. Releasing the Physgun
parks the room, and the room itself has no rope, weld, hinge or thruster
attachment support. Duplicate and server world saves reconstruct the local
children at the stored pose. Undo removes the room and its children. The
server retains the BSP registration for reuse during the current map.

This adds one card for a total of 1,043 MP choices. The server verifies the
retail BSP checksum, available inline models and BSP configstring capacity
before exposing the brush. Native game API 7 requires the matching engine
and game module. Wire protocol 1051 and world format JRW18 are retained.

The lifecycle fixture covers blocked poses and entry, independent duplication,
world reconstruction and cleanup. The loopback dedicated-server test checks
active and late-join resource registration, two distinct roots, eight stock
speaker placements and both Undo events. Rendered and audible play, remote
players, load and in-game world restore remain open. Six other sections
available in SP still need MP implementations.

## Latest installed release: 1.49.0 (2026-11-11)

Build `20261111-084154-4b7eb34e` installed at `2026-11-11T08:48:10.5029424-05:00` through `scripts/build-jar.ps1`.
All 150 automated suites passed. Independent readback verified
72 installed files, 17 replacement backups and
2093 archived source files. ZIP SHA-256: `DC01AD33E794A5CDA7EBB8276E82206D4F79E30F7A6EF501DF2518BE1BE8C82F`.
Receipt: `build/deploy/20261111-084728-f8bf1b50/receipt.json`. Qualification: `build/sp-city-1.49.0/qualification.json`.

The exact retail section test retained 106 children, 13 entry points, two wind
speakers and 69 native breakable brushes. The native callback fixture checked
rotated effects, collision removal and delayed free. The retail audit admitted
seven of 57 stock sections and rejected six modified City files and four
modified hazards. Evidence: `build/jar/Release/retail-bsp-admission-20261111-084303-195203/result.json` and
`build/jar/Release/retail-bsp-admission-20261111-084303-195203/duel8-section.log`. Rendered behavior, audible playback,
full in-game save/load and carry performance remain open. These receipt notes
were added after packaging.

## 1.49.0: Single-player Abandoned City portable section (2026-11-11)

Abandoned City (`mp/duel8`) is directly spawnable from the single-player
Entities tab. It brings the SP catalog to 1,180 cards and the portable retail
sections to seven. The native BSP keeps 69 individually breakable stone
brushes, 13 local entry points, the intermission point, 21 markers and two
positional wind loops.

The section uses the original stone health, material, collision and damage
callbacks. Debris, sound and AI alerts follow the rotated block. Breaking a
block clears its collision and rendering while its native cleanup timer
runs. Section-owned blocks do not open area portals in the host map.
Selecting a block with a Sandbox tool selects the section. Ordinary attacks
still break individual blocks. Physgun movement uses the section's enclosing
hull and refuses movement with riders on the attached blocks.

Undo and cleanup remove the section's surviving children. Duplicating a
section reconstructs its original intact layout. Native field and ownership
serialization retain the surviving brushes for full saves. Actual in-game
save/load, rendered and audible behavior, and carry performance still need
gameplay testing.

Admission validates the 70 inline models against the engine's reserved
collision handle. The retail audit checks the child brush and draw ranges,
planes and vertex bounds. Six altered City files exercise rejection of bad
inline references, flags, collision planes, surface ownership and draw
geometry. The existing retail profiles and hazard mutation checks still run.

## Latest installed release: 1.48.0 (2026-11-11)

Build `20261111-082510-b58c23d8` installed at `2026-11-11T08:31:23.9012568-05:00` through `scripts/build-jar.ps1`.
All 149 automated suites passed. Independent readback verified
72 installed files, 15 replacement backups and
2090 archived source files. ZIP SHA-256: `6F6FF34034953F8CDD3CDC8AB0D7B51EFB802FB959AA656F5CB8CD7B1A2C454E`.
Receipt: `build/deploy/20261111-083042-25f874b3/receipt.json`. Qualification: `build/mp-pure-1.48.0/qualification.json`.

The staged dedicated-server test rejected a bad proof and admitted both
independent clients. Their checksum proofs used all four retail archives and
the six staged JA:R MP archives. Both pure and non-pure loopback checks passed
the portable light and music late-join and Undo paths.
Evidence: `build/mp-smoke-20261111-082846/network-client-result.json` and
`build/mp-smoke-20261111-082756/network-client-result.json`. Graphical, remote-human and load
acceptance remain open. These receipt notes were added after packaging.

## 1.48.0: Pure dedicated-server archive compatibility (2026-11-11)

The 64-bit JA:R client now resolves the expected retail cgame and UI archive
checksums when it sends its pure-server proof. It uses the same mounted-archive
lookup as the server. Extra client archives excluded by the server's allowlist
stay out of the proof. The same builder runs on connect and after a video restart.

Set `sv_pure 1` before the `map` command in the dedicated server's own
`server.cfg` to enable archive matching. Use the same JA:R and retail PK3s
on both ends. Protocol 1051 and the existing archive proof format apply.

The release checks the client proof builder and connects independent, headless
UDP clients to a loopback pure dedicated server using copies of the retail and
staged JA:R archives. They compute seeded checksums from their own archive
directories. The server must reject a bad proof and accept two valid peers.
The peers then exercise late join and Undo for portable lights and music.
The non-pure wire checks still run. This verifies archive agreement. Graphical
client connection, real remote play and server load acceptance remain open.

## Latest installed release: 1.47.1 (2026-11-11)

Build `20261111-081229-555b3290` installed at `2026-11-11T08:18:04.3060077-05:00` through `scripts/build-jar.ps1`.
All 148 automated suites passed. Independent readback verified
72 installed files, 15 replacement backups and
2088 archived source files. ZIP SHA-256: `670D816AEF59A9C72847FD1A94659B6F74323FA086BBFD173BC46EF76DBDDEF6`.
Receipt: `build/deploy/20261111-081725-6b018ac5/receipt.json`. Qualification: `build/mp-music-1.47.1/qualification.json`.

The dedicated wire check used two non-pure loopback clients. It verified the
late-join light, initial music cue and Undo restoration on both clients.
Evidence: `build/mp-smoke-20261111-081517/network-client-result.json`. Audio, remote-human and server-load
acceptance remain open. These receipt notes were added after packaging.

## 1.47.1: Restore empty map music cues in multiplayer (2026-11-11)

Restoring an empty map cue now passes through the native sound startup path.
The sound engine records the empty cue for `snd_restart`, uses the map's
dynamic music when configured, and stops the previous track when no music
is configured. Tagged late-join cues still start through cgame. The server
fixture now covers undoing all playback on an empty baseline.

1.47.0 build `20261111-080540-b4383b4b` installed at `2026-11-11T08:11:23.9536416-05:00`. Independent
readback verified all 72 installed files, 15
replacement backups and 2088 archived sources.
Receipt: `build/deploy/20261111-081044-6382ae9e/receipt.json`. Qualification: `build/mp-music-1.47.0/qualification.json`.

## 1.47.0: Multiplayer music controllers and late-join checks (2026-11-11)

MP's Entities tab adds `target_play_music` with its SP card ID and stock crate
model, bringing MP to 1,042 choices. In C > Controller, select a stock track or
enter an intro and loop separated by a space. Play/Stop and linked activation
use the map's native background-music channel. A late join receives the active
cue at initialization. Tracks come from the same 133-path retail list as SP.

Stopping or removing a controller restores the previous controller's cue or the
map's original cue. Editing the active track is reversible with property Undo.
Duplicating a controller copies its cue and Enabled state with playback off.
JRW18 preserves cue settings and the activation order in world saves. Playback
restores against the loaded map's cue. Native map music can take control again.

Feature version is 56. MP protocol 1051, game API 6, cgame API 8, renderer API 12
and the JRW18 record layout are unchanged. Both endpoints should use 1.47.0 for
the controller and late-join playback. A previous build cannot load a world
revision containing the new card.

The normal build exercises two native-codec UDP clients. The first spawns the
light and music controller before the second joins. The second receives the
light state and the active music configstring. Undo restores the map cue and
removes the light for both clients. The tests use loopback and a non-pure
dedicated server. Actual audio, graphical UI, remote peers and server load
acceptance remain open.

MP portable BSP sections remain open. The stock MP loader marks its BSP entity
permanent, which excludes it from snapshots, and runs host-map setup. A portable
path needs engine-side BSP admission, a replicated root, scoped children and
native brush movement. Imperial Control Room is a candidate with one inline
model and no moving or damaging children. Its local speakers and entry markers
need to remain separate from the host map's spawn list.

## Latest installed release: 1.46.0 (2026-11-11)

Build `20261111-074412-db2fc161` installed at `2026-11-11T07:50:00.8750209-05:00` through `scripts/build-jar.ps1`.
All 148 automated suites passed. Independent readback verified
72 installed files, 15 replacement backups and
2085 archived source files. ZIP SHA-256: `6B3F135A1D5E6D88BDFA8727A022610A4DF77C89E73EE5C9A39D0F5C839341E8`.
Receipt: `build/deploy/20261111-074922-d4548c26/receipt.json`. Qualification: `build/sp-hazards-1.46.0/qualification.json`.

The retail audit checked 57 stock BSPs and six portable profiles, including the
new Generator Room and Taspir Landing sections. Both preserve native hazard
behavior with exact sampled contact after rotation. The audit refused four altered
hazards with unchanged model bounds. Evidence: `build/jar/Release/retail-bsp-admission-20261111-074512-169442`.
Rendered acceptance and full engine save relocation remain open. The project
checklist remains active. These receipt notes were added after packaging.

## 1.46.0: Generator Room and Taspir Landing portable sections (2026-11-11)

SP's Entities tab adds **Generator Room** (`mp/duel2`) and **Taspir Landing**
(`mp/duel5`), bringing the stock section choices to six and the full SP catalog
to 1,179 cards. The Generator Room uses the native SP falling-death path for its
stock fatal pit. Taspir Landing retains its 50-point hazard with a one-second
cooldown. The former retains its local ambient speaker. All local references
and entry points remain isolated from the host map and other copies.

Admission checks each stock hazard's actual six planes, material and inline
model bounds before using its exact shape as an oriented trigger. After a
section moves or rotates, its trigger bounds follow. Contact uses the actor
hull sampled by the server, including during movement, and rejects the empty
corners of the enclosing bounds. Native damage, cooldown and fall callbacks
remain authoritative. The arrival check skips entries that would overlap an
active local hazard. The original four section cards and their stable IDs
remain available. Other stock maps still require specific adapters.

The retail audit exercises all six stock sections and rejects four altered
hazard brushes that retain their original enclosing bounds. Native server
contact, hurt spawn/touch, movement, cleanup and saved fields have fixture
coverage. Live graphics, complete engine save/load and gameplay acceptance
remain open. No MP gameplay or save/protocol layout changes are needed.

## Latest installed release: 1.45.0 (2026-11-11)

Build `20261111-072944-b13a3257` installed at `2026-11-11T07:35:37.2849030-05:00` through `scripts/build-jar.ps1`.
All 147 automated suites passed. Independent readback verified
72 installed files, 16 replacement backups and
2080 archived source files. ZIP SHA-256: `A98196C7A271248E4AB66547751236F63EC924EE88532E29F4E787EC278A5F64`.
Receipt: `build/deploy/20261111-073457-5d0007be/receipt.json`. Qualification: `build/sp-sections-1.45.0/qualification.json`.

The retail audit checked all 57 stock BSPs and qualified four section profiles:
Bespin Courtyard, Imperial Control Room, Rancor Pit and Vjun Fuel Processing.
The source and native fixtures verify local entry points, child transforms,
resource preflight, copy reconstruction and cleanup. Evidence: `build/jar/Release/retail-bsp-admission-20261111-073043-814371`.
Rendered acceptance and full engine save relocation remain open. The project
checklist remains active. These receipt notes were added after packaging.

## 1.45.0: Three additional portable SP map sections (2026-11-11)

The SP Entities tab adds **Bespin Courtyard** (`mp/duel1`), **Imperial Control
Room** (`mp/duel4`) and **Rancor Pit** (`mp/duel7`). The existing `misc_bsp`
card keeps its stable ID and still spawns **Vjun Fuel Processing**. SP now has
1,177 cards, including the three new variants, with no listed unavailable cards
and the same 29 approved exclusions. Use a large clear area, then Activate the
section to enter a clear local spawn point. Entry checks the actual arrival hull
and refuses seated players; the section's local starts do not change host spawns.

Each profile qualifies the original BSP, native child classes and required
resources before placement. It checks registration capacity and rolls back
incomplete sections. Stock local reference points remain available instead of
being discarded by their ordinary map-load callback. Music and environment from
a section do not replace the host map settings. Copies reconstruct the selected
section with isolated children; cleanup removes the entire section.

Bespin retains its native bobbing cloud car, including phase, pause state and
brush/visible model alignment as the section moves. Root sweeps exclude their
own supported brush children for the duration of the collision check, restore
links afterwards and refuse moves when the brush has riders or unsafe motion.
The Imperial Control Room retains four local speakers. The Rancor Pit retains
six oriented native torch emitters with the retail flame, loop sound and damage.

Production admission and section controller fixtures run against all four retail
BSPs. Separate native tests check the moving brush pusher and torch callbacks,
including save fields and cleanup. These checks do not prove graphical lighting,
complete save/load relocation or how the sections feel in play. Broader section
profiles remain open. Save layouts, MP catalog/protocol and runtime APIs are
unchanged; no graphical game is launched during the build.

## Latest installed release: 1.44.0 (2026-11-11)

Build `20261111-071047-bbf5bf95` installed at `2026-11-11T07:16:18.3601010-05:00` through `scripts/build-jar.ps1`.
All 146 automated suites passed. Independent readback verified
72 installed ZIP files, 19 replacement backups,
and 2072 archived source files.
ZIP SHA-256: `A423A514034102870BCB0F9087575B85CFE5F8AC49E946C9CAC7B3A6E7CFD73C`.
Receipt: `build/deploy/20261111-071537-319b44bf/receipt.json`. Evidence: `build/mp-1.44.0/qualification.json`.

The installed dedicated server passed two concurrent headless client connections
using its normal install paths. Client commands spawned both portable light cards;
native snapshots carried the expected light values and removed them after Undo.
This does not establish GPU behavior, remote-human play or server load acceptance.
MP scope stays open for further portable controllers/map sections, full Rend2
parity and rendered/remote acceptance. No graphical game was launched. These
receipt notes were added to the workspace after packaging.

## 1.44.0: Multiplayer lights, Rend2 option and local wire testing (2026-11-11)

MP's Entities menu adds `light` and `misc_dlight` with their SP stable IDs and
stock art. They spawn a movable blue light, controlled through Activate or C >
Controller. The radius is 4–1020 units in steps of four. Linked Activate and
Deactivate preserve the selected light state; radius and on/off state survive
Undo, duplication and JRW18 world saves. The two entries bring MP to 1,041
choices. They use the existing native light snapshot field and renderer path.

`Play-JA-R-Multiplayer-Rend2.cmd` tries the separately packaged MP Rend2 DLL for
that launch. The regular MP launcher still uses Classic by default. Missing or
ABI-incompatible Rend2 falls back to Classic without changing the saved renderer
preference. Graphics initialization, actual rendered maps and SP's extended
Rend2 features require separate verification; MP automap is unavailable in Rend2.

Clients predict the touch of a stationary, permitted portable teleport for
ordinary on-foot players when both ends and the exit hull can be checked.
Short-lived grants, local collision/PVS checks and replay guards limit the
prediction. The server still resolves arrival, events and all permission checks.
Vehicles, spectators, moving portals, overlaps and explicit activation continue
through the authoritative route.

The build also tests two concurrent headless loopback UDP clients through the
native challenge, compressed connect, game state, input, snapshots and reliable
disconnect path. It tests bad challenge/old protocol rejection. This is local
wire coverage, separate from a graphical client, a real LAN peer or internet play.
Feature version is 55; protocol 1051, game API 6, cgame API 8, renderer API 12 and
JRW18 are unchanged. The current build is recommended on both ends for new controls.

Three legacy neutral-flag/Harvester pickups remain open: retail resources and
native One Flag/Harvester mode rules are both absent. Full MP parity still needs
remaining portable controllers and map sections, rendered acceptance, remote
clients and load tests. Existing SP gameplay is outside this release's scope.

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

## Latest installed release: 1.41.0 (2026-10-12)

Build `20261012-065330-40bfacb4` installed at `2026-10-12T06:57:47.0929826-04:00` through
scripts/build-jar.ps1. All 142 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
70 installed files, 17 replacement backups and 2050 archived sources.
All five engine/launcher executable resources report version 1.41.0.0.
ZIP SHA-256: `E0E0B6311C07F3D7366956AF374C30FBCF7D81CD3FD594C1CF5D9DB11C8AA524`.
Receipt: `build/deploy/20261012-065739-33266f93/receipt.json`.
Qualification: `build/mp-combat-1.41.0/qualification.json`.

MP Combat now offers SP's Quick/Legacy ignition and Continuous/Legacy fist
choices. Shared policy preserves accepted SP behavior. Quick ignites local
held blades in 250 ms; server collision/damage/reach remain authoritative.
Continuous punches use native animation hold timers, with no restart while
busy and no queued punch after release. Kicks, grabs and protected actors
retain native rules. The preference is per player; older-server prediction
falls back to native timing. Tests include MP's actual animation-timer code.

Catalog stays at 1,039; feature 53; JRW17/protocol 1050. APIs and wire layouts
are unchanged. All nine settings pages validate atomically. Existing asset
packs are byte-identical to 1.40.0; settings, saves and other mods were preserved.
The MELEE-03/FISTS-01 and MP-01 checklist rows reflect this implementation.

No graphical game or human remote client was launched. Visible ignition and
remote combat acceptance remain open, along with MP Rend2, controller/teleport
prediction gaps, rope scenery interaction and broader gameplay/load testing.
The goal stays active. Next public release: 1.42.0. Receipt notes were added
after packaging; runtime sources remain exact.

## 1.41.0: MP saber ignition and continuous fist controls (2026-10-12)

MP JA:R Settings now has Combat with SP's Quick/Legacy saber extension and
Continuous/Legacy fist choices. Both default to the enhanced mode and apply
live through the same transaction as the other eight settings pages. The
option lists, ignition arithmetic and completed-punch admission policy are
shared with SP. SP's behavior and defaults are preserved.

Quick extends the local player's held energy blades in 250 ms, including
both sabers and staff blades. Retraction, NPCs, other players, thrown sabers,
swords, death, vehicles, emplaced weapons and protected views retain native
MP routes. Repeated rendering in one frame cannot advance ignition twice;
entity resets clear the local clock. MP's server remains authoritative for
collision, damage and reach, which are unchanged by this visual setting.
Legacy keeps MP's original frame-dependent extension calculation.

Continuous primary punches use the actual accepted animation's hold timer,
including its native Rage adjustment. Stationary punches use the full body;
moving/crouching punches use the torso. Held input cannot restart an unfinished
punch or indefinite animation. Release finishes the current punch without
queuing another. Completed kicks can transition back to punches. Alternate
kicks, combined-button grapples, NPCs, nonhumanoids and vehicle/held/Force
states retain their native rules. MP already supports fists without SP's
iknowkungfu cheat; the new setting does not enable that cheat or Force powers.

The native MP punch path alternated animations using PM_StartTorsoAnim, which
does not create a hold timer, then added the weapon's fire delay. The new
adapter uses PM_SetAnim and the accepted duration. Existing firing events,
damage handlers and ammo accounting remain in the native PM_Weapon path.
Legacy preserves its original behavior.

Fist preference uses a bounded native userinfo value, is stored per client
across respawns and feeds the same PM_Weapon code on server and prediction.
The server advertises capability through jar_mpVersion 53; prediction uses
native behavior on older servers. Missing/malformed preferences retain native
behavior. There are no new playerstate/entitystate/usercmd wire fields.

Seven focused suites pass: MP combat and menu, sandbox regression, SP melee,
saber extension and graphics checks. The MP fixture includes the production
native animation-timer implementation and unchanged native saber-length
fallback, covering duration, dual/staff blades, repeated frames, protected
paths, retraction, Rage, release, busy timers and rejected animations. These
are automated checks; real client/server input, damage and visual acceptance
remain open. No graphical game or human remote client was launched.

Catalog remains 1,039; MP feature 53, protocol 1050, JRW17, game API 5, cgame
API 7 and renderer API 12. MP Rend2, controller parity, portable teleport
prediction, rope scenery interaction, terrain/legacy content and broader
remote multiplayer/load/gameplay acceptance remain open. The overhaul goal
stays active. Full build/install and exact-release receipt follow.

## Latest installed release: 1.40.0 (2026-10-12)

Build `20261012-063454-9d4bc743` installed at `2026-10-12T06:39:15.6323741-04:00` through
scripts/build-jar.ps1. All 141 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
70 installed files, 17 replacement backups and 2045 archived sources.
All five engine/launcher executable resources report version 1.40.0.0.
ZIP SHA-256: `BECEC6BFDD17AAE8974644F4DEC5D3EA07C3E0D5FBFED8958F5BF9EAFAF08EA2`.
Receipt: `build/deploy/20261012-063907-a1ca9c63/receipt.json`.
Qualification: `build/mp-faces-1.40.0/qualification.json`.

MP Effects now shares SP's four facial presets and four speech-sensitivity
choices. Both use the same smoothed, speech-linked jaw/lip/brow controller.
MP's engine/renderer bridge preserves native talk/blink, independently owned
bone overrides and legacy voice levels. Tests cover live menu application,
Stock/death/ragdoll cleanup, repeated frames, model/reload resets, missing
rigs and PCM/MP3 sample boundaries. OpenAL uses native voice levels, as in SP.

Catalog stays at 1,039; feature 52; JRW17/protocol 1050. MP cgame API is 7 and
renderer API is 12; all matched modules were installed together. SP APIs and
MP game API remain unchanged. Existing asset packs are byte-identical to
1.39.0; settings, saves and other mods were preserved.

No graphical game or human remote client was launched. Rendered/audible
facial/cosmetic acceptance, MP saber-extension/fist settings, Rend2, controller
parity, teleport prediction and broader gameplay acceptance remain open.
The goal stays active. Next public release: 1.41.0. Receipt notes were added
after packaging; runtime sources remain exact.

## 1.40.0: shared facial presets and speech variation in multiplayer (2026-10-12)

MP JA:R Settings > Effects now offers SP's Stock, Expressive, Exaggerated and
Extreme facial presets, with Subtle/Normal/Sensitive/Very sensitive speech
sensitivity. Changes apply live and participate in the existing atomic
settings transaction. Stock remains the default. Back cancels pending edits;
invalid values reject the entire transaction before any settings change.

SP and MP now use one facial math and bone-control implementation. It adds
smoothed jaw, lower/upper lip and brow accents with per-entity variation tied
to speech energy and energy changes. MP reads its own playing voice samples;
the standard mixer provides a continuous envelope and OpenAL uses native
voice levels, as in SP. This is audio-reactive variation, not phoneme decoding.
Native talk/blink animation and script-facing voice levels remain available.

MP's cgame calls an engine bridge into the renderer-owned skeletal pool.
The adapters respect independent overrides, clear owned accents for Stock,
death and ragdoll, and reset smoothing on model/lifetime/time changes.
Repeated rendering in one frame does not advance the envelope twice. Missing
or incompatible face rigs safely retain their native animation. Sample reads
are bounded for PCM and decoded MP3 windows, including truncated clips.

Nine focused automated suites pass, including production controller, cgame
routing, audio and menu fixtures, plus existing SP facial/graphics checks.
They cover preset/sensitivity choices, atomic refusal, cleanup, independent
bone ownership, model/reload resets, repeated-frame stability and audio
boundaries. No graphical game or human remote client was launched; rendered
and audible acceptance of these controls remains open.

MP cgame API advances to 7 and renderer API to 12 for the new bridge; install
the matched release modules together. SP APIs, MP game API 5, protocol 1050,
JRW17 and the 1,039-choice catalog remain unchanged. MP feature is 52.
MP saber-extension/fist-input settings, Rend2, controller parity, teleport
prediction, terrain/legacy content and real remote multiplayer/load/visual
acceptance remain open. The overhaul goal stays active. Full release build,
installation and independent receipt verification follow this checkpoint.

## Latest installed release: 1.39.0 (2026-10-12)

Build `20261012-061223-96ec7db6` installed at `2026-10-12T06:16:42.9982064-04:00` through
scripts/build-jar.ps1. All 138 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
70 installed files, 15 replacement backups and 2036 archived sources.
All five engine/launcher executable resources report version 1.39.0.0.
ZIP SHA-256: `C050E63C9116E617487DDC76DD789E984EC80CC7641958A0FD549276AE65B36D`.
Receipt: `build/deploy/20261012-061635-5bf2b78c/receipt.json`.
Qualification: `build/mp-effects-1.39.0/qualification.json`.

MP Effects now shares SP's saber-blade, saber-sound and combat-effect choices.
Its private pack reuses 533 SP cosmetic/dependency/notice payloads, adapting
only three muzzle Sound blocks to MP's native firing audio. All other media
match SP. Real effect/shader/image/sound dependencies are closed. The MP audio
hook retains source SFX identity while choosing the correct WAV/MP3 decoder.

All eight settings tabs validate together. Apply restarts audio when needed,
then requests one video reload, including combined display trials. Tests cover
choices, unavailable packs, native fallback and cancellation. Two real pack
builds match byte for byte. Existing asset packs, settings, saves and other
mods were preserved. Catalog remains 1,039; feature 51; JRW17/protocol 1050.

No graphical game or human remote client was launched. Rendered/audible pack
acceptance, MP facial/saber-extension/fist settings, Rend2, controller parity,
teleport prediction and broader gameplay acceptance remain open. The goal
stays active. Next public release: 1.40.0. Receipt notes were added after
packaging; runtime sources remain exact.

## 1.39.0: shared SP cosmetic packs in multiplayer (2026-10-12)

MP JA:R Settings now includes Effects with SP's complete Saber blades,
Saber sounds and Combat effects choices. Their option lists share one header:
Retail/Cinematic narrow/Soft glow/installed Saber GFX; Retail/MBII/Original
Trilogy/Battlefront/Force Unleashed/Plasma sounds; Retail/MBII combat effects.
All eight settings tabs share atomic validation, cancellation and preserved
untouched custom values. Missing packs are refused before any settings change.
Individual absent assets fall back through the native reader to retail.

MP's private jar-effects.pk3 reuses 533 existing SP cosmetic/dependency/notice
files. Three muzzle effects omit their Sound primitives because MP already
plays firing audio in native weapon code. Every other included payload stays
byte-identical to SP, including saber textures, sound samples, shaders, nested
effects and notices. No stock shader, model, script or gameplay definition is
overridden. The existing saber-contact screen-flash suppression remains active.
Native pure-pak rules remain authoritative on both lookup and fallback.

The MP sound loader shares SP's WAV/MP3 selection policy and updates its local
filename, length and decoder extension together, preserving the original SFX
identity. Apply queues one video restart; a changed sound pack first restarts
audio. The native cgame shutdown/restart clears effect registrations. Combined
sound/graphics/display changes still use one video restart and the existing
engine-owned display confirmation.

Production menu/overlay/audio-hook fixtures pass, including all offered choices,
unavailable packs, cross-page atomic refusal, native fallback, both sound
encodings, empty samples, one combined reload and custom-value preservation.
The real MP pack is checked for exact SP bytes, the three limited adaptations,
closed effect/shader/image/sound dependencies and native event/cache/flash routes.
Existing SP graphics/appearance fixtures pass. These are automated/source
checks, not rendered or audible gameplay acceptance. No graphical game was
launched. MP facial expressions, saber-extension/fist-input settings, Rend2,
controller parity, teleport prediction, terrain/legacy content and real remote
multiplayer/load/visual acceptance remain open.

MP feature 51, 1,039 choices, unchanged protocol 1050 and JRW17. The overhaul
goal remains active. Full build/install and receipt evidence follows.

## Latest installed release: 1.38.0 (2026-10-12)

Build `20261012-055847-70ebf664` installed at `2026-10-12T06:02:55.0135725-04:00` through
scripts/build-jar.ps1. All 138 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
69 installed files, 14 replacement backups and 2032 archived sources.
All five engine/launcher executable resources report version 1.38.0.0.
ZIP SHA-256: `086CF33A1444660B1AD5CDB1CD2A229187BF4769BD88DC6008B30321FAEC1AFB`.
Receipt: `build/deploy/20261012-060247-68f9dd22/receipt.json`.
Qualification: `build/mp-display-1.38.0/qualification.json`.

MP's Display page now shares SP's resolution/window choices and size limits.
Custom dimensions choose Custom mode; borderless fullscreen uses the desktop.
An engine-owned 20-second Keep/Restore trial survives UI reload, starts after
the restart completes, and restores active values plus prior pending latches
on timeout or cancellation. All seven settings pages validate before writes.

Menu/engine lifecycle fixtures and SP regression tests pass. All asset packs,
settings, saves and other mods were preserved. Catalog remains 1,039; feature
50, unchanged JRW17/protocol 1050. No graphical game or human remote client was
launched. Display/monitor acceptance, MP Rend2, effects/audio/combat/controller
parity, teleport prediction and broader gameplay acceptance remain open.
The goal stays active. Next public release: 1.39.0. Receipt notes were added
after packaging; runtime sources remain exact.

## 1.38.0: MP display controls and timed confirmation (2026-10-12)

MP JA:R Settings now has a Display page: Desktop/Custom/standard resolution,
custom width and height, Windowed/Exclusive fullscreen/Borderless fullscreen,
and normal/borderless window decoration. These choices and size limits share
SP's policy. Editing dimensions chooses Custom; borderless fullscreen chooses
the desktop mode. All seven settings pages validate together before applying.
Invalid settings change nothing, and combined graphics/display changes request
one video restart. Untouched custom values are preserved.

MP's display backup and 20-second confirmation timer live in the engine,
surviving UI/renderer module reload. The countdown starts after the video
restart completes. Keep accepts the result; Restore, Escape or timeout restores
the previous active display values. Pre-existing pending display latches are
preserved separately and reinstated after rollback. The engine uses real time,
retains UI input during confirmation and permits only one rollback restart.
As with any video restart, this does not recover from a hard driver/process
crash. Reverting the display leaves other successfully applied settings intact.

The actual MP menu and engine-trial code are tested with controlled native
services, including invalid/overflowing sizes, custom/desktop mode selection,
atomic failure across pages, refusal before writes, a single combined restart,
UI state loss, Keep/Restore, timeout, clock wrap and pending-latch preservation.
Existing SP appearance/graphics tests pass. Native renderer/SDL, wire/save/API
contracts and catalog rows are unchanged. No graphical game was launched;
rendered confirmation, monitor/driver behavior and live multiplayer acceptance
remain untested. MP Rend2, effects/audio/combat/controller parity, teleport
prediction and remaining catalog/terrain work remain open.

MP feature 50, 1,039 choices, unchanged protocol 1050 and JRW17. The overhaul
goal remains active. Full build, installation and receipt evidence follows.

## Latest installed release: 1.37.0 (2026-10-12)

Build `20261012-053808-239beb9c` installed at `2026-10-12T05:42:20.3169240-04:00` through
scripts/build-jar.ps1. All 137 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
69 installed files, 15 replacement backups and 2029 archived sources.
All five engine/launcher executable resources report version 1.37.0.0.
ZIP SHA-256: `59323D9EBCCBA19F4E0897AFA8EC2641F112C384D6B2F5836194BD1D01E96326`.
Receipt: `build/deploy/20261012-054212-6cc94a0d/receipt.json`.
Qualification: `build/mp-classic-graphics-1.37.0/qualification.json`.

MP now has Graphics, Textures and Detail pages with fifteen SP controls,
using shared choices and native renderer/cgame consumers. All six settings
pages share one validated transaction, preserve untouched custom values and
request one reload only for settings that require it. Catalog remains 1,039;
feature 49, unchanged JRW17/protocol 1050.

Native menu/graphics fixtures and exact SP choice expansion pass. Rendered
quality, hardware behavior and performance remain untested. The interface
pack now has stable timestamps; asset member bytes and native pak CRC
sequences are unchanged. Settings, saves and other mods were preserved.
No graphical game or human remote-client session was launched. MP display
confirmation, Rend2, effects/audio/combat/controller parity, teleport prediction
and real gameplay acceptance remain open; the goal stays active.
Next public release: 1.38.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.37.0: shared native classic graphics controls in MP (2026-10-12)

MP JA:R Settings now has Graphics, Textures and Detail pages with fifteen
SP controls: MSAA, dynamic glow, VSync, frame limit, FPS display, texture detail,
texture filtering, anisotropic filtering, model detail, curved surfaces,
character shadows, dynamic lights, grass/foliage, impact marks and simple sky.
The option lists and MP control metadata share one header; SP's existing
choices expand exactly as before. MP uses its existing native consumers.
Hardware capability limits and native fallback behavior remain authoritative.

All six settings pages share one pending transaction. Apply validates every
changed option before writing anything. Restart-sensitive settings request one
video reload, including when combined with HUD/text/artwork changes; live
settings use their native updates. Merely opening a page changes no runtime
settings. Untouched custom cvar values, even those outside the offered choices,
remain unchanged; Revert and Back discard pending edits. This adds no renderer
preset that would overwrite unrelated preferences.

The production MP menu fixture covers all fifteen controls, exact shared
choices, live/restart routing, atomic cross-page refusal, combined one-reload
behavior, no-op Apply, custom values and cancellation/parent return. Existing
SP graphics menu/policy tests pass. Source audits verify all native graphics
consumer code is unchanged. These checks do not prove visual quality, driver
behavior, frame rate or graphical video-restart acceptance.

The interface pack builder now fixes the manifest entry timestamp, avoiding
archive-byte changes when its contents are unchanged. Its production script
produces identical small fixture archives under two different clocks. Full
release readback compares real asset members and native CRC sequences against
the previous package. No font/UI asset content changes are intended.

MP feature 49, 1,039 choices, protocol 1050 and JRW17. Display selectors still
need MP-specific timed Keep/Revert handling across UI module reloads; the
existing native video menu remains available. Rend2 and its advanced graphics
are not yet ported to MP. Effects/audio/combat/controller parity, teleport
prediction, terrain/legacy pickups and real multiplayer/load/visual acceptance
remain open. The overhaul goal remains active; full release evidence follows.

## Latest installed release: 1.36.0 (2026-10-12)

Build `20261012-052558-0ecdb355` installed at `2026-10-12T05:30:00.8624494-04:00` through
scripts/build-jar.ps1. All 137 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
69 installed files, 18 replacement backups and 2028 archived sources.
All five engine/launcher executable resources report version 1.36.0.0.
ZIP SHA-256: `F01EB24CEAAB8A0E6EE59AE124759B79A3AE11F1E3FF1032238D2ACB62E8212E`.
Receipt: `build/deploy/20261012-052953-87386883/receipt.json`.
Qualification: `build/mp-hud-motion-1.36.0/qualification.json`.

MP now exposes Academy/Simple numeric HUDs and Reduced/Normal UI motion.
SP/MP share native menu-motion logic, choice data and classic-renderer clock
scoping. HUD selection reloads video once on Apply; motion changes live.
Native HUD consumers and MP normal motion remain unchanged. Catalog remains
1,039; feature 48, unchanged JRW17/protocol 1050.

Native SP/MP motion, MP surface-clock and menu transaction fixtures pass.
Rendered HUD/motion, graphical reload and remote acceptance remain open.
Asset member bytes, settings, saves and other mods were preserved. The MP
interface ZIP manifest timestamp changes on rebuild; fixing that build-only
nondeterminism remains a follow-up. No graphical game or human remote-client
session was launched. Broader graphics/effects/audio/combat/controller parity,
teleport prediction and gameplay acceptance remain open; the goal stays active.
Next public release: 1.37.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.36.0: MP HUD choices and shared reduced UI motion (2026-10-12)

MP's Interface page now exposes Academy/Simple numeric HUD layouts and
Reduced/Normal UI motion, matching SP. Both use shared option data. HUD changes
validate the native retail menu resource and request one video reload after
successful Apply, including when combined with text/artwork changes. Motion
applies live. The three settings pages retain atomic Apply, Revert, Back and
untouched custom values. Existing custom HUD paths are preserved unless the
owner explicitly chooses one of the two built-in layouts.

The HUD drawing/loading paths already existed in MP and remain unchanged.
SP/MP now execute SP's same native menu orbit/rectangle-transition code:
Reduced stops orbiting and finishes rectangle transitions at their endpoint;
Normal resumes native timing. Model transitions, fades, input, scripts and
cinematic playback retain native behavior. UI motion defaults to Normal.

MP's classic renderer scopes its float, millisecond and shader clocks to zero
only while completing a reduced-motion 2D surface. All clocks restore on normal
and early-return paths. World surfaces continue using normal time. SP's classic
renderer shares the clock scope with equivalent prior behavior; SP Rend2's
existing implementation is unchanged. This also applies to native HUD menus.

Native surface-completion fixtures cover normal/reduced UI, world draws,
empty/debug/sky/shadow returns and clock restoration. Shared production item
motion fixtures run against both SP and MP UI structures and cover orbit
cadence, freeze/resume, transition bounds/endpoints and unrelated flags. The
production MP menu fixture covers HUD resource validation, custom preservation,
invalid transactions, one reload and cancellation. Source audit proves SP's
item motion is copied exactly and MP's normal motion remains equivalent, with
native HUD consumers unchanged. Rendered appearance, graphical reload and real
multiplayer acceptance remain open; these fixtures do not establish them.

MP feature 48, 1,039 choices, protocol 1050 and JRW17. Broader graphics,
effects/audio/combat and controller parity remain open, along with teleport
prediction, terrain/legacy pickups and real multiplayer/load/visual acceptance.
The overhaul goal remains active. Full release and installation evidence follow.

## Latest installed release: 1.35.0 (2026-10-12)

Build `20261012-051030-65dc8f1c` installed at `2026-10-12T05:14:34.5826274-04:00` through
scripts/build-jar.ps1. All 134 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
69 installed files, 14 replacement backups and 2023 archived sources.
All five engine/launcher executable resources report version 1.35.0.0.
ZIP SHA-256: `1342591760590B870FBFF7CF5B71D306576506BCEE75E81122D48BC1BC49673A`.
Receipt: `build/deploy/20261012-051428-ae0252a6/receipt.json`.
Qualification: `build/mp-interface-parity-1.35.0/qualification.json`.

MP now has an Interface page for SP's text/UI artwork and font detail choices.
The native filesystem loads selected font/UI assets with normal pure checks
and retail fallback. All three settings pages share validated pending edits;
text/artwork changes request one video reload only after successful Apply.
Independent package readback verified 1766 exact SP font/UI/notice files in MP.
Catalog remains 1,039; feature 47, unchanged JRW17/protocol 1050.

SP's asset policy and choices are unchanged through shared implementations.
Native menu/adapter fixtures and asset readback passed. Rendered appearance,
graphical reload and remote pure-server behavior still need owner acceptance.
Existing asset packs, settings, saves and other mods were preserved. No
graphical game or human remote-client session was launched. Broader settings,
controller parity, teleport prediction and gameplay acceptance remain open;
the goal remains active.
Next public release: 1.36.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.35.0: shared SP/MP text and UI artwork (2026-10-12)

MP JA:R Settings now has an Interface page with SP's Retail/Crisp Sans/Crisp
Serif text, Retail/2x smooth/4x sharpened UI artwork, and Normal/High/Maximum
font variant detail. SP/MP share the choice lists and namespaced asset policy.
New MP defaults match SP (sans/up2); existing archived settings are preserved.
The MP package contains unmodified font/UI assets and notices from the SP
appearance pack. It introduces no replacement menus or new hand-drawn icons.

The MP filesystem adapter tries selected font/UI assets through the native
reader, then falls back to the original path if unavailable. Pure-server rules,
search order, handles and archive checksums remain native. Model, map, config,
save, code, world-lighting, saber/sound/effect reads are outside this adapter.
The native font reader and image renderer consume the same files and retain
glyph metrics. Stock selections skip the overlay. Custom pack cvars remain
usable; the menu offers the qualified built-in choices only.

Camera, Console and Interface share one pending transaction. Apply validates
all changed settings and probes selected pack resources before any write.
Missing/disallowed packs refuse the transaction with a status message. Text
or artwork changes append one video reload after successful Apply; font detail
uses its existing live native consumer. Revert and Back discard pending edits.
Untouched custom values remain unchanged. Server asset restrictions can cause
individual files to fall back to Retail; they are never bypassed.

Production adapter-call and menu fixtures cover fallback, path normalization,
stock/custom selections, excluded asset classes, unique handles, invalid/missing
packs, atomic cross-page edits, single/no reload, Revert/Back and custom values.
Source audit proves SP's policy/choices expand exactly as before and MP's native
reader (including pure checks) and font/image consumers are unchanged. Package
readback compares every MP font/UI/notice byte against SP. These checks do not
establish rendered font quality, reload behavior in a graphical session, or
remote pure-server acceptance; those remain open for owner testing.

MP feature 47, 1,039 choices, protocol 1050 and JRW17. UI-motion/HUD, broader
graphics, effects/audio/combat and controller parity remain open, together with
teleport prediction, terrain/legacy pickups and real multiplayer/load/visual
acceptance. The overall goal remains active. Full release evidence follows.

## Latest installed release: 1.34.0 (2026-10-12)

Build `20261012-045532-0a83cece` installed at `2026-10-12T04:59:13.3438882-04:00` through
scripts/build-jar.ps1. All 133 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
68 installed files, 14 replacement backups and 2018 archived sources.
All five engine/launcher executable resources report version 1.34.0.0.
ZIP SHA-256: `B1A6BB584442B3BC3FB6CE62F5EF85CED332328C2D9CC8AE0477EC9B9E2A02CB`.
Receipt: `build/deploy/20261012-045908-e3ec8801/receipt.json`.
Qualification: `build/mp-console-parity-1.34.0/qualification.json`.

MP now has a Console page for Retail/Slate/Amber/Black backgrounds, opacity,
text size and height. SP/MP share the palette and menu choices. Camera/Console
pending edits persist across tabs and Apply validates the full transaction.
Production menu fixtures and native console source audits pass; rendered
appearance and the new page still need owner acceptance. Catalog remains
1,039; feature 46, unchanged JRW17/protocol 1050.

SP's palette/choice data now use the shared implementation, preserving prior
behavior. Asset packs, settings, saves and other mods were preserved. No
graphical game or human remote-client session was launched. Broader settings,
controller parity, teleport prediction and gameplay acceptance remain open;
the goal remains active.
Next public release: 1.35.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.34.0: shared SP/MP console settings (2026-10-12)

MP JA:R Settings now has Camera and Console pages. Console exposes SP's
Retail, Slate, Amber and Black backgrounds, 25/50/75/100-percent opacity and
height, and 100/150/200/300/400-percent text size. Background colors and menu
choice lists are shared by SP and MP. Retail keeps the native console texture;
flat styles use native rectangle drawing, and a full-screen console stays
opaque. MP's new jar_consoleStyle setting is archived and defaults to Retail.

Opacity, size and height use MP's existing native consumers. Console text,
scrollback, timestamps, command entry, resize and open/close animation paths
are unchanged. Existing SP palette values and menu choices are preserved
through the shared implementation; no assets or shaders are introduced.

Camera/Console page switches preserve one pending transaction. Apply validates
both pages before writing any value. Revert reloads current settings, and Back
returns to the original main/pause menu without applying pending changes.
Untouched custom values, including values outside the offered choices, remain
unchanged. Host settings and unrelated controls are outside this transaction.

The production MP menu fixture covers all four controls, both page directions,
atomic invalid-input refusal, custom-value preservation, edge choices, Revert,
Back, parent return and virtual-canvas bounds. Existing SP graphics menu/policy
checks pass. Source audit verifies the native console bodies differ only by the
shared background call, MP style registration and includes; SP menu choices
expand exactly to their previous data. Rendered console appearance and the
new page still need owner acceptance. Full release/installation evidence follows.

MP feature 46, 1,039 choices, protocol 1050 and JRW17. Broader interface/font,
HUD, graphics, effects/combat and controller parity remain open, alongside
teleport prediction, terrain/legacy pickups and real multiplayer/load/visual
acceptance. This closes the basic MP console-controls gap, not all settings
parity. The overall goal remains active.

## Latest installed release: 1.33.0 (2026-10-12)

Build `20261012-044437-64dfed0f` installed at `2026-10-12T04:48:13.8942831-04:00` through
scripts/build-jar.ps1. All 133 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
68 installed files, 15 replacement backups and 2017 archived sources.
All five engine/launcher executable resources report version 1.33.0.0.
ZIP SHA-256: `36B309CD224070007DDFF2156C143019098E9ACD705B35A6216F0E3799D55E01`.
Receipt: `build/deploy/20261012-044808-aacce8dd/receipt.json`.
Qualification: `build/mp-camera-parity-1.33.0/qualification.json`.

SP and MP now share Legacy/Comfort/Crisp camera math. MP exposes the archived
selector in JA:R Settings with validated Apply/Revert behavior. Native view
and menu fixtures passed for bob/run scaling, chase damping, collision/mover
traces, special-view bypass and unchanged custom cvars. The new MP camera feel
and selector still need graphical owner acceptance. Catalog remains 1,039;
feature 45, unchanged JRW17/protocol 1050.

SP's existing view/menu integration is unchanged; its preset wrapper now calls
the shared implementation. Asset packs, settings, saves and other mods were
preserved. No graphical game or human remote-client session was launched.
Broader settings/controller parity, teleport prediction and gameplay acceptance
remain open; the goal remains active.
Next public release: 1.34.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.33.0: shared SP/MP camera presets (2026-10-12)

MP JA:R Settings now offers SP's Gameplay camera choices: Legacy / custom,
Comfort and Crisp. Comfort scales first-person run/bob to 25 percent and sets
chase damping to 0.7; Crisp removes run/bob and chase lag. Legacy retains the
existing custom bob/damping values. Both executables now call the same small
shared preset implementation; SP keeps its existing menu and view integration.

MP applies this only to ordinary local gameplay cameras. Vehicle, zoom,
emplaced, spectator/follow, held-monster, falling-death, camera-mode and saber
action-camera paths retain native behavior. The adapter checks both snapshot
and predicted states at transitions. Native weapon/damage kick, landing,
duck/step smoothing, death/intermission views, collision clipping, moving-brush
retrace and teleport snapping remain intact. No run/bob/damping cvars are
rewritten, and no movement, aim, server policy or protocol changes are needed.

The archived jar_cameraPreset setting defaults to Legacy. MP Apply validates
the whole camera transaction before changing values; Revert and Back discard
pending edits. Unchanged custom values outside menu ranges are preserved.
Opening the menu does not reset existing settings. The native menu widgets
and virtual-canvas layout have production fixture coverage, but the new
selector and resulting view feel still need graphical owner acceptance.

New tests execute the extracted production first-person/chase functions with
controlled ideal positions and collision imports. They cover normal/crouched
run/bob, shared coefficients, native kick/landing/death, protected views,
clipping, moving-brush retrace and teleport behavior. The existing SP graphics
policy test passes through the shared implementation. MP menu fixtures cover
all choices, invalid input, Apply/Revert/Back and custom-value preservation.
Source extraction is checked against 1.32.0: only the preset coefficients and
adapter calls change within those native camera paths. This is not rendered
camera or actual network acceptance. Full build and installation follow.

MP feature 45, catalog 1,039, protocol 1050 and JRW17. No asset or save-format
change. Broader SP/MP settings, presentation/combat, controller catalog,
portable teleport prediction and real multiplayer acceptance remain open;
terrain and legacy pickup dispositions also remain open. Overall goal active.

## Latest installed release: 1.32.0 (2026-10-12)

Build `20261012-043141-8541d25f` installed at `2026-10-12T04:35:18.3068701-04:00` through
scripts/build-jar.ps1. All 132 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
68 installed files, 15 replacement backups and 2010 archived sources.
All five engine/launcher executable resources report version 1.32.0.0.
ZIP SHA-256: `4F5CF199FBA87179D2DF16EFC7C1394AE8ABC970846FF3C9C8CE4AFFC8685E9C`.
Receipt: `build/deploy/20261012-043512-830ad6ea/receipt.json`.
Qualification: `build/mp-vehicle-push-prediction-1.32.0/qualification.json`.

MP now predicts portable push volumes for occupied vehicles as well as
players. Separate pilot masks apply current native seat/owner/consent checks;
prediction uses final vehicle bounds and retains launch momentum through
subsequent steering. Four-family production server/client, seat-policy,
replay, native movement and snapshot-codec fixtures pass. Catalog remains
1,039; feature 44, unchanged JRW17/protocol 1050. Portable teleport prediction
and real remote-client latency, visuals and gameplay acceptance remain open.

SP runtime sources and asset packs are unchanged. Settings, saves and other
mods were preserved. No graphical game or human remote-client session was
launched. Remaining controller/settings parity, specialized prediction
and gameplay acceptance stay open; the goal remains active.
Next public release: 1.33.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.32.0: MP occupied-vehicle push prediction (2026-10-12)

Pilots now predict portable trigger_push volumes after native vehicle movement,
using the vehicle's final collision bounds. Linear, relative and arc pushes
share the server trajectory calculation and momentum replacement. Horizontal
launch velocity enters the existing vehicle drift path, so steering on later
commands retains the impulse. Input, orientation and seat attachments stay
under the native vehicle controller.

Sensors publish separate 32-bit player and pilot permission masks in existing
snapshot fields. The server grants pilot prediction only for the current
settled vehicle with permitted occupants, applying the same ownership and
consent checks as authoritative touch. Passengers cannot predict its movement.
Boarding, held/frozen vehicles, mismatched seats, missing vehicles and invalid
trajectories refuse prediction. Disabled sensors clear both permission masks.
Cooldown and one-shot state reset from the snapshot before each command replay.
Server touch still rechecks current permissions; remote changes can arrive
after a predicted command and require ordinary authoritative reconciliation.

Production client/server fixtures compare all three trajectories across four
vehicle families, final-hull contact, moving volumes, cooldown/once replay,
all 32 pilot bits, passenger opt-out, boarding, ownership and invalid seat
references. Native snapshot codecs check late-join/delta permission fields.
The native step/slide fixture checks predicted push momentum through forty
subsequent steering outputs for each family. These are controlled fixtures,
not a graphical client, real latency measurement or human driving acceptance.

No new assets, cards, protocol/API or world format. MP feature 44, catalog
1,039, protocol 1050 and JRW17. SP runtime and native vehicle steering/seat
policy remain unchanged. Full build, resource checks and verified installation
follow in the release receipt. One initial fixture cleanup assertion was fixed:
WorldClear correctly refuses occupied vehicles; runtime behavior was correct.

Still open: portable teleport prediction, remote multiplayer/late-join/load
acceptance, broader controller/settings parity, specialized breakable behavior,
terrain disposition and legacy pickup resources. The overall goal stays active.

## Latest installed release: 1.31.0 (2026-10-12)

Build `20261012-041546-ff57d68a` installed at `2026-10-12T04:19:13.5803346-04:00` through
scripts/build-jar.ps1. All 132 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
68 installed files, 15 replacement backups and 2010 archived sources.
All five engine/launcher executable resources report version 1.31.0.0.
ZIP SHA-256: `AA5F87E1FABF04587A11EA98C51AE4208E5B124A5C8BB5067A3683A44CA3EF7A`.
Receipt: `build/deploy/20261012-041909-ca4ad354/receipt.json`.
Qualification: `build/mp-push-prediction-1.31.0/qualification.json`.

MP now predicts player movement through portable push volumes, including
rotated/moving geometry, linear/relative/arc trajectories, cooldown/once replay
and all 32 authoritative client-consent bits. Prop-triggered cooldowns publish
in the same frame. The actual client trigger loop, shared server/client paths
and native message codecs passed their new regressions. Catalog remains 1,039;
feature 43, unchanged JRW17/protocol 1050. Occupied vehicle/teleport prediction
and real remote-client latency, visuals and gameplay acceptance remain open.

SP runtime sources and asset packs are unchanged. Settings, saves and other
mods were preserved. No graphical game or human remote-client session was
launched. Remaining controller/settings parity, specialized prediction
and gameplay acceptance stay open; the goal remains active.
Next public release: 1.32.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.31.0: MP portable push-volume prediction (2026-10-12)

Portable trigger_push volumes now publish their geometry, current motion,
target, mode, cooldown and server-approved player mask through their existing
sensor. The client predicts linear, relative and arc pushes in the native
trigger-touch loop. Server and client share oriented contact and trajectory
calculations; rotated sensors reject the extra corners of their enclosing
box. Source motion extrapolates for at most 100 ms, like prop rider prediction.
Only the most recent known target position is available to the client.

Cooldown and one-shot decisions are local to each command replay and rebuild
from authoritative snapshot state on the next pass. A snapshot's consent bits
cover all 32 client slots. Death, spectator/follow mode, active grip, boarding,
invalid geometry and invalid trajectories refuse prediction. Disable/cleanup,
missing or protected destinations, and player permission changes remove or
update the sensor. Prop-triggered cooldowns now publish in the same frame.
Server touch still rechecks current consent and permissions on every action.

No new helper allocation, menu card, asset, protocol, API or world format.
Catalog remains 1,039; MP feature 43, protocol 1050, JRW17. Authored BSP pads,
teleports and pickup prediction retain their original native branch. SP runtime
is unchanged. Existing vehicle, NPC and prop trigger routes remain authoritative.

New production-code fixtures exercise the actual client trigger-touch loop,
shared geometry/velocity, translated and rotated volumes, linear/relative/arc
pushes, cooldown/multiple/once replay, high client consent bits, malformed
snapshots, and unchanged native item/BSP-pad/teleport handling. Server fixtures
compare published and authoritative velocities and verify current consent,
motion and post-touch cooldown publication. Native message codecs round-trip
all sensor fields in late-join and delta snapshots. Full release evidence and
verified installation follow; these are not rendered/network-latency acceptance.

Remaining prediction scope: occupied vehicle trigger movement and portable
teleport arrivals (which require server collision/seat transactions). Other
clients can consume shared cooldowns or change moving geometry/consent before
their updates arrive, so authoritative reconciliation still applies. Real
remote-client latency, visuals and gameplay acceptance remain open. Remaining
SP/MP controller/settings parity, detailed breakable variants, terrain decision,
legacy pickups and broad performance/campaign acceptance remain open. Goal active.

## Latest installed release: 1.30.0 (2026-10-12)

Build `20261012-035648-aab8acb3` installed at `2026-10-12T04:00:18.7972855-04:00` through
scripts/build-jar.ps1. All 131 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
68 installed files, 15 replacement backups and 2004 archived sources.
All five engine/launcher executable resources report version 1.30.0.0.
ZIP SHA-256: `B3EFB73778BB4CE31E31171479D95B5D08F4821DFF632A95A53FC16FD3A9B49F`.
Receipt: `build/deploy/20261012-040013-2e1664c9/receipt.json`.
Qualification: `build/mp-breakables-1.30.0/qualification.json`.

MP now includes three portable breakable families and 1,039 choices; feature
42, unchanged JRW17/protocol 1050. Production fixtures passed for health,
effects, damage policy, bounded allocation, nonrecursive destruction, repair,
physics dormancy and persistence. The private native server loaded/restored
twelve intact/broken/pending/disabled states, ran scheduled destruction and
checked real model bounds and saved cooldowns. Weapon input, rendered effects
and multiplayer gameplay acceptance remain untested by these checks.

SP runtime sources and asset packs are unchanged. Settings, saves and other
mods were preserved. No graphical game or human remote-client session was
launched. Optional authored brush/model breakable behavior, remaining parity, prediction
and gameplay acceptance stay open; the goal remains active.
Next public release: 1.31.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.30.0: portable MP breakables (2026-10-12)

MP now includes func_breakable, func_glass and misc_model_breakable: 1,039
menu choices, using SP's existing stock crate model/icon. As in SP's portable
catalog, glass uses the breakable-object route with the stock glass material;
it is not an arbitrary BSP glass pane. Intact objects take weapon damage,
fire Secondary when hurt (with a configurable cooldown), and fire Primary
on destruction. Brush/glass Use can signal without breaking; model Use can
ignore breaking. Direct USE is optional. Damage filters use the native
saber/heavy-weapon flags; invulnerable objects can still be activated.

C properties expose health (1-1000), destruction delay, damage policy, direct
USE, use mode, Repair, Enabled, stock debris material, chunk amount, splash
damage/radius and explosion effects. Native radius damage retains visibility
and falloff with current Sandbox permission checks. Model No Explosion also
suppresses splash, matching SP; brush/glass only suppress the visual explosion.
Destruction always queues at least one server tick to avoid recursive splash
chains. Optional effect entities use bounded allocation and may be omitted
at capacity without losing the object state or linked outputs.

Destroyed objects retain identity, pose and attachment records for context,
duplication and saving. They release Physgun holds, lose collision and suspend
physics/forces; Repair refuses occupied space. Property changes support Undo
with revision checks. JRW17 preserves health, intact/destroyed/pending state,
remaining destruction time and pain-output cooldown. Reconstruction does not
replay effects/outputs or retain activator pointers. Repair Undo restores the
pending mode with a fresh configured delay, not its former absolute deadline.
No protocol/API/save-format change; MP feature 42. SP runtime/assets unchanged.

Focused server/UI fixtures cover production callbacks, stock event payloads,
allocation refusal, permissions, nonrecursive destruction, Box3D/fallback
dormancy, repair obstruction, typed edits/Undo and world round trips. The
private native server pipeline checks twelve intact/broken/pending/disabled
states across all three families, scheduled destruction, actual model bounds,
saved cooldowns and reconstruction. Engine weapon input, radius falloff and
rendered debris are not simulated by the fixtures; human gameplay acceptance
remains open. Full release verification and installation evidence follows.

These are useful model-backed controllers, not complete authored-map parity.
Brush portals/geometry, impact/crusher behavior, attached missile destruction,
alternate damaged/use models, dead-solid variants, custom chunks/scripts and
special model-specific behavior remain open. Terrain disposition, legacy MP
pickups, remaining controllers/settings, prediction and broader multiplayer
gameplay/load/performance acceptance also remain open. Goal stays active.

## Latest installed release: 1.29.0 (2026-10-12)

Build `20261012-033129-660b274f` installed at `2026-10-12T03:34:36.7248829-04:00` through
scripts/build-jar.ps1. All 131 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
68 installed files, 15 replacement backups and 2000 archived sources.
All five engine/launcher executable resources report version 1.29.0.0.
ZIP SHA-256: `BB1DFDF34E0433BB3140A8C54B93D4A24A32B3900FB474306051A6B7EA145965`.
Receipt: `build/deploy/20261012-033431-df8b4924/receipt.json`.
Qualification: `build/mp-usable-1.29.0/qualification.json`.

MP now includes portable func_usable and 1,036 choices; feature 41, unchanged
JRW17/protocol 1050. The native server loaded/restored six usable states,
including actual reappearance and cooldown completion, saved pending cooldowns,
hidden/spent/disabled state and real stock-model bounds. The first native check
used too short a cooldown for its own RCON readback; the timing-corrected check
passes with separate expired and persistent cooldown cases. No runtime change
was needed for that test correction.

SP runtime sources and asset packs are unchanged. Settings, saves and other
mods were preserved. No graphical game or human remote-client session was
launched. Optional authored shader/brush behavior, remaining parity, prediction
and gameplay acceptance stay open; the goal remains active.
Next public release: 1.30.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.29.0: portable MP func_usable (2026-10-12)

The MP menu now offers func_usable using SP's existing stock crate model/icon,
bringing the catalog to 1,036 choices. It uses native toggle, button, pain and
death callbacks with a model-backed geometry adapter. Native map brush behavior
is unchanged. Toggle hides/shows the body, firing Primary on hide and Secondary
on return. The default block check delays reappearance until the space clears;
the current moved/rotated pose is retained. Button mode stays visible and fires
Primary, with a configurable reuse delay (0 runs once until Rearm).

C properties expose mode, visibility, reuse delay, health (0-1000; initial 0
is invulnerable), direct USE, NPC-only direct use, block checking, Rearm/heal
and Enabled. Requests are identity-bound and bounded, with conflict-aware Undo;
showing into occupied space is refused when block checking is enabled. Existing
links, physics/Physgun, duplication and world saves apply. Hidden bodies release
holds, lose collision and stop accumulating gravity/thruster forces. Attachments
are retained and suspend while hidden, resuming on return. Native damage dispatch
remains the engine's responsibility; callbacks preserve shoot-to-use behavior.

JRW17's existing typed state fields store visibility, health, button availability
and pending return/cooldown. Copies and saves do not replay outputs or retain
activator pointers. No save-format, network protocol or API change; feature 41.
All previous 1,035 MP rows, SP runtime sources and asset packs are unchanged.

Focused server/UI fixtures cover actual native callbacks, links, blocked return,
Box3D/fallback dormancy and attachments, health callbacks, current consent,
typed edits/Undo and saved pending states. The existing private native server
pipeline now also loads/restores six usable states and checks real model bounds,
scheduled reappearance, cooldown, hidden, spent and inactive persistence.
These checks are not rendered or human remote-client gameplay acceptance.

This ports the usable object's toggle/button and damage behavior. Optional
authored shader animation/brush area portals are not provided by the stock
crate adapter; broader model/brush/controller and settings parity remain open.
Terrain's owner decision, legacy MP pickups, moving-volume prediction and broad
gameplay/save/render/performance acceptance remain open. Goal stays active.

## Latest installed release: 1.28.0 (2026-10-12)

Build `20261012-030851-ca274adb` installed at `2026-10-12T03:11:44.1329160-04:00` through
scripts/build-jar.ps1. All 131 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
68 installed files, 15 replacement backups and 1995 archived sources.
All five engine/launcher executable resources report version 1.28.0.0.
ZIP SHA-256: `BAF35D5BA74AA85ED5580C049E1ECC2EC20652D532AAD69BED8A38BF75E26F92`.
Receipt: `build/deploy/20261012-031139-0252607b/receipt.json`.
Qualification: `build/mp-stations-1.28.0/qualification.json`.

MP now includes five functional native station families and 1,035 choices;
feature 40, JRW17 with JRW1-16 reads, protocol 1050. Native station spawn,
recharge, model bounds and save/restore passed in the existing private server
session. SP runtime sources and asset packs are unchanged. Settings, saves and
other mods were preserved. No graphical game or human remote-client session
was launched. Remaining parity, prediction and gameplay acceptance stay open;
the goal remains active.
Next public release: 1.29.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.28.0: usable MP ammo, shield and health stations (2026-10-12)

MP now has 1,035 choices. Four SP station cards (ammo/shield floor units and
ammo/shield model converters) and MP's native health converter can be spawned
directly with existing retail models/icons, full posed collision, Physgun/tools,
native USE and C/linked activation. Existing catalog rows are unchanged.

The five station adapters reuse native dispensing and recharge callbacks;
map-authored station behavior is unchanged. Owned stations enforce current
consent, reject empty finite supply, clamp drained charge, deduct health grants
from finite supply and apply a 100 ms minimum dispensing interval to the floor
ammo path. Native ammo/double-ammo, Siege armor and per-use grant rules remain.
Recharge stops during use; stale/replaced activators cannot retain the sound
loop. Disable stops sound immediately. Native recharge adds one charge per
configured interval. MP's native station defaults are retained: capacity 200,
100 ms recharge, with editable alternatives; this is not a claim of numerical
SP station-balance parity.

C properties show capacity, remaining charge, recharge interval, unlimited
supply, Refill and Enabled. Bounded identity-based edits and conflict-aware Undo
apply. Copies and JRW17 preserve charge, capacity, recharge deadline and pending
use cooldown; loops/users are transient. JRW1-16 remain readable. JRW17 adds one
four-byte cooldown field (284-byte nodes). Feature version 40; protocol 1050
and API versions unchanged. SP runtime sources and asset packs are unchanged.

Fixtures execute native dispensing/recharge, direct use and links, all five
families, NPC/player consent, Siege armor/double ammo, empty/unlimited supply,
typed menu requests, Undo, current-client lifetime, copies and JRW17/legacy 16.
The existing private native server test also loads/restores all five stations,
reads real model bounds and checks timed recharge/disabled state and cooldowns.
Engine input/collision/network/render imports in fixtures remain distinct from
gameplay acceptance; no graphical or human remote-client session is launched.
Remaining controller/settings/presentation parity, terrain disposition, legacy
MP pickups, moving-volume prediction and broad live acceptance remain open.
Goal remains active; this release does not close those requirements.

## Latest installed release: 1.27.0 (2026-10-12)

Build `20261012-025312-a5e8940a` installed at `2026-10-12T02:55:51.0160121-04:00` through
scripts/build-jar.ps1. All 131 automated suites and the pipeline's retail
resource/BSP/native droid/YT-1300 checks passed. Independent readback verified
68 installed files, 15 replacement backups and 1990 archived sources.
All five engine/launcher executable resources report version 1.27.0.0.
ZIP SHA-256: `5F57F62A1A7E5827FE53E122ED2758D75CD0871FB61F446EA70CAF9790C66F4B`.
Receipt: `build/deploy/20261012-025545-aabe734b/receipt.json`.
Qualification: `build/mp-prop-push-1.27.0/qualification.json`.

MP push volumes now affect both Box3D and fallback props with swept translation,
preserved spin, current ownership/freeze/hold guards, shared native cooldowns
and saved launch motion. MP remains at 1,030 choices; feature 39, JRW16,
protocol 1050. SP runtime sources and asset packs are unchanged. Settings,
saves and other mods were preserved. No graphical game or human remote-client
session was launched. Remaining parity, exact moving/rotating volume sweeps,
prediction and gameplay acceptance stay open; the goal remains active.
Next public release: 1.28.0. Receipt notes were added after packaging;
runtime sources remain exact.

## 1.27.0: MP push volumes affect physics props (2026-10-12)

Portable MP push volumes now launch model props and controller bodies through
the ordinary server frame with either Box3D or fallback physics. Linear,
relative and arc modes share the actor trajectory checks, linked-destination
permissions and speed limits. Launches set exit velocity regardless of mass,
retain spin and attachments, wake rigid bodies, and publish the new velocity.
Frozen, held, pending-copy and protected objects are excluded, as are a
volume's own visible body and its linked destination.

Engine broad-phase queries and oriented swept-AABB clipping catch props that
translate across an entire volume between frames. Slot epochs reject stale
paths after deletion/reuse. Actor and prop contacts share the native positive
cooldown and same-frame policy; once mode disarms after the first accepted
contact. Context help now distinguishes continuous, once and cooldown modes.
JRW16 already saves the resulting prop motion and spent/cooldown state; there
is no format or protocol change. MP feature version is 39; 1,030 choices remain.

Production-frame fixtures exercise real Box3D and fallback motion, mass and
spin preservation, momentum after exit, all push modes, current permissions,
freeze/hold/copy restrictions, shared actor/prop cooldown, one-shot state,
swept rotated contact, slot reuse and JRW16 motion restoration. Engine area
queries, collision traces and network/render services are fixture imports.
Rotating prop sweeps use a conservative hull envelope; moving volumes use
their current pose. Exact rotational/relative-volume sweeps and dynamic-volume
client prediction remain open, along with graphical/remote-client acceptance,
the terrain owner decision, three legacy MP pickups and broader SP/MP parity.
SP runtime and asset packs remain unchanged. Goal stays active.

## Latest installed release: 1.26.0 (2026-10-12)

Build `20261012-023943-4639ddf2` installed at 02:42 EDT through scripts/build-jar.ps1.
All 131 automated suites and retail resource/BSP/droid/YT-1300 checks passed.
Transport-volume coverage includes native push modes and teleport, current
permissions, cooldown/once/rearm, spectator filtering, four vehicle families,
obstructed passenger/droid arrival, copies/JRW16, legacy 14/15 and rollback.
Context input/echo checks passed. No graphical game or human remote-client
session was launched.

Independent readback verified 68 installed files, 15 replacement backups
and 1988 archived sources. All five engine/launcher executables report
Windows version 1.26.0.0. ZIP SHA-256:
`BB6752B368C777488D66E0491553918D68D62876BD6C9482F32C7B5029A1AC87`.
Receipt: `build/deploy/20261012-024213-d65dd5a4/receipt.json`.
Qualification: `build/mp-transport-triggers-1.26.0/qualification.json`.

MP has 1,030 choices, feature 38, JRW16 with JRW1-15 reads, protocol 1050 and APIs
5/6/11. SP runtime sources and asset packs are unchanged. Settings, saves and
other mods were preserved. Terrain's owner disposition, three legacy MP pickup
adapters, movable-volume prediction/Box3D pushes, broader MP parity and remaining
gameplay/save/rendered/network acceptance stay open. The goal remains active.
Next public release: 1.27.0. These receipt notes were added after packaging;
runtime sources remain exact.

## 1.26.0: MP push and teleport volumes (2026-10-12)

MP now offers trigger_push and trigger_teleport directly, bringing the menu to
1,030 choices. Both spawn with a linked destination marker and derived oriented
sensor; the body/marker share one Undo group. Bodies retain existing tool,
Physgun, physics and context-outline behavior. Failed destination placement
removes the body and sensor. Primary links accept destination markers only.

Push uses the native trigger touch and shared jump-pad functions. Context offers
50-4000 speed, linear/actor-relative/arc direction, linear wait 0-60000 ms or -1
for one push, native same-frame MULTIPLE, Armed and Enabled. Re-arm a spent
single push deliberately. Arc mode requires a valid elevated linked destination;
unlinked direction follows body angles. Per-controller cooldown, current actor
consent, marker ownership and frozen/held/boarding states are rechecked. Vehicle
push preserves horizontal momentum through native driving without changing seats.

Teleport uses native touch/TeleportPlayer with an owned destination adapter.
Context offers ordinary or spectator-only entry, Armed and Enabled. Spectator
teleports avoid native killbox/events and refuse follow-camera mode. Players,
NPCs and vehicles use bounded, collision-checked arrival; every permitted driver,
passenger and droid seat is resolved before moving an occupied vehicle. Blocked
arrival leaves the complete assembly in place. Passive touch avoids repeated HUD
messages. Authored-map native push/teleport paths retain their previous behavior.

Copies and world restoration retain push speed, mode, remaining cooldown, spent
state, spectator filtering and remapped destination links. MP feature 38 writes
JRW16 (280-byte nodes, adding pushSpeed) and reads JRW1-15; prior records default
that new field to zero. Protocol 1050 and APIs 5/6/11 remain unchanged. SP runtime
sources, 1,175 cards, V43 saves, asset packs and approved exclusions are unchanged.

Focused fixtures execute native push/jump-pad/teleport callbacks, commands,
context input/echo, actor and four-family vehicle routes, obstructed seat arrival,
permissions, cooldown/rearming/Undo, save/copy, legacy JRW14/15 and rollback.
Collision/seat-bolt/network/animation services are imported; this is not rendered
or human remote-client acceptance. These movable sensors are server-authoritative;
authored brush-trigger client prediction is not reused for oriented owned volumes.
Live latency/feel and broader prediction parity remain part of MP acceptance.
The native non-client missile push path remains authored-map behavior; dynamic
Sandbox push volumes currently act on clients/NPCs/vehicles, not Box3D props.

The normal build/install pipeline records separate complete-suite and installed
hash evidence. Terrain's owner disposition, three legacy MP pickup adapters,
broader entity/feature parity and gameplay/save/rendered/network acceptance remain
open. No new exclusions were made. The full overhaul goal remains active.

## Latest installed release: 1.25.0 (2026-10-12)

Build `20261012-022053-5a3187a7` installed at 02:23 EDT through scripts/build-jar.ps1.
All 131 automated suites and retail resource/BSP/droid/YT-1300 checks passed.
Trigger checks cover native Repeat/Once/Hurt touch/use, oriented volume contact,
filters/consent, cooldown Undo, actor lifetime, copies/world restoration and
helper cleanup. Context input/echo and 12 oriented outline edges passed.
No graphical game or human remote-client session was launched.

Independent readback verified 68 installed files, 15 replacement backups
and 1982 archived sources. All five engine/launcher executables report
Windows version 1.25.0.0. ZIP SHA-256:
`501A17C64DC3D2AB96712D1790A0A38BEE6BD9A5FF229FD3E65F0EBDC20E29E9`.
Receipt: `build/deploy/20261012-022329-099cbb91/receipt.json`.
Qualification: `build/mp-triggers-1.25.0/qualification.json`.

MP now has 1,028 choices, feature 37, JRW15 with JRW1-14 reads, protocol 1050
and APIs 5/6/11. SP runtime sources and asset packs are unchanged. Settings,
saves and other mods were preserved. Terrain's owner disposition, three legacy
MP pickup adapters, broader MP parity and gameplay/save/rendered/network
acceptance remain open. The goal stays active. Next public release: 1.26.0.
These receipt notes were added after packaging; runtime sources remain exact.

## 1.25.0: portable MP trigger volumes (2026-10-12)

MP adds trigger_multiple, trigger_once and trigger_hurt, bringing the menu to
1,028 choices. Each uses a stock model body and an owned derived sensor volume;
normal Physgun rotation/movement, tool selection, frozen/simulated states and
assembly attachments remain available. Context selection draws the rotated
volume outline. Half-size is editable from 32 to 512 world units; narrow-phase
oriented contact rejects the extra corners of the engine's bounding box.

Repeat and Once offer player/NPC filters, Touch/Use/Fire/Use+Fire input, facing,
Armed and Enabled. Repeat has bounded cooldown/random timing; its optional
Secondary link fires one second after the volume clears, followed by cooldown.
Both spawn with a linked stock Print controller for immediate feedback. Once
retains its body after firing and can be deliberately rearmed. Hurt exposes
0-1000 damage, native-tick/one-second rates, active and Enabled controls. Native
Hurt bypasses protection; current tool permissions and player consent gate the
adapter. Damage zero performs no damage. Authored-map callbacks keep their
existing semantics outside owned Sandbox entities.

Disarming preserves pending timing and suppresses further outputs; Undo restores
the arm setting without resetting cooldowns. Copies and world restoration keep
cooldown/clear phases, spent Once state, Hurt rate timing and typed settings.
Sensors are derived, capacity-reserved and removed with their bodies; save/copy
records contain no sensor slots. Actor activators use connection/entity lifetime
epochs, preventing delayed outputs from impersonating a replacement slot.

MP feature 37 writes JRW15 (276-byte nodes; new trigger-state semantics) and reads
JRW1-14. Protocol 1050 and APIs 5/6/11 are unchanged. SP runtime sources, assets,
1,175 cards, V43 saves and the 28 approved exclusions are unchanged.

Focused fixtures execute the extracted native touch/use callbacks, oriented
contact math, actual commands/context, permissions, cooldown Undo, actor reuse,
duplicate/world codecs and helper cleanup. UI fixtures execute bounded input,
server echoes and capture 12 rotated outline edges. Broad-phase collision,
link/network/render services, damage and actor animation are imported boundaries;
these checks do not prove live graphical or remote-client behavior. The normal
release pipeline and installed-file readback record their evidence separately.

Terrain's owner decision, the three legacy MP pickup adapters, broader MP/SP
parity and outstanding gameplay/save/rendered/network acceptance remain open.
No new exclusions were made. The overhaul goal remains active.

## Latest installed release: 1.24.0 (2026-10-12)

Build `20261012-015937-6d251fa0` installed at 02:02 EDT through scripts/build-jar.ps1.
All 131 automated suites and retail resource/BSP/droid/YT-1300 checks passed.
Give/Score/Kill coverage includes native pickup dispatch and initialization,
properties/Undo, permission and connection reuse, consumed-item copies and world
restoration. Menu input/echo coverage passed. No graphical game or human remote-client session was launched.

Independent readback verified 68 installed files, 15 replacement backups
and 1977 archived sources. All five engine/launcher executables report
Windows version 1.24.0.0. ZIP SHA-256:
`D19A950715FC1177B3C9F8897B2575B0EB55D43668608C8D6CF2F4A7C11C5B9D`.
Receipt: `build/deploy/20261012-020211-0b020944/receipt.json`.
Qualification: `build/mp-actions-1.24.0/qualification.json`.

MP has 1,025 choices: three new action controllers and 18 native retail pickups.
Context settings support signed Score points, Kill modes and Enabled/Undo.
Give consumes linked pickups through native checks/events, preserving consumed
state through copies and JRW14 saves; JRW1-13 remain readable. Runtime pickups
now initialize from the requested position and publish their item registrations.
MP feature version 36, protocol 1050 and APIs 5/6/11 apply. SP runtime sources and
asset packs are unchanged; settings, saves and other mods were preserved.
Terrain's owner disposition, three legacy MP pickup adapters, broader MP parity
and gameplay/save/rendered/network acceptance remain open. The goal stays active.
Next public release: 1.25.0. These receipt notes were added after packaging;
runtime sources remain exact.

## 1.24.0: MP Give, Score, Kill and native pickups (2026-10-12)

MP adds target_give, target_score and target_kill, plus 18 native retail pickups:
instant medpacks/shields, portable shields, bacta variants, jetpack, dispensers,
E-Web, cloak, Force powerups, all-ammo and red/blue team flags. The catalog has
1,025 choices. All 21 added cards use existing retail models/icons verified by
build/mp-actions-1.24.0/catalog-audit.json. Native gametype/item/weapon restrictions
still apply; team flags require a supported flag mode.

Give spawns with a linked stock medpack and accepts Primary/Secondary pickup
links. It uses native Touch_Item checks and pickup events; full/refused pickups
remain available. Successful grants consume the linked item without respawning.
Consumed state persists through independent assembly copies and world restores;
cleanup still removes the hidden owned item. Score offers -1000..1000 points
through native AddScore, with overflow refusal. Kill offers Normal, Falling and
Electrical modes using MP damage and replicated electrify state. All actions
honor current player consent and server permissions; delayed actions reject a
reused client connection. Context settings support identity/revision-bound Undo.

An existing runtime pickup placement bug was also repaired: the delayed native
FinishSpawningItem read s.origin, which had remained zero. Runtime spawns now
supply their requested origin and publish registered items to clients. A fixture
executes the unchanged native finish callback and verifies the requested XY and
floor placement against an imported collision service.

Native Give/Score/Kill callbacks retain authored-map behavior outside the owned
adapter. Touch_Item and FinishSpawningItem were extracted unchanged for shared
production/fixture use. Fixtures execute commands, links, native pickup dispatch,
context input/echo, consumption, copies and the world codec. Item eligibility,
item-specific health/ammo effects, AddScore, G_Damage and collision/network/render
services remain imports; these checks do not prove human gameplay acceptance.

MP feature version 36 writes JRW14 (276-byte nodes, adding consumed-item state)
and reads JRW1-13. Protocol 1050 and APIs 5/6/11 are unchanged. SP runtime sources,
1,175 cards, its one unavailable terrain entry, 28 approved exclusions, V43 saves
and APIs 1024/1023 are unchanged. No asset-pack or setting changes are required.

Three newly audited legacy MP records remain open, not owner-excluded:
team_CTF_neutralflag, item_redcube and item_bluecube lack their native retail
models; neutral-flag/Harvester behavior also needs a concrete JA adapter. Their
new menu routes are deferred pending that work. Terrain's separate owner decision
is still pending. Full MP parity and gameplay/save/rendered/network acceptance
remain open; the goal remains active. Next public release: 1.25.0.

## Latest installed release: 1.23.0 (2026-10-12)

Build `20261012-014307-7a517d7a` installed at 01:45 EDT through scripts/build-jar.ps1.
All 131 automated suites and retail resource/BSP/droid/YT-1300 checks passed.
Native Timer/Random coverage includes scheduling, properties/Undo, permission
and connection reuse, copy/cleanup and world restoration. Menu input/echo
coverage passed. No graphical game or human remote-client session was launched.

Independent readback verified 68 installed files, 15 replacement backups
and 1970 archived sources. All five engine/launcher executables report
Windows version 1.23.0.0. ZIP SHA-256:
`7AC8BBA761AC8078A9AB2D10B3C92FC00B1FEFB6029AB0D3E3831620AEB36C7E`.
Receipt: `build/deploy/20261012-014544-1a2cdca6/receipt.json`.
Qualification: `build/mp-logic-1.23.0/qualification.json`.

MP has 1,004 choices, including Timer, Random Target and two position markers.
C -> Controller edits timing, running/armed state and labels. The new controllers
retain state through copies and JRW13 world saves; JRW1-12 remain readable.
MP feature version is 35; protocol 1050 and APIs 5/6/11 are unchanged. SP runtime
sources are unchanged, with 1,175 cards, one unavailable terrain entry, 28
approved exclusions, V43 saves and APIs 1024/1023. Asset packs, settings, saves
and other mods were preserved. Terrain remains open pending the owner's
exclusion-versus-new-implementation decision. The goal remains active; full MP
parity and outstanding gameplay/save/rendered/network acceptance remain open.
Next public release: 1.24.0. These receipt notes were added after packaging;
runtime sources remain exact.

## 1.23.0: MP Timer, Random Target and marker parity (2026-10-12)

The MP Entities menu now exposes func_timer, target_random, info_null and
info_notnull using the same stable catalog IDs and stock marker body as SP.
MP now has 1,004 choices. Timer and Random Target use native callbacks through
the existing server-owned link dispatcher. Menu spawning supplies the ordinary
feedback target in the same Undo group. Timer toggles repeating Primary output;
Random Target chooses one eligible Primary/Secondary output and supports Use
Once. Position-marker entries offer editable labels and transport destinations.

C -> Controller offers Timer interval/random timing, Running and Enabled, plus
Random Use Once, Armed and Enabled. Bounds enforce 50-60000 ms including jitter.
Changing timing preserves the already scheduled tick; disabling suppresses
output while retaining the schedule. Property Undo checks identity and revision.
Links recheck current permissions when fired; delayed use does not impersonate
a new occupant of a reused client slot. Duplication and world restoration retain
timer remaining time and the random controller's spent/armed state. Invalid
running/timer combinations and secondary Timer outputs are refused before paste.

Native Timer callbacks were extracted unchanged; the native Random callback has
an early owned-controller dispatch hook, leaving authored map behavior intact.
Production server tests cover native callbacks, scheduling, context/Undo,
permissions, connection reuse, copy, cleanup and world codec behavior. The menu
fixture covers actual click/input/echo handling. These deterministic fixtures do
not establish listening/rendered or human remote-client gameplay acceptance.

MP feature version is 35. World writes are JRW13 with JRW1-12 reads; node layout
is unchanged, but old readers must not accept the new controller semantics.
Protocol 1050 and MP APIs 5/6/11 are unchanged. SP remains at 1,175 cards, one
unavailable (terrain), 28 approved exclusions, game/renderer APIs 1024/1023 and
save V43. No asset packs, user settings or other mods are changed.

Terrain audit: both native SP_terrain callbacks immediately free the entity.
The collision registration is a stub, required landscape headers are absent,
and installed retail archives contain no heightmaps or RMG terrain definitions;
none of the 57 retail BSPs uses terrain. This is disabled legacy functionality,
not a missing model to repair. The owner has been asked whether to exclude/hide
it or retain it for a new implementation. It remains open and visible pending
that decision. Evidence: build/mp-logic-1.23.0/terrain-audit.json.

The overall goal remains active. Other MP catalog families, specialized context,
BSP/terrain disposition and gameplay/save/rendered/network acceptance remain
open. Next public release is 1.24.0. Installation evidence follows separately.

## Latest installed release: 1.22.0 (2026-10-12)

Build `20261012-012636-a13e25fe` installed at 01:29 EDT through scripts/build-jar.ps1.
All 131 automated suites passed. The native Vjun section fixture also passed
against the retail BSP with 60 owned children, 17 local entries and four sound
emitters, including hold/freeze/save/release and cleanup. Retail resource,
lighting, 57-map fog/grid, droid and YT-1300 checks passed. These checks do not
substitute for menu input, complete save relocation or rendered gameplay.

Independent readback verified 68 installed files, 15 replacement backups
and 1966 archived sources. All five engine/launcher executables report
Windows version 1.22.0.0. ZIP SHA-256:
`3D7E7AEE75800D75809AA452C5C34FB0C42C9CA5F57DDA7B1D0B527268DC6BDE`.
Receipt: `build/deploy/20261012-012912-669cd70e/receipt.json`.
Qualification: `build/bsp-section-1.22.0/qualification.json`.

SP Entities -> misc_bsp now places the fixed stock Vjun Fuel Processing section
in sufficiently large clear space. Activate enters a clear local destination;
Physgun moves its geometry and local points/sounds; cleanup removes the section.
SP keeps 1,175 cards with only terrain unavailable and 28 approved exclusions.
New saves use V43; older versions remain readable. SP APIs remain 1024/1023.
MP remains at 1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and JRW12.
The section route is SP-only pending its MP port. Broader BSP adapters, terrain,
remaining isolation/fog work and gameplay/save/rendered acceptance remain open.

Asset packs, settings, saves and other mods were preserved. No graphical game
was launched. The overall goal remains active; next public release is 1.23.0.
These receipt notes were added after packaging; runtime sources remain exact.

## 1.22.0: first spawnable stock BSP section (2026-10-12)

The SP Entities misc_bsp card now creates Vjun Fuel Processing (mp/duel10),
using its stock brush geometry and all 60 local entity records. Its 17 player
positions become section-local destinations rather than host respawn points;
Activate enters the first clear destination. Four native sound emitters and
other position markers retain isolated ownership and follow section transforms.
Use a sufficiently large clear area: placement checks the whole rotated map
bounds, centers those bounds on the aimed surface, and refuses overlap.

Physgun holding, freezing and release preserve the section controller frame.
Mouse/tool/context rotation uses the existing swept mover rotation path. Context
position and Undo check rotated bounds, the selection outline rotates, and
native frame motion also updates the child poses. Duplicate reconstructs this
fixed section through its catalog route with fresh ownership and local names;
Undo/cleanup removes its children. The fixed profile accepts stationary points
and local sound emitters only; scripts, movers and unsupported fields remain
refused rather than silently omitted. Other BSP profiles still need adapters.

The native controller fixture exercises actual parsing, admission, root setup,
speaker setup, rigid child motion, independent reconstruction, entry selection,
hold/freeze/save/release and cleanup. Retail qualification runs it on the exact
stock Vjun bytes. Collision/linking, actual teleport, complete save relocation,
menu input, rendered scene and gameplay remain distinct acceptance boundaries.
Existing regression fixtures cover generic clipboard, Undo and native pushing.
The stock speaker callback was extracted without behavioral changes.

SP still has 1,175 cards; terrain is the one unavailable card. Of the original
182 unavailable entries, 154 now have routes, 27 were owner-excluded and one
remains open. Entity-class coverage is 204 of 211 routed, six excluded and one
open. All 28 owner-approved exclusions remain unchanged. These are source-route
counts, not a claim that every interaction has been gameplay-accepted.

New SP saves use V43; earlier versions remain readable. SP game API 1024 and
renderer API 1023 are unchanged. MP still has 1,000 choices, feature 34, protocol
1050, APIs 5/6/11 and JRW12. This section route has not yet been ported to MP;
full SP/MP parity remains required. Broader BSP admission, arbitrary section
state/copy adapters, terrain, script/reference/camera/navigation isolation,
actor/view fog and full gameplay/save/rendered acceptance remain open. No
asset packs or settings were changed. The overall goal remains active. Next
public release after this package is 1.23.0; release evidence follows separately.

## Latest installed release: 1.21.0 (2026-10-12)

Build `20261012-005555-75fac242` installed at 00:58 EDT through scripts/build-jar.ps1.
All 130 automated suites passed, including the new portable-content gate and
native root admission/refusal coverage. The production gate was also audited
against all 57 retail BSPs; all remain refused pending further adapters or
ambiguous-key handling. Retail resources, fog/grid, lighting, droid and YT-1300
checks also passed. This is initial opt-in admission, not a spawn-menu route,
complete BSP decoder validation, rendered acceptance or gameplay validation.

Independent readback verified 68 installed files, 14 replacement backups
and 1961 archived sources. All five engine/launcher executables report
Windows version 1.21.0.0. ZIP SHA-256:
`45286092859B61B249237510610CD1A3DCBC62B12CD966CD9474939B6568230C`.
Receipt: `build/deploy/20261012-005833-67bc83a5/receipt.json`.
Qualification: `build/subbsp-admission-1.21.0/qualification.json`.
SP game API is 1024, renderer API 1023 and new saves V42; older saves remain
readable. Catalogs, assets, configuration/launcher scripts and MP contracts
are unchanged from 1.20.0. Settings, saves and other mods were preserved.
No graphical client was launched.

Portable BSP/terrain menu controllers, broader content admission, other class geometry, remaining
script/reference/camera/navigation isolation, copied-section reconstruction,
actor/view fog and full gameplay/save/rendered acceptance remain open. SP keeps
1,175 cards (two unavailable, 28 approved exclusions); MP keeps 1,000 choices,
feature 34, protocol 1050, APIs 5/6/11 and JRW12. Broader MP parity remains open.
The goal stays active. Next public release: 1.22.0. These receipt notes were
added after packaging; runtime sources remain exact.

## 1.21.0: initial portable BSP content admission (2026-10-12)

An opt-in native misc_bsp path now checks portable content before collision
registration, namespace ownership, linking or native child callbacks. It requires
an optional allocation transaction and forces an isolated namespace after
admission. Refusal frees the proposed root and reports the offending entity and
reason. Ordinary authored map instances retain their previous route.

The initial profile accepts six established brush mover classes (door, button,
platform, bobbing, rotating and pendulum) and simple position markers. It checks
RBSP header/lump ranges, model bounds, entity syntax and parser budgets, inline
brush references, finite parameters and required secondary MD3 resources. It
refuses scripts, unsupported classes/fields and ambiguous duplicate keys without
silently dropping them. The wrapper checks remaining collision model capacity
before registration. Cached resources must match the native collision source
checksum and model count; compatible reuse does not consume new model slots.
The engine's existing MD4 checksum implementation is reused by the SP game.

The new fixture exercises content decisions on synthetic BSP bytes. The native
root fixture exercises the real admission wrapper and root setup with mocked
file/registry/link services, including early refusal, file closure, capacity,
missing models, matching/changed cached resources and namespace creation. The
build pipeline also records admission results and source hashes for all 57
retail BSPs. All currently require further adapters or duplicate-key handling;
none is misreported as spawnable. Entity text has a 4 MiB budget and entity
counts follow the increased SP entity limit, not the old 2,048-map-entity limit.

This is initial content admission, not complete BSP structural validation or a
usable menu controller. Native collision/render lump decoding is unchanged.
Resource publication/rollback, placement, remaining class/script/reference/
camera/navigation adapters, copied-section reconstruction, actor/view fog and
full gameplay/save/rendered acceptance remain open. The two unavailable SP
cards, misc_bsp and terrain, remain unavailable. No new exclusions were made.

SP retains 1,175 cards and 28 approved exclusions; MP retains 1,000 choices.
SP game API 1024, renderer API 1023 and save V42 are unchanged. MP feature 34,
protocol 1050, APIs 5/6/11 and JRW12 are unchanged. Assets and settings are
unchanged. The goal stays active; next public release is 1.22.0. Full build and
installation evidence is recorded separately.

## Latest installed release: 1.20.0 (2026-10-12)

Build `20261012-003806-d4f335a6` installed at 00:41 EDT through scripts/build-jar.ps1.
All 129 automated suites passed, including native rotating/pendulum paths,
toggle/pause/resume, model-offset and held-interpolation math, native angle
setting, team rider movement/blocked rollback, Physgun freeze/save/release and
SP snapshot transport. Native trajectory/entity serializers are exercised;
collision services, complete save relocation, drawn rendering and gameplay
remain separate acceptance boundaries. Retail resource, 57-map fog/grid,
lighting, droid and YT-1300 checks also passed.

Independent readback verified 68 installed files, 16 replacement backups
and 1957 archived sources. All five engine/launcher executables report
Windows version 1.20.0.0. ZIP SHA-256:
`0CC4D9CA66CF84501E9390C2F325BA28203F3BB025117338D5092341E628DAC7`.
Receipt: `build/deploy/20261012-004106-1e8c4a86/receipt.json`.
Qualification: `build/angular-movers-1.20.0/qualification.json`.
SP game API is 1024, renderer API 1023 and new saves V42; older saves remain
readable. Catalogs, assets, configuration/launcher scripts and MP contracts
are unchanged from 1.19.0. Settings, saves and other mods were preserved.
No graphical client was launched.

Portable BSP/terrain controller admission, other class geometry, remaining
script/reference/camera/navigation isolation, copied-section reconstruction,
actor/view fog and full gameplay/save/rendered acceptance remain open. SP keeps
1,175 cards (two unavailable, 28 approved exclusions); MP keeps 1,000 choices,
feature 34, protocol 1050, APIs 5/6/11 and JRW12. Broader MP parity remains open.
The goal stays active. Next public release: 1.21.0. These receipt notes were
added after packaging; runtime sources remain exact.

## 1.20.0: rotating and pendulum section movers (2026-10-12)

Native sub-BSP rotating and pendulum brushes now carry world-axis angular paths
evaluated by the same SP server/client trajectory code. Their pitch/yaw/roll
motion follows the section frame, including tilted and vertical starting poses.
Rotating use stops at the current orientation and resumes without catching up
on paused time; pendulums retain their native period/phase. New spinner paths
start at the section's spawn time. Their secondary models preserve the authored
rest appearance as a rigid body-local offset and rotate with the brush.

The native team pusher uses the actual before/after rotation matrix for these
paths, keeping rider position and view yaw continuous through Euler singularities.
Blocked motion retains normal rollback and clock pausing. Physgun/native holds
keep a distinct interpolated angular state; release and external rotation rotate
the complete saved path. The native angle setter handles that frame change, and
the client interpolates held angular brushes using quaternions. Frozen native
save admission also now accepts valid sine/gravity trajectories that the old
range check rejected, while rejecting zero-duration sine records.

The new angular fixture executes native parsing/transforms, rotating/pendulum
spawn and use/touch callbacks, shared trajectory evaluation, the angle setter,
secondary-model math and native trajectory serialization. It checks all three
rotation axes, signed speeds, multiple tilted/vertical local and parent poses,
pause/resume and held interpolation. Existing fixtures now cover actual team
pushing/blocked rollback, native freeze/rotate/save/release and SP entity snapshot
transport for the added trajectory kinds. Collision services, actual sound or
damage effects, renderer draw calls and complete gameplay remain boundaries.

SP save V42 prevents older engines from misreading the new angular semantics;
previous saves remain readable. SP game API is 1024 for matching game/client
evaluation; renderer API remains 1023. Native entity layouts and save chunks are
unchanged. Workshop and vehicle dispatch includes V42. Ordinary map and portable
box mover spawn routes remain on their native trajectory types.

There are still 1,175 SP cards (misc_bsp and terrain unavailable, 28 approved
exclusions) and 1,000 MP choices. MP feature 34, protocol 1050, APIs 5/6/11 and
JRW12 are unchanged. Remaining section work includes usable controller/admission,
other class-specific geometry, direct named filters and script/camera/reference/
navigation isolation, copied-section reconstruction, actor/view fog, and full
gameplay/save/rendered acceptance. Terrain and broader MP parity remain open.
The goal stays active; next public release is 1.21.0. Full build/install evidence
is recorded separately.

## Latest installed release: 1.19.0 (2026-10-12)

Build `20261012-001814-b3bb254d` installed at 00:20 EDT through scripts/build-jar.ps1.
All 128 automated suites passed. The new native mover fixture covers transformed
doors/buttons/platforms/bobbing, secondary-model offsets, native state/trajectory
evaluation, oriented automatic triggers, nested ownership/cleanup and saved
geometry fields. Collision/linking, activation effects, rendering and full-save
pointer/string relocation remain service boundaries. Retail resource, 57-map
fog/grid, lighting, droid and YT-1300 checks also passed.

Independent readback verified 68 installed files, 14 replacement backups
and 1949 archived sources. All five engine/launcher executables report
Windows version 1.19.0.0. ZIP SHA-256:
`991D6D61993D04C2AD861AFA27D2C9AB56E33ADC61AD9516050F49F3AC3548DE`.
Receipt: `build/deploy/20261012-002044-b694ee46/receipt.json`.
Qualification: `build/subbsp-movers-1.19.0/qualification.json`.
SP game/renderer APIs remain 1023, saves remain V41. Catalogs, asset packs,
configuration/launcher scripts and MP contracts are unchanged from 1.18.0.
Settings, saves and other mods were preserved. No graphical client was launched.

Portable BSP/terrain controller admission, rotating/pendulum and other class
geometry, remaining script/reference/camera/navigation isolation, copied-section
reconstruction and actor/view fog remain open. Gameplay, full-save and rendered
acceptance remain open. SP retains 1,175 cards (two unavailable, 28 approved
exclusions); MP retains 1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and
JRW12. Broader MP parity remains open. The goal stays active. Next public release:
1.20.0. These receipt notes were added after packaging; runtime sources are exact.

## 1.19.0: native translating movers in rotated map sections (2026-10-12)

Sub-BSP doors and buttons now separate movement direction from brush orientation.
They calculate travel against the original local brush dimensions, keep the
section's geometry rotation, and preserve lip, speed, start-open and native
linear/eased state transitions. Transformed world yaw -1/-2 no longer aliases
the editor's up/down directions. Platforms move along section-local Z; bobbing
movers use the section's chosen axis, retain pause/resume phase and link at their
initial evaluated position. Secondary-model angle offsets for these four mover
families compose the section once, including vertical/gimbal orientations.

Automatic section door/platform triggers retain their original local volumes.
Door teams compute bounds from actual brush corners in the leader's frame,
expand along the local thin axis, and use oriented-box contact after the linked
broad-phase bounds. This rejects empty corners of tilted trigger AABBs while
allowing actor-edge contact. Trigger geometry uses existing saved entity fields;
native ownership handles delayed helpers, nested sections and cleanup. Ordinary
map entities and workshop box stand-ins retain their prior routes.

The new fixture runs production parsing/transforms, four mover spawn callbacks,
InitMover, state/trajectory evaluation, trigger creation/touch gates, ownership,
and entity-field serialization. It covers seven section orientations, up/down,
start-open, explicit/default platform heights, narrow platform triggers, door
teams, locks/inactivity, edge contact, nested cleanup and bobbing pause/resume.
Collision registration/linking, actual activation effects, model rendering and
full save pointer/string relocation remain service boundaries. It does not
constitute gameplay or visual acceptance.

SP game/renderer APIs remain 1023; saves remain V41. No catalog entries are added
in this release: 1,175 SP cards retain two unavailable (misc_bsp and terrain) and
28 approved exclusions; MP retains 1,000 choices, feature 34, protocol 1050,
APIs 5/6/11 and JRW12. Portable controller/admission, rotating/pendulum and other
class-specific geometry, remaining script/reference/camera/navigation isolation,
copied-section reconstruction, actor/view fog and complete gameplay/save testing
remain open. Broader MP parity remains open. The goal stays active; the next
public release is 1.20.0. Full build/install qualification is recorded separately.

## Latest installed release: 1.18.0 (2026-10-12)

Build `20261012-000259-5d3dd204` installed at 00:06 EDT through scripts/build-jar.ps1.
All 127 automated suites passed. BSP save checks execute native registration,
JBSP serialization, restored brush linking and the client model bridge with
reordered resources. They reject missing/changed maps, malformed records and
unregistered brush models, while preserving old-save and autosave behavior.
The native loader/cache regression uses real MD4 and synthetic map data to check
per-map checksums under interleaved loading and normal/low-memory cached reloads.
Lump decoding, renderer upload, linking and full gameplay remain fixture boundaries.
Retail resource, 57-map fog/grid, lighting, droid and YT-1300 checks also passed.

Independent readback verified 68 installed files, 16 replacement backups
and 1937 archived sources. All five engine/launcher executables report
Windows version 1.18.0.0. ZIP SHA-256:
`7E0A87788579AC0652A506DAD7445255CE2CE50B20164FE4C35A28627B0D2D03`.
Receipt: `build/deploy/20261012-000556-7cb63188/receipt.json`.
Qualification: `build/bsp-save-1.18.0/qualification.json`.
SP game API is 1023; renderer API remains 1023. New SP saves use V41 with the
JBSP resource registry; previous saves remain readable. Catalogs, assets,
configuration/launcher scripts and MP contracts are unchanged from 1.17.0.
Settings, saves and other mods were preserved. No graphical client was launched.

Portable BSP/terrain controller admission, remaining script/reference/camera
isolation, class-specific movers, copied-section reconstruction and actor/view
fog remain open. Full native save/gameplay and rendered acceptance remain open;
resource remapping does not prove the complete portable controller. SP retains
1,175 cards (two unavailable, 28 approved exclusions); MP retains 1,000 choices,
feature 34, protocol 1050, APIs 5/6/11 and JRW12. Broader MP parity remains open.
The overhaul goal stays active. Next public release: 1.19.0. These notes were
added after packaging; runtime sources remain exact.

## 1.18.0: persistent BSP collision/model identities (2026-10-12)

SP full saves now record each loaded BSP's canonical name, inline-model range
and source checksum before writing native entities. On restore, the registry
validates the main map and all record names/ranges, registers missing BSPs by
name, checks their model counts/checksums, and maps saved brush indices to the
current collision registry before linking entities. Existing resources may be
loaded in a different order or include unrelated BSPs. Roots and inline children
retain the right geometry; main-map and ordinary model indices stay unchanged.
Unlinked brushes are remapped without being linked. The client continues to
resolve these collision ranges by canonical BSP names, independent of renderer
load order. No native entity layout or authored model string changes.

Each loaded collision map now owns its source checksum. This also fixes cached
main/client reloads reporting the checksum of the most recently loaded sub-map.
The native loader/cache regression executes the real MD4 checksum and interleaved
loads with normal and low-memory caching; lump decoding and file services are
mocked. The registry fixture executes native registration/lookup, the production
JBSP writer/reader and restored-link helper, then the shared client model bridge.
It checks reordered loads, extra resources, root/child lookup, malformed and
truncated records, changed/missing maps, model capacity, old saves and autosaves.
Collision linking, model uploads and complete gameplay remain service boundaries.

SP save V41 adds JBSP ahead of native entities. Autosaves carry an empty registry
and leave the fresh map alone; earlier saves still read without the new chunk.
SP game API is 1023 for the two registry imports; renderer API remains 1023.
Vehicle and workshop version dispatch includes V41. The actual workshop writer
and native vehicle registry checks pass with the selected current version.

This completes BSP resource identity/registration during native full-save
restore, not the portable menu controller. misc_bsp and terrain remain unavailable:
controller/admission, remaining script/reference/camera isolation, class-specific
movers, copied-section reconstruction and actor/view fog still need work. There
are still 1,175 SP cards (two unavailable, 28 approved exclusions) and 1,000 MP
choices. MP feature 34, protocol 1050, APIs 5/6/11 and JRW12 are unchanged, as are
asset packs and menu content. Rendered/full-game save acceptance and broader
MP parity remain open. Full installation qualification is recorded separately.
The overhaul goal remains active; next public release is 1.19.0.

## Latest installed release: 1.17.0 (2026-10-11)

Build `20261011-234648-6a4386b2` installed at 23:49 EDT through scripts/build-jar.ps1.
All 125 automated suites passed. The new native transaction fixture covers
required/optional exhaustion, reserve preservation, successful and failed menu
spawns, nesting, churn, slot reuse, cleanup events, AI scratch state and reset.
The existing entity-setup fixture also passed its new class-alias/parser unwind
check. Retail resource, 57-map fog/grid, lighting, droid and YT-1300 checks passed.

Independent readback verified 68 installed files, 14 replacement backups
and 1931 archived sources. All five engine/launcher executables report
Windows version 1.17.0.0. ZIP SHA-256:
`A1D1922009026B99A135B39D9A9AB6F3CEA66B7D8C985E45901204938F580C92`.
Receipt: `build/deploy/20261011-234921-29810e74/receipt.json`.
Qualification: `build/spawn-transactions-1.17.0/qualification.json`.
Catalogs, assets, configuration/launcher scripts, save V40 and APIs are unchanged
from 1.16.0. Settings, saves and other mods were preserved. No graphical client
was launched; fixtures and installed hashes are not gameplay acceptance.

Synchronous SP menu spawns now roll back their allocations on failure. This does
not undo arbitrary mission/world/resource side effects or cover delayed child
spawns. Portable BSP/terrain admission, remaining namespace/global-script
isolation, class-specific movers, dynamic registration/copy/save integration and
actor/view fog remain open. SP retains 1,175 cards (two unavailable, 28 approved
exclusions); MP retains 1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and
JRW12. Rendered/gameplay acceptance and multiplayer parity remain open.
The overhaul goal stays active. Next public release: 1.18.0. These notes were
added after packaging; runtime sources remain exact.

## 1.17.0: optional menu-spawn allocation rollback (2026-10-11)

SP menu spawns and the duplicator's per-object spawn route now use an allocation
transaction. Required native G_Spawn children share the optional region during
that synchronous action, preserving critical reserve slots. Exhaustion unwinds
only through a typed allocation failure; other engine errors retain their normal
behavior. An ignored optional allocation failure also prevents committing a
partial setup. Failure frees newly allocated entities through native cleanup in
reverse order, checking lifetime tokens so freed/reused slots are never mistaken
for the original allocation. Actor recovery is suspended during rollback.

Nested transactions retain ancestor records, including committed children; a
failed child can be replaced without confusing its reused slots. The bounded
allocation budget counts churn as well as live entities. Cleanup events use the
normal allocation policy and are not adopted by an enclosing menu transaction.
Level init/shutdown clears abandoned transaction pointers. Temporary native class
aliases and spawn-parser state unwind, and the menu preserves active/saved NPC AI
scratch state so failure cannot leave it pointing to a newly freed actor.

The new native fixture executes the production allocator, lifetime tracker,
transaction, door-trigger callback and menu wrapper. It covers required and
optional exhaustion, reserve access outside the transaction, commit/failure,
nesting, allocation churn, reused slots, cleanup events, NPC state and level
reset. The existing entity-setup fixture also checks exception unwinding of the
class alias and parser/helper context. Native free services, placement, AI and
full menu/gameplay execution remain fixture boundaries.

This is allocation rollback, not a transaction for arbitrary mission scripts,
world/configstring edits or resource registration. Delayed children run outside
this synchronous boundary. misc_bsp and terrain remain unavailable until their
remaining controller/admission, mover, dynamic registration, copy/save and
isolation work is complete. Actor/view fog, rendered acceptance and MP parity
also remain open. SP retains 1,175 cards (two unavailable and 28 approved
exclusions); MP retains 1,000 choices, feature 34, protocol 1050, APIs 5/6/11
and JRW12. Assets, settings, save V40 and SP APIs are unchanged. Full build and
installation qualification is recorded separately. Goal active; next 1.18.0.

## Latest installed release: 1.16.0 (2026-10-11)

Build `20261011-233240-ed77168d` installed at 23:35 EDT through scripts/build-jar.ps1.
All 124 automated suites passed. New native search/output/team and ICARUS index
checks cover duplicate names across isolated copies, nested inheritance, player
access, global class queries, rename/removal, saved namespace flags and invalidated
callback contexts. The actual workshop writer preserves isolated roots in V40;
vehicle registries and earlier ownership/spawn fixtures also pass. Existing retail
resource, 57-map fog/grid, lighting, droid and YT-1300 checks passed.

Independent readback verified 68 installed files, 14 replacement backups
and 1926 archived sources. All five engine/launcher executables report
Windows version 1.16.0.0. ZIP SHA-256:
`2CEC8106AFCD0D6495E393CDE374539AEC17A7F2C24CDB91CBD3D681EB7DC41E`.
Receipt: `build/deploy/20261011-233505-507bce12/receipt.json`.
Qualification: `build/subbsp-names-1.16.0/qualification.json`.
Catalogs, assets, configuration/launcher scripts and APIs are unchanged from
1.15.0. SP writes save V40; V39 ownership reads with its original global-name
behavior, and earlier saves remain readable. Settings, saves and other mods
were preserved. No graphical client was launched.

Portable-map isolation remains incomplete beyond core named references: direct
named filters, asynchronous cameras, reference/navigation registries, script
variables/signals and mission-wide commands still need scoped behavior. Optional
allocation transactions, mover behavior, dynamic registration/copy/save integration
and actor/view fog also remain open. misc_bsp and terrain remain unavailable.
SP retains 1,175 cards (two unavailable, 28 approved exclusions); MP retains
1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and JRW12. Full script
interpretation, rendered/gameplay acceptance and multiplayer parity remain open.
The overhaul goal stays active. Next public release: 1.17.0. These notes
were added after packaging; runtime sources remain exact.

## 1.16.0: scoped native target and ICARUS entity names (2026-10-11)

SP map-section roots accept an opt-in _jar_namespace spawn key. Existing authored
sections default to the original global namespace. Nested authored sections inside
an isolated copy share its namespace; a separately isolated nested root gets its
own. Names and script files are unchanged. This is controller support, not a new
menu route: misc_bsp and terrain still need their remaining integration.

Native G_Find filters name/link fields by the active section while leaving class,
model and physical/AI queries global. The player remains an intentional named
target across sections. Named main-map entities and other copies are excluded
from isolated searches. G_UseTargets2 selects its source's context even when called
outside a think callback. Native team chains cannot merge different namespaces.
Deleting an isolated root invalidates still-active name contexts; they cannot
fall back to main-map targets during callback unwinding.

ICARUS keeps all same-name entity associations and resolves the namespace at
lookup time, retaining last-registration precedence within each namespace.
Removal affects only that entity's registrations; reassociation drops obsolete
names. Lookup rejects freed entities and invalid ICARUS bindings. Because names
are indexed independently of namespace, ownership restored after native ICARUS
linkage needs no name rewrite. Non-scripted frees keep their existing fast path.

SP save V40 adds the isolated-root bit in the existing JBSO record. V39 records
remain readable with global-name behavior; older engines reject V40 before loading.
The actual writer round-trips isolated roots. Native target search/use/team and
script-index fixtures cover collisions across copies, nested inheritance, player
access, class queries, renaming/removal, post-load lookup and stale contexts.
Script interpretation and complete gameplay are not claimed by these fixtures.

This implements core named-reference isolation. Direct named contact/damage
filters, asynchronous camera contexts, script variables/signals and commands with
global mission effects still need separate treatment before arbitrary portable
map scripts are admitted. Optional allocation transactions, class-specific mover
geometry/behavior, dynamic registration/copy/save integration and actor/view fog
also remain open. The catalog is unchanged: SP 1,175 cards, two unavailable and
28 approved exclusions; MP 1,000 choices, feature 34, protocol 1050, APIs 5/6/11
and JRW12. Assets and SP game/renderer APIs are unchanged. Full installation
qualification is recorded separately. Goal active; next release 1.17.0.

## Latest installed release: 1.15.0 (2026-10-11)

Build `20261011-231902-2084bfd0` installed at 23:21 EDT through scripts/build-jar.ps1.
All 123 automated suites passed. The ownership fixture executes the native
allocator and delayed door-trigger creator, checking nested groups, removal,
reused slots, unrelated callbacks, reentrant teardown, nesting limits and saved
ownership. The actual workshop writer round-trips nonempty ownership in full
saves and omits it from auto-saves. Native vehicle registries also pass with V39.
The first full run caught the missing V39 vehicle-registry branch; it was repaired
before this qualified build. Existing retail resource, 57-map fog/grid, lighting,
droid and YT-1300 checks passed. No graphical game was launched.

Independent readback verified 68 installed files, 14 replacement backups
and 1921 archived sources. All five engine/launcher executables report
Windows version 1.15.0.0. ZIP SHA-256:
`5C72F2F63D2AB8853B85C6DB56F999B3B6F63931ECC739F94B58E81755DD3C7E`.
Receipt: `build/deploy/20261011-232127-6bbe740d/receipt.json`.
Qualification: `build/subbsp-ownership-1.15.0/qualification.json`.
Catalogs, assets, configuration/launcher scripts and APIs are unchanged from
1.14.0. SP writes save V39 with the JBSO ownership chunk; older saves remain
readable. Settings, saves and unrelated mods were preserved.

Native section ownership, descendant cleanup and ownership persistence are
installed. Complete portable misc_bsp still needs an optional spawn transaction,
consistent target/script namespaces, class-specific mover behavior, dynamic
registration/copy/save integration and actor/view fog classification. Terrain
also remains unavailable. SP retains 1,175 cards (two unavailable, 28 approved
exclusions); MP retains 1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and
JRW12. Native free services, ICARUS execution, full gameplay and rendered scenes
remain verification boundaries. Multiplayer parity remains open.
The overhaul goal stays active. Next public release: 1.16.0. These notes
were added after packaging; runtime sources remain exact.

## 1.15.0: native map-section ownership and saved cleanup (2026-10-11)

SP map sections now track native child allocations independently of the small
Sandbox assembly registry. Each allocated entity gets a lifetime token; ownership
never depends on replacing authored target, script, model or NPC names. Nested
sections retain their own children and their membership in the outer section.
Native entity frames (including missile/item/mover paths),
think/reached/blocked/use/touch/pain/die callbacks, scheduled ICARUS updates
and behavior activation carry the section context. A nested callback on an
unowned entity clears that context, preventing unrelated allocations from being
adopted. The actual delayed-door trigger allocation is covered by the fixture.

Freeing a section removes its currently owned descendants. Freed/reused slots and
other sections are preserved; reentrant deletion cannot free the root twice.
Cascade teardown suspends actor stand-in recovery. Ordinary member removal detaches
that member. New-map initialization and shutdown clear transient ownership.

New SP save V39 writes the JBSO ownership chunk through the actual workshop writer.
Full-load teardown clears old ownership before native entities are overwritten;
validated ownership publishes after entity restoration. Duplicate slots, invalid
roles/owners, cycles and excessive nesting reject. Auto-saves omit world ownership
and retain freshly spawned authored groups. Older save versions remain readable,
with their previous lack of section ownership; older engines reject V39 early.
This persists ownership only: dynamic map registration and complete portable-map
restoration still need the controller integration.

Native allocator, nested spawn, delayed door helper, section cleanup, reuse,
reentrancy, depth limits, malformed records and full/auto-save writer tests pass.
Native free-service internals, collision linking, ICARUS execution and rendered
scenes remain fixture boundaries. No graphical game or gameplay acceptance is
claimed. Optional allocation retains the existing critical reserve; required
native allocation still uses ERR_DROP on exhaustion. Portable admission needs an
allocation transaction before those required paths can serve menu spawns safely.

misc_bsp and terrain remain unavailable. Target/script namespaces, mover geometry,
travel/triggers/trajectories, portable copy/save/registration and actor/view fog
remain open. SP keeps 1,175 cards and 28 approved exclusions; MP keeps 1,000 choices,
feature 34, protocol 1050, APIs 5/6/11 and JRW12. SP game/renderer APIs and assets
are unchanged. Full release/install qualification is recorded separately.
The overhaul goal remains active. Next public release: 1.16.0.

## Latest installed release: 1.14.0 (2026-10-11)

Build `20261011-230036-7ede4aa5` installed at 23:03 EDT through scripts/build-jar.ps1.
All 122 automated suites passed, including the new native sub-BSP spawn fixture
and the existing workshop entity setup transactions. Nested maps restore the
parent parser and active collision BSP; child origin/orientation spawn inputs
compose through all three axes before native field parsing and class callbacks.
The fixture covers sibling/nested placement, scope restoration, invalid input,
capacity rejection, depth/instance limits and map resets. Collision registration,
entity-class callbacks and ICARUS remain fixture boundaries. Existing retail
resource, 57-map fog/grid, lighting, droid and YT-1300 checks also passed.

Independent readback verified 68 installed files, 14 replacement backups
and 1916 archived sources. All five engine/launcher executables report
Windows version 1.14.0.0. ZIP SHA-256:
`4B7A08C3FEFAED1E1E306B61504369309EE555C1EBFE56202C34E3E2FF323896`.
Receipt: `build/deploy/20261011-230307-ca7a06ac/receipt.json`.
Qualification: `build/subbsp-spawn-1.14.0/qualification.json`.
Catalogs, asset packs, configuration/launcher scripts, APIs and save formats
are unchanged from 1.13.0. Settings, saves and unrelated mods were preserved.

Portable misc_bsp still needs owned allocation/cleanup, consistent target/script
namespaces, class-specific mover geometry/distances/triggers/trajectories,
copy/save/restore and actor/view fog classification before menu admission.
Target/script strings remain unchanged. Terrain also remains unavailable.
SP retains 1,175 cards (two unavailable, 28 approved exclusions); MP retains
1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and JRW12. SP save V38
and renderer API 1023 are unchanged. No graphical client was launched;
rendered/gameplay acceptance and multiplayer parity remain open.
The overhaul goal stays active. Next public release: 1.15.0. These notes
were added after packaging; runtime sources remain exact.

## 1.14.0: native sub-BSP spawn isolation and transforms (2026-10-11)

Native SP sub-map spawning now restores the caller's complete spawn-key buffer,
key pointers/counts, spawning flag and NPC precache flag. Workshop setup uses the
same scope. Nested misc_bsp returns to its parent collision BSP instead of forcing
the main map. Temporary target-prefix pointers, offset, yaw and nesting depth are
restored; new-map initialization clears transient state even after ERR_DROP.
Instance-ID exhaustion and recursive nesting are bounded before registration.

Child origin, angles and explicit modelAngles are transformed before native fields
and spawn callbacks read them. Positions rotate as well as translate; orientations
compose through pitch/yaw/roll rather than adding Euler components. Directional
angle -1/-2 conventions are interpreted only for the native classes using them.
Transformed key records are validated and fit-checked before replacing the parser
buffer. The root retains all orientation axes and its existing entity flags.

A native-parser fixture executes the production root setup, nested entity loop,
context restoration and transform code. It checks nested BSP switches, sibling
placement, composed orientations, scoped keys/pointers, early exits, invalid input,
capacity, limits and reset. Collision registration, entity-class callbacks and
ICARUS remain fixture boundaries. No graphical game or gameplay acceptance is claimed.

This is spawn-path groundwork. Owned child allocation/cleanup, target and script
namespaces, class-specific mover geometry/distances/triggers/trajectories,
copy/save/restore and actor/view fog remain open. Target/script strings are
preserved until that adapter exists. misc_bsp and terrain remain unavailable;
SP retains 1,175 cards and 28 approved exclusions, MP 1,000 choices. Protocol,
APIs, save formats, assets and controls are unchanged. Full release/install
qualification is recorded separately. The goal stays active. Next: 1.15.0.

## Latest installed release: 1.13.0 (2026-10-11)

Build `20261011-224247-4f1a3718` installed at 22:45 EDT through scripts/build-jar.ps1.
All 121 automated suites passed. Three new fixtures exercise production light-grid
loading and CPU sampling with actual SP/MP/Rend2 structures and native parsers.
They cover transformed instances, edge/single-cell sampling, styles, wall fallback,
explicit lighting origins, empty grids, malformed records and HDR paths. All 57
stock BSP grids also pass the loader/sampler in all three modes; existing fog,
lightmap/resource, droid and YT-1300 native checks pass. No graphical game was
launched. Fixture color shifting, allocation and debug drawing remain boundaries;
rendered/gameplay acceptance remains open.

Independent readback verified 68 installed files, 17 replacement backups
and 1910 archived sources. All five engine/launcher executables report
Windows version 1.13.0.0. ZIP SHA-256:
`330E528010AB110818395E9C15D1554D92094F76218C6A36CDBFD1C9C820C8A4`.
Receipt: `build/deploy/20261011-224518-f65bae32/receipt.json`.
Qualification: `build/bsp-grid-1.13.0/qualification.json`.
Catalogs, asset packs, configuration/launcher scripts, APIs and save formats
are unchanged from 1.12.0. Settings, saves and unrelated mods were preserved.

Scene-instance grid lighting is installed in classic SP/MP and Rend2. Portable
misc_bsp remains unavailable pending native child ownership, target namespaces,
transforms and copy/save/restore, plus actor/view fog classification. Point queries
before scene assembly still use the main grid. Terrain remains unavailable.
SP retains 1,175 cards (two unavailable, 28 approved exclusions); MP retains
1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and JRW12. SP save V38
and renderer API 1023 are unchanged. The overhaul goal stays active.
Next public release: 1.14.0. These notes were added after packaging; runtime
sources remain exact.

## 1.13.0: map-section light-grid loading and entity lighting (2026-10-11)

Classic SP/MP and Rend2 now share light-grid loading, scene ownership and CPU
sampling. Classic renderers load each section's grid and parse its grid spacing
without changing the main environment. Rend2 HDR lookup preserves the complete
map directory, including nested paths. Grid dimensions, sample/array counts,
compressed references, style indices and HDR sizes/values are checked before
use. Unlit maps remain valid; oversized GPU grid dimensions retain CPU lighting.

Each rendered scene collects whole-map instances once, copying their transforms.
Entity lighting selects the smallest containing section, converts the sample
point to its local grid and transforms the light direction back. Translation,
rotation, reflection and orthogonal scale are supported; degenerate/sheared
transforms are ignored. Doors do not become independent map lighting volumes.
Missing/wall-only grids fall back to main-map lighting. Scene/model reset clears
the registry. Point queries made before a scene is assembled still use main-map
lighting; no persistence of previous-frame entity transforms is claimed.

Sampling clamps each axis before choosing corners, fixing row/slice wrapping at
grid edges and supporting single-cell dimensions. Native light styles, HDR,
explicit lighting origins, fullbright and goggles remain supported. SP's debug
grid display also transforms sample locations and directions with the section.

Three fixtures execute production code with actual renderer structures and
native parsers. They check edges, weights, wall samples, styles, separate/overlapping
instances, transform changes, invalid data, empty grids and HDR paths. The retail
check now loads and samples all 57 stock BSP grids in all three modes, alongside
the existing fog checks. Color shifting, allocation and debug drawing are fixture
boundaries. No rendered scene or human gameplay acceptance is claimed.

Portable misc_bsp still needs actor/view fog classification, native child ownership,
target namespaces/transforms and copy/save/restore. Terrain remains unavailable.
Catalogs, asset packs, controls, APIs and saves are unchanged: 1,175 SP cards,
two unavailable and 28 approved exclusions; 1,000 MP choices, feature 34,
protocol 1050, APIs 5/6/11, JRW12, SP save V38 and renderer API 1023.
Full release/install qualification is recorded separately. The goal stays active.
Next public release: 1.14.0.

## Latest installed release: 1.12.0 (2026-10-11)

Build `20261011-222033-bcec699d` installed at 22:26 EDT from the exact scripts/build-jar.ps1 ZIP.
Deployment resumed with scripts/deploy-jar-release.ps1 after the owner closed JA:R.
All 118 automated suites passed. New Rend2 fixtures execute fog loading, owner
lookup, instance transforms, local/global grid matrices and per-entity GPU-block
generation. All 57 stock BSPs pass fog loading and surface-reference checks in
classic SP, classic MP and Rend2. A hidden WGL test compiled/linked 24 real fog,
generic and sprite shader variants on the installed NVIDIA driver and verified
their std140 layouts. No scene was drawn and no graphical game was launched.
Retail lightmap/resource checks, droid spawn/restore and YT-1300 rig/persistence
checks also passed. Gameplay/visual acceptance remains open.

Independent readback verified 68 installed files, 15 replacement backups
and 1906 archived sources. All five engine/launcher executables report
Windows version 1.12.0.0. ZIP SHA-256:
`251A1905A151EDA9BA994AC7828473E8C27ED91B2F3F2130B142DF30391BC19F`.
Receipt: `build/deploy/20261011-222621-08ff6dcf/receipt.json`.
Qualification: `build/rend2-fog-1.12.0/qualification.json`.
Catalogs, asset packs, configuration/launcher scripts, APIs and save formats
are unchanged from 1.11.0. Settings, saves and unrelated mods were preserved.

Rend2 per-instance fog constants and volumetric grid bindings are installed,
including separate inherited/main and local grids, transformed sprite placement
and live fog inheritance. Portable misc_bsp remains unavailable pending CPU
light-grid sampling/source validation, actor/view classification, native child
ownership/target namespaces/transforms and copy/save/restore. Terrain remains
unavailable. SP retains 1,175 cards (two unavailable, 28 approved exclusions);
MP retains 1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and JRW12.
SP save V38 and renderer API 1023 are unchanged. The overhaul goal stays active.
Next public release: 1.13.0. These notes were added after packaging; runtime
sources remain exact.

## 1.12.0: Rend2 sub-map fog and volumetric grid binding (2026-10-11)

Rend2 now uploads fog constants per brush instance and binds them in opaque,
translucent and surface-sprite passes. Local fog planes follow translation,
rotation and orthogonal scale; instances with equal local indices keep separate
tables. A sub-map may supply its own global fog or inherit the main map's live
fog. Fogged batches cannot merge across entities. The special night-vision slot
uses the scene's main fog block and white volume texture.

Volumetric sampling uses world-to-grid matrices in the fog block, with distinct
local and global grid textures. Inherited fog samples the main grid, while a
section's fog samples its own transformed grid. Global fallback segments use the
global density and intersection point. Reverse fog-plane draws retain their own
samplers. Each BSP retains its volumetric density scale; child worldspawn parsing
does not change the main environment. Bounded entity-lump copying and balanced
parse sessions replace the previous unterminated copy/early-return paths.

The fog loader checks references and GPU capacity before use, reserves the
night-vision slot, accepts child global fog and preserves shared shader settings
when a fog shader needs fallback density. Surface sprites now use the owning
entity matrix for placement, camera facing, fade distance and fog position.

Native fixtures execute the loader, fog lookup, transformed grid matrices and
GPU-block generation, checking separate instances, inheritance, goggles, missing
grids, bad references and limits. Fog loading and surface references pass across
all 57 stock BSPs in classic SP/MP and Rend2 modes. A dedicated hidden WGL test
compiles and links 24 real fog/generic/sprite shader variants through the installed
driver and checks their std140 offsets against C++. It does not draw a scene or
launch the game. Shader parameters and GPU upload are boundaries of CPU fixtures;
shader compilation is real, while rendered/gameplay acceptance remains open.

This completes Rend2 brush-surface fog and volumetric grid binding. CPU light-grid
sampling, actor/view classification inside instances, source light-grid validation,
native child ownership/target namespaces/transforms and copy/save/restore remain
open. Portable misc_bsp and terrain remain unavailable. Catalogs, asset packs,
controls, APIs and save formats are unchanged: 1,175 SP cards (two unavailable,
28 approved exclusions), 1,000 MP choices, feature 34, protocol 1050, APIs 5/6/11,
JRW12, SP save V38 and renderer API 1023. Full build/install qualification will be
recorded after deployment. The goal stays active. Next public release: 1.13.0.

## Latest installed release: 1.11.0 (2026-10-11)

Build `20261011-220414-f53f2279` installed at 22:06 EDT through scripts/build-jar.ps1.
All 116 automated suites passed, including production classic SP/MP fog loading,
instance ownership/transforms, texture coordinates, guarded allocations and
malformed input. Both fog loader modes checked all 57 installed stock maps and
every surface fog reference, including inherited and per-map global fog.
Retail lightmap/resource checks, seven-card droid spawn/restore and YT-1300
rig/persistence checks also passed. Shader parameters are mocked in the fog
fixtures. No graphical client was launched; drawing and gameplay acceptance
remain unverified.

Independent readback verified 68 installed files, 16 replacement backups
and 1902 archived sources. All five engine/launcher executables report
Windows version 1.11.0.0. ZIP SHA-256:
`920AC745942D272761EDA469480D52252536A2B76D7AA984806DABE877B03916`.
Receipt: `build/deploy/20261011-220633-a07499a2/receipt.json`.
Qualification: `build/bsp-fog-1.11.0/qualification.json`.
Catalogs, asset packs, configuration/launcher scripts, APIs and save formats
are unchanged from 1.10.0. Settings, saves and unrelated mods were preserved.

Classic brush-surface fog ownership is installed, including the MP allocation
fix and transformed per-instance fog planes. Portable misc_bsp remains unavailable
pending Rend2 fog/volumetric resources, light-grid selection, actor/view fog
classification, native children/target namespaces/transforms and copy/save/restore.
Terrain remains unavailable. SP retains 1,175 cards (two unavailable, 28 approved
exclusions); MP retains 1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and
JRW12. SP save V38 and renderer API 1023 are unchanged. The goal remains active.
Next public release: 1.12.0. These receipt notes were added after packaging;
all runtime sources remain exact.

## 1.11.0: classic renderer sub-map fog (2026-10-11)

SP and MP classic renderers share the production fog loader, ownership lookup
and clipped-fog texture-coordinate calculation. Brush models retain their BSP
owner; fog shaders, hardware-fog decisions, color generation and surface sprites
read that owner's fog instead of indexing the main map's table. Entity changes
flush fogged batches even for mergeable shaders, keeping separate instances
independent. Local fog planes and bounds follow translation, rotation and
orthogonal scale, without modifying source BSP data. Inherited global fog reads
the main map's current values, preserving transitions. SP night-vision fog
continues to use the main special slot.

The loader reserves inherited and special slots explicitly, fixing MP's
one-past-allocation write. SP now accepts a sub-map's own global fog. Bounded,
global and absent fog retain distinct handling. BSP fog/surface/brush/side/plane
references and shader termination are checked before dereferencing. The existing
five-bit sort limit is enforced before aliasing indices; its capacity has not
changed. All 57 installed stock BSPs fit (the largest has eight fog records).

Two fixtures execute the production SP/MP loaders, owner resolution, instance
transforms and texture-coordinate calculations. They cover multiple instances,
equal local indices, scale/rotation, live inheritance, goggles, guarded
allocations, malformed references and limits. The release pipeline additionally
loads fog data and checks every surface fog reference in all 57 stock maps,
with mp/ffa2 as the main map and the other 56 as children. Shader parameters
are mocked in these fixtures; GPU rendering and gameplay remain untested.

This completes classic brush-surface fog ownership, not portable misc_bsp.
Rend2 fog/volumetric resources, light-grid selection, actor/view fog inside
instances, native child ownership/target namespaces, transforms and copy/save/
restore remain open. Terrain remains unavailable. Catalogs stay at 1,175 SP
cards (two unavailable, 28 approved exclusions) and 1,000 MP choices. MP feature
34, protocol 1050, APIs 5/6/11, JRW12, SP save V38 and renderer API 1023 are
unchanged. Full build/install qualification will be recorded after deployment.
The overall goal remains active. Next public release: 1.12.0.

## Latest installed release: 1.10.0 (2026-10-11)

Build `20261011-214919-218c665d` installed at 21:51 EDT through scripts/build-jar.ps1.
All 114 automated suites passed, including production classic/Rend2 lightmap
loading, packing, HDR/deluxe lookup, capacity and reset fixtures. Two native
lightmap loaders also passed on two retail main/sub-map pairs using mp/ffa3
(26 source lightmaps), mp/duel4 (two) and t1_inter (12). Three native retail
resource loaders, seven-card droid spawn/restore and YT-1300 rig/persistence
checks passed. No graphical client was launched; GPU drawing and gameplay
acceptance remain unverified. External image decoders/uploads are fixture
boundaries, while real embedded retail BSP lightmap data is decoded directly.

Independent readback verified 68 installed files, 17 replacement backups
and 1897 archived sources. All five engine/launcher executables report
Windows version 1.10.0.0. ZIP SHA-256:
`90CD036FD69680ED86B988AA47C7E37DC3B725BD5F568E589C5C87971151FCDE`.
Receipt: `build/deploy/20261011-215132-c2564e71/receipt.json`.
Qualification: `build/bsp-lighting-1.10.0/qualification.json`.
Catalogs, asset packs, configuration/launcher scripts, APIs and save formats
are unchanged from 1.9.0. Build-info now correctly labels the existing SP
renderer API as 1023. Settings, saves and unrelated mods were preserved.

BSP lightmap ownership/resource isolation is installed. Portable misc_bsp
remains unavailable pending native child ownership, target namespaces,
transforms, copy/save/restore and full renderer integration. The next renderer
audit covers sub-map fog indices and light-grid selection (Rend2 drawing still
references the main world's volumetric grid). Terrain remains unavailable.
SP retains 1,175 cards (two unavailable, 28 approved exclusions); MP retains
1,000 choices, feature 34, protocol 1050, APIs 5/6/11 and JRW12. SP save V38
and renderer API 1023 are unchanged. The overhaul goal remains active.
Next public release: 1.11.0. These receipt notes were added after packaging;
all runtime sources remain exact.

## 1.10.0: BSP lighting ownership (2026-10-11)

The classic SP and MP renderers now share the actual lightmap loader. Sub-map
lightmaps append to the global table, with bounded per-map indices and external
textures resolved from their owning map's directory. Main-map lighting remains
intact. MP's single-lightmap workaround no longer reads a nonexistent second
tile. RGB-to-RGBA expansion no longer reads past the last RGB pixel. The existing
256-image classic limit fails explicitly before overflow; it was not increased.

Rend2 gives each BSP its own packing, HDR and deluxe-map state, unique image
names, and stable indices in a bounded aggregate texture table. Shaders select
HDR/deluxe resources by their owning lightmap. Missing external HDR can fall
back to TGA, rectangular HDR pixels decode correctly, and temporary HDR buffers
use the decoder's allocator. Partial deluxe atlases initialize missing tiles
to a neutral direction. Main-map sun, exposure, tone mapping and culling state
survive child loading; child worldspawn settings cannot globally remap shaders.

The fixtures execute production loaders, packing, resource lookup and scope
restoration. They check mixed LDR/HDR and deluxe maps, exact pixel output,
external paths, missing textures, limits and map resets. Additional native
checks decode real retail BSP data from mp/ffa3, mp/duel4 and t1_inter in two
main/sub-map pairs. Disk-image decoders and GPU upload/drawing remain fixture
boundaries; no graphical result or gameplay acceptance is claimed.

The release pipeline now includes the retail BSP checks. Its build-info renderer
API label is corrected to the existing 1023 (no API change). Full build/install
qualification and receipt evidence will be recorded after deployment.

Portable misc_bsp remains unavailable until native child ownership, target
namespaces, transforms, copy/save/restore and renderer integration are complete.
Terrain also remains unavailable. Catalogs stay at 1,175 SP cards (two unavailable,
28 approved exclusions) and 1,000 MP choices. MP feature 34, protocol 1050,
APIs 5/6/11, JRW12, SP save V38 and renderer API 1023 are unchanged.
The overhaul goal remains active. Next public release: 1.11.0.

## Latest installed release: 1.9.0 (2026-10-11)

Build `20261011-212936-ccd33075` installed at 21:32 EDT through scripts/build-jar.ps1.
All 112 automated suites passed, including shared BSP registration, classic
renderer cache, and SP/MP collision metadata fixtures. Three native retail
resource loaders, seven-card droid spawn/restore and YT-1300 rig/persistence
checks also passed. File/GPU loaders and synthetic collision maps remain
explicit boundaries of the new fixtures; no graphical client was launched.

Independent readback verified 68 installed files, 19 replacement backups
and 1890 archived sources. All five engine/launcher executables report
Windows version 1.9.0.0. ZIP SHA-256:
`B4C3DC1A15534720338BCAC85C30098D15079413D7DE32EC3D9CEEAB04993030`.
Receipt: `build/deploy/20261011-213213-7ea19153/receipt.json`.
Qualification: `build/bsp-registration-1.9.0/qualification.json`.
Catalogs, asset packs, configuration/launcher scripts and save formats are
unchanged from 1.8.0. MP client API is now 6; the matching engine and client
DLL were installed together. No settings, saves or unrelated mods were replaced.

Shared dynamic registration is installed. Portable misc_bsp remains unavailable
pending safe lightmaps/lighting state, child ownership/target namespaces,
transforms and persistence. Terrain also remains unavailable. SP retains 1,175
cards (two unavailable, 28 approved exclusions); MP retains 1,000 choices,
feature 34, protocol 1050, game/client/renderer APIs 5/6/11 and JRW12. SP save
V38 and renderer API 1023 are unchanged. Gameplay/visual acceptance and the
overhaul goal remain open. Next public release: 1.10.0.
These receipt notes were added after packaging; all runtime sources remain exact.

## 1.9.0: shared dynamic BSP model registration (2026-10-11)

SP and MP now use one client registration path for loaded map sections. It runs
at map initialization and on BSP configstring changes, resolves each section's
actual collision-model range, and registers named root/child draw models rather
than guessing handles from renderer load order. It handles gaps, duplicate
names, bounded ranges and retries on a subsequent registration call. A missing
draw model is not cached as a completed section. Aggregate collision-model
allocation now reserves the engine's temporary box/capsule handles.

Classic SP/MP and Rend2 accept named child models and reuse already loaded map
sections. Missing files do not consume renderer BSP slots. Rend2 clears its
actual BSP registry when models reset. Sub-map loading preserves the main
world's directory and uses its own visibility data; MP classic no longer reads
the main map's cached file as a different sub-map.

Production-code fixtures cover the shared client path, classic renderer alias
lookup, and both collision registries, including late arrivals, different load
orders, invalid names, partial failures, capacity and reset behavior. File/GPU
loads and synthetic collision-map data remain explicit fixture boundaries.
Full release build/install qualification will be recorded after deployment.

MP's native client API advances from 5 to 6, requiring the matching engine and
client DLL supplied together. Game/renderer APIs stay 5/11, protocol 1050,
feature 34, world format JRW12; SP save V38 and renderer API 1023 are unchanged.
Public numbering continues from the owner's 1.0 baseline: 1.9.0, then 1.10.0.

This is registration groundwork, not completed misc_bsp menu spawning. MP
classic and Rend2 still need safe shared lightmap/lighting state, and the native
controller still needs child ownership, target namespaces, transforms and
save/restore integration. Terrain remains unavailable. Catalogs retain 1,175
SP cards (two unavailable, 28 approved exclusions) and 1,000 MP choices. No
graphical/gameplay acceptance is claimed, and the overhaul goal remains active.

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

## Latest installed release: 1.7.0 (2026-10-11)

Build `20261011-205412-ac54978b` installed at 20:56 EDT through scripts/build-jar.ps1.
All 107 automated suites, three native retail-resource loaders, the seven-card
droid spawn/restore fixture and the native YT-1300 rig/persistence checks passed.
The new SP fixture executes the production turret controller and projectile
emitter, covering factions, LOS/PVS, banked aiming, lead, clamps, alternation,
cooldowns, saved fields and allocation failure without spending ammunition.
Renderer matrices, traces and allocation are explicit fixture boundaries.
No graphical game or human remote client was launched; SP combat, visual
appearance and full in-game save/load still require acceptance.

Independent readback verified 68 installed files, 15 replacement backups
and 1882 archived sources. All five engine/launcher executables report
Windows version 1.7.0.0. ZIP SHA-256:
`0C8395F5F377524A9775145DCD46CB057C59855DB19A19B644B0B19F5506BA0E`.
Receipt: `build/deploy/20261011-205623-1ad24333/receipt.json`.
Qualification: `build/sp-vehicle-turrets-1.7.0/qualification.json`.
Source comparison confirms the projectile extraction preserves every original
branch/property and its existing entry point, adding allocation-success returns.
All catalogs, asset packs, configuration/launcher scripts and save/API headers
match 1.6.0. Only the SP turret path and its tested integration are added.

SP remains at 1,175 cards with two unavailable (misc_bsp and terrain), plus
28 approved exclusions. MP retains 1,000 choices, feature 34, protocol 1050,
APIs 5/5/11 and JRW12. SP save V38 and renderer API 1023 are unchanged.
SP autonomous vehicle guns are implemented. Its single-pilot boarding remains;
SP passenger/gunner integration is absent and MP passenger use needs gameplay
acceptance. The broader overhaul goal and outstanding parity/acceptance work
remain active. The next normal public release is 1.8.0.
These receipt notes were added after packaging; all runtime sources remain exact.

## 1.7.0: SP autonomous vehicle turrets (2026-10-11)

SP now runs the vehicle turret definitions already present in retail data,
including the restored YT-1300's two guns. They acquire hostile actors using
the pilot's SP faction, respect native explicit enemies, line of sight, range
and gun limits, lead moving targets and turn before firing. Empty neutral ships
do not automatically attack the player. Pilot, attached droid and friendly
actors are excluded. Native gun matrices are sampled in the game update, so
turret fire does not depend on a recent rendered frame. Full vehicle pitch,
yaw and bank are included in aiming.

Shots use the existing SP vehicle projectile implementation and effects.
Barrels alternate with the configured delay and ammunition; failed allocation
does not charge ammo or emit a muzzle flash. Native ammo recharge and the
existing saved ammunition, next barrel, target and muzzle cooldown fields are
retained. The old vehicle weapon entry point remains compatible with pilot
weapons and the randomizer. No save, API or protocol revision is needed.

The new production-code fixture checks target filtering, blocked/PVS shots,
turn rate, aim limits, banked orientation, leading, projectile properties,
allocation failure, ammo/cooldown, invalid model/weapon data and native field
serialization. Renderer matrices, world traces and allocation are fixture
boundaries; graphical SP combat and full save/load acceptance remain open.
SP passenger storage/boarding and gunner views are not implemented: its native
boarding code always makes the player the pilot. That integration remains
explicitly open; autonomous turret implementation is not passenger parity.

Catalogs are unchanged: 1,175 SP cards, two unavailable (misc_bsp and terrain),
28 approved exclusions and 1,000 MP choices. MP feature 34, protocol 1050,
APIs 5/5/11, JRW12, SP V38 and renderer API 1023 are unchanged. The broader
overhaul goal remains active. Release/install evidence follows after qualification.

## Latest installed release: 1.6.0 (2026-10-11)

Build `20261011-204040-c147816d` installed at 20:43 EDT through scripts/build-jar.ps1.
All 106 automated suites, three native retail-resource loaders, seven-card droid
spawn/restore checks and the new YT-1300 asset/native checks passed. A separate
pre-release native run spawned all 21 vehicle choices together, saved/reloaded
them and restored them after map restart; two native bot clients also connected.
Only public numbering/docs changed after that vehicle run. No graphical game
or human remote client was launched. Vehicle flight, rendered gears/effects,
combat, passenger use and SP native spawn remain pending gameplay acceptance.

Independent readback verified 68 installed files, 17 replacement backups
and 1879 archived sources. All five engine/launcher executables report
Windows version 1.6.0.0. ZIP SHA-256:
`09894FE71698388B85BDFA27D6E807A11F6C78683B79ACC98CC5AC192A60CCAF`.
Receipt: `build/deploy/20261011-204310-7440f380/receipt.json`.
Qualification: `build/yt1300-1.6.0/qualification.json`.
The two private mod folders receive the same additive YT-1300 pack. Existing
asset packs, stable catalog IDs and original vehicle/weapon definitions remain
unchanged. Exactly one SP card changes availability and one prop card is added;
MP gains the corresponding vehicle and prop choices.

SP has 1,175 cards, two unavailable entries (misc_bsp and terrain) and 28 approved
exclusions. MP has 1,000 choices and feature version 34. Protocol 1050, APIs 5/5/11,
JRW12, SP save V38 and SP renderer API 1023 remain unchanged. YT-1300 uses the
native fighter path; SP turret AI parity is still open because that engine lacks
the MP turret controller. The broader overhaul goal and other MP parity/acceptance
requirements remain active. The next normal public release is 1.7.0.
These receipt notes were added after packaging; all runtime sources remain exact.

## 1.6.0: YT-1300 catalog restoration (2026-10-11)

The previously unavailable YT-1300 now has a spawn route in both SP and MP.
Its missing vehicle model is supplied by adapting the retail Falcon scenery
mesh to Ghoul2: all 4,218 vertices, 3,703 triangles, UVs, normals and materials
are preserved. A 28-bone rig supplies native cockpit, missile, turret-camera,
gun and exhaust attachments plus 17 landing-gear poses. Missing engine audio,
effects and damage icons reuse unedited retail assets. The original NPC,
vehicle and weapon definitions stay unchanged; no retail files are replaced.
The same body is also available as a decorative prop.

An independent asset audit compares every original vertex and triangle and
checks the rig, poses, materials and alias bytes. An isolated native MP server
has verified vehicle/prop spawn, health, model bounds, all twelve required bolts,
turret yaw/pitch directions, gear bounds, save/restore and cleanup. The diagnostic
uses the same bolt convention as native weapons and turret cameras. These are
loader/transform/lifecycle checks, not rendered flight, firing or passenger
acceptance. SP native turret AI is absent from the existing engine; bringing
that behavior across remains open. SP graphical spawn and flight also need
acceptance. The standard release workflow now gates on the YT asset/native tests.

SP has 1,175 cards, two unavailable entries (misc_bsp and terrain) and the same
28 approved exclusions. Of the original 182 unavailable cards, 153 are routed,
27 excluded and two open. Entity classes remain 203 routed / six excluded /
two open out of 211. MP has 1,000 choices: 21 native vehicle definitions and
132 skeletal props among 688 prop choices. MP feature version is 34; protocol
1050, APIs 5/5/11, JRW12, SP save V38 and SP renderer API 1023 are unchanged.
Public numbering continues upward from the requested 1.0 restart. Release
qualification and installation will be recorded after the standard build.
The broader overhaul goal, MP parity and outstanding gameplay acceptance stay open.

## Latest installed release: 1.5.0 (2026-10-11)

Build `20261011-201718-056a7e35` installed at 20:19 EDT through scripts/build-jar.ps1.
All 106 automated suites, three native retail-resource loaders and the isolated
seven-card MP droid spawn/restore checks passed. Reflection-controller admission,
client ownership, refresh, offscreen submission, UI commands, properties, copy,
Undo and native save fields are covered. The save audit also extended vehicle
registry handling to V38; production vehicle restore tests exercise V34, V37 and
the current format with all 63 definitions registered in a different order.
The earlier test failure is preserved in the evidence folder; it did not install.
OpenGL drawing/convolution, visual quality and capture frame times still require
in-game acceptance. No graphical game or human remote client was launched.

Independent readback verified 66 installed files, 18 replacement backups
and 1875 archived sources. All five engine/launcher executables report
Windows version 1.5.0.0. ZIP SHA-256:
`7F799984AFDD65CB751579DEE7CB7C757C06A4D1BC8CCFD0C46DF54D1D6D34CC`.
Receipt: `build/deploy/20261011-201904-90a51e1f/receipt.json`.
Qualification: `build/sp-cubemap-controller-1.5.0/qualification.json`.
Standalone deployment's shared-version default was separately checked without
invoking deployment a second time. Existing asset packs and MP catalog match 1.4.

SP now writes V38 saves and retains renderer API 1023. MP feature 33, protocol
1050, APIs 5/5/11 and JRW12 remain unchanged. Of 1,174 SP cards, three remain
unavailable: YT-1300, misc_bsp and terrain. The approved exclusion count stays 28.
MP retains 998 choices. SP misc_cubemap is implemented; reflection appearance,
performance, MP reflection-controller parity and broader overhaul requirements
remain open. The goal stays active. The next normal public release is 1.6.0.
These receipt notes were added after packaging; all runtime sources remain exact.

## 1.5.0: usable Sandbox reflection probes (2026-10-11)

The SP `misc_cubemap` card now spawns a removable stock crate marker controlling
a live reflection probe. Select it with C and open Reflect to edit radius
(16-32768 units), turn it on/off or recapture the map. Move it with the Physgun
or context properties. Captures require Rend2 and Graphics > Cubemap reflections;
the page reports disabled/unsupported/pending/ready status without changing
the user's graphics settings. Up to 64 portable probes coexist with authored
map probes. A settled marker requests one six-face static-map capture; nearby
reflective materials use it. Actors and props are not included in these native
map captures. Visual quality and capture frame time still need gameplay checks.

Property Undo, removal, cleanup, copies and save/load are integrated. Each new
spawn, replacement, restore or renderer lifecycle obtains a new capture owner;
radius-only changes reuse pixels. Disabling removes influence, and re-enabling
recaptures. Native USE toggles the probe. SP saves now write V38 while retaining
older readers; the new callback appends to existing serialized identifiers.
SP renderer API stays 1023. MP feature 33, protocol 1050, APIs 5/5/11 and JRW12
remain unchanged. MP reflection-controller parity is still open: this work
uses SP's existing Rend2 implementation, not the installed MP Classic renderer.

SP retains 1,174 cards and 28 approved exclusions, with three unavailable entries:
YT-1300, misc_bsp and terrain. Of the original 182 unavailable cards, 152 are now
routed, 27 explicitly excluded and three open. Entity classes: 203 routed,
six excluded and two open out of 211. MP retains 998 choices. No other exclusions
or catalog IDs change. The overall goal and broader MP parity remain active.

The native controller/client fixture covers admission, stable/new capture owners,
out-of-view submission, activation, movement, refresh, corrupt restore rejection,
native save fields and the 64-probe boundary. Existing context, duplicate, UI and
native spawn fixtures exercise the new route. OpenGL drawing/convolution and
human gameplay remain separate acceptance boundaries. Release qualification and
installation evidence will be recorded after the standard build completes.

Public versioning continues upward from the requested 1.0 restart; the next
normal release is 1.6.0. Standalone deployment now resolves its default ZIP from
the shared release header, like the build script, instead of an obsolete version.

## Latest installed release: 1.4.0 (2026-10-11)

Build `20261011-200020-11cf61be` installed at 20:02 EDT through scripts/build-jar.ps1.
All 105 automated suites, three native retail-resource loaders and the isolated
seven-card native droid spawn/restore check passed. The new reflection fixture
executes production ownership, scheduling, six-face view construction, selection
and sorting. It covers capture deferral, movement/refresh/radius, deletion/reuse,
64 live slots plus 128 native probes, invalid snapshots, stereo/anaglyph, clock
rollover, allocation retry, map reset and gameplay-camera history preservation.
OpenGL drawing/convolution remain fixture boundaries; no appearance or frame-time
acceptance is claimed. Native renderer-DLL checks verify the new API and reject
the old ABI. No graphical game or human remote client was launched.

Independent readback verified 66 installed files, 9 replacement backups
and 1871 archived sources. All five engine/launcher executables report
Windows version 1.4.0.0. ZIP SHA-256:
`776C7E5B11AFAD6EEAA8F67853E7A8628DB7331AA408B95CDEC81654E6370331`.
Receipt: `build/deploy/20261011-200205-282b2dbb/receipt.json`.
Qualification: `build/sp-reflection-probes-1.4.0/qualification.json`.
An earlier 1.4 candidate was rebuilt after review found that capture passes
must retain gameplay-camera temporal history. Its ZIP and log are preserved
under the same evidence directory; both installations retain rollback receipts.

SP renderer API is 1023. MP feature 33, protocol 1050, APIs 5/5/11, JRW12 and SP
save V37 remain unchanged. SP has 1,174 cards, four unavailable entries and 28
approved exclusions; MP has 998 choices. Both catalogs and existing asset packs
are unchanged. misc_cubemap is still blocked pending its game controller and
menu/save/copy/Undo integration. Broader MP parity and gameplay acceptance stay
open; the overall goal remains active. The next normal release is 1.5.0.
These receipt notes were added after packaging; runtime sources still match
the archived release.

## 1.4.0: live reflection renderer foundation (2026-10-11)

The SP Rend2 renderer now accepts an authoritative set of live reflection
probes through its API and the cgame bridge. This is the renderer prerequisite
for the still-blocked misc_cubemap Sandbox controller. The card is not yet
enabled: entity spawning, C properties, client submission, Undo/copy/save
integration and gameplay acceptance remain required. SP still has four
unavailable entries; neither catalog changes in this release.

Submissions copy and validate owners, revisions, finite positions and radii,
reject duplicates and report unsupported Classic, disabled cubemapping,
missing world or capacity explicitly. Rend2's existing Cubemap reflections
graphics setting remains opt-in and requires its normal renderer restart.
No graphics settings are changed automatically. At most 64 live probe images
are retained and reused in addition to the map's existing probes. Moving or
refreshing a probe hides its old capture until its position settles for 150 ms;
one complete six-face world capture and native roughness convolution is queued
per frame. Radius-only edits reuse pixels. Unchanged probes do no capture work.

Captures run before scene submissions, guard against recursive BeginFrame calls,
preserve game render time and the previous gameplay camera used by temporal
anti-aliasing, and use valid stereo/anaglyph frame selection. Six capture passes
preserve jitter parity. Prior model/bone GPU offsets are discarded after the
frame buffers cycle, rather than reused for unrelated data. Input with pending
scene geometry defers work. Omitting an owner retires its reflection;
reusing its renderer slot cannot expose its previous image before recapture.
Native map probes, their assignments and images remain intact. Live probes
override nearby model/world/merged-surface reflections inside their radius;
moving brush geometry uses its world-space center. Removal restores native
assignments. Map/renderer reset clears ownership; native texture teardown owns
GPU image cleanup. The 64-image budget prevents accumulation across edits.

Draw sorting now retains 16-bit cubemap indices. The previous six-bit field
aliased index 64 and above, including existing authored probes. Missing native
images and zero parallax radii no longer enter the cubemap shader sampling path.
SP renderer API advances from 1022 to 1023; both renderer providers and the
engine bridge agree. MP protocol 1050, APIs 5/5/11, feature 33, JRW12 and SP
save V37 remain unchanged. Public release numbering continues to 1.5.0 next.

The focused fixture executes the production registry, frame scheduler, native
face construction, point/surface selection and sort functions. It covers all
six face directions, 128 authored plus 64 live probes, reordered snapshots,
invalid input, quiet-period movement, explicit refresh, radius-only changes,
removal/reuse, deferred scene data, stereo/anaglyph, clock rollover, image
allocation failure/retry, no-native-probe maps, camera-history preservation and
map reset. OpenGL drawing
and convolution are explicit fixture boundaries: these results do not prove
rendered appearance or frame-time cost. No game launch or owner acceptance is
claimed. Release/install evidence is recorded separately after verification.

## Latest installed release: 1.3.0 (2026-10-11)

Build `20261011-193754-7f258f3b` installed at 19:39 EDT through scripts/build-jar.ps1.
All 104 automated suites, three native retail-resource loaders and the isolated
seven-card native droid spawn/restore check passed. Laser fixtures cover native
trace geometry, slot-zero/creator hits, eight refusal cases, current target
positions, harmless mode, property Undo, switches, JRW12 state, copy/cleanup,
source/helper slot reuse and connection-safe damage attribution. The client
fixture executes the unchanged native RT_BEAM draw function and typed C inputs.
Regression checks read all JRW1-11 strides and retain old vehicle-motion bounds.

Independent readback verified 66 installed files, 16 replacement backups
and 1865 archived sources. All five engine/launcher executables report
Windows version 1.3.0.0. ZIP SHA-256:
`4F0D960F82FA65EEE8CC306B9BCEF96B8A627414482B2FACEDD28E62CC709E13`.
Receipt: `build/deploy/20261011-193940-fde543de/receipt.json`.
Qualification: `build/mp-laser-1.3.0/qualification.json`.

MP feature 33, 998 cards and 13 controller/marker entries. Existing 997 MP rows,
the SP catalog, stock icon/appearance packs, controls and legacy packs are
unchanged. JRW12 appends laser damage; readers retain JRW1-11. Protocol 1050,
APIs 5/5/11 and SP save V37 stay unchanged. Public versioning has one source.

No graphical game or human remote client was launched. Collision and health
damage are fixture boundaries; these checks do not establish rendered visibility,
combat feel or remote behavior. Four unavailable SP entries, broader MP parity
and outstanding gameplay acceptance remain open; the overall goal is active.
These receipt notes were added after packaging; runtime sources still match
the archived release.

## 1.3.0: portable multiplayer laser controllers (2026-10-11)

The MP Entities tab adds target_laser with its existing SP ID and stock card/body
art: 998 choices and 13 controller/marker cards. Its movable body owns a private
native ET_BEAM helper. The existing red RT_BEAM renderer needs no new texture or
effect assets. The native point trace retains its 2048-unit range and solid/body/
corpse collision mask. Beam bounds span both endpoints for network visibility.
Rotate the body to aim, or Link Primary to a spawned prop, NPC, vehicle or marker;
aim follows the current hull center instead of a stale network origin.

Activate toggles firing. C > Controller offers Firing, Enabled and whole damage
0-1000 per native tick; zero keeps a harmless visible beam. Setting changes and
copy/restore do not inflict damage immediately. Enabled/Firing and permission
changes hide or resume the beam without deleting its body. Deleting an aim target
stops firing before its slot can be reused. Activate/Deactivate controllers also
pause/resume the laser. Property Undo and linked assembly copies are supported.

Damage uses native G_Damage and its game rules, with current Sandbox ownership
and player-consent checks. Player slot zero and the creator can be hit. Players'
tool opt-outs and release grace are honored, including for restored server-owned
hazards. Protected targets still block the trace. Connection epochs prevent kill
credit from transferring to a later occupant of a client slot; context Firing and
Undo restarts credit the current editor. Damage callbacks may destroy/reuse the
source, so all publishing occurs before damage and no stale write follows it.
Authored-map lasers retain their existing callback behavior.

JRW12 (JRWC) appends the typed damage value to 272-byte records. JRW1-11 read
support retains original strides and vehicle-motion bounds; older builds reject
the new magic rather than misreading records. Protocol 1050, APIs 5/5/11 and SP
save V37 are unchanged; MP feature 33. Public version remains sourced from
shared/qcommon/jar_release.h; the next normal release is 1.4.0.

Focused production fixtures passed for native trace geometry, eight damage
refusals, moving targets, harmless mode, switch/Undo behavior, state validation,
copy/world restore, deletion/source/helper reuse and connection attribution.
Client fixtures execute native RT_BEAM submission and the typed/pinned C controls.
Engine collision and health damage are fixture boundaries; visual, combat and
remote-client acceptance require gameplay. Full release/install evidence follows
below. SP remains at 1,174 cards, four unavailable entries and 28 approved
exclusions. Broader MP parity and remaining owner acceptance stay open.

## Latest installed release: 1.2.0 (2026-10-11)

Build `20261011-192401-2c277fd2` installed at 19:25 EDT through scripts/build-jar.ps1.
All 104 automated suites, three native retail-resource loaders and the isolated
seven-card native droid spawn/restore check passed. The new switch fixture covers
12 controller families, both outputs, media pause/resume, six refusal cases,
native pending-Delay behavior, authored-map isolation, property Undo conflicts,
copy/JRW11 restoration, slot reuse and grouped menu helpers. The menu fixture
checks typed settings, Enabled commands and malformed echoes.

Independent readback verified 66 installed files, 16 replacement backups
and 1860 archived sources. All five installed engine/launcher executables
report Windows version 1.2.0.0. ZIP SHA-256:
`9F38F3DA1A65FE5E7968BF41FD4689D4FDF003785DEFAC7E34DFBDDADA348DBF`.
Receipt: `build/deploy/20261011-192546-2dd75295/receipt.json`.
Qualification: `build/mp-controller-switches-1.2.0/qualification.json`.

Versioning now has one source: shared/qcommon/jar_release.h. Public releases
continue 1.0.0, 1.1.0, 1.2.0, then 1.3.0. Earlier package-only numbering did not
update the stale SP/MP runtime labels; this release fixes that inconsistency.
The post-build checker initially assumed DLL version resources and a different
path ordering; those checker assumptions were corrected without runtime changes.

MP feature 32, 997 cards and 12 controller/marker entries. Existing 995 MP rows
and all SP catalog entries are unchanged. JRW11, protocol 1050, APIs 5/5/11 and
SP save V37 retain their compatibility meaning. No graphical game or human
remote client was launched. Four unavailable SP entries, broader MP parity and
new gameplay/rendered acceptance remain open; the overall goal is active.
These receipt notes were added after packaging; runtime sources still match
the archived release.

## 1.2.0: shared release numbering and MP activation controllers (2026-10-11)

Public numbering continues from 1.0.0 and 1.1.0. SP/MP banners, Windows engine
and launcher version resources, ZIP naming and build-info now read the single
`shared/qcommon/jar_release.h` source. The prior packages had advanced while SP
still displayed 0.88 and MP retained its stock label; 1.2.0 corrects this mismatch.
Future normal releases increment the minor number there (next 1.3.0).
Protocol, API and saved-world revisions remain independent compatibility values.

The MP Entities tab now includes target_activate and target_deactivate with the
same stable IDs and stock icon/body art as SP: 997 choices, 12 controller/marker
cards, 82 actor aliases and 687 model props. Link Primary and Secondary to spawned
controllers, then use Activate or C. Each permitted linked target is enabled or
disabled without invoking its action. Both cards spawn with a Print helper and
one grouped Undo. Protected, unready or deleted targets are skipped; authored map
wiring is outside this adapter. Destination markers are valid state targets.

Native activation callbacks and FL_INACTIVE behavior are retained. Sounds and
effects immediately pause/resume their helpers while preserving Playing state.
Disabling a Delay prevents new activations but preserves an already-scheduled
event, matching native behavior. Cycles remain rejected. C shows Enabled state
and link help, with normal property Undo/conflict checks. Clipboard assemblies
and JRW11 saves preserve state and remap links; JRW1-10 readers remain supported.
The teleport panel now correctly describes travel with consenting vehicle riders.

Focused production-code fixtures passed for all 12 controller families, both
outputs, immediate media state, six permission/readiness refusals, delayed events,
native authored-map isolation, deletion/slot reuse, copy/world restore, grouped
Undo and typed context commands. Full release qualification/install is pending
below. Fixtures do not establish human listening/rendering or remote acceptance.

MP feature 32; protocol 1050 and APIs 5/5/11 unchanged. SP remains at 1,174 cards,
four unavailable entries and 28 approved exclusions. This MP port does not solve
YT-1300, misc_bsp, misc_cubemap or terrain. Broader controller/settings/combat
parity and outstanding gameplay acceptance remain open; the goal is active.

## Latest installed release: 1.1.0 (2026-10-11)

Build `20261011-191233-c88e1420` installed at 19:14 EDT through scripts/build-jar.ps1.
All 104 automated suites, three native retail-resource fixtures and the isolated
native seven-card droid spawn/restore check passed. Vehicle Push fixtures cover
four families, seated C/Delay activation, 17 refusal cases, consent revocation,
exact high-speed world restoration and old-format validation. The native swept
step/slide fixture verifies maximum pushes at 20/40/125 commands per second,
full-hull impacts and a single gravity pass.

Independent readback verified 66 installed files, 9 replacement backups
and 1856 archived source files. ZIP SHA-256:
`05803EBFECC0AC2E5058D31A4FD4E2165E299FD2DEF746C1A3444C090964A030`.
Receipt: `build/deploy/20261011-191419-968ca48b/receipt.json`.
Qualification: `build/mp-vehicle-push-1.1.0/qualification.json`.

MP feature is 31, with JRW11 (JRWB) writes and JRW1-10 reads. Protocol 1050,
APIs 5/5/11, SP save V37, catalogs, controls, launchers and legacy packs remain
unchanged. The initial focused movement check caught two fixture setup errors
(a reset timestep and an exact-contact boundary); these were corrected before
the full passing build. Production behavior was not weakened to satisfy them.

The authoritative checklist now also reflects the owner's later acceptance of
Noghri attacks and Desann/Mon Mothma/RebornBoss/Morgan spawning. These do not need
repeat approval. Nulldriver and newly restored legacy actors remain separate.

No graphical game or human remote client was launched. Native vehicle
hover/flight/steering continue after a push, so exact ballistic landing is not
promised under driving input. Human driving/rendered acceptance, four remaining
SP unavailable entries, broader controllers and MP presentation/combat parity
remain open. The overall goal is active. These receipt notes were added after
packaging; runtime sources still match the archived release.

## 1.1.0: multiplayer vehicle Push controllers (2026-10-11)

Push controllers now accept settled vehicles with consenting pilots/passengers,
including activation through C while seated and through linked Delay controllers.
The native push callback still owns behavior activation and sound/debounce.
Push replaces current travel velocity, resets commanded travel speed to zero,
and stores its horizontal component as replicated external momentum. Native
steering, input commands, hover/flight/gravity and swept collision then continue;
no seat link, position, angle, health or queued animation/weapon event is changed.
This does not promise an exact ballistic landing while vehicle controls or
native hover/flight physics continue acting.

Constant, unlinked angle and arc modes retain their existing properties and
full 4000-unit component limit. Existing thruster/joint impulses no longer
truncate an already stronger controller push to their usual 1500-unit limit.
Boarding, invalid seats, held/frozen actors, missing/protected destinations,
invalid trajectories and every rider's consent are checked before the impulse.
Failure messages remain visible. Authored-map Push callbacks retain their
original route. Physgun and ordinary force defaults are unchanged.

JRW11 (JRWB) writes the existing record layout with an explicit wider bounded
vehicle-motion range; readers retain JRW1-10 with their original drift bounds.
New high-speed motion saves fail clearly on older builds rather than being
misidentified as old-format data. World restoration retains the impulse;
clipboard copies remain stationary and automatic occupied snapshots remain
parked. Protocol 1050, APIs 5/5/11 and SP save V37 stay unchanged; MP feature 31.

Production fixtures cover four vehicle families, C/Delay activation, 17 refusal
cases, command/seat/event preservation, consent revocation, exact high-speed
world restore and legacy validation. Native step/slide fixtures cover maximum
pushes at 20/40/125 commands per second, swept hull impacts and a single gravity
pass. Two test setup defects (a reset timestep and an exact-contact boundary)
were corrected without changing the runtime to satisfy them. Native controller
output and map/bolt imports remain fixture boundaries; human driving, rendered
travel and remote-client acceptance remain open.

SP remains 1174 cards with four unavailable and 28 owner-approved exclusions;
MP remains 995 choices and 82 map actor aliases. Broader controllers, the four
unavailable SP entries, presentation/combat parity and real remote/late-join/load
validation remain open. The goal remains active. Release installation and
independent package verification are recorded separately.

## Installed baseline before 1.1.0: 1.0.0 (2026-10-11)

Build `20261011-190204-e0e7680e` installed at 19:03 EDT through scripts/build-jar.ps1.
All 104 automated suites, three native retail-resource fixtures and the isolated
native seven-card droid spawn/restore check passed. Vehicle teleport fixtures
cover four hull families, pilot/passenger/droid arrivals, seated C activation,
25 refusal/rollback cases and delayed consent changes.

Independent readback verified 66 installed files, 9 replacement backups
and 1855 archived source files. ZIP SHA-256:
`CF0822C706130237E5F551B260D221BD8CE909918979639A81C6E905CDCCCD7A`.
Receipt: `build/deploy/20261011-190350-9f92de08/receipt.json`.
Qualification: `build/mp-vehicle-teleport-1.0.0/qualification.json`.

At the owner's request public numbering restarts at 1.0.0, then continues
1.1.0, 1.2.0 and upward. MP feature is 30; protocol, API and save-format numbers
retain their existing compatibility meaning. Catalogs, controls, launchers and
legacy content packs are unchanged. The first pre-release build caught a stale
feature-29 expectation in the droid fixture, before packaging or installation.
That fixture now checks the candidate source's feature number.

No graphical game or remote human client was launched. Vehicle teleporting is
implemented and fixture-qualified; rendered travel and human driving acceptance
remain open. The four unavailable SP cards, wider controllers, vehicle Push and
broader MP presentation/combat parity remain open. The overall goal is active.
These receipt notes were added after packaging; runtime sources remain exact.

## 1.0.0: multiplayer vehicle teleport controllers (2026-10-11)

At the owner's request, public release numbering restarts at 1.0.0 and then
continues 1.1.0, 1.2.0 and upward. Internal protocol, API and save-format
versions keep their existing compatibility meaning.

Owned teleport controllers now carry a settled vehicle, pilot, passengers and
its attached droid together. A seated player can select a controller in C and
use Activate without dismounting; other editing/equipment commands retain their
existing restrictions. Linked Delay controllers resolve the current vehicle at
activation time and recheck every rider's consent and ownership permissions.

Arrival is checked before changing positions: the destination-oriented native
vehicle hull, native yaw-only driver/droid bolts and MP passenger locations must
all fit. Failed consent, boarding, stale seats, missing bolts, blocked hulls,
invalid coordinates or entity reserve leave all members untouched. Ropes, welds
and hinges involving the vehicle or riders must be removed first. Native
teleport effects and vehicle launch velocity are retained; stale steering,
turbo, angular motion and external drift are cleared. Every rider receives the
teleport interpolation bit without consuming queued animation/weapon events.
Seat bindings, health and animations remain intact. Authored map teleport paths
are unchanged, and failure messages are no longer overwritten by "activated".

Production-code fixtures cover four vehicle families, seated C activation,
pilot/passenger/droid placement, 25 refusal cases, delayed consent changes,
empty native vehicle targets and stale context identity. Engine collision and
bolt imports are deterministic fixtures: rendered travel, human driving and
remote-client acceptance remain open. Full release/installation checks are
recorded separately. No graphical game is launched by this change.

MP feature is 30; protocol 1050, APIs 5/5/11, JRW10 and SP save V37 are unchanged.
SP still has 1174 cards, four unavailable entries and 28 approved exclusions;
MP still has 995 choices and 82 map actor aliases. Broader MP controllers,
vehicle Push behavior, presentation/combat parity and human remote/late-join/load
validation remain open. The overall overhaul goal remains active.

## Installed baseline before 1.0.0: 0.98.0 (2026-10-11)

Build `20261011-184839-d5870310` installed at 18:50 EDT through scripts/build-jar.ps1.
All 104 automated suites and three native retail-resource fixtures passed.
The isolated native server also spawned and restored all seven new MP droid
cards, checking their actual Ghoul2 poses, authored teams, gravity and cleanup.
Independent readback verified 66 installed files, 10 replacement backups
and 1853 archived source files. ZIP SHA-256:
`8BA4F81737C5E91DF5164FCD88390ADCA52A068CA5E6E70F97031AAB30ECDD0F`.
Receipt: `build/deploy/20261011-185021-dd6144a5/receipt.json`.
Qualification: `build/mp-droids-0.98.0/qualification.json`.

MP now has 995 choices, including 82 map actor aliases. The four original
Mouse/Remote/Seeker NPC definitions and three aliases use the original two
static meshes through a nine-file MP-only Ghoul2 adapter. All 622 vertices,
468 triangles, materials and UVs are retained. Existing 988 MP choices and the
complete SP catalog are unchanged. MP team parsing now uses the authored value.
The first full run caught an outdated 79-alias test expectation, now covering
all 82. The initial native check also exposed incorrect team-number/GLA-suffix
assumptions in the new fixture; those now match the engine's actual contracts.
Deployment initially rejected the new pack because its exact path was absent
from the installer list. The final build includes that path and requires the
adapter pack for 0.98.0 and newer releases, while preserving older ZIP rollback.
No graphical game or remote human-client testing occurred. Rendered/combat
acceptance, four SP unavailable entries and broader MP parity remain open.
The goal is active. These receipt notes were added after packaging; all runtime
sources still match the archived release.

## 0.98.0: original static droids in multiplayer (2026-10-11)

MP now exposes Mouse, Remote, Remote_SP and Seeker plus the existing
NPC_Droid_Mouse, NPC_Droid_Remote and NPC_Droid_Seeker map aliases. Their original
SP stable IDs are retained: 995 MP choices and 82 actor aliases. SP remains at
1174 cards, four unavailable entries and 28 owner-approved exclusions.

The two original one-frame MD3 bodies are converted to a single-bone Ghoul2
representation in an MP-only nine-file pack. All 622 vertices, 468 triangles,
UVs, normals, surface names and materials are retained; the bind vertices cancel
Ghoul2's 90-degree root-axis convention. The original one-frame animation is
retained. No original asset or NPC definition is overwritten. The adapter is
restricted to the four original body references and default skins; an existing
replacement Remote body takes precedence. Source/converted-model fingerprints
and required-file checks prevent silently using a changed or missing adapter.

Native classes, AI, sounds, teams and gravity remain in the MP NPC pipeline.
A pre-existing MP parser defect read the keyword token instead of its team
value; playerTeam/enemyTeam and team precaching now read the authored value.
`jar_actor_info CATALOG_ID` is a read-only server diagnostic for actual NPC
model, class, team, gravity, health and native posed bounds. MP feature is 29;
protocol 1050, APIs 5/5/11, JRW10 and SP save V37 remain unchanged.

Offline asset comparisons and the isolated native server check cover exact
geometry, seven-card admission, model/GLA loading, teams/gravity, frozen-state
save/restoration and cleanup. This does not establish rendered, AI/combat,
remote-client or performance acceptance. Full MP parity remains open, including
wider controllers, occupied-vehicle transport and presentation/combat behavior.
The goal remains active; release/installation evidence is recorded separately.

## Installed baseline before 0.98.0: 0.97.0 (2026-10-11)

Build `20261011-183158-4190ecd0` installed at 18:36 EDT through scripts/build-jar.ps1.
All 104 automated suites and three native retail-resource fixtures passed.
Independent readback verified 65 installed files, 11 replacement backups
and 1847 archived source files. ZIP SHA-256:
`26001D8784AD5DCD47737162AEEEFE8D5C4C95B2496C65D4BAB9D1C9EF6B53BF`.
Receipt: `build/deploy/20261011-183650-f03f752b/receipt.json`.
Qualification: `build/catalog-exclusions-0.97.0/qualification.json`.
Only the seven newly approved unavailable cards were removed. The remaining
1121 generated entries and all 988 MP choices are unchanged. SP has 1174 visible
cards, four unavailable entries and 28 total owner-approved exclusions.
The four remaining unavailable entries and MP parity work remain open; the goal
is active. No graphical game was launched. These receipt notes were added after
packaging; runtime sources still match the archived release.

## 0.97.0: approved unavailable-content exclusions (2026-10-11)

The owner explicitly approved: "Exclude and hide these seven". The NPC `test`
and entity spawners `NPC_Monster_Claw`, `NPC_Monster_Fish`, `NPC_Monster_Flier2`,
`NPC_Monster_Lizard`, `NPC_Monster_Murjj` and `NPC_Monster_Swamp` are hidden.
Their stable IDs, original metadata and missing-resource reasons remain in
`JAR-spawn-catalog.json` under `ownerExcludedContent`; the exact approved names
and decision are recorded in `JAR-catalog-exclusions.json`. This is an explicit
scope exclusion, not an implementation or asset restoration.

SP now has 1174 visible cards and four unavailable entries: `yt-1300`, `misc_bsp`,
`misc_cubemap` and `terrain`. There are 28 total approved exclusions: 21 empty
helpers plus these seven entries. Of the original 182 unavailable entries,
151 have implementation routes, 27 are owner-excluded and four remain open.
Entity-class accounting is 202 routed, six excluded and three open out of 211.
The remaining 1121 generated entries and all 988 MP choices are byte-for-byte
unchanged at the catalog-data level. Protocol 1050, APIs 5/5/11, MP feature 28,
JRW10 and SP save V37 are unchanged.

MP still needs the native Mouse/Remote/Seeker body path, wider controller and
presentation/combat parity, occupied-vehicle transport and real remote/late-join
acceptance. Those actors use original static MD3 meshes in SP, while MP's native
NPC loader requires Ghoul2. No replacement geometry or actor behavior is shipped
in this release. New Galak/legacy actor gameplay acceptance also remains open.
The overhaul goal remains active. Build/deployment evidence will be recorded
separately; source/catalog checks do not establish gameplay acceptance.

## Installed baseline before 0.97.0: 0.96.0 (2026-10-11)

Build `20261011-182321-593a0b22` installed at 18:24 EDT through scripts/build-jar.ps1.
All 104 automated suites and three native retail-resource fixtures passed,
including fourteen admitted retail NPC definitions. Independent readback verified
65 installed files, 14 replacement backups and 1847 archived source files.
ZIP SHA-256: `41B66C6CC0816990D46ADB6B959F8C67740A37A34E3DC7B4FF017E0B58EAA413`.
Receipt: `build/deploy/20261011-182443-cac169bf/receipt.json`.
Qualification: `build/galak-0.96.0/qualification.json`.
Both private mod folders contain the same qualified 156-file legacy asset pack.
Galak mech is directly available as an NPC and a model prop in SP/MP. Catalogs:
SP 1181 cards / 11 unavailable / 21 owner-excluded helpers; MP 988 choices.
The new native lifecycle/client tests and phase snapshot tests passed. Early
fixture runs caught missing engine-import stubs; asset checks found the missing
original small explosion and four clanking footstep sounds before packaging.
The goal remains active. New actor gameplay/rendering, actual remote clients,
late join, campaign saves and remaining catalog/SP-MP parity remain open.
These receipt notes were added after packaging; runtime sources still match the
archive. No graphical game was launched or owner process terminated.

## 0.96.0: Galak mech restoration (2026-10-11)

Implementation record; release installation evidence is recorded separately.
The Galak mech NPC and its original model-prop card are now directly available
in both menus. SP: 1181 cards, 11 unavailable, 21 owner-excluded helpers. Of the
original 182 unavailable entries, 151 are routed, 20 excluded and 11 remain open.
MP: 988 choices, 687 model props, 79 map-actor aliases, ten controllers and twenty
weapons. MP feature version is 28; protocol 1050, APIs 5/5/11, JRW10 and SP save
V37 are unchanged.

SP restores the original mech AI in a separate source file, preserving Academy's
vehicle-pilot behavior. The native pain dispatch, identity/class alias, initial
shield hull, muzzle bolts, corpse behavior and cleanup are connected. MP restores
previously disabled melee, shield regeneration, generator destruction and laser
animation branches. Both use the original animation frames and sounds. The
original Galak shield material is bound to its real shield geometry; shield,
antenna and death surfaces now use names present in the original mesh. Placement
checks include the full 60-unit-radius, 104-unit-tall shield hull.

The private legacy pack now has five families and 156 files, including the
original Galak model/skeleton/materials, missing effects, weapon/voice/footstep
sounds, 43 converted Galak sound events and model-local aliases for removed
animation names. No Academy stock asset/definition is replaced. Meshes and
skeletons remain source-identical. MP uses the same beam art, with a per-frame
trace because its FX engine lacks SP's entity-relative trace-line implementation.
Short bolt-follow muzzle effects follow replicated charge/firing state and expire
within 125ms of the last emission. Other NPC/vehicle surface-bit meanings are
preserved. Optional beam sound helpers fail safely at capacity, are released on
abort/death/removal, and clear their backlink when independently removed.

Three new native fixtures execute SP/MP shield, regeneration obstruction,
one-time generator destruction, charge/fire/end, single melee hit, capacity
failure, independent helper removal, stale-slot cleanup and corpse shutdown.
The client fixture checks model-specific surface deltas, clipped following beams,
phase/death shutdown, emission rate and unchanged vehicle surface handling.
Native snapshot tests round-trip the phase bits; retail resource tests admit the
actual Galak definition with conservative bounds and reject missing resources.
Offline asset qualification covers all LODs, skeleton indices, requested clips,
events, sounds, effects, surfaces, original hashes and derived effect provenance.

These checks are not rendered gameplay, real networking or campaign-save
acceptance. No graphical game is launched. Owner-accepted work remains accepted;
new Galak rendering/combat and remote-client/late-join/load behavior remain open.
The eleven remaining unavailable cards are test, yt-1300, six absent creature
aliases, misc_bsp, misc_cubemap and terrain. Full MP controller, presentation and
combat parity also remains open. The overhaul goal is still active.

## Installed baseline before 0.96.0: 0.95.0 (2026-10-11)

Build `20261011-163803-0266626e` installed at 16:39 EDT through scripts/build-jar.ps1.
All 101 automated suites and 3 native retail-resource fixtures passed. Independent
readback verified 65 installed files, 12 replacement backups and 1836 archived
sources. ZIP SHA-256: `406604966A988E1D6F03D495BD02B3EF6889C8A958CF0B76E02CE52F524FAC5D`.
Receipt: `build/deploy/20261011-163908-8f9cdac1/receipt.json`.
Qualification: `build/legacy-npcs-0.95.0/qualification.json`.
Both private mod folders contain the same qualified 66-file legacy NPC pack.
Four NPC families and four map aliases are restored, plus four prop models:
SP 1180 cards / 12 unavailable / 21 owner-excluded helpers; MP 986 choices.
The initial test run caught the old 75-alias expectation; updating it to 79
allowed the full native alias spawn/copy/save/Undo cases and all suites to pass.
The goal remains active. New actor rendering/combat, remote clients, late join,
load behavior and full remaining catalog/SP-MP parity are not claimed complete.
These receipt notes were added after packaging; runtime sources match the archive.
No graphical game was launched or owner process terminated.

## 0.95.0: Four restored legacy NPC families (2026-10-11)

Implementation record; build/install evidence is recorded separately. Glider,
Mark I, Mark II and Mine Monster now have their original visible bodies, skins,
textures, skeletons and missing sounds restored from the owner's local Jedi
Outcast installation. Eight existing NPC/map-spawner entries become available,
with four additional model-prop cards. SP: 1180 cards, 12 unavailable, 21 approved
helper exclusions; original 182: 150 routed, 20 excluded, 12 open. MP: 986 choices,
686 model props and 79 map-actor aliases; ten native controllers and 20 weapons.

The private jar-legacy-npcs.pk3 is built for both SP and MP. It contains 66 files
and a per-resource source/hash manifest. It replaces no Academy stock path,
NPC definition or shared humanoid asset. Build inputs require the owner's
Outcast installation (a sibling Steam directory by default); missing source
assets fail clearly instead of silently producing an incomplete release.

Models and skeletons are byte-identical to the source. Original animation frame
ranges/rates are retained, with native idle/sleep aliases and 12 converted sound
events. Glider uses its original wing animation as a looping neutral ambient
idle/movement clip; this is not new flight-path or combat AI. Mark I's unused
precache of an absent walk sound is removed in both engines; actual footsteps
use the original sound-event choices. Stock icons and stable IDs are retained.
Protocol 1050, APIs 5/5/11, MP feature 27, JRW10 and SP save V37 are unchanged.

Offline qualification checks original bytes, all mesh LOD triangles/vertices,
bone references, compressed animation frame indices, native animation names,
AI-requested clips/sounds/effects, event timing and missing-resource rejection.
It does not prove live rendered/combat/physics behavior or remote-client loading.
No game was launched. The owner-accepted areas remain accepted; these newly
restored actors and model props still need gameplay/visual acceptance.

Galak's mech remains open: original assets were found, but Academy's SP Galak
AI source was repurposed for vehicle pilots. Restoring the model alone would not
restore its intended behavior. The other remaining entries are test, yt-1300,
six absent creature aliases, misc_cubemap, terrain and misc_bsp. Full MP controller,
presentation/combat parity and remote/late-join/load validation remain open.

## Installed baseline before 0.95.0: 0.94.0 (2026-10-11)

Build `20261011-162034-f143ae4e` installed at 16:21 EDT through scripts/build-jar.ps1.
All 101 automated suites and 3 native retail-resource fixtures passed. Independent
readback verified 63 installed files, 10 replacement backups and 1833 archived
sources. ZIP SHA-256: `9E8C71AEB450B453BABAFC7DA61641BF81A5C9F6917A43517C00C4915B2AACE1`.
Receipt: `build/deploy/20261011-162134-a5b673ce/receipt.json`.
Qualification: `build/media-0.94.0/qualification.json`.
MP has 974 choices, including ten native controller/marker cards. Sound and FX
entities now support editable assets/playback, typed links, copies and JRW10 saves.
All ten picker assets are verified retail paths; readers retain JRW1-9.
The goal remains active: full catalog/controller/SP-MP parity and new gameplay,
audio/visual, remote-client, late-join and load acceptance are still open.
These receipt notes were added after packaging; runtime sources match the archive.
No graphical game was launched or owner process terminated.


## 0.94.0: MP native sound and effect entities (2026-10-11)

Package-time implementation record. Completed release qualification and installation
are recorded above.

The MP Entities tab adds target_speaker and fx_runner with their existing SP
stable IDs, stock crate models and stock icons: 974 total MP choices, including
ten controller/marker cards. Each movable body owns a private ET_SPEAKER or ET_FX
helper, so native playback fields never replace its collision/model state.
Helpers follow body movement/rotation, are excluded from saved object counts,
and are removed with their source, failed copies, group Undo and cleanup.

The C Controller page supports typed sound/effect paths, five stock defaults per
type, previous/next asset, playback mode, interval, random timing, Playing and
Enabled. Speaker modes are local, looping, global and activator-only. Local/global
sounds may repeat with at least 200 ms between events; zero is manual playback.
Effects repeat or fire once, with 200-60000 ms intervals and bounded random delay.
Link Primary aims an effect at a destination marker; Secondary activates a linked
controller on emission. Connections recheck permissions and reject cycles/types.

Paths are bounded, checked against actual server files and native configstring
reserves before registration. Missing assets/resources refuse the edit or spawn.
Settings have identity-bound editing and conflict-aware Undo. Copies/world saves
retain paths, modes, intervals, active state and remapped links. Playback timers
restart on copy/load; audio decoder position and individual particles are not saved.
JRW10 adds an active-state word (268-byte node); readers retain JRW1-9. Protocol
1050, APIs 5/5/11 and SP save V37 are unchanged; MP feature version is 27.

Use_Target_Speaker and fx_runner_use/think were extracted without body changes;
authored map callers retain those callbacks. Owned FX correct the native tenth
one-shot state so it cannot accidentally become continuous, and fire one owned
secondary output per emission. Damage-radius and bmodel sound-set properties are
not exposed by this route and remain specialized-controller parity work.

Focused native-callback/server/menu tests and all ten retail picker paths pass.
Evidence: build/media-0.94.0/native-audit.json and runtime.patch. No graphical game
was launched. Hearing/visual confirmation, remote clients, late join and load
performance remain untested. SP remains 1176 cards, 20 unavailable and 21 approved
helper exclusions; these two existing SP routes do not reduce those remaining 20.
Full catalog/controller/SP-MP parity is unfinished. The overall goal remains active.


## Previous installed release: 0.93.0 (2026-10-11)

Build `20261011-160420-ef85e0a2` installed at 16:05 EDT through scripts/build-jar.ps1.
All 101 automated suites and 3 native retail-resource fixtures passed. Independent
readback verified 63 installed files, 10 replacement backups and 1827 archived
sources. ZIP SHA-256: `02D5ED02BF52C96115EB261B6EFA2A9451B061060910D5708BCFFFF67BC16FC5`.
Receipt: `build/deploy/20261011-160518-ff25c19d/receipt.json`.
Qualification: `build/transport-0.93.0/qualification.json`.
MP has 972 catalog choices, including eight native controller/marker cards.
Push, Teleporter and destination helpers support typed links, context settings,
copies and JRW9 persistence; older JRW1-8 worlds remain readable.
Native vehicle/seat transport, full catalog and specialized-controller parity,
new graphical/gameplay acceptance and other-client/late-join/load tests remain open.
The overhaul goal stays active. These receipt notes were added after packaging;
runtime sources match the archived release. No graphical game was launched or
owner process terminated.


## 0.93.0: native MP push/teleporter catalog routes (2026-10-11)

Package-time implementation record. Completed release qualification and installation
are recorded above.

MP adds target_push, target_teleporter and misc_teleporter_dest with the existing
SP stable IDs, stock crate bodies and stock icons. Push/teleporter spawns include
a linked, movable destination helper in the same Undo group. Failed helper
placement removes the whole setup. Push initially uses a 600-unit/sec constant
direction toward its target_position; Teleporter uses misc_teleporter_dest.
The MP spawn menu now exposes 972 total catalog choices, including eight native
controller/marker cards; 682 ordinary props, 20 weapons and 75 actor aliases remain.

Link Primary connects either spatial source to a destination marker. Activation
links from Relay/Delay/Counter can call a Push or Teleporter. Typed ports reject
incorrect marker/action connections and secondary spatial outputs. Copies and
world saves remap these references, and deletion clears them before slot reuse.

Push has C properties for speed (50-4000), constant/arc mode, native jump sound
and enabled state, with validation and conflict-aware Undo. Arc mode requires a
linked destination above the source and positive gravity; impossible/excessive
trajectories are refused. Without a destination, source rotation sets the push
direction. Native push applies to the activator and retains native movement-mode
and sound timing semantics; collision/rendering bounds never become velocity data.

Teleport uses the linked marker's position/orientation, placing the activator
above its visible body. The full arrival hull must be clear before the native
TeleportPlayer routine runs. That routine is moved unchanged into a tested include:
stock events, outgoing velocity, angle, interpolation bit and NPC identity remain.
Actor health/readiness, player consent, current destination permissions, pending
copy state and entity reserve are checked. Exit vehicles and release held actors
first; native vehicle/seat transport is not implemented by this route. A delayed
activation cannot inherit a replacement client's identity. Already-authored map
push/teleport callbacks keep their original paths.

Destination markers have editable labels. Spatial sources, settings, helper
placements and connections survive assembly duplication and JRW9 saves. The new
264-byte record adds explicit push speed; readers retain JRW1-8. SP save V37,
protocol 1050 and APIs 5/5/11 are unchanged; MP feature version is 26.

Focused production fixtures pass for actual native push/teleport callbacks,
constant/arc motion, stock teleport state, NPC snapshots, bounds, consent changes,
obstructed arrivals, typed links, saved/copy reference remapping, deletion/reuse,
helper rollback and authored-map preservation. Native-body comparison evidence:
build/transport-0.93.0/native-audit.json. No graphical game launched; new rendered
gameplay and other-client networking acceptance remain open.

SP stays at 1176 cards, 20 unavailable and 21 owner-excluded helpers. These three
entries already worked in SP and now have MP routes; the 20 remaining original
blocked SP entries are unchanged. Complete catalog, specialized controllers,
other SP/MP parity and gameplay/performance acceptance remain open. Goal active.


## Previous installed release: 0.92.0 (2026-10-11)

Build `20261011-155319-f9181113` installed at 15:54 EDT through scripts/build-jar.ps1.
All 101 automated suites and 3 native retail-resource fixtures passed. Independent
readback verified 63 installed files, 10 replacement backups and 1822 archived
sources. ZIP SHA-256: `58E95A2785F1F4CAEBA6794ABA8D1C7087C49DCFCBF42C30D0DD183B1DA4E4C0`.
Receipt: `build/deploy/20261011-155418-b4ad92f1/receipt.json`.
Qualification: `build/context-0.92.0/qualification.json`.
SP-style MP C panel/actions, world-click selection and properties for the five
initial native controllers are installed, with JRW8 saves and JRW1-7 reads.
New graphical/gameplay acceptance and other-client networking remain open.
Full catalog/specialized-controller/SP-MP parity and the overall goal stay active.
These workspace receipt notes were added after packaging; runtime sources match
the archived release. No graphical game was launched or owner process terminated.


## 0.92.0: multiplayer context parity and controller properties (2026-10-11)

Package-time implementation record. Completed release qualification and installation
are recorded above.

MP C now uses the SP right-side panel and compact action grid. Panel/world/action
geometry and action labels live in a shared SP/MP header. Position and angles have
separate Apply buttons; Pose is a tab, using SP's zero-based frame display. Clicking
a field still pins the menu for typing; Enter applies numeric/controller values,
and Esc closes. Status/help stays at the top left. MP-specific self-release and
consent controls remain directly available. Script vars and specialized SP pages
are not falsely presented as functional MP controls; those ports remain open.

World clicks select from the rendered camera ray, including widescreen and reduced
viewports. The server checks finite normalized rays, a camera within 512 units of
the player eye, unobstructed eye-to-camera space, a 4096-unit trace and existing
entity permissions. Select player exposes self health/release without allowing
self Physgun control or deletion. Selection echoes and property identity tokens
continue to protect against slot reuse and revoked editing permission.

C has Freeze, Release, Activate, Remove, Copy, Paste, Rope, Thruster, Weld, Hinge
and Unlink actions. They reuse server tool controllers without changing the chosen
Toolgun mode. Context attachments use entity origins and a vertical axis; aimed
face placement remains available through the Toolgun. Switching link types clears
incompatible first anchors. Prop gravity/mass, native health and pickup quantities
remain editable. Undo restores a valid property edit; Pose has its own Undo.

The five existing MP controller cards now expose useful native properties:
- Print: 1-63 character message and everyone/team/activator audience.
- Delay: 50-60000 ms delay with bounded random variation, restart/ignore pending
  input and enabled state. Settings affect future uses, not an already scheduled timer.
- Counter: 1-1000 required activations, remaining progress/reset, -1 to 1000 rearms
  (-1 repeats indefinitely) and enabled state.
- Relay: everyone/red/blue input filtering and enabled state.
- Position: editable marker label. Specialized destination consumers remain open.

Controller edits are validated, replicated, conflict-aware and undoable. New
activations invalidate obsolete Undo. Literal message transport rejects command
delimiters and control characters; fixed entity-owned storage avoids repeated
string allocations. Copy/assembly paste and world saves keep all edited settings,
current counter/rearm state and pending delay. JRW8 uses 260-byte object records;
JRW1-7 still load. JRW7 controllers receive their original fixed settings.
Malformed state is rejected before spawn allocation. SP save V37, protocol 1050
and APIs 5/5/11 are unchanged. MP feature version is 25.

Focused production fixtures pass for edits, native callbacks, bounded text/time/
count input, stale identities, ownership, Undo conflicts, clipboard isolation,
JRW8 round trips, JRW7 controller defaults, older-world motion compatibility,
cursor rays, self-release and compact input routing. No graphical game launched.
Fresh visual/gameplay acceptance and other-client networking remain open.

Catalog totals remain SP 1176 cards/20 unavailable/21 owner-excluded helpers and
MP 969 cards (682 ordinary props, five controllers, 20 weapons). This release
improves usability of existing MP entries; it does not close complete catalog,
specialized native-controller/property or overall SP/MP parity requirements.
All previous owner acceptance is retained and the overall goal remains active.


## Previous installed release: 0.91.0 (2026-10-11)

Build `20261011-153408-fefd1e56` installed at 15:35 EDT. The normal release workflow passed
all 101 automated suites and 3 native retail-resource fixtures. Independent
readback verified 63 installed files, 14 replacement backups and 1817 archived
sources. ZIP SHA-256: `EC51F5D962AFA14DCA2092A1F34E148B1C727F25D53D3A70902016E3B436D244`.
Receipt: `build/deploy/20261011-153515-a63d43e7/receipt.json`.
Qualification: `build/feedback-0.91.0/qualification.json`.
The feedback fixes below are installed. No graphical game was launched; fresh
visual/gameplay acceptance and other-client networking remain open. Previously
accepted behavior stays accepted. Full MP parity and the overall goal remain
active. These receipt notes are workspace-only, added after ZIP qualification.


## 0.91.0: owner feedback fixes (2026-10-11)

Package-time implementation record; completed release qualification and
installation are recorded above.

- Clicking a search/context/pose field pins Q/C open in both modes. Release the
  held key to type; Enter/Apply retains the menu, Esc closes it. Context fields
  now capture movement, attack and view input as search fields do. Ordinary held
  browsing still closes on release. SP's empty script slots are explicitly labeled
  Script vars; Properties continues to show applicable position/angle/health etc.
- Vertical Physgun USE rotation is inverted in SP and MP, including rigid props,
  native actors and vehicles. Wheel direction and accepted beams stay as accepted.
- Shared rope rendering solves its requested arc length and sweeps interior
  points against terrain instead of letting slack sink below the floor. This is
  cosmetic collision for the displayed cable; the endpoint physics remains the
  existing length constraint, not full simulated rope wrapping around obstacles.
- Thruster force is reversed in every SP/MP backend, including saved mounts.
  It pushes into the mounted surface, so an underside mount pushes up. Both modes
  render an unedited stock battery housing and outward exhaust marker. MP uses a
  cosmetic replicated helper; deletion, off-state, ownership and world-save
  exclusion are checked. New models/icons are not authored.
- Undo gives stock UI sound and top-left status feedback, including MP property
  and pose undo. Server confirmation controls MP success feedback.
- The remaining saber-contact ScreenFlash primitive is filtered in both effect
  parsers, including appearance presets. Sparks, smoke and other effects remain.
- Retail nullDriver uses unprefixed class `kyle`; both native parsers now accept
  that alias. Exact retail admission passes with its Kyle body, neutral teams and
  no explicit weapon. This is a driver helper template, not a distinct character.
- Host defaults: everyone can edit spawned objects, departing objects persist,
  saved-world editing and automatic restore/save are on. Host creation offers
  owned-only editing and a Keep world across sessions master toggle. Off disables
  disconnect retention and automatic save/restore without deleting old save files.
  Existing host-dialog defaults migrate once; subsequent selections persist.
  Dedicated example config uses the same defaults; existing private server.cfg
  choices are preserved. Persistence restores the last saved snapshot; orderly
  shutdown saves, abrupt process termination can lose changes since autosave.
- MP native ClientSpawn resets health/loadout. Sandbox's existing respawn hooks
  retain spawned objects and tool mode/settings and regrant Physgun/Toolgun.
  Added fixtures check this hook boundary; they do not claim a live death test.

Focused production tests pass. Save formats SP V37 and MP JRW7, protocol 1050,
APIs 5/5/11 and catalog counts remain unchanged. MP feature version is 24.
No graphical game launched. These new fixes need owner gameplay/visual checks;
other-client networking remains untested. Full specialized MP controller/context
and catalog parity remains open; overall goal remains active.


## Previous installed release: 0.90.0 (2026-10-11)

Build `20261011-151628-97f54e12` installed at 15:17 EDT. All 101 automated suites and
3 retail resource fixtures passed. Independent readback verified 63 installed
files, 10 replacement backups and 1813 archived sources. ZIP SHA-256:
`4E9762DEF02B4209229F4744712072B8011F73E0DD052CD3A54BE61ECF8CB110`. Evidence: `build/controllers-0.90.0/qualification.json`.
MP Activate/Link supports five initial native controller cards; specialized
controllers and full context parity remain open. JRW7 reads versions 1-6.
No graphical game was launched; goal remains active.

## Owner feedback received 2026-10-11: next revision

Accepted in the owner's tests: LAN textures/HUD, held-Q opening, SP/MP menu
appearance, weapon equip/drop, tool models/beams/sounds, wheel/flinging, LAN
physics, duplication, named NPCs and Noghri attacks. Other-client networking,
freeze and individual tool modes remain untested. Prior acceptance is retained.

Requested fixes: Q/C typing without continuing to hold the key; invert vertical
Physgun rotation in SP/MP; MP ropes too long/below ground; reverse or provide a
reverse option for thrusters and give them a model; Undo UI sound and popup;
useful context properties instead of unexplained Parameters 1-16; remove saber
weapon/destructible contact screen flash in both modes. Nulldriver spawn fails;
investigate its intended role without silently excluding it. Server creation
should default to everyone editing everyone's objects, with owned-only option;
persistence across disconnect/restart should default on with a creation toggle.
Death should reset ordinary inventory/health while retaining Physgun, Toolgun,
tool selections and spawned objects. These are pending implementation/testing.


## 0.90.0: MP Activate/Link and native target controllers (2026-10-11)

Implementation prepared; normal release build/test/package/install is pending.
Latest verified installation remains0.89.0 until a new receipt is recorded.

MP now has functional Activate and Link tool modes. Activate calls the native use
callback after existing ownership/consent checks; alternate inspects. Link chooses
a spawned Relay, Delay or Counter and connects its primary/secondary output to a
spawned activation controller. Alternate cancels. Connections recheck permissions
when fired, refuse cycles and never rewrite authored-map target wiring. Native
activation recursion is guarded at64 calls; larger/specialized graph semantics
remain subject to controller coverage, not a claim of complete SP parity.

Five SP catalog entries now have MP cards with the same stable IDs and stock crate
models: target_print, target_relay, target_delay, target_counter, target_position.
The first four use the actual native callbacks, moved unchanged from g_target.c
into a directly tested include. The target dispatcher adds an owned-controller
route; authored map dispatch remains intact. Relay/Delay/Counter menu spawns include
a connected Print helper and share one Undo group. Position is a movable destination
marker; aim-based source controllers are still to be ported. These crate-backed
controllers support Physgun/USE rotation, freezing, physics, attachments and
existing body properties through the existing prop adapter.

Logical connections join copied assemblies in either direction. Duplication and
world saves remap links to the new instances, preserve counter progress/inactive
state and pending delay time, reject malformed graphs transactionally and clear
incoming references on deletion before slot reuse. Delayed callbacks cannot reuse
a disconnected player's identity. Unlink removes logical links as well as physical
attachments. World writer advances toJRW7; readers retainJRW1-6. Feature23;
protocol1050, APIs5/5/11 and SPsaveV37 remain unchanged.

MP catalog is969 cards:682 ordinary props, five controller bodies,20 native weapons
and the other existing choices;75 map NPC aliases retained. SP remains1176cards,
20unavailable and21 owner-excluded helpers. No exclusions or SP gameplay changes.

Focused production-code tests pass for native callbacks/dispatch, menu selection,
primary/secondary links, timing, cycle/ownership rejection, deletion/reuse,
assembly copy/Undo, JRW7 state and prior-world compatibility. The native callback
comparison is recorded in build/controllers-0.90.0/native-callback-audit.json.
Full release qualification is pending; no graphical game launched.

Still OPEN: specialized SP controller and complete catalog parity, Link semantics
for aim/route/other controller families, controller-specific C properties and text,
full context presentation/properties, plus remaining SP/MP functionality and
human gameplay/visual acceptance. Activate/Link are implemented for the stated
coverage, not blanket completion of every controller. All32 owner-accepted SP
areas remain accepted. Overall goal remains active.


## Previous installed release: 0.89.0 (2026-10-11)

Build `20261011-145842-e8e73d76` installed at15:00EDT from the exact ZIP after
owner confirmed JA:R closed. The first deployment attempt safely refused the
running game; no process was terminated. Normal build and all101 suites passed,
plus3 production retail-resource fixtures. Independent readback verified63
installed files,11 replacement backups and1808 archived source files before
these workspace-only receipt notes. ZIP SHA-256:
`E501D7A08E3221A5602BDD25EA4C100E1B84D6B4FFF685A0A7EF305EF176E4A8`.
Receipt: `build/deploy/20261011-150039-90914382/receipt.json`.
Qualification: `build/menu-0.89.0/qualification.json`.
Sources/symbols: `build/symbols/20261011-145842-e8e73d76.zip`.

Shared SP/MP Q layout, font/colors, card/search helpers and tool descriptions
are installed. MP adds Select, Rotate, Gravity and Unlink; Duplicate primary
copies first, Remove alternate undoes, thruster defaults/steps match SP, and
tool selection/feedback use server acknowledgements. No catalog or save-format
changes. Feature22, protocol1050, APIs5/5/11, SPsaveV37/worldJRW6.

No graphical game launched. Gameplay/visual acceptance remains open. The full
MP clone is unfinished: Activate/Link, context presentation/property coverage,
and remaining SP native controllers/catalog/functionality still require work.
All32 previously accepted SP areas remain accepted; overall goal stays active.
The installed documentation is the captured pre-install record; this workspace
receipt block was added after packaging and does not alter the verified ZIP.


## 0.89.0: shared Sandbox menu and more MP tool parity (2026-10-11)

Package-time implementation record; the completed build, qualification and
installation are recorded above.

SP and MP now consume `shared/qcommon/jar_sandbox_menu.h`: the same five tabs,
12-card layout, stock Arial Narrow font, panel/card/status colors, vertical tool
list, right-side properties, tool names and primary/alternate descriptions.
MP has case-insensitive search (95 characters), per-tab pages and alphabetical
catalog browsing. SP's existing card order and backend scripts remain intact.
Search remains open after Q is released while typing; Enter finishes editing,
Escape closes, and movement/attack input is suppressed during text entry without
clearing held movement keys. Weapon left-equip/right-ground clicks are preserved.

MP Select, +/-15-degree Rotate, Gravity and Unlink now use existing validated
entity controllers. Duplicate primary fire captures when the clipboard is empty;
Remove alternate fire undoes the last owned creation even when aimed at world.
Tool selection waits for server acknowledgement and uses the native weapon-grant
restrictions. Tool aim range matches SP at4096. New thrusters default to5000N and
adjust by1000N, matching SP; existing saved mounts and JRW1/2 compatibility defaults
retain their recorded values. Server feedback appears in the fixed top-left panel.

Concrete MP difference: Box3D affects the shared world, so its Q control reports
server state and explains that the host controls it. Ownership, consent, entity
identity and collision validation remain authoritative on the server. These are
not reasons for cosmetic differences or blanket refusals to port functionality.

Still OPEN: Activate and Link entity controllers (their shared rows remain visible
and identify the unfinished port when clicked), full context-menu presentation/
property coverage, native entity/controller and catalog coverage, and remaining
SP/MP functionality parity. The new tools operate on supported props/actors;
unsupported native transform controllers remain explicit work. This release does
not claim the whole MP clone is finished. Map-instance controller work remains open.

Focused production-code fixtures pass for SP and MP menu construction/input, all
964 MP card actions, search lifecycle and movement suppression, ownership/consent,
rotation collision refusal, tool selection, Gravity, Unlink and Duplicate/Undo.
Full release qualification is recorded above. No graphical gameplay was launched; actual
menu appearance and networked gameplay still require acceptance. All32 previously
accepted SP areas remain accepted. Goal remains active.

SP1176 cards/20 unavailable/21 owner-excluded helpers; MP964 cards/682props/20weapons.
No catalog changes. MP feature22; protocol1050, APIs5/5/11, SPsaveV37 and worldJRW6
remain unchanged. Deploy matching client/server binaries for the new tool echoes.


## Previous installed release: 0.88.0 (2026-10-11)

Build `20261011-143937-303994e9` installed at 14:40 EDT through `scripts/build-jar.ps1`.
**101/101 automated suites passed**, plus three production resource fixtures
using exact retail definitions. Independent readback verified **63 installed
files**, **16 replacement backups** and **1803 archived source files** before
these workspace receipt notes. ZIP SHA-256:
`35B7C9B06334794C9E352E94A69F31E29F0CDCDDA3C10FF34780BA949289CBCF`.
Receipt: `build/deploy/20261011-144052-32dcb4d8/receipt.json`.
Qualification: `build/weapons-0.88.0/qualification.json`.
Source/symbols: `build/symbols/20261011-143937-303994e9.zip`.

Weapons cards now left-click to grant/select and right-click to spawn. MP has
all 20 native weapon cards. Its tool models, stock beam effects, selection/firing
sounds and Physgun freeze/release/wheel behavior now follow SP. The separate
owner-approved empty no-weapon GLM is hidden; the previous twenty exclusions are
preserved. SP1176 cards/20 unavailable; MP964 choices/682 props; MP feature21.
The installed LAN hosting repair and previous NPC/weapon resource repairs remain
intact. Protocol1050, APIs5/5/11, SP saveV37 and worldJRW6 are unchanged.

No graphical game was launched. Weapon hands/pickups, effects and multiplayer
freeze/release still await owner gameplay checks. The explicit MP requirement
is a clone of SP except concrete engine/network constraints; full menu/tool/
controller parity is NOT complete. Next work: share the Q menu layout and tool
properties/actions, then context/controller coverage. Map-instance investigation
remains open. All32 accepted SP areas remain accepted; broader goal active.


## 0.88.0: weapon card actions and initial SP/MP tool parity (2026-10-11)

Owner instruction: multiplayer should clone SP menus, controls, assets and
functionality unless a specific engine/network constraint prevents it. Shared
presentation is the default; server authority, replication, ownership and player
consent are adapters, not reasons for different menus or effects. Full parity
is still OPEN. Next: consolidate the Q menu layout/properties and port missing
SP tool/context actions and native controllers; document each real exception.
The earlier map-instance investigation remains unfinished, with no BSP controller
changes included in this release.

Weapons-tab left click now grants and selects the weapon directly. Right click
creates the native world pickup. SP shortblade, pike and Sith sword use distinct
stable IDs (2101-2103), native saber pickups and preserved variant identity for
ownership, saves and duplication. Direct grants include ammo, interrupt ordinary
weapon cooldown and release held Physgun objects, while preserving death,
scripted control, saber-in-flight and locked-action protections. MP validates
catalog/type, server weapon/item restrictions, living unheld player state and
rate limits; it confirms selection to the requesting client only. Other tabs
retain ordinary spawning. Multiplayer now includes all 20 native weapon cards,
including the 16 omitted by the previous catalog derivation.

MP now uses SP's Concussion Physgun and blaster-pistol Toolgun models, selection
sounds and bundled stock-icon aliases. Both builds consume shared beam shader,
width, color and Toolgun sound constants. MP uses the same stock blue/red line
particles rather than plain white geometry, with client muzzle calculation.
MP wheel increments/range match SP (24 units, 48-2048). Right click freezes a held
object; after releasing both triggers, right click on an aimed object releases
it. The beam clears on release. Ownership and consent checks remain enforced.
This is initial parity work, not a claim that all MP tools or menus match SP.

The separately approved empty `models/weapons2/noweap/noweap.glm` card is hidden.
Geometry checks still verify its lack of default-visible triangles. The twenty
previous helper exclusions are unchanged. SP now has 1176 cards / 20 unavailable;
21 helpers are owner-excluded overall. The original 182 backlog remains 142
routed / 20 excluded / 20 open (the separate GLM was not in that original set).
MP now has 964 choices / 682 props. SP save V37, MP protocol 1050, APIs 5/5/11 and
world JRW6 remain unchanged; MP feature version is 21. Update server and client
together for the new equip request and selection acknowledgement.

Production fixtures cover card gestures, all MP cards, direct grants/refusals,
client acknowledgement, melee pickup variants, save IDs, beam parameters,
freeze/release latch and wheel bounds. Retail checks cover the approved empty
helper, tool models/hands, sounds and native weapon catalog. Full build/package/
installation evidence is recorded after the release workflow. No graphical game
is launched; owner checks for weapon hands/pickups, effects and freeze controls
remain open. All 32 previously accepted SP areas remain accepted. Goal active.


## Latest installed release: 0.87.0 (2026-10-11)

Build `20261011-141651-ad216b8e` installed at14:18 EDT through `scripts/build-jar.ps1`.
**100/100 automated suites passed**, plus three production resource fixtures
using exact retail NPC/item/weapon definitions. Independent readback verified
**63 installed files**, **10 replacement backups** and **1798 archived source
files** before these workspace receipt notes. ZIP SHA-256:
`44BE994D140D00C216F437369B0940BA89CF4E8C459FA33C2DA3FE3E6F5A906C`.
Receipt: `build/deploy/20261011-141813-fe42420a/receipt.json`.
Qualification: `build/resources-0.87.0/qualification.json`.
Source/symbols: `build/symbols/20261011-141651-ad216b8e.zip`.

Remote now uses the available stock droid body while retaining its native
Seeker identity; class-based initialization supplies flight and30-shot ammo
for its neutral allegiance. Jawa pickup and equipped models use stock Bryar
art when the original is absent, preserving Jawa firing/ammo behavior. Custom
resources retain precedence. Exact retail admissions include Desann, Mon Mothma,
RebornBoss, Morgan Katarn, Mouse, remote_sp, Remote and Seeker; this is automated
resource evidence, not observed gameplay. No game was launched.

SP: **1177 cards /21 unavailable**, original182: **142 routed /20 excluded /20
open**, entity routes198/211 and111 presets. Only Remote and Jawa pickup catalog
records changed. MP remains948 choices/682 props; all MP binaries and root
launchers match0.86, retaining its LAN hosting texture repair. Save/network
formats are unchanged. Owner confirmation of Remote combat, Jawa pickup/equip,
MP hosting/Q/C and other open acceptance remains outstanding. Preserve all32
accepted SP areas. Goal active for remaining catalog/parity work and acceptance.

## 0.87.0: Remote and Jawa weapon resource repairs (2026-10-11)

Two further catalog entries now have usable native resource paths. `remote`
uses the shipped `remote_sp` MD3 body and animation when its requested stock GLM
is absent. Native precaching, spawn setup and loaded-definition admission agree
on that fallback. It retains the original CLASS_SEEKER, TEAM_FREE allegiance,
weapon and statistics. Seeker gravity and 30-shot ammunition initialization now
applies by class, also covering neutral Remote; the existing friendly Seeker
setup is preserved. Native following/combat and finite ammo/lifetime semantics
remain unchanged. Original GLMs, custom model paths and nondefault skins retain
precedence. Campaign randomizer eligibility is unchanged.

`weapon_jawa` now uses the shipped Bryar pistol world/view/hand models when its
missing original gun model is requested. Native Jawa identity, ammunition cost,
firing interval, projectile behavior and pickup quantity remain intact. Empty
weapon icons and selection sound receive matching stock resources; custom values
are preserved. This is stock substitute art, not a recovered original Jawa gun.
Both the pickup and equipped model are repaired, avoiding an equip-time missing
model failure. No custom art was created.

Production regressions cover loaded NPC admission, animation cache exhaustion,
model-registration failure, exact-name/default-skin gates, original resource
priority, missing replacement assets, class/team/weapon preservation, Seeker
initialization and saved ammo/flags. Weapon tests run the native parser and
actual player/NPC projectile function, including allocation failure; unrelated
weapon definitions compare unchanged. The release workflow now also runs three
native resource fixtures against exact installed retail NPC/item/weapon data.
Retail audits cover visible geometry, attachment tags, animation ranges, stock
materials/icons, selection sound and projectile effects.

SP remains **1177 cards**, now **21 unavailable**; original182: **142 routed /
20 owner-excluded /20 open**. Entity routes198/211 and111 portable presets are
unchanged. Only Remote and Jawa pickup catalog records changed from0.86; stable
IDs and all twenty approved helper exclusions are preserved. MP catalog remains
948 choices/682 props; these SP-only legacy actor/weapon cases are not claimed
as MP ports. SP saveV37 and MP protocol1050/feature20/APIs5/5/11/JRW6 stay unchanged.

No game was launched. Actual Remote hovering/combat, Jawa pickup/equip/rendering,
full saves and prior owner checks remain open, including the installed0.86 MP
LAN texture repair. Preserve all32 accepted SP areas. The broader goal remains
active for21 catalog gaps, multiplayer parity and outstanding gameplay acceptance.
Build/install evidence is recorded after the release workflow.

## Latest installed release: 0.86.0 (2026-10-11)

Build `20261011-140257-3caa9c01` installed at 14:04 EDT using `scripts/build-jar.ps1`.
**99/99 automated suites passed**. Independent readback verified **63 installed
files**, **13 replacement backups** and **1794 archived source files** before
these workspace receipt notes. ZIP SHA-256:
`02A999A0A67F6D7B63F669F5F461DC0FC24F3BA4077CF4FFB633AFC199A2A855`.
Receipt: `build/deploy/20261011-140408-e227baf6/receipt.json`.
Qualification: `build/hosting-0.86.0/qualification.json`.
Source/symbols: `build/symbols/20261011-140257-3caa9c01.zip`.

The LAN type-1 missing-texture failure is repaired in the graphical MP client
and host menu. Renderer imports use actual engine mode; old saved dedicated
flags no longer suppress images. Host JA:R Sandbox now offers host-and-play LAN
hosting. Dedicated LAN/public modes and launchers remain unchanged. Reopen MP
to load the repaired binaries. Production image/menu regressions pass; owner
confirmation of rendered maps, HUD and Q/C remains open. No game was launched.

The already-started SP NS turret repair is included. Only its catalog record
changed: SP1177 cards/23 unavailable, original182:140 routed/20 excluded/22 open,
198/211 entity routes and111 presets. MP948 choices/682 props and all save/network
formats remain unchanged. Native turret and retail-resource checks pass; actual
combat remains an owner check. Preserve all32 accepted SP areas. The overall
goal stays active for remaining catalog/parity work and gameplay validation.

## 0.86.0: multiplayer hosting texture repair (2026-10-11)

The reported LAN type-1 failure was traced to a mode mismatch: the graphical MP
executable stays a client using `_dedicated=0`, but the host menu set the public
`dedicated=1` cvar. The renderer then refused every requested image, including
map textures, fonts and HUD art. Renderer integer imports now report the actual
engine mode for `dedicated`, including after loading an old profile. Other
renderer cvars pass through unchanged. The standalone dedicated renderer import
and LAN/public server modes are unchanged.

**Host JA:R Sandbox now hosts a playable LAN server.** Its misleading numeric
server-type field is replaced by “Host and play (LAN server)”. Valid Start clears
the stale public dedicated flag; failed validation still changes no settings.
Standalone servers use `Host-JA-R-Server.cmd`; append `+set dedicated 1` for LAN
only (the launcher defaults to public mode 2). Close and reopen the MP executable
after installation so the repaired client and renderer are loaded.

The production image-lookup regression reproduces the old failure, then verifies
world/HUD/font image loading with legacy flags 0/1/2, cache and missing-image
behavior, and unchanged dedicated suppression. Native menu tests cover old
saved choices and host commands. These are automated checks without a graphical
session; rendered maps and hosting still require owner confirmation.

This release also completes the already-started SP `misc_ns_turret` repair:
when its original Nar Shaddaa model is absent, the stock Imperial mine cannon
and its muzzle attachment are used with native targeting/firing/death behavior.
Original resources retain precedence. Initialization failures roll back cleanly;
portable fire spacing is 200 ms. Production spawn, firing, failure and saved-field
fixtures and retail model/rig/attachment/sound/effect audits cover the repair.

SP catalog: **1177 cards / 23 unavailable**, original 182: **140 routed / 20
owner-excluded / 22 open**, entity-class routes **198/211**, portable presets111.
Only the NS turret catalog record changed from0.85; all stable IDs and the twenty
approved helper exclusions remain. MP catalog remains948 choices/682 props.
SP saveV37 and MP protocol1050/feature20/APIs5/5/11/JRW6 are unchanged. No game
was launched; prior owner checks and remaining parity work stay open. Preserve
the32 owner-accepted SP areas. Build/install evidence follows the release workflow.

## Latest installed release: 0.85.0 (2026-10-11)

Post-package receipt: build `20261011-134629-a20a79e0` installed at 13:47 EDT via
`scripts/build-jar.ps1`. **97/97 automated suites passed**. Independent readback
verified **63 installed files**, **10 backups** and **1787 archived source files**
before these workspace notes. ZIP SHA-256:
`77BFD00FACB00BCC223116D350C2D47A1A41BA3FD16E55E7A929EDEAEA8032E4`.
Receipt: `build/deploy/20261011-134746-150840e6/receipt.json`.
Qualification: `build/seeker-0.85.0/qualification.json`.
Source/symbols: `build/symbols/20261011-134629-a20a79e0.zip`.

Seeker and NPC_Droid_Seeker now use the shipped remote_sp body when the original
remote MD3/MDR is absent, retaining the native Seeker definition, allegiance and
AI. Custom/original resources retain precedence. Only those two catalog records
changed; all stable IDs and the twenty owner-approved helper exclusions remain.
SP: **1177 cards / 24 unavailable**; original182: **139 routed /20 excluded /23
open**; entity routes197/211, portable presets110. MP remains948 choices/682props;
all MP binaries and root launchers match0.84. SP saveV37 and MP protocol1050,
feature20/APIs5/5/11/JRW6 are unchanged.

No game was launched. Seeker hovering/combat, MP Q/C, rails/riders/save, pickup,
Morgan/Noghri and other outstanding owner checks remain open. Preserve the32
owner-accepted SP areas. Remote is still unavailable and needs its own body/AI
repair. The overall goal remains active; remaining work includes24 catalog gaps
and multiplayer parity. Earlier helper-exclusion questions are resolved.

## 0.85.0: retail Seeker body repair (2026-10-11)

Seeker and NPC_Droid_Seeker now resolve their missing retail remote/lower.md3
body to the shipped remote_sp/lower.md3 and its animation file before native
precache/spawn registration. This retains CLASS_SEEKER, player allegiance,
weapon, movement and native lifetime behavior (including its limited ammo).
It does not replace the NPC definition. Existing original MD3/MDR bodies and
custom model/skin names retain precedence. Missing replacement resources fail
normally; campaign randomizer eligibility is unchanged. The unused missing
Seeker fire.wav precache was removed; firing still uses the native blaster FX.

Catalog: **1177 cards / 24 unavailable**, **139 of182 originally blocked routes**,
**20 owner-excluded / 23 original entries open**, **197/211 entity-class routes**,
110 portable presets. The twenty approved tag-only helpers stay hidden. Remote
remains unavailable: its separate legacy playerModel and AI setup need work.
MP still omits segmented MD3 actors; this does not claim multiplayer parity.
SP V37 and MP protocol1050/feature20/APIs5/5/11/JRW6 are unchanged.

Extended production resource/catalog fixtures exercise original-model priority,
MD3/MDR selection, missing replacement model/animation, exact-name matching,
custom paths, class/team/weapon preservation and unchanged campaign admission.
Retail asset checks cover visible body geometry, animation frames, textures,
Seeker hiss/blaster effects and the entity alias. Build/install evidence follows
only after the release workflow. No gameplay validation is claimed; Seeker
hovering/combat, prior rail/Q/C and other owner checks remain open.

## Latest installed release: 0.84.0 (2026-10-11)

Post-package workspace receipt note: build `20261011-133606-d7f9338d` installed at
13:37 EDT through `scripts/build-jar.ps1`. **97/97 automated suites passed**.
Independent readback verified **63 installed files**, **10 changed-file backups**
and **1785 archived source files** before these notes. One new audit JSON was
installed. ZIP SHA-256:
`B8C622F1FBAD0D49E4A7F5AD375A9E26EBD9526AE4A2A53E17F8246E4906E97C`.
Receipt: `build/deploy/20261011-133722-41c18add/receipt.json`.
Qualification: `build/catalog-exclusions-0.84.0/qualification.json`.
Symbols/source: `build/symbols/20261011-133606-d7f9338d.zip`.

The owner-approved twenty tag-only MD3 helper cards are hidden and excluded from
the remaining requirement. Their exact names and decision are retained in
`docs/JAR-catalog-exclusions.json` and the shipped catalog's `ownerExcluded` audit.
All other 1124 generated records and IDs match 0.83; actual weapons and visible
models remain available. The separate empty no-weapon GLM stays unresolved.
SP catalog: **1177 cards / 26 unavailable**; original 182 blocked entries:
**137 routed / 20 owner-excluded / 25 open**. Entity routes196/211, presets110.
MP remains948 choices/682 props, with all MP binaries and root launchers unchanged.
SP V37, MP protocol1050/feature20/APIs5/5/11 and JRW6 reads1-6 are unchanged.

0.83 rail assemblies and the 0.81 multiplayer Q/C fix remain included. No game
was launched. Real rail/rider/save behavior, Q/C, pickup, Morgan/Noghri and other
outstanding owner checks remain open; preserve all 32 accepted SP areas. The
overall goal remains active. Next implementation remains the 26 other catalog
gaps and multiplayer parity. The earlier pending helper-disposition question is
resolved; do not ask it again or restore those twenty menu cards.

## 0.84.0: approved helper-card exclusions (2026-10-11)

The owner explicitly approved excluding the twenty audited, tag-only MD3
attachment helpers from the everything-spawnable requirement and hiding their
cards. They are now omitted from the generated SP menu catalog. Visible weapon
models and actual weapon choices remain available; no game asset was removed.
Every remaining catalog record and stable ID matches 0.83. The separate empty
`noweap.glm` entry remains listed and unresolved; this decision covers only the
specified twenty MD3s.

The exact allowlist and owner decision are recorded in
`docs/JAR-catalog-exclusions.json`; shipped `docs/JAR-spawn-catalog.json` retains
an `ownerExcluded` audit with IDs, hashes, tag counts and zero-surface evidence.
Regeneration checks that every approved helper still has tags and no surfaces.
Coverage checks account for all retail models, including the approved exclusions.

SP menu: **1,177 cards / 26 unavailable**. Original blocked cohort: **137 routed,
20 owner-excluded, 25 still open**, out of182. Entity routes196/211 and portable
presets110 are unchanged. MP remains948 choices/682 props because these unusable
helpers were already omitted from its catalog. SP V37, MP protocol1050/feature20,
APIs5/5/11 and JRW6 reads1-6 remain unchanged. Rail assemblies and the multiplayer
Q/C repair remain included; outstanding gameplay acceptance and MP parity remain
open. The 32 accepted SP areas stay closed. Overall goal remains active.

## Previous installed release: 0.83.0 (2026-10-11)

Post-package workspace receipt note: build `20261011-133055-75ac3f72` installed at
13:31 EDT through `scripts/build-jar.ps1`. **97/97 automated suites passed**.
Independent readback verified **62 installed files**, **4 final-install backups**
and **1784 archived sources** before these notes. Prior 0.83 build ZIPs and
receipts are preserved in `build/rails-0.83.0/` and `build/deploy/`.
ZIP SHA-256: `DFE7B42166E18A1654FD049E737BFEF95A7BE5D2442D0C561B136F83BE7388C2`.
Receipt: `build/deploy/20261011-133154-ed252fdd/receipt.json`.
Qualification: `build/rails-0.83.0/qualification.json`.
Symbols/source: `build/symbols/20261011-133055-75ac3f72.zip`.

Rail track/lane/mover assemblies, Rail properties, native wall/clock checks,
property Undo, copied links and V37 native saves are implemented. Catalog:
1197 cards / 46 unavailable / 196 of 211 entity classes / 110 presets /
137 of the original 182 unavailable entries routed. MP binaries and root
launchers are byte-identical to 0.82; the 0.81 Q/C fix remains included.
No game launched. Rail, Q/C, pickup and other outstanding gameplay acceptance
remain open; preserve the 32 accepted SP areas. Overall goal remains active.

The owner has now explicitly approved excluding the 20 audited tag-only MD3
helpers and hiding their menu cards. This replaces the earlier pending question;
0.83 still lists them. Implement that exact disposition next, preserving visible
weapon models and the separate no-weapon GLM case.

## 0.83.0: portable rail assemblies (2026-10-11)

`rail_track`, `rail_lane` and `rail_mover` now each spawn a complete, owned
three-part assembly in the SP Sandbox. Track/lane markers use a small stock
crate; the solid mover uses the same stock model and native pusher collision.
Move or rotate the track to set direction; move the lane sideways to choose its
columns. Hold C and select **Rail** to edit length, width, whole grid spacing,
speed, per-row count, delay and traffic. Movers also offer centered/random-column
placement. Activate toggles traffic; Link connects a mover to a lane/track or a
lane to a track. Selection outlines the route. Existing map-authored rails retain
their native registry and behavior.

Trips use native mover trajectories and a swept hull wall check. Blocked paths
wait without advancing the trip clock; blocked starting hulls wait; riders prevent
recycling at the finish. Disabling or holding a track/lane pauses dependent trips;
holding a mover leaves it under Physgun control. Active trips retain their endpoint;
new trips use edited track properties. Copies preserve properties and remapped
assembly links but restart traffic; scalar Undo and native saved fields are covered.
Full SP saves write **V37** for the appended rail callback and read V1-V36.

Catalog: **1,197 cards / 46 unavailable / 196 of 211 entity-class routes /
110 portable presets / 137 of the original 182 unavailable entries routed**.
MP remains **948 choices / 682 props**, protocol1050, feature20, APIs5/5/11,
JRW6 with reads1-6. This release adds SP controllers; MP map-entity parity is open.
Targeted production setup, scheduler, context, link, duplicate and menu fixtures
pass; full release build, packaging and installation follow this source snapshot.
Real rider/collision behavior, rendered UI, graphical save/load/hub transitions
and owner multiplayer Q/C confirmation remain open. The 32 accepted SP areas
stay closed. The pending disposition of 20 tag-only models is unchanged; no cards
were hidden. Overall goal remains active.

## Previous installed release: 0.82.0 (2026-10-11)

Post-package workspace receipt note: build `20261011-130258-cd783627` installed at
13:03 EDT through `scripts/build-jar.ps1`. **96/96 automated suites passed**.
Independent readback verified **62 installed files**, **11 backups** and
**1779 archived sources** before these receipt notes. ZIP SHA-256:
`EACF1744FE60BC692E17E3B83D03F44B05AF1853DC5198EA762A732BA22817EB`.
Receipt: `build/deploy/20261011-130355-fef308bf/receipt.json`.
Qualification: `build/datapad-0.82.0/qualification.json`.
Symbols/source: `build/symbols/20261011-130258-cd783627.zip`.
Installed SP numeric file version is0.82.0.0; save headerV36.

Datapad pickup is usable again: appended slot8, reusable inventory action opens
the native mission/equipment screen, stock screen image supplies the missing
icon, and the C text editor can set a63-character pickup message. Pickup messages
are literal, immediate notifications, not an archived journal. Context Undo and
copies preserve the text. Actual player-state inventory serialization and old
transition strings pass; full gameplay save/hub and visual acceptance remain.
SP catalog1197/49, original blocked cohort134/182 routed, entity classes193/211,
portable presets107. The actual retail item parser/icon audit passes alongside
all12 previous resource repairs.

All MP binaries and root launchers match0.81 byte for byte; Q/C repair preserved.
MP948/682, protocol1050/feature20/APIs5/5/11, JRW6 reads1-6 are unchanged.
No graphical game was launched. The32 accepted SP areas remain closed; Q/C,
shield, datapad, Morgan, flash removal and prior outstanding acceptance are open.
Remaining controller/resource families, deployed-shield duplication, multiplayer
parity and human/remote/load/capacity checks keep the overall goal active.

A fresh retail binary inspection records20 MD3 helper models with zero surfaces
in `build/datapad-0.82.0/blocked-helper-models.json`. Owner disposition was requested
for those20 tag-only assets (exclude/hide, retain acknowledged unavailable cards,
or keep the requirement open). **No answer yet; no cards hidden or requirement
closed.** The separate no-weapon GLM was not included in that20-item decision.
The rail investigation found track/lane entities are freed into fixed native
registries at map spawn. A proper live owned/saved adapter remains required;
none of rail_track, rail_lane or rail_mover is counted as implemented.

## 0.82.0: usable retail datapad pickup (2026-10-11)

`item_datapad` now has a real reusable inventory route. Its retail item tag was
an item-list index, not an inventory slot, and its definition supplied no icon.
The loader now maps it to appended inventory slot8 and uses the unchanged stock
`models/items/datapad_screen` image when no custom icon was supplied. Selecting
it and using inventory opens the existing mission/equipment datapad screen.
Repeated use does not consume the reader or rewrite campaign objectives.

The world pickup also accepts a short message through the C context menu's
existing text/label editor. Collecting it displays that message and runs normal
item targets. Additional pickups retain one reader while delivering their own
message. Messages are bounded to63 printable characters, use literal formatting,
and cannot inject commands. Existing identity checks, property Undo, assembly
copy and native message save fields apply. These short pickup messages are not
an archived journal; they are displayed at collection, while the inventory
reader opens the game's existing datapad. NPC/dead/cinematic/remote-view use is
rejected. UI rendering and full in-game save/hub behavior still need acceptance.

SP catalog:1197 cards /49 unavailable, with193/211 entity-class routes and107
portable presets.134 of the original182 unavailable entries have routes; datapad
was an additional gap found in0.81, so that original cohort is unchanged. SP
save header advances toV36 while prior inventory indices and old transition
strings stay readable. Native player-state serialization and the production
workshop writer regression cover the new header/slot. The12 existing retail
resource repairs and the actual datapad definition/icon audit pass.

No MP runtime or catalog feature changes are part of this increment: the0.81
Q/C handoff repair is preserved. MP remains948 choices/682 props, protocol1050,
feature20, APIs5/5/11 and JRW6 reads1-6. The32 owner-accepted SP areas stay closed.
Rail track/lane/mover still require an owned, saved traffic controller; source
inspection confirms the map implementation frees track/lane entities into fixed
process-local registries, so merely spawning their names is insufficient.
Remaining missing-resource/controller families, shield assembly duplication,
fuller multiplayer parity and gameplay acceptance keep the goal active.

Release build/install evidence follows in workspace receipt notes. No graphical
client was launched, and no new gameplay acceptance is claimed.

## Latest installed release: 0.81.0 (2026-10-11)

Post-package workspace receipt note: build `20261011-125047-6c5c98f0` installed at
12:52 EDT through `scripts/build-jar.ps1`. **95/95 automated suites passed**.
Independent readback verified all **62 installed files**, **13 changed-file
backups** and **1777 archived source files** before this note. ZIP SHA-256:
`A04F5DA7A57E7AA265D6497D0740C03465A52911D1DEB3028EB636CE10BE4450`.
Receipt: `build/deploy/20261011-125203-17c121fa/receipt.json`.
Qualification: `build/q-menu-shield-0.81.0/qualification.json`.
Symbols and source: `build/symbols/20261011-125047-6c5c98f0.zip`.
Installed SP numeric file version is 0.81.0.0.

The native MP engine previously cleared Q while opening the held-Q menu,
immediately closing it. 0.81 retains Q/C and movement across the overlay input
transition, releases attack/use, and still clears all keys on console/focus/
disconnect. The production catcher plus cgame menu regression passes. The
owner's saved Q binding was correct and was not changed. Root launchers, MP
server/game DLL, controls and catalog are byte-identical to 0.80. MP remains
1050/feature20/APIs5/5/11, JRW6 reads1-6. SP renderers relinked with byte-identical
.text executable sections and no renderer source changes.

SP portable-shield pickup/controller work was also completed in this release:
stock inventory/model/materials, bounded placement, health, friendly passage,
expiry, cleanup and native save fields. Actual retail parser admits all 12
resource repairs. SP saves now V35; old readers and inventory indices remain.
The deployed transient shield is not an assembly-duplication route yet.
SP catalog remains1197/50: item_enviro gained a route and the existing broken
item_datapad inventory tag is now explicitly unavailable. Original182 routes
are134; entity classes193/211, portable presets107; MP948/682 unchanged.

No graphical client was launched. Q/C operation, visible shield behavior/full
save-load, Morgan spawning and removed saber-impact flash need owner gameplay
confirmation. The 32 previously accepted SP reports remain closed. Remaining
catalog/controllers and fuller multiplayer parity, human/remote/late-join/load
and capacity/performance checks keep the overall goal active.

## 0.81.0: multiplayer held-menu input and SP portable shield (2026-10-11)

The owner reported that Q could not open the multiplayer spawn menu. The saved
binding was already `+jar_menu`. The native engine cleared every held key when
cgame took input, synthesizing Q-up and immediately closing the new menu.
The Q/C overlay transition now retains its held key and movement keys while
releasing attack/use. Releasing Q/C closes the menu; console takeover, focus
loss and disconnect still clear all input. Closing the overlay preserves the
movement handoff before resetting `jar_mpMenuOpen`. No profile rebind is needed.
The production engine catcher is now exercised together with the actual cgame
menu in the input regression, including queued releases and nested cleanup.

The previously unfinished SP `item_enviro` pickup is also implemented: the
retail portable shield model/icon grants one inventory charge (up to five),
then inventory Use deploys a 250-health wall for up to 25 seconds. Placement
requires clear space and a nearby static world floor; failed placement retains
the charge. Native damage, friendly passage, occupancy checks, expiry while
held/frozen, cleanup and saved callbacks are wired. The visible wall uses the
stock blue shield material with matching collision geometry. The deployed,
transient wall is not yet an owned assembly-duplication route. Rendered behavior
and full gameplay save/load remain unconfirmed.

SP saves advance to V35, retaining existing inventory indices, older transition
strings, and earlier workshop readers. Twelve retail pickup resource repairs
pass the actual items.dat parser audit. Holdable tags outside the inventory
range are rejected before indexing: this exposes `item_datapad` as lacking a
usable SP inventory controller, now honestly marked unavailable. Catalog totals
remain 1,197 cards / 50 unavailable; 134 of the original 182 blocked entries
now have routes, with the later test placeholder and newly identified datapad
gap included in the unavailable total. Entity routes remain 193/211 and portable
presets 107. MP remains 948 choices / 682 props, protocol 1050, feature 20,
APIs 5/5/11 and JRW6 (reads 1-6). This input repair requires the updated MP
client executable and cgame module; it does not change the network protocol.

Release build/test/install evidence follows in workspace receipt notes.
No graphical game was launched. Owner Q/C, shield, Morgan and flare confirmation
remain open; the 32 accepted SP reports stay closed and the overall goal stays
active. Human multiplayer/remote/late-join tests and broader MP parity remain.

## 0.80.0: Morgan Katarn and saber-impact screen flare (2026-10-11)

The owner's latest reports are addressed separately from the accepted SP work.
Retail `morgankatarn.npc` uses **CLASS_MORGAN**; the native class table exposed
only **CLASS_MORGANKATARN**. The SP loaded-definition validator rejected this
as `missing-or-invalid-AI-class`, despite the model being present. Both native
SP and MP tables now map the retail alias to the existing Morgan class. The
NPC tab and `NPC_MorganKatarn` entity alias keep their existing IDs and model.
No NPC asset or saved identity was replaced.

The owner identified the missing-texture flash as the screen overlay produced
when a lightsaber hits a weapon or destructible object and requested its removal.
The SP and MP saber-clash flare draw functions, call sites and shader
registrations are removed. Native hit detection, damage, sounds and in-world
sparks remain intact. This also removes the flare from cinematic/spectator views.

The class/spawn-admission regression passes for the Morgan retail alias, and the
production loaded-definition audit admits the installed Morgan model alongside
Desann, Mon Mothma, RebornBoss and legacy droids. Graphical SP spawning and the
visible absence of the flash still need owner confirmation; no graphical game
was launched. Full release/install evidence follows in workspace receipt notes.

The 0.79.0 MP capacity release is retained: 16,384 entity slots, 4,096 snapshot
entities, fixed 4,096 Sandbox props and 512 NPCs/vehicles. MP protocol1050,
feature20, APIs5/5/11, SP V34 and JRW6 reads1-6 are unchanged. SP1197/50 unavailable
and MP948/682 catalogs are unchanged. The 32 accepted SP reports remain closed;
remaining catalog controllers, MP parity and broader acceptance keep the goal active.

**0.80.0 release evidence** (workspace note after exact-artifact verification):

- Installed 2026-10-11 at 12:22 EDT, build `20261011-122133-9c7b1fc4`, through
  `scripts/build-jar.ps1`; **94/94 suites passed**.
- Independent comparison matched **62 installed files**, **10 backups** and
  **1769 archived sources** before these workspace notes. ZIP SHA-256:
  `5EC6634052D6F6AE37201FBCC1B653780BF7405F4B4FC325065B3E6312E4A0A7`.
- Receipt: `build/deploy/20261011-122230-63a1a37b/receipt.json`.
  Qualification: `build/morgan-flare-0.80.0/qualification.json`.
- Native MP run `build/mp-smoke-20261011-122034` spawned and restored all
  **75 map actor aliases**, including Morgan, with two native bots. Its game
  module matches the installed module byte for byte. The owned server exited.
  Retail-definition regression also admits Morgan through the SP validator.
- The saber-impact screen overlay is removed from SP and MP, including its
  shader registration. In-world sparks, impact sounds and damage are retained.
- The 0.79.0 MP capacity changes, root launchers, both SP renderers, catalogs,
  protocol1050 and save formats are retained. Settings, saves, other mods,
  prior packages and rollback evidence are preserved.
- No graphical client was launched. Morgan's visible SP spawn and the absence
  of the impact flash still need owner confirmation. The 32 accepted SP reports
  remain closed; full multiplayer parity and the overall goal remain open.

## 0.79.0: multiplayer capacity (2026-10-11)

MP now has 16,384 entity slots (16 times stock), 4,096 entities per snapshot,
2,048 model and sound indices, and a fixed Sandbox allowance of 4,096 props
plus 512 NPCs/vehicles. The player limit remains 32 and attachment limit 128.
Optional spawning preserves 128 entity slots for critical game work; admission
also keeps helper headroom. Reserved world/none identities are never allocated.
Full prop/actor batches are preflighted; over-limit copies/restores leave the
existing world unchanged. Critical exhaustion still ends the map with a clear
error instead of indexing outside storage; this is not unlimited allocation.

Server/client/module snapshots preserve high entity/resource IDs and the full
4,096-state frame. Four-MiB messages use 32-bit fragment offsets, bounded
reassembly and completed-sequence replay rejection. Server history is bounded
to 524,288 entity states: at the worst case of 32 clients each seeing 4,096
states, it holds four full frames. Sparse scenes retain more history. This
bounds memory; dense/high-latency sessions may require more full snapshots and
bandwidth. The client retains a 131,072-state parse ring.

The shipped classic MP renderer uses 64-bit sort keys, 32,767 reference entities,
16,384 model/skin registrations and 65,536 stable Ghoul2 surface records per
frame. Full surface pools drop new submissions without reusing earlier data.
Ghoul2 handles have 32,768 slots; packed attachments retain 14-bit entity IDs.
High-ID effects accept signed packed values. NPC cleanup sends bounded batches
without losing queued removals. NPC client/AI state has per-entity storage rather
than exhausting the legacy 3 MB shared animation/string pool. Vehicle storage
also follows entity capacity. These changes increase memory needs.

MP protocol **1050**, feature **20**, game/client APIs **5/5** and renderer API
**11** require matching hosts, clients and modules. SP V34 and JRW6 (reads 1-6)
are unchanged. Existing world files remain readable; older executables cannot
restore worlds that exceed their old limits. The catalogs remain SP1197/50
unavailable and MP948/682 props. SP gameplay is unchanged.

Qualification uses production snapshot writer/parser/module delivery, full
4 MiB transport in both directions, allocation/reserve reuse, render sorting,
surface exhaustion, high-ID attachments, cleanup queues, actor storage and
fixed-limit world transactions. Private headless native run
`build/mp-smoke-20261011-121228` loaded/saved/restored 4,096 frozen separated
props and 512 frozen NPCs, rejected each limit plus one atomically, and connected
two bots. The earlier run exposed shared-pool exhaustion; the per-entity storage
fix is included and the native rerun passed. The optional, unshipped MP Rend2
renderer also compiled; this does not establish graphics parity.

Full release build and installation are pending at source-snapshot time. Later
workspace receipt notes identify the exact installed artifact. No graphical
client was launched. Crowded rendering, active physics/AI performance, remote
human multiplayer, hosting/menu acceptance and full SP/MP parity remain open.
The shared rigid solver still has 4,096 body slots (including world anchors),
256 actor proxies and 128 links; these are separate from entity capacity.
All 32 owner-accepted SP reports remain closed. The overall goal remains active.

**0.79.0 release evidence** (workspace note after exact-artifact verification):

- Installed 2026-10-11 at 12:16 EDT, build `20261011-121531-c7fdc57e`, through
  `scripts/build-jar.ps1`; **94/94 suites passed**.
- Independent comparison matched **62 installed files**, **14 backups** and
  **1769 archived sources** before these workspace notes. ZIP SHA-256:
  `B3B155469CEB7936F313C3DCEAFEB2A16C34987961A5F1D953EC0E755F780234`.
- Receipt: `build/deploy/20261011-121634-818edd63/receipt.json`.
  Qualification: `build/mp-capacity-0.79.0/qualification.json`.
- Installed private run `build/mp-smoke-20261011-121732` verified protocol1050,
  rejection of1049, feature20, default paths, 4096-prop/512-NPC save/load,
  atomic overflow rejection and two bots. The owned server exited. No graphical
  client, owner gameplay, remote humans or performance acceptance was tested.
- The root launchers, both SP renderers and both catalogs are unchanged. The only
  SP runtime source change is release metadata. Settings, saves, other mods,
  prior artifacts and rollback evidence are preserved. All 32 SP acceptances
  remain closed. Full multiplayer parity and the overall goal remain open.


## 0.78.0: exact multiplayer context placement (2026-10-11)

MP **C > Position / angles** exposes X/Y/Z and pitch/yaw/roll for props and
frozen NPCs, vehicles and consenting players. Release the Physgun before using
these fields; freeze actors first. Enter commits a field to the draft. **Apply
pose** sends all six values together; **Reload** discards the draft and reads
the latest server-confirmed pose. Refreshes retain typed input; selection reuse
or lost eligibility closes the editor. Release myself and consent controls
remain accessible even when a numeric draft is invalid.

The server validates identity, ownership, player/occupant consent, initialization,
finite XYZ within +/-65535 and angles within +/-3600 degrees, normalized to a
full turn. Final hulls must be clear. Occupied vehicles also preflight native
driver/droid bolt positions and every passenger before moving anything; native
seat links and animations remain intact. A rejected placement preserves position,
angles, link flags and the prior property Undo. This is exact placement, not a
swept move through the space between positions.

Property Undo restores the previous complete pose if the entity, pose, frozen
state and edit revision still match. An obstructed return retains Undo for a
later retry. Other edits, Physgun grabs, freeze/release and manual rotations
invalidate stale edits. Moving props retain their velocity, angular velocity,
mass and gravity; frozen objects remain frozen. Active attachments remain
attached and may move the selected object on the next simulation step. This
edits the selection, not an entire connected assembly.

Existing JRW6 world saves and the duplicate clipboard retain edited positions
and angles. No world-format/API change: JRW6 reads 1-6, game/client/renderer APIs
4/4/10 and SP V34. MP protocol is **1049**, feature version **19**; hosts and
clients must update together because selection messages carry six new values.

Production tests cover staged UI/bounds/consent escape, command rejection,
atomic body/seat/droid checks, motion retention, Undo conflicts, saved/copied
poses and actual rigid-solver placement. SP behavior and both catalogs remain
unchanged: SP1197/50 unavailable, MP948 choices/682 props. MP front-end and
JAPro exclusions from 0.76.0 are retained; no JAPro files were moved or deleted.

Full build/deployment evidence follows separately. No graphical client was
launched; rendered context interaction, occupied driving, remote human play,
MP menu/hosting acceptance and broader SP/MP parity remain open. Other native
map entities/pickups do not yet gain this transform editor. The 32 accepted SP
reports remain closed and the overall goal remains active.

**0.78.0 release evidence** (workspace note after exact-artifact verification):

- Installed **2026-10-11 at 11:33 EDT**, build `20261011-113240-e3cbbdee`,
  through `scripts/build-jar.ps1`. SP engine metadata is **0.78.0.0**.
- **91/91 suites passed**. Independent readback matched the release manifest,
  **62 installed files**, **14 replacement backups** and
  **1751 archived sources** before these workspace notes.
- ZIP SHA-256: `483BB63B0B61ECCEEC1017CC6AF98AF5CE9DF5771AF10C53567FDB85EB955C79`.
  Receipt: `build/deploy/20261011-113344-2df1fa21/receipt.json`.
  Qualification: `build/mp-transform-0.78.0/qualification.json`.
- Runtime changes against 0.77.0 are limited to MP context commands/UI, transform
  validation/Undo, selection transport, protocol and release metadata. Both
  catalogs, root launchers, MP front-end DLL and SP renderers are byte-identical.
  SP code changes are confined to release metadata; its gameplay remains intact.
- Production tests cover complete placement validation, failed-operation state,
  velocity retention, conflict/control/pose Undo, world/clipboard persistence,
  frozen NPCs/players, revoked consent, four vehicle families and droid bolts,
  live rigid-solver adoption, staged fields and invalid-draft release/consent.
- Installed private headless run `build/mp-smoke-20261011-113425` passed default
  directory startup, module feature19, protocol1049/rejection of1048, two native
  bots, world save/reload, map-start restore, ropes and retained prop velocity.
  The owned loopback server exited. This does not validate rendered transforms,
  graphical menus, remote human multiplayer, occupied driving or performance.
- Catalogs remain SP1197/50 and MP948/682. SP V34, APIs4/4/10 and JRW6 reads1-6
  remain unchanged. Host/client must both use protocol1049. JAPro files were
  neither moved nor deleted. Settings, saves, prior releases and other mods are
  preserved. Installed and archived documentation remains immutable.
- Full MP feature/settings/tool/capacity parity and remaining SP native catalog
  controllers/acceptance stay open. The 32 owner-accepted SP reports stay closed.
  The overall goal remains active.

## 0.77.0: live navigation waypoints (2026-10-11)

SP **waypoint** and **waypoint_small** now spawn as editable markers backed by
the native navigation graph and A*. Place them near the floor for walking AI;
elevated routes require the NPC's existing permission to fly. Existing AI
behavior still decides its destination. These points do not command NPCs to patrol.

**Context > Nav** edits automatic connection range (0-1024 units) and enabled
state, with property Undo. Range zero uses explicit routes only. The Link tool's
Primary and Secondary outputs connect spawned waypoints up to 4096 units apart;
navigation links may form cycles. Activate toggles a point. Physgun movement,
deletion and cleanup refresh the graph between simulation frames. Connected
copies retain their settings and remap both route outputs to copied members.

Connections use native A*, actor clearance and current solid/floor traces. Walls,
unsupported gaps and steep floor normals block walking routes; this does not
implement jumping or door-opening behavior. Dynamic obstacle changes affect
new path searches; native steering/replanning remains responsible for an NPC
already following a route. The graph retains map-authored nodes/edges and door
references, clears stale live path/search handles and updates combat-point lookups.

Native limits remain 1023 nodes including map nodes, 3071 edges and 20 neighbors
per node. Each live point adds up to eight nearby automatic links, subject to
those limits; explicit links have priority. Nearest-cell lists are bounded at
60 candidates, including coincident and widely separated layouts. The optional
region cache is invalidated after edits; native A* performs exact route searches.
With no live points, restore leaves native map navigation untouched.

Existing entity fields save position, range, enabled state and route names;
transient graph handles rebuild after load. SP V34 and MP protocol 1048 / feature
18 / APIs 4/4/10 / JRW6 are unchanged. MP menus and scoped JAPro exclusions from
0.76.0 are retained; these new waypoint controllers are SP only.

Catalog: **1197 SP cards / 50 unavailable / 193 of 211 entity-class routes /
107 portable presets / 133 of the original 182 unavailable entries routed**.
MP remains **948 choices / 682 props**. Production fixtures cover native A*,
collision, flying/large actors, edits/removal, dense graph bounds, reset/restore,
saved scalar fields, spawn dispatch, menu validation, Undo and linked duplication.
In-game NPC routing, rendered markers/menu and full game save/load still need
acceptance; automated fixtures are not gameplay or performance acceptance.
The 32 previously accepted SP reports remain closed. The overall goal is active.
Full release build and exact-package installation are recorded separately.

**0.77.0 release evidence** (workspace note after exact-artifact verification):

- Installed **2026-10-11 at 11:14 EDT**, build `20261011-111323-4df2fdbf`,
  SP engine **0.77.0.0**, through `scripts/build-jar.ps1`.
- **91/91 suites passed**. Independent verification matched the ZIP manifest,
  **62 installed files**, **10 replacement backups**
  and **1748 archived sources** before these workspace notes.
- ZIP SHA-256: `FB478A251985434B8E7A81BDAEC281643C3625CF04F440DFD413440D7E32A389`.
  Receipt: `build/deploy/20261011-111429-19a1509d/receipt.json`.
  Qualification: `build/waypoint-0.77.0/qualification.json`.
- Runtime changes against 0.76.0 are limited to the SP navigation controller,
  native integration, marker/context/link/duplicate paths, catalog and version.
  MP source, catalog, every MP binary and both root launchers are byte-identical.
  SP game and engine binaries changed; renderers remain byte-identical.
- Production fixtures exercise actual native graph/A*, wall/pit/actor-clearance
  checks, flying permissions, 400 live nodes, 1023-node exhaustion, coincident
  and wide-layout cell bounds, stale path/search handles, reset/restore, saved
  scalar fields, Link cycles/range, spawn dispatch, UI validation, Undo and copies.
  No graphical client was launched. This is not NPC gameplay, full-game save/load,
  rendered UI or performance acceptance.
- Catalog now 1197/50, 193/211 entity routes, 107 presets, 133/182 original
  blocked entries routed. MP stays 948/682; SP V34 and MP1048/feature18/APIs4/4/10,
  JRW6 reads1-6 unchanged. JAPro assets were neither moved nor deleted.
- Remaining controller families, MP feature/settings/tool/capacity parity and
  gameplay acceptance stay open. The 32 owner-accepted SP reports remain closed.
  Goal active. Settings, saves, other mods, prior packages and rollback evidence
  are preserved; installed and archived documentation is immutable.

## 0.76.0: multiplayer front end and JAPro asset isolation (2026-10-11)

The MP main menu now has **JA:R Settings** and **Host JA:R Sandbox**. Its
layout, dark palette, Arial font and pointer follow the SP design. A matching
pause menu links to player/team, session/voting/bots, controls and video/audio.
The 48 other retail MP menus retain their native actions, widget positions,
feeders, models and conditions while adopting the palette and typography.
Known frame artwork becomes plain panels; map previews, objective markers,
Force/weapon icons, gamma ramps and other informative images remain intact.
The in-game Q/C overlays also share the SP palette and retained cursor asset.

JA:R Settings exposes camera FOV, third-person distance/height, third-person
mode and aspect adjustment. Edits are staged and validated together; untouched
custom console values remain unchanged. Sandbox bindings install only through
the explicit button, with affected keys listed. Player tool consent sends a
request to the server only while connected. No settings reset occurs on opening.

Host JA:R Sandbox stages name, installed map, player count, local/LAN/public
server type, shared editing, disconnect cleanup, player/map/world editing and
automatic save/restore. Only **Start Sandbox** validates and applies the fields
and issues the map command. Invalid fields or missing maps change no live server
settings. Native server browsing, settings, profile and controls remain available.

When JediAcademyRemixedMP is selected, filesystem startup ignores exactly
**base/japro-assets.pk3** and **base/japro-win-x86.pk3**, case-insensitively.
It logs the exclusion and preserves both files. Other mods, custom packages,
retail assets and explicit other fs_game/fs_forcegame selections retain their
normal lookup rules. This is scoped handling of the two observed conflicts,
not generic detection of renamed packages or loose foreign files. Codex did
not move or delete the owner's JAPro material.

Production menu fixtures check navigation/actions, camera validation, host
staging, consent, explicit bindings, retained informative images and cursor
geometry. Retail asset checks cover all 49 menu names and existing fonts/art.
A private headless fixture deliberately supplies the two conflicting PK3s with
invalid map/menu overrides: native startup skips both, loads the retail map
and JA:R module, executes a separate allowed custom package, admits two bots
and rejects protocol 1047. Fixture package hashes remain unchanged; the test
server exits. No graphical client or human multiplayer acceptance is implied.

Catalogs remain SP **1,197 cards / 52 unavailable** and MP **948 choices / 682
props**. SP V34, MP protocol 1048 / feature 18 / APIs 4/4/10 and JRW6 (reads 1-6)
are unchanged. MP front-end integration is implemented; rendered layout,
click-through hosting and full SP feature parity still need acceptance/work.
The 32 accepted SP reports stay closed, and the overall goal remains active.
Full build, exact-package installation and release evidence follow separately.

**0.76.0 release evidence** (workspace note after exact-artifact verification):

- Installed **2026-10-11 at 10:47 EDT**, build `20261011-104620-51526142`,
  SP engine **0.76.0.0**, through `scripts/build-jar.ps1`.
- **90/90 suites passed**. Independent readback verified the ZIP manifest,
  **62 installed files**, **13 replacement backups**,
  and **1743 archived sources** before these workspace notes.
  Deployment updated 14 files: 13 replacements and the new MP icon package.
- ZIP SHA-256: `C9A344539027034A302174A402E00AEB68CEB269A3EF038058BA59C5FE909987`.
  Receipt: `build/deploy/20261011-104724-0d947cf6/receipt.json`.
  Qualification: `build/mp-frontend-0.76.0/qualification.json`.
- Runtime source comparison against 0.75.0 is limited to MP front-end/theme,
  scoped filesystem loading, shared palette/cursor and release metadata.
  Both catalogs, MP game DLL and root launchers remain byte-identical.
  The MP icon package is byte-identical to the existing SP package.
- Installed native run `build/mp-smoke-20261011-104759` and fixture `build/mp-asset-fixture-20261011-104759-050537/result.json` passed:
  conflicting JAPro packages skipped; retail map/JA:R module loaded; unrelated
  custom package executed; two bots admitted; old protocol rejected. Fixture
  PK3 hashes are unchanged and the owned process exited. Owner mod files were
  neither moved nor deleted. This validates the two known filenames only.
- Graphical layout, native menu click-through and human hosting remain open.
  No graphical client was launched. Full SP feature parity, remote play,
  occupied driving and prior named-NPC/Noghri/context/rope/save/hub/sky
  acceptance remain open. The 32 owner-accepted SP reports stay closed.
- SP V34; MP 1048 / feature 18 / APIs 4/4/10; JRW6 reads 1-6. Goal active.
  Prior packages, backups, settings, saves and other mods are preserved.
  Installed and archived documentation remains immutable.

## 0.75.0: portable scene cameras (2026-10-11)

Two more SP map entities are directly usable from the spawn menu:
**misc_camera_track** creates a view camera with two editable path corners;
**misc_camera_focus** creates a focus marker with its own stationary camera.
Activate either setup to view it. Move or press USE to return to the player,
including while a camera is frozen or held. Deletion, death and a replacement
view cancel its controller. The camera moves and aims without moving the player
body; a swept hull stops travel at solid scenery.

**Context > Camera** edits travel speed, turn speed, corner pause and looping.
Focus markers expose their turn speed. With the Link tool, Secondary links a
track camera to a focus marker or subject; Primary links a focus marker to a
subject. Move supplied corners with the Physgun to edit the track. Speed zero
keeps the camera stationary while aiming. Route links cannot be replaced by
ordinary activation links. Models and icons use existing retail assets.

Copies retain settings, paths and focus relationships, remapped to copied
members; copied views start inactive. Property Undo, helper rollback/cleanup,
controlled-view exit and scalar native save fields have production fixtures.
Callback IDs are appended, and existing serialized entity fields store state;
SP V34 is unchanged. Full in-game save/load and rendered camera acceptance
remain open. No graphical client was launched.

Catalog: **1,197 SP cards / 52 unavailable / 191 of 211 entity-class routes /
105 portable presets / 131 of the original 182 unavailable entries routed**.
MP remains **948 choices / 682 props**, protocol **1048**, feature **18**, APIs
**4/4/10**, JRW6 reads 1-6. These new scene controllers are SP only; full MP
catalog and front-end parity remain open. The 32 accepted SP reports stay closed.

The owner's latest MP report is tracked separately: JAPro assets appearing,
missing JA:R main-menu access and a UI that differs from SP. Earlier native logs
mount both JAPro PK3s from base; later filesystem readback finds them in
base/#japro removal folder. Codex did not move or delete them. The asset pack
contains ui/jamp/main.menu. Fresh client confirmation, scoped loading isolation
and JA:R MP menu/design work remain open. This release does not claim those fixes.
Full build/install and exact-artifact evidence are recorded separately.

**0.75.0 release evidence** (workspace note after exact-artifact verification):

- Installed **2026-10-11 at 10:20 EDT**, build `20261011-101843-3bce3b6a`,
  SP engine **0.75.0.0**, through `scripts/build-jar.ps1`.
- **88/88 suites passed**. Independent readback verified the ZIP manifest,
  **61 installed files**, **12 replacement backups**,
  and **1734 archived source files** before these workspace notes.
- ZIP SHA-256: `F90AB9D2C6E8EEE20DF2FED726C179437826845DCE24BC63E6E4E1BCF49B36A7`.
  Receipt: `build/deploy/20261011-102001-32e081d6/receipt.json`.
  Qualification: `build/scene-camera-0.75.0/qualification.json`.
- Only scene-camera/catalog/context/copy/view code and release metadata changed
  in runtime sources since 0.74.1. All MP binaries and MP catalog are byte-identical.
- Installed private headless run `build/mp-smoke-20261011-102056` passed default-path retail map/module
  startup, two bot clients and old-protocol rejection. Its current search path
  contains neither JAPro PK3; those files now reside in the owner's removal
  folder. Codex did not move/delete them. The owned server exited.
- No graphical client, rendered scene camera, full in-game save/load, human
  occupied driving or remote multiplayer acceptance ran. MP menu/UI parity and
  scoped asset isolation remain open; they are the next implementation priority.
- SP V34; MP 1048 / feature 18 / APIs 4/4/10; JRW6 reads 1-6. Goal active.
  The 32 accepted SP reports stay closed. Preserve all prior packages/receipts,
  settings, saves and mods. Installed/archived documentation is immutable.

## 0.74.1: direct-launch release qualification (2026-10-11)

The launcher and directory fixes described under 0.74.0 are unchanged. The
combined native test now disables automatic world restore while constructing
its isolated vehicle-motion fixture, so a preceding world test cannot restore
objects into that fixture. This was test setup state, not a game runtime fix.
The 0.74.0 candidate and its installation receipt remain preserved. 0.74.1
repeats the required build, package and exact-ZIP installation with this test
correction included in its source archive. Runtime changes since 0.74.0 are
release metadata only. Full-release evidence follows separately.

**0.74.1 release evidence** (workspace note after exact-artifact verification):

- Installed **2026-10-11 at 09:53 EDT**, build `20261011-095224-a09913d6`,
  SP engine **0.74.1.0**, through `scripts/build-jar.ps1`.
- **87/87 suites passed**. Independent readback matched the ZIP manifest,
  all **61 installed files**, **8 replacement backups**,
  and **1730 archived sources**. Launcher binaries and matching PDBs
  are also preserved in the symbol archive. Catalogs remain byte-identical.
- ZIP SHA-256: `AD30ED6557FFB9A792FC344A24671545089DEE771B6FA1AC877C889286567CAA`.
  Receipt: `build/deploy/20261011-095321-86ce0de9/receipt.json`.
  Qualification: `build/launch-motion-0.74.1/qualification.json`.
- Native installed-artifact run `build/mp-smoke-20261011-095334` passed **without fs_basepath or
  fs_game arguments**, finding retail maps and JA:R's module from its private
  directory with an unrelated working directory and isolated profile. It passed
  world save/load/map restore, all four empty vehicle families resuming JRW6
  horizontal momentum, autosave/map shutdown/torn-journal recovery/orderly quit,
  two bot clients and protocol-1047 rejection. The owned server exited.
- Exact immediate speed/drift restoration and held/occupied snapshot policy
  are production-fixture evidence. Native unattended vehicle controllers may
  change commanded speed or health while simulation runs; the native motion
  test checks continued coasting and living actors, not frozen state equality.
- No graphical client, human occupied driving or remote multiplayer acceptance
  ran. Root SP/MP launchers are installed; actual owner startup confirmation
  remains distinct from harmless-child/path tests and dedicated-server startup.
- MP **1048 / feature 18 / APIs 4/4/10**, SP **V34**, MP **JRW6** reads **1-6**.
  The full goal remains active. The 32 accepted SP reports remain closed;
  named NPC/Noghri/context/rope/full-save/hub/sky and further MP acceptance
  remain open. Installed/archived documentation is immutable.

## 0.74.0: SP/MP direct launch and vehicle motion saves (2026-10-11)

The main GameData folder now contains both **jar_sp.x86_64.exe** and
**jar_mp.x86_64.exe** launchers. Both use a static runtime, start the matching
private engine with GameData as its working directory, and forward arguments
and child exit codes. Multiplayer also selects JediAcademyRemixedMP by default.
The existing Play-JA-R-Multiplayer.cmd delegates to the new executable.

Direct starts from either private runtime folder now locate the retail assets
in the GameData parent. Discovery uses the executable's actual location even
when Explorer, a shortcut or a shell supplies a different working directory.
Both assets0.pk3 and assets1.pk3 must exist before choosing the parent; a
standalone runtime with its own base directory remains supported. Explicit
fs_basepath/fs_game arguments still override defaults. Engine DLLs remain in
their private folders, preserving other mods' runtimes in GameData.

The in-progress MP vehicle persistence work is included: **JRW6** stores native
commanded speed and the separate horizontal external momentum, together with
total velocity. Delayed native spawning restores all three; repeated saves and
map restores preserve the split. JRW1-5 remain readable and keep their legacy
speed derivation. Clipboard duplication still starts stationary. Automatic
snapshots park occupied/held vehicles without changing their live motion;
manual save/clear protections remain. Invalid or truncated motion records are
rejected before spawning anything.

MP protocol **1048**, feature **18**, APIs **4/4/10**, SP **V34**. Catalogs are
unchanged: SP 1,197 cards / 54 unavailable; MP 948 choices / 682 props. New JRW6
saves require this release or later. Earlier releases cannot read JRW6.

Focused production tests cover exact motion restoration for all four vehicle
families, JRW1-5 imports, malformed records, stationary copies and occupied
snapshot protections. Launcher tests use a harmless child executable; path
discovery tests cover unrelated working directories, missing/partial retail
assets, both private folders and standalone layouts. Full release build,
installation and private dedicated-server evidence is recorded separately.
No graphical client or human driving acceptance is implied. The goal remains
active, and the 32 owner-accepted SP reports remain closed.

## 0.73.1: driven vehicle attachments and sustained vertical thrust (2026-10-11)

This corrects the 0.73.0 candidate before final qualification. Rope, Thruster,
Weld and Hinge still accept settled occupied vehicles with every rider's consent,
and Physgun fling retains sideways momentum while native steering runs. Native
seat publication, inert boarding transitions, consent/release, ownership and
occupied-save protections from the previous implementation remain.

A new four-second production movement regression exposed a flaw in 0.73.0:
separately accumulated upward thrust reached its limit while native falling speed
continued to increase, eventually overcoming a stronger upward force. **Vertical
impulses now stay in native velocity together with gravity and vertical contacts.**
Only horizontal external momentum is separated from steering and swept after
native movement. Step/slide-generated vertical velocity goes back to the native
component. Horizontal impulse speed remains bounded; vertical impulses cannot
increase velocity beyond 1,500 units/second unless native velocity was already
higher. The sustained-thrust regression failed before the repair and now passes.
Native vehicle controller output remains a fixture boundary for occupied tests.

MP protocol **1048**, feature **17**, APIs **4/4/10**, SP **V34**, MP **JRW5** and
both catalogs are unchanged from 0.73.0. Matching current clients and servers are
required. World saves retain total velocity; restoring the split between native
motion and horizontal external momentum after a world reload remains follow-up
work. Human occupied driving, rendered seats/cameras, collision reactions,
latency, remote/late-join/load behavior and full multiplayer parity remain open.
The 32 accepted SP reports remain closed and the overall goal stays active.

The original 0.73.0 ZIP/source archive and installation receipt are preserved.
Its full native baseline run passed world restoration, empty-vehicle forces,
Weld/Hinge, all 20 vehicle definitions together and two bots, but did not exercise
the newly discovered occupied vertical-force flaw. Current-release build, install
and native evidence will be recorded after qualification. No graphical game or
owner save was opened.

**0.73.1 release evidence** (workspace record added after exact-artifact verification):

- Installed **2026-10-11 at 09:24 EDT**, build `20261011-092339-3886b1d9`,
  SP file version **0.73.1.0**, through `scripts/build-jar.ps1`.
- **85/85 automated suites passed**, including the sustained vertical-thrust
  regression that failed against 0.73.0, and retail vehicle/asset qualification.
- Independent readback verified **60 installed files,
  10 replacement backups and 1725 archived sources**,
  exact ZIP/manifest/stage, unchanged catalogs and native step/slide/clipping
  extraction. Native driving functions are unchanged. SP runtime changes since
  0.72.0 are version metadata only.
- ZIP: `build/JediAcademyRemixed-0.73.1-win64.zip`.
  SHA-256: `90075F5336BA26A957A361C6C9D6B79650D2BD062B3221DDB5E7EE03FDB19C36`.
  Receipt: `build/deploy/20261011-092439-b67ff574/receipt.json`.
  Qualification: `build/driven-vehicles-0.73.1/qualification.json`.
- Current installed binaries passed private native run `build/mp-smoke-20261011-092505`:
  world save/load/map restoration, four empty vehicle families, NPC/vehicle
  Rope/Thruster motion, two bot clients and old-protocol rejection. The test-owned
  server exited. No human driver, occupied-seat gameplay or graphical client ran.
- The prior 0.73.0 candidate also passed native Weld/Hinge and all 20 vehicle
  definitions together in `build/mp-smoke-20261011-091414`. That candidate is
  superseded for the sustained-force defect; its ZIP and evidence are preserved.
- Occupied attachment forces, rider permission/boarding lifecycle and client
  state have production-fixture coverage. Native controller output, BSP geometry
  and impact side effects are explicit fixture boundaries. Human driving,
  rendered seats/cameras, collision reactions and remote/late-join/load behavior
  remain unvalidated. Persisting the native/external velocity split is next MP work.
- MP **1048 / feature 17 / APIs 4/4/10**, SP **V34**, MP **JRW5**; catalogs remain
  SP **1,197 cards / 54 unavailable / 189/211 entity routes / 103 presets /
  129/182 original blocked routes**, MP **948 choices / 682 props**.
- Goal active; 32 accepted SP reports remain closed. Further catalog/controller,
  context/rope/sky/full-save/hub/Noghri/NPC and multiplayer acceptance remains open.
- Archived/installed documentation remains immutable. Workspace receipt notes,
  the authoritative requirement rows and dated handoffs were reconciled after
  source/archive/install verification.

## 0.73.0: attachment forces on driven multiplayer vehicles (2026-10-11)

Rope, Thruster, Weld and Hinge now accept settled occupied vehicles when the
vehicle and every rider permit the edit. Native steering, throttle, weapons and
boarding code continue to run. External linear momentum is kept separately from
native commanded speed and replicated to the pilot's prediction. The shared
movement path runs native driving, then sweeps the external component through the
native vehicle hull using step/slide contacts, without applying gravity twice.
Ground pushes damp quickly; airborne momentum decays slowly. External speed is
bounded to 1,500 units/second without imposing that limit on native throttle.

Physgun fling now retains transverse as well as forward momentum on occupied
vehicles. Joint projection updates pilot, passenger and droid positions after
moving the vehicle. Native impact responses remain active. Existing attachments
stay inert during structurally valid boarding/pilot transitions, then resume;
invalid seats, death, disconnect or lost permission remove their force sources.
Any rider's `jar_release` or tool opt-out also clears the external momentum.
Ordinary attachment removal retains momentum, as a released rope should.

MP advances to **protocol 1048 / feature 17**, with game/client APIs **4/4** for
the expanded player state; renderer API **10** is unchanged. Matching clients and
servers are required. Three full-precision drift fields are included in normal,
internal-pilot and vehicle snapshot tables, including fresh and zero-reset deltas.
SP V34 and MP JRW5 remain unchanged. Autosaves still park player-free occupied
assemblies without disturbing live riders; occupied copy/remove/manual world
replacement remain protected. JRW5 stores total linear velocity, not its division
between native driving and external momentum; restoring that division across a
world reload remains follow-up work for moving, newly unoccupied vehicles.

Focused production tests cover four vehicle families, all four tools, suspended
boarding attachments, shared/server ownership, consent and revocation, rider
publication, and unchanged native commands/speed. A new fixture exercises the
actual native step/slide and velocity-clipping code with deterministic collision
and impact adapters: wall sweeps, one gravity pass, bounded impulses, zero-drift
behavior and command-rate damping. Its controller output is supplied by the
fixture. It is not a human driving, camera, retail collision-damage or latency test.
Native movement code extracted for this fixture is otherwise unchanged.

**Still open:** actual occupied driving/steering and attachment behavior with human
clients, rendered seats/cameras, remote and late-join behavior, load/performance,
full multiplayer feature parity and remaining catalog/controllers. The 32 accepted
SP reports remain closed; newer SP controller/pose/context/rope/sky/full-save/hub,
Noghri attack and named-NPC acceptance remain open. The all-objectives goal stays
active. No graphical game was launched for this implementation.

Catalogs are unchanged: **1,197 SP cards / 54 unavailable / 189 of 211 entity
routes / 103 portable presets / 129 of the original 182 blocked routes**, and
**948 MP choices / 682 props**. SP runtime changes are version metadata only.
Build, installation and exact-artifact evidence will be recorded after release
qualification; earlier release sections below are historical records.

## 0.72.0: occupied multiplayer vehicle manipulation (2026-10-11)

A settled, occupied vehicle can now be held with the Physgun, rotated with
**left click + USE + mouse**, flung on release, or frozen with right click.
Toolgun Freeze and the C menu support Freeze/Release, rotation while controlled,
and vehicle health/armor edits with property Undo. Every player rider must allow
tools, and the editor must have permission for the vehicle and any NPC riders.
Ownership and server player/map-tool policies remain authoritative.

Native pilot/seat links are preserved. The pilot follows the native driver-bolt
callback; passenger positions match the native MP client's vehicle-origin camera
placement, and optional droid helpers follow their native bolt. Position updates
preserve queued weapon/animation events. Vehicle hull checks still govern movement
and rotation. No Force damage or rider choke pose is introduced.

Any rider can use `jar_release` or disable tools to release the vehicle immediately.
USE or jump input also releases it with a short re-grab cooldown, allowing native
dismount handling to proceed. Inputs are not rewritten. Boarding/ejection/pilot
exchange, invalid seat links, death, consent changes and disconnects end control.
Occupied assemblies remain protected from Remove, copy, Undo and cleanup; owner
disconnect retains the occupied assembly under server ownership. Automatic saves
park a player-free copy without interrupting the live occupants. Manual world save
and replacement still require occupants to exit. JRW5 and SP V34 are unchanged.

The pilot's client now preserves the authoritative frozen vehicle pose and updates
its native seat attachment instead of running the view-angle mutation that precedes
Pmove's freeze return. MP advances to **protocol 1047 / feature 16**; matching
clients and servers are required. Game/client APIs 3 and renderer API 10 remain.

**Remaining occupied-vehicle work:** Rope, Thruster, Weld and Hinge while driving
need an additive native driving adapter. These tools still refuse occupied targets
and detach existing attachments on boarding; the empty-vehicle movement override
must not replace a human pilot's driving. Boarding itself remains native. This
release does not complete all-entity tool support or multiplayer parity.

The expanded production fixture exercises all four vehicle hull families, native
pilot attachment with a deterministic Ghoul2 matrix boundary, passenger/droid
position publication, client prediction, consent/revocation, native exit input,
health Undo, frozen/held autosave capture and occupied cleanup/retention. This is
headless evidence, not actual human boarding, rendered seats, camera smoothness,
driving, latency or remote/late-join acceptance. Native dedicated-server regression
and exact release-install evidence are recorded separately below. The 32 accepted
SP reports remain closed and the overall goal remains active.

Catalogs remain **1,197 SP cards / 54 unavailable / 189 of 211 entity routes /
103 portable presets / 129 of the original 182 unavailable entries routed**, and
**948 MP choices / 682 props**. SP gameplay code is unchanged apart from version
metadata. The 0.69.1 save repair and 0.70-0.71 additions are retained.


**0.72.0 release evidence** (workspace receipt added after packaging):

- Installed **2026-10-11 at 08:51 EDT**, build `20261011-084954-b89b86c8`,
  installed SP file version **0.72.0.0**, through `scripts/build-jar.ps1`.
- **84/84 suites passed** plus retail vehicle/asset checks. The production MP
  suite now includes occupied seat/consent/control/context/prediction checks.
- Independent readback verified **60 installed files, 15 replacement backups,
  1,720 archived source files**, the exact ZIP/manifest/stage, and the bounded
  runtime changes. Both catalogs are byte-identical to 0.71.0; native pilot
  callback extraction is unchanged after expansion. SP runtime sources are
  unchanged apart from release version metadata.
- ZIP: `build/JediAcademyRemixed-0.72.0-win64.zip`.
  SHA-256: `D8078B8951ED9FB24603C5599306ABCCF19578EAACE10C8BAB6C3B5BAF2E3263`.
  Receipt: `build/deploy/20261011-085057-aef187be/receipt.json`.
  Qualification: `build/occupied-vehicles-0.72.0/qualification.json`.
- Installed binaries passed private native run `build/mp-smoke-20261011-085149`:
  world save/load/map restore, four empty vehicle families, native NPC/vehicle
  rope/thruster motion, all 20 vehicle definitions together, cleanup, two bots,
  and protocol-1046 rejection by protocol 1047. The test-owned server exited.
  This native run had **no human driver or occupied-seat gameplay test**.
- Occupied controls and client pose preservation have production-fixture evidence
  with a deterministic Ghoul2 matrix boundary. Actual boarding, rendered seats,
  camera smoothness, driving and remote/late-join behavior remain unvalidated.
  Occupied Rope/Thruster/Weld/Hinge forces remain implementation work. The next
  MP step is additive attachment forces that preserve native pilot driving.
- MP protocol **1047**, feature **16**, APIs **3/3/10**; SP **V34**, MP **JRW5**
  remain. Goal active, 32 accepted SP reports preserved, other outstanding catalog,
  context/rope/save/hub/Noghri/NPC and multiplayer acceptance remains open.
- Archived/installed documentation stays immutable. Only these workspace receipt
  notes and dated handoffs were added after source/archive/install verification.


## 0.71.0: native stock swoop color variants (2026-10-11)

Seven previously blocked vehicles now have direct menu routes in **SP and MP**:
**swoop_black, swoop_blue, swoop_gold, swoop_green, swoop_purple, swoop_red and
swoop_silver**. Retail ships their NPC records, shared swoop model and all seven
skins, but omits vehicle definitions for these names. When an explicit definition
is absent, the native loader now uses a qualified retail Swoop behavior block and
the requested skin. Red uses the stock `default` skin. No geometry, textures or
icons are fabricated. Original card IDs are preserved.

Explicit vehicle definitions take precedence in the native loader; the fallback
accepts either known retail Swoop block and rejects an unsupported replacement.
SP Sandbox retains its existing loaded-definition fingerprint checks. Each color
has its own native vehicle identity and settings, so copies and restored worlds
retain the choice without changing other swoops. These use SP Swoop's native
2,000 armor and other behavior, not the separate 80-armor `swoop_mp` definition.

The distinct vehicle-definition registry expands from **16 to 64 slots** (one is
reserved). Existing loaded definitions remain available when it fills; further
new definitions fail cleanly. This is separate from the live entity/prop limits.
SP workshop saves advance to **V34**, retaining all eight Sandbox chunks and
reading complete V1-33 saves. The real registry serializer is tested at 63 entries
with reordered registration and an invalid saved count. Incomplete older saves
remain unrecoverable as documented in 0.69.1.

MP advances to **protocol 1046 / feature 15** so old clients cannot encounter
vehicle identities they cannot load. Matching clients and servers are required.
Game/client APIs 3 and renderer API 10 are unchanged; JRW5 still reads versions
1-5. Catalogs now have **948 MP choices / 682 props** and **1,197 SP cards / 54
unavailable / 189 of 211 entity-class routes / 103 portable presets / 129 of the
original 182 unavailable entries routed**. The remaining missing models and map
controllers stay open.

The native server check also exposed an existing MP NPC-loading regression:
the 2 KB filename buffer silently omitted late entries when installed mod packs
expanded the list (2,069 bytes in this installation). `z-95` failed to spawn as a
result. MP now matches SP's 64 KB filename buffer and reserves space for the
appended newline/NUL in the data buffer. A production loader test first reproduced
the truncation, then loaded all 201 fixture files including the final NPC. Other
mods and their assets are preserved.

Two new headless tests execute the complete native SP and MP vehicle loaders.
Retail qualification checks all seven visible skins and their texture files,
both shipped Swoop definitions, inherited vehicle settings, explicit-definition
precedence, stable repeat lookup, 64-slot exhaustion and SP registry save/load.
Native dedicated-server qualification and exact installation evidence follow
below. These checks do not establish rendered colors, driving/boarding behavior,
human multiplayer, full graphical SP saves or load performance. The 32 previously
accepted reports remain closed, and the overall goal remains active.


**0.71.0 release evidence** (workspace receipt, added after packaging):

- Installed at **08:31 EDT on 2026-10-11** through `scripts/build-jar.ps1`.
  Build `20261011-083032-7971ac08`; installed SP executable **0.71.0.0**.
- **84/84 automated suites passed**, followed by both native retail Swoop
  definition variants in the SP and MP loaders. The NPC filename-list fixture
  reproduced the old truncation and passes with all 201 files after the fix.
- Independent readback matched the exact ZIP, manifest, stage, **60 installed
  files**, **18 replacement backups**, and **1,716 archived source files**.
- ZIP: `build/JediAcademyRemixed-0.71.0-win64.zip`.
  SHA-256: `D61E0AA2B29F37B20F22E1CE94CFCDFE4F883AF6F766E81A0D25F184C06B1E02`.
  Receipt: `build/deploy/20261011-083152-831b908f/receipt.json`.
  Qualification: `build/vehicle-variants-0.71.0/qualification.json`.
- Exact installed dedicated-server artifacts passed the private loopback run
  `build/mp-smoke-20261011-083231`: all **20 vehicle definitions**, including
  seven stock colors, spawned together, saved, restored and survived map
  restart; world cleanup, two native bot clients and old-protocol rejection
  passed. The test-owned process exited. No graphical client was launched.
- Previous 941 MP catalog rows and all unaffected SP card IDs/data are unchanged.
  SP now has **129/182 original blocked routes implemented, 54 unavailable
  cards**; MP has **948 choices / 682 props**. Vehicle registry: 64 slots,
  63 usable; SP saves V34; MP protocol 1046 / feature 15, matching peers required.
- Rendered skins, driving/boarding, human/remote/late-join multiplayer, full
  graphical SP save/load and performance remain unvalidated. The 32 earlier
  owner-accepted reports remain closed. Overall goal active; remaining real map
  controllers, missing resources, MP parity and acceptance remain open.
- Archived and installed documentation is immutable. This workspace-only receipt
  was written after the source/archive/installation comparison.


## 0.70.0: portable sky viewpoints (2026-10-11)

`misc_skyportal` is now directly spawnable from **Q / Entities** in SP Sandbox.
It creates a movable stock marker that starts off. Position it with the Physgun,
then Activate/USE it or select **C / Sky / Viewpoint: On / Apply**. Its position
becomes the sky camera; rotating it adds an offset to the player's viewing angle.
**Entities in sky** chooses whether current entities visible from that position
are included. The map must have sky surfaces: this does not create geometry.

The latest enabled controller takes precedence. Turning it off or deleting it
restores the previous enabled controller, then the map's exact original sky.
Authored native sky changes preempt the stack. Stationary views avoid repeated
config updates. Context edits support Undo and selection-lifetime checks. Copies
retain the pose and entity option but start inactive. Cleanup, Sandbox disable
and transitions release the override. Classic and Rend2 now clear sky-pass state
between frames so disabling the last controller restores ordinary sky rendering.
Native authored portal-entity flags are preserved.

Workshop saves now use **V33**, adding the `JSKY` ownership chunk and use callback
86. Full saves retain controller order and the original sky; player-only autosaves
restore the baseline without restoring world controllers. Complete native,
recovery and workshop V1-32 saves remain readable. The 0.69.1 save-dispatch repair
is retained; missing chunks in earlier incomplete saves cannot be reconstructed.

Catalog: **1,197 cards / 61 unavailable / 189 of 211 entity-class routes / 103
portable presets / 122 of the original 182 unavailable entries routed**. This
controller and its Sky page are SP-only. MP remains at **941 choices / 682 props**,
protocol 1045 / feature 14, APIs 3/3/10 and JRW5. The shared Rend2 frame change is
conditional on the SP build; multiplayer behavior is unchanged.

Targeted fixtures exercise the production spawn, activation, movement, context,
Undo, copy, removal and serializer paths, exact eight-chunk writer dispatch,
server sky-view visibility dispatch, client entity filtering and both renderer
frame resets. The visibility fixture supplies deterministic PVS answers; it does
not establish retail-map visibility or graphical correctness. Native fog keys
remain unused by the renderer and are not exposed as editable properties.
Graphical sky appearance in both renderers, full in-game save/load and hub
behavior still need acceptance. No owner game was launched. The 32 previously
accepted reports remain closed; the overall goal and remaining MP/catalog work
remain active. Full release/install evidence is recorded after packaging.

### 0.70.0 release receipt (workspace entry after packaging)

**0.70.0 is installed and independently verified**, 11 October 2026 at 08:09 EDT.
The full workflow passed **82/82 suites** and retail asset checks. All **60
installed files**, **11 changed-file backups** and **1,707 archived source files**
matched the exact package and source snapshot. The installed private SP engine
reports **0.70.0.0**. All **590 MP runtime source files** and its catalog remain
unchanged; the only shared-renderer behavior change is conditional on SP.

The production fixtures cover the controller lifecycle, spawn/context/copy paths,
eight-chunk full/autosave dispatch and Classic/Rend2 frame-state reset. Graphical
sky appearance, actual retail-map sky entity visibility, in-game full saves and
hub transitions remain untested. No owner game was launched or terminated. No
new MP server run was required for the unchanged MP runtime. The 32 accepted SP
reports remain closed. Outstanding map controllers, MP parity and acceptance keep
the overall goal active.

- Build: `20261011-080801-25bd89c9`.
- ZIP: `build/JediAcademyRemixed-0.70.0-win64.zip`.
- SHA-256: `84334D1CD3E82EB9251293066A170E2135A75F36A43164F92533F66DE237E257`.
- Receipt/backups: `build/deploy/20261011-080908-2b14d3d8/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-080801-25bd89c9.zip`.
- Qualification: `build/sp-skyportal-0.70.0/qualification.json`.
- Workflow log: `build/release-0.70.0.log`.


## 0.69.1: Sandbox save dispatch repair (2026-10-11)

The engine selected workshop save version 32, but the game writer still accepted
only version 31. This mismatch omitted all seven Sandbox chunks from newly
written workshop saves. 0.69.1 uses one current-version constant for both header
selection and writing. The on-disk version and chunk formats remain unchanged;
native, recovery and workshop versions 1-32 remain readable when complete.

A new test executes the actual production writer with the engine-selected
version. It first reproduced the omission, then passed full and player-only
autosave round trips with both prepared and on-demand state. It checks chunk
order, carried tools, owned objects, ropes, frozen native actors, stacked music,
ship-boundary steering and native mover translation/spin through their real
serializers. Native/recovery-only saves still omit Sandbox chunks. A prepared
snapshot is used unchanged even if live inventory changes before dispatch.

This repairs future writes. It cannot reconstruct Sandbox chunks that older
builds never stored. No existing owner saves are modified. Earlier isolated
codec checks and release hashes did not validate this dispatch path; full
in-game save/load and hub acceptance therefore remain open. The 32 previously
accepted SP reports remain closed; this is a newly identified regression.

No skyportal controller is included in this repair. Its native fog keys are
ignored by the renderer and need further implementation. Remaining map
controllers and multiplayer parity stay on the active goal. Multiplayer runtime,
protocol 1045/feature 14, APIs 3/3/10, JRW5 and both catalogs are unchanged.

### 0.69.1 release receipt (workspace entry after packaging)

**0.69.1 is installed and independently verified**, 11 October 2026 at 07:47 EDT.
All **80/80 suites** and asset checks passed. **60 installed files**, **8 changed
file backups** and **1,699 archived source files** matched the exact ZIP/source.
The private SP engine reports 0.69.1.0. All **590 MP source files**, both catalogs
and **636 other SP source files** are unchanged from 0.69.0. The only SP runtime
changes are save dispatch, its shared version constant and the release version.

The new production writer regression failed before the repair and passed in the
final full workflow. It checks all seven chunks and real serializer round trips
for full and player-only autosaves, with prepared and on-demand state. This is
headless fixture evidence, not a graphical full-save/hub test. No owner game was
launched or terminated. The prior native MP qualification is retained; unchanged
MP code did not require another dedicated-server run for this SP-only repair.

Earlier isolated codec checks did not exercise the mismatched version dispatch.
Their historical receipts remain intact; they do not establish that version-32
Sandbox saves from those builds were complete. Missing chunks cannot be recovered
by this patch. Existing owner saves were left untouched. The goal remains active;
remaining catalog/controllers, multiplayer parity and acceptance remain open.

- Build: `20261011-074554-7650a1b8`.
- ZIP: `build/JediAcademyRemixed-0.69.1-win64.zip`.
- SHA-256: `9553E5B9FED3EE48A177FE97133124CE2E879DC92CCEBE947A12AE8DD774FD55`.
- Receipt/backups: `build/deploy/20261011-074656-c651027c/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-074554-7650a1b8.zip`.
- Qualification: `build/sp-save-0.69.1/qualification.json`.
- Workflow log: `build/release-0.69.1.log`.

These post-package workspace notes leave immutable archives and installed docs intact.

## 0.69.0: multiplayer entity context properties (2026-10-11)

Hold **C** while aiming at an editable entity. The context menu now identifies the
selected type, catalog name and entity number, and shows server-confirmed values.
It presents only applicable actions. Props expose typed mass (**5-1500 kg**) and
gravity; NPCs, empty vehicles and consenting players expose current health
(**1-32000**); native weapons/ammo/health/powerup pickups expose their quantity
(**0-1000**, with 0 retaining the native default). Armor and holdable pickups do
not show a quantity field because their native pickup code ignores that value.
Native pickup caps and health/overhealth behavior still apply. Vehicle health
updates both native health and vehicle armor. These edits do not change max health.

The context's **Undo property edit** reverses the player's latest mass, gravity,
health or quantity edit if that property and its entity are still unchanged.
Damage, another editor, entity reuse, lost ownership or revoked consent prevent a
stale Undo. This history is transient; the edited values persist in duplication
and JRW5 world saves. Existing pose Undo and spawn/assembly Undo remain separate.
Freeze/release, pose editing, rotation, attachment removal and self-release remain
available according to entity type and permission. Pickup removal now works too.

Every context mutation includes a server-issued entity-lifetime token. Delayed
input cannot modify a replacement entity in the same slot. Deletion and player
respawn invalidate selections, periodic replies refresh open menus, and closing C
stops periodic updates. Pending actors and occupied vehicles cannot be edited;
server ownership, shared/world/map editing and player tool consent still apply.
Typed drafts survive ordinary refreshes and cancel when the selection changes.

MP protocol is now **1045**, with feature **14**; client/server builds must match.
Game/client APIs stay 3, renderer API 10, and JRW5 continues reading versions 1-5.
SP behavior is unchanged, including workshop save 32. Catalogs remain **941 MP
choices / 682 props**, and **1,197 SP cards / 62 unavailable / 188 of 211 entity
routes / 102 presets / 121 of the original 182 unavailable entries routed**.

Production command/UI fixtures cover typed bounds, applicable controls, stale
identities, consent/ownership, occupied vehicles, shared-edit and damage conflicts,
scalar Undo, native health/armor mirrors, pickup removal, copies and actual world
restoration. Mass/gravity commands are also tested against the real rigid solver.
Native server and release qualification follow below. This does not establish
rendered UI, remote-human play or gameplay/load acceptance. Remaining map
controllers, multiplayer parity and open acceptance stay on the active goal;
the 32 owner-accepted SP reports remain closed.

### 0.69.0 release receipt (workspace entry after packaging)

**0.69.0 is installed and independently verified**, 11 October 2026 at 07:34 EDT.
All **79/79 suites** and asset checks passed. **60 installed files**, **13 changed
file backups** and **1,696 archived source files** matched the exact ZIP/source.
SP identifies as 0.69.0.0; all **638 other SP source files** are unchanged from
0.68.0. The entire 941-entry MP catalog is byte-identical to that release.

Production context fixtures passed typed values/ranges and applicable actions,
identity/lifetime rejection, consent/ownership, occupied-vehicle restrictions,
scalar Undo and conflicts, native health/armor mirrors, pickup removal, copies and
world restoration. The real Box3D solver responded to the typed mass/gravity
commands. Client tests cover typed input, server-confirmed values, identity changes,
draft preservation/cancellation, pose interactions and closing the menu.

Installed native run `build/mp-smoke-20261011-073442` passed protocol 1045/old-client rejection,
world save/load/map restore, four frozen vehicle families, health persistence,
retail-map rigid floor/stack, spin restoration, safe solver disable/reenable and
two native bots. The test DLL matched the installed DLL; its owned process exited
and installed hashes were rechecked. This native run did not send the new property
commands from a human client. Those paths have production-fixture evidence, while
rendered context UI and human/network/gameplay acceptance remain open.

Intermediate native run `build/mp-smoke-20261011-073031` predates the final
context-open subscription refinement; it is retained as separate evidence.
Protocol 1045/feature 14 requires matching clients and servers. APIs 3/3/10,
JRW5 reads 1-5 and SP save 32 are unchanged. Goal remains active; remaining map
controllers, fuller MP parity and acceptance are open. The 32 owner-accepted SP
reports remain closed. No owner game was launched or terminated.

- Build: `20261011-073304-f7e2f59f`.
- ZIP: `build/JediAcademyRemixed-0.69.0-win64.zip`.
- SHA-256: `B23B571CB9F68AE9523BFE74B2814FAB030988EAC94A8CB898B31D6E430A3312`.
- Receipt/backups: `build/deploy/20261011-073406-0c78bb43/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-073304-f7e2f59f.zip`.
- Qualification: `build/mp-context-0.69.0/qualification.json`.
- Workflow log: `build/release-0.69.0.log`.

These post-package workspace notes leave immutable archives and installed docs intact.

## 0.68.0: map actor spawners in both menus (2026-10-11)

The multiplayer Entities tab now includes **75 map NPC/vehicle spawner aliases**,
including Desann, Mon Mothma, named bosses, droids and the generic NPC/vehicle
spawners. Each uses the same explicit native actor choice as its SP menu adapter;
it creates that actor immediately, without requiring map scripts or a target.
These are additional entry points to existing retail actors, not 75 new models.
The original map-class name appears on the card and in full at the fixed top-left
hover label. Cards reuse stock icons. Existing 866 MP cards and IDs are preserved;
MP now has **941 choices, including 682 props**.

`misc_atst_drivable` now directly spawns **atst_vehicle** in both SP and MP. This
uses the existing native drivable walker, rather than the legacy player-model
transformation. Its original map callback is not claimed repaired. SP catalog:
**1,197 cards / 62 unavailable / 188 of 211 entity routes / 102 portable presets**;
**121 of the original 182 unavailable entries** now have usable routes.

Aliases retain their original map-class IDs in MP copies/world saves. Ownership,
actor initialization, Physgun, empty-vehicle controls, Undo and cleanup use the
existing native paths. The two SP segmented MD3 droid aliases remain unavailable
in MP; its native actor renderer still needs that support. Full authored spawner
scripting, delayed-wave properties and the other map controllers remain work.
SP saves stay at workshop version 32; MP protocol 1044, APIs 3/3/10, feature 13 and
JRW5 reads 1-5 are unchanged. Use matching release catalogs on clients and server;
an older server rejects these new IDs. New-ID world saves require this catalog.

Production fixtures exercise all 75 native spawn selections, copies, world IDs,
Undo and every MP menu card/full hover label. Release/native qualification follows
below when complete. Frozen headless spawn/restore checks are not NPC behavior,
vehicle boarding, rendered appearance, remote human play or load-performance tests.
The goal remains active and the 32 owner-accepted reports remain closed.

Pre-package native evidence: `build/mp-smoke-20261011-071158`. All 75 aliases
spawned and restored in five frozen batches (at most 16 at a time), retaining
identity, health and position. World persistence/map-start restoration, old-protocol
rejection and two native bots also passed. The test-owned server exited normally.
The installed build is checked separately below; no graphical client was launched.

### 0.68.0 release receipt (workspace entry after packaging)

**0.68.0 is installed and independently verified**, 11 October 2026 at 07:15 EDT.
All **79/79 suites** and asset checks passed. **60 installed files**, **11 changed
file backups** and **1,691 archived source files** matched the exact ZIP/source.
The private SP executable reports 0.68.0.0. All prior 866 MP catalog rows and IDs
were preserved. The 637 other SP source files are unchanged from 0.67.0, apart
from its catalog header and release version.

Installed native run `build/mp-smoke-20261011-071546` passed all 75 actor aliases through actual
initialization, frozen save/restore and cleanup in batches of at most 16. Original
IDs, positions and health were retained. World save/load/map restore, protocol
1044, old-protocol rejection and two native bots passed too. The test DLL matched
the installed DLL, its owned process exited and installed hashes were rechecked.
Pre-package run `build/mp-smoke-20261011-071158` remains separate evidence.

This is no claim of rendered appearance, autonomous NPC behavior, vehicle boarding,
remote human play or crowd/load performance. SP's new AT-ST menu adapter also
awaits gameplay acceptance. Other map controllers and fuller multiplayer parity
remain open. Goal active; the 32 owner-accepted reports remain closed. No owner
game was launched or terminated.

- Build: `20261011-071412-81234fad`.
- ZIP: `build/JediAcademyRemixed-0.68.0-win64.zip`.
- SHA-256: `9F08ED83CC3636897F5978DC5789BD798CE15E4AEB2CECD2BDD4B7ADBA494ED1`.
- Receipt/backups: `build/deploy/20261011-071524-a1fb407c/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-071412-81234fad.zip`.
- Qualification: `build/mp-aliases-0.68.0/qualification.json`.
- Workflow log: `build/release-0.68.0.log`.

These post-package workspace notes leave immutable archives and installed docs intact.

## 0.67.0: usable native combat points (2026-10-11)

`point_combat` is now directly spawnable from the SP Entities tab. Its stock crate
control marker persists, can be moved with the Physgun, and contributes a real
point to native NPC combat selection. Hold C and open **Combat** to change native
selection flags or enable/disable it. Activate also toggles it. Status identifies
an available/reserved point, blocked placement or a missing nearby navigation node.
NPCs choose points according to their existing behavior, enemy and route checks;
this is not an order to every NPC. The investigation bit retains its retail
exclusion behavior and is labeled **Skip investigation**. Other flags only affect
NPC behaviors that use them.

Movement, edits, disabling and removal release reservations and stale temporary
combat goals before a slot can be reused. Authored map points are preserved.
Copies receive independent native slots, retain settings and participate in group
Undo. Context property edits also support Undo. Clear-space checks refresh after
moving and periodically as map geometry changes. Ordinary map spawns retain their
original navigation-building path. Existing 512-point native capacity is shared;
exhaustion rejects the new spawn without overwriting authored points.

Native saves omit the combat-point table but retain NPC indices. The new controller
rebuilds the saved stable indices from persistent entities, validates conflicts,
and restores valid reservations. SP writes **workshop save version 32** and still
reads native, recovery and workshop versions 1-31; older executables reject version
32 rather than loading unsupported point indices. Native entity layouts and the
multiplayer save/wire/API versions are unchanged.

Production tests cover native AI selection/reservation/route filters, authored
coexistence, movement, activation, blocked placement recovery, deletion/reuse,
capacity, malformed restore, entity serialization, context UI/validation/Undo and
assembly copying. Build/install qualification follows below after packaging.
Graphical AI behavior, actual in-game full saves and campaign/hub acceptance remain
open. No owner game has been launched or terminated.

Catalog totals: **1,197 cards / 63 unavailable / 187 of 211 entity-class routes /
102 portable presets**; **120 of the original 182 blocked cards** now have routes.
MP remains at **866 choices / 682 props**, protocol 1044, game/client APIs 3,
renderer API 10, feature 13 and JRW5 reads 1-5. Its 0.66.1 Weld/Hinge implementation
is preserved. Goal active; the 32 owner-accepted reports remain closed.

### 0.67.0 release receipt (workspace entry after packaging)

**0.67.0 is installed and independently verified**, 11 October 2026 at 06:58 EDT.
All **79/79 suites** and asset checks passed. **60 installed files**, **11 changed
file backups** and **1,689 archived source files** matched the exact ZIP and working
source before these receipt notes. The private SP engine identifies as 0.67.0.0.

The production native combat-point fixture passed AI selection/route/flag filters,
reservation release, authored-point preservation, transform updates, disabled and
blocked placement, periodic recovery, slot reuse, exhaustion, serialization and
stable-index restoration. Context, menu, duplicate/group Undo, entity-spawn and
save-version checks passed too. The new entity has not had graphical NPC behavior,
actual in-game full-save or campaign/hub acceptance; those remain open.

All **750 MP/shared source files** are byte-identical to 0.66.1. The MP game and
client DLLs were rebuilt, so installed run `build/mp-smoke-20261011-070000`
rechecked protocol/old-client rejection, world save/load/map restore and two native
bot connections. Its module matched the installed DLL and its owned process exited.
The broader 0.66.1 native suite remains separate historical evidence; it was not
rerun or relabeled for this release. Installed file hashes were rechecked afterward.

Catalog: **1,197 cards, 63 unavailable, 187/211 entity routes, 102 presets**;
**120/182** original unavailable cards now have routes. MP stays at 866 choices.
SP workshop saves now use version 32 and read earlier versions. Goal active;
32 accepted reports remain closed. No owner game was launched or terminated.

- Build: `20261011-065739-5ddf8f4b`.
- ZIP: `build/JediAcademyRemixed-0.67.0-win64.zip`.
- SHA-256: `307210C091149CB95F41974BE008051E6116110A3C555917D232CC163725EA65`.
- Receipt/backups: `build/deploy/20261011-065844-c8374e44/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-065739-5ddf8f4b.zip`.
- Qualification: `build/sp-combat-0.67.0/qualification.json`.
- Full workflow: `build/release-0.67.0.log`.

These receipt notes leave the immutable package and installed docs unchanged.

## 0.66.1: multiplayer Weld and Hinge (2026-10-11)

The multiplayer Tools tab now includes **Weld** and **Hinge**. Left click a first
surface, then a second entity or the world; right click cancels the pending
selection. Hinge uses the first face's normal as its rotation axis. Weld retains
the members' relative position and orientation. The compact menu shows all seven
tools, with the selected tool's instructions and properties on the right.

These links support props, NPCs, empty vehicles and consenting players. Frozen
members and the world act as anchors. Server ownership, shared editing, consent,
occupied-vehicle checks, deletion and disconnect cleanup still apply. Z undoes
the last link; the C menu's Detach attachments action removes links and thrusters.
Player self-release and opt-out also remove their links without dealing damage.

Prop assemblies use the existing rigid solver. Graphs that include native actors
use the existing native constraint solver as a whole, preserving native movement,
AI and collision controllers. Corrections are swept against the world and unrelated
entities. Held members remain constrained; native/mixed dragging is bounded to the
solver's correction range. Native actors retain their normal snapshot/event path.
Links add no permanent beam or rope visual; the firing Toolgun effect is unchanged.

Joint frames remain fixed across freeze/release, duplication, entity reuse and
save/load. Duplicated assemblies preserve internal links and share group Undo;
external world anchors are excluded with the existing copy status message.
**JRW5** stores joint types and both local frames, and reads **JRW1-4** worlds.
Malformed frames are rejected before mutation. Automatic saves retain persistent
links and omit transient player/map-actor endpoints under the existing policy.
The existing 128-attachment budget is shared by ropes, welds and hinges.

Production checks cover real rigid welds, axial hinges, native/mixed graph routing,
Physgun-constrained dragging, blocked corrections, pending NPC initialization,
immutable copied/restored frames, legacy saves, ownership and player revocation.
Native server qualification also exercises retail props, a mixed two-prop/NPC
assembly and native-only world welds for an NPC and four empty-vehicle families.
Exact installed-build evidence follows after packaging; these checks do not imply
graphical, human-network or load acceptance.

MP protocol is **1044** so older servers cannot silently retain the previous
tool when a client selects Weld/Hinge. Matching clients and servers are required.
Game/client APIs **3**, renderer API **10** and SP compatibility remain unchanged;
MP feature version is **13**. Catalog totals remain **1,197 SP cards / 64 unavailable / 866 MP
choices**, including 682 props. The 32 owner-accepted SP reports stay closed.
Remaining entity/controllers, occupied-vehicle handling, fuller context/capacity/
presentation work and gameplay/remote/load acceptance remain open. Goal active.

### 0.66.1 release receipt (workspace entry after packaging)

**0.66.1 is installed and independently verified**, 11 October 2026 at 06:28 EDT.
All **78/78 suites** and asset checks passed. **60 installed files**, **13 changed
file backups** and **1,684 source files** matched the exact ZIP and archived source
before these receipt notes. All **634 SP source files** except the release version
are byte-identical to 0.65.0. The private SP engine identifies as 0.66.1.0.

The installed dedicated server passed world welds, rotating/restored hinges,
immutable joint frames, a mixed two-prop/NPC assembly and native-only world welds
for a stormtrooper, swoop, fighter, tauntaun and walker. The native-only cases used
verified AI-decision freezing while retaining native movement; all retained their
health and anchors. Production tests additionally cover held constrained members,
consent/revocation, copy/group Undo, blocked corrections and malformed/legacy saves.

The full installed suite also passed all 126 skeletal prop default/last poses,
all 132 larger MD3 props, pose rejection, rigid floor/stack/spin, rope anchors,
mounted/legacy thrusters, NPC/vehicle forces, automatic saves/recovery/orderly quit
and two native bot clients. Eleven result files and the skeletal audit are in
`build/mp-smoke-20261011-063049`. The game DLL matched the installed module, the
owned process exited, and installed hashes were rechecked. All 866 catalog choices
are byte-identical to 0.65.0.

The server rejected a protocol-1043 connection with the expected version message.
Matching protocol-1044 clients and servers are required. The intermediate 0.66.0
archive and receipt are preserved; 0.66.1 adds that compatibility boundary. The
preliminary source run's incorrect AI-freeze flag is explicitly corrected in
`build/mp-smoke-20261011-061623/qualification-note.json`; the installed run is the
final native qualification.

These are automated headless checks, not graphical, human-network or load acceptance.
Goal active; remaining map controllers, occupied vehicles, fuller context/capacity/
presentation parity and outstanding acceptance stay open. The 32 accepted SP
reports remain closed. No owner game was launched or terminated.

- Build: `20261011-062717-2841bd25`.
- ZIP: `build/JediAcademyRemixed-0.66.1-win64.zip`.
- SHA-256: `B33C1241172D8D515D6761B7E20A8C536D77E8C1BB931E58DFE8072F96093C51`.
- Receipt/backups: `build/deploy/20261011-062823-d47943fc/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-062717-2841bd25.zip`.
- Qualification: `build/mp-joints-0.66.1/qualification.json`.
- Full workflow: `build/release-0.66.1.log`.

These receipt notes leave the immutable package and installed docs unchanged.

## 0.65.0: skeletal multiplayer props (2026-10-11)

The multiplayer spawn menu adds **126 retail skeletal prop models**, bringing it
to **866 choices** and **682 props** (556 MD3 and 126 GLM). They use the game's
models and default skins. These are decorative physics props; the NPC and vehicle
tabs retain their native controllers. Existing 740 choices remain available.

Hold C while aiming at a prop to select its pose. Previous/Next, Default and Undo
remain available; **Jump to frame** accepts a displayed frame number followed by
Enter. This is useful for humanoid skeletons with thousands of frames. The server
validates the requested pose, computes bounds from visible weighted vertices, and
checks clearance before changing the prop. Every skeletal root is frozen at that
frame. Collision is a rotated bounding box, not a ragdoll or concave mesh.

Each client maintains independent skeletal instances and clears them on entity
replacement, leaving visibility or shutdown. Pose changes discard old smoothing
history. Server physics caches posed bounds rather than rebuilding skeletons on
every simulation step. Copies, Undo and JRW4 saves retain the selected frame;
JRW1/2/3 worlds remain readable. Invalid poses leave existing props running.

Matching client and server builds are required: **MP protocol 1043, game API 3,
client-game API 3, renderer API 10 and feature version 12**. SP save/wire/APIs and
accepted gameplay behavior are unchanged. The optional MP Rend2 renderer shares
the helper; the installed MP package continues to use its vanilla renderer.

Production tests cover root posing/rollback, geometry validation, bounds caching,
independent client instances, slot reuse, cleanup, typed frame input, rejection,
Undo, copies, saved instances and snapshot replication. The client menu fixture
clicks all 866 cards. Native source run `build/mp-smoke-20261011-054128` spawned
and restored all 126 models at default and last frames, checking rotated hulls;
86 models have multiple frames. This used a spaced frozen grid above a retail map,
not a graphical or performance test. Intermediate animation frames remain checked
on demand; the test does not certify every frame in every skeleton. Exact release
and installed-server evidence follow after packaging.

Goal active: other map entities/controllers, occupied vehicle handling, Weld/Hinge,
fuller tool/context/capacity/presentation parity and human/remote/load acceptance
remain open. The 32 owner-accepted SP reports remain closed. Catalog totals:
**1,197 SP cards / 64 unavailable / 866 MP choices**.

### 0.65.0 release receipt (workspace entry after packaging)

**0.65.0 is installed and independently verified**, 11 October 2026 at 05:53 EDT.
All **78/78 suites** and asset checks passed. **60 installed files**, **13 changed
file backups** and **1,681 source files** matched the exact ZIP and archived source
before these receipt notes. All **634 SP source files** except the release version
are byte-identical to 0.64.0. The private SP engine identifies as 0.65.0.0.

The installed dedicated server spawned and restored all **126 skeletal props**
in their default and last poses, checking rotated posed bounds. **86** have multiple
frames, with up to **21,376** frames. All 740 previous menu choices remain intact.
The installed suite also passed all 132 larger MD3 prop cases, pose rejection,
independent poses, rigid floor/stack/spin, rope anchors, mounted/legacy thrusters,
NPC/vehicle forces, automatic saves/recovery/orderly quit and two native bot clients.
Ten result files and the skeletal bounds audit are in `build/mp-smoke-20261011-055339`.
Its game DLL matched the installed module, its owned process exited, and all
installed hashes were rechecked. Optional MP Rend2 compiled successfully too.

The skeletal bulk test uses a frozen spaced grid above the map. It does not certify
graphical appearance, every intermediate frame, human/remote play or performance.
Collision remains a rotated box. Matching protocol-1043 client/server builds are
required. Goal active; the current checklist records remaining work and acceptance.
The 32 accepted SP reports stay closed.

- Build: `20261011-055231-951c2a9b`.
- ZIP: `build/JediAcademyRemixed-0.65.0-win64.zip`.
- SHA-256: `889E8D204745695D6798B799875DCCA824FDA45C2CA2E3F74FC84C3648723EAF`.
- Receipt/backups: `build/deploy/20261011-055324-2af8ea06/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-055231-951c2a9b.zip`.
- Qualification: `build/mp-skeletal-0.65.0/qualification.json`.
- Full workflow: `build/release-0.65.0.log`.

These receipt notes leave the immutable package and installed docs unchanged.

## 0.64.0: larger multiplayer prop catalog (2026-10-11)

The multiplayer menu now includes **132 more retail props**, raising it from
608 to **740 choices**. All **556 available retail MD3 props** are exposed,
including ship models and wreckage, larger crates/tanks, trees, rocks, columns,
factory machinery and the animated turbine/electric-pipe models. These are
ordinary selectable prop cards with stock art; ship decorations are props,
while the Vehicles tab retains its native driveable vehicle routes.

Prop placement uses the actual selected frame's rotated bounds before allocating
an entity. Crosshair placement supports the full hull against the aimed surface
with a four-unit gap and a 2,048-unit prop aiming range. Insufficient clearance or
missing/invalid model bounds refuses that spawn, leaving existing props running.
New prop mass comes from its actual box volume within the existing 5-1,500 kg
range. Collision remains a model-oriented bounding box, including hollow models;
this release does not add concave collision meshes.

World restore and assembly paste recompute all prop hulls, including frame zero,
rather than trusting serialized bounds. A saved nonzero frame is created directly
in that pose, so a larger default pose cannot incorrectly block a smaller selected
pose. Large props retain existing grip/rotation, ropes/thrusters, context poses,
copy/Undo and JRW4 persistence. Protocol **1042**, native game API **2**, JRW1-4
reads and SP save/wire/APIs stay unchanged; MP feature version is **11**. Install
matching client/server builds to expose the same menu choices.

Production tests cover full-hull placement, offset origins on sloped surfaces,
frame-zero preflight, direct selected-pose creation and refusal without changing
existing physics. The actual client menu test traverses and clicks all 740 cards;
native snapshot tests cover large rotated bounds. Build-time asset checks validate
**556 MD3 models / 1,158 frames / 21 multi-frame models**. The native source run in
`build/mp-smoke-20261011-052147` created and restored all 132 added models, checked
their rotated selected-frame hulls, and passed existing world/pose/two-bot checks.
The bulk-model check used a spaced, frozen grid above the retail map; it is asset,
collision and persistence qualification, not playable placement or a performance
benchmark. Exact installed release evidence is recorded after packaging.

The all-objectives goal remains active. Skeletal decorative MP models, other
entity/controller families, Weld/Hinge and fuller tool/context/capacity/presentation
parity remain open, as do outstanding SP and human/remote/load acceptance. The
32 owner-accepted SP reports remain closed. No owner gameplay is launched here.
Catalog: **1,197 SP cards / 64 unavailable / 740 MP choices**.

### 0.64.0 release receipt (workspace entry after packaging)

**0.64.0 is installed and independently verified**, 11 October 2026 at 05:24 EDT.
All **77/77 suites** and asset checks passed. **60 installed files**, **10 changed
file backups** and **1,676 source files** matched the exact ZIP and archived source
before these receipt notes. All **634 SP source files** except the release version
are byte-identical to 0.63.0; the 32 accepted reports remain closed. The installed
private SP engine identifies as 0.64.0.0.

The installed dedicated server created and restored every one of the 132 added
models, checking their rotated selected-frame bounds. The asset audit covered
556 MD3 props / 1,158 frames / 21 multi-frame models. Existing pose rejection,
independent frames, rigid floor/stack/spin, world ropes, mounted and legacy thrusters,
NPC/vehicle forces, automatic saves/recovery/quit and two bot clients also passed.
Nine result files are in `build/mp-smoke-20261011-052455`. Its game DLL matched the
installed module, its owned process exited, and all installed hashes were rechecked.
The large-model check used a frozen, spaced grid above the map; graphical appearance,
playable placement, remote humans and performance are not certified by that check.

- Build: `20261011-052344-d497862d`.
- ZIP: `build/JediAcademyRemixed-0.64.0-win64.zip`.
- SHA-256: `49C3DEC9BEEB42473F41E4C794E26997665216716F132ABCBA4A331914917ED3`.
- Receipt/backups: `build/deploy/20261011-052442-320fba7c/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-052344-d497862d.zip`.
- Qualification: `build/mp-large-0.64.0/qualification.json`.
- Full workflow: `build/release-0.64.0.log`.

Overall goal active. The next catalog gap includes **126 qualified SP skeletal
prop models** excluded from MP. They need shared root-bone posing, posed bounds
and client rendering integration before exposure. Other controllers, remaining
MP tools/context/capacity/presentation and human/remote/load acceptance stay open.
These receipt notes leave the immutable package and installed docs unchanged.

## 0.63.0: multiplayer prop poses (2026-10-11)

The multiplayer C context menu now offers **Previous frame**, **Next frame**,
**Default pose** and **Undo pose** for selected catalog props. The server returns
the current frame/count, refreshes other selected inspectors after edits/deletion,
and validates ownership, the real MD3 frame and its full
rotated bounds before changing anything. A pose that intersects the world or an
entity is refused. Successful edits release a held prop, preserve its motion,
and rebuild its rigid body/rope constraints using the selected frame's bounds.
Rope and thruster anchors remain in the object's local coordinates. Native NPCs
and vehicle animation controllers are unaffected.

A bounded model/frame cache lets instances of the same model keep independent
poses without rereading assets each physics tick. Context Undo restores one pose
edit only while that object/lifetime/revision remains unchanged; it cannot undo
another player's later pose edit or affect a reused entity slot. Existing Z/group
Undo keeps its previous behavior. Frame zero is the default pose.

Copied assemblies, manual saves and automatic world saves retain the frame.
JRW4 adds a frame field to each 136-byte object record and still reads JRW1/2/3
as frame zero, preserving their existing mount and motion fields. Restore/paste
recomputes the selected hull from the installed model and rejects an unavailable
pose before creating a partial assembly. Older binaries cannot read JRW4.
The normal 16-bit entity frame field carries pose state to clients; MP protocol
**1042** and native game API **2** stay unchanged, feature version is **10**.
Use matching clients and servers. SP save/wire/APIs and all SP gameplay code
remain unchanged apart from the release version.

Production tests exercise separate same-model poses, real rigid bounds/ropes,
held release, overlap rejection, permissions, revision-scoped Undo, copy/deletion/
reuse, corrupt/unavailable frames and JRW1-4 compatibility. Snapshot tests cover
initial/delta frames including the 16-bit limit. Actual menu click/server-response
paths are tested without a window. The private native server qualification in
`build/mp-smoke-20261011-050306` checked the retail beacon's final frame with
rotated hulls, two independent instances, save/reload and transactional rejection.
The same check audits every frame of the current MP MD3 prop assets. Full release
build/install and installed native results are recorded after packaging.

The current MP prop catalog is MD3-only. Skeletal decorative models, larger
excluded models, other catalog/controller families, Weld/Hinge and broader MP
context/capacity/presentation parity remain open. The 32 accepted SP reports
remain closed, and no graphical/human/remote/load acceptance is inferred.
Catalog counts remain **1,197 SP / 64 unavailable / 608 MP choices**. Goal active.



### 0.63.0 release receipt (workspace entry after packaging)

**0.63.0 is installed and independently verified**, 11 October 2026 at 05:07 EDT.
All **77/77 suites** and asset checks passed. **60 installed files**, **10 changed
file backups**, and **1,676 source files** matched the exact ZIP, prior hashes and
archived source before these receipt notes. All **634 SP source files** other than
the release version are byte-identical to 0.62.1. The 32 accepted reports remain
closed. The installed private SP engine identifies as 0.63.0.0.

The installed dedicated server passed pose restoration, rotated bounds, separate
same-model frames and atomic invalid-frame rejection. Retail asset inspection
covered **424 MD3 props / 873 frames**, including **18 multi-frame models**.
Rigid floor/stack/spin, anchored ropes, mounted and legacy thrusters, native
world/NPC/vehicle forces, periodic/map/quit saves, revision recovery and two bot
clients also passed. Eight native result files are in
`build/mp-smoke-20261011-050814`; its game DLL matched the installed module and
the test-owned process exited. All installed hashes were rechecked. A supplemental
production-code check confirms linear/angular velocity preservation across an
offset-center prop pose/body rebuild; its fixture, binary and result are retained
under `build/mp-poses-0.63.0/supplemental-momentum*`.

- Build: `20261011-050651-d06f66db`.
- ZIP: `build/JediAcademyRemixed-0.63.0-win64.zip`.
- SHA-256: `4C3BE3EF4D3E275C029621FA81D9B581FAB502397C6E07E55CEBCDA1859472A7`.
- Receipt/backups: `build/deploy/20261011-050753-f40ffa4e/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-050651-d06f66db.zip`.
- Qualification: `build/mp-poses-0.63.0/qualification.json`.
- Full workflow: `build/release-0.63.0.log`.

These are automated/headless checks; graphical/human/remote/load acceptance is
still open. No owner game was launched or terminated. Goal active: skeletal and
large-model MP catalog support, other entity/controllers, remaining tool/context/
capacity/presentation parity and broader acceptance remain. These workspace-only
receipt notes do not modify the immutable ZIP, installed docs or archived source.

## 0.62.1: multiplayer mounted attachments (2026-10-11)

MP prop-to-prop and prop-to-world ropes now use the actual 120 Hz Box3D rope
constraint at each selected local anchor. They constrain held props and allow
rotation at off-center anchors. Native/mixed ropes retain the native force path,
but forces on their rigid-prop endpoints act at the anchor, including torque and
angular point velocity. Ropes longer than 8,192 units use the existing native
spring fallback. Visible rope geometry follows the same endpoints; ropes do not
collide with or wrap scenery. Weld/Hinge and full native constraint parity remain open.

The MP held-Q menu has a **Tools** tab: a small vertical list on the left and
selected-tool properties on the right. Rope slack ranges from 0% to 200% extra
length, default 10%. Thruster power ranges from 0.1 to 100 kN, default 3 kN.
These per-player, server-validated settings affect new ropes/mounts and last for
the current connection/map. Menu values update from the server response.
Left-click Thruster mounts at the aimed surface and points outward along its
normal; later left clicks toggle the mount. Right click removes it for remounting.
Off-center mounts rotate rigid props. Native actors retain native orientation
and movement. Held/frozen targets remain inert, and existing consent/ownership,
occupied-vehicle and lifecycle rules still apply.

Duplicated assemblies and world saves preserve each mount's local point,
direction and power. Worlds now write **JRW3**, 132-byte object records, and read
JRW1/JRW2 with the old local-up 3 kN behavior without added torque. Older binaries cannot read
JRW3; keep earlier revision backups when rolling back. SP save V31/wire 7/APIs
1022 are unchanged. MP protocol **1042**, native API **2**, feature version **9**;
use matching clients/servers. No SP gameplay caller changed: the shared bridge
only gains the fixed-anchor and bounded-impulse methods used by MP.

Production tests exercise mounted torque/limits, held/frozen behavior, per-player
settings/rate bounds, copying/slot reuse, malformed JRW3 rejection, JRW1/JRW2
compatibility, fixed/world/mixed ropes and native actor state. A new test executes
the actual cgame menu's click/draw paths, server setting responses, button gaps,
held-key lifecycle and context/wheel routing. Dedicated retail-map attachment
qualification and the full 77-suite release workflow have separate evidence;
installation is not claimed until the post-package receipt entry.

The intermediate 0.62.0 release passed 77 suites and was installed/hash-verified,
but final review found that an unmounted legacy thruster could acquire torque
from an offset model center. 0.62.1 preserves its old linear-only force path and
adds production/native regression coverage. The 0.62.0 ZIP/source/receipt remain
immutable; this patch supersedes it. Native pre-patch attachment/rigid evidence:
`build/mp-smoke-20261011-044322`. Final installed evidence is recorded below.

The overall goal remains active. The 32 accepted SP reports remain closed.
Catalog counts are unchanged: **1,197 SP cards, 64 unavailable, 608 MP choices**.
Further map-entity/controller routes, MP tools/context/catalog/capacity and
presentation parity, and owner/human/remote/load validation remain unfinished.

### 0.62.1 release receipt (workspace entry after packaging)

**0.62.1 is installed and independently verified**, 11 October 2026 at 04:48 EDT.
All **77/77 suites** and asset checks passed. **60 installed files**, **10 changed
file backups**, and **1,674 source files** matched the exact ZIP, archived source
and prior hashes before this receipt entry. **632 SP source files** were byte
identical to 0.61.0, excluding the version and two reviewed shared physics files
that add MP-only methods. Existing SP callers and the 32 accepted reports remain
unchanged. The installed private SP engine identifies as 0.62.1.0.

The installed dedicated module passed retail-map rigid floor/stack/spin and
world-anchored rope checks, mounted-thruster torque/frame/power restoration,
legacy unmounted thrusters without added torque, native NPC/world persistence,
four vehicle families, actor/vehicle forces, periodic/map/quit autosaves and
corrupt-latest-revision recovery. Two native bots connected. All seven result
files are under `build/mp-smoke-20261011-044848`. The private test DLL matched
the installed module, its owned process exited, and every installed hash was
rechecked. These are headless native/production checks, not human, visual,
remote-client or sustained-load acceptance. Those remain open.

- Build: `20261011-044742-4ece3d9f`.
- ZIP: `build/JediAcademyRemixed-0.62.1-win64.zip`.
- SHA-256: `E48AE32E178B59269EFF2B1FBFC5D82882AC54D945E30E413F923221E47A5886`.
- Receipt/backups: `build/deploy/20261011-044834-44e2b1e4/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-044742-4ece3d9f.zip`.
- Qualification: `build/mp-attachments-0.62.1/qualification.json`.
- Full workflow: `build/release-0.62.1.log`.

The intermediate 0.62.0 archive, source snapshot and receipt remain intact.
Its compatibility correction is included in this patch. These receipt paragraphs
are workspace-only additions after packaging; installed/archived documentation
stays immutable. The all-objectives goal remains active and incomplete.

## 0.61.0: multiplayer rigid props (2026-10-11)

MP catalog props now use the existing Box3D solver against exported retail BSP
brush/patch collision geometry, at fixed 120 Hz substeps. MD3 frame bounds define
oriented boxes with their actual model offsets. Edited mass and gravity drive
real bodies; props tumble, stack, retain spin and collide while being dragged or
rotated with Physgun USE. Native solid objects use lifecycle-scoped proxies.
Players/NPCs retain native movement and receive mass-weighted prop contact;
standing players are transported, and blocked rider movement rolls back the
whole prop transform. Collision sounds and firing/beam controls remain in place.

`jar_mpBox3D 1` is the default. `jar_physics_status` reports active geometry and
fixed steps. Disabling the solver freezes existing props before returning to the
native fallback. Unsupported geometry, invalid model bounds or exhausted rigid
budgets freeze props with a diagnostic instead of silently losing collisions.
This is oriented-box simulation, not per-triangle collision for individual models.
Rope/Thruster continue their existing server force implementation; the complete
SP attachment solver and tool set have not yet been ported.

Prop snapshots carry full asymmetric/rotated collision bounds and measured
motion for bounded rider prediction. MP protocol is **1042**, native game API **2**,
and `jar_mpVersion` is **8**. Clients and servers must use this matching build.
World files now write **JRW2** (100-byte object records including angular velocity)
and still read JRW1 (88-byte objects, restored with zero spin). Older JA:R binaries
cannot read newly written JRW2 worlds. Journal backups and map identity checks
remain; held/occupied autosaves park both linear and angular motion.

Production tests exercise real solver stacking/contact, edited mass/gravity,
Physgun rotation/fling, actor pushing/riding and obstruction, deletion/reuse,
rotation bounds, fresh/delta snapshots, saved spin and old-world compatibility.
Native loopback checks cover a retail floor/stack, saved spin, solver disable/
reenable, world persistence and bot clients. Full release build/install and exact
archive/hash evidence are recorded separately after packaging. No graphical
client, owner game or remote human session is launched for these checks.

The shared SP bridge gains an MP-only mass/gravity setter and include/export
compatibility guards; existing SP callers and simulation policy are unchanged.
The 32 owner-accepted reports remain accepted. Catalog totals remain 1,197 SP
cards / 64 unavailable / 608 MP choices. SP save V31, wire 7 and APIs 1022 remain.
The overall goal stays active: MP attachment/tool/catalog/capacity/presentation
parity, latency/remote-human/load qualification, remaining SP entity controllers,
and outstanding owner confirmation (including Noghri attacks/reported NPCs) remain.

### 0.61.0 release receipt (workspace entry after packaging)

Installed **2026-10-11 04:21 EDT**, build `20261011-042048-58b41853`; engine 0.61.0.0.
**76/76 suites** and retail asset checks passed. Independent verification matched
all **60 installed files**, **13 replaced-file backups** and **1,671 source files**
to the exact runtime/source archives before these receipt entries. 631 SP source
files besides the version header and three reviewed compatibility files match
0.60.0 byte for byte. Existing SP simulation callers remain unchanged.

- ZIP: `build/JediAcademyRemixed-0.61.0-win64.zip`.
- SHA-256: `2725E2038BAC683190DD93FC52C97AB83BD972AFE13FFD624A8AD0A9689FB813`.
- Receipt/backups: `build/deploy/20261011-042145-b35baabf/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-042048-58b41853.zip`.
- Qualification: `build/mp-rigid-0.61.0/qualification.json`.
- Workflow: `build/release-0.61.0.log`.
- Installed native tests: `build/mp-smoke-20261011-042211`.

The installed server passed real retail floor/stack contacts (51,516 map
triangles), saved spin/reload, solver disable/reenable, world/NPC restoration,
four frozen vehicle families, native rope/thruster cases, periodic/map/quit
persistence, corrupted-latest recovery and two bot connections. Its private test
module matched the installed DLL; all installed hashes were rechecked afterward.
The test-owned server exited. No owner gameplay was launched or terminated.

The first package was rejected before deployment because the installer allowlist
omitted the two new MP Box3D license/provenance files. That candidate and its log
are retained under `build/mp-rigid-0.61.0`; the allowlist was corrected and the full
build/test/package/install workflow reran for the final build identified above.

These receipt/status notes were added only to the workspace after immutable
archive/install verification. No release archive or installed file was rewritten.
Human/remote prediction, latency/concurrency/load, remaining MP attachment/tool/
catalog/capacity/presentation parity and outstanding SP acceptance remain open.
The overall goal is active; this release does not establish complete GMod parity.

## 0.60.0: automatic multiplayer world persistence (2026-10-11)

Hosted Sandbox worlds now save automatically every **120 seconds** and on orderly
quit, map change or restart, before native actor storage is freed. Each map/BSP
has its own automatic slot under `jar/worlds` in the separate MP home profile.
Map start restores its latest valid revision by default. The two-revision journal
verifies writes and retains the preceding revision; unchanged snapshots avoid
repeat writes. Existing manual save/load/clear commands and the file format remain
compatible. A crash can recover the latest completed periodic save; an abrupt
process exit cannot run the shutdown hook.

Server controls are `jar_worldAutoSave` (seconds; 0 disables saves, positive values
clamped to 30-3600), `jar_worldAutoLoad` (0/1), and `jar_world_status`. Interval changes
restart the timer. An explicit `jar_worldLoad SLOT` takes precedence at map start.
A failed or deliberately disabled restoration of an existing automatic world
blocks automatic replacement until a successful explicit load or clear. Failed
asynchronous NPC initialization also protects the saved world. An explicit clear
can persist an empty world. Ordinary journal write failures retry on the next
interval; a first failed write can retry its own torn record. Unknown unreadable
revisions are not automatically overwritten. After an explicit clear or successful
load, one can be replaced while retaining the other.

Automatic capture does not interrupt play. Held objects and occupied vehicles are
parked only in the saved snapshot, and restore frozen without a grip or occupants.
Their live transforms, pilots, velocity and control remain untouched. Ropes tied
to transient player/map-NPC endpoints are omitted and counted in autosave status;
owned-to-owned and world-anchor ropes persist. Uninitialized/dead catalog members
or invalid persistent records defer the entire save and retain the prior revision.
Manual saving still requires released objects and empty vehicles. Native AI tasks,
players, occupancy and campaign execution are not serialized as server-world state.

Restored objects keep the existing server ownership policy. The bundled hosting
example now sets `jar_worldEditing 1` so players can edit the restored shared world;
set 0 in your own `server.cfg` to protect it. Existing custom configuration is never
replaced. Disconnect cleanup remains governed separately by `jar_cleanupDisconnect`.

Production-module tests cover timers, runtime controls, unchanged worlds, held and
occupied capture, transient ropes, filesystem failure/retry, restoration failure,
intentional empty saves, map/BSP isolation and shutdown. Native loopback qualification
also exercises real periodic saving, map shutdown/restore, corrupted-latest recovery
and orderly quit. Full build/install evidence is recorded after packaging; fixtures
and bots do not establish human/remote gameplay acceptance.

SP gameplay is unchanged, including the 32 accepted reports and the 0.59 script
controllers. Catalog counts remain 1,197 SP cards / 64 unavailable / 608 MP choices.
The overall goal remains active: full MP physics/tool/catalog/capacity/presentation
parity, remaining SP controllers and outstanding gameplay acceptance are still open.

### Installed 0.60.0 evidence (post-package workspace entry)

**0.60.0 is installed and independently verified**, 11 October 2026 at 03:51 EDT.
Build `20261011-035057-444e4a6e`; engine file version **0.60.0.0**; MP feature version 7.
The full workflow passed **76/76 suites** and asset checks. Independent verification
matched all **58 installed files**, **12 changed-file backups** and **1,665 source
files** to the exact ZIP/stage/source archive/current tree before this receipt entry.
All **634 SP source files** other than the release-version header match 0.59.0.

- ZIP: `build/JediAcademyRemixed-0.60.0-win64.zip`.
- SHA-256: `BDC9F779D860599386B8809E3458371507715BEF1C16578ED4D1E04D96F648BD`.
- Receipt/backups: `build/deploy/20261011-035155-df2658c8/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-035057-444e4a6e.zip`.
- Qualification: `build/mp-autosave-0.60.0/qualification.json`.
- Workflow: `build/release-0.60.0.log`.
- Installed native check: `build/mp-smoke-20261011-035211/autosave-result.json`.
- Earlier source-build check: `build/mp-smoke-20261011-034815/autosave-result.json`.

The installed native server loaded the actual game DLL, performed a real timed
save at a configured 30-second interval, saved on map shutdown, isolated map slots,
restored an owned prop/world rope, recovered from a damaged latest revision and
persisted an intentional clear on orderly quit. The test also qualified native
NPC/prop world restoration and connected two native bots. The tested private DLL
matches the installed DLL hash. All test-owned processes exited. No owner process,
graphical client, public service or firewall/router change was involved.

Human/remote/late-join/load acceptance and the broader MP/SP checklist remain open;
these native checks do not establish full gameplay parity. All 32 accepted SP
reports remain accepted. The overall goal is active. Prior releases and rollback
evidence remain preserved. These notes are workspace-only receipt additions; the
installed archive and source snapshot remain immutable.

## 0.59.0: portable script tags and NPC goals (2026-10-11)

`ref_tag` and `waypoint_navgoal` are now directly spawnable SP Sandbox controllers.
They stay in the world with a stock marker model, support Physgun movement and
rotation, and provide current positions through the native script tag lookup.
Select one with C and open **Tag** to edit its name, optional script owner, goal
radius (0-1024) and On/Off state. Native activation toggles availability. Link can
supply a look-at target for the tag's direction; otherwise its rotation is used.
Navigation goals use world scope, matching native NPC navgoal scripts.

Authored mission tags take precedence. Invalid or occupied names are rejected;
inactive tags keep their names reserved. Undo retains a blocked transaction until
the old name is available. Duplicate retains settings with a unique tag name and
remaps copied entity links; it does not rewrite names embedded in script files or
literal script parameters. Full saves retain the native fields and rebuild the
portable registry, rejecting malformed or conflicting records. Deletion and map
initialization remove old registrations. Save V31 and native callback IDs stay
unchanged.

Catalog coverage is now **1,197 cards**, **101 portable presets**, **119/182**
original unavailable entries routed, **64 unavailable cards** and **186/211**
entity-class routes. Routing does not establish gameplay acceptance. These are
script reference/goal controllers, not a runtime waypoint-graph editor. Scripts
resolve the current goal when assigning it; an already assigned NPC destination
does not automatically follow a later Physgun move. Actual NPC pathfinding and
in-game save/hub behavior still need gameplay confirmation. The new Tag page is
SP-only; MP retains its 608 choices and 0.57 actor Rope/Thruster implementation.

Focused tests exercise the actual native tag registry, authored precedence,
scoped and inactive conflicts, current transforms, linked orientation, production
spawn/edit/Undo/copy/removal paths, native field serialization, restoration and UI
command validation. No graphical gameplay was launched. All 32 owner-accepted SP
reports remain accepted; remaining catalog, MP parity and acceptance work keep
the overall goal active. Release/install qualification is recorded after packaging.

### Installed 0.59.0 evidence (post-package workspace entry)

**0.59.0 is installed and independently verified**, 11 October 2026 at 03:38 EDT.
Build `20261011-033739-db939852`; engine file version **0.59.0.0**.
The complete release workflow passed **76/76 suites** and asset checks. Independent
verification matched **58 installed files**, **12 changed-file backups** and
**1,663 source files** to the exact ZIP/stage/source archive/current tree before
this receipt entry. All **580 codemp source files** match the preceding release.

- ZIP: `build/JediAcademyRemixed-0.59.0-win64.zip`.
- SHA-256: `932DB8D16527B8FE12400B3F725B2889E176DA44FD9E9914C4B7E95A1CA2C025`.
- Receipt/backups: `build/deploy/20261011-033837-dab1c0ad/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-033739-db939852.zip`.
- Qualification: `build/references-0.59.0/qualification.json`.
- Workflow: `build/release-0.59.0.log`.

Initial focused linking exposed a missing debug-draw stub in the new native tag
fixture; it was corrected before the focused and full suites passed. Native
registry tests additionally reject saved world/private namespace conflicts in
both slot orders, including inactive reservations and malformed radii. These are
automated source/runtime fixtures, not owner gameplay or a launched save test.

No owner game was running after installation. No game was launched or terminated,
and no new MP server smoke was run for this SP-only change. The prior MP force
smoke evidence remains separate. The 32 accepted reports stay closed; remaining
catalog/controllers, MP parity and unaccepted gameplay keep the goal active.
Earlier releases, backups and source archives are preserved. These receipt notes
are workspace-only additions; the installed package and snapshot remain immutable.

## 0.58.0: decorative model poses (2026-10-11)

The SP Sandbox context menu now exposes **Pose** on owned decorative props with
multiple animation frames. Hold C, select a prop, open Pose, then use Previous,
Next, Reset or enter a whole frame and Apply. The displayed range comes from the
model itself. These are static authored frames, including decorative copies of
character models; NPC animation controllers are not edited by this page.

Visible geometry, rotated native collision and Box3D shape bounds use the same
selected frame. MD3 uses that frame's bounds; GLM freezes every skeletal root and
samples the existing visible-vertex bounds path. Bounds are cached per model and
frame with a bounded map-local cache. Edits reject obstruction or unusable geometry
before changing anything. They preserve frozen state, world transform and saved
local joint anchors. The solver rebuilds the shape and its links; moving props
retain their velocity. Oversized poses keep the existing native-physics fallback.

Duplicate captures the frame and posed hull, and uses them for placement from the
start. Undo restores the previous frame only when its collision space is clear.
Full saves retain the native entityState frame and reconstruct skeletal overrides
and collision on load. No save format, engine/game API or multiplayer protocol
change is required. Normal transitions retain their existing owned-world cleanup;
hub/full-save restoration retains poses through the saved world.

Focused automated checks cover two-root skeletal overrides and rollback, malformed
headers, MD3 frame indexing, per-pose cache isolation, rotated/solver bounds,
reconstructed Box3D joints and retained motion, editor range/token validation,
blocked edits and Undo, native frame serialization/restoration, assembly copying,
and the UI controls. The save fixture uses native serializers and deterministic
engine adapters; it is not a launched save-game acceptance run. Retail metadata
qualification covers **701 present, eligible decorative models**, including
**107 with multiple frames** (maximum 21,376). One older catalog reference,
`models/map_objects/kejim/crate_01.md3`, is absent from stock assets and is explicitly
reported as an existing catalog gap, not treated as a qualified pose model.

This release does not port Pose to multiplayer. Graphical Classic/Rend2 pose,
prop/player contact, connected-assembly and in-game full-save/hub acceptance still
need real gameplay evidence. MP retains the 0.57 native actor Rope/Thruster work;
full MP parity, remaining catalog/controller features and other unaccepted items
remain open. All 32 previously accepted SP reports remain accepted. No owner game
was launched or terminated. The overall goal remains active.

Release workflow/installation evidence is appended after the exact package is
built and independently checked; these source notes alone do not establish an
installed or gameplay-validated release.

### Installed 0.58.0 evidence (post-package workspace entry)

**0.58.0 is installed and independently verified**, 11 October 2026 at 03:21 EDT.
Build `20261011-031956-b1decf48`; engine file version **0.58.0.0**.
The workflow passed **75/75 suites** and all asset checks. Independent verification
matched all **58 installed files**, **10 changed-file backups** and **1,658 source
files** to the exact ZIP/stage/archive/current source before this receipt entry.

- ZIP: `build/JediAcademyRemixed-0.58.0-win64.zip`.
- SHA-256: `297BBEC8F773D84F33B518241617574A34117FFD1A576AFE04E0ACD38906828B`.
- Receipt/backups: `build/deploy/20261011-032112-d4b6e0e1/receipt.json`.
- Source/symbol archive: `build/symbols/20261011-031956-b1decf48.zip`.
- Qualification: `build/poses-0.58.0/qualification.json`.
- Retail pose metadata: `build/poses-0.58.0/retail-metadata.json`.
- Workflow: `build/release-0.58.0.log`.

Focused compilation first used an incorrect local target name and then exposed a
missing native string-helper link in the context fixture; corrected before the
passing release. The metadata probe also identified the old missing Kejim crate
reference, now reported separately. No runtime failure was hidden or counted as
a passing gameplay test. The new pose checks include native frame serialization
and restoration, and passed in the final full suite.

No graphical gameplay or new MP server smoke was run for this SP-only change.
The installed 0.57 actor-force smoke evidence remains the prior MP qualification;
MP code is unchanged. Pose visual/contact/assembly/full-save/hub acceptance,
MP pose support, broader MP parity and the other open checklist items remain.
The 32 owner-accepted SP reports remain accepted; the goal remains active.
No owner game process was running after installation. Earlier ZIPs and backups
are preserved. These receipt notes are workspace-only additions; the installed
package and source snapshot remain immutable.

## 0.57.0: multiplayer actor ropes and thrusters (2026-10-11)

Rope and Thruster now accept ordinary NPCs, consenting players and empty native
vehicles as well as props. Ropes apply tension to both endpoints according to
mass and elapsed time, with damping; slack ropes do not push. Held/frozen bodies
and world anchors remain immovable. Thrusters push along the object's local up
axis, so rotation changes thrust direction. Alternate fire disables the thruster.
Native actors receive velocity for normal movement/collision; forces do not
teleport them, replace their animation, or invoke Force damage.

Empty attached vehicles use passive movement through native swept step/slide
collision and gravity. This preserves applied velocity through the flight/idle
controller, including fighter lift, while retaining native lifecycle updates and
animation. Occupied/boarding vehicles return to the normal driving path. This is
linear force support; off-center torque and full rigid-body angular dynamics are
still unfinished.

The C menu adds **Detach ropes / thruster**. **Release myself** and **Block tools
on me** remove player attachments immediately. Current ownership, shared editing,
map-actor permissions and player consent are checked throughout attachment life.
Death, boarding and invalid native state remove affected attachments; respawn and
connection reuse cannot inherit another player's body attachments. Retained owned
NPC assemblies transfer to server ownership under the existing disconnect policy.

Copied/restored NPC and vehicle thrusters retain their setting and new owner.
Attachments wait for delayed native initialization. Frozen objects suspend their
forces. Actor force updates preserve queued native firing/animation events, and
rope render endpoints are published after prop movement. Protocol 1041 and world
file version 1 remain compatible; existing native velocity fields are used.

Production fixtures cover command admission, mixed mass/tension, slack and frozen
endpoints, timestep checks at 10/20/40/120 Hz, local thrust orientation, native
snapshot/event preservation, consent/revocation, map/shared permissions, all four
vehicle families, copy initialization, playerless persistence and slot lifecycle.
Engine movement imports in those fixtures are adapters. Dedicated-server motion
qualification and the full build/package/install are recorded below when complete.

Remaining work includes native occupied vehicles, scenery wrapping, Box3D/full
constraint and property parity, capacity/catalog/presentation ports, and human/
remote gameplay validation. This does not close the broad tools or multiplayer
requirement. SP accepted behavior and catalog counts remain unchanged: 1,197 SP
cards, 66 unavailable, 608 MP choices. The goal is active.

### Native movement qualification before packaging

The actual dedicated module passed the isolated `mp/ffa5` checks in
`build/mp-smoke-20261011-025935`: stock NPC baseline, NPC rope and thruster motion,
and thruster lift for swoop, TIE fighter, tauntaun and AT-ST. Force cases used
normal gravity 800; the stationary baseline used gravity 1. Native AI decisions
were disabled only in the private test profile, retaining native movement,
collision, vehicle updates and animation. Health stayed at the supplied value
10, below native overhealth-decay thresholds. World/rope restoration, map reload
and two native bot clients also passed. No graphical client or owner game ran.

The early zero-gravity baseline entered the retail zero-G shove path, and health
above the NPC's native maximum decayed normally; the final controlled fixture
avoids both confounders. A real fighter failure exposed flight-controller
velocity replacement and led to the passive collision integration above. Prior
failed runs remain preserved. Full 75-suite release and installed-file evidence
are recorded in later workspace receipt entries; human acceptance remains open.

### Installed 0.57.0 evidence (post-package workspace entry)

**0.57.0 is installed and independently verified**, 11 October 2026 at 03:02 EDT.
All **75/75 suites** and asset checks passed. **58 installed files**, **10 changed
file backups**, and **1,656 source files** matched the exact package, prior hashes
and archived source before these receipt notes. Engine file version is 0.57.0.0.

The installed dedicated server repeated the native NPC baseline, Rope/Thruster
motion and thrust lift for swoop, TIE fighter, tauntaun and AT-ST. All six force
cases used gravity 800 and produced positive vertical displacement; health stayed
10. AI decisions were disabled in the private test profile while native movement,
collision and vehicle update paths ran. The world/rope save/load and map-start
checks passed, followed by two native bot clients. No owner game or graphical
client was launched or terminated. Human/remote acceptance is still outstanding.

- Build: `20261011-030131-7090d40b`.
- ZIP: `build/JediAcademyRemixed-0.57.0-win64.zip`.
- SHA-256: `E3FC325EAF188E73FEE8336F75C94DC7413F62E0CE591E8287BBCEECAF35293A`.
- Receipt/backups: `build/deploy/20261011-030227-8264fb01/receipt.json`.
- Qualification: `build/mp-forces-0.57.0/qualification.json`.
- Installed native evidence: `build/mp-smoke-20261011-030248/force-result.json`,
  `world-result.json` and `result.json` in the same directory.
- Source/symbol archive: `build/symbols/20261011-030131-7090d40b.zip`.

These are post-package workspace notes. The exact ZIP, installed package, source
archive and earlier releases remain unchanged. The all-objectives goal is active;
remaining SP work, full MP parity and gameplay acceptance have not been closed.

## 0.56.0: multiplayer vehicle manipulation (2026-10-11)

Empty native vehicles now support Physgun holding, wheel distance, USE + mouse
rotation, right-click freeze, Toolgun Freeze/Release and C-menu rotation/release.
Vehicle animations remain native; the humanoid choke pose is not applied. Control
suspends vehicle movement, throttle, active turbo and angular drift. Release hands
linear fling velocity and forward speed back to the native controller. Native
vehicle orientation is kept consistent across entity and vehicle snapshots.

The existing native hull routine is shared without changing its calculations.
Fighter/flier collision bounds follow orientation; other families retain the
engine's fixed hull policy. Vehicle spawn and restored hulls are checked against
the actual world. A blocked rotation leaves orientation and collision bounds
unchanged. Copy/world restoration retains frozen vehicles, native health/armor,
position and orientation after delayed NPC initialization, including when no
human player is connected.

Boarding, a pilot/passenger/old-pilot transition, dying vehicles, invalid native
state and pending initialization exclude control. An already held/frozen vehicle
releases as boarding starts, allowing normal driving. Remove, Undo, cleanup,
copy and persistence all recognize occupied/boarding transitions. Spawned vehicles
use owner/shared editing; map vehicles use the existing map-actor permission and
server-owned vehicles use world-edit permission. Actor context Remove now works
for editable spawned NPCs/empty vehicles while player slots remain protected.

Production fixtures cover speeder/fighter/animal/walker control, native hull
rotation and obstruction, unchanged animation/Force state, momentum handoff,
boarding/occupied/death/ownership cleanup, frozen copies and playerless save/load.
The native vehicle-playerstate serializer is checked for fresh frozen snapshots
and release deltas. The actual dedicated server restored frozen `swoop_mp`,
`tie-fighter`, `wildtauntaun` and `atst_vehicle` in clear `mp/ffa5` airspace with
zero connected players; all retained health 230 and exact position/orientation.
The ordinary world/rope/map-reload checks and two native bot connections also
passed (`build/mp-smoke-20261011-023254`). Tight player spawn areas rejected the
larger hulls without leaving objects behind; the successful candidates came from
large non-opaque BSP leaf boxes and passed the unchanged native world trace.

Engine imports for input/movement in the fixtures remain adapters. Human Physgun
interaction, driving after release, remote prediction and contacts still need
gameplay qualification. Full 75-suite build/package/install is pending at source
snapshot time; subsequent workspace evidence entries record its exact result.

SP accepted behavior and catalog counts are unchanged: 1,197 SP cards, 66 still
unavailable, 608 MP choices. SP save V31/wire 7/APIs 1022, MP protocol 1041 and
world-file version 1 are unchanged. This remains a partial MP port: occupied
vehicle manipulation, broader actor/constraint/tool properties, Box3D/capacity,
map entities, presentation and human/remote testing remain open. The full goal
is active and unfinished.

### Installed 0.56.0 evidence (post-package workspace entry)

**0.56.0 is installed and independently verified**, 11 October 2026 at 02:35 EDT.
All **75/75 suites** and asset checks passed. **58 installed files**, **10 changed
file backups**, and **1,653 source files** matched the exact package, previous
hashes and archived source before these receipt entries. The installed server
repeated the four-family frozen vehicle restore on `mp/ffa5`, preserving health,
position and orientation with zero players connected. Native world/rope save/load,
map-start restore and two native bot connections passed as well. No owner game
or graphical client was launched or terminated. Human interaction, driving after
release and remote multiplayer acceptance remain open; the overall goal is active.

- Build: `20261011-023443-ed357437`.
- ZIP: `build/JediAcademyRemixed-0.56.0-win64.zip`.
- SHA-256: `A0FD4528951ED0CACB135386A4D4C54E2DD97A72368D05345155793F9D86AE04`.
- Receipt/backups: `build/deploy/20261011-023533-c91a6cf3/receipt.json`.
- Qualification: `build/mp-vehicles-0.56.0/qualification.json`.
- Installed native evidence: `build/mp-smoke-20261011-023616/vehicle-result.json`,
  `world-result.json` and `result.json` in the same directory.
- Source/symbol archive: `build/symbols/20261011-023443-ed357437.zip`.

These receipt notes were added to workspace documentation after packaging; the
exact release ZIP, installed package and archived source have not been rewritten.

## 0.55.0: multiplayer world persistence (2026-10-11)

Server-console/RCON commands `jar_world_save SLOT`, `jar_world_load SLOT`,
`jar_world_clear` and `jar_world_status` now preserve a spawned Sandbox world.
Saves include catalog props, pickups, NPCs, unoccupied vehicles, internal ropes
and world-anchored ropes. They retain transforms, prop mass/gravity/thruster/freeze
and velocity, pickup quantities, native health and eligible NPC freeze state.
Native AI/scripts restart from the catalog definition; players and occupants are
not serialized. Held, occupied, uninitialized or unsupported members reject saving
with a reason. Authored map entities are not modified by world clearing.

Loading requires an empty spawned Sandbox world and the original map/BSP checksum.
It validates the whole file, catalog, resource capacity and placement before
restoring server-owned objects. `jar_worldEditing 0` protects these by default;
set it to `1` to let players edit them independently of `jar_sharedEditing`.
A native actor failure or three-second initialization timeout removes its pending
group. NPC transforms are reapplied after native floor settling, with a fresh
collision check; this also fixes displaced NPCs in duplicated assemblies.

Slot names use 1-32 letters, digits, underscores or hyphens. Each slot stores two
alternating checked revisions as `jar/worlds/SLOT.0.jrw` and `.1.jrw` under the MP
home profile. Write/readback verification retains the previous valid revision;
loading can recover it if the other file is invalid. Unknown/corrupt pairs are
preserved rather than overwritten. Records use bounded little-endian values,
content checksums and stable catalog IDs, with no native pointers/client slots.

Set `jar_worldLoad "SLOT"` before a map command to restore once at map start.
Saving is manual: use `jar_world_save` before quitting or changing maps. There is
no autosave or native AI execution checkpoint. Vehicle/player interaction,
remote/human testing and long-running persistence qualification remain open.

Production tests cover corruption, truncated I/O, revision recovery, map identity,
server-only command admission, world ownership, world anchors, no-player native
readiness, occupied-vehicle refusal and protection of authored entities. The
actual dedicated server also restored a stock prop, frozen stormtrooper and two
ropes with zero connected players, saved/reloaded their properties, restored at
map start and connected two bots (`build/mp-smoke-20261011-020923`). No graphical
client or owner game was launched. Full 75-suite build/package/install is pending
at source-snapshot time; later workspace receipt entries record its exact result.

SP accepted behavior, catalog 1,197/66 unavailable, MP 608 choices, save V31,
wire 7, APIs 1022 and MP protocol 1041 are unchanged. The overall goal remains
active. Remaining work includes MP capacity, Box3D/vehicle/tool/context/catalog
and presentation parity, SP catalog/poses/controller gaps and human acceptance.

### Installed 0.55.0 evidence (post-package workspace entry)

**0.55.0 is installed and independently verified**, 11 October 2026 at 02:11 EDT.
All **75/75 suites** and asset checks passed. **58 installed files**, **11 changed
file backups**, and **1,651 source files** matched the exact package, previous
hashes and archived source before these receipt entries. Installed engine version
is 0.55.0.0. The installed server restored a frozen stock NPC and prop with their
saved transforms/properties and internal/world ropes, with no players connected;
save/reload and map-start restore passed, followed by two native bot connections.
No owner game or graphical client was launched or terminated. This is bounded
native dedicated-server evidence, not human/remote multiplayer acceptance.

- Build: `20261011-021055-f524019b`
- ZIP: `build/JediAcademyRemixed-0.55.0-win64.zip`
- SHA-256: `BF32988E36C33F131FF7E1DE0CF311F7A38F09D3950E8B193FB1B33A1E7423D4`
- Receipt/backups: `build/deploy/20261011-021148-2a08c33b/receipt.json`
- Independent qualification: `build/mp-world-0.55.0/qualification.json`
- Full workflow: `build/release-0.55.0.log`
- Installed native world/bot tests: `build/mp-smoke-20261011-021205/world-result.json`
  and `build/mp-smoke-20261011-021205/result.json`
- Exact source/symbols: `build/symbols/20261011-021055-f524019b.zip`

Earlier releases and rollback evidence remain preserved. The overall goal stays
active; human validation, MP capacity/Box3D/vehicle/tool/catalog/presentation parity
and SP catalog/controller/pose acceptance remain open. These workspace notes
postdate packaging and do not change the installed ZIP or source archive.

## 0.54.0: multiplayer assembly duplication (2026-10-10)

The MP Duplicate tool now uses **right click to copy** and **left click to paste**.
Each player has an independent server-owned clipboard. Copy follows connected
ropes through spawned props, pickups, NPCs and unoccupied vehicles; every object
and internal rope must be editable by that player. World anchors are excluded
with a count in the copy feedback. Sources are never modified; deleting/reusing
them does not alter the snapshot. Player bodies and native mission/AI execution
state are not cloned. Actors restart their catalog definition.

Paste retains relative placement, orientation, prop mass/gravity/thruster/freeze,
pickup quantities, NPC/vehicle health, eligible NPC freeze state and internal rope
anchors/rest lengths. It checks whole-group space and resource capacity, restores
independent endpoint references, and rolls back synchronous failures including
native helpers. A single Undo removes the complete pasted group. NPC properties
apply after native initialization; failed or stalled members cancel the pending
group after at most three seconds. Occupied vehicles defer failed-group removal
until their occupants leave. Undo/cleanup refuse occupied vehicles; disconnect
retains their whole group as world-owned instead of deleting another player's
vehicle. Pending groups and clipboards never transfer to a new client-slot user.

Native NPC spawn placement now accepts the intended assembly transform before
initialization; legacy console/mission callers retain their existing placement.
Owned NPC goals and vehicle droid goals are removed on cleanup. Freeing a held
actor explicitly unlinks it after restoring its temporary pose, preventing the
final restoration snapshot from relinking a deleted body.

Production server fixtures cover graph/property copies, source slot reuse,
client isolation/permissions, world-anchor exclusion, clear-space/model/entity
admission, item-rule rejection, synchronous rollback, delayed NPC failure/timeout,
health/freeze readiness, grouped Undo and occupied-vehicle/helper lifetimes.
Native parser/physics/animation imports are fixture adapters; this does not prove
human multiplayer, late joins, lag behavior or native vehicle gameplay. Full
build/package/install is pending at source-snapshot time; later workspace receipt
entries record the exact result. The overall goal remains active and unfinished.

Catalog remains 1,197 SP cards / 66 unavailable and 608 MP choices. Save V31,
workshop wire 7, APIs 1022 and MP protocol 1041 are unchanged. Remaining work
includes multiplayer Box3D, capacity, additional tools/controllers, world
persistence, presentation parity and human/remote/load qualification, along with
the unresolved SP catalog, poses, Rope and acceptance requirements.

### Installed 0.54.0 evidence (post-package workspace entry)

**0.54.0 is installed and independently verified**, 10 October 2026 at 21:44 EDT.
All **75/75 suites** and asset checks passed. **58 installed files**, **10 changed
file backups**, and **1,648 source files** were checked against the exact package,
previous destination hashes and source archive before these receipt notes.
The installed dedicated server loaded `mp/ffa3` with protocol 1041 and two native
bots in an isolated loopback test, then exited. No owner game or graphical client
was launched or terminated. Human assembly/vehicle/late-join testing remains open.

- Build: `20261010-214354-3619ed69`
- ZIP: `build/JediAcademyRemixed-0.54.0-win64.zip`
- SHA-256: `59C2B6043506BFC499AB4A6B41E0D9C6B10FE6B872F7716CC8A5910EC3E233FF`
- Receipt/backups: `build/deploy/20261010-214449-cfa4b329/receipt.json`
- Independent qualification: `build/mp-duplicate-0.54.0/qualification.json`
- Full workflow: `build/release-0.54.0.log`
- Installed-server smoke: `build/mp-smoke-20261010-214540/result.json`
- Exact source/symbols: `build/symbols/20261010-214354-3619ed69.zip`

The test harness uses native-service adapters for per-player tool input and NPC
initialization; the two-bot server smoke verifies native startup/connectivity,
not in-game Duplicate interaction. Earlier releases and rollback evidence remain
preserved. The goal is active, with the unresolved requirement rows still open.
These workspace receipt entries postdate packaging and do not change its bytes.

## 0.53.0: portable map-transition controller (2026-10-10)

`target_level_change` is directly spawnable from the SP entity catalog as a
visible stock marker. Select it with C, open **Travel**, enter an installed map
name (for example `t1_surprise`, without `maps/` or `.bsp`) and an optional named
spawn point, choose normal or hub travel, then Apply. **Activate** performs the
native map transition. The default destination is the current map. Hub travel
saves the current map state and can restore an existing destination state.
Spawning, editing and duplicating do not initiate travel. Undo restores settings
before departure; it cannot reverse a completed map transition.

Destination names and installed BSP availability are validated in the game,
including at activation. Hex-encoded UI requests preserve an empty spawn point
and reject console syntax, traversal, oversized input and stale selections.
The callback rechecks entity lifetime after use behavior. Authored mission
transitions retain their existing story/menu handling. The portable controller
does not rewrite story/menu cvars when spawned or activated. Link can activate
it from another spawned controller; its destination spawn point is kept separate
from same-map target wiring, duplicate graphs and deletion cleanup. Copy/paste
preserves portable settings and rolls back if the destination is unavailable.
Copying an authored story transition is rejected instead of silently converting
its mission behavior into a portable transition.

Settings use existing native saved message/target/spawnflags fields and the
existing callback ID. Save V31 / workshop wire 7 / APIs 1022 / MP protocol 1041
are unchanged. Catalog: **1,197 cards, 99 portable presets, 117/182 original
unavailable entries routed, 66 unavailable total, 184/211 entity-class routes**.
MP remains 608 choices; this SP controller does not complete the multiplayer port.

Focused production-code fixtures pass for portable spawn/activation, invalid or
missing destinations, use-time deletion/reuse, native serialized fields with
string-fixup boundary simulation, UI visibility/commands, Undo, duplication and
Link behavior. This is not a graphical save/reload or campaign travel test.
The full 75-suite build/package/install is pending at source-snapshot time; see
the post-package receipt below (when present) for installed verification.
Owner acceptance of the 32 annotated SP items remains unchanged. The overall
goal remains active: remaining catalog/property work, rope integration and full
hosted multiplayer parity/testing are still open.

### Installed 0.53.0 evidence (post-package workspace entry)

**0.53.0 is installed and independently verified**, 10 October 2026 at 21:29 EDT.
The full workflow passed **75/75 suites** and asset checks. All **58 installed
files** match the exact ZIP; **11 replaced files have verified backups**. All
**1,646 archived source files** matched the workspace before these receipt notes.
The installed engine reports version 0.53.0.0. No graphical game was launched
or terminated. Map travel, hub return and graphical save/reload remain untested.

- Build: `20261010-212853-aa787757`
- ZIP: `build/JediAcademyRemixed-0.53.0-win64.zip`
- SHA-256: `DA9F2C49867F161698B56E24833C2419B7C1A8C0A9F0C447FA0989BB111FD4E1`
- Receipt/backups: `build/deploy/20261010-212949-562e5011/receipt.json`
- Independent qualification: `build/level-change-0.53.0/qualification.json`
- Build/test/package/install log: `build/release-0.53.0.log`
- Exact symbols/source: `build/symbols/20261010-212853-aa787757.zip`

An initial full compile found a missing callback-header dependency; it was fixed
before the successful release. The failed compile log remains at
`build/release-0.53.0-compile-attempt.log`. No failed candidate was installed.
These workspace receipt notes postdate packaging; the exact release and source
archives are unchanged. This milestone does not complete the active goal.

## 0.52.1: multiplayer actor Physgun control (2026-10-10)

The MP Physgun can hold living NPCs and active players, rotate them with USE +
mouse, change distance with the wheel, freeze them with alternate fire, and
release with measured fling momentum. The server sweeps their native hulls and
publishes native player/NPC snapshots. Holding consumes no Force points and does
not activate Force Grip's damage logic. Humanoids use the native choke pose;
nonhumanoids keep their own animations. Ordinary environmental/combat damage and
post-release fall damage are not disabled.

Toolgun Freeze/Release and the C menu now support these actors. Actor context
properties are Freeze, Release and yaw rotation while held/frozen. Other prop
properties are rejected for actors; the Remove tool cannot free player slots.
Player clients and NPC AI suspend normal movement/actions while controlled;
release restores movement and unwinds the temporary pose. NPC snapshot type is
preserved. Respawn, death, disconnect, deletion/reuse, intermission, server-policy
changes and Sandbox disable release control. Respawn/disconnect also invalidate
other players' selections of that player slot, so stale context actions cannot
freeze a newly spawned body. Taking a player releases anything
that player was holding, preventing control chains.

`jar_playerTools 1` allows player control and `jar_mapActorEditing 1` allows
unowned map NPCs. Spawned NPCs retain owner/shared-edit permissions. Players can
always use C / **Release myself** (one-second regrab grace), or **Block tools on
me** / **Allow tools on me**, regardless of request throttling or frozen state.
Console equivalents are `jar_release` and `jar_allowTools 0` / `1`. The opt-out
lasts for the current connection/map, survives respawn, and does not transfer to
a new occupant of the player slot. It does not override server restrictions.

Vehicles, vehicle occupants, spectators, dead actors, duel participants,
monster-held actors and actors already in native Force Grip remain excluded.
Vehicles and remaining native/controller/tool support are still open work, not
silently treated as complete. MP Thruster/mass/gravity remain prop-only, and
assembly duplication, world persistence, Box3D/capacity/presentation parity and
human/remote/late-join/load qualification remain unfinished.

Regression checks exercise production server control/command/lifecycle code,
with fixture adapters for native animation/view/snapshot imports. Native network
serialization tests cover fresh and delta held/released player state. An isolated
loopback dedicated-server check loaded the actual game DLL, `mp/ffa3` and two
native bots (`build/mp-smoke-20261010-211049`). These checks do not prove graphical
actor motion, remote prediction or multiplayer gameplay acceptance. Full release
build/test/package/install results are recorded separately in the receipt.

SP behavior, the 32 accepted items, catalog counts (1,197 SP / 608 MP), SP save
V31/wire 7/APIs 1022 and MP protocol 1041 are unchanged. Goal remains active and
unfinished. No owner game was launched or terminated.


### Installed 0.52.1 evidence (post-package workspace entry)

**0.52.1 is installed and independently verified**, 10 October 2026 at 21:15 EDT.
All **74/74 suites passed**; **58 installed files** match the exact ZIP, **9
changed-file backups** match their prior hashes, and **1,642 source files** matched
current source and the archived snapshot before these receipt entries were added.
The installed dedicated executable/game module loaded `mp/ffa3` and connected
two native bots in an isolated loopback test. This is not graphical/human-client
actor validation; that acceptance remains open.

- Build: `20261010-211502-0469bd13`
- ZIP: `build/JediAcademyRemixed-0.52.1-win64.zip`
- ZIP SHA-256: `BE6E6761A4A481E5AE9B20482506C3037117D8E6EA94C47634DC56046E4A1694`
- Receipt/backups: `build/deploy/20261010-211553-4728c212/receipt.json`
- Qualification: `build/mp-actors-0.52.1/qualification.json`
- Full workflow: `build/release-0.52.1.log`
- Symbols/source: `build/symbols/20261010-211502-0469bd13.zip`
- Installed native-server smoke: `build/mp-smoke-20261010-211607/result.json`

The intermediate 0.52.0 ZIP, source archive and receipt remain preserved. Final
review found and repaired selection lifetime across player respawn/disconnect;
0.52.1 contains the regression check. No owner game was launched or terminated.
No public service, firewall or router configuration was changed. The final ZIP
and its source archive are immutable evidence; these are workspace-only notes.
The all-objectives goal remains active and unfinished, with the 32 accepted SP
items preserved. Vehicles, broader MP parity, the 67 blocked SP catalog entries,
remaining context/pose/controller work and outstanding gameplay acceptance remain.

## 0.51.0: restored native pickups and holocron properties (2026-10-10)

Eleven formerly unavailable pickups are usable from the Sandbox menu: nine
Force holocrons (Heal, Jump, Speed, Push, Pull, Mind Trick, Grip, Lightning and
Saber Throw), `item_bacta`, and `weapon_stun_baton`. They reuse unedited stock
MP holocron/bacta models and the stock baton world model. Native item classes,
power tags, icons, sounds, quantities and pickup effects are retained.

After reading items.dat, the engine repairs only the exact known missing retail
model paths with the matching class/type/tag and an existing replacement.
A supplied original model or mod-specific path/type/tag takes precedence.
This also repairs ordinary native item registration; randomizer candidate names
and selection algorithms are unchanged. Workshop skeletal pickups receive the
Ghoul2 instance normally created by FinishSpawningItem, including failure cleanup.
The baton retains its native view model, barrel parts and attack behavior.

Select a holocron with C and use **Properties / Force level / Apply Force level**
to choose 1, 2 or 3. The level is the native saved item count. Pickup learns or
upgrades the corresponding power and never lowers a known higher level. Editing
supports Undo, validated selection tokens and duplication; collection/deletion
invalidates old edits. A prior authored level 0 can be restored by Undo. Native
invalid power tags/levels are rejected before indexing player state. These are
SP Force upgrades, not MP Holocron-mode possession effects. Bacta remains the
native 25-health pickup, not a carried bacta canister. Normal pickup expiry applies.

Coverage: **1,197 cards**, **98 portable presets**, **116/182** original unavailable
entries now routed; **66 original plus one empty placeholder (67 total)** remain
unavailable. **183/211** entity-class routes are unchanged. Of the additional
pickup cards, only `weapon_jawa` and `item_enviro` remain unavailable: the former
has no retail world model, and the latter needs a functional shield controller
and valid inventory tag. These are still open requirements.

MP adds the stock baton pickup, bringing its catalog to **608**. Server pickup
requests now supply their native classname, isolate default wait/random fields
from the last BSP spawn, and roll back both disabled-item return paths. Native
item/weapon server restrictions remain authoritative. SP holocron upgrades and
its context page are not newly ported to MP.

Checks exercise the actual item parser/resource repair, all nine native Force
grants, higher-level preservation, malformed tags/levels, skeletal pickup setup,
context bounds/Undo/stale tokens, UI commands/visibility, duplication and native
count serialization. Independent retail checks cover models, embedded material
images, icons, pickup sound and baton view parts. MP command fixtures cover the
baton spawn, ownership, Undo, isolated spawn keys and disabled-item cleanup.
The full workflow now runs **74 suites**. No graphical gameplay is claimed.

SP saves remain **V31 `0x4a415220`**, workshop wire 7, APIs 1022; MP protocol 1041
and saved callback IDs are unchanged. Full multiplayer parity, remaining catalog
controllers/properties/poses/Rope work and outstanding gameplay acceptance remain
open. Previously accepted SP items remain accepted; the completion goal is unfinished.

### Installed 0.51.0 verification (post-package workspace entry)

**0.51.0 is installed**, completing the previously blocked workflow after the
owner closed both JA:R instances. All **58 installed files** independently
byte-matched the exact verified ZIP; **13 changed-file backups** matched their
previous hashes. The release passed **74/74 suites** before packaging, with
**1,640 source files** verified before post-package workspace receipt entries.

- Installed: 10 October 2026, 20:48 EDT
- Build: `20261010-180638-3163d69d`
- ZIP: `build/JediAcademyRemixed-0.51.0-win64.zip`
- ZIP SHA-256: `942D907405CB221ED8B19D8000A5B523FB49250327C56FE0FA8785D3CEF1A0A7`
- Receipt/backups: `build/deploy/20261010-204812-88a7f5d6/receipt.json`
- Qualification: `build/pickups-0.51.0/qualification.json`
- Deployment log: `build/pickups-0.51.0/deployment.log`
- Symbols/source: `build/symbols/20261010-180638-3163d69d.zip`

No game was launched or terminated. Package/source archives remain unchanged;
this receipt updates workspace documentation only. Existing 0.50 rollback
artifacts are retained. New pickup gameplay and full multiplayer parity remain
unverified/unfinished. Goal state was not changed by this installation.

## 0.50.0: native legacy droid spawning (2026-10-10)

Q / NPCs now admits `mouse` and `remote_sp` using the single-player engine's
native segmented model renderer. Q / Entities also enables `NPC_Droid_Mouse`
and `NPC_Droid_Remote`. These actors retain their retail models, embedded
materials, animation files, AI classes, weapons and definition bounds.

Loaded-definition validation now checks required lower/upper/head models,
required part skins and the legs animation file. The spawn path probes model
and skin registration and checks animation-cache capacity before creating an
actor. Missing resources fail with a specific message. No substitute actor
is used for `seeker` or other genuinely missing model families. Legacy legs
with a skin suffix remain unsupported because the native NPC/client animation
cache paths disagree. Seeded campaign randomizer pools are unchanged.

Coverage remains **1,197 cards** and **98 portable presets**: **105 of the
original 182 unavailable entries now have routes**, leaving **77 original plus
one empty placeholder (78 total)** unavailable. **183/211** entity-class cards
have routes. The MP catalog remains **607**; SP legacy actor rendering is not
ported to MP by this change. SP save **V31 `0x4a415220`**, wire 7, APIs 1022,
MP protocol 1041 and saved callback IDs are unchanged.

Production catalog/actor fixtures cover opt-in admission, missing models,
skins, animation files, model/skin initialization failure, full animation cache,
bounds and campaign-pool stability. Retail qualification checks both MD3s'
visible surfaces, triangles, frames, embedded material images and animation
ranges. The loaded-definition fixture admits both retail droids alongside
Desann, Mon Mothma and RebornBoss. These checks are not gameplay acceptance.
No graphical client is launched for this milestone.

## 0.49.0: visible ropes and initial multiplayer Sandbox (2026-10-10)

New checks exercise rope mesh geometry, actual MP server command/lifecycle code,
request policy and actual MP snapshot serialization. Existing SP suites remain
required. The dedicated smoke starts only a loopback test-owned process, loads
the real DLL and retail mp/ffa3, verifies protocol 1041/Sandbox status, and connects
two native bot clients before shutting down. This is not a two-human-client test.

Ropes now submit persistent 3D mesh geometry in SP and MP. Automated geometry
checks do not establish in-game appearance. No graphical client was launched.
Full multiplayer feature parity and live remote/load tests remain open; detailed
scope and limitations are in JAR-multiplayer.md. The owner's 32 annotated SP
acceptances are recorded in JAR-owner-acceptance-2026-10-10.md and supersede older
pending-owner-test statements for those exact items.

The release script builds/tests/packages and attempts normal verified deployment.
Check build/deploy receipts for actual installation; a running game blocks it.

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

## 0.46.0 portable crystal crate (2026-10-10)

`misc_crystal_crate` is directly spawnable from Q / Entities. Its original open
crate and hit effect are absent from retail JA. The portable assembly uses an
unedited Imperial crate and blue crystal, the stock crystal icon, stock sparks,
thermal explosion/metal debris effects and cargo explosion sound. Native
map-authored crates retain their existing model and behavior.

The crate starts with 80 health and native mass 40. A hit pulses 16 damage within
32 units; destruction or Activate detonates it for 40 damage within 80 units.
Both paths use native radial damage with falloff and line-of-sight checks.
Hit pulses are limited to one per 100 ms to prevent recursive neighboring-crate
feedback; direct weapon damage still applies on every hit. Death clears damage
and use callbacks before firing targets or blast damage. Generation checks
prevent callbacks deleting/reusing a root from acting on its replacement.

**C / Blast** edits whole hit/blast damage (0-1000), hit radius (1-1024), whole
blast radius (1-1024) and Solid/Shot-only collision. Zero damage retains effects.
Enabling solidity rejects an obstructed placement. Undo restores properties
before destruction; health remains under Properties. The native menu allocation
is increased from 150 to 256 items because hidden context-page fields also count.
The headless UI checks construct the entire menu and exercise the new page.

Physgun movement/USE rotation and part selection keep the assembly together;
Rope, Weld/Hinge and Thruster aiming at the crystal uses its root. Rotated bounds
enclose both retail models. Setup reserves one helper and rolls back allocation
or collision failures; deletion removes the crystal and clears reciprocal links.
Duplicate retains health, settings and owned parts using the regenerated helper.
Native saved fields retain settings; restore validates them and resets transient
hit timing. Save **V29 `0x4a41521e`**, wire 7, APIs 1022; callback IDs unchanged.
Earlier supported save readers remain present; full native save/load remains open.

Coverage: **1,197 cards**, **98 portable presets**, **101 of the original 182**
unavailable entries have routes; **81 original plus one empty placeholder
(82 total)** remain unavailable. **181/211** entity-class cards have routes.
The release workflow now contains **68 suites**. Targeted checks cover pulses,
death chains, deletion/reuse during callbacks, rotated assembly bounds,
solidity validation, saved fields, setup rollback, context/Undo, duplication,
attachment targeting and UI command validation. Evidence: `build/crystal-crate-20261010/`.

No game was launched. Native visuals/sound, full-save/campaign/performance
acceptance, remaining catalog/tool/pose work and earlier gameplay reports remain
open. The all-objectives goal stays active.
The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.46.0 is installed and independently verified**: **68/68 automated suites**,
**37 installed files** matched to the exact ZIP, **9 changed files**
backed up, and **1006 implementation/build/test snapshot files** matched.
Saved callback source files match the preceding 0.45.0 snapshot byte for byte.

- Build: `20261010-162553-7fd8ad81`
- ZIP: `build/JediAcademyRemixed-0.46.0-win64.zip`
- SHA-256: `F856A19D1A90038922BE669981FCBF8D801068C61A8FFC533332D4117B785295`
- Receipt/backups: `build/deploy/20261010-162652-4df76aee/receipt.json`
- Controller tests/assets/logs/result: `build/crystal-crate-20261010/`
- Symbols/source: `build/symbols/20261010-162553-7fd8ad81.zip`

The initial focused run exposed the context menu's 150-item allocation limit
and an incomplete duplicate spawn fixture. Both were corrected; the full
release run includes all final checks. The initial build command also used an
incorrect game target name; the successful build uses `jar_gamex86_64`.
Initial and corrected logs are retained with the evidence above.

This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; native visual/full-save/campaign/performance checks and the
remaining overhaul checklist are open. The 0.45.0 ZIP remains available.

## 0.45.0 portable extending door camera (2026-10-10)

`misc_model_jabba_cam` is directly spawnable from Q / Entities. The native
entity extends/retracts its eye; it does not switch the player's view. Its
original Ghoul2 model is absent from retail JA. The portable assembly uses an
unedited stock panel, telescoping Imperial camera mount and camera head, plus
the retail camera servo loop and zoom icon. Native map-authored callbacks remain.

**C / Camera** edits travel (8-256 units beyond the retracted position), speed
(1-512 units/second), and Extended/Retracted. Activate reverses the requested
position. Refresh shows extension and obstruction state. The eye sweeps its
hull against solid surfaces and actors in both directions and retries blocked
movement. Inactive controllers pause without accumulating motion on resume.
Servo sound stops at endpoints, obstructions, deactivation or deletion.

All three parts follow Physgun movement and USE rotation, including roll.
A fixed 20-unit native mass avoids adding mass as the enclosing hull extends.
Selecting a visible part maps to the root. Rope, Weld/Hinge and Thruster aiming
on a part also uses the assembly root. A conservative enclosing hull protects
translation; extension uses its own swept head hull. Shaft length interpolates
with the head in cgame through the existing MD3 renderer path. No renderer/API
or saved callback IDs change; native visual alignment remains unverified.

Setup reserves two helpers and rolls back incomplete or obstructed assemblies.
Deleting parts clears reciprocal references and stops the incomplete controller;
slot reuse cannot reconnect unrelated entities. Context Undo restores settings.
Duplicate retains settings, current extension, poses and owned parts, using the
newly spawned helpers instead of allocating duplicates. Native saved fields retain
settings and extension; restore validates values and reconstructs poses, sound
IDs and transient timing. Save **V28 `0x4a41521d`**, wire 7, APIs 1022. Earlier
supported save readers remain present; full native save/load acceptance is open.

Coverage: **1,197 cards**, **97 portable presets**, **100 of the original 182**
unavailable entries have routes; **82 original plus one empty placeholder
(83 total)** remain unavailable. **180/211** entity-class cards have routes.
Six focused suites and the game DLL pass. The full release workflow contains
**67 suites**. Checks exercise extension/retraction, obstacles, pause/resume,
rotated pole/head alignment, cgame interpolation, deletion/slot reuse, saved
fields, setup rollback, context/Undo, duplication, attachment targeting and UI.

Evidence: `build/door-camera-20261010/`. No game was launched. Native visual,
sound, full-save/campaign and active-load acceptance remain open, alongside the
remaining catalog families, tool/pose work and earlier gameplay reports.
The all-objectives goal stays active.
The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.45.0 is installed and independently verified**: **67/67 automated suites**,
**37 installed files** matched to the exact ZIP, **9 changed files**
backed up, and **1002 implementation/build/test snapshot files** matched.
Saved callback source files match the preceding 0.44.0 snapshot byte for byte.

- Build: `20261010-161228-9bb333b3`
- ZIP: `build/JediAcademyRemixed-0.45.0-win64.zip`
- SHA-256: `3501894BABB625A882E2885005D988D88E33E4418192F97633275F29A1D51747`
- Receipt/backups: `build/deploy/20261010-161321-5479c381/receipt.json`
- Controller tests/assets/logs/result: `build/door-camera-20261010/`
- Symbols/source: `build/symbols/20261010-161228-9bb333b3.zip`

This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; native visual/full-save/campaign/performance checks and the
remaining overhaul checklist are open. The 0.44.0 ZIP remains available.

An exact-model-path audit of 68 installed PK3 archives found no additional
missing NPC/vehicle/entity models for the blocked catalog entries. The 21
blocked entries with present models are already-known invisible/helper models.
This does not test NPC-definition overrides or loose files. Audit evidence:
`build/door-camera-20261010/remaining-assets-audit.json`.

## 0.44.0 portable welder (2026-10-10)

`misc_model_welder` is directly spawnable from Q / Entities. Its original
Ghoul2 model, blue-weld effect and welding sound are absent from retail JA.
The portable branch uses the unedited DEMP2 housing/icon, small stock sparks
and stock electrical loop. Native map-authored welders keep their callbacks.
No new art, shaders, renderer API or callback IDs are introduced.

**C / Welder** edits reach and damage radius (1-256 units), pulse interval
(0.1-10 seconds), damage (whole 0-1000) and On/Off. Activate toggles it.
Zero damage retains decorative sparks and sound. Physgun USE rotation aims
the housing. Default native behavior remains 10 radial damage in a 45-unit
radius every 0.2 seconds, with native damage falloff and line-of-sight checks.

The spark emission point follows the verified retail muzzle offset and chosen
reach. A collision trace from the housing clips it at a nearby surface; an
embedded housing emits no pulse. Movement, toggles and property reapplication
preserve the current cooldown. Slow frames emit one pulse, without catch-up
bursts. Pulsing continues while held or frozen; disabling or deleting the
welder stops its electrical loop. Registry generations prevent deletion and
immediate reuse of a later slot from running a newly spawned welder twice.

Context Undo restores settings. Duplicate retains settings, orientation,
ownership and supported links, starting a fresh pulse cycle. Cleanup removes
the controller. Native saved fields retain settings and the pulse deadline;
restore validates controller identity/settings and reloads transient effect
and sound IDs. Save **V27 `0x4a41521c`**, workshop wire 7, APIs 1022; earlier
supported save readers remain present. Full native save/load acceptance is open.

Coverage: **1,197 cards**, **96 portable presets**, **99 of the original 182**
unavailable entries have routes; **83 original plus one empty placeholder
(84 total)** remain unavailable. **179/211** entity-class cards have routes.
Five focused suites and the game DLL pass. The full release workflow contains
**66 suites**. Coverage includes pulse cadence, rotation, collision clipping,
decorative mode, deletion/slot reuse, saved fields, spawn dispatch, context
validation/Undo, duplication and menu visibility/command validation.

Evidence: `build/welder-20261010/`. No game was launched. Native appearance,
sound, full saves, campaign behavior and load performance remain unverified.
The Jabba camera, other catalog families, tool/pose backlog and wider gameplay
acceptance remain open. The all-objectives goal stays active.
The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.44.0 is installed and independently verified**: **66/66 automated suites**,
**37 installed files** matched to the exact ZIP, **9 changed files**
backed up, and **997 implementation/build/test snapshot files** matched.
Saved callback source files match the preceding 0.43.1 snapshot byte for byte.

- Build: `20261010-155900-c9e6868e`
- ZIP: `build/JediAcademyRemixed-0.44.0-win64.zip`
- SHA-256: `1A05A9888ED7D8E142DBBD744CF09369C8CE9D0C9AE41F6A02A4941E68A19F5E`
- Receipt/backups: `build/deploy/20261010-155952-86de7093/receipt.json`
- Controller tests/assets/logs/result: `build/welder-20261010/`
- Symbols/source: `build/symbols/20261010-155900-c9e6868e.zip`

This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; native visual/full-save/campaign/performance checks and the
remaining overhaul checklist are open. The 0.43.1 ZIP remains available.

## 0.43.1 portable searchlight controller (2026-10-10)

`misc_spotlight` now spawns from Q / Entities with an owned aim marker and
detection feedback target. Its original model and `env/light_cone` effect are
absent from retail JA. The portable branch uses the unedited Imperial camera
housing and retail `gfx/effects/light_cone` material. The native map-authored
spotlight route is retained; the portable branch avoids its unchecked target
pointer and nonexistent model tag.

**C / Light** edits range (64-8192), cone half-angle (1-30 degrees), impact-light
radius (0-1024), RGB color (0-1 each), target tracking versus manual angles,
and On/Off. Activate toggles the light. Move its aim marker with the Physgun,
or use Primary Link to choose another spawned target. Secondary Link selects
the detection output. These outputs can also be selected in C / Light; Q Link
labels now distinguish Primary/Forward/Aim from Secondary/Reverse/Detection.

The visible cone stops at the native collision trace. Detection checks the
player's body center against the cone and unobstructed line of sight, fires on
entry, and has a one-second cooldown. Staying inside does not repeat activation.
Reapplying properties does not reset the detection latch/cooldown. Scans run at
20 Hz, including while the Physgun or Freeze controls motion. The visible cone
starts at the interpolated housing; its endpoint follows those collision scans.
Missing targets use the last/manual angle and retry lookup at most once per
second. Deletion clears cached targets; callback-triggered deletion of the
current or a later registry member does not invalidate traversal.

Creation reserves two helpers and rolls back incomplete/obstructed setups.
Both outputs are renamed with owned helpers and rewired to new members when
duplicated. Copy includes settings, poses and the owned target network; Undo
removes the entire copy or restores edited settings. Detection execution state
is not copied. Native save fields retain settings, detection latch and cooldown;
restore validates the callback/type/settings and rebuilds only transient caches.
Save **V26 `0x4a41521b`**, wire 7 and APIs 1022. Callback IDs are unchanged.

Coverage: **1,197 cards**, **95 portable presets**, **98 of the original 182**
unavailable entries now routed; **84 original plus one empty placeholder (85
total)** remain unavailable. **178/211** entity-class cards have routes.
Six focused suites and the game DLL pass. The full workflow has **65 suites**,
including production controller/cgame submission, saved value fields,
deletion/slot reuse, bounded cadence, setup rollback, both Link outputs,
cycle rejection, properties/Undo, duplication and UI command validation.
Evidence and asset checks are under `build/spotlight-20261010/`.
The initial 0.43.0 release passed 65 suites and was installed/hash-verified
(build `20261010-154601-576f26a6`, receipt
`build/deploy/20261010-154647-21ad3a44/receipt.json`). Its ZIP/source evidence is
preserved. **0.43.1** removes an unnecessary cone-width cap so the maximum
8192-unit, 30-degree setting keeps its advertised half-angle. A cgame
submission regression checks the resulting 4729.653-unit end radius.
Final patch release evidence: `build/spotlight-cone-20261010/`.

No game was launched. Visual alignment, light appearance, real map occlusion,
native controls, full-save round trips, campaign behavior and performance remain
open. The welder, Jabba camera, other catalog families, physics/attachment/pose
backlog and the broader acceptance checklist are still unfinished.
The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.43.1 is installed and independently verified**: **65/65 automated suites**,
**37 installed files** matched to the exact ZIP, **8 changed files**
backed up, and **993 implementation/build/test snapshot files** matched.
Saved callback source files match the preceding 0.42.0 snapshot byte for byte.

- Build: `20261010-155016-0b86a806`
- ZIP: `build/JediAcademyRemixed-0.43.1-win64.zip`
- SHA-256: `4046666D67FA6437AFB4850D440ABA0CCD610EB5B4BBFDC83CC39BD19D133D96`
- Receipt/backups: `build/deploy/20261010-155105-5c10a363/receipt.json`
- Logs/result: `build/spotlight-cone-20261010/`
- Controller tests/assets/initial release: `build/spotlight-20261010/`
- Symbols/source: `build/symbols/20261010-155016-0b86a806.zip`

This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; native visual/full-save/campaign/performance checks and the
remaining overhaul checklist are open.

## 0.42.0 portable portal surfaces and cameras (2026-10-10)

Q / Entities now spawns `misc_portal_surface` and `misc_portal_camera` as a
linked, owned pair. Move and rotate the panel and camera independently with the
Physgun or context properties. **C / Portal** edits panel width/height (16-1024),
Camera/Mirror mode and On/Off. On a camera, choose Linked target to track a
spawned entity connected by the Link tool, or Angles to use its own rotation.
An unlinked camera also uses its angles. Activate toggles either member.
These are native view windows and mirrors; they do not teleport objects.

The surface defaults to 128 by 192 world units; the camera starts 128 units
away. Spawning either menu entry creates its counterpart, with obstruction and
allocation rollback. Link can connect a surface to another portal camera.
Property Undo restores settings; duplication captures both owned members,
their independent poses, settings and internal links. Removal clears cached
links before entity slots can be reused. Saved native fields and pointer
relocation retain the pair; restore validates the mode, size and callback.

Both Classic and Rend2 now accept explicit dynamic portal geometry and camera
matrices. Portable panels do not borrow nearby authored portal entities.
Dynamic panel normals support native front-face visibility checks. Surface
visibility uses normal server portal recursion so entities at the destination
camera are included. Stock `textures/system/mirror_yavin`, black material and
Imperial camera model are reused without new artwork. Camera views retain the
stock 512-unit distance fade. The native renderer supports **one live portal
view**; other panels, recursive panels and disconnected/disabled views draw
dark. A fixed 128-descriptor frame budget bounds added renderer storage; extra
submissions draw dark within the ordinary polygon budget. Authored BSP portals
keep their existing route and have priority in Rend2.

Save **V25 `0x4a41521a`**, workshop wire 7, game/renderer APIs **1022**.
Portable Use callback 85 is appended; earlier serialized IDs are unchanged.
The matching engine, game and renderer binaries must be installed together.
Coverage is now **1,197 cards**, **94 portable presets**, **97 of the original
182** unavailable entries routed, **85 original plus one empty placeholder
(86 total)** still unavailable, and **177/211** entity-class routes.

Seven focused suites and the game/both renderer builds passed. Two new suites
exercise production portal geometry, lifetime, native saved value fields,
renderer payload ownership, selection and sort fallback. Existing fixtures
cover setup rollback, context validation/Undo, duplication and actual Link/UI
commands. The complete release has **64 suites**. Evidence and retail material
checks are in `build/portals-20261010/`. No game was launched. Actual visual
alignment/culling, remote actor visibility, full native save/load, campaign
and performance acceptance remain open; headless fixtures are not those tests.
The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.42.0 is installed and independently verified**: **64/64 automated suites**,
**37 installed files** matched to the exact ZIP,
**11 changed files** backed up, and
**988 implementation/build/test snapshot files** matched.
The append-only callback audit matches every previous enum entry to 0.41.0.

- Build: `20261010-153513-a80645aa`
- ZIP: `build/JediAcademyRemixed-0.42.0-win64.zip`
- SHA-256: `12D70B3602E263BB35E7DF6B2F2AF4777A90802FDCA4A652441FAE2E5FF71F2C`
- Receipt/backups: `build/deploy/20261010-153610-c6c444ee/receipt.json`
- Logs/result/assets: `build/portals-20261010/`
- Symbols/source: `build/symbols/20261010-153513-a80645aa.zip`

The first full build exposed a missing renderer-test shader stub. After fixing
the fixture, the successful run also covered actual production scene-buffer
lifetime, multi-scene payload retention, exhaustion and ordinary-poly reuse.
This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; the native visual/full-save/campaign/performance checks above
and the rest of the authoritative overhaul checklist remain open.

## 0.41.0 portable laser-arm assembly (2026-10-10)

`misc_laser_arm` now spawns from Q / Entities as an owned three-part controller.
The original DN models and L-arm sounds are absent from retail JA; their old
positioning and beam drawing were commented out. The portable branch instead
uses unedited retail Imperial camera-base/camera and laser-trap models, stock
laser-trap sounds and the stock redLine effect material. Authored map callbacks
retain their existing branch; saved callback numbers are unchanged.

Select any part with C, Physgun or the ordinary Toolgun selector to select its
root. Moving or rotating that root carries its two parts, including base roll.
The **Laser** context tab edits Fire/Left/Right/Up/Down action, relative yaw/pitch,
turn speed, damage per 100 ms tick, duration and beam range. Apply stops firing;
Activate performs the chosen action. Damage zero gives a harmless beam. Repeated
activation cannot increase the damage tick rate. Freeze/Physgun motion control
does not extend firing or leave its loop sound running. The assembly is a native
controller; independent force-driven articulation is not implemented.

Creation reserves two owned helpers and rolls back incomplete or obstructed
setups. Native pointer links are cleared on either deletion order before slots
can be reused. Duplication reuses the two freshly generated helpers, copies
settings and starts idle. Property Undo restores settings without replaying
damage; transform Undo keeps firing timers independent. Normal ownership cleanup
and transition removal apply. Full-save restoration uses native saved fields and
resolved pointer links. Save **V24 `0x4a415219`** rejects older readers; workshop
wire 7 and APIs 1021 are unchanged.

Coverage: **1,197 cards**, **92 portable presets**, **95 of the original 182**
unavailable entries now routed; **87 original entries plus one empty placeholder
(88 total)** remain unavailable. **175 of 211** entity-class cards have routes.
Five focused suites and the game DLL build passed; the full release has **62
suites**, including native damage/timing, geometry, lifecycle, allocation and
obstruction rollback, UI validation, Undo and assembly duplication checks.
Retail asset verification and logs are in `build/laser-arm-20261010/`.
No game was launched. In-game art/beam alignment, controls, complete native-save
round trips, campaign behavior and performance still need gameplay acceptance.
The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.41.0 is installed and independently verified**: **62/62 automated suites**,
**37 installed files** matched to the exact ZIP, **9 changed files** backed up,
and **979 implementation/build/test snapshot files** matched. Existing saved
callback source files match the preceding installed 0.40.0 snapshot byte for byte.

- Build: `20261010-151201-073ba7a3`
- ZIP: `build/JediAcademyRemixed-0.41.0-win64.zip`
- SHA-256: `CB375EF4B7FD6CD8DB92CA71CCBA8C62943D91AE116F51F4F342880BB8788CF7`
- Receipt/backups: `build/deploy/20261010-151248-81f32bf2/receipt.json`
- Logs/result/assets: `build/laser-arm-20261010/`
- Symbols/source: `build/symbols/20261010-151201-073ba7a3.zip`

This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; the native visual/full-save/campaign/performance checks above
and the rest of the authoritative overhaul checklist remain open.

## 0.40.0 portable local weather volumes (2026-10-10)

Q now directly spawns **fx_wind_zone** and **misc_weather_zone**. Both use stock
crate control markers with non-solid, rotatable **512 x 512 x 256** volumes.
Hold C and select the marker or volume; **Properties** edits dimensions (8-8192
units per axis), position and angles. Physgun USE rotation changes the volume.
**Weather** edits mode/enablement and local wind speed (0-5000). Activate toggles.

Wind zones offer Steady/Gusting/Swirling. They add forces only to particles and
weather wind queries inside their oriented bounds; overlapping winds add.
They do not create particles or push physics props by themselves. Add rain,
snow or puff effects to see their influence. The old native `misc_weather_zone`
was a load-time cache hint; its portable adapter provides useful **Shelter**
(default) and **Exposed** rules instead. These override particle exposure and
weather hazard queries only inside the volume. The latest enabled/applied
overlapping exposure rule wins. It does not create weather by itself.
Removing/disabling a volume restores the underlying authored rules.

Both renderers own this state independently of authored weather configstrings,
brushes and wind zones. Classic applies CPU particle/point checks; Rend2 applies
the same oriented geometry in the actual GPU simulation and visibility shaders,
including world-position reconstruction for moving viewer chunks. Upward/downward
winds wrap particle heights safely, including travel across multiple map heights.
Wind and exposure work on both authored and Sandbox particle systems.
`r_weatherScale=0` disables portable volumes without changing that cvar.

There is a separate **32-volume renderer budget**, counting disabled volumes
too, shared by local wind and exposure controllers. Admission fails before
native setup or partial duplication at the boundary; the menu explains why.
This bounds per-particle shader work and fits OpenGL 3.3 minimum vertex-uniform
storage. It does not reduce the existing 4096 owned-object/prop capacity.
All 32 volumes are transported; no camera-nearest selection silently omits one.
Shader upload scratch storage accommodates the full 160-vec4 zone array.

Property and volume Undo, duplicate/group Undo, full saved settings/geometry,
priority, cleanup and entity reuse are implemented. Save **V23 `0x4a415218`**
rejects older executables that cannot restore the new volume kinds. Prior
supported saves remain readable. Use callback **84**, wire **7**, and APIs
**1021** are unchanged. The shaders/materials use existing stock weather art.

Catalog: **1,197 cards**, **91 portable presets**, **94 of the original 182**
unavailable entries routed, **88** remaining plus one empty placeholder
(**89 unavailable total**); **174 of 211** entity-class cards have routes.
Other blocked catalog families, native attachment/pose work and gameplay
acceptance remain open. The full overhaul goal remains active.

Seven focused suites passed for the new geometry, native setup, priority,
transport rejection, capacity boundary, saved fields, context, duplication and
UI commands. The production uniform submission adapter is also tested at all
32 slots. The complete release workflow now contains **61 suites**.
`tests/jar/weather_shaders.py` compiled/linked the production weather GLSL in a
hidden OpenGL 3.3 context on the RTX 4080 SUPER and read back transform feedback.
It verified rotated membership, world-space local wind, all 32 slots, opposing
vertical extremes, shelter/exposure precedence, removal restoring baseline,
and the full vertex/geometry/fragment program. Evidence:
`build/weather-zones-20261010/gpu-result.json` and focused/build logs.
No game was launched. This is shader/native-function evidence, not in-game
visual, collision, campaign, full-save round-trip or performance acceptance.
The required workflow completed; exact installation evidence follows.

### Installed release evidence

**0.41.0 is installed and independently verified**: **62/62 automated suites**,
**37 installed files** matched to the exact ZIP, **9 changed files** backed up,
and **979 implementation/build/test snapshot files** matched. Existing saved
callback source files match the preceding installed 0.40.0 snapshot byte for byte.

- Build: `20261010-151201-073ba7a3`
- ZIP: `build/JediAcademyRemixed-0.41.0-win64.zip`
- SHA-256: `CB375EF4B7FD6CD8DB92CA71CCBA8C62943D91AE116F51F4F342880BB8788CF7`
- Receipt/backups: `build/deploy/20261010-151248-81f32bf2/receipt.json`
- Logs/result/assets: `build/laser-arm-20261010/`
- Symbols/source: `build/symbols/20261010-151201-073ba7a3.zip`

This installation record was added to workspace documentation after packaging;
the exact installed ZIP is unchanged. No game was launched. The completion goal
remains active; the native visual/full-save/campaign/performance checks above
and the rest of the authoritative overhaul checklist remain open.

### Installed release evidence

The required build/test/package/deploy workflow completed: **61/61 suites**,
**37 installed files** verified against the exact ZIP, and **11 replacements**
backed up. Independent verification matched every installed hash and
**975 implementation source/build/test snapshot files**. Both saved-callback
source files remain byte-identical to the installed 0.39.1 source snapshot.
The separate hidden OpenGL shader/transform-feedback run passed all five checks.

- Build: `20261010-145754-29048153`
- ZIP: `build/JediAcademyRemixed-0.40.0-win64.zip`
- SHA-256: `1016EE0B8FC417E7F54DF6E9F90B25C38C7850EF28D796CA3A3B0E6DE24AE589`
- Receipt/backups: `build/deploy/20261010-145840-523ab337/receipt.json`
- Log/result: `build/weather-zones-20261010/release-install.log` and `release-result.json`
- GPU evidence: `build/weather-zones-20261010/gpu-result.json`
- Symbols/source: `build/symbols/20261010-145754-29048153.zip`

This record was added to workspace docs after packaging; the exact installed
ZIP remains unchanged. No game was launched. Native appearance, full gameplay
save/load, campaign/performance testing, NPC spawning, walking and Noghri attack
confirmation remain open. The overall goal is active.

## 0.39.1 portable global weather (2026-10-10)

Q now directly spawns **fx_rain, fx_snow, fx_puff and fx_wind**, each with a
stock crate control marker. Hold C, select the marker and open **Weather**.
Rain offers Light/Normal/Heavy/Acid; snow Light/Normal/Heavy; puff
Fog/Sand/Smoke/Snowstorm; wind Steady/Gusting/Swirling. Particle controls are
density (0.1-4), size (0.1-8) and opacity (0-1). Wind speed is 0-5000;
marker rotation, including Physgun USE rotation, steers it in three dimensions.
These effects are global and follow the viewer; marker position is not a local
weather volume. Native outside/roof checks still apply. Acid uses the native
outdoor damage query. The menu identifies this behavior.

The latest enabled/applied controller of each type wins. Activate toggles it;
disabling/removing the winner exposes the previous enabled controller. Apply
has property Undo, including previous priority. Duplicate retains settings and
pose; an enabled copy takes priority. Held/released markers remain controllers.
Native entity fields persist settings, enablement, priority and pose; renderer
state reconciles after full load, removal, cleanup, cgame init and shutdown.

Both Classic and Rend2 keep three bounded Sandbox particle slots separate from
authored weather. Removing controllers never sends a native global clear or
rewrites map configstrings. Wind is added separately to native forces and
queries. The 0.39.1 correction preserves Classic's original global/local wind
direction query when no portable wind is present. The intermediate 0.39.0 ZIP,
source and deployment evidence are retained; no gameplay occurred with it. Each Sandbox layer is capped at 4000 particles (Rend2 uses nine viewer
chunks and two GPU buffers per layer); more controllers do not allocate more
layers. Wind changes reuse particle buffers. Both GPU buffers are initialized
so later density increases cannot read uninitialized particle tails.
`r_weatherScale=0` disables Sandbox weather; positive scale is capped at 4.
No user cvar is overwritten. Real GPU/CPU/frame-rate cost remains unmeasured.

Materials are unedited retail `gfx/world/rain.jpg`,
`gfx/effects/snowflake1.tga` and `gfx/effects/alpha_smoke2b.tga`. No new artwork.
Save **V22 `0x4a415217`** appends use callback **84**, preserving all earlier
callback IDs. Prior supported saves remain readable; wire **7** and APIs
**1021** are unchanged. Older executables reject new weather saves.

Catalog: **1,197 cards**, **89 portable presets**, **92 of the original 182**
unavailable entries routed, **90** remaining plus one empty model placeholder
(**91 unavailable total**); **172 of 211** entity-class cards have routes.
**fx_wind_zone and misc_weather_zone remain open**, along with other catalog,
attachment, pose and gameplay acceptance objectives. The overall goal is active.

Automated evidence covers actual native setup, priority/toggle/removal and
overflow compaction, scale/wind, native saved fields, transactional renderer
transport, both production renderer adapters/GPU initialization, editor
validation and Undo, duplicate/group Undo, and UI visibility/commands.
The initial full run passed 60/60 suites; subsequent focused editor/GPU runs
passed. Evidence is under `build/weather-20261010/`. Full release build,
package and installation are performed by `scripts/build-jar.ps1`.
No game was launched. Native weather appearance, indoor/outdoor behavior,
campaign/full-save integration and load performance still require gameplay.

### Installed release evidence

The required `scripts/build-jar.ps1` workflow completed: **60/60 suites**,
**37 installed files** verified against the exact ZIP, **10 replacements**
backed up. Independent verification matched all installed hashes and
**971 implementation source/build/test snapshot files**. The callback audit
confirmed all prior saved callback IDs unchanged; only weather use **84** appended.

- Build: `20261010-144151-22f732c0`
- ZIP: `build/JediAcademyRemixed-0.39.1-win64.zip`
- SHA-256: `7CD4084D0436D3862AAAC3FB6C6DC81051EBACC5A841FB6F6231CC5BDAEFB516`
- Receipt/backups: `build/deploy/20261010-144243-46de2b44/receipt.json`
- Log/result: `build/weather-20261010/release-install.log` and `release-result.json`
- Symbols/source: `build/symbols/20261010-144151-22f732c0.zip`

This installation record was added to workspace docs after packaging; the
installed ZIP remains exact and unchanged. No game was launched. Weather
visuals, full native save/load, performance, NPC spawn confirmation, walking
and Noghri attack confirmation remain open. The overall goal remains active.

## 0.38.0 portable cloud layers (2026-10-10)

Q now spawns **fx_cloudlayer** with a small stock crate control marker and a
512-unit outer radius. Hold C, select the marker, and use **Cloud** to edit
outer radius (16-32768), inner radius (at least 8 smaller), curvature (-20 to
20), Disk/Hollow shape, Standard/Alternate scrolling style and On/Off state.
**Activate** also toggles the layer. The marker stays available when the layer
is off. Properties or Physgun USE rotation now rotate the visible cloud plane;
position changes move it. The layer is visual, with no solid cloud collision.
Native release restores the controller rather than turning it into a falling
physics object. Duplicate preserves settings and pose; Undo restores properties
or removes the pasted copy. Removing an owned controller or Sandbox cleanup
removes its layer, with no global weather state to restore.

The retail haze shaders reference an absent `gfx/world/bhaze` texture and the
native cache effect is also absent. Portable layers therefore use namespaced
shaders retaining native scrolling/fading with the **unedited retail
`textures/skies/cloudlayer2.png`**. Existing authored layers keep their original
shader names. No new image, icon, model or sound was generated. The unused cache
effect request was removed; cgame directly registers the selected material.

Classic and Rend2 now share the stock strip geometry with entity rotation.
Drawing no longer mutates tube curvature, so repeated/mirrored views receive the
same geometry. The fixed mesh emits 144 vertices/72 triangles for a disk and
240 vertices/120 triangles for a hollow layer. This bounds geometry per layer;
large translucent areas and many simultaneous layers still need live profiling.
Portable layers are broadcast so their visibility is not limited to the marker's
small visibility leaf. Their editable/held bounds remain the marker's bounds.

Native entity fields save radius, inner radius, curvature, flags, pose and use
callback. The appended use callback is **83**; all earlier callback numbers are
unchanged. Full saves use **V21 `0x4a415216`** to reject older engines before
loading it. Existing supported saves remain readable; wire **7** and APIs
**1021** are unchanged. Autosaves/transitions retain the existing policy for
Sandbox-owned temporary entities. Cloud load checks validate portable settings
and callback identity, and promote older authored null-use callbacks.

Game DLL, both renderer builds and **seven focused suites** pass. Checks cover
production spawn/use/save fields, actual cgame render submission, on/off/stock
shader selection, transformed geometry, repeated-view stability, malformed
settings, context tokens/Undo, clipboard properties and menu transport. Retail
coverage checks pass. The full workflow passed **58/58 suites**; final package/install
evidence is recorded below. No game was launched or stopped. Live
cloud appearance, UI, full engine save/load and load performance remain open.

Catalog: **1,197 cards**, **85 portable presets**, **88 of the original 182**
unavailable entries routed, **94** remaining from that set plus one empty model
placeholder (**95 unavailable total**). **168 of 211** entity-class cards have
routes. Weather, portal/navigation/controller routes and the wider active
checklist remain unfinished; the all-objectives goal stays active.

### 0.38.0 installed (workspace post-package record)

- Full release workflow: **58/58 suites passed**. Retail catalog, cloud material
  dependency/pack checks, unchanged stock icons and appearance checks passed.
- **37 installed files** independently verified against the ZIP; **12 replaced
  files** backed up. **961 implementation source files** match the preserved
  symbol/source snapshot.
- Build: `20261010-142258-a2b33ed4`; ZIP: `build/JediAcademyRemixed-0.38.0-win64.zip`.
- ZIP SHA-256: `BEB189B66FEAEABF0F113002CF6F92907D786EE1BB60BD3872F1CAAC0A295964`.
- Receipt: `build/deploy/20261010-142348-0fe80e39/receipt.json`.
- Evidence: `build/cloudlayer-20261010/release-result.json`,
  `release-install.log`, `focused-tests.log` and `callback-compatibility.json`.

No game was launched or stopped. Cloud appearance, full engine save/load,
controller input and live performance remain unverified, alongside walking,
Noghri/NPC confirmation and the other active checklist items. The full goal
remains active. This record was added to workspace documents after packaging;
the exact installed ZIP is unchanged.

## 0.37.1 portable fighter flight boundary (2026-10-10)

Q directly spawns **trigger_shipboundary** with an owned destination marker.
The default volume is **1024 x 1024 x 512** units, aligned to its placed angles;
the destination starts 1024 units forward and 256 units above the origin. Place
it in a clear flight area. A healthy, inhabited fighter entering the volume
turns toward its destination at up to 90 degrees/second with forward throttle
for **3 seconds**. Repeated contacts do not extend that timer; after expiry the
fighter must leave before that boundary rearms. Overlapping boundaries retain
the current turn until it ends and the ship leaves its source volume.

The native callback was previously disabled. Its existing destination/time
fields now reach both the fighter's native orientation/movement update and the
pilot's Pmove input. It uses the destination's current position, so the marker
can move. Frame-rate checks cover 20/60/120 updates per second; flight physics
and the final camera/input behavior still require native gameplay acceptance.
The controller yields to Physgun/Force-grip state and vehicle boarding.

Hold C and select the volume. **Properties** edits position, rotation and local
dimensions; **Boundary** edits steering duration (0.1-60 seconds) and the policy
for empty/damaged fighters. Q defaults to **Ignore**. **Destroy** exposes the
original native behavior for unpiloted fighters or those with missing surfaces.
Ordinary actors and ground vehicles do not receive boundary steering/damage.
**Activate** toggles the volume. Move the owned marker with the Physgun or use
the Link tool to select another spawned destination. Missing destinations fail
without a fatal map error. No new icons/models/sounds were created.

Undo restores duration/policy and cancels current turns; it does not rewind
flight. Duplicate retains settings, active state, rotated dimensions and the
owned destination network, with independent new target names and no inherited
active steering. Failed assembly paste retains its existing rollback behavior.
Disabling/removing the volume, deleting/reusing its destination, pilot departure
or death, Sandbox-owned cleanup and map transitions release affected control.
Newer external movement state is not overwritten when the boundary releases it.

Full saves retain steering ownership in a bounded **JSHP** chunk, alongside
existing native destination/time fields. Expired inside-volume records retain
their rearm state. The save preparation reconciles ownership before native
entity serialization. Autosaves omit temporary steering and clear stale pilot
references on load. Load validates references, callbacks, bounds and timing;
older inert native boundary callbacks are promoted. Save header **V20
`0x4a415215`** rejects older engines before loading the new callbacks/chunk;
earlier supported saves remain readable. Workshop wire **7** and game/renderer
APIs **1021** are unchanged.

0.37.1 preserves the existing saved use-callback IDs (space 79, directional
80, marker 81) and appends flight boundary as 82. A new compile-time regression
check reproduced the misplaced callback in intermediate 0.37.0; that ZIP,
symbol archive and receipt are retained as superseded diagnostic evidence.
All prior callback numbers are retained in the final installed release.

The game DLL and **nine focused suites** pass, covering production callbacks,
geometry, native command/orientation outputs, duration/rearm, lifecycle and
external override cleanup, serialization, context transport/Undo, menu
visibility, setup ownership and duplication. Retail coverage/asset checks pass.
The full release workflow passed **57/57 suites**; final package/install evidence
is recorded below. No game was launched or stopped. Live
flight, controller UI, full engine save/load and campaign acceptance remain open.

Catalog: **1,197 cards**, **84 portable definitions**, **87 of the original 182**
unavailable entries routed, **95** remaining from that set plus one empty model
placeholder (**96 unavailable cards**). **167 of 211** entity-class cards have
routes. Remaining catalog/attachment/pose implementation and the broader native
acceptance checklist keep the all-objectives goal active.

### 0.37.1 installed (workspace post-package record)

- Full release workflow: **57/57 suites passed**, including the saved callback
  ID regression. Retail catalog, stock icons and appearance asset checks passed.
- **37 installed files** independently hash-verified against the exact ZIP;
  **10 replaced files** backed up. **955 implementation source files** match
  the preserved symbol/source snapshot.
- Build: `20261010-141117-afed5ef6`; ZIP: `build/JediAcademyRemixed-0.37.1-win64.zip`.
- ZIP SHA-256: `C8D35E03DF4478A1B65B27EEFD8AFFAEE4D02B3918EC475EF50B93BC30DBD0AA`.
- Receipt: `build/deploy/20261010-141214-20490bff/receipt.json`.
- Evidence: `build/shipboundary-20261010/release-result.json`,
  `release-install.log`, `callback-order-repro.log` and
  `callback-compatibility.json` in that directory.

The intermediate 0.37.0 ZIP, build `20261010-140756-39cef51a`, receipt
`build/deploy/20261010-140848-22ad74a3/receipt.json` and
`release-install-0.37.0.log` remain preserved. They are superseded by 0.37.1,
which keeps all prior callback numbers and appends the new ones.
No game was launched or stopped. Native flight/controller UI/save-load,
walking/Noghri/NPC confirmation and broader acceptance remain open. The
all-objectives goal remains active. This installation record was added to the
workspace after packaging; the exact installed ZIP was not modified.

## 0.36.1 portable native script runner (2026-10-10)

Q directly spawns **target_scriptrunner** as a stock crate control marker.
Its default is the retail `common/dooropenedelsewhere` ICARUS script, which
plays a stock player voice line on the activator. The default allows unlimited
uses, no delay and a one-second cooldown. No compiled script/audio bytes are
redistributed; the generated catalog contains **1,011 retail script paths**.

Hold C, select the controller and open **Script**. Prev/Next chooses a retail
script; the path field also accepts installed custom scripts. Choose **Self**
or **Activator**, delay/cooldown (0-3600 seconds) and remaining uses (-1 unlimited,
0 exhausted, 1-100000 limited). Apply validates the path and installed IBI 1.57
header, stores properties and cancels any pending activation. Activate runs it;
using a linked trigger preserves that trigger's activator. Campaign scripts
retain their native map/entity requirements. The default is independent of
mission targets; selecting a campaign script does not make its dependencies
portable. The former Script parms tab is now labeled **Parameters**.

Undo restores runner properties without replaying a pending use or reversing
script actions. Duplicate retains the script, receiver choice, timing, remaining
uses and all 16 parameters, starting without queued activation or copied ICARUS
execution state. Failed paste rolls back its new entities. Deleting the runner
uses normal native ICARUS cleanup; a script already running on an activator
belongs to that activator and is not undone by deleting the controller.

Native fixes: a delayed callback clears itself before scheduling cooldown,
preventing repeated automatic execution. Deleting an activator cancels pending
uses before its slot can be reused. ICARUS initialization is lazy for portable
entities and uses a persistent, unique script name. Remaining-use/timer state
is committed before script execution, so destructive scripts cannot write back
into freed/reused controller slots. Ordinary authored runner dispatch remains
native, including its inactive flag and self/activator behavior.

The existing native entity fields serialize settings, activator references and
pending timers/callbacks. Save header **V19 `0x4a415214`** prevents older releases
from accepting the new catalog route; earlier supported saves remain readable.
Workshop wire **7** and game/renderer APIs **1021** are unchanged.

Seven focused suites passed for callbacks, paths/header validation, script
initialization, delay/cooldown/use counts, deleted/reused activators, destructive
execution, context validation/Undo, UI transport/visibility, duplication/rollback,
portable spawn defaults and native serialization. Retail checks confirmed all
script paths and the default voice-line assets. The full release workflow has
**56 suites**; its final build/install result is recorded separately below after
completion. A maximum-delay round-trip check reproduced a float rounding error
at 3,600 seconds in the intermediate 0.36.0 build; 0.36.1 corrects the unit
conversion. The original 0.36.0 ZIP, symbols and receipt are preserved.
No game was launched or stopped. Live UI, ICARUS execution and full
engine save/campaign acceptance remain open.

Catalog: **1,197 cards**, **83 portable definitions**, **86 of the original 182**
unavailable entries routed, **96** remaining from that set plus one empty model
placeholder (**97 unavailable cards**). **166 of 211** entity-class cards have
routes. Remaining controllers/assets, native attachment/pose work and the full
gameplay acceptance checklist keep the all-objectives goal active.

### 0.36.1 installed (workspace post-package record)

The standard `scripts/build-jar.ps1` workflow passed **56/56 suites**, retail
catalog/assets checks and installed the exact release ZIP. Deployment verified
**37 files**, backing up **8 replacements**. Independent installed readback
matched every receipt hash and **951 source/build/test/runtime snapshot files**.
The maximum-delay regression now passes in the full release suite.

- Build: `20261010-135209-9ad85425`
- ZIP: `build/JediAcademyRemixed-0.36.1-win64.zip`
- SHA-256: `ECEC96C6457D8A99489F0DCE2FB822AA4BB2DB939AFA9D424D992A262476B295`
- Receipt/backups: `build/deploy/20261010-135253-47ff7d64/receipt.json`
- Workflow: `build/scriptrunner-20261010/release-install.log`
- Independent verification: `build/scriptrunner-20261010/release-result.json`
- Symbols/source: `build/symbols/20261010-135209-9ad85425.zip`

The intermediate 0.36.0 ZIP and build `20261010-135007-f2b024a0` are retained
with receipt `build/deploy/20261010-135053-76b6950a/receipt.json` and workflow
`build/scriptrunner-20261010/release-install-0.36.0.log`. They are superseded by
0.36.1. The reproduction is `build/scriptrunner-20261010/limit-repro-test.log`.

No game was launched or stopped. Native script execution, UI, full engine
save/mission behavior and broad overhaul acceptance remain open. The 0.35 NPC
admission/class fixes remain included; live NPC confirmation and walking/Noghri
confirmation are still pending. The all-objectives goal stays active. This
deployment record was added after packaging and exists in the workspace,
leaving the ZIP and installed hashes unchanged.

## 0.35.0 NPC admission fixes and script parameters (2026-10-10)

The NPC menu accepts valid loaded NPC definitions, including installed mod
overrides, using their actual model, width and height. It no longer rejects an
ordinary NPC solely because its definition differs from the retail fingerprint.
Required models, explicit skins, weapon resources, AI classes and bounds are
still checked; failures identify the NPC and specific reason. Vehicle and player
templates retain their separate construction checks. Other installed mods are
preserved.

The reported Desann and RebornBoss definitions are overridden by the installed
`zzzzzzz_SWGL_JKJO.pk3`; their fingerprints differ from the stock catalog. Mon
Mothma has a separate retail mismatch: `CLASS_MONMOTHMA` in her definition versus
the engine's `CLASS_MONMOTHA`. The native class table now accepts both spellings.
The audit's default total height also matches the native 64-unit standing hull.
The production admission/resource checks pass against the installed definition
stream and file inventory for **desann, monmothma and rebornboss**. These checks
stub renderer initialization; they do not prove live AI/rendering or placement.

Q directly spawns **target_change_parm** as a stock crate control marker, with
parameter 1 initially `Sandbox`. Hold C, select any entity (or Select player),
and open **Script parms**. Prev/Next selects one of 16 slots; Apply stores up to
63 printable characters and Clear empties the selected slot. Undo restores the
previous text. Refresh rereads script changes; switching the selected entity
retains the chosen slot while the menu stays open. Properties returns to the
position, rotation and entity-specific controls. Help retains the top-left style.

Activate a parameter controller directly or through a linked trigger to copy
its nonempty slots to the activator. `+5` and `-2` apply native numeric increments
on activation. Editing and duplication preserve these strings literally; blank
controller slots are skipped. Parameters can be inspected/edited on players,
NPCs, props and other selected entities. The native callback now remains present
on empty controllers, allowing later edits. The setter checks entity/slot bounds
and null inputs and rejects non-finite counter results. Existing finite native
counter behavior and native string truncation remain supported.

Duplicate retains all 16 supported literal values, including empty slots that
replace a fresh controller's defaults. Invalid/unrepresentable authored values
cannot be silently truncated or overwritten by the editor/clipboard. Parameter
values use the existing native entity PARM serialization; the new save header is
**V18 `0x4a415213`**, with workshop wire **7** and game/renderer APIs **1021**.
The runtime reads earlier supported saves. Full engine save/mission acceptance
remains open.

Nine focused suites cover properties, transport/selection tokens, Undo,
duplication, all 16 slots, native activation/counters/parameter serialization,
spawn setup, the class alias and loaded NPC admission. The game DLL compiled.
The normal release workflow now contains 55 suites; its final build/install
result is recorded separately after completion. No game was launched or stopped.

Catalog: **1,197 cards**, **82 portable definitions**, **85 of the original 182**
unavailable entries routed, **97** remaining from that set plus the empty model
placeholder (**98 unavailable cards**). **165 of 211** entity-class cards have
routes. Broad live NPC, controller/UI/save/campaign, walking/Noghri and physics
load acceptance, plus remaining catalog and tool work, keep the goal active.

### 0.35.0 installed (workspace post-package record)

The normal `scripts/build-jar.ps1` workflow passed **55/55 suites**, retail
catalog/assets checks and installed the exact release ZIP. Deployment verified
**37 files**, backing up **9 replacements**. Independent installed readback
matched every receipt hash and **943 source/build/test/runtime snapshot files**.

- Build: `20261010-133451-b9755081`
- ZIP: `build/JediAcademyRemixed-0.35.0-win64.zip`
- SHA-256: `2352CF1E269E30204210EAF6B57C5861F92ADA4AD4B3A489B364F889AEC38517`
- Receipt/backups: `build/deploy/20261010-133542-4482eb49/receipt.json`
- Workflow: `build/script-parms-20261010/release-install.log`
- Independent verification: `build/script-parms-20261010/release-result.json`
- Installed NPC definition/resource audit: `build/script-parms-20261010/installed-npc-validation.log`
- Symbols/source: `build/symbols/20261010-133451-b9755081.zip`

No game was launched or stopped. The NPC audit uses real installed definitions
and file names with a stub renderer boundary; live spawning/AI, parameter UI and
full engine save/mission acceptance remain open. Walking/Noghri fixes still lack
owner confirmation. The all-objectives goal remains active. This deployment
record was added after packaging and exists in the workspace, leaving the ZIP
and all installed hashes unchanged.

## 0.34.0 portable music controller (2026-10-10)

Q now directly spawns **target_play_music** as a stock crate control marker.
It starts inactive, with the retail Hoth exploration track selected. Activate
with the Toolgun, C menu or a linked native trigger to toggle playback. Hold C,
select it and use **Music / Prev / Apply / Next** to choose from **133 retail
music files**. Prev/Next applies one file immediately; the text field also
accepts an intro path followed by a loop path, separated by one space. Paths
are validated against the retail list and limited to 127 characters total.
No music assets or custom catalog icons are generated or redistributed.

Track edits support Undo. Editing the currently playing controller updates its
music; editing a controller underneath another updates what will resume later.
The most recently activated controller plays. Deactivating or deleting it resumes
the previous active controller, or the exact pre-controller music cue. Removing
an older controller leaves the current one playing. Sandbox off restores that
baseline; map transitions release playback ownership. Music restarts from the
beginning when restored; decoder seek position is not saved by this controller.

Native mission/console music takes ownership and discards the portable stack,
even if it selects the same track. A different externally written music
configstring is also detected. Later removal cannot rewind that new mission cue.
Authored target_play_music entities retain their original one-shot behavior;
their retail track properties are editable, while authored dynamic/custom cues
outside the retail editor list keep native playback. Direct client-side credits
music bypasses CS_MUSIC and still needs campaign integration acceptance.

Duplicate copies track properties and target wiring into a fresh inactive
controller. It does not copy the original's playback ownership. Full saves retain
activation order, entity identities and the baseline cue in a bounded **JMUS**
chunk, alongside native saved message/model fields. Loading a player-only autosave
restores the baseline because its Sandbox controllers are absent; writing that
autosave does not interrupt current playback. Reused/missing slots and malformed
records are rejected. Old ownership resets before save-load entity teardown.

New saves use **V17 `0x4a415212`**; workshop wire **7** and game/renderer APIs
**1021** are unchanged. Earlier saves remain readable; older builds reject V17.
Seven focused suites and the native game DLL build passed. Fixtures exercise real
spawn/use callbacks, stacked toggle/deletion, mission replacement (including
identical tracks), property changes, reuse, full/autosave serialization, malformed
records, C dispatch/visibility, duplication and Undo. The independent retail check
matches all 133 paths to installed assets. The full workflow now contains **53
suites**. These tests replace engine/audio services and do not prove audible
playback, native UI appearance or complete campaign/save-load behavior. No game
launched; those acceptance checks remain open.

Catalog totals: **1,197 cards**, **81 portable definitions**, **84 of the original
182 unavailable entries routed**, **98 still unavailable from that set**, plus
the empty model placeholder (**99 unavailable cards**). Of 211 entity-class cards,
164 have spawn routes. Remaining controllers/properties, scripted-mover forces,
rope wrapping, skeletal poses and broad campaign/save/physics acceptance remain
open. The owner has not tested the installed walking/Noghri fixes yet. The full
overhaul goal remains active.

### 0.34.0 installed successfully (workspace post-package record)

The owner confirmed the game was closed. Deployment of the exact tested ZIP
completed, with **37 installed files hash-verified and 10 replacements backed up**.
An independent readback also matched **936 implementation source files** against
the retained build snapshot. The full workflow passed **53/53 suites**.

- Build: `20261010-131512-e3d136cf`
- ZIP SHA-256: `98EF73C1B54D627DD07E30A95D0C5C52DD16AA958E09D8898157CC14BA40CA6E`
- Receipt: `build/deploy/20261010-132129-6e730a57/receipt.json`
- Verification: `build/music-20261010/release-result.json`

This supersedes the first deployment attempt below. No game was launched or
stopped. Gameplay acceptance and the all-objectives completion goal remain open.

### 0.34.0 first deployment attempt (historical; superseded above)

The normal `scripts/build-jar.ps1` workflow passed **53/53 suites** and retail
asset checks, then packaged build **20261010-131512-e3d136cf**. Deployment stopped at
its running-game check: **jar_sp.x86_64 PID 6340** was running from the Steam
GameData installation. No destination files were changed and no process was
stopped. At that time **0.33.0 remained installed** (`20261010-130203-6cd52d6a`). The
owner subsequently confirmed the game was closed; the retry above succeeded.

Independent readback verified all **36 payload hashes** in the ZIP manifest
(37 packaged files including the manifest), its identity and **936 matching
source/build/test/runtime files**. This does not confirm local installation.

- ZIP: `build/JediAcademyRemixed-0.34.0-win64.zip`
- SHA-256: `98EF73C1B54D627DD07E30A95D0C5C52DD16AA958E09D8898157CC14BA40CA6E`
- Workflow: `build/music-20261010/release-install.log`
- Independent package verification: `build/music-20261010/package-result.json`
- Symbols/source: `build/symbols/20261010-131512-e3d136cf.zip`

After the game closes, finish the already-built release with
`scripts/deploy-jar-release.ps1 -ZipPath build/JediAcademyRemixed-0.34.0-win64.zip`.
Preserve this ZIP and use its exact hash; do not rebuild solely to retry deployment.
Then record the new receipt, independently verify installed files and update the
installed-version status. The process check remains authoritative; never stop a
running game. This note was added after packaging and exists only in the workspace.
The all-objectives goal remains active, with native acceptance still open.


## 0.33.0 native and mixed Weld/Hinge (2026-10-10)

Weld and Hinge now connect native NPCs, the player, pickups, ordinary native
models, owned props and world surfaces. Box3D can be off. In Q choose the tool,
then click two different entities or an entity and scenery. The first clicked
face supplies the hinge pivot and axis; the second click retains the entities'
current relative poses. Right click cancels the first anchor. Frozen entities,
world surfaces, occupied controllers and scripted/path movers act as anchors.
Path movers retain their native motion; this does not add force-driven movers.

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


## 0.32.0 persistent interest and location markers (2026-10-10)

Q now directly spawns **target_interest** and **target_location** with stock
marker models. An interest point attracts the local player's idle head-look
within 256 units when visible and no enemy has priority. Its owned feedback
target fires when the player starts looking at it. Move it with the Physgun or
C position controls; Activate toggles it. Authored interest points retain their
native fixed table. Portable points persist separately, so spawning, moving,
deleting or disabling them cannot leave a discarded point in that table.
This does not add a new autonomous NPC attention behavior.

Location markers register immediately and label nearby entities. Hold C and
select a location or feedback marker to edit its **Label** (1-63 printable ASCII
characters; no quotes, semicolons or backslashes). Apply label supports Undo;
hover the selected entity's status to see its location in the existing top-left
help area. Native trigger_location volumes retain lookup priority; outside those
volumes the nearest visible active target_location is preferred, falling back
to the nearest active marker. Activate toggles location markers. This fallback
also serves native scripted location queries and needs campaign acceptance.

Duplicate retains supported labels, marker active state and fresh owned feedback
links. Saved native fields retain labels/origins/flags; interest registrations
and location lists rebuild after loading. Deletion clears look references and
unlinks locations before entity slots can be reused. Target callbacks that delete
or recreate a marker cannot leave a stale selection. New saves use **V15
`0x4a415210`** for the appended marker callback; workshop wire 6 and APIs 1021
remain unchanged. Older saves remain readable; older builds reject V15.

Seven focused suites passed: actual native interest/location selection, authored
priority, visibility/range, movement/toggling/deletion/reuse, registration restore,
native callback serialization, real spawn wrappers, label validation/Undo, UI
transport and property/assembly duplication. The native game DLL also compiled.
The normal release workflow now includes **51 suites** plus retail asset checks.
Native visuals/input, campaign behavior and complete engine save-load acceptance
remain open; these headless fixtures replace engine services. No game launched.

Catalog totals: **1,197 cards**, **80 portable definitions**, **83 of the original
182 unavailable entries routed**, **99 still unavailable from that set**, plus
the empty model placeholder: **100 unavailable cards**. Of 211 registered entity
class cards, 163 have spawn routes. Remaining controllers, native Weld/Hinge,
scripted-mover forces, skeletal poses, walking/Noghri confirmation and broader
gameplay/performance acceptance remain open. The all-objectives goal is active.

### 0.32.0 installation evidence (workspace post-package record)

The normal `scripts/build-jar.ps1` workflow completed with **51/51 suites**,
retail/catalog/appearance checks, packaging and Steam GameData installation.
Build **20261010-123033-2731c18a**; **37 files verified**, **11 replaced with backups**.
Independent readback matched the ZIP, installed build identity and hashes, plus
**928 source/build/test/runtime snapshot files**.

- ZIP: `build/JediAcademyRemixed-0.32.0-win64.zip`
- SHA-256: `CC3A333212BE7DCC563DE47EE4288D6BA118A8006D217D2F4AD1FBED783A64D6`
- Receipt/backups: `build/deploy/20261010-123116-2f3adb41/receipt.json`
- Workflow: `build/markers-20261010/release-install.log`
- Independent verification: `build/markers-20261010/release-result.json`
- Symbols/source: `build/symbols/20261010-123033-2731c18a.zip`

This record was appended after packaging and is workspace-only; installed docs
contain the implementation notes above. **No game launched.** The pending owner
question about walking and Noghri attacks has no reply, so both acceptance items
remain open. The goal is active. Remaining catalog controllers/properties, native
Weld/Hinge and scripted-mover forces, skeletal poses and actual visual/input,
campaign/save/physics acceptance are still required.

## 0.31.0 directional trigger and volume selection (2026-10-10)

**trigger_bidirectional** is directly spawnable from Q. The formerly unfinished
native function now creates an oriented volume with two owned feedback targets.
Cross its center plane along the displayed arrow to fire **Forward**; cross
against it for **Reverse**. Standing still or moving entirely on one side does
not repeat activation. The default cooldown is one second. Activate toggles it.

In Q, choose **Link** and use the property **-/+** buttons to choose Forward or
Reverse, then select the source and destination with Toolgun left clicks. Right
click cancels the source. Reverse applies only to directional triggers. Both
branches participate in bounded loop checks and assembly duplication. Fresh
copies receive fresh target names, dimension/orientation and filter/count/wait
properties; one Undo removes the complete copied setup. Spawn failures remove
all provisional helpers. Deleting a target leaves an inert named output rather
than a pointer into a reused entity slot.

Hold C to edit its Width/Depth/Height or angles; Physgun left click + bound USE
also rotates it. Nearby outlines show the true oriented shape and forward arrow.
The shared geometry adapter now serves space and directional volumes. Selection
from inside a volume uses its exit face, allowing closer entities to be picked.
Overlapping volumes are ordered by their actual boundary distance. Disabled
volumes remain selectable. Empty corners of a rotated box's broad hull do not
intercept selection.

The native callback uses the actor's saved previous/current origin segment to
test the center-plane intersection within the volume. It supports players/NPCs,
PLAYER_ONLY and NPC_targetname filters, inactive state, cooldown, negative wait
for one firing and positive count limits. Activate rearms one exhausted count
firing. Dead/noclip actors and segments longer than the native 1024-unit swept
trigger limit are excluded. Cooldown/count are committed before native targets
run, because those callbacks may delete the trigger. Authored BSP bounds and
native movedir remain supported; portable volume geometry follows its rotation.

Full saves use **V14 `0x4a41520f`** for the appended callbacks/new catalog route.
Existing native serializers retain origin segments, callbacks, links, local
bounds, filters and cooldown/count fields; no per-actor tracking pool was added.
Workshop wire 6 and both APIs 1021 are unchanged. Older saves remain readable;
earlier builds reject the new header. Copy restarts the cooldown, as other copied
native controllers restart execution state.

Six focused suites passed, including actual crossing callbacks/touch dispatch,
native field serialization, filters/exhaustion/deletion, both Link outputs,
cycle/reuse guards, selection geometry, spawn rollback, duplication/Undo and UI
output selection. The release workflow now runs **50 suites**, followed by asset
checks and verified installation. These headless tests replace engine services;
native visual/input/campaign acceptance remains open. No game launched.

Catalog totals: **1,197 cards**, **78 portable definitions**, **81 of the original
182 blocked entries routed**, **101 still blocked from that set**, plus the empty
model placeholder: **102 unavailable cards**. Of 211 registered class cards,
161 have a route. Other catalog/controllers, native Weld/Hinge, scripted-mover
forces, skeletal poses, walking/crash confirmation and the broader acceptance
checklist remain open. The all-objectives goal remains active.

### 0.31.0 installation evidence (workspace post-package record)

The normal `scripts/build-jar.ps1` workflow completed with **50/50 suites**,
retail/catalog/appearance checks, packaging and Steam GameData installation.
Build **20261010-121359-dd46f381**; **37 files verified**, **11 replaced with backups**.
Independent readback matched the ZIP, installed build identity and hashes, plus
**923 source/build/test/runtime snapshot files**.

- ZIP: `build/JediAcademyRemixed-0.31.0-win64.zip`
- SHA-256: `886816A934E83108B2CDD3C80AB60C1CF6B5F2103358D11C052407371B8174AB`
- Receipt/backups: `build/deploy/20261010-121448-eb722d94/receipt.json`
- Workflow: `build/bidirectional-20261010/release-install.log`
- Independent verification: `build/bidirectional-20261010/release-result.json`
- Symbols/source: `build/symbols/20261010-121359-dd46f381.zip`

This record was appended after packaging and is workspace-only; installed docs
contain the implementation notes above. **No game launched.** Walking/Noghri
owner confirmation was requested separately and remains open without a reply.
The goal remains active; remaining controllers, native attachments and actual
visual/input/campaign/physics acceptance still need completion.

## 0.30.0 editable and oriented space volumes (2026-10-10)

Hold C, select a portable **trigger_space**, and set **Width, Depth and Height**
from 8 to 8192 units each. Apply dimensions retains the local lower center; Undo
restores the previous size. Apply angles or hold Physgun left click + the bound
USE action and move the mouse to rotate the volume. Ordinary entities retain
their existing position/angle/health controls. Selection tokens and finite/range
checks reject stale or malformed property requests.

The space effect and selection ray now use the rotated local box. The engine's
larger axis-aligned hull is only a broad collision-query bound. Gold selection
and blue nearby outlines follow the actual rotated shape; disabled volumes use
a muted outline and remain selectable. Non-solid space volumes can enclose map
geometry or actors during spawning, exact positioning, Physgun movement and
paste. Ordinary solid entities keep their placement collision checks.

Duplicate preserves local dimensions, orientation and enabled/inactive state.
Full saves use **V13 `0x4a41520e`** and retain those existing native fields;
workshop wire 6 and both module APIs 1021 remain unchanged. V12 world-aligned
bounds migrate once into local dimensions. Earlier inert volumes receive the
usable default. Authored BSP brush volumes retain their native axis-aligned
behavior and do not expose the new dimension controls. Older saves remain
readable; earlier releases reject the new header before loading state.

Eight focused headless suites passed, including geometry/ray/corner membership,
native serialization and legacy migration, context input/undo, native movement,
copy/paste and UI field visibility/command validation. The shared test formatting
helper now matches the game's four rotating buffers, so nested formatted cvar
names/values are tested correctly. Full release validation and installation are
recorded separately after `scripts/build-jar.ps1` completes.

Catalog totals remain **1,197 cards**, **77 portable setup definitions**, **80 of
the former 182 blocked entries routed**, and **103 unavailable cards**. More
controllers, properties for other entity types, native Weld/Hinge, scripted-mover
forces, skeletal poses and broad gameplay acceptance remain open. Walking and
Noghri attack fixes still require native confirmation. No game launched; the
all-objectives goal remains active.

### 0.30.0 installation evidence (workspace post-package record)

`scripts/build-jar.ps1` completed: **48/48 suites**, retail/catalog/appearance
checks, exact ZIP packaging and local Steam GameData installation. Build
**20261010-120222-61cfcd5c**; **37 files verified**, **8 replaced with backups**.
Independent verification matched the ZIP, installed build identity, all installed
file hashes and **918 source/build/test/runtime snapshot files**.

- ZIP: `build/JediAcademyRemixed-0.30.0-win64.zip`
- SHA-256: `6D80509742992186DBBDCBB2EA2570ECF3FB0317C3D63143C56A8B1248743991`
- Receipt/backups: `build/deploy/20261010-120312-e0916240/receipt.json`
- Build/tests/install log: `build/volume-properties-20261010/release-install.log`
- Independent result: `build/volume-properties-20261010/release-result.json`
- Symbols/source: `build/symbols/20261010-120222-61cfcd5c.zip`

The initial build caught an effects-API color-pointer type mismatch; it was fixed
before the successful full workflow. Its log is retained as
`build/volume-properties-20261010/release-build-attempt1.log`.
This evidence was appended after packaging and is workspace-only; installed
documentation contains the implementation notes above. **No game launched.**
The completion goal remains active. In addition to the authoritative backlog,
selection priority from inside/overlapping large volumes needs follow-up: the
current ray treats a containing volume as a zero-distance hit.

## 0.29.0 functional space volumes (2026-10-10)

**trigger_space** now has a working native touch controller. Its Q-menu marker
creates a 128 x 128 x 128 world-aligned volume, tall enough to contain a standing
player's origin. Inside it, normal actor gravity becomes zero. Breathing actors
get five seconds of air, then native 20-40 damage at 1-2 second intervals, with
humanoid choking feedback. Droids and vehicles retain space membership without
suffocation; enclosed living vehicles protect their riders at every valid entity
index, including high NPC slots. Existing custom-gravity controllers retain their
gravity. Activate toggles the volume while keeping its stock marker selectable.

Leaving, disabling, moving away or removing the volume clears its exposure state;
normal gravity returns on the next movement update. Invalid, freed or reused
non-space entity references are rejected before indexing the entity array.
Overlapping touches retain the exposure timer instead of endlessly renewing air.
Noclip/dead actors do not receive suffocation feedback. Removing a volume clears
its occupants before slot reuse. Fighter space checks use the same validation.

Duplicate preserves a space volume's bounds and enabled/inactive state. Full
saves retain its callbacks, bounds, enabled state and actor exposure fields.
Older saved inert space volumes acquire the missing callbacks; old portable
crate-sized bounds become the usable default. Existing authored BSP bounds stay
unchanged. New saves containing any space volume use **V12 `0x4a41520d`**, even
without an owned Sandbox registry, so earlier builds reject the new callbacks.
Workshop wire 6 and module APIs 1021 remain unchanged. Older saves remain readable.

The controller, actual touch dispatcher, native field serializers, portable spawn
transaction, copy/Undo and version routing have headless regression coverage.
Damage application, native collision services, animation and audio are fixture
boundaries; this is not a full game save/load or visual acceptance run. The save
fixture now handles native padding in both write/read modes; production save
layout is unchanged. An independent audit of **61 installed retail BSP entity
lumps** found no authored `trigger_space`, `trigger_shipboundary` or
`trigger_bidirectional` entities (`build/space-20261010/retail-volume-audit.json`).
That is content evidence, not campaign gameplay acceptance.

The catalog remains **1,197 cards**, **77 portable setup definitions**, **80 of the
former 182 blocked entries routed**, and **103 unavailable cards**. This release
fixes an existing listed controller rather than increasing those counts. Space
volumes use native axis-aligned bounds: independently configurable extent and
rotated-volume property editing remain open, as do other catalog/controllers,
native Weld/Hinge, scripted-mover forces and the broader acceptance checklist.
The release workflow runs 48 suites and asset checks before verified installation.
**No game launched; the all-objectives goal remains active.**

Workflow log: `build/space-20261010/release-install.log`; before-source copies:
`build/space-20261010/before`. Exact build, receipt and hashes follow deployment.

## 0.28.0 portable magnetic lock and explosive barrel (2026-10-10)

The Q catalog now spawns **misc_maglock** with an owned, crate-sized sliding door.
The stock Kejim security panel represents the lock because the original door-lock
model is absent from retail Jedi Academy. The native shielded lock takes saber
damage; destruction unlocks its door, plays a stock thermal effect and leaves the
door available for Use. Manual removal/Undo removes the owned setup. Removing a
door clears lock references before its entity slot can be reused. Multiple native
locks retain their count until the last lock is removed. A map lock embedded in
solid now removes itself with a diagnostic instead of dropping the game.

**object_cargo_barrel1** uses the stock Imperial barrel pair, measured collision
bounds and a stock thermal effect instead of the absent legacy cargo models.
Damage uses its native delayed explosion (100-500 ms); Activate detonates it.
It retains native 100 splash damage / 200 radius defaults. These are native
controllers, with ordinary native manipulation; they are not new Box3D props.

Spawn transactions own every helper and roll back capacity/placement failures.
Duplicate copies the lock/door graph with a fresh door reference and current lock
health. A destroyed lock's surviving door remains independently owned and saved;
Undo of a pasted group still removes all surviving members of that pasted group.
The new native callback fixture covers attachment counts, removal, slot reuse,
authored trace failures, one target dispatch, delayed barrel death and explosion
callbacks. Collision, damage application, scripts and rendered FX are mocked
service boundaries; full native gameplay/save/load acceptance remains open.

The catalog has **1,197 cards**, **77 portable setup definitions** and three added
NPC aliases: **80 of the former 182 blocked entries have spawn routes**, with 102
still disabled plus the empty no-weapon model found in 0.27 (**103 unavailable
cards total**). A further functional gap was found in the listed `trigger_space`:
its native touch callback is disabled. That route is explicitly unfinished and
must not be counted as gameplay-complete merely because it spawns a volume.

New full saves use **V11 `0x4a41520c`** so older releases cannot load this new
ownership behavior. Older saves remain readable. Workshop wire 6 and both module
APIs 1021 are unchanged. The normal release workflow runs 47 suites plus retail
and packaged asset checks, then installs the exact ZIP with backups and hashes.
No game launched; the all-objectives goal remains active.

Workflow evidence: `build/native-objects-20261010/release-install.log`;
before-source copies: `build/native-objects-20261010/before`. Exact receipt,
build identity and installation verification will be recorded after deployment.

## 0.27.0 decorative model bounds qualification (2026-10-10)

Added production sampler and model-bound fixtures for native bone-cache vertices,
weighted influences, hidden/bolt surfaces, visibility overrides, hierarchy cycles,
invalid references, nonfinite/oversized geometry and transactional bounds outputs.
Placement tests cover off-center models, native rotation, per-map caching and
cleanup and failed-query caching, larger-model native fallback, and agreement
with real Box3D hulls/mass. Actor proxy searches now include each full rotated
body hull plus the existing margin, rather than only a radius around its origin.
Context, duplication, attachments and new/old save routing have focused regression
coverage. Both renderer DLLs export the new callback and reject API 1020.

The independent retail check covers 126 visible Ghoul2 candidates, native surface
and vertex budgets, bone references and required stock/built-in skeletons. It
identified the empty no-weapon placeholder, now marked unavailable: 105 total
unavailable cards, including the earlier 104; total cards and routes unchanged.

Normal workflow is `scripts/build-jar.ps1`, with 46 headless suites plus independent
retail and packaged asset checks. V10 `0x4a41520b`, wire 6, APIs 1021. Full workflow
and installation evidence will be appended below after completion. No game launch,
rendered-model survey, native save/load, walking/crash confirmation or gameplay
stress acceptance is claimed. The completion goal remains active.

## 0.26.0 native attachment qualification (2026-10-10)

New `jar_attachment_tests` executes production attachment creation, force and
projection code, including real Box3D bodies for mixed connections. Native Pmove,
collision queries and trajectory services are fixture boundaries; the simulated
native gravity loops explicitly exercise 20/60/120 Hz input time steps, not a
launched game. Tests cover unequal mass/momentum, slack, collision-blocked
projection, world and player-zero anchors, local thruster rotation/direction,
on/off/removal, frozen/protected state, native pickup gravity rebasing, projectile
trajectory retention, mixed solver response, stale/dead entity pruning, saved
references, autosave omission and rejection of native references labelled wire 5.
Production Duplicate tests cover internal NPC/prop rope cloning/remapping and
explicit exclusion of external native/world links. C-menu tests check token-bound
Rope/Thruster/Unlink dispatch. Full release gate is **44 suites**, plus retail and
packaged asset checks. V9 save header `0x4a41520a`, workshop wire 6, APIs 1020.

Native force adapters leave player/NPC controllers in place. Scripted/path movers
remain rope anchors; thrusters on these require further adapters. Native/mixed
ropes sweep endpoints against geometry but do not wrap the rope around scenery.
Native angular torque, vehicle/script integration and active-load/campaign/save
acceptance remain limited or unqualified. No game launched and no goal closure.
Workflow log: `build/attachments-20261010/release-install.log`; before sources:
`build/attachments-20261010/before`. Exact installation evidence follows below.

## 0.25.0 native assembly and catalog qualification (2026-10-10)

`jar_entity_setup_tests` executes the production portable setup transaction plus
the actual native rack, camera and distance-trigger callbacks. These callbacks
are shared from `g_rack_spawn.inc`, `g_security_camera.inc` and
`g_trigger_distance.inc`; engine collision, item finalization, view services and
allocation are explicit headless fixture boundaries. Cases cover mixed weapons,
five-item ammo shelves, difficulty counts, one-time synchronous fill, helper
capture, allocation/space rollback, normal map allocation outside capture,
camera view entry/movement exit/death exit, rejected late-effect ownership and
distance near/far/one-shot/rearm behavior. The test does not emulate the renderer
or prove native pickup collection and camera visuals.

`jar_duplicate_tests` now also executes production native child classification
and ownership, checks late effects in saved ownership records, rejects unsupported
children/full capacity, and checks partly emptied and empty rack duplication.
Paste removes freshly regenerated helpers absent from the clipboard instead of
replenishing deleted/collected pickups. Existing full group Undo tests still apply.
The independent retail fixture checks all 75 presets and required rack pickups,
camera/base models, movement/switch sounds and spark assets. Catalog: 78 of 182
formerly blocked routes implemented, 104 blocked. V8/wire 5/APIs 1020 unchanged.

Full gate: 43 headless suites plus retail/package checks, normal build/package/
install workflow. Log: `build/catalog-20261010/release-install.log`; source backups:
`build/catalog-20261010/before`. Exact installed ZIP/receipt follows below.
No game launched; native save/load, placement/collection/camera visual behavior,
cleanup/transition acceptance and other checklist items remain open.

## 0.24.0 assembly/property duplicate qualification (2026-10-10)

`tests/jar/duplicate.cpp` executes production clipboard, native controller,
paste/rollback and undo implementations. Native spawning, collision, actor services
and resource loading are explicit fixture boundaries. Cases cover connected
properties/joint references, thruster settings, copy after source deletion, partial
spawn failure, blocked placement, full link capacity, occupied undo, deleted/reused
slots, NPC health/freeze, pickup quantity, portable helper reuse/poses/cyclic links,
owned target graphs and resulting state encode/decode. The real Box3D bridge also
simulates original/copied welded assemblies and compares relative positions/angles.

The native game build compiles the new explicit-placement path used by paste;
existing production portable setup, native controller/save and physics fixtures
remain part of the gate. The workflow runs **43 suites** plus retail/package asset
checks. Logs: `build/duplicate-20261010/release-install.log`; prior sources:
`build/duplicate-20261010/before`. Exact receipt is recorded after installation.
No game launched. Full native placement/controller/save/cleanup acceptance remains
open, as do walking confirmation, catalog gaps, native attachments and broad stress
or campaign validation. V8/wire 5/APIs 1020 are unchanged.


## 0.23.0 entity context and continuous Physgun rotation (2026-10-10)

Production context/undo/native controller code is exercised by a new headless
fixture: token replacement, entity generation reuse, invalid/non-finite/injected
inputs, health/player-state synchronization, bounded transforms, blocked placement,
occupied-object gating, native mover endpoints/trajectories, frozen-controller undo
and native held rotation. Engine collision, UI and actor services are fixture
boundaries. Existing native controller save/load tests remain applicable.

The real Box3D fixture now exercises free held rotation, release-to-fling, rejected
invalid angles, a weld resisting rotation and a hinge permitting its axis. The
real UI builder/controller fixture checks C-menu unpaused lifecycle, field edit
capture, token refresh, world clicks, safe numeric command construction and no Q/C
release interference. Binding tests cover independent C backup and later user edits.

The normal release gate runs 42 suites plus retail/catalog/packaged asset checks.
Full workflow output: `build/context-20261010/release-install.log`; changed-source
backups: `build/context-20261010/before`. Exact release and installation receipt are
recorded below after the workflow completes. V8/wire 5/APIs 1020 remain unchanged.
No game was launched; native UI/rotation feel, property save/load in gameplay,
walking, campaign integration and physics stress acceptance remain open.


## 0.22.1 Noghri staff and native Sandbox crash fixes (2026-10-10)

The owner reported a crash when attacking with the Noghri staff. Read-only analysis
of retained dumps against the exact 0.22 build's local PDBs identified:

| Dump | Matched function and original line | Fault |
| --- | --- | --- |
| `jar_sp.x86_64.exe.47280.dmp` | `WP_FireNoghriStick`, `wp_noghri_stick.cpp:42` | Read at `0x1c0`: player has no NPC aiming record. |
| `jar_sp.x86_64.exe.18120.dmp` | `DoImpact`, `g_active.cpp:1244` | Read at `0x1a80`: a moving prop has no client record. |

Both were build `20261010-085746-3577ab16`. The second session's final log records
Physgun grabbing `misc_exploding_crate`. Symbolized locations are recorded in
`build/noghri-20261010/crash-locations.json`; retained log copies are alongside it.
DbgHelp read local files and symbols only; no game was launched or attached.

Noghri player shots retain base spread without dereferencing NPC aim data. Player
direct-hit damage is explicitly 10, matching the existing hard-difficulty NPC
shot; NPC damage remains 1/5/10. Force Sight 2+ still removes spread. Prop impact
damage checks a client before the vehicle class; destructible props still take
native impact damage. A related source audit found and fixed missing NPC and enemy
dereferences in linked grenade shooters, including the position-only destination
path. Diagnostic version text now derives from the release macros.

Three new headless fixtures execute the production firing/impact implementations:
repeated player staff fire, NPC aim/difficulty, Force Sight, entity exhaustion;
low/high-speed non-client prop impacts and retained vehicle/client handling; native
and aliased grenade shooters with entity/position targets, player throws and
exhaustion. Engine allocation, collision/trajectory solving, damage application
and audiovisual services are mocked at the fixture boundary. The impact and
thermal firing bodies are shared includes to avoid copying runtime logic into tests.

Focused fixtures passed. The normal release gate runs **41 suites** plus catalog
and packaged asset checks. Full output is `build/noghri-20261010/release-install.log`;
exact installed hashes/backups are recorded in its matching `build/deploy` receipt.
The pre-fix sources are retained under `build/noghri-20261010/before`.
V8 `0x4a415209`, workshop wire 5 and game/renderer APIs 1020 remain unchanged.

**No gameplay session was run.** These fixes address the identified faults and
pass their headless regressions; owner confirmation in game is still needed.
Walking jitter and the 108 blocked catalog entries remain on the overhaul backlog.

## 0.22.0 portable map entities and player foot-speed policy (2026-10-10)

Adds 71 portable native setups and three NPC aliases, reducing blocked entries
from 182 to 108. Catalog size stays 1,197; 154 registered entity-class cards now
have a spawn route. A shared preset manifest drives generation and runtime setup.
Activate/Link operate native callbacks/target fields. Stock box models replace
inline map geometry; owned destinations, train routes, feedback targets and keys
are allocated transactionally. Helpers, incoming links and pending target thinkers
are cleaned up on removal. Train route replacement/cyclic links are rejected.

The default `g_noFootSlide 1` applies adaptive speed to NPCs only. The production
animation fixture now exercises player horizontal velocity alternating between
zero and nonzero, independent of the displacement estimate, while preserving NPC
scaling. `2` retains the legacy player-adaptive option. The owner accepts beam
appearance and still reports leg-only jitter; gameplay confirmation is outstanding.

The new production setup fixture checks parser-state restoration, independent
target strings, train route/initial position, dispatch aliases, no-space/capacity
rollback and invalid mine placement. Native SP callbacks are mocked; this fixture
does not establish complete native controller behavior. Existing menu, native-hold,
serialization and state checks cover the new modes and V8 save header. Retail
coverage independently verifies every preset's stock model and selected effect,
sound and helper-key resources.

The normal release gate runs **38 headless suites**, retail coverage and packaged
asset checks. Full build/package/install output is
`build/spawnable-20261010/release-install.log`; exact ZIP hashes and changed-file
backups are recorded under `build/deploy/`. Source backups for this change are
`build/spawnable-20261010/before`. V8 `0x4a415209` prevents older engines from loading
portable brush setups they cannot interpret. Workshop wire 5 and APIs 1020 remain.

**No game was launched.** Walking, native setup interactions, rendering, save/load
gameplay and stress performance require in-game acceptance. See the current
[backlog](JAR-overhaul-checkpoint.md); older dated sections below retain their
original results and are not current completion claims.

## 0.21.0 retail catalog and walk-cycle correction (2026-10-09)

The owner narrowed the remaining stutter to leg animation, with smooth movement.
Player no-foot-slide scaling now uses horizontal velocity, and resulting movement
speed uses the current command's starting origin. The production animation fixture
exercises repeated complete walk/run cycles while the server displacement estimate
alternates between zero and a large value, plus independent NPC speed scaling.

Physgun wheel direction is inverted. Its one-frame blue stock laser is submitted
before the effects draw pass, avoiding accumulated electrical arcs. Toolgun shots
play the native Bryar firing sound. The generated 1,197-entry catalog adds search,
retail name coverage, explicit unsupported reasons, NPC spawner presets, native
point entities and stable saved IDs. Native spawn-key globals are isolated from
the last map entity. The catalog guide records coverage and practical limits.

Focused animation/menu/state/serialization suites passed. The release gate runs
all 37 headless suites, independent retail coverage checks and icon/effect/sound
asset checks. Full output is `build/catalog-20261009/release-install.log`; deploy
receipts and exact source/symbol archives identify the final installed build.
Changed source backups are in `build/catalog-20261009/before`.

Save header V7 `0x4a415208`, workshop wire 5, game API 1020 and renderer API 1020.
Old headers remain readable; older builds reject new saves. **No game was launched.**
Visual confirmation of the gait/beam fixes and gameplay acceptance of the expanded
retail catalog are outstanding; compilation and fixtures do not establish them.

## 0.20.0 Sandbox control and physics qualification (2026-10-09)

Implements the owner's October 9 control/physics/UI request. The release gate runs
37 headless suites, including three new production-code fixtures for native hold
lifecycle/save chunks, leg animation phase, and actor/prop support and collision.
Expanded solver/state/menu/face tests cover mass ratios, release momentum and
gravity, 120 Hz cadence, collision hit events, Rope/Thruster behavior, old wire
formats, maximum records, corruption rejection and stock-icon asset provenance.
The retail-asset check verifies 15 copied icon aliases byte-for-byte, 21 direct
stock icon references, both beam shaders and both collision sounds.

Build/package/install output: `build/overhaul-20261009-release-install.log`.
The normal deploy receipt records exact ZIP/installed hashes and changed-file
backups. Original sources changed this turn were backed up under
`build/overhaul-20261009/before`; prior releases and source/symbol archives remain.

New full save header V6 `0x4a415207`, workshop wire 5; Game API 1019, renderer
API 1020. Matching engine/game DLLs are required. Earlier saves remain readable;
retain older saves for rollback to older builds.

**No game was launched for this release.** Required native acceptance: wheel
distance/freeze/fling; primary versus alternate Toolgun actions and shot effects;
heavy/light prop feel; walking/riding/standing under rotation and obstruction;
NPC/native holds; native save/load; ropes/thrusters under load; run/walk cycles;
top-left help and compact menu readability; varied speech in Classic and Rend2.
The 4,096-body allocation fixture is not a sustained active-physics benchmark.
The catalog remains 53 entries; physical attachments require spawned Box3D props;
Duplicate does not clone scripts/assemblies; visual rope segments have no wrapping.


## 0.19.0 Facial preset qualification (2026-10-08)

Adds live Stock/Expressive/Exaggerated/Extreme facial accents and speech
sensitivity to Styles. Stock remains default. Continuous DMA voice energy is
separate from native script/animation states. Additive jaw/lip/brow matrices use
existing Ghoul2 APIs in both renderers, with ownership flags for cleanup.

New tests exercise production PCM amplitude extraction, monotonic response,
finite bounds, frame-rate-independent smoothing, jaw pivot invariance, lower-lip
following, real cgame controller application/cleanup, restored accent ownership,
independent bone overrides, malformed/missing face rigs, and real menu Apply,
Revert and graphics-preset isolation. Full build and CTest results are retained
in `build/face-release-install.log`; exact release files are verified by deploy.

No game was launched. Manual checks still required: compare all four settings
on speaking player/NPC faces in both renderers, including quiet/loud dialogue,
pauses, death, save/load, video restart and model changes. Inspect teeth and mouth
interiors at Extreme; headless tests cannot establish artistic quality.


## 0.18.0 Weld/Hinge qualification (2026-10-05, the development workstation)

This milestone adds saved Weld/Hinge constraints and Unlink to Toolgun, with
stable owned-object references, a 128-link limit, transactional wire validation,
Undo/lifecycle handling and Physgun angular freedom on linked props. It keeps
Box3D Off by default. Game API 1017 pairs the new engine/game save contract;
renderer API 1020 is unchanged. Full saves use V5 `0x4a415206`, workshop wire 4.

The normal release gate runs all 31 Release tests. The four affected suites
(physics, workshop state, serialization, menus) also pass in Debug. New checks
exercise rotated welds, frozen-to-dynamic hinges, pivot radius, saved swing,
rounded quaternion normalization, linked dragging, Unlink/identity reuse,
128-link capacity, the maximum 29,728-byte record set, corrupt references/frames,
transactional rejection, old wire formats and native menu command whitelists.

Native evidence uses disposable profiles and bounded scripts:

- `constraints_classic_v5` / `constraints_rend2_v5`: actual Toolgun primary input
  creates Weld and Hinge. Weld sag is 0.562 native units in this two-crate fixture.
  Hinge moves under gravity; both motion records and local frames restore exactly
  at load. Off retains one saved link with zero solver joints; On recreates it;
  Unlink and cleanup remove it.
- `constraints_lifecycle`: secondary cancels a pending pair, self/duplicate pairs
  are rejected, Undo restores linked freeze state and removes the last joint,
  Rotate is blocked until Unlink, deleting a member clears the link, reused
  entity 193 receives ownership ID 3, and map transition clears all links.
- `constraints_drag_classic` / `constraints_drag_rend2`: actual Physgun ID 29
  drags the hinged crate from Z 1912 to 2002.619 while the frozen anchor stays at
  Z 1912. Primary release freezes it there; Gravity resumes its swing.
- `constraints_ui_classic_v3`: actual held-Q binding and window-local pointer
  clicks choose Weld, Hinge and Unlink, with screenshot inspection of all labels.
- `constraints_compat_017`: the prior 0.17 occupied-tauntaun save loads, then
  resaves/reloads as 0.18 with occupancy intact. `constraints_old_017`: the old
  installed reader rejects header 1245794822 before workshop/world initialization.

Earlier fixture attempts are retained as failures of setup, not passes: weapon
selection attempted while in noclip; a read-only gravity cvar; incorrectly aimed
eye height; and overlay helpers called without the developer flag or before Q's
binding update. Two stalled disposable UI fixtures were closed through their
verified process's normal window-close event. No game process was terminated.

`build/validation/JAR-0.18.0-constraints-evidence.zip` retains the source delta,
review, scripts, diagnostics, screenshots, unit logs, deployment verification and
the final installed smoke results. The source/symbol archive records the exact
shipping build. Prior 0.17 package/source/symbols and existing user configs,
saves and retail PK3s are retained. No third-party core or new asset dependency
was added for joints.

The current gap audit is in `JAR-overhaul-checkpoint.md`. This is not completion
of every requested change: full catalog coverage and unrestricted map/controller
editing remain unfinished. Props do not drive or damage native controllers.
Physical controller hardware, large joint assemblies and a full manual campaign
remain unqualified. Unchanged camera, fists, native menus and projectile/sound
paths retain the prior 0.17 evidence; their full native matrix was not rerun just
for this audit.


## 0.13.0 bounded melee milestone (2026-10-04 UTC)

All work remained on the development workstation. Installed 0.12.0 was retained before edits:
28 runtime files, two player config/save files, and hashes of all 66 base PK3s.
Disposable profiles and guarded, cleanly exiting native sessions were used.

- **27 Release and 27 Debug checks pass.** The new fist fixture compiles the
  production admission/animation helper and covers accepted/rejected animations,
  standing/moving/crouched input, indefinite timers, completed and unfinished
  kicks, grab holds, release, Legacy/invalid values, NPCs and nonhumanoid models.
  A read-only review caught the initial playable-creature scope gap; the final
  predicate requires the native humanoid animation set and regression cases
  include Rancor/Wampa and negative/nonzero animation indices.
- Native Classic/Rend2 hold tests produced 16/17 primary punches versus eight
  with Legacy over the same window. No primary events occurred after release;
  a tap produced one. Movement, kick-to-primary, missed-grab-to-primary and full
  save/load resumed punches. The original baseline also produced no attacks
  after release: this release repairs cadence/held-input continuity, not a
  demonstrated queued-damage bug. Existing per-hit range/damage stays native.
- Shared production saber math is tested across lengths and frame times,
  Quick/Legacy/invalid values, protected paths, saved partial lengths and bounds.
  Native single, dual, staff, solid sword and full-save/load cases run on both
  renderers. Timing uses actual frame sums, so final duration may exceed its
  target by a frame. Solid `sith_sword` retains its native path.
- Native gun selection/fire and screenshots confirm that the user's first-person
  preference returns after fists. Both renderers retain cinematic views and
  return; because the player was outside that cinematic view, those captures do
  not measure protected-camera ignition. Thrown re-ignition observed the native
  protected rate on both renderers; all logged steps match the selected formula.
  Native wall/retraction changes interrupt some throws, so elapsed throw time is
  not reported as a continuous ignition measurement.
- No third-party melee code/art/binary was imported. Stock `sith_sword` mechanics
  and mounted definitions were inspected; JA+ release notes were only a behavior
  reference because source/animation reuse terms were not verified.
- No possession-mask, wield-count or save-format change is part of this release.
  The included inventory plan requires stable definition IDs, bounded carried
  records, qualified asset registration, a versioned save extension and legacy
  migration tests before implementation. It does not claim an expanded inventory.

Evidence workspace: `~/Documents/Codex/2026-10-01/task-3/melee/`.
Installation, preservation hashes, native captures and exact release evidence
are recorded in `build/validation/JAR-0.13.0-melee-evidence.zip`.
Full successful-grab/opponent combat, physical controller, vehicle and campaign
coverage remain open; bounded native fixtures do not certify those paths.


## 0.12.0 native menu and missing-projectile milestone (2026-10-04 UTC)

All work remained on the development workstation. Before edits, retained the installed 0.11.0
runtime (28 files) and current two profile/save files and hashed all 66 installed
base PK3s. Tests used the guarded isolated runtime and disposable profiles,
native commands/screenshots and clean exits. No existing game was terminated.

- **25 Release and 25 Debug checks pass.** Production menu-builder tests cover
  preserved actions/cvar conditions, column focus order, hover-undertext removal,
  idempotent restyling, and unrelated/custom loading controls. Loading stages are
  bounded to 0-9. Existing seed, preset, camera, ownership and save regressions
  remain in the suite. Read-only review findings were fixed before packaging.
- Native before/after captures cover main, pause, character and saber screens;
  final Classic/Rend2 shell captures exercise both new-character and campaign
  saber states. Active loading captures show the actual stage 8/9 bar, levelshot
  and briefing. Earlier static/failed baseline-loading attempts do not establish
  a before capture of active loading. Physical-controller, multilingual and
  complete campaign flow qualification remain open.
- Corrected an evidence limitation in the 0.11 projectile audit: generic model/
  trail flags did **not** establish that vehicle effects loaded or rendered. The
  vehicle branch returns before the generic trail. Mounted source inspection
  found missing `ships/bomb` and `ships/ion_blastershot` effects with no model.
  The new original two-primitive fallbacks retain the original effect keys and
  only apply when native loading reports that the requested file is absent.
- The actual packaged effects passed all four **Classic/Rend2 x stock/MBII**
  combinations: 12 fixed shots (seeds 54 and 234), both effects registered with
  two primitives, full save/load resumed the same fixed bomb selection, and
  each process exited cleanly without fallback shader-load warnings. Open-air
  native trajectory captures additionally show the cyan ion in flight; short
  near-wall bursts are not sufficient visual evidence for a very fast projectile.
  The initial inline-shader syntax attempt and a non-firing noclip fixture are
  retained as failed attempts, not passing evidence. This is a targeted repair,
  not certification that every one of the 78 variants is equally readable.
- An existing custom bomb definition and an existing empty ion definition both
  retained native precedence, with zero fallback registrations. A custom loading
  screen retained its own text, background and original progress bar. The
  production helper tests separately cover malformed/existing files and editor
  paths; no broad effect override was introduced.
- Normal-map production menu transactions passed on Classic and Rend2 with
  cheats disabled: Chaos plus video restart retained six live overrides (speed
  306 from baseline 250), then Vanilla restored all six baselines. No seeded
  selection, emitter, damage, cadence, collision, save format or camera policy
  changed in this milestone.
- Appearance packaging checks **2,295 entries**, 12 font tables and dependency
  closure, including the two privately namespaced original fallback effects.
  Fallback contents participate in the package cache fingerprint. The effects
  reference installed retail shaders and import no raster assets; their original
  source/license notice is included. This does not grant redistribution rights
  for the pre-existing locally derived retail/mod appearance content.

Evidence workspace: `~/Documents/Codex/2026-10-01/task-3/native-ui/`.
Release installation is performed from the hash-verified ZIP, with a per-file
receipt under `build/deploy/`; subsequent installation/preservation hashes and
native smoke results are recorded in `build/validation/JAR-0.12.0-native-ui-evidence.zip`.
Full remaining scope: [overhaul checkpoint](JAR-overhaul-checkpoint.md).

## 0.11.0 first overhaul milestone (2026-10-04 UTC)

All work and native execution stayed on the development workstation. Baseline 0.10 installation,
settings and saves were retained before edits. Disposable test assets were
aligned with the current 66 installed base PK3s; four old test copies were moved
to a retained workspace folder. Normal player files were not test inputs.

- 25 Release and 25 Debug checks passed. Production menu tests cover all 19
  category presets, pending navigation/cancel/revert, invalid transactions,
  seed/mode and independent-preference preservation, actual new-profile gating,
  latched values, native omitted-default migration and live ownership restoration.
  The menu fixture links the production live randomizer for Intended/Vanilla
  restore interactions. Camera policy tests cover Legacy, Comfort, Crisp and
  protected/invalid preset selection.
- Captured native baseline main/pause/special/character/saber screens and static
  loadscreen menu; captured Simple/Advanced and movement through all three camera
  presets in Classic and Rend2. These are bounded smoke tests, not full campaign,
  physical-controller or active-loading-bar certification.
- Classic and Rend2 normal-map native production UI-script fixtures, with cheats disabled: Chaos
  plus a graphics restart retained six live overrides, including speed 306 from
  baseline 250. Vanilla restored all six baselines. Initial QA fixture attempts
  that failed to load a menu or queued assertions ahead of Apply are retained as
  failed harness attempts; the corrected chained fixture supplies the evidence.
- Complete projectile audits executed 109 Classic and 110 Rend2 shots and all 78 qualified
  variants. Observed travelling shots had a model or trail; hitscan beams remain
  instantaneous. New per-missile diagnostics do not change gameplay. Broader
  projectile readability and optional effect-pack qualification remain pending.
  Turning the current projectile toggle Off now stops future random shots; the
  real emitter fixture verifies both inputs, preserved reroll sequence/seed
  marker, qualified re-enable and unqualified-world rejection. Native Classic
  emitted 5 random shots, then 4 native shots with no random decisions while Off,
  then resumed random sequence at 5 after re-enable.
- Read-only review identified migration, live ownership and restart-ordering
  defects before release; each was fixed and covered. Native camera collision
  and authoritative camera paths remain unchanged.

Complete scope and explicit remaining work: [overhaul checkpoint](JAR-overhaul-checkpoint.md).
Workspace evidence: `~/Documents/Codex/2026-10-01/task-3/overhaul/`.


## 0.10.0 seed-fixed mappings (2026-10-03 UTC)

Validated on the development workstation using disposable profiles and the existing copied mounted
assets. No other game was interrupted and no normal player settings/saves were
used as writable test data. The installed 0.9 rollback and profile copies are
retained under `build/retained-versions/installed-0.9.0-before-fixed-mapping-20261003-120300`.

- **25 Release and 25 Debug checks pass.** Fixed-source tests exercise the actual
  pickup/NPC/prop/arsenal and shot implementations, audio runtime/cache and all
  audio entry routes, the live-cvar transaction, save migration and menu builder.
  Repetition, reversed event/catalog order, catalog removals/restoration, seed and
  mode changes, alias normalization, reload/reconstruction, invalid modes, native
  identity/mission guards and legacy full-cycle coverage are asserted. Independent
  golden hashes pin unsigned arithmetic and key normalization.
- Audio fixture tests distinguish permanent decode rejection from temporary cache
  pressure. A busy cache must use native fallback and later return to the same
  fixed destination. The new audio test exercises music literals/default sets,
  SFX, loops, voices and embedded movie playback through production helpers.
- Classic and Rend2 native T1 Fatal runs have identical whole-world counts and
  digests: **19 changed pickups, 27 NPCs, 49 props, arsenal mask 8832** at seed 73.
  Individual source groups remain fixed, and both fresh processes produce the
  same choices. Detailed records are bounded during precache; the unthrottled
  world digests cover every decision, including records omitted by that bound.
- Native blaster primary maps to `disruptor-charged-beam-10`, blaster alternate to
  `thermal-impact-0`, and repeater primary to `tripmine-proximity` for seed 73 and
  this mounted catalog. Repeated metal-step source files retain their destinations
  after other events and full-save reload. Seed 74 recomputes tested selections;
  per-event mode varies shots/audio again. This is evidence for these seeds, not
  a claim that every changed seed must produce a different replacement.
- Full saves preserve all 49 prop choices and the same fixed future shots/audio.
  Native cvar Apply repeats fixed values, rerolls in per-event mode and restores
  the saved values. Original movie timing survives fixed ROQ-source substitution.
  The 13-row Entity menu fits without overlapping status/actions; shared mode
  controls also exist on Sound and Cvars, with no renderer restart on Apply.
- A three-process reroll chain saves, loads and immediately resaves before firing,
  then reloads and fires **sequence 7 / bryar-charged-4**, matching the original
  continuation exactly. Review found the seed-marker registration edge behind
  this case; registration is now performed before any save, not only on firing.
- A real 0.9 recovery-bearing Taspir save loads with default fixed preferences and
  legacy applied-world mode, retains Rosh's identity/controller, completes his
  pending native script wait, and saves again with the same recovery format.

Evidence scripts, native logs, disposable saves, screenshots, review delta and
preservation hashes are collected in `build/validation/JAR-0.10.0-mapping-evidence.zip`.
The exact release ZIP installation has its own hash receipt under `build/deploy/`;
the evidence archive records follow-up tests of the installed binaries.

Qualification remains Windows x64 SP, SDL/DMA, Classic and Rend2. OpenAL is still
not enabled in this target. Qualified pools, NPC mission protection and optional
actor-recovery limits are unchanged. These regressions do not establish every
seed/mod/campaign path as softlock-free. Existing per-event loops/music/movie
choices remain stable while playing; changing mode/seed never restarts an active
movie or rewrites already serialized actors/items.

## 0.9.0 campaign and available audio coverage (2026-10-03 UTC)

Validated on the development workstation with disposable profiles and copied mounted assets. No
global input, game termination, multiplayer changes or OpenAL installation was
used. The verified installed 0.8 release and original profile files are retained
under `build/retained-versions/installed-0.8.0-before-campaign-20261002-225337`.

- **24 Release and 24 Debug checks pass.** New production-helper coverage tests
  ID3v2.2/2.3/2.4 metadata bounds, ROQ chunk bounds/short reads/native signed
  predictors/large skipped video chunks, and movie output at 11025, 22050, 44100
  and 48000 Hz with uneven packets, silent tails, rewind and restart fallback.
- Static mounted-content audit: **34 shipped SP maps, 1,986 NPC spawners, 2,595
  compiled scripts, zero parse errors**. Shared references are followed; yavin1b
  and t2_dpred are not mistaken for script-free maps. This is not all missions
  played. Counts/provenance are in `JAR-campaign-audit.csv`.
- Native production NPC decisions cover **311 spawners** across Vjun II,
  Taspir II, Korriban II and T1 Sour. All **208 named actors** are preserved with
  mission safety On; **59 ordinary spawners** receive feasible replacements.
  Triggered mission actors use original names/types. T1 Sour is a spawn/cinematic
  observation, not a save/load qualification or full mission completion.
- Vjun II Kyle is excluded by his authored death hook. Direct deletion stays
  final; loading the earlier native full save restores his original actor and
  pending navigation task. Korriban II Luke's unplanned death becomes a stand-in
  and survives a v1 full save/load. Killing actual `boss_kyle` executes
  `kor2/endgame_DS`, including its native camera transition, without recovery.
- Taspir II's actual `npc_rosh1` survives direct deletion, completes a controlled
  ICARUS wait group, reloads the full save made while pending and completes again.
  A subsequent authored REMOVE remains final. The native intro is skipped and
  allowed to clean up before saving. Two earlier cases tried to save during the
  camera and are retained as failed fixture attempts, not passing evidence.
- A copied ordinary 0.7 save receives one direct deletion and one authored REMOVE.
  Both actors remain absent after save/load and enabling recovery. Both saves keep
  native header 101; neither contains the deleted identities. No story state or
  actor resurrection is fabricated from map references.
- Mounted audio audit: **15,017/15,024 decode**, including the previously missed
  tagged MP3, the installed ID3v2.2 track and 13 stock ROQ soundtracks. One malformed
  WAV and six silent ROQs have explicit reasons. Twenty-five shipped maps have a
  blank music field; native default music now reaches the shared pool.
- Native Classic plays `video/ja01.roq` as a music destination at seed 2313;
  invalid labels/files do not map. Rend2 plays consecutive ja01/ja02, a looping
  movie, a held frame, then the silent crawl with its normal sidecar route. Each
  playback chooses once. Natural audio durations are 331081 and 324797 frames at
  22050 Hz. Captures for music, movie replacement and ROQ-as-music contain nonzero
  PCM. These are software-mixer captures, not a claim of human listening tests.
- Read-only independent reviews caught and verified fixes for cinematic rewind/
  refused-stop lifetime and preservation of existing ID3v2.2 support. OpenAL is
  excluded from the current x64 SP build; no backend port was needed.

Accepted fixtures, captured audio, structured assertions and logs are in workspace
`campaign/` (`native-evidence.json`). Earlier failed/superseded cases remain labeled.
The exact release source/symbols and deployment receipt are retained under `build/`.
Full campaign traversal, arbitrary missing-save reconstruction, unsupported actor
mechanics, foreign-language sidecar playback and other hardware/backend combinations
remain unqualified. Original settings/saves and all 70 base PK3s are hash-checked
after deployment; installed-build smoke tests are recorded with the release evidence.


## 0.8.0 opt-in missing-ally script recovery (2026-10-03 UTC)

Validated on the development workstation using isolated assets/profiles and scripted native console
commands. The 0.7 package, installed files and user profile/save hashes were retained
under `build/retained-versions/installed-0.7.0-before-recovery-20261002-214928`.

- All **23 Release and 23 Debug checks pass**. Production tests cover original
  spawner eligibility, authored death exclusions, task/controller/name retention,
  death cleanup bypasses, exact authored friction and animation timers, runtime
  cleanup suspension, player-clone/flying/gravity exclusions, and native internal
  temporary/default behavior precedence. A test of the actual native save-field
  rule protects every task ID beside the four-byte ICARUS handle on x64.
- Fresh Classic death and Rend2 deletion runs each remove the visible actors for
  `npc_roshintro1` and `npc_roshintro2` at game time 6800. Their original controller
  slots/owners remain. Both openings change camera 1 to 0 with player health 100
  and normal movement state. The player was god-protected for unattended testing;
  actor damage prevention was Off. Subsequent native `maptransition` to the map's
  authored destination `yavin1b` succeeds with health 100 and clean exit. This
  exercises transition cleanup, not a full traversal or campaign playthrough.
- A native combined dialogue/upper-animation/navigation task group survives death
  and completes. A full save made while all three waits are pending reloads and
  also reaches `parm1=combined_complete`; saved group members all become complete.
  A living-actor control passes the same reload. Navigation stops at its goal.
- Those tests exposed an inherited x64 writer defect: pointer-sized clearing of
  integer `m_iIcarusID` overwrote adjacent voice task 6 with zero. The typed-field
  fix preserves it. Older already-corrupted saves cannot be inferred/repaired.
- Native disabled-option, authored KILL, recovered-actor KILL, zero HEALTH,
  SAFE_REMOVE, REMOVE and runtime death-script cases retain their intended final
  outcomes. The authored callback executes. Unsupported combat behavior is logged
  and keeps its task pending instead of falsely completing the enclosing group.
- A native normal kick changes victim health 500 to 497 and moves it. The same
  scripted kick from a stand-in leaves health 500 and position unchanged. Mover
  contact and breakable impact guards were source-reviewed; an exhaustive native
  brush-contact matrix was not run.
- A retained **0.7 full save** from the installed-runtime scenario loads and
  resaves with native header 101. Recovery saves use v1 header `0x4a415201`. Actual
  0.7 engine/game binaries in the disposable runtime reject that header before
  reading game state and exit normally. Native partially played audio is not
  serialized; its existing post-load voice-completion behavior remains.
- Independent source review found and verified fixes for friction, contact/kick/
  impact side effects, specialized player-clone equipment and the save overwrite.
  An initially proposed scripted BS_WAIT case was invalid: native ICARUS does not
  expose that internal state. Actual internal WAIT is preserved and dispatched
  natively, with shared production precedence tests; no new script API was added.

Reproducible local fixtures, configs, logs and assertions are in the task workspace
`recovery/`; `native-evidence.json` records accepted outcomes. Superseded fixtures
are retained as investigation history and are not counted as passing validation.
The exact release source/PDB snapshot and deployment receipt live under `build/`.
Broader campaign, mod and hardware combinations remain unqualified. OpenAL's
installed 64-bit library is present, but the SP backend and unified audio contract
still require implementation and device/timing qualification; see JAR-randomizer.


## 0.7.0 broad randomizer catalogs (2026-10-03 UTC)

Validated on the development workstation, Windows x64 / RTX 4080 SUPER. Tests used isolated copies
of installed assets and disposable profiles, with scripted native console input
and no global keyboard/mouse injection. The installed 0.6 release remains retained
under `build/retained-versions/installed-0.6.0-before-breadth-20261002-202333`;
its ZIP and exact source/symbol archive also remain available.

- All **23 checks pass in Release and Debug**. Added production catalog tests
  cover full-cycle permutations, native NPC definition parsing/precedence,
  missing/custom/default skin paths, shot scope/state restoration, early-map
  guided acquisition and neutral-target rejection. Actual mixer/channel tests
  cover shared PCM ownership, logical-source stopping, authored voice deadlines,
  silent tails and short-buffer amplitude bounds. Existing serializer, entity,
  renderer, launcher, UI transaction and legacy-setting tests remain enabled.
- Native stock audio audit: **13,588 / 13,588** files decoded. Full mounted audit:
  **15,003 / 15,005** decoded. The two exact rejected assets and format reasons
  are listed in `JAR-randomizer.md`; every file has a TSV audit row.
- In separate Yavin voice-only and combined safe NPC/audio scenarios, **20 real
  dialogue events** used broad replacements, including dialogue-to-effect and
  dialogue-to-other-character substitutions. The combined scene changed camera
  state from 1 to 0 and exited normally. Original captions/voice deadlines remain
  authored. Subjective listening quality is not a coverage claim.
- Classic and Rend2 each fired **207 shots** with an **identical ordered sequence**
  spanning all **78 admitted variants**. The full-save checkpoint restored sequence
  128 and continued through 207. Both exited normally. The original Classic case
  used a position outside playable geometry; Rend2 repeated it at an authored
  playable location. These checks establish emission/resource stability, not
  complete combat balancing or damage-model equivalence.
- Focused native proton-torpedo and guided-rocket cases acquired hostile entity
  227, emitted actual homing missiles targeting 227, and completed full-save
  reloads. Other full-cycle shots verify the no-target unguided path. Resource
  registration occurs after native vehicle precaching and is refreshed after
  restored configstrings without reordering weapon identities.
- The initial broad-NPC `t1_fatal` scenario changed 27 existing spawn locations.
  The final mounted catalog has **193 viable definitions from 246**. Safety On
  protected all authored Yavin intro actors; Experimental Off changed named
  actors while retaining script-facing identities and original allegiance. The
  experimental run also reached camera 0; this is not proof for other seeds/maps.
- With named-ally continuity On, an explicit lethal hit to `npc_roshintro2` left
  that same actor alive at health 1 with its original script identity/task state;
  the scene subsequently reached camera 0. Production tests also cover authored
  kill scopes, ordinary death callbacks and friendly-fire death callbacks, which
  must not be suppressed. Already deleted actors are not reconstructed.
- A retained 0.6 save containing an in-flight repeater projectile loaded cleanly
  in 0.7, restored its applied seed/projectile enablement, defaulted new safety
  On and continuity Off, and fired twelve new catalog shots without errors.
- Native screenshots confirm Entity's eleven controls and Sound's five role
  toggles are visible and readable. Production UI tests exercise pending/apply/
  cancel/restore transactions and safe preset defaults.
- A combined installed-runtime smoke exposed a missing armed-Jawa weapon model.
  Candidate validation now checks every explicitly precached NPC weapon and the
  native fallback loadouts for restored source teams. The broken candidate is
  excluded; both renderers repeat the combined NPC/audio/projectile save-load case.
- Independent source/asset review found and drove fixes for shared audio-cache
  ownership, dialogue timing/bounds, actor identity/allegiance, script kill scopes,
  vehicle resource types/precedence, native save registry ordering, neutral
  targeting, early-map homing and embedded default-model materials.

Detailed JSONL logs, catalog TSV, screenshots and `gameplay-evidence.json` are
retained in the task workspace's `breadth` directory. Release deployment receipts,
SHA-256 manifests and exact source/PDB archives live in the repository's `build`.
Full-campaign progression, arbitrary mod combinations, OpenAL substitutions,
subjective listening and general removed-actor recovery remain unverified or
unsupported as documented. No multiplayer or anti-cheat paths were activated.

## 0.6.0 Entity, Sound and Cvars randomizer expansion (2026-10-02 UTC)

The 0.6.0 runtime was exercised on the development workstation, RTX 4080 SUPER, Windows x64,
at 1280 x 720 windowed in both Classic and Rend2. The candidate was
`20261001-222300-d4d30bab`; the final package includes this evidence and a
deployment-default version correction. Exact source hashes, matching PDBs and
binaries are retained by the release build. No full-campaign or arbitrary-mod
compatibility claim is made.

- All 19 automated checks passed in Release and Debug. These include production
  randomizer policy, native UI actions, sound mixer capture, save/load ownership,
  capacity/serializer, renderer allocation, launcher and diagnostics regressions.
- Stock `t1_fatal`, seed 73: both renderers made 19 pickup, 13 NPC and 49 static
  object substitutions, with identical digests (321404775, 923407991,
  3246565075). Seed 74 changed all three digests (3352680724, 2436350950,
  1611514628). The object pool is 14 fingerprinted stock models; scripted,
  unknown, scaled and oversized alternatives stay protected.
- Full saves retained seed-73 objects and projectile enablement after pending
  settings were changed to seed 999 and those switches were disabled. The
  restored world kept all 49 object choices and its original object digest.
- In `t1_surprise`, seed 73, primary blaster shots selected native repeater
  projectiles in both renderers, before and after full-save loading. A live
  `repeater_proj` was present when saved, 50 game milliseconds after firing.
  Secondary blaster fire kept the native 2300-unit/second projectile; remapped
  primary shots used 1600. Weapon inventory/ammo and ordinary alt-fire dispatch
  remain native. Earlier prop-scene checks fired the protected disruptor and
  are not counted as projectile substitution coverage.
- Ordinary `map yavin2` sessions, with `helpUsObi=0`, applied all six live
  settings and resumed the same values after a full save: gravity 658, speed
  254, scene brightness 0.914, FOV 104, simple sky 1 and dynamic lights 0.
  Restore returned owned values to their baselines while retaining a later
  manual brightness edit. Loading a save with no active live roll preserved
  current brightness/FOV preferences. Config writes retained user baselines.
- Fixed-camera `t1_surprise` screenshots confirmed immediate dimming at 0.85
  and lifting at 1.25 in both renderers. Restoring scene brightness to 1 yielded
  exactly zero pixel difference from the original screenshot. This scene-only
  adjustment precedes the HUD and does not change native gamma or restart video.
- Actual software-mixer WAV captures verified nonzero stereo 44.1 kHz output
  for original/remapped effects, remapped loops, streamed music, dynamic music
  and restored audio: six bounded captures per renderer, 26 seconds each.
  Dynamic `t1_surprise` music mapped to `t2_wedge` and reached action state;
  restore returned to `t1_surprise`. `music/death_music.mp3` stayed original
  with dynamic music disabled. Dialogue/control identities remain protected.
  This is PCM/output validation, not a claim of a human listening review.
- A retained 0.5.0 checkpoint loaded successfully with all new feature switches
  reset off. Native screenshots verified the Entity, Sound and Cvars menus;
  headless production-UI tests cover Apply/Cancel/restore and restart confirmation.
- An independent code review found save ownership/cheat-reset, inactive graphics
  persistence, OpenAL residency/capture, protected music and mounted-player edge
  cases. All were corrected and re-reviewed without a remaining confirmed defect.
  OpenAL is not enabled in this x64 build; its rejection policy was tested, not
  its real playback backend.

Evidence is retained locally under
`~/Documents/Codex/2026-10-01/task-3/expansion/`: profile-specific
JSONL traces, qconsole logs, native PNG screenshots, WAV captures, saves, test
logs and `gameplay-evidence.json`. Each profile is separate from normal user
settings/saves. One abandoned test fixture granted a weapon inside solid
geometry and triggered the existing native give-command error; it was corrected
by granting at the native spawn, then rerun successfully. It is excluded from
passing evidence. Existing native asset skin warnings and Classic reload
`glGetError() = 0x501` diagnostics remain; no visual corruption or fatal failure
was observed in the completed cases.

Remaining coverage: extended campaign progression and vehicle/mounted combat,
broader mod stacks, alternate sound backends, sustained sessions and subjective
audio review. Arbitrary entities, scripted mission NPCs/props, dialogue and
unbounded physics remain intentionally outside this release's supported pools.

## Historical capacity and graphics records

Historical 0.3.0 status: JA:R 0.3.0 adds display and appearance controls; the release build runs 16 headless checks plus generated-asset validation. Prior 0.2.1 passed its 15 Release/Debug checks. The owner reported that the previous 0.1.1 build appeared stable and could no longer be forced to crash in their tests. That is owner feedback on 0.1.1, not validation of the new Rend2 renderer. No game was launched for this update.

## Graphics and crowd work

The source pin and integration boundaries are recorded in `JAR-renderer-sources.md`.
The new renderer retains the high-ID sort encoding, model-handle and surface
limits. The audit also fixed the donor's wrapping skeletal surface heap,
unchecked GPU constant writes, insufficiently checked aligned allocations and
inverted shadow-exclusion flag checks. The renderer API is 1019, game API is
1011 and cgame import API is 5; all modules must come from the same package.
Save version 101 and local protocol 1040 remain unchanged.

New tests cover the production Rend2 sorter, surface pool, frame allocator and
constant writer; production menu construction/staging with substituted engine
services; preset values; and the optional NPC scheduling policy. Two additional
tests load the real renderer DLLs, reject the old API, fill all 32,768 Ghoul2
handles, verify controlled exhaustion and exercise reuse. They do not call GPU
initialization or start the game.

The synthetic scheduling test runs 100 eligible stationary actors for five
simulated seconds. It checks that 400 ms scheduling performs fewer than one-third
as many decisions as the 100 ms baseline. This measures the scheduling policy,
not real NPC processing cost or FPS. The near/active/disabled cases retain the
normal interval in policy tests. Gameplay remains necessary to evaluate reaction
delay and campaign behavior.

Before calling 0.2.0 gameplay-validated, compare Classic and Rend2 in the same
saved scene, at the same resolution and camera position, with 0, 25, 50 and
100 NPCs. Record GPU/CPU, FPS/frame time and whether actors are idle or fighting.
Compare Balanced and Maximum FPS, then isolate the AI toggle while keeping
graphics unchanged. Exercise menu navigation, Apply/Cancel, renderer switching
mid-level, save/load, portals, player customization, cutscenes, weapons, shadows,
gore, fog and effects. Confirm the launcher override can recover Classic after
an unsupported graphics setting. None of these live checks has been claimed.

## Reproduce automated checks

Run `scripts/build-jar.ps1` from PowerShell. To rerun the tests alone:

```powershell
ctest --test-dir build/jar -C Release --output-on-failure
```

The tests require no game assets and create no window. Snapshot encoding/decoding and renderer sort functions were moved into small production translation units so the same implementations can be tested directly.

The Windows launcher test uses a substitute child process in a path containing spaces, with invalid SDL and Visual C++ DLLs beside the launcher. It verifies isolation from those DLLs, the default asset directory, argument forwarding and exit-code propagation. The real engine and its 64-bit runtime libraries install inside `GameData/JediAcademyRemixed/`; the launcher depends only on Windows system DLLs. The installer rejects generic runtime DLLs at the shared GameData root.

`jar_loader_probe.exe` is a bounded developer diagnostic, not part of the release. It observes native dependency loading, then stops only its own child at the loader breakpoint before the application entry point. Passing this check does not establish game-module loading, rendering or gameplay.

## September 29 stress regressions

- Windows Application Error event 1000 at 07:36:22 recorded `jar_sp.x86_64.exe` failing with `0xc00000fd` (stack overflow). The weapon death callback recursively called `WP_Explode`, which reentered `G_RadiusDamage`; each active radius query retains a 16,384-pointer list on the stack. No native crash dump was available, so the exact crash stack has not been symbolized. The new 1,024-pack test runs the production blast enumeration and weapon callbacks with engine-service substitutes. It failed on nested damage before the fix and passes after scheduling damage-triggered explosions through the existing next-tick think state. No explosion queue can overflow, and pending explosions use the existing saved entity fields.
- `AllocRS` reused a 2,048-entry ring while pending draws still referenced its entries. This could pair one actor's shader/entity transform with another actor's mesh/bone cache. The surface lifetime test reproduced overwritten references before the fix. The pool now holds 32,768 entries, never wraps within a frame, returns null on exhaustion, and resets with the other frame storage after rendering. All four allocation sites handle exhaustion. The test covers scene clears within the same frame, repeated rejected allocations, unchanged earlier records and next-frame recovery; it does not render real NPCs on a GPU.
- Both stress regression tests passed in Release and Debug for 0.1.1, whose save format and game/renderer interfaces were unchanged from JA:R 0.1. They continue passing in 0.2.0; the renderer-related ABI changes for 0.2.0 are listed above.

## Gameplay checks still required

Use an isolated game-data copy and new JA:R saves. Record the executable version, map, mods, elapsed time, `jar_capacity` output, memory use, frame time and any console errors.

- Start a new campaign; exercise loading screens, level transitions, scripting, pickups, NPC dialogue, both saber and projectile combat, vehicles, cutscenes and pause/resume.
- Save and reload with entities above IDs 1,023, 4,095, 8,191 and 9,999. Check collisions, targets, attachments, saber models, sounds and AI after loading. The headless serializer tests cover data fields, not the entire game restore sequence.
- Increase a controlled stress scene in measured batches across the original 1,024-slot ceiling. Include a mix of actors, their helper entities, projectiles and effects. Compare `jar_capacity` and frame times at each step; stop adding work if the machine becomes unresponsive.
- Fill the optional region with a bounded test scene. Verify new optional shots/effects are omitted, existing actors survive, reserve slots remain available, and rejected-attempt warnings do not flood the console.
- Free temporary entities and verify later shots and deferred NPC spawns recover without restarting the level. Confirm deferred spawns preserve their count and intended script behavior.
- In a dedicated test scene, exhaust required allocations too. Verify the descriptive error returns to the menu, then load another level. Repeat for skeletal instance exhaustion.
- Exercise crowded effects and 512-channel audio. Check delayed/looped FX saturation, fades, mirrors/portals, transparent objects, channel replacement and output clipping.
- Repeat the reported det-pack cluster test, including remote detonation and shooting one pack to trigger its neighbors. Check damage, effects, scripted targets and cleanup across the next simulation tick.
- Repeat the reported NPC crowd test in first- and third-person, with shadows and portal/mirror views. Verify player/NPC mesh, skin and animation stay paired. At the bounded surface limit, expect omitted surfaces rather than swapped model data.
- Verify stock saves are rejected and the normal upstream game still uses its own binaries and user-data directory.

Do not mark the build gameplay-validated until these checks have actual results. Capacity is a bounded resource increase, not a guarantee that arbitrary content or 16,000 NPCs run smoothly.

## Implementation map

- `code/qcommon/jar_capacity.h`: compiled capacity profile and save version.
- `code/game/g_entity_pool.h`, `g_utils.cpp`: allocation policy, critical reserve, controlled required-allocation failure and `jar_capacity` command.
- `code/game/NPC_spawn.cpp`, weapon/event call sites: deferred or optional allocation handling.
- `code/game/g_explosives.cpp`, `g_radius_damage.cpp`: production explosion and splash-damage paths exercised by the chain-reaction regression.
- `code/qcommon/snapshot_codec.cpp`, `msg.cpp`, `net_chan.cpp`: enlarged local snapshot transport and queue bounds.
- `code/rd-vanilla/tr_sort.cpp`: 64-bit renderer keys, sorting and surface bound.
- `code/rd-vanilla/tr_ghoul2_pool.cpp`, `tr_scene.cpp`, `tr_ghoul2.cpp`: skeletal surface allocation, frame lifetime and guarded submission.
- `code/qcommon/jar_bolt.h`: 14-bit entity attachment encoding with range validation.
- `code/client/snd_mix.cpp`: wide audio accumulation.
- `tools/jar-launcher/main.cpp`: launcher with a static C++ runtime and a private engine path.
- `code/ui/ui_jar.cpp`, `code/qcommon/jar_graphics.h`: staged graphics menus, controls and presets.
- `code/game/g_npc_lod.h`, `code/game/NPC.cpp`: optional distant idle decision scheduling.
- `code/rd-rend2`, `shared/rd-rend2`: adapted SP Rend2 renderer and bounded allocation helpers.
- `tests/jar/`: 15 headless checks including the two real renderer DLLs and the Windows launcher.

## 0.2.1 diagnostic instrumentation

The engine-owned JSONL recorder is exercised independently for escaping, truncation, verbosity filtering, four concurrent writers, rate-limit accounting, eight-file retention, failed opens, a competing process, immediate exit without cleanup, and a handled native exception that continues through Windows exception dispatch. Both renderer DLL tests receive the real versioned logging import and exercise populated model vectors as well as slot exhaustion/reuse. REF_API is now 1020 and GAME_API is 1012; all runtime modules must be installed together. Save format is unchanged.

The diagnostic hooks do not fix or reproduce the 19:50 Rend2 double free. No gameplay launch or FPS measurement is part of this change. Matching Release PDBs, module hashes, a build ID and source snapshot are retained separately from the deployed runtime.


## 0.3.0 display and appearance validation

The menu harness exercises the production controller and builder: custom numeric
fields, atomic rejection of invalid input, automatic custom-mode selection,
desktop borderless selection, display confirmation keep/timeout, missing-pack
rejection, sound reload scheduling, and local catalog parsing (including invalid
IDs, duplicates and overlong labels). Display and sound overlay policies are also
exercised directly. Existing capacity, serialization, renderer DLL and diagnostics
checks remain in the suite.

`tests/jar/appearance_assets.py` verifies the actual generated PK3 before deployment:
ZIP integrity and namespace isolation, all generated font layout metrics against
retail, atlas dimensions/alpha, TGA loader compatibility, saber line continuity,
pack markers and shader image dependencies. This validation consumes the owner's
local retail assets without modifying them.

Manual follow-up after installation:

- Try custom windowed and exclusive modes and desktop borderless on the intended
  monitor. Check Keep, Escape and timeout; confirm Alt-Tab and mouse behavior.
- Inspect menus, pause, subtitles, HUD and console at 1080p/1440p/4K, including
  accented text. Compare Retail/Sans/Serif and 2x/4x art; check animation frames.
- Compare each blade/sound/effect style in the same scene, including saber on/off,
  hum, swing, impact, blaster fire and NPC sabers. Check missing-asset warnings and
  memory/frame-time behavior. Some game-side effect caches may need a map reload.
- Confirm custom local pack catalog entries appear after video restart.

No game launch, audio playback, monitor-mode switching or gameplay validation is
performed by the automated build. The previously investigated Rend2 double-free
is a separate issue; this feature update does not claim to repair it.


## Pickup randomizer validation (0.4.0)

The headless `jar_pickup_randomizer_tests` compiles the production policy with
substituted cvars, map fields, item definitions and logging. It checks a fixed v1
hash vector, seed/map changes, category independence, entity-index independence,
gameplay RNG isolation, conservative metadata/bounds filtering, invalid config,
sparse item-table termination, and full-save/autosave mode handling.
`jar_item_registration_tests` compiles the production registration functions and
reproduces full-save config restore followed by an unrelated registration and a
second save/load. It verifies replacement precache bits survive and truncated
configstrings cannot retain stale bits. Both review defects were reproduced with
failing regressions before their fixes. The menu regression exercises seed bounds,
Cancel, Revert, Apply, archive/save flags, and no video restart for pickup changes.
This does not exercise the full game save loader, collision trace, item pickup,
model loading or campaign progression.

Coordinate an owner-approved idle game window and isolated test profile before
launching. Use disposable saves, verified package hashes, stock assets and the
Classic renderer for the first pass. Keep the current installation available for
rollback; do not mix modules from different builds.

A read-only audit of 34 stock SP BSPs found 432 eligible pickups (400 supplies,
28 ammo, 4 weapons), 23 metadata-protected and 144 excluded-class pickups.
`t1_surprise` and `t3_bounty` cover all three eligible categories; `hoth2`,
`vjun2` and `vjun3` provide metadata-protected pickup cases. These are initial
test targets, not a campaign-completion claim.

When safe game-test readiness has been coordinated:

- With all toggles Off, compare fresh map pickups against the prior build.
- With seed 73 and each category independently enabled, load the same fresh map
  twice and compare `randomizer/pickup_decision` logs at `jar_logLevel 2`.
- Enable the other categories and verify previously enabled category decisions
  remain identical. Change the seed and verify eligible choices change.
- Check model, icon, quantity, ammo award and collision for each replacement.
  Test tight alcoves, sloping surfaces and pickups on movers.
- Verify keys, DEMP2, disruptor, explosive weapons/ammo, power cells, holocrons,
  dropped weapons and scripted/targeted pickups remain original and usable.
- Save before collecting a randomized pickup, change preferences, then load that
  full save. Verify its item and pickup state are preserved. Continue until a
  scripted NPC/weapon assignment registers another item, save again, reload, and
  restart video; replacement models/icons must remain visible and collectible.
  Check a level-entry autosave recreates the recorded seed and settings, including
  after a restart.
- Load an older JA:R save with randomizers Off and check compatibility. Complete
  representative scripted mission objectives before claiming progression safety.


## 0.14 carried inventory and initial Sandbox

Thirty Release/Debug checks include bounded transactional inventory/ownership
serialization, native equipment/control predicates, real native menu dispatch
and page geometry, and canonical Q binding ownership/restoration. The packaging
check verifies both original melee definitions and their installed GLM references.

Guarded candidate sessions passed in Classic and Rend2: distinct shortblade/pike/
Sith sword, repeated idle swaps, colored dual-to-single-to-dual, exact explosive
selection without ammo changes, owned prop selection/grab/rotation/freeze/gravity,
full-save restoration, undo/off cleanup, Q restoration and later user edits.
E-11/repeater drops granted 10/50 stock ammo, fired, relinquished ownership on
pickup, reused an entity slot with new IDs and expired through native item code.

Compatibility fixtures passed 0.13 full-save migration, real old entry autosave,
carried map transition, new entry-autosave reload without world ownership and
fresh-map registry reset. The installed 0.13 reader rejected the new version
cleanly. Missing explicit saber skin and unsupported MD3 attachment definitions
were rejected with prior weapon, colors, reach and ammo unchanged.

The first native passes exposed a canonical Q lookup and native WEAPON_IDLE
guard mismatch; both were corrected and covered by regression tests and native
reruns. Read-only independent review also closed exact explosive selection,
drop ammo, asset validation, saber string lifetime, protected-control, paging
and unrelated-entity gravity issues. No further material blocker was found.

Exact installed build IDs, renderer captures, source/package/receipt hashes and
post-test preservation checks are recorded in the 0.14 inventory/Sandbox evidence
archive under `build/validation/`. Tests use disposable profiles and clean exits;
normal player settings/saves and all existing base PK3s are preserved.

Limits: protected-state predicates are tested with native states/flags; staged
live saberlock/Force-grip combat and physical controller testing are not complete.
This does not qualify every campaign/mod combination or complete the overhaul.


## 0.15 Sandbox actors, physical tools and dynamic vehicle saves

Thirty Release and thirty Debug checks pass, including old/new workshop wire
fixtures, tool and parent identity validation, bounded menus and the Sandbox
ally-continuity exclusion. Native disposable-profile runs qualify NPC death
drops, parent/drop saves, navigation-helper cleanup, Swoop/tauntaun boarding,
occupied full saves, driving and safe deferred cleanup in Classic and Rend2.
Native capacity tests reach eight actors and 32 objects, reject the next spawn,
clean all owned objects and reuse slots with fresh ownership IDs.

Tool fixtures verify native ammunition is unchanged during manipulation,
switching while held does not fire the selected ordinary gun, release/repress
resumes normal firing, saved grabs become frozen, and off/load trigger latches
require a release. Old 0.14 full saves retain all 17 carried choices in the
fixture; old entry saves, tool-bearing transitions/new entry saves and fresh-map
reset also pass. The installed 0.14 reader rejects the V2 header before loading.

Testing found and fixed an occupied-tauntaun crash: native vehicle information
is allocated on demand and its saved numeric index was uninitialized after BSP
reload. V2 now writes bounded vehicle names and remaps indices before restoring
entities. It also found native spawner origin and tool-HUD/beam issues, fixed
before qualification. Earlier failing fixtures remain in the evidence archive
and are not counted as passes.

Exact package/source hashes, install receipt and installed-renderer validation
are recorded in `build/validation/JAR-0.15.0-sandbox-tools-evidence.zip`. Runtime
rollback, current player settings/saves and all 66 existing base PK3s are checked
against the pre-change baseline. No global input, process termination, persistent
helper, extra installation or third-party code/art import is used.

These checks qualify the bounded catalog and named fixtures. They do not claim
all campaign/mod combinations, physical controller operation or completion of
the remaining UI/projectile/camera/icon overhaul.


## 0.17 optional rigid props, catalog and native-flow integration

31 Release and 31 Debug checks cover the production Box3D bridge alongside the
existing randomizer, save, ownership, menu, melee and capacity suites. New solver
coverage includes world-mesh settling, constrained drag into a floor, rotated
freeze, actual kinematic contacts, offset rotating save/restore motion, entity
identity reuse, teardown and prop capacity. Native layout fixtures cover 30
qualified stock menus, unchanged action/condition pointers, custom-layout
rejection, pagination, empty-card rejection and native animation coordinates.

Classic and Rend2 qualify all eight added props and nine added NPCs. The overlay
spawns a second-page entry and toggles physics. A 128-prop native sample retains
128 exact saved/restored motion records; nine wake frames have at least 100 awake
bodies, with maximum summed solver step time 0.940 ms. This is a short fixture,
not a sustained frame-rate or full-campaign performance result. A separately
moving prop retains its nonzero velocity through a full save and continues falling.

Actual contact events demonstrate native NPCs and both vehicle kinds pushing
props. Occupied Swoop/tauntaun fixtures in both renderers board, save/load, drive
into a rigid crate, switch physics and Sandbox off, then exit for deferred
cleanup. These tests found and fixed a rider-filter bug that also excluded the
occupied vehicle. Native use-ray logging diagnosed an initial test approach that
was out of boarding reach; boarding mechanics were not changed for that failure.
Rotated-prop freeze/off overlap tests prove the hull becomes solid only after an
overlapping native actor clears it. Map change and disconnect release old bodies.

0.16.1 tool saves migrate, retain owned props/tools, enable physics and resave/
reload. The retained 0.16.1 reader rejects the V4 header before loading a world.
The previous version's ZIP, installed rollback, source and evidence remain intact.

Both renderers load all 30 qualified additional menu layouts without a signature
mismatch. Actual mission selection chooses Tatooine and continues to Force and
weapon selection. Native joystick hat/confirm and window-local pointer actions
select, unassign and reassign Force upgrades; screenshots show the restored
available choices and changed upgrade. These synthetic inputs do not establish
physical-controller compatibility. Menu captures cover settings, archives,
difficulty, mission tiers, debrief, datapad and dialogs; late native framing and
Force button-coordinate issues were fixed and recaptured. Native gameplay actions,
conditions, models, save feeders and mission animation scripts remain connected.

Fist regressions in both renderers pair each accepted punch animation with its
primary fire event, leave no queued punch after release, accept a single tap,
retain Legacy timing and resume after movement, missed grabs, kicks and save/load.
Camera/saber fixtures check gun return and protected throw extension; all 274
observed extension steps match their configured native/quick timing. Scripted
camera command coverage is bounded and is not an entire cutscene playthrough.
Each renderer executes 110 randomized shots covering all 78 qualified variants;
travelling missiles have a model or trail, while beam variants remain instant.
The optional MBII pack repeats the bomb/ion visual repairs and full-save reload.
Damage, ammo, cadence and seeded choices are unchanged by this physics/UI work.

The launcher and engine embed all nine original icon sizes, including a 256px
resource verified pixel-for-pixel against the generated source. Box3D's 111
pinned source/license files remain unmodified. The package ships its MIT notice
and hash provenance, and the corresponding source archive includes the core.
Exact build/install/source/preservation hashes, native logs and captures are
recorded under `build/validation/JAR-0.17.0-physics-ui-evidence.zip`.

Remaining limits: no full campaign playthrough, no physical controller hardware,
no universal mod/map qualification, no reverse forces on native controllers,
and no hinges/welding. Procedural-terrain/sub-BSP rejection is a conservative
source guard; this run does not claim a native test of every fallback map type.

### 0.23.0 post-install evidence

Build `20261010-100733-f5d64d69` passed all **42 suites** and the retail/package
asset gates. Installed from `build/JediAcademyRemixed-0.23.0-win64.zip`, SHA-256
`25889C8C2A03BDC16AC42F20812523CF49F5C0BE09BAFC8BBCA7977C11E6BEA9`.
Receipt: `build/deploy/20261010-100816-fcf693ef/receipt.json`: **37 verified files,
9 changed**, with backups. Machine-readable result:
`build/context-20261010/release-result.json`. No game was launched. The active
completion goal remains open; see the authoritative overhaul checklist.
This receipt was added to the workspace after packaging; the installed validation
record includes the test scope and evidence boundary, not this post-install hash.

### 0.24.0 post-install evidence

Build `20261010-102150-12dd76e2` passed **43 suites** and all retail/package asset
checks. Installed ZIP: `build/JediAcademyRemixed-0.24.0-win64.zip`, SHA-256
`86F5824DB9BC69DF05C767FDD42F1E83EDD8E6AEE2A954D8C24869906F0F5666`.
Receipt: `build/deploy/20261010-102232-2e9f8309/receipt.json`: **37 verified files,
9 changed**, with backups. Structured result:
`build/duplicate-20261010/release-result.json`. No game was launched. The overall
completion goal remains active. This workspace receipt was appended after packaging;
the packaged validation guide records the scope and evidence boundary.

### 0.25.0 post-install evidence

Normal `scripts/build-jar.ps1` completed: **43/43 suites passed**, plus retail
and packaged asset checks. Installed ZIP: `build/JediAcademyRemixed-0.25.0-win64.zip`;
SHA-256 `C76E37163F9C4B9C1711634A03EF1F69C3346CDA90C5449709A2A9475A4BF6D2`. Build ID:
`20261010-103754-a8eb4da7`. Receipt/backups:
`build/deploy/20261010-103839-93e97596/receipt.json`. **37 files verified; nine changed**.
A post-install reread confirmed the build manifest and every receipt payload hash.
Catalog: 75 portable presets, 78 former blocked routes implemented, 104 still
blocked. No game launched; overall goal remains active. This post-install record
was appended to workspace documentation after packaging; installed documents
contain the pre-install qualification notes.

### 0.26.0 post-install evidence

Normal `scripts/build-jar.ps1` completed: **44/44 suites passed**, plus retail
and packaged asset checks. Installed ZIP: `build/JediAcademyRemixed-0.26.0-win64.zip`;
SHA-256 `F2E9420B08C9F9B472286D4A89F72FE593CAEB2894921908741ECA0FEE4C5219`. Build ID:
`20261010-105526-9f6ad017`. Receipt/backups:
`build/deploy/20261010-105614-3ad1ad8d/receipt.json`. **37 files verified; nine changed**.
A post-install reread confirmed the build manifest and every receipt payload hash.
V9/wire 6, APIs 1020; catalog unchanged (104 blocked). No game launched; overall
goal remains active. This evidence was appended to workspace documentation after
packaging; installed documents contain the pre-install qualification notes.

### 0.27.0 post-install evidence

Normal `scripts/build-jar.ps1` completed: **46/46 suites passed**, plus retail
and packaged asset checks. Final build **20261010-111455-a406a702**; release
`build/JediAcademyRemixed-0.27.0-win64.zip`; SHA-256
`8C2559D27751D7535D0D4A5042D55A73C2D6A0E3B4BF8F374CCCABB48116694A`.
Receipt/backups: `build/deploy/20261010-111538-c47e381d/receipt.json`.
**37 installed files verified, eight changed** from the first qualification build.
An independent reread matched the ZIP, all 37 installed files, the installed build
identity and 621 runtime/test/script source files against the symbol snapshot.

The earlier qualification build was retained as
`build/model-bounds-20261010/first-release-20261010-111222-ab78d093.zip`, with its
original receipt `build/deploy/20261010-111319-61c9da3e/receipt.json` and symbols.
The final build adds failed-query caching and full-body actor-search bounds.
Final log/result: `build/model-bounds-20261010/release-install.log` and
`build/model-bounds-20261010/release-result.json`. Exact source/symbol archive:
`build/symbols/20261010-111455-a406a702.zip`.

V10/wire 6, APIs 1021. Catalog: 1,197 cards, 105 unavailable including the newly
identified empty no-weapon placeholder; 78 earlier formerly blocked routes kept.
No game launched. Goal remains active: selectable skeletal poses, native
Weld/Hinge and scripted mover thrusters, rope wrapping, catalog gaps, walking/crash
confirmation and native visual/save/campaign/performance acceptance remain open.
This workspace evidence was appended after packaging; installed documentation
contains the pre-install qualification record, with the receipt as deployment proof.

## 0.28.0 installation evidence (2026-10-10)

Normal build/test/package/install completed: **47/47 suites passed**,
with independent retail/catalog and packaged asset checks. Release identity:

- Build: `20261010-113203-b2555ea0`.
- ZIP: `build/JediAcademyRemixed-0.28.0-win64.zip`.
- ZIP SHA-256: `F70296BF6BCE249302070AB5C10730D8B0B2B4EFAC45A30C46C253BAD9DADE46`.
- Receipt/backups: `build/deploy/20261010-113247-2984a08b/receipt.json`.
- Installed to the authorized Steam GameData path: **37 files verified,
  9 changed files backed up and replaced**.
- Independent reread matched every installed file and ZIP hash, installed build
  identity and **911 runtime/build/test/source snapshot hashes**.
- Evidence: `build/native-objects-20261010/release-install.log`,
  `release-result.json`, `verify-release.ps1`; symbols and exact source snapshot:
  `build/symbols/20261010-113203-b2555ea0.zip`.

V11 `0x4a41520c`, workshop wire 6, game/renderer APIs 1021. No game launched.
The goal remains active. Native unlock/damage/Use, full save/load, tools and
visual acceptance remain open, along with the broader checklist. Next controller
work includes the listed but inactive `trigger_space` touch/suffocation path.
This evidence block was appended to workspace documentation after packaging;
the exact installed documentation remains captured in the verified ZIP.

## 0.29.0 installation evidence (2026-10-10)

Normal build/test/package/install completed: **48/48 suites passed**,
with independent retail/catalog and packaged asset checks. Release identity:

- Build: `20261010-114617-9aa6de76`.
- ZIP: `build/JediAcademyRemixed-0.29.0-win64.zip`.
- ZIP SHA-256: `FE5F67B0CEE2F1263F82B09E9B13773F9CB04622A12DC3CC4EFBC44F7FF50C97`.
- Receipt/backups: `build/deploy/20261010-114707-3a997ec2/receipt.json`.
- Installed to the authorized Steam GameData path: **37 files verified,
  11 changed files backed up and replaced**.
- Independent reread matched every installed file and ZIP hash, installed build
  identity and **916 runtime/build/test/source snapshot hashes**.
- Evidence: `build/space-20261010/release-install.log`, `release-result.json`,
  `verify-release.ps1`, `retail-volume-audit.json`; symbols and exact sources:
  `build/symbols/20261010-114617-9aa6de76.zip`.

V12 `0x4a41520d`, workshop wire 6, game/renderer APIs 1021. No game launched.
The goal remains active. Native space effects, save/load, input/visual/campaign
and load acceptance remain open. Remaining controller work includes ship-boundary
turnaround and the unfinished bidirectional trigger; native attachments and
volume extent/orientation editing also remain in the checklist.
This evidence block was appended to workspace documentation after packaging;
the exact installed documentation remains captured in the verified ZIP.
