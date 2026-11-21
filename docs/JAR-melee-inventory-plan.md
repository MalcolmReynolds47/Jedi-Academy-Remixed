# JA:R melee and carried-inventory design checkpoint

Historical 0.13 source audit and staged plan, 2026-10-04. The implemented 0.14
foundation is documented in [Inventory and Sandbox](JAR-inventory-sandbox.md).
The following records the pre-implementation decisions. No inventory
format, weapon enum, possession mask or simultaneous-wield limit is changed by
the fists/extension milestone.

## Current source and behavior

- `weapons.h` defines native weapon families; `MAX_PLAYER_WEAPONS` ends before
  `WP_STUN_BATON`. `g_cmds.cpp` and the normal selector use `1 << weapon` masks.
  More named weapons cannot safely be added by extending this signed bitmask.
- `q_shared.h` fixes `MAX_SABERS` at two and explicitly warns about save-layout
  coupling. `g_savegame.cpp` has explicit string fields for both saber names.
  These are equipped weapon slots, not a general carried inventory.
- Mounted `assets1.pk3/ext_data/sabers/sith_sword.sab` defines Glory of the Sith,
  type `SABER_SITH_SWORD`, native sword model and length 44. Its solid model has
  no energy-blade ignition visual. `wp_saberLoad.cpp` precaches dedicated sword
  sounds; `wp_saber.cpp` selects sword swing/hit/fall behavior, and `cg_players.cpp`
  uses a distinct faint sword trail. It must not become a reskinned energy saber
  with altered script or knockback behavior by accident.
- `iknowkungfu` sets `g_debugMelee`, equips `WP_MELEE` and grants Force levels.
  It is not merely an inventory command. The bounded punch repair retains that
  command's existing grants and does not automatically execute it.
- Native camera selection forces third-person for saber/fists (`cg_view.cpp`).
  Returning to a gun should restore the user's normal view preference. Grab,
  throw, vehicle and scripted views remain authoritative.

## Proposed carried inventory, before implementation

1. Add a bounded inventory independent of weapon-family masks: initially at most
   64 records with a stable definition ID, family adapter, acquisition provenance
   and persistent record ID. Do not serialize pointers or mounted catalog indices.
   Definition IDs must be validated, canonical and resolvable from installed data.
2. Keep native ammo pools, attack code and at most two equipped saber slots. A
   carried sword, saber or fist option selects a compatible native family; carrying
   many choices does not imply additional hands, blades or simultaneous weapons.
3. Build one qualified definition registry with model/skin/sound/animation/resource
   checks and explicit exclusion reasons. Missing custom assets must retain the
   carried record and fall back to a known compatible equipped item, not discard
   inventory, mutate story actors or mount unapproved content automatically.
4. Equip through existing weapon/saber adapters. Validate the whole target loadout
   before changing it; defer selection during grabs, throws, scripted possession,
   vehicle control, death and weapon transitions. Failed equip keeps the prior
   usable loadout. Reconcile maximum reach, saber style, second slot and precache.
5. Introduce a versioned, bounded optional save chunk only after defining its
   validation/migration contract. Old saves synthesize records from existing
   possession bits and equipped saber names; preserve every old native field.
   Inventory-bearing saves need an explicit compatibility marker so older builds
   reject unsupported data safely. Validate counts, duplicate IDs, corrupt fields,
   missing definitions, mid-transition saves and load-then-resave before release.
6. Provide selection in a native menu with keyboard/controller focus, equipped
   markers, clear family/ammo information and unavailable-entry reasons. Preserve
   existing next/previous/direct weapon bindings. Do not take Q: Sandbox owns its
   separately requested optional Q binding and must restore conflicts safely.
7. Keep campaign acquisition and mission gates explicit. Do not grant every sword
   or gun by opening the inventory. Randomizer-selected possession and Sandbox
   objects need separate ownership/provenance, so cleanup cannot erase authored
   items or permanent carried records.

Acceptance before widening: native equip/unequip and old-save migration fixtures,
model/resource failures without partial mutation, two renderers, both attack
inputs, weapon/camera transitions, story weapon restrictions, inventory save
corruption tests and bounded catalog/resource performance. Multiple copies with
independent ammo, additional simultaneously held weapons and new physics are not
implied by this initial design.

## Reference and asset provenance

The existing local OpenJK source is GPLv2. Changes in this milestone are original
modifications to that source; no JA+ code, binaries or animations were imported.
The archived [JA+ author readme](https://mrwonko.de/jk3files/Jedi%20Academy/Mods/Multiplayer/120346/)
and [client release notes](https://jkhub.org/files/file/952-ja-client-side/) describe
additional melee combinations and camera behavior. They are behavioral reference
material; this review did not establish a licensed JA+ source distribution or
permission to reuse its separate animation/model assets. JA++ is a different
project and must not be presented as JA+ source. Existing SP kicks/grabs are kept
while the original primary-punch cadence is repaired.

Any future imported melee asset needs its author, exact source/version, hash,
license/permission and required notices recorded before inclusion. An installed
PK3 or a public download alone is insufficient evidence of reuse permission.

## Current implementation status (0.17)

This document records the original 0.13 plan. The subsequent releases implement
64 carried records, three distinct melee choices, the native weapon bar and real
tool slots, held-Q Sandbox, ownership/undo, expanded catalogs and optional rigid
props. Fist cadence and quick saber extension remain independently selectable.
Native mission/settings/weapon/Force flows and the original application icon are
now implemented. See [current controls](JAR-inventory-sandbox.md) and
[physics/provenance](JAR-physics.md).

The original native assets remain separate from any future third-party melee
imports. Physical-controller tests, exhaustive campaign/camera coverage and
additional licensed weapon assets remain further work; no extra simultaneous
hands or independent ammunition per duplicate carried record is promised.
