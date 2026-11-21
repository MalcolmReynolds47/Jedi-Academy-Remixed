# Sandbox and weapon bar (0.20.0)

## 0.24.0 property and assembly duplication (2026-10-10)

**Duplicate** now captures connected owned assemblies, including Weld/Hinge/Rope
links, Thruster frames/force/on-off state, owned target networks and portable
controllers' helper entities. It retains relative positions, angles, frozen states,
positive health for damageable native entities and pickup counts. Select Duplicate
in Q: right click copies; left click pastes at the crosshair. Context **Copy/Paste**
uses the same clipboard. Q displays the copied object/link count.

Paste checks space for the whole assembly and remaps every link to the new ownership
IDs. Native helpers are reused and moved to their copied positions; internal target
names refer to the new copies. A failed native spawn/property/placement/link step
rolls back the new objects. One **Undo** removes the entire pasted group, while
checking ownership and entity generation so reused slots are not removed. Occupied
copied vehicles/controllers defer Undo until exited. Original objects remain intact.
The immutable clipboard can be pasted after its sources have been deleted.

Native controllers start from their catalog definitions: this does not snapshot a
mission script's instruction pointer, NPC AI state, elapsed timers, animation pose
or live linear/angular velocity. Authored mission target chains are not inherited;
map entities use standalone portable definitions. Only owned target networks are
copied. Some native controller setups can still reject placement where their initial
helper layout has no space; failure leaves no pasted assembly. Clipboard and Undo
are session-local and cleared on load/transition. Resulting owned objects, native
frozen controllers and joints use existing full-save V8/wire 5; APIs remain 1020.

Production clipboard/paste/undo tests cover source deletion, stale entity slots,
partial allocation rollback, blocked placement, link-capacity rejection, helper
reuse/rewiring, health/freeze/count properties, and save-record round trips. A real
Box3D comparison verifies that the translated welded copy retains the source's
relative motion under simulation. Full native gameplay acceptance remains open.


## 0.23.0 entity context menu and held rotation (2026-10-10)

Hold **C** while Sandbox is enabled to select an entity under the crosshair, then
click another visible entity in the left world area to select it. A gold outline
and the class/entity label identify the selection. **Select player** targets the
player directly. The right panel edits position, pitch/yaw/roll, and positive health
for living damageable entities. Apply each section separately; **Refresh** reads
the current values without continually overwriting text being edited. Freeze,
Release, Activate, Remove, Copy and Paste use the native tools; Paste places the
copied catalog definition at the crosshair. Undo restores property/pose changes.
Deleting authored entities still requires a prior save to restore them.

While holding an object with **Physgun + left click**, hold the bound **USE** action
and move the mouse to rotate it. The mouse no longer turns the view during that
gesture. Release USE to resume ordinary holding. Wheel distance, right-click freeze,
fling momentum and accepted beams are retained. Simulated props rotate through an
angular target in the existing solver; welds/hinges/ropes retain their local frames
and may restrict rotation. Native entities use their held controller. Exact context
poses require unlinking constrained props; occupied vehicles must be exited first.
Native AI can face its targets again after release.

C is backed up/restored independently of Q and later user binding edits are kept.
The command is `+jar_contextmenu`, so it can be rebound. Text entry captures movement;
release C closes unless a field is being edited, in which case Esc/Close exits.
Entity generations and transient selection/hold tokens reject stale requests after
deletion, re-grab, save load or level change. Native health/transforms and frozen
controllers retain the existing full-save representation; undo/selection are local
to the current session. Save header V8, wire 5 and APIs 1020 are unchanged.

The complete overhaul goal remains active. This is an implementation milestone;
no game was launched for this release and visual/control acceptance is pending.


> **0.22 update (2026-10-10):** 1,197 searchable entries, 74 newly spawnable
> from the previously blocked set, 108 remaining blocked. [Catalog controls](JAR-spawn-catalog.md)
> cover portable map controllers, automatic helpers, Activate and Link. The fixed
> limits are 4,096 owned objects and 128 actors. Default `g_noFootSlide 1` now
> adapts NPCs only; the player's walking fix still needs visual acceptance.
> Beams are owner accepted and unchanged. New saves use V8 `0x4a415209`, wire 5.
> Older release sections below are historical where they conflict with this update.

Enable Sandbox in Settings, resume, then hold Q for the unpaused overlay. Release
Q or press Esc to close. The five tabs contain 1,197 stock-icon cards: 723 props,
178 NPCs, 31 weapons, 242 entities and 23 vehicles. Cards are paged in
groups of 12 and each tab remembers its page. Icons reuse unedited installed game
art; generic categories stand in where the game has no suitable specific icon.
The build records each copied icon's original path/hash and verifies exact bytes.

The vertical tool list uses small text; selected tool instructions and properties
appear to its right. Click a tool to grant/equip Toolgun and select its mode. Tool
help, action status and menu hover descriptions are anchored at the top left.
Rope slack and thruster force apply to newly created attachments.

## Controls

| Tool | Left button | Right button |
| --- | --- | --- |
| Physgun | Hold to move; release to fling | Freeze held target; release aimed target if nothing held |
| Select | Select aimed entity | Select aimed entity |
| Activate | Fire/toggle native use action | Select aimed entity |
| Link | Select source, then destination | Cancel pending link |
| Rotate | Rotate +15 degrees | Rotate -15 degrees |
| Freeze | Freeze | Release |
| Gravity | Release | Freeze |
| Remove | Remove aimed entity | Undo last reversible action |
| Weld / Hinge / Rope | Choose first and second frozen props | Cancel pending first selection |
| Unlink | Remove attachments on aimed prop | Inspect/select |
| Duplicate | Copy if empty, otherwise paste at crosshair | Copy another supported entity |
| Thruster | Attach or toggle on/off | Remove thruster |

While Physgun holds something, weapon-wheel input changes distance (48-2,048
units) instead of switching weapon. Both triggers must be released after a freeze,
switch, protected player action or load. No ammunition is consumed. Physgun uses
native electricity from muzzle to object; Toolgun uses a distinct short amber shot.
Toolgun has no idle beam. Physgun/Toolgun remain native weapon IDs 29/30.

## Placement, capacity and entity control

Spawn placement uses the crosshair, validated assets and collision clearance.
Capacity is fixed at 4,096 owned objects, including props and death drops. There
is no adjustable prop cap; the old limit command only reports the fixed capacity.
The separate eight-owned-actor bound and native entity exhaustion checks remain.
Cleanup removes spawned objects and defers occupied vehicles until exited.

All in-use entity slots, including player zero, are eligible for direct selection,
hold, rotation and freeze. World geometry is not an entity that these tools can
rebuild. Invisible/non-solid entities can be selected with `jar_sb target <id>`.
Normal crosshair traces select visible collision entities. To release a console
freeze of the local player: `jar_sb target 0`, then `jar_sb tool release`.

Native holds use swept collision movement and the harmless Force-grip flag/pose;
they do not activate the damaging Force power. Native player/NPC movement and
think/trajectory updates are suspended, then restored on release. Movers resume
their translated path. Generic objects without a separate scripted thinker use
native gravity on a fling. Existing scripted thinkers retain authority afterward.
Freeze persists across a full save. Save/weapon changes release a live hold first.

Remove affects authored map entities as well as spawned ones. Player removal uses
native death handling. Deleting mission entities has no undo and may break scripts;
reload an earlier save to restore them. Actor randomizer Mission Safety remains
separate from explicit Sandbox edits. Occupied owned vehicles retain their removal
guard. Native controllers and linked physics assemblies have different capabilities:
Weld/Hinge/Rope/Thruster require qualified spawned Box3D props.

Duplicate copies the supported prop/NPC/vehicle/pickup catalog definition and
orientation. It validates placement and actor/resource limits like a normal spawn.
It does not clone arbitrary scripts, health/inventory, modifiers or entire assemblies.
Unsupported definitions return an explicit message instead of a partial clone.

## Undo and saves

Undo retains 64 recent spawn/transform/attachment actions. It checks entity lifetime,
destination clearance and linked assembly state. Deleting/reusing an entity removes
stale references. Undo history is session-local and clears on load/map change.
Unlink itself is not undoable. Full saves retain native freeze records, spawned
registries, rigid poses/velocities and validated local attachment frames.

The 0.20 save header is `0x4a415207`, workshop wire revision 5. Native and earlier
JA:R saves remain readable. Earlier releases reject newer saves. Map transitions
and autosaves carry inventory only, and start with no map-owned attachments.
Game API 1019 enforces the matching engine/game save contract.

See [physics details](JAR-physics.md) and [automated versus native evidence](JAR-validation.md).
