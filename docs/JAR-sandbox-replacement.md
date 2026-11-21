# JA:R replacement Sandbox scope — 2026-10-05

> **Current scope (0.51.0, 2026-10-10):** The authoritative status is in
> [the overhaul checklist](JAR-overhaul-checkpoint.md); the goal is active.
> All 32 annotated SP items remain accepted. There are 1,197 cards, with 116 of
> 182 former unavailable entries routed and 67 unavailable total. Holocron levels
> are editable in C / Properties. SP saves V31 / wire 7 / APIs 1022; MP catalog 608.
> Persistent visible ropes and a partial MP port are implemented; full parity and
> remaining catalog/controller/pose/Rope work remain open. Older notes are historical.

> **0.22 update (2026-10-10):** 1,197 searchable entries, 74 newly spawnable
> from the previously blocked set, 108 remaining blocked. [Catalog controls](JAR-spawn-catalog.md)
> cover portable map controllers, automatic helpers, Activate and Link. The fixed
> limits are 4,096 owned objects and 128 actors. Default `g_noFootSlide 1` now
> adapts NPCs only; the player's walking fix still needs visual acceptance.
> Beams are owner accepted and unchanged. New saves use V8 `0x4a415209`, wire 5.
> Older release sections below are historical where they conflict with this update.

## 0.18 update: saved Toolgun constraints

Weld, Hinge and Unlink are implemented in the held-Q overlay. Box3D remains Off
by default; enable it explicitly. Use two frozen spawned props, then Gravity on
the member(s) that should move. See [exact controls, save migration and limits](JAR-physics.md).
New full saves use header `0x4a415206` / workshop wire revision 4. The 0.17
checkpoint and older saves remain retained. Statements below describing the
absence of joints are historical and superseded by this release. Full catalog
coverage and unrestricted map manipulation remain unfinished.


This owner direction supersedes the earlier protected-map-object, small catalog,
carried-inventory-as-primary, and Simple/preset-default requirements. Other
overhaul requirements remain active. Work stays on the development workstation.

## Current implementation: 0.17

The requested replacement is implemented through optional Box3D integration.
There are 53 qualified paginated cards; real tool weapons; held-Q controls;
map-object transforms/removal with explicit limits; 128 default props; native
world/actor/vehicle/mover collision coupling; save/load and lifecycle support;
30 additional native UI layouts; and an original application icon.
[Physics details](JAR-physics.md) describe the pinned MIT library, one-way native
coupling, resource bounds and fallback. The dated sections below retain the
actual earlier checkpoints and are not current feature omissions.

## Checkpoint and reuse

0.15.0 was already installed before the direction-change message arrived. Its
build is `20261005-095153-6ffd1e74`, ZIP SHA-256
`3FE362B030A2ADEDB161134883647B3BD796DA341E06A7A23AEC022A82DA5191`.
It is an implementation checkpoint, not the completed requested Sandbox.
Its rollback and verified evidence remain intact. Unfinished native-flow work
(menu joystick input, a window-local QA command and unused layout fixtures) is
snapshotted in the workspace; no destructive reset or installation followed the
new direction. The joystick adapter passed 30 Release checks and a native
confirm/back navigation test, but no physical-controller claim is made.

Reusable work: native actor spawning/precache, tool input release guards,
vehicle name/index save restoration, ownership identities, cleanup/death-drop
hooks, settings transactions, native renderer/test infrastructure. Replace the
old pause-menu Sandbox and carried-selector integration as primary interaction.

## Required replacement

- Hold Q for a cursor overlay. Release closes it. Do not pause simulation;
  preserve movement input while the overlay is open. Releasing/off/loading must
  not leave mouse capture, attacks or bindings stuck.
- Icon cards for every catalog entry, tabs Props / NPCs / Weapons / Entities /
  Vehicles, and a tool list on the right. Spawn at the crosshair aim location
  with category-appropriate collision/placement validation.
- Substantially higher practical prop limit, configurable and profiled. Do not
  promise unlimited objects. Retain explicit allocation/performance boundaries.
- Physgun and Toolgun are real selectable entries in the bottom weapon bar.
  Every new weapon has its own name and icon. Toolgun is one weapon slot; choosing
  a tool changes its active properties. Replace the rejected primary inventory
  interaction rather than retaining it under a different menu label.
- Draw the Physgun beam from the weapon to the held object.
- Permit explicit Sandbox tools to interact with existing map objects/entities;
  remove blanket authored-object exclusion. Explain mission-break risk and
  provide undo where feasible. Keep unsupported technical cases explicit.
- Add optional, toggleable Box3D physics for suitable crates/boxes/barrels,
  including contact with NPCs/vehicles. Doors, elevators, turrets and vehicles
  keep native simulation, with kinematic collision coupling where appropriate.
  Verify the actual library, license, build requirements and integration limits.
- Use the Facepunch repository as a reference; copy only material with verified
  applicable reuse permission. Do not call its Lua repository the full engine.
- Explain actual loading stages with readable text.
- Advanced settings is the default again. Remove the preset-driven Simple /
  Chaos / Vanilla / Intended entry flow without resetting existing gameplay,
  graphics, seed or randomizer configuration.

## Implementation order and acceptance

1. Verify source/license and record architecture boundaries. Restore Advanced
   entry and remove automatic preset initialization without rewriting profiles.
2. Implement held-Q input routing and the unpaused icon overlay. Verify visible
   cursor, continued movement/simulation, clicks and release/off transitions.
3. Integrate tool weapons into native selection/bar, named icons, aim placement,
   higher limits and explicit existing-entity interaction/undo semantics.
4. Integrate a pinned Box3D core, build locally without installing dependencies,
   couple eligible props to native world/movers/actors, qualify on/off and saves.
5. Validate both renderers, menu/controller paths, save migration, resource bounds
   and existing campaign behavior, then package/install coherent verified
   milestones. Finish remaining native UI/projectile/camera/fists/icon work.

Keep all existing source changes and retained releases. No UAC, service/ACL or
persistent access changes. Guard every native launch/deploy against active games;
use disposable test profiles and scripted exits. Do not claim a full GMod clone.

## Replacement implementation checkpoint (not deployed)

Advanced opens directly; preset-driven entry and automatic Intended defaults
are removed. Existing settings, pending transactions and live randomizer ownership
remain intact. Held Q now opens a five-tab icon overlay and its tool list; movement
bindings pass through, clicks stay in the overlay, and release/off dismiss it.
Native Classic and Rend2 fixtures both demonstrated continued game time, about
288 units of movement, card spawning, tab changes and clean closure.

Physgun and Toolgun append native IDs 29/30, preserving all earlier IDs. Their
names/icons appear in the normal bottom selection interaction, alongside distinct
carried saber variants. Toolgun properties remain in one slot. Native mission
statistics keep their original 29-entry layout. Save header 0x4a415204 and workshop
wire revision 3 migrate old aliased tools; previous readers reject the new header.
The first successful native tool fixture selected both real slots, held a prop,
rendered the muzzle-to-object beam, preserved ammo and restored Toolgun/mode on
full save/load. Initial failed fixtures are retained and are not pass evidence.

Spawn placement traces the rendered crosshair and offsets the full collision hull
onto the hit surface. Actors require level ground. The prop setting currently
defaults to 128 (16–384 configurable), within a separate 512-object registry;
performance qualification is pending. Existing objects survive lowering the cap.
The current overlay covers 26 existing catalog entries; broader catalog coverage,
map-entity interaction/undo and optional Box3D integration remain unfinished.

Box3D main is pinned at e77352cd606dc1a34209094076199549a52ea0a1.
Its MIT core compiled locally as C17 with the matching /MD C runtime, without
samples, installations or security changes. This does not establish game-world
coupling. The v0.1.0 release is alpha. Facepunch's public repository contains Lua
and supporting files, not its binary engine; no general reuse license was found,
so its code/assets have not been copied. New pictograms are original local code.

## 0.16 milestone qualification

The replacement milestone adds 36 icon cards, including 12 audited stock prop
models. Every supported catalog entry has a card; the full retail NPC/model/item
universe is not yet exposed. Props use conservative measured model bounds and
asset fingerprints. The configurable 128-prop default was reached in stock
t2_trip with all 128 retained when the limit was lowered to 16; further spawning
was refused and cleanup removed all spawned props. A bounded Classic sample
remained at the configured frame cap. This is not a full campaign benchmark or
a Box3D-in-game performance claim. The 384 upper setting remains an opt-in limit.

Eligible existing models, pickups and free NPCs can be selected and removed.
Stationary misc_model/misc_model_static/misc_model_breakable entities can be
held, rotated and frozen. Doors, elevators, brush geometry, vehicles and other
native controllers keep their update paths; movement/gravity of pickups/NPCs
is not replaced by a prop controller. Explicit map-entity removal bypasses
automatic actor recovery for that entity only and can break mission progression.
It requires a prior save to restore. Transform/spawn undo stores 64 session-local
actions, checks entity lifetime and destination occupancy, and is cleared on
load/map change. Cleanup affects spawned objects only. Map transforms themselves
use native saved entity state. Box3D integration remains the next milestone.

The pinned Box3D core harness ran 600 fixed steps at 60 Hz, four substeps, twice
each with 128 and 384 bodies. Repeated final-state hashes matched on this machine
and build. Observed step p95 was about 0.07 ms / 0.21 ms respectively; sleeping
steps dominate the median. A kinematic proxy pushed a dynamic box while retaining
its own prescribed motion. This demonstrates one-way coupling only. It does not
test BSP collision, native NPC/vehicle integration, physics saves or reverse forces.
No Box3D runtime dependency is shipped in 0.16.

Sources checked: https://github.com/erincatto/box3d/tree/e77352cd606dc1a34209094076199549a52ea0a1
and its LICENSE (MIT, copyright 2026 Erin Catto); release v0.1.0 is alpha.
Facepunch reference: https://github.com/Facepunch/garrysmod . No code/assets reused.

0.16.1 follows the retained 0.16.0 qualification build. Larger hulls that overlap
a shallow terrain dip get a bounded 64-unit outward clearance sweep back onto
the aimed surface. Blocked clearance, player obstruction and steep actor support
remain rejected. The complete hull is validated again at the resulting position.

The additional menu-cursor request uses original locally generated 256px RGBA
art: a dark outlined ice/cyan arrow with a small amber inset. Menu drawing keeps
square pixels, scales from 28 to 64 screen pixels, and offsets the artwork so
its (12,12) tip is the exact native hover/click coordinate. It applies to native
menus and held-Q; weapon crosshairs are untouched. No cloud or third-party art.

## Licensed gameplay reference received during qualification

Facepunch/sandbox is a different repository from Facepunch/garrysmod. Its HEAD
was pinned locally at e280e2a667bd0d95cbd6606db687d66f05199507; LICENSE explicitly
grants MIT terms with copyright 2026 Facepunch. THIRD_PARTY_NOTICES excludes
Envato audio from reuse. LICENSE, notices and Code/Weapons/PhysGun/Physgun.cs were
read locally with SHA-256 provenance in the evidence directory. No reference code
or assets are included in this release; current JA:R tools/cursor remain original.

This is the preferred licensed behavior reference for the next physics milestone.
Its scene traces, Rigidbody/GameObject identities, synchronized grab offsets and
weapon callbacks depend on the s&box C# engine. A useful adaptation must map them
to OpenJK collision traces, generation-checked native entities, a Box3D body
registry and the existing native input/muzzle/save lifecycle. Networking callbacks
are not copied into this SP implementation. Retain the MIT notice for any later
substantial code adaptation and exclude third-party audio/assets.

The supplied SandboxPlus constraints/duplication and nanos-world Lua references
remain future inspection candidates; neither was imported. GMod 9 native source
and general permission to copy its assets remain unverified. No s&box binaries,
accounts, SDK installation or native Source 2 code are required for this research.
