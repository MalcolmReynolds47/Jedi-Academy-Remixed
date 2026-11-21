# JA:R retail spawn catalog and portable map entities

## Latest installed release (1.94.0, 2026-11-12)

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

Stable card 93973657 uses models/map_objects/rocky_ruins/floor_grate.md3 in
both modes with the exact stock model hull. Existing saved single-player
platforms retain their original setup. Raised lift copies are placed relative
to the bottom of the route.

## 1.86.0 Sandbox Imperial button (2026-11-11)

Stable card 96653372 uses models/map_objects/imperial/switch.md3 in SP and MP.
Its solid hull encloses all five frames. It spawns with a linked feedback
marker and can drive any supported linked controller through its Pressed or
Released output.

## 1.85.0 Sandbox sliding door parity (2026-11-11)

The single-player func_door card (100052696) uses
models/map_objects/factory/f_door_b.md3, matching the multiplayer card. Doors
from old saves and the compact door supplied with a maglock retain their
native setup.

## 1.84.0 Multiplayer sliding door (2026-11-11)

`func_door` spawns the stock Factory door panel in multiplayer.
Activate opens or closes it. Context sets motion and timing, and Link
can fire other controllers when the panel reaches either endpoint.

## 1.83.0 Portable wall (2026-11-11)

`func_wall` uses the stock Rocky Ruins wall panel in both modes.
Activate toggles the solid panel. An off wall keeps a small gizmo for
selection, relocation, and reactivation.

## 1.82.0 Multiplayer distance trigger (2026-11-11)

`trigger_entdist` is available from Entities in both modes. Multiplayer
adds matching Inside/Outside outputs, reference/NPC inputs and a
Context editor. Player mode checks living players on the server.

## 1.81.0 Entity-distance trigger (2026-11-11)

The single-player `trigger_entdist` comes with separate Inside and Outside
feedback. Context > Distance configures the check and selects a role for
the Link tool. The marker uses its own position until a reference is
linked. NPC and Either require an NPC subject link.

## 1.80.0 Visibility trigger (2026-11-11)

`trigger_visible` is usable from Entities in both modes. It fires its
Primary link once when seen, and Activate rearms the same gizmo. Context
exposes range, viewing half-angle, wall checks and Force Sight.

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

Hold Q and use Search within a tab. Names and asset paths are searchable without
case sensitivity. Click Search to type; movement input is suspended while editing.
Releasing Q during text entry keeps the menu open. Finish typing, then press/release
Q or press Esc to close. Clear Search with X. Full names and remaining limitations
appear in the fixed top-left description. A `!` marks an unavailable entry.

| Tab | Entries |
| --- | ---: |
| Props / model copies | 723 |
| NPC definitions | 178 |
| Weapons | 31 |
| Entities / pickups / spawner presets | 242 |
| Vehicles | 23 |
| Total | 1,197 |

0.28 gives **80 of the previously blocked 182 entries** a menu spawn route:
77 portable setup definitions and three NPC spawner aliases. **102 of that set
remain disabled**, plus one empty model found in 0.27: **103 unavailable cards**.
Of the 211 registered entity-class cards, 160 now have a spawn route. Available
entries still need clear placement space and enough native resources at runtime.
Spawn-route coverage is not gameplay acceptance. The previously inert listed
`trigger_space` controller is implemented in 0.29, with native acceptance open.

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

## 0.27 decorative model qualification

The 126 visible Ghoul2 model copies now use runtime default-pose vertex bounds
for placement and collision, and can use Box3D when inside its size limits. They
remain decorative model copies; the separate NPC/vehicle entries retain their
native controllers. Hidden and bolt-only surfaces do not enlarge their boxes.
The empty no-weapon helper `models/weapons2/noweap/noweap.glm` is now explicitly
unavailable. This corrects an invisible spawn: there are 105 unavailable entries
in total (the previous 104 plus this placeholder), with all 78 earlier routes kept.

## 0.25.0 portable racks, security camera and distance trigger (2026-10-10)

Four more entity cards can spawn directly from Q: **misc_model_gun_rack** creates
an E-11, repeater and rocket launcher; **misc_model_ammo_rack** creates three ammo
packs, an E-11 and health; **misc_camera** supplies its native viewing controller
and base; **trigger_entdist** tests whether the player is within 192 units when
activated. The catalog still has 1,197 cards: **78 of the former 182 blocked routes
are implemented; 104 remain blocked**, with 75 portable map-entity presets.

Use/Activate the camera to enter its view; movement exits. Link can aim it at
another owned entity. Its base follows explicit tool movement with the native
world-up mounting offset. Destruction exits the view and creates owned sparks;
Undo/cleanup removes the base and effects too. Original absent camera switch
sounds now use existing game button sounds. No new art is required.

Rack pickups retain native pickup/Force-pull behavior and difficulty quantities.
They become separately movable pickups, not physical joints to the rack. All
children are owned for cleanup and full saves; portable ammo shelves fill inside
the spawn transaction so delayed items cannot escape tracking. Insufficient space
or native allocation rolls the setup back. Duplicate preserves current remaining
children and counts: consumed pickups are not replenished on paste. As with other
portable setups, initial default helper placement can reject a paste even where
the final edited layout would fit.

The distance trigger has a visible marker and message destination. Activate tests
the current player distance and deactivates it after one check. Link replaces its
near destination; a linked **target_activate** rearms it. It is a use-triggered
test, not a continuously polling proximity volume.

Production callback and ownership/paste regression tests cover these routes;
retail checks qualify the required models, pickups, effects and sounds. Full
native gameplay/save/load/visual acceptance is still open. V8/wire 5/APIs 1020
remain unchanged. No game was launched for this update.

## Using map entities

- **Activate:** Toolgun left-click fires or toggles an entity's native use action;
  right-click selects/inspects it.
- **Link:** left-click a source, then a destination to connect their native target
  fields. Right-click cancels. Both must be owned Sandbox objects. Links that form
  activation cycles are refused. `target_give` needs a pickup destination.
- Doors, lifts, buttons, rotating/bobbing/pendulum movers, walls, usable objects,
  breakables and glass use a stock crate model with its box hull and the native
  controller. These are portable functional versions, not imported map geometry;
  glass uses glass debris with the crate appearance.
- Triggers and control targets receive visible stock markers. Trigger volumes
  are outlined while a tool is equipped. Teleporters/push triggers/effect beams
  create their own destination; move that marker with Physgun. Relays, counters,
  timers and similar sources have an owned message target for initial feedback.
- Trains create a closed two-corner route. Move those corners to reshape it;
  Link does not replace the route. Removing a setup component removes its whole
  setup, avoiding orphaned destinations or train pointers.
- Turrets, ion cannons, sentries, surface-mounted trip mines, security panels,
  cargo crates and weapon shooters use stock controllers and assets. A security
  panel creates a collectible key. Trip mines require a valid nearby surface.
- Speakers use the stock Bryar sound, effect runners/trails use stock sparks, and
  beam controllers use native effects. Rocket, grenade and plasma shooter aliases
  use the working native weapon-shooter controller. Activate fires the shooter.
- Compile-only model entities become removable stock models. The unavailable
  wall shield-converter model is represented by the functional floor converter.
  Player-start/null/path markers can serve as destinations. Lights use a local
  dynamic light instead of altering the map's baked lighting.

Native target actions retain their meaning: kill, gravity/friction changes,
autosave and secret/score targets affect play when activated. Some destinations
only have an effect after Link connects them to an appropriate source.

## Coverage and remaining limitations

The independent retail check covers 722 distinct MD3/GLM files, 201 NPC names
(including vehicle NPCs), 16 vehicle definitions, 59 pickup classnames and 211
registered entity classes. Original 53 card IDs remain stable. Categories overlap:
a character can have an NPC, decorative model and spawner-preset entry.

The remaining 104 blocked entries include missing retail models/definitions,
invisible tag/helper meshes, unfinished native triggers, controllers that require
map rendering/navigation/scripts, and assemblies without a portable adapter.
Laser-arm assemblies and a drivable AT-ST adapter remain work.
The JSON report gives each exact reason; these are not silently replaced with props.

There is room for 128 owned actors within the fixed 4,096-object registry and
native resource limits. Helpers consume registry slots. These are capacity limits,
not active-physics performance promises. Large ships require a clear floor area.

Model props begin frozen. Suitable MD3 bounds can use Box3D; oversized, offset or
skeletal decorative copies use native box collision/movement. GLM copies use a
generic hull and default skeleton pose; use the corresponding NPC/vehicle for its
controller. Weld/Hinge require suitable Box3D props. Rope/Thruster support ordinary native
entities as described in JAR-physics.md. Duplicate copies properties,
connected owned assemblies, helpers and physical links; script/AI execution state
is not cloned. Depleted racks retain only their remaining items.

Icons reuse installed game art; no new renders or pictograms are generated. Only
twelve cards per tab are populated. Indexed name/path-derived IDs avoid precaching
every model when opening the menu.

## Build and saves

`scripts/build-spawn-catalog.py` generates the shared header and JSON report from
`base/assets*.pk3`. The normal release workflow verifies both against installed
retail assets and independently checks coverage, portable models and effect/sound
resources. New full saves use V9 `0x4a41520a`, with workshop wire 6.
Earlier saves remain readable; older releases reject V9. Keep older saves for rollback.

No game was launched for this update. Automated setup tests verify spawn-variable
isolation, dispatch, helper placement/ownership strings and failed-setup rollback.
Rack, camera and distance callbacks now run as production code in the fixture;
other callbacks remain mocked. The fixtures do not prove full native controller behavior,
visual placement, save/load gameplay or long-session stability for every new class.

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
