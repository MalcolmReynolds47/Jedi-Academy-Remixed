# JA:R overhaul requirements and checkpoints

## Multiplayer priority backlog

Owner priorities recorded on November 16, 2026. The multiplayer-only scope
applies to every development task. The multiplayer rope items reopen that
work for multiplayer. The earlier single-player rope hold still applies.

1. **NPC spawn stability and capacity.** JA:R 1.330.0 reproduced a multiplayer
   client drop after loading 512 NPCs on a graphical listen server. NPC client
   records filled the fixed shared allocation pool. In 1.331.0, each record is
   allocated and freed with its entity. The patched listen server loaded, saved,
   cleared and respawned 768 live NPCs, and separately kept 423 NPCs alive through
   the spawn-menu command. The exact crowd from the owner's report is unknown.
2. **Context menu numeric fields.** JA:R 1.332.0 removes the former tuning
   ranges from multiplayer tool and entity editors. Numeric fields filter
   typing and validate complete numbers before applying. The server enforces
   each property's native type, geometry and storage requirements. Wider
   actor movement values round-trip through a new journal revision, with
   previous world journals still readable.
3. **Rigid constraints.** JA:R 1.333.0 uses inelastic rope tension and
   projects multiplayer rope, weld and hinge positions before publishing the
   server frame. Tests kept a 25,000 kg Box3D load on a 128-unit rope at up to
   1,400 units per second, stopped an NPC at the 100-unit limit after a
   100 ms frame, and preserved a mixed rope and weld with a 1,000:1 mass
   ratio. A hinge retained its free rotation axis under load.
4. **Rope length and pulling around corners.** JA:R 1.334.0 recomputes the
   full routed length after collision sweeps and shifts correction to the
   free end when the other end is blocked. Movable bend supports participate
   in the same tension solve. A routed path that cannot fit the configured
   length is withheld from snapshots until it becomes feasible. Tests covered
   sustained pulling around a pillar and a moving support with frozen ends.
5. **Grenade spawner visibility.** JA:R 1.335.0 registers newly announced
   item and projectile models on active multiplayer clients. A graphical
   Flatgrass listen-server test reproduced grenades reaching the host without
   a model while a late joiner drew them. The fix drew matching primary and
   alternate grenades on the host, an existing client and a late joiner.
6. **Host-only prop visibility.** JA:R 1.336.0 loads renderable Ghoul2 skins
   when multiplayer client prediction first builds a prop's hull. An existing
   client previously cached skins with zero render passes for several NPC-model
   props. A graphical Flatgrass listen server verified visible materials and
   matching props for the host, an existing client and a late joiner.
7. **Multiplayer-only development.** Treat multiplayer as the sole default
   development target. Single-player work requires an explicit owner request.
8. **Player Context editing.** JA:R 1.337.0 adds clearly labeled self and
   remote player selections, plus editable health and armor. Movement
   properties remain available to eligible players. A player held by a
   Sandbox tool can still edit their own health or armor and release their
   body. Remote edits and Revert require server permission, target consent
   and the identity of the selected player life.
9. **Physgun NPC throws.** JA:R 1.338.0 advances a released, airborne NPC
   under ordinary gravity on every multiplayer server frame. The native
   movement code applies the release velocity and handles impacts, gravity
   and landing. Normal AI steering resumes on landing. JA:R 1.344.0 fixes
   the missing humanoid grip animation and preserves a moving guest's throw
   when release falls between aim packets.
10. **NPC combat, movement and landings.** JA:R 1.339.0 gives ground-based
    ranged Sandbox NPCs clear-shot aiming, pursuit and spacing. They can
    advance, back away, move around obstructions and follow a friendly
    spawning player while idle. Scripted tasks and specialized NPC behavior
    keep their native logic. NPC landings now apply damage based on impact
    severity and honor the server's fall-damage setting. In 1.345.0 NPCs
    ignore passive props during enemy searches, and a follower can pass a
    barricade to reach the player who spawned it.

[November 17 multiplayer audit](JAR-multiplayer-audit-2026-11-17.md): verified release state, reproduced defects, test coverage, and recommended follow-up.

## Multiplayer NPCs following around scenery

In 1.344.0 a rebel following either the host or the joined player fired at
a row of seven frozen crates and stopped 653 units short of its owner.
The crates had a positive health value even though they could take no
damage. Fixing the enemy search alone stopped the gunfire, but the
follower then stalled at the wall, about 419 units from its owner.

In 1.345.0 a grounded follower checks the face of the obstacle and keeps
the same direction long enough to pass adjacent props. It checks ground
and hull clearance at every step. On Flatgrass the rebel went around the
end of the barricade for both owners. The two Classic clients recorded
matching NPC positions at shared server times. The clear-path run also
reached the owner. These cases use two processes on one computer.

Run `scripts/test-mp-listen-crowd.py --npc-follow` with the staged MP
package, retail game data, native snapshot reader and a fresh results
directory. The scenario covers both owners, an unobstructed route and
opposing riflemen fighting beside the same barricade.

The next AI investigation is multiplayer sight and sound alerts. The
native alert search returns early when player slot zero has no health.
The native follow behavior also indexes the alert list when the search
returns no event. Guest detection while the host is dead and behavior
after the first player disconnects still need verification.

## Multiplayer patrols and combat squads

In 1.342.0, multiplayer NPCs only merge into occupied squads of their own
faction with the same enemy. Idle patrols must still pass the native range
and map visibility checks.
NPCs that change faction leave their previous roster on the next squad update.
Groups can fill the last ordinary squad seat on a merge without exceeding the
existing size limit. This builds on priorities 1 and 10.

The repeatable `scripts/test-mp-server.py --network-crowd` scenario takes the
usual build and game-data arguments. It puts 48 rebels, 48 stormtroopers and
16 simulated props in one arena. The second client joins after shooting has
begun. Both must keep receiving the same NPCs and props for 30 simulated
seconds and see them disappear after cleanup. The wire probe requests full
snapshots and records their arrival intervals in server time.

On Flatgrass, a graphical listen host and a second graphical client on the
same machine recorded the fight with the staged Classic renderer. The guest
joined after combat began. At matching server times the recordings agreed
on NPC and prop identifiers, flags and positions, including the extra prop
spawn, its Undo, and cleanup. Both saw an injured NPC already on the map,
a new fall from 650 units that left 22 of 50 health, and a lethal fall from
1,800 units. The guest saw an authorized armor edit and could still edit
their own player when the server disabled remote edits. Both player opt-out
and server policy blocked the host from editing the guest.

Run `scripts/test-mp-listen-crowd.py` with the retail GameData directory,
staged MP package, `jar_mp_snapshot_tests` executable and a fresh results
directory. Use `--interactions` for falls and player permissions. The
crowd run records both perspectives and samples host server work, process
memory and snapshot intervals. A friend connecting from a second machine
over the Internet remains an open check.

## Multiplayer crowd snapshot delivery

In 1.342.0, a Flatgrass host and guest running at 40 server ticks per second
both received updates a median of 75 ms apart with 384 live NPCs and 16 props.
Small local snapshots got the next server frame. A fragmented snapshot
could take several frames because each remaining piece was scheduled
separately, with an additional network rate delay.

In 1.343.0, an active listen host and clients covered by the server's LAN
override can receive up to four fragments per frame. Larger messages
continue on subsequent frames. Downloads and clients using ordinary network
rate limits use their existing scheduler. The message and demo formats are
unchanged.

In the staged Windows build the two recordings agreed at 1,410 server times
during 35.6 simulated seconds with 384 live NPCs and 16 props. Both clients
received a median update every 25 ms at 192 and 384 NPCs. Both saw the
additional NPC appear and disappear on Undo, then saw the crowd disappear
on cleanup. These same-machine measurements used the Classic renderer.

The graphical `scripts/test-mp-listen-crowd.py --crowd-scale` scenario
records both players during 35-second fights at 192 and 384 NPCs and then
spawns and undoes an additional NPC. The `--crowd-scale-paced` variant
sets the UDP guest to 25,000 bytes per second with the LAN override off.
Both runs compare NPC identities, flags and positions, Undo, cleanup and
the time between snapshots.

## Multiplayer Physgun holds and throws

On 1.343.0 a listen host and a connected guest recorded 202 shared held-NPC
snapshots across their two throws. None showed the stock grip pose. In the
guest's throw the trooper moved 41 units horizontally during the swing,
then dropped straight down. The animation starters were receiving an actor
whose movement had already been frozen. Release velocity used only the last
server frame, which could have the same aim as the preceding frame.

In 1.344.0 the server applies the native grip pose to humanoids before
freezing movement. It measures the on-foot actor's actual displacement
over the preceding 100 ms. When a frame gets no movement, the last measured swing
velocity is held for up to 100 ms. A blocked trace discards earlier motion.
Parking or re-grabbing the actor clears the old momentum.
Vehicles keep their existing movement calculations.

The graphical `scripts/test-mp-listen-crowd.py --physgun-throws` scenario
records the host and guest on Flatgrass. It compares the trooper's pose,
trajectory, health and return to combat, and checks both players' release
paths. The guest also throws with outgoing input capped at 20 packets per
second. Internet play and the feel of a physical mouse remain owner checks.

On the staged 1.344.0 build, the host, guest and capped guest threw the NPC
349, 182 and 454 units horizontally. The two clients agreed on position,
pose and landing, and the NPC resumed firing in all three cases. A stock
30-health trooper kept its health during the stationary holds. The combat
checks raised its health through Context so it could survive hard landings.
Both players stayed at 100 health when held. The player who opted out moved
54 units before the holder released the trigger.

## Multiplayer support on props and moving platforms

On Flatgrass in staged 1.342.0, a connected player stood on an unfrozen
Box3D supply crate, jumped and landed back on it. The same player rode a
bobbing crate through 24 units of vertical travel and a vertical lift through
a 128-unit ascent and return. Both players' recordings agreed on the
supporting entity, platform position and jump transitions at each shared
server time. The host's screenshots show the rider moving with each deck.

Use `scripts/test-mp-listen-crowd.py --platforms` with the usual graphical
test arguments to repeat the three rides. The test uses a Classic listen
host and a second client on one machine.

The companion `--vehicle-lift` check puts a guest on a stock Swoop over the
grate. The host activates the lift and observes the pilot and vehicle. Both
recordings showed the same mounted player and vehicle positions throughout
the 128-unit ascent and return. The driver dismounted with USE. The host's
Undo was refused while the Swoop was occupied. Once the driver dismounted,
Undo removed the Swoop on both clients and left the lift in place.

## Multiplayer NPC Undo crash

The Windows crash dump for 1.339.0 resolved to `AI_InsertGroupMember` during
the squad update after Undo. Removing an NPC left its slot in the native
squad roster. In 1.340.0, entity removal clears squad references before the
slot can be reused. Squad updates also discard invalid members before
merging groups.

## Spawnable map sections come last

The owner directed that remaining spawnable map-section work come after the
other overhaul objectives. Preserve the staged section work and leave its
unresolved requirements on the checklist.

## Single-player saved hub revisits

JA:R 1.323 restores a hub's saved level, objectives and Sandbox objects
before applying the arriving player's carried inventory. The outgoing
loadout and completed campaign tiers survive the destination save's cvars.
The requested start controls the arrival view. When a saved start marker
is missing, the player uses a normal start. A restored seat or player
freeze clears before the player arrives. The client-entry callback
publishes the hub autosave after the restored player enters the world.
Native fixtures cover the level-save layout and inventory merge.
In-game hub traversal and respawn remain open.

## Player carry data across single-player maps

JA:R 1.324 validates the original player carry record before applying
health, saber, weapon and Force data. The native strings retain their
existing field order. Ammo and Force ranks use bounded, transactional
readers that accept shorter records and check the native rank ranges.
Precache registers matching installed item definitions. Missing or
malformed carry data uses the map's starting setup, and an invalid
auxiliary list falls back independently. Native tests cover legacy
records, current tools, malformed values and excess entries.

## Indexed helper removal in single-player Sandbox

JA:R 1.293 indexes the direct helpers of each owned entity. Removal
checks the selected object's descendants and frees them through native
cleanup. Grouped Undo checks each affected member once in preflight
and checks live occupancy before each removal. Duplicate uses the same
index to find regenerated helpers. Despawn All retains its preflight
for occupied controllers and supported actors.

The index rebuilds from the existing save records and preserves their
order, including older saves that list children before parents. Native
tests cover nested occupied helpers, detached pickups, slot reuse,
groups, and save round trips. Large-scene performance and in-game
removal still need measurement.

## Batched single-player assembly paste

JA:R 1.292 publishes a copied assembly once, after its objects and links
are ready. Undo stores one assembly entry and leaves earlier history in
place. Paste matches regenerated helpers to their copied parents using
only ownership IDs created during that paste.

A failed paste discards its uncommitted copies even when one overlaps or
temporarily supports an actor. The previous selection is restored only
when it still refers to the same entity lifetime. The native fixtures
cover a 72-object assembly with 24 trains and 48 reused path corners,
one menu refresh, one grouped Undo, prior history preservation, and
allocation failure with an overlapping copy. In-game paste latency and
interactive confirmation remain open.

## Faster ownership lookups in large single-player Sandbox scenes

JA:R 1.291 indexes the Sandbox entity slots and searches ordered
ownership IDs directly. Spawning, selecting, linking, copying and
clearing entities can resolve an ownership record without scanning all
objects in a normally ordered world. The index is rebuilt on load and
older saves with out-of-order records keep their original wire order.
Reused slots receive fresh ownership IDs.

The native ownership and save fixture exercises the full 16,382-record
registry. Its median whole-process runtime over seven runs dropped from
133.5 ms to 34.1 ms. Link, Duplicate, save and legacy map-section
restoration fixtures also passed. Live frame time, spawning at capacity
and interactive cleanup still need measurement.

## Stock models for carried single-player weapons

JA:R 1.290 gives the seven Weapons tab entries with empty retail models
visible first-person and hand models on foot, plus visible Sandbox drops.
The AT-ST Main uses the stock concussion rifle, AT-ST Side uses the
Merr-Sonn, Emplaced Gun and Rapid Concussion use the repeater, TIE Fighter
and Turret use the E-11, and the probe-droid laser uses the Bryar pistol.
They keep their own weapon IDs, firing behavior and ammunition rules.
Existing custom model paths and native mounted visuals take precedence.

The native save loader refreshes older empty hand attachments and old
Sandbox drops. Fixtures exercise unavailable media, custom paths, mounted
exclusion and failed attachment replacement. Retail assets were checked
for visible view meshes and the hand, muzzle and barrel tags. In-game
appearance and full interactive save/reload remain open.

## Single-player firing for carried mounted weapons

JA:R 1.289 lets the on-foot player fire the AT-ST Main, AT-ST Side,
Emplaced Gun, TIE Fighter, Rapid Concussion, and Turret weapons from
the Weapons tab using the player's aim. The muzzle trace checks scenery
and shot clips. The AT-ST cannons no longer look for walker bolts on the
on-foot character. Real walker mounts still use the native weapon bolts
and check that the current firing mode's bolt is available.

Turret now launches the stock sentry bolt and uses the stock impact
effect. Configured turret damage applies, with the stock sentry's damage
as the default. Native fixtures cover six card selections, aim and muzzle
obstruction, mounted-state exclusion, and turret projectile fields and
allocation failure. Rendered firing and mounted gameplay remain open.
The stock mounted weapons still use their existing model resources.

## Shared portable camera feeds

JA:R 1.288 lets visible portable screens share a live feed when their
camera position, orientation, and screen proportions match. The four feed
slots can serve four distinct views. A fifth distinct view keeps the dark
fallback, while a matching screen can use an assigned slot even after the
budget is full. The map's own portal or mirror keeps its view.

The Classic and Rend2 renderer tests cover matching screens, different
camera views, offscreen panels, failed renders, and fallback materials.
In-game camera output and frame timing remain open.

## Single-player NPC patrol routes

JA:R 1.287 lets compatible Sandbox NPCs patrol ordinary Waypoint and Small
waypoint markers. Use the Toolgun's Primary Link between successive waypoints,
then link the NPC to the first waypoint and apply Patrol route in NPC orders.
The NPC stops at the last waypoint unless a Primary link continues the route.
A return link makes a loop. Patrol uses native steering and suspends
autonomous attacks. A disabled or missing current waypoint pauses movement.

Context shows the start and current waypoint. Reverting an order or changed
start restores the prior route position. V57 saves the current destination
alongside the V56 starting link. Duplicate remaps the starting link, current
waypoint, and waypoint chain to the copied assembly. Native tests cover
arrival-based advance, disabled nodes, an open end, a loop, Revert and copy
remapping. In-game navigation and a full save/reload remain open.

## Single-player NPC navigation links

JA:R 1.286 lets the Toolgun's Primary Link connect a compatible Sandbox
NPC to a spawned Nav goal (`waypoint_navgoal`). Select Go to goal in
NPC orders to move there using native steering and navigation. The actor
waits when the goal is disabled, missing or reached. Moving the goal can
direct the actor to its new position. The marker's configured radius
sets arrival range, and a solid obstruction prevents arrival through a
wall. This order keeps native script and mounted-controller priority.

Links use both the owned ID and the entity slot, so a replacement in
the same slot cannot inherit the order. Context shows whether the goal
is linked, disabled or missing. Clear goal link and Revert property
edit the saved association. Duplicate copies the linked NPC and marker
together and remaps the destination to the new marker. V56 saves carry
the link in a separate NPC navigation chunk, with older saves supported.
The native fixtures cover dispatch, disabled and reused destinations,
Context Revert, serialized link fields and remapping. In-game walking,
obstacle routing and full save/reload remain open.

## Single-player NPC orders

JA:R 1.285 adds the NPC orders page for compatible living Sandbox NPCs.
Autonomous uses the actor's original AI. Wait here suspends autonomous
movement and attacks. Follow player uses native follow movement and combat
with an editable distance from 72 to 1024 units. The actor and player must
both be on the Player team. A follower waits while the leader is unavailable.
Native sequences, vehicles and mounted controllers have priority.

Apply clears queued attacks and movement when the order changes. Revert
checks the edited fields. Follow uses the player as a temporary leader
during the AI decision, preserving any saved native leader. Native saves
and Duplicate carry the configured order and follow distance. The fixtures
exercise the menu, dispatch, attack cancellation, protected states, saved
fields and copied orders. Interactive navigation, combat and a full
save/reload remain open.

## Single-player NPC weapon picker

JA:R 1.284 adds an NPC weapon page for compatible living Sandbox NPCs.
Equip picks a native hand-held weapon, fists or unarmed. The actor keeps
previously carried weapons and existing shared ammo. A newly given weapon
receives stock starting ammo if it uses an empty ammo pool. Manual ammo
policy stays active. The picker excludes saber, dual-hand, vehicle and
emplaced loadouts.

The native replacement attaches a candidate to the hand before clearing
the previous model. A failed load or attachment leaves the active weapon
and ammo intact. Context Revert restores the old weapon and its fire
settings. It preserves live ammo when the initial grant has changed.
The NPC's native client and AI save fields carry equipped
weapons, ammo and the player-weapon matching setting. Native fixtures
cover stale selections, failed loads and attachments, Revert, and the saved
fields. Rendered combat and full save/reload remain open.

## Single-player NPC ammo supply in Context

JA:R 1.283 adds Manual and Native refill supply settings to NPC settings.
Manual uses the available pool without AI top-ups. Native refill preserves
the stock AI's refill behavior and is the default for existing NPCs and
saves. Unlimited ammo survives top-ups and pickups, and charged weapons
recognize it at charge start and when firing. Manual ammo also applies to
the native scripted fire path that bypasses ordinary weapon animation.

Apply and Revert check the refill mode independently of the other fields.
Reverting just a supply-mode change preserves ammo spent in the meantime.
Duplicate and native saves retain the setting. The native fixtures exercise
depletion, unlimited ammo, Revert, saved NPC state and Duplicate. In-game
combat and a full save/reload remain open.

## Single-player NPC ammo and fire mode in Context

JA:R 1.282 adds held-weapon ammo and fire mode to NPC settings. Whole ammo
uses the equipped weapon's native pool. Use -1 for unlimited ammo. The
fire-mode option is available when the weapon has a native alternate fire
definition. Changing mode updates the NPC's native firing cadence. Ammo-only
edits preserve the current cadence. Apply and Revert check the same held
weapon and edited fields, including ammo spent while the panel was open.

Native tests check ownership and weapon eligibility, input bounds, fire mode,
live ammo conflicts, active weapon changes, Revert, and the saved NPC and
client fields. In-game weapon behavior and full save/reload remain open.

## Single-player NPC allegiance and armor in Context

JA:R 1.281 adds NPC settings for living, freely controlled Sandbox NPCs.
Team, target team, and armor use the native NPC fields. Allegiance edits
clear stale combat targets and old AI group membership. Native target locks
prevent allegiance edits and still allow armor changes. Apply and Revert check
only the edited fields for concurrent changes, so team edits preserve armor
lost during combat. Duplicate uses the same actor values, and native saves
already carry them.

The Context and menu fixtures cover ownership, input bounds, target and group
changes, locked actors, conflicting edits, Revert, and the native client save
fields. In-game combat behavior and full save/reload remain open.

## Single-player fall cleanup beneath rotated rooms

JA:R 1.280 computes the depth of each live portable room within the falling
object's horizontal bounds. It clips the room's oriented bounds to that
column before applying the existing fall allowance. Empty corners around
rotated rooms return to the host map's boundary. A tilted room supplies a
fall limit based on its local depth under the object. Slot lifetimes,
occupancy checks, and native removal still govern cleanup.

Native fixtures check diagonal room corners, partial overlap, different
depths at each end of a tilted room, absolute limits, and live ownership.
In-game fall and cleanup confirmation remains open.

## Single-player Duplicate clearance near raised scenery

JA:R 1.279 lets a free assembly recover from an obstructed placement next
to raised scenery. Duplicate sweeps the copied solid members and mounted
charges together along the aimed normal over a 64-unit allowance. The first
contact sets one translation, and Duplicate checks the resulting assembly
before allocating it. A blocked starting position or blocked final position
rejects the entire paste. World-anchored assemblies, standalone planted
charges, and portable map sections require their exact placement.

The native fixture checks a raised edge under one of two props, a ceiling
that blocks recovery, a shelf obstructing only a mounted charge, retained
offsets and frames, and creation Undo. Interactive placement remains open.

## Single-player Duplicate contact on slopes

JA:R 1.278 seats ordinary copied assemblies using the support of each solid
member along the aimed surface normal. Qualified decorative props contribute
their rotated, saved-frame convex hulls. Other solid members contribute their
own bounds. Mounted charge extents, lift lowering allowance, and bobbing travel
still contribute. Non-solid trigger volumes stop moving an otherwise physical
assembly away from the surface. All-non-solid copies and portable map sections
keep their enclosing placement rules. Copy members retain their relative poses.

The native Duplicate fixture checks oblique contact, support from a second
member, mounted charge support, mover allowance, a non-solid volume, collision
rejection, and creation Undo. In-game placement and rendered contact remain open.

## Single-player convex prop placement

JA:R 1.277 uses the model-derived convex hull for decorative prop placement
against host and linked BSP scenery. Menu spawning uses the hull's support
along the hit normal, including oblique surfaces, and uses the same shape
when recovering a placement from a dip. Duplicate checks each copied prop's
saved frame and rotation before allocation, then checks the resulting pose
before publishing the copy. The scenery filter keeps placement through loose
props and NPCs available. Models without a qualified hull use the existing
rectangular fallback.

The native fixtures cover oblique support, recovery sweeps, selected frame
and rotation, a clear corner, an intersecting brush, and atomic paste and
creation Undo. The SP engine and game module use matching API 1031. In-game
placement and rendered contact remain open.

## Single-player decorative prop pose clearance

JA:R 1.276 tests Apply and Revert against the requested pose's rotated,
model-derived convex hull when one is available. Empty corners of the
rectangular bounds no longer block a valid pose. Native clearance still
checks solid geometry, actors, and mounted charges. Models without a
qualified hull retain the rectangular collision fallback. Accepted edits
preserve current Box3D velocity and spin for the rebuilt body and retain
existing mounted charge locations and timers.

The Context fixture tests a blocked Revert that succeeds once the obstacle
is gone, the convex-vs-box difference between two frames, solver motion
preservation, and charge clearance and beam refresh. Existing native saved
frames and clipboard state carry the chosen pose. Rendered contact and
full in-game save/reload remain open.

## Single-player occupied vehicle resources

JA:R 1.275 extends the Context Vehicle tab to living hulls with settled native
seats. On foot, select a vehicle in the world. While piloting, hold C to open
the current hull directly. The mounted editor exposes its armor, shields and
installed weapon and turret ammo, with Refresh and Revert for those fields.
Dismounting, damaged seat relationships, or a camera takeover invalidates the
mounted selection. The existing native fields carry resource changes through
saves. Automated checks cover native selection, Apply, scoped Revert and menu
dispatch. Driving, HUD feedback, and a full in-game save/reload remain open.

## Single-player vehicle thrusters

JA:R 1.274 applies mounted thruster force to the native hull for empty and
settled occupied single-player vehicles. Horizontal thrust feeds the saved
vehicle drift. Vertical thrust feeds native vertical velocity. Pilot input
continues to steer and drive, and the native seat relationships are preserved.
The existing transport validation pauses thrust during boarding, held or
frozen control, unsafe seat changes, and incompatible constraints.

Headless SP checks cover speeders, walkers, animals and fighters at 20, 40,
and 125 movement updates per second. They exercise native swept contact,
vertical launch, throttle, an attached pilot and droid, and reversed force.
The production save writer round-trips thrust-generated drift through its
existing chunk. Interactive driving, impacts and full save/reload remain open.

## Single-player entity removal and incoming links

JA:R 1.273 clears supported secondary outputs and distance endpoints when a
Sandbox destination is deleted or falls out of the map. Other live entities
with the same target name keep named outputs valid. A distance trigger with
a removed reference waits for a new reference or an explicit Clear before it
can fire. Property Revert checks the applied link and the previous
destination's lifetime.

Reconciliation discards stale ownership records without freeing a replacement
entity at that slot. Occupied children whose stale parent is discarded remain
owned, and the creation Undo record follows them. Fixtures exercise both
removal routes, a shared target name, a reused slot, the occupied child and
its saved creation Undo. Interactive cleanup and full save/reload remain open.

## Single-player map search and startup checks

JA:R 1.272 searches map titles as well as BSP names. It scans one undiscovered
title per UI update during a search and renders thumbnails only for viewed
cards. Start map uses the same difficulty mask as native SP spawning. Named
starts, multiplayer-only starts, and starts filtered out at the current
difficulty do not qualify for a fresh map launch. The menu checks the current
archive again when Start is clicked, including if an archive changed while the
menu was open. The fixtures cover title search without reading preview
geometry, difficulty changes, malformed entity data, and an archive changing
after selection. A read-only comparison of 68 installed BSP entries against
1.271 found identical titles, launch eligibility and preview start positions
at the default difficulty. Rendered menu and interactive map starts remain open.

## Portable weapon and ammo rack stock

JA:R 1.271 carries supported Sandbox racks and their attached stock on
portable single-player lifts, bobbing platforms and angular movers. The
platform plans the rack and shelf clearances before moving either. A blocked
pickup stops the platform, including when an obstruction appears after the
platform's native push step. Detached pickups keep independent motion.
Headless coverage includes clip-only ammo rack bodies, vertical movement,
rotation, frozen racks and clearance rollback. In-game acceptance is open.

JA:R 1.270 keeps copied shelf pickups suspended through the native item
update in normal and zero gravity. A replacement pickup created during
paste inherits the shelf's lifetime. Native flung racks carry the stock
as long as its swept hulls fit. A shelf obstruction stops the rack at its
last clear pose. A clear throw keeps its native momentum, and an independently
detached pickup resumes native item physics. Headless fixtures exercise the
item update after paste, fallback pickup creation, and blocked native motion.
Interactive throw, impact and full save/reload remain open.

JA:R 1.269 moves suspended pickups with owned single-player racks during
Physgun drag and USE rotation, Toolgun rotation, and Context pose changes
and Revert. The move checks the shelf items for clearance before changing
either the rack or its stock. Force Pull and independent Physgun movement
detach a pickup from the shelf while retaining its cleanup ownership.
Duplicate preserves the remaining stock and its attachment status. The
existing ownership records and item state carry the attachment across saves.
MP already updates the shelf pose from its rack controller. Matching the
SP clearance checks in MP and interactive validation remain open.

## Local installation of 1.267.0

Build `20261114-205354-f7e03115` passed 159 automated suites and 30 two-client
checks. The packaged Sentinel lift check passed on retry against the same
staged binaries. The remaining original build gates then passed. The verified
ZIP was installed at the owner's request, and all 75 installed hashes and 23
replacement backups verified. See
`build/deploy/20261114-210937-708b6c38/receipt.json`. In-game acceptance
remains open.

## Brightness on return from Alt-Tab

JA:R 1.265 queues the most recent hardware gamma ramp when its SDL window
regains focus, is restored, or changes size. It applies the ramp after a
focused frame is presented and reapplies it once after 250 ms if the first
frame was earlier. This covers display mode changes that finish after the
focus event. The shared window code serves SP and MP with Classic and Rend2.
An isolated check of the production scheduling code covered focus loss,
minimized and unrelated windows, a failed first write, repeated focus
events and timer wraparound. Interactive Windows brightness remains open.

## Single-player cleanup under portable rooms

JA:R 1.263 checks the falling object's horizontal hull against each live
section's transformed bounds. A lower room only extends the fall allowance
where the hull overlaps the section. Elsewhere the host map's boundary still
applies. Native and Box3D props use their current hulls. The cleanup path still
preserves occupied objects and handles loose drops separately from their
parents. The native fixture covers rotated rooms, partial overlap, absolute
limits, unlinked rooms and reused entity slots. In-game confirmation remains
open.

## Segmented NPCs with an explicit lower-body skin

JA:R 1.264 supports `legsmodel model/skin` for compatible single-player
segmented NPC definitions. Precaching, initial spawn and render registration
share the same model directory for animation lookup. The suffix selects the
lower-body skin. Catalog admission requires the custom skin and the animation
file, and continues to allow embedded materials for default lower skins.
Native fixtures cover asset admission, missing resources and malformed names.
An isolated registration probe compiled the production functions against model,
skin and animation lookup spies. It verified the requested lower skin, the
same animation directory at spawn and render registration, and embedded default
materials when a default skin file is absent. See
`build/npc-legacy-render-audit-1.264.0/result.txt`. The stock generated catalog
still has 1,186 cards. In-game rendering and save/reload with a custom lower
skin remain open.

## NPC catalog parsing of compact definitions

JA:R 1.265 reads recognized NPC fields individually and keeps the closing
brace as a catalog boundary. Native tokenizer fixtures cover adjacent
compact NPCs, ordinary and multi-value fields outside the catalog, quoted
braces, missing field values and first-definition precedence. A comparison
of all 201 retail records produced identical metadata and admission results
in both the Sandbox and campaign catalog modes. See
`build/brightness-catalog-audit-1.265.0/`.

JA:R 1.266 shares a quote-aware definition lookup across single-player NPC
spawn, animation precache, model precache, and template fingerprint checks.
The precache passes skip unrelated values, including named and numeric
`customRGBA`, without interpreting them as resource keys. Missing values
and unknown end-of-line fields keep the closing brace available to the
native parser. Case-insensitive first-definition precedence is preserved.
The production-function probe reproduced the failure on 1.265 and verified
the 1.266 spawn, model, skin, and animation calls with engine registration
spies. The 201 retail definitions had identical results for the observed
precache calls, spawn parameters and model selection. All 186 generated
NPC fingerprints in the comparison still matched their definitions. See
`build/npc-native-lookup-audit-1.266.0/`. In-game spawning with custom
definitions remains unverified.

## Rope work when anchors move

An isolated native sweep fixture in 1.266 found a cold-start failure.
Three alternating baffles allow a six-bend route using the existing eight
slots. With empty pivots, the solver reports blocked after 424 sweeps. With
the independently verified route as a seed, it keeps six clear bends and
builds a clear rope curve. See `build/rope-corridor-audit-1.266.0/result.txt`.
An isolated prototype solves the blocked middle span and checks the far
passage in the direction the rope crosses it. Its 20 mirrored, reversed,
and endpoint-offset cases all produced clear six-bend routes and meshes,
with a maximum of 502 of the 512 allowed sweeps. The existing solver
failed all 20 cold starts. See
`build/rope-corridor-audit-1.266.0/directed-matrix.txt`.

JA:R 1.267 integrates that third passage in the shared Rope solver. It
tries shorter routes first, allocates bounded work per candidate pair, and
reserves sweeps for final route validation. The original eight saved bend
slots and 512 sweeps per route are sufficient. Native SP and MP fixtures
cover all 20 layouts, trace-count accounting, clear curves, visible meshes,
and 16 simultaneous cold routes sharing the 4,096-sweep frame limit. The SP
Workshop test creates a six-bend Rope, round-trips it through its save
codec, revalidates the mesh and removes the saved joint. Wider geometry,
rendered gameplay, full in-game saves and map frame times remain open.

In the isolated SP load fixture, all 16 cold routes resolved over two
simulated frames. The heaviest route used 494 sweeps. All 16 retained moving
routes updated within quota on each of 180 frames. With 32 moving routes,
361 of 5,760 updates were deferred by the 4,096-sweep limit. A separate
four-baffle fixture verified an eight-bend route by native sweeps. The
solver retained it when supplied and exhausted its usable search budget
from empty pivots. See `build/rope-routing-audit-1.267.0/result.txt`.
An isolated eight-bend prototype projects central offsets along their hit
faces and adds lateral aperture samples. It resolves all 60 axis-aligned
and 120 rotated cases within 511 sweeps, with clear final rope curves.
The existing SP native route fixture passes against the prototype.
The staged runtime still uses the six-bend detour. See
`build/rope-routing-audit-1.267.0/four-extra-offsets-rotated.txt` and
`build/rope-routing-audit-1.267.0/four-extra-offsets-matrix_prototype.txt`.

JA:R 1.268 integrates the eight-bend route in the shared Rope solver.
It samples narrow lateral openings and projects the two middle offsets
along the obstructing faces. Two- and four-bend routes are checked first.
The longer routes share the existing eight saved bend slots and 512-sweep
limit. Native SP and MP tests cover 60 axis-aligned and 120 rotated cases,
clear curves and meshes, and byte-identical retained pivots after an
exhausted budget. The single-player Workshop test also creates, saves,
reloads and removes all eight bends through the production codec.
Interactive confirmation and real map frame times remain open.

An isolated 1.268 performance fixture identified a retained-route failure
near a brush corner. At an anchor offset of about 1.92 units, the forward
sweep stopped just short of a saved pivot and the reverse sweep passed.
The router reported blocked with 512 sweeps available. A prototype verifies
a small outward adjustment of the existing pivot and reuses one swept
prefix during tightening. With the same 4,096-sweep frame limit, the
prototype handled all 5,760 updates for 32 three-passage ropes. In the
32-rope four-passage fixture, deferred updates dropped from 1,170 to one.
Both changes remain outside the staged runtime. Native fixture results are
in `build/rope-budget-audit-1.268.0/retained-blocked-spans.txt` and
`build/rope-budget-audit-1.268.0/pivot-budget-adjustment.txt`.
The same audit found that an eight-bend retained route could succeed
under a 36- or 40-sweep remainder without probing any bend contacts.
The experimental solver reserves that work and defers those attempts.
With 42 sweeps available all 120 rotated retained routes complete with
contact probes. The revised prototype maintains the 32-rope results above.
See `build/rope-budget-audit-1.268.0/retained-contact-reserve.txt` and
`build/rope-budget-audit-1.268.0/prototype-retained-contact-reserve.txt`.

JA:R 1.262 tightens an existing clear route once and uses two passes after
adding new bends. Collision validation and the trace ceilings still apply.
The native moving-rope fixture now checks 32 four-bend routes under a single
4,096-sweep allowance. Collision, visible rope, blocked recovery, and SP
Workshop save-codec tests passed. In the 180-step audit, all 32 route checks
completed each step with at most 2,592 sweeps. The 64-rope audit still
deferred 12 checks per step. The audit isolates route work from the game frame.
See `build/rope-corridor-audit-1.260.0/native-budget-result-1.262.0.txt`.
In-game behavior and populated-scene frame times remain open.

## Fall cleanup with portable rooms below the map

Through 1.262, single-player took the lowest portable room anywhere in the map
when setting the fall threshold. That calculation retained a falling prop
under the host map even when the lower room was far to one side.
The folder `build/sandbox-fall-audit-1.262.0/` reproduces this and checks a
horizontal overlap policy for the lower room, partial hull overlap,
an unlinked room and absolute world bounds. The 1.263 release applies the fix.

## Rope routes through narrow offset openings

JA:R 1.261 extends the staggered Rope search to four bends when a shorter
route fails. The line can clear the first face before turning toward the
second opening. Every span is checked before changing the saved route,
and the existing bend and trace limits still apply. C and C++ fixtures,
native SP and MP brush sweeps, and the SP Workshop save codec cover the
narrow corridor, the visible curve, and removal. The exact release solver
cleared all 140 audited corridors. The fixture used up to 495 traces on a
first solve and 152 with a retained path after the sampled anchor move.
See `build/rope-corridor-audit-1.260.0/result-1.261.0.txt`. Interactive
gameplay, longer obstacle sequences, and timing in a populated scene remain
open.

In a 180-step native brush fixture with moving four-bend ropes, 16 ropes
stayed within the 4,096-sweep frame allowance. A 32-rope workload completed
30 solves per step and deferred the other two. This isolates route work from
the game frame, renderer, and physics. See
`build/rope-corridor-audit-1.260.0/native-budget-result-1.261.0.txt`.

The one-pass tightening prototype was outside 1.261 and became the 1.262
change above.

## Rope routing around offset barriers

JA:R 1.260 first tries the existing route around a blocked span, then tests
different offsets at the two ends when those bends cannot share an offset.
The candidate span and its connections must clear the same collision hull
as ordinary Rope. Existing bends keep their side when they still fit, and
a failed route leaves the previous path intact. The 512-trace per-route
and 4,096-trace per-frame limits still apply. The native fixtures cover
the alternate corridor route and moving anchors. An offline sweep of 140
rotated corridor configurations with known clear routes found that 92 could
use this two-bend search. First solves used up to 385 traces with a maximum
of 89 after the tested anchor movement. The audit is in
`build/rope-corridor-audit-1.260.0/`. In-game verification, routes requiring
more turns, and representative frame cost remain open.

The separate four-bend prototype evaluated during 1.260 became the 1.261
search above. It was outside the 1.260 release.

## Single-player Duplicate timing for Welders and Laser Arms

JA:R 1.259 preserves a Welder's remaining pulse cooldown and a firing Laser
Arm's remaining burst and next sample delay in Duplicate. Pasting uses the
copied parts and samples the new position before drawing or damaging anything.
It does not replay the initial firing sound. Finished bursts stay finished,
and undoing the paste removes its scheduled work. Native save fields retain
the laser's pending deadline. Interactive gameplay and full-save confirmation
remain open.

The 1.259 Rope solver had a reproducible routing gap. In an
enclosed corridor with two offset barriers, independently swept two- and
four-bend routes cleared both barriers. That solver returned `BLOCKED` after 82 traces.
The reproduction and output are in `build/rope-concave-audit-1.259.0/`.

## Staging, multiplayer player appearance and brightness

The build stages locally by default and requires explicit `-Install`
or a separate deployment command. After staging a release, it prunes
older generated releases and test runs. Shareable test installers, the
map package and installation backups are preserved.
Retention also clears the executable-relative install-path test fixtures.

Multiplayer player selection discovers installed humanoid models and
skins without requiring a portrait. Twenty-six NPC meshes get separate
player-sized adaptations that use the humanoid skeleton, including
Rancor and Wampa. The stock definitions also reference the player slot,
the Remote's legacy MD3 (served by the Remote adaptation), and a missing
test model. Nonstandard models in third-party packs need a compatible rig.
The multiplayer hilt lists include installed single-player hilts, and
JA:R Settings accepts a hexadecimal RGB color for each hand.
The shared window code reapplies the hardware gamma ramp across the Alt-Tab
display transition.
In-game appearance, animation and brightness confirmation remain open.

## Multiplayer Controls and hosting

JA:R 1.257 opens the native binding editor from the main menu,
Escape menu, and JA:R Settings. Back and Escape return to the page
that opened Controls. Host JA:R Sandbox exposes an editable UDP
port and an Allow cheats toggle. A busy port keeps the host screen
open with an error. Cheats default to off and apply on map changes.
Dedicated loopback tests confirmed exact port binding, recovery from an
occupied port, and server-side enforcement of both cheats settings.
Interactive Controls and listen-server confirmation remain open.

The legacy multiplayer world fixture still rejects one rope that joins
a prop and an NPC at distant retail `mp/ffa3` spawn points. The same
saved objects and a separate world-anchored rope restore when tested
individually. Builds 1.255 through 1.257 reproduce the rejection.

## Multiplayer prop hulls and Sandbox logic markers

JA:R 1.256 uses the single-player model support hull for ordinary
multiplayer MD3 and Ghoul2 props. Box3D, server traces, player prediction
and moving-prop actor contacts share the same model and pose. Dedicated
controllers still use their authored collision shapes.

Invisible map logic uses a compact, translucent blue stock reticle in
both modes. The marker stays readable at distance with a capped size.
Interactive collision, prediction under remote latency, and the new
marker appearance remain open.

## Toolgun surface picking and rotation pivot

JA:R 1.255 shares Physgun surface picking with the fired Toolgun in
single-player and multiplayer. Rotate and alternate Rotate preserve the
clicked point while turning the object. Attachment tools use the picked
surface, and shared scenery anchors use the world click even when it is
second. Ray hits, anchor frames and blocked rotation have fixture coverage.
Interactive confirmation remains open.

## Single-player collision during Weld and Hinge corrections

JA:R 1.255 uses a spawned prop's model hull along the full translation
and rotation of a native or mixed Weld/Hinge correction. Empty corners
of the old bounding box can clear scenery, and an obstructed correction
keeps the prop and its mounted charges at their previous pose. Models
without a qualified convex hull use a conservative swept box. Native
fixtures exercise mixed Box3D connections and collision along a moving
arc. Interactive constraint behavior and performance remain open.

## Independent portable camera feeds

JA:R 1.254 gave up to four visible portable camera screens their own
256×256 camera feed in SP and MP, in Classic and Rend2. Camera feeds
and the map's mirror or portal can render in the same view. As of 1.288,
the four slots are allocated to distinct visible views, and matching
screens share a feed. In-game camera output and the frame cost of four
live feeds remain open.

## Portable map section pickup placement

JA:R 1.253 places Vjun Sentinel, Rift Sanctuary, and Korriban FFA
pickups using only their own room's BSP collision in single-player and
multiplayer. The drop follows the room's local down direction and
unsupported anchors stay inside the authored layout. Host terrain and
nearby props cannot intercept the placement trace. Native collection
and respawn timers still belong to each room. Interactive pickup
placement and full save/reload remain open.

## Single-player scenery anchors and attached map-model copies

JA:R 1.252 puts the fixed Weld or Hinge pivot on the scenery click even
when the entity was clicked first. Hinges still take their axis from the
first click. Duplicate carries valid world anchors and mounted thrusters
when copying a supported native map model. The paste validates matching
scenery before creating anything, and Z removes the copy and its links.
Existing save records retain the remapped attachments. In-game placement
and full save/reload remain unverified.

## Single-player Context controls for mounted thrusters

JA:R 1.251 lets players select an entity in Context and adjust a mounted
thruster's force, power and direction. Reversing the thrust also reverses
the exhaust at the existing mount point. Full saves and eligible Duplicate
copies keep the edited values. Revert property checks the thruster and its
applied values before restoring the previous settings. Rendered controls and
in-game handling remain unverified.

## Single-player Context controls for attached ropes

JA:R 1.250 adds a Ropes button in Context when the selected entity has a
rope attached. It shows the other endpoint and lets players cycle through
multiple ropes and set their length in world units. Applying a length
validates the route around scenery and updates both the visible rope and
the active constraint. Full saves and Duplicate retain the edited length.
Revert property checks that the selected entity and applied length still
match. Z continues to undo spawns and Toolgun creations. In-game rope
handling and the rendered controls remain unverified.

## Portal screen visibility and single-player vehicle impacts

JA:R 1.249 preserves the linked camera's areas when single-player
builds the rendered view. Portal panels in Classic and Rend2 use their
world-space corners to determine visibility, without filling the shared
draw buffers during that check. The Rend2 monitor clears its target even
when its camera sees no surfaces, and draws its own sky when available.
The spawn menu calls the sky controller Map Sky View and points players
to Portal Surface for a linked live screen.

Single-player vehicle updates now use the vehicle's client record when
applying retained Physgun momentum. The prior code read an uninitialized
client pointer before the native vehicle update. Vehicle think and slide
collision validate controller and former-rider data. Fatal impacts log before
and after the damage callback, and stop handling the collision if that
callback removes the entity. The matching Windows crash reports are from
1.217. Current-build gameplay confirmation remains open for both reports.

## Single-player rope tension at off-center attachments

JA:R 1.248 uses the actual tie-point velocity and rotational inertia
when a mixed or wrapped rope pulls a single-player prop. A bend applies
one combined linear and angular impulse per prop, including when two
corners press on that same prop. Both Box3D and native fallback use the
same force directions. Actor controllers and saved rope geometry keep
their existing behavior. Fixtures cover off-center ties, movable bends,
slack, frozen props and saved topology. In-game rope feel remains open.

## Single-player Charge properties on map surfaces

JA:R 1.247 extends the Charge tab to player-planted standalone charges,
including older save records and copies pasted onto map geometry. It
shows arming or detonation status and exposes the same health, blast
damage, blast radius and proximity expiry controls as mounted charges.
Revert checks ownership, mount identity and the applied values. Native
save fields and Duplicate retain edits. In-game selection, detonation
and full save/reload remain open.

## Single-player Duplicate for charges planted on scenery

JA:R 1.246 copies player-planted det packs, tripwires and proximity
mines directly. Paste places them on clear world geometry facing away
from the surface, keeping blast settings, mode, and remaining timers.
The copied tripwire traces its new surroundings, and the stock remote
detonator works on copied packs. Creation Undo removes the copy. Older
native charge records remain eligible, and planted copies use existing
save fields. Charges mounted on Sandbox props still copy with their
assembly. Interactive detonation and full save/reload remain open.

## Single-player Context properties for mounted explosives

JA:R 1.245 adds a Charge tab for player-planted det packs, tripwires and
proximity mines mounted on owned Sandbox props. It shows live arming
or detonation status. Edit health, blast damage and blast radius before
detonation, and set a new expiry on a proximity mine. Zero keeps the
existing expiry. Revert checks the applied fields and mount identity.
An expired previous deadline cannot be restored. Native save fields
and Duplicate carry the edits. In-game selection, detonation and full
save/reload remain unverified.

## Portal camera visibility and single-player vehicle think guards

JA:R 1.244 draws portable camera screens with opaque world surfaces so
the sky pass cannot cover the feed in Rend2. Classic selects eligible
screens independently of shader sort. Camera feeds omit active portable
screens and newly placed Portal Cameras put their linked screen behind
the lens. This applies in single-player and multiplayer. Sky View still
controls the map sky. Interactive camera rendering remains unverified.

Single-player NPC think checks the actor before touching its client and
clears stale former-rider links before reading rider velocity. The
available swoop dump is from 1.217 and identifies the cvar read already
fixed in 1.219. A repeat of the reported impact on 1.244 remains open.

## Single-player Context properties for deployed portable shields

JA:R 1.243 adds a Shield tab for living deployed shields. It shows
raised or lowered state, current health and time left. Edit health,
remaining lifetime, left and right spans, and height. Zero in the time
field keeps the current expiry. Resize checks the rotated hull before
committing, including during friendly passage. Revert checks that the
applied values still match and restores the previous absolute deadline
if it remains in the future. Native save fields and Duplicate retain
the edited values. The in-game UI and full save/reload remain open.

## Single-player Duplicate for deployed portable shields

JA:R 1.242 copies a deployed portable shield directly or as part of a
physically linked assembly. Paste rebuilds its local wall dimensions,
health, active or lowered state, and remaining lifetime and rearm delay.
The copy uses the stock shield collision, friendly passage, effects and
expiry without consuming inventory. Links use the new shield's identity.
A blocked shield rejects the whole paste, and creation Undo removes the
copy with its assembly. Native save fields are unchanged. In-game visuals,
contact and full save/reload remain open.

## Single-player Duplicate for prop-mounted det packs and trip mines

JA:R 1.241 copies native det packs, tripwire mines and proximity mines
planted on Sandbox props. Paste retains each attachment's local pose,
arming state and remaining fuse or expiry time. Active tripwires trace
their new surroundings, and the single-player character can detonate
copied remote packs through the stock weapon. An obstructed attachment
rolls back the entire paste. Creation Undo removes the copied assembly
and its charges. The existing native save and mount records carry the
copies. In-game firing, rendering and full save/reload remain open.

## Sandbox camera monitors and swoop crash follow-up

JA:R 1.240 gives the Portal Surface in Camera mode a fixed feed from its
linked Portal Camera in SP and MP, using a bounded 256×256 render target
in Classic and Rend2. The screen's aspect ratio determines the feed's
field of view. One visible portable panel can use the shared portal view.
Sky View still controls the map sky. Mirror mode still reflects the viewer.
The Windows swoop dump resolves to `NPC_Think` in 1.217. The cvar fix from
1.219 is present in the subsequent installed builds. Visual camera-feed
and current-build swoop impact checks remain open.

## Single-player Sandbox pickup lifetimes

JA:R 1.239 adds a Lifetime tab for eligible menu-spawned dropped pickups.
Set the remaining timer in milliseconds, or enter zero to keep the pickup
until collection or removal. Duplicate preserves the remaining time and
Physgun freeze pauses it. A Give link suspends the remaining time until
unlinked. Native save fields keep the timer. Live collection and full
save/reload remain unverified.

## Single-player pickup quantities

JA:R 1.238 lets Context edit the quantity on eligible spawned health,
armor, ammo, battery and ammunition-bearing weapon pickups. Native
pickup callbacks apply the quantity and retain inventory limits. Zero
uses the stock item amount, or gives a dropped weapon without ammo.
Duplicate and native save fields retain the count. Revert validates
the selected pickup and its applied quantity. Live collection and
interactive save/reload remain unverified.

## Single-player Sandbox actor spawn diagnostics and mission tally

JA:R 1.237 keeps Sandbox NPC spawns and kills out of campaign enemy totals.
The spawn menu distinguishes native allocation, initialization,
visibility, displacement and hull clearance failures. Native spawn
callbacks check actor identity before continuing after scripts and
the first player-state update. Nulldriver passes installed definition
and model admission. A live spawn retest remains open.

## Single-player Item Replicator

JA:R 1.236 keeps the Sandbox Item Replicator selectable between uses.
On activation it displays a stock cup or medpac. After the configured
delay, a living actor can collect it for up to the configured healing
amount. The Replicator tab exposes healing, delay and Enabled with Revert.
Duplicate preserves the item, phase and remaining delay. Older owned
replicators restore to a selectable idle state. Interactive visuals
and full save/reload remain open.

## Single-player Sandbox Secret marker

JA:R 1.235 gives spawned Secret markers independent discovery state.
They use the stock sound and message, can fire a Primary output, and
default to one discovery. Context exposes Repeat, Enabled and Rearm.
Campaign secrets retain their original mission tally. Duplicate and
native save fields retain the marker state. Interactive audio and
full campaign save/reload remain unverified.

## Single-player weapon shooter controls

JA:R 1.234 adds Context controls to the four portable weapon shooters.
Edit primary or alternate fire, single shot or repeat, the shot interval
and random variation, and whether to accept activation. A new aim link
leaves an active repeat scheduled. Duplicate restores a running shooter's
next shot against its copied aim target, including while Physgun-frozen.
Native save fields retain the settings. In-game combat remains unverified.

## Single-player vehicle shield recharge and HUD

JA:R 1.233 applies a vehicle's shield recharge interval to every point,
including after native save restore. Fully depleted shields still need an
external refill. Mounted vehicles display a distinct live shield bar when
they have shields, and animal health uses the mount's capacity. The native
fixture covers interval timing, save data and the capacity limit. Rendered
HUD and live combat remain unverified.

## Single-player vehicle Context resources

JA:R 1.232 adds a Vehicle tab for eligible living, empty vehicles. It edits
native hull armor, shields, gun ammunition and turret ammunition within
the vehicle definition's capacities. Animal mounts also update native
health when hull armor changes. Revert property checks for intervening
damage, shots, recharging and occupancy. Native saves retain the edited
values, and Duplicate captures them. In-game editing, boarding and full
save/reload remain open.

## Single-player Duplicate for empty vehicles

JA:R 1.231 preserves eligible empty vehicles' hull health, armor, shields,
gun and turret ammo, and orientation. Copies begin their own recharge cycle
with fresh controllers. Vehicles with riders, detached surfaces, or an active
destruction sequence are ineligible. Interactive boarding and combat remain
unverified.

## Single-player Duplicate for NPC movement, armor and allegiance

JA:R 1.230 preserves eligible NPCs' armor, teams, custom gravity and
ground friction. A Physgun-frozen copy retains its custom gravity when
released. NPCs that use map gravity calculate it at the new location.
Interactive movement and combat verification remain open.

## Single-player Duplicate for NPC saber loadouts

JA:R 1.229 rebuilds named NPC sabers on paste, including dual sabers,
saved fighting style and per-blade color, dimensions and activation.
A copied saber starts in its owner's hand, even if the source was
throwing it. The pasted NPC owns its own saber helper, which disappears
with Undo or a failed paste. Specialized weapon rigs continue to use
the native spawn loadout. In-game saber combat remains unverified.

## Single-player Duplicate for NPC weapons and ammo

JA:R 1.228 copies the active weapon, carried ordinary handheld weapons,
ammunition and alternate-fire setting of eligible on-foot NPCs. A copied
disarmed NPC starts disarmed. The paste initializes the weapon through
the native model and AI routines and rolls back if the new model cannot
attach. Sabers and specialized weapon rigs continue to use the native
spawn loadout. In-game combat verification remains open.

## Single-player Duplicate for ammo and shield stations

JA:R 1.227 preserves the remaining recharge delay and Use cooldown
when copying a Sandbox station. Pasting keeps the captured charge and
starts an independent countdown without replaying the source's dispensing
sound. Model and floor variants share the same logic. Interactive
dispensing and audio verification remain open.

## Single-player Duplicate for Repeat and Once volumes

JA:R 1.226 retains the remaining delay of queued Repeat and Once
trigger outputs and the reuse cooldown of Repeat triggers. A pasted
trigger fires through its own assembly links using itself as activator.
Once triggers still become spent after firing, and Physgun-frozen
triggers resume their captured delay when released. Hurt volumes keep
their settings and enabled state with fresh per-actor contact clocks.
Interactive verification remains open.

## Single-player Duplicate for queued Sandbox logic

JA:R 1.225 preserves the time remaining on queued Delay and Relay
outputs, along with a Relay's cooldown. The copied controllers use
themselves as activators for captured work and resolve the new assembly's
links. Undo removes the copied controllers and their queued actions.
Interactive verification remains open.

## Single-player Duplicate for Sandbox timers

JA:R 1.224 preserves a running Sandbox Timer's remaining delay when
duplicating it. A pasted Timer fires only through the copied links and
uses itself as the activator until someone restarts it. Stopped timers
stay stopped. Muted timers keep cycling silently, and Physgun-frozen
timers keep their countdown paused until released. Undo removes the
copied assembly. Interactive verification remains open.

## Single-player Duplicate for Sandbox speakers

JA:R 1.223 keeps running loops and active or paused repeats when a
Sandbox speaker is duplicated by itself or through a linked assembly.
Repeat deadlines start from the time remaining when the assembly was
copied. Pasting a speaker emits no immediate sound event. The copied
speaker uses its own playback controls, and creation Undo removes it
with the rest of its assembly. Audible gameplay remains unverified.

## Sandbox camera screens and authored mirrors

JA:R 1.222 gives a visible Sandbox Portal Surface first use of the shared
portal view in Classic and Rend2, in single-player and multiplayer.
With all panels offscreen, disabled, or disconnected, the renderer can
use the view for an authored mirror. The panel material leaves the
camera image intact with a destination-only blend and still writes
depth. Single-player labels the Sky View and the linked Portal Surface
and Camera cards separately. Matching the 1.217 crash reports to the
installed code confirms that 1.221 already includes the 1.219 NPC cvar
fix for the reported swoop crash. Interactive verification remains open.

## Single-player Duplicate for linked weapon shooters

JA:R 1.221 resolves a Sandbox weapon shooter's linked target when it
fires. A pasted shooter uses its copied target on the first shot, and
repeat fire tracks the target as it moves. A missing target clears the
old grenade aim point and uses the shooter's facing. The native grenade
fixture exercises first-shot, repeat, and target-slot reuse behavior.
Interactive verification remains open.

## Single-player Duplicate for portal pairs

JA:R 1.220 creates copied Portal Surfaces and Portal Cameras at their
recorded positions. A wall beside either member can block the default
companion position without preventing paste. Links resolve to the new
members, and grouped Undo removes the pasted assembly. The spawn menu
still creates a linked pair. Interactive verification remains open.

## Single-player swoop crash and portable camera panels

Two Windows reports from 1.217 resolve to the cached AI cvar read in
`NPC_Think` after a Physgun release. JA:R 1.219 queries the setting through
the engine, where a missing setting reads as disabled. New portable panels
place their paired camera on the visible side and stop short of scenery.
Classic and Rend2 use a panel material without the retail portal's distance
fade. The Sky View marker controls map sky surfaces. A Portal Surface is
the screen linked to a Portal Camera. Interactive verification remains open.

## Single-player Duplicate for map-section ambience

JA:R 1.218 preserves each stock ambient speaker's loop state and
remaining use cooldown when pasting a room. Speakers at different
heights stay independent, including those that share horizontal
coordinates and a sound. The paste uses the destination speaker's
registered stock sound. Retail fixtures check rotated copies, source
state, and sound identity. Interactive listening remains open.

## Single-player Duplicate for map-section hazards

JA:R 1.217 retains each stock room torch's remaining time before its
next effect and damage tick. Stopped torches stay silent. Pasted hurt
triggers retain their active and linked states and remaining cooldown,
with their native damage and rotated bounds. Retail fixtures exercise
Rancor Pit, Yavin and Korriban torches, Taspir's timed damage, and the
fall volumes. Interactive verification remains open.

## Sandbox Undo only removes creations

JA:R 1.216 limits Z, spawn menu Undo and the Remove tool's alternate
fire to the latest live entity or prop spawn, pasted assembly, or physical
Toolgun attachment. Context retains its own Revert controls. Single-player
derives spawn history from ownership when loading pre-V55 saves. A door
that survives its magnetic lock stays in the original setup's Undo entry.
Multiplayer identifies a removed rope, weld or hinge as an attachment in
the confirmation. Regression fixtures cover empty creation history, spawn
order after detachment, and gameplay pickups and map entities in MP.

## Single-player Duplicate for damaged map sections

JA:R 1.215 preserves the remaining health of individual breakable
brushes and holes left by destroyed brushes in Abandoned City and Yavin
Training Grounds. Pasting the room omits destroyed brushes without
replaying destruction effects. Yavin's dust timer resumes from its
captured deadline and fires targets inside the pasted room. Retail BSP
fixtures cover rotated copies, brush damage, entity-slot reuse and timer
isolation. Interactive verification remains open.

## Single-player Duplicate for Imperial Shaft and Bespin Courtyard

JA:R 1.214 copies Imperial Shaft lift travel and return timing, plus
both floor pads' queued actions and cooldowns. Each copied pad resolves
its lift in the new section. Bespin Courtyard's cloud car retains its
running phase or paused position and resume phase. Retail BSP fixtures
exercise rotated copies, independent target dispatch and both cloud-car
states. Interactive verification remains open.

## Single-player Vjun Sentinel state in Duplicate

JA:R 1.213 preserves the Vjun Sentinel lifts' travel phase and remaining
wait, queued floor-pad actions and cooldowns, and pending pickup respawns
when the room is copied. Rift Sanctuary and Korriban FFA pickup respawns
use the same clipboard path. Pasted timers start from the captured
remaining time and act on the new section's own children. The native
fixture checks a rotated paste, moving and raised lifts, pad timing and
an independent pickup respawn. Interactive play remains open.

## Single-player Duplicate for large map sections

JA:R 1.212 accepts the rotated enclosure of a map section in the
clipboard. Several stock BSPs, including Vjun Sentinel and Rift
Sanctuary, have model origins far enough from their rooms to exceed the
ordinary prop bounds. Spawn and Duplicate also verify that the entire
section fits within the world coordinates before creating it. The retail
BSP fixture checks clipboard bounds for all thirteen sections at four
headings. Interactive placement and the copied rooms' gameplay remain
open.

## Single-player spawn clearance in portable map sections

JA:R 1.211 checks the host BSP and linked brush models when placing
spawn-menu objects, setup helpers and Duplicate assemblies. Spawned prop
and NPC hulls can still overlap. NPC and vehicle placement can trace past
an obstructing actor or prop to the floor of a portable room. Duplicate
checks existing scenery before spawning a section and keeps the newly
created section's own brushes out of its enclosure recheck. The retail
catalog has 1,186 cards and zero listed unavailable entries. Interactive
placement and actor verification remain open.

## Single-player portable lift and bobbing platform manipulation

JA:R 1.210 carries supported actors, loose props and vehicles with their
seats when the Physgun, Rotate tool or Context moves a portable lift or
bobbing platform. The same transaction checks carried prop charges. A
blocked rider or cargo leaves the platform and the group in place. The
native lift extension and bobbing phase continue from the edited pose.
Fixtures cover actor rollback and staged cargo, including seated vehicles.
Interactive verification remains open.

## Single-player Context placement of brush movers

JA:R 1.209 uses the native mover push for Context Position, Angles, and
Revert on brush movers and portable spinners and pendulums. Position
requires the full path to clear scenery and carries supported riders.
An obstructed pose leaves the mover and riders in place. A failed
Revert remains available after the path clears. Frozen movers retain
their original path until release, when it shifts to the new position.
Context Revert handles these edits independently of Sandbox Undo.
Interactive verification remains open.

## Single-player Physgun dragging of native movers

JA:R 1.208 routes held brush movers and portable spinners and pendulums
through the native mover push transaction. The mover carries supported
riders and rolls back when a rider is blocked, without crushing damage.
The Physgun measures accepted movement and rebases the paused path once
on release. The Rotate tool uses the same mover collision checks for
brushes. Native tests cover rider rollback, partial movement, path
resumption and portable section relocation. Interactive verification
remains open.

## Single-player Space volume activation in Context

JA:R 1.207 adds separate Vacuum and Enabled buttons to single-player
Space volumes. Enabled controls whether the volume takes effect while
retaining its vacuum setting. Revert property restores the previous
state. The native contents and inactive flag continue to carry the
settings through Duplicate and full saves. Headless tests cover UI
visibility and commands, property Revert, and actor exposure when the
volume is disabled. Interactive verification remains open.

## Exposure in overlapping Space volumes

JA:R 1.206 preserves the current suffocation deadline when an actor
remains inside another Space volume after the current one moves away,
is disabled, or is removed. Both modes check nearby volumes against the
actor's exact position before resetting exposure. Multiplayer also checks
tool permissions on the replacement volume. Native fixtures cover movement
and removal while overlapping. Interactive verification remains open.

## Independent Space volume dimensions in multiplayer

JA:R 1.205 adds Width, Depth and Height to the MP Space volume's Context
panel. The dimensions accept whole units from 8 to 8192. New volumes grow
upward from their marker, and rotating it turns the outline and the server's
trigger together. Earlier saved cubes retain their centered placement and
original size. Context Revert, Duplicate and world saves carry the three
dimensions. Two headless clients verified that lowering the ceiling leaves
gravity unchanged above it, raising the ceiling admits the actor, and Undo
restores normal gravity. Interactive verification remains open.

## Multiplayer Space volume

JA:R 1.204 adds the SP `trigger_space` card to the MP Entities tab.
Its movable gizmo controls an oriented zero-gravity volume. Breathing
actors start taking damage after five seconds. Droids and enclosed riders
retain their respective protections, and the server checks current tool
permissions. MP Context adjusts the cube's half-size and activation state.
SP also supports independent width, depth and height. MP Duplicate and
world saves reconstruct the sensor. Fixtures cover orientation, exposure,
permissions, Undo and restoration. Two connected headless clients saw
zero gravity only inside the volume and normal gravity return after Undo.
Interactive verification remains open.

## Multiplayer cleanup of shared thruster mounts

JA:R 1.203 includes a player's thruster mounts in personal Despawn All,
including mounts on another builder's props. Creation Undo can still
remove the player's own mount if shared editing has since been disabled.
Both actions preserve the host prop and mounts created by other players.
An occupied owned setup blocks Despawn All before it removes any mounts.
The multiplayer feedback fixture covers these cases. Interactive
verification remains open.

## Single-player Despawn All preflight

JA:R 1.202 checks all live Workshop entities, including owned helpers,
before Despawn All releases holds, tears down attachments or removes
objects. If an actor occupies a setup or stands on a portable section,
the player can move clear and retry with the build intact. The duplicate
fixture covers an occupied descendant and a successful preflight after
it clears. Interactive verification remains open.

## Multiplayer map anchors at full prop capacity

JA:R 1.201 uses the shared Box3D world anchor for multiplayer ropes,
welds and hinges. Each attachment keeps its own clicked frame, and
the static anchor uses no prop slot. The MP adapter fixture fills all
4,096 rigid prop slots, then registers each of the three world-anchored
attachment types. Interactive stress performance remains open.

## Single-player Box3D ropes attached to the map

JA:R 1.200 simulates straight map-anchored prop ropes in Box3D. The
constraint uses the clicked anchor and the saved rope length. Ropes
with bends continue to apply tension along their routed path, and an
obstructed route suspends tension. Fixtures cover both endpoint orders,
the hanging length, obstruction and recovery, and attachment removal.
Rendered gameplay verification remains open.

## Single-player Box3D welds and hinges attached to the map

JA:R 1.199 runs world-anchored welds and hinges in Box3D when the
connected props are simulated there. Separate builds can share map
geometry as an anchor without joining each other's solver components.
Mixed builds and Box3D-off mode use the native solver. The existing
attachment frames and IDs still drive save restoration and removal.

## Protecting actors during map-section removal

JA:R 1.198 keeps portable map sections in place while a player or NPC
stands on the root or an owned solid brush, or a vehicle is landed on
either. Remove, creation Undo, Despawn All and fall cleanup check the
support before teardown. MP server world clear and disconnect cleanup
also preserve sections supporting actors who remain in the world. A
player can leave the section and retry removal. Interactive verification
remains open.

## Duplicating assemblies attached to scenery

JA:R 1.197 preserves ropes, welds and hinges attached to static map
surfaces in SP and MP. The clipboard retains the attachment frame and
surface normal. Paste translates the assembly onto matching scenery,
checks the new anchors and reroutes ropes before committing. A failed
anchor or route leaves the clipboard and existing objects available.
Undo removes the pasted members and their attachments together.
Attachments to outside live entities retain their existing copy behavior.

## Falling objects and live Sandbox counts

JA:R 1.196 removes falling single-player pickup drops independently of
their owned source. The pickup retires from the ownership registry while
its source remains available for creation Undo. Multiplayer refreshes
personal spawn and NPC/vehicle counts and the server prop count after
world changes, including automatic fall cleanup. Rope and joint helpers
use the creation history and do not count as entity spawns.

## Creation Undo and Context feedback

JA:R 1.195 aligns the multiplayer creation Undo sound and top-left
confirmation with single-player. Z, Q-menu Undo and the Remove tool's
alternate fire remove the newest live spawn, paste or physical attachment.
Context calls its separate actions Revert property and Revert pose.

## Single-player creation Undo after reload

JA:R 1.194 stores the live order of entity spawns, Duplicate groups and
Toolgun attachments in full single-player saves. Restoring a save resolves
each record through stable ownership and attachment IDs. Z can remove a
later attachment before undoing an older pasted group. Context property
revert history remains session-local. Earlier saves still load with the
existing spawn fallback. The V55 save writer and reader are covered by
fixtures for full saves, autosaves, invalid IDs, attachment order,
occupied-group retry, and whole-group removal. In-game reload remains open.

## Multiplayer pickup catalog IDs

JA:R 1.193 exposes Bacta Canister, Binoculars and Portable Shield under
their SP catalog IDs in the MP Entities tab. The cards use stock MP art
and native MP holdable behavior. The previous MP IDs still spawn and
restore through the existing world format. Searching an old class name
finds the corresponding card. Automated checks cover both sets of IDs,
native item admission, server disable rules, Duplicate, save and restore,
creation Undo, and the menu search route. In-game use remains unverified.

## Multiplayer static brush and brush group

JA:R 1.192 adds `func_static` and `func_group` to the MP Entities tab with
their SP catalog IDs and the stock crate model. They spawn frozen and
solid. Use or the Activate tool fires their Primary link, which can be
disabled in Context. Duplicate, world saves and creation Undo use the
existing owned-object and link formats. Multiplayer now has 1,137 catalog
entries. The catalog-ID comparison still has 41 SP entity cards and 28 SP
item cards without matching MP cards. Some item names already have MP
counterparts, and others require native weapon or pickup support.

## Single-player vehicle freeze across saves

JA:R 1.191 keeps seated NPCs under the frozen or held vehicle's control
while their native seat links are valid. Riders need no independent
freeze records. Full saves use the existing native pose and freeze
records, so a loaded frozen vehicle continues controlling its riders.
Freeze refuses a vehicle that is boarding or has unsettled riders.
An invalidated freeze releases the hull without throwing it. Automated
checks cover native vehicle steering serialization, frozen control
restoration, broken seat links and release. In-game save/load remains open.

## Single-player vehicle placement and property revert

JA:R 1.190 applies Context position and angle edits to a frozen vehicle
and its seated actors together. Revert checks the previous pose for
clearance and refuses to overwrite a later vehicle move. The Rotate
tool also checks the hull and seats along their movement paths. Select
the hull to move an attached rider. Automated checks cover blocked edits,
blocked revert and retry, rider position, steering, link state and the
vehicle's frozen state. In-game verification remains open.

## Multiplayer Physgun vehicle handling

JA:R 1.189 checks the native vehicle hull, pilot, passengers and droid
before a Physgun grab, translation or rotation. The server uses the same
seat destinations as Sandbox vehicle transport. A blocked seat stops
the attempted move. Clearance checks preserve the group's link state,
and accepted movement feeds the existing release momentum. The server
fixtures cover missing seat bolts, a blockage at each member, off-center
rotation, linked and unlinked riders, and throw velocity. Human multiplayer
gameplay confirmation remains open.

## Single-player Physgun vehicle handling

JA:R 1.188 moves and rotates a settled native vehicle together with its
pilot and droid. The Physgun uses the vehicle's seat attachment points and
checks each member for clearance before moving the group. It updates the
native steering orientation and carries horizontal throw momentum into
the native vehicle controller. To move a seated NPC, aim at the vehicle.
An invalidated hold releases the vehicle without throwing it. Automated
checks cover blocked movement, missing seat attachments, position and
steering synchronization, and throw velocity. In-game confirmation is open.

## Single-player Physgun rotation input

JA:R 1.187 uses the tilted camera axes for manual USE rotation around
the clicked point. Normal holding still follows camera yaw. A USE press
consumed by the Physgun stays consumed through release, freeze, or weapon
switching until the player lets go of USE. Tests cover tilted pivots,
off-center grip placement, freeze input and the next ordinary USE press.
In-game confirmation remains open.

## Single-player Toolgun world anchors

JA:R 1.186 lets Weld and Hinge use bare world surfaces for either
anchor. Rope, Weld, Hinge and Thruster share the Toolgun shot trace.
A hit on a selectable controller part uses its parent entity and the
clicked surface. Fixtures cover a world anchor on either click, alternate
fire cancellation, parent selection and thruster removal. Interactive
confirmation remains open.

## Single-player rope routing and rendering

JA:R 1.185 invalidates cached rope routes when a Sandbox prop rotates
or changes model frame, including poses with identical outer bounds.
Ropes reuse their visible mesh while the route and the objects around
its sag remain unchanged. A moving obstacle can still update the visible
curve even when the physical rope's bends stay the same. Fixtures cover
both pose changes, mesh reuse without collision sweeps, and sag after a
floor moves out of the way. Rendered gameplay and frame-time measurements
remain open.

## Single-player removal and Undo feedback

JA:R 1.184 checks a spawn and all its owned descendants for occupied
vehicles or controlled actors before Remove, Undo, fall cleanup or
Despawn All can tear down that group. Undo of a pasted assembly also
checks its owned descendants, including helpers created after the paste.
An occupied group stays available for retry. A successful Undo plays
the UI sound and shows its result above the usual tool hint.

## Sandbox Undo for creations

JA:R 1.183 limits global Undo in SP and MP to spawned entities and props,
pasted assemblies, and created ropes, welds, hinges and thruster mounts.
The newest surviving creation goes first. Rotate, Freeze, Gravity,
activation, controller rewiring, Unlink and thruster toggles or removal
leave the creation history alone. Context keeps its separate property
revert action.

## Single-player Thruster Undo

JA:R 1.182 makes the single-player Undo command reverse Thruster toggles
and alternate-fire removal before reaching older spawn actions. A
restored mount keeps its original ID, force, direction and on/off state.
Undo retries when a conflicting mount or attachment capacity prevents
restoration. Entity deletion, Sandbox shutdown and Despawn All retire
stale records. This brings the common Thruster actions in line with MP.

## Thruster direction in single-player and multiplayer

JA:R 1.181 adds a direction selector under **Q → Thruster**. The default
pushes into the mounted surface. Click the selector to push away from it.
New mounts retain their chosen direction through toggles, rotation,
Duplicate and saves. The visible exhaust points opposite the applied
force. The mount's existing local direction stores this setting, so
existing saves and the multiplayer snapshot format remain compatible.

## Sandbox Undo, NPC cards and Classic multiplayer startup

JA:R 1.180 scopes the Sandbox Undo hotkey and spawn menu button to
spawned objects, pasted assemblies and reversible Toolgun operations.
Physgun moves and ordinary Use bypass this history. The Context panel
uses a separate Revert property action for the selected entity. MP now
also tracks Rotate, Freeze, Gravity, Thruster and Link actions in order
with spawns and created constraints. Destructive removal and arbitrary
controller activation cannot generally be reversed.

The Entities tab hides NPC spawner aliases that duplicate direct cards
in NPCs. Both modes retain their original IDs for console requests and
saved worlds. Vehicle spawner cards remain available in Entities.

Classic multiplayer aligns the renderer's command buffer so the backend
reads the first frame command at the address used by the frontend.
An isolated startup with the owner's copied 2560×1440 settings reproduced
the stuck splash before the fix and rendered the menu with the rebuilt
renderer.

## Multiplayer console NPCs and placement

JA:R 1.179 counts native `npc spawn` commands in multiplayer Sandbox
when the server enables cheats. Undo and Despawn All use the player's
existing ownership rules. Catalog NPCs and vehicles also participate
in the existing Duplicate and world journal paths. A native NPC without
a catalog definition can be counted and removed during the session.
Sandbox NPCs can overlap at placement without telefragging. The same
placement rule applies to NPCs restored by Duplicate or the world
journal. Menu and Duplicate placement still check map solids and player hulls.

## Sandbox spawn count and Undo

JA:R 1.178 binds Z to Undo while the single-player Sandbox is enabled
and restores the previous binding when it is disabled. The count and
Despawn All include objects placed through the menu, Duplicate, and
the single-player `npc spawn` and `spawn` console commands while
Sandbox is enabled. Spawn Undo retains the history of live objects
and removes a pasted assembly as one action. The SP and MP NPC and
vehicle quotas are removed. The engine still reserves entity slots for
mission-critical allocations and the Box3D prop budget is 4,096.
Single-player menu and Duplicate placement may overlap other spawned
objects while retaining world-geometry checks.

## Spinner and Pendulum in multiplayer

JA:R 1.177 adds both stock-crate movers to **Q → Entities**. They share
single-player's defaults, local axes and motion step with multiplayer.
Context edits the spinner's signed rate and the pendulum's amplitude
and cycle, and toggles either mover. Use or a linked controller can
start or pause them. They pause while held or released from freeze.
The server sweeps the deck and supported bodies as a group. It keeps
their original poses if a body is obstructed or a rider has opted out
of player tools. Duplicate and the world journal preserve the current
angle, pendulum phase and settings. Graphical and remote gameplay
acceptance remain open.

## Portable portals in multiplayer

JA:R 1.176 adds Portal surface and Portal camera to the Entities tab.
Each spawns a linked pair. A panel can show the linked camera, mirror
the room, or turn off. The camera can follow a Primary aim target or
use its own angles. Context edits panel dimensions, mode, power and
enabled state. The server uses the camera for snapshot visibility
and keeps settings and links in Duplicate and world saves. Classic and
Rend2 share the single-player panel geometry and rendering code.
Each rendered view has the engine's existing one-portal limit. Visual
and remote gameplay acceptance remain open.

## Multiplayer map browser

JA:R 1.175 lists installed BSPs in the multiplayer host browser. Arena
files can supply titles. The browser uses levelshots and the shared SP
thumbnail generator, inspecting one visible map per refresh. It reads
the BSP's entity lump to identify an FFA start that can accept a human
player. Both a card selection and a manually typed map go through that
check before server settings are applied. This checks the start entity
and its FFA flags. Map scripts and in-game rendering remain separate
checks.

## Searchlight in multiplayer

JA:R 1.174 adds the Searchlight to multiplayer's Entities tab. It uses
the stock Imperial camera housing and cone material. The server clips
the beam against the scene and checks each player's line of sight and
tool consent. The Link tool connects an aim target and a detection
output. Context edits power, tracking, range, cone angle, RGB color and
impact light. Duplication and world saves preserve those settings,
links and the remaining detection cooldown.

## Directional trigger in multiplayer

JA:R 1.173 adds the single-player directional trigger to multiplayer.
Its two linked outputs fire when an eligible actor crosses the local
center plane in the corresponding direction. The server checks the full
movement segment, rejects teleports, and honors tool permissions. The
trigger spawns with two feedback markers. Context shows the rotated
volume and forward direction, and edits size, cooldown, use limit,
subject filter and armed state. Copies and world saves retain the
remaining uses and cooldown. In-game rendering and remote play remain
open.

## Actor movement controls in multiplayer

JA:R 1.172 adds the single-player **Gravity change** and **Friction
change** cards to multiplayer. Edit their values and enabled state in
Context, then Activate them or link them as outputs. Each marker acts on
an eligible on-foot activator and honors server tool permissions. The
Movement tab edits an eligible player or NPC directly and offers Undo and
native defaults. Clients receive the movement settings in their normal
player state for prediction. Duplication and JRW24 save edited NPC settings.
Player overrides last until respawn. Interactive movement and remote
play remain open.

## Yavin Training Grounds in multiplayer

JA:R 1.171 adds Yavin Training Grounds to multiplayer Sandbox under
**Q → Map Sections**. The retail BSP supplies the 16 entry points, 30
saber-only breakable brushes, 51 scenery models, 16 damage torches and
74 aimed dust emitters. The original timer chooses an emitter every
two to eight seconds. Classic and Rend2 use Yavin's detached backdrop
from the stock camera inside the playable area. JRW23 preserves brush
damage and the time until the next dust pulse.

Multiplayer now offers all thirteen single-player Map Sections. Both
converted Garry's Mod maps are full maps available to multiplayer hosts.
Interactive section rendering and remote multiplayer gameplay remain open.

## Korriban FFA in multiplayer

JA:R 1.170 adds Korriban FFA to the multiplayer Map Sections tab. Its
17 entry points, eight fall volumes, 47 respawning pickups, 28 torches,
seven ambient winds and local fog use the retail map's authored data.
The world journal preserves pickup timers. Multiplayer now has twelve
of the thirteen single-player sections, with Yavin Training Grounds and
interactive verification still open.

## Imperial Shaft in multiplayer

JA:R 1.169 adds Imperial Shaft to the multiplayer Map Sections tab with its
two original lift and floor-trigger pairs, fall volume, three ambient loops
and six scaled scenery models. Multiplayer now has eleven of the thirteen
single-player sections. Korriban FFA and Yavin Training Grounds remain open.
The multiplayer journal also resumes Shaft lift and pad timing and Rift
pickup respawn timers. Live graphical verification remains open.

## Rift Sanctuary in multiplayer

JA:R 1.168 adds Rift Sanctuary to the multiplayer Map Sections tab with
its stock levelshot, 16 entries, 38 respawning pickups, and both fatal
fall triggers. Classic and Rend2 use room-local fog and the multiplayer
client emits cold breath inside the playable BSP area. Multiplayer now
has ten of the thirteen single-player sections. Imperial Shaft,
Korriban FFA, and Yavin Training Grounds are still awaiting their ports.

## Hoth Canyon in multiplayer

JA:R 1.167 adds Hoth Canyon to the multiplayer Map Sections tab with its
stock levelshot, 14 entry points, eight wind emitters, and fatal fall
volume. Classic and Rend2 use the stock detached backdrop inside the
canyon's playable BSP area. Its broader placement range accommodates the
stock model bounds. The multiplayer catalog covers nine of thirteen
single-player sections. Interactive rendering and play remain open.

## Multiplayer Vjun Sentinel world saves

JA:R 1.166 resumes Sentinel's lift movement and upper-stop waits, queued
floor pads and their cooldowns, and collected pickups' respawn timers.
JRW20 stores this state for each room. Older journals load with stock
child state. Duplicating a room creates a fresh set of lifts and pickups.
Interactive play and validation of the five remaining multiplayer Map
Sections are still open.

## Multiplayer Vjun Sentinel

JA:R 1.165 adds Vjun Sentinel to multiplayer's Map Sections tab, with the
stock lift groups, floor pads, fall trigger, pickups and ambient sounds.
The multiplayer section catalog now covers eight of the thirteen
single-player sections. See the validation record for dedicated-server
coverage and remaining interactive checks.

## Overlapping Sandbox placement in multiplayer

JA:R 1.164 brings multiplayer spawn, Duplicate and world restore in line
with single-player placement around overlapping prop hulls. The server
continues to check the host BSP, portable sections and native actor hulls.
The Construct test checks an overlap with an authored sofa and rejection
by a solid floor brush. Interactive placement remains open.

## Sandbox cleanup below the map

JA:R 1.163 brings the single-player fall boundary to multiplayer. The
server uses the loaded BSP and linked portable section bounds. A fallen
Sandbox object releases its physics and tool references through native
entity cleanup. Native movement, Box3D and occupied-vehicle handling pass
the automated fixtures. Interactive cleanup remains to be checked.

## Construct and Flatgrass multiplayer support

The 1.175 installation was also checked through the native dedicated
server for Construct's Color Room. A player's USE command switched
all three channels of baked light style 32 off and back on. A nearby
spectating client received both updates and one positional sound
event per press. A third client authenticated while the lights were
off and received that state in its initial gamestate. It received the
next switch sound without replaying the earlier click. Both original
clients received the map-authored mirror at its original coordinates.
The map pack check also covers the switch sound, 174 switched faces
and mirror shader. In-game appearance and listening remain open.
Evidence is in `build/construct-room-mp-1.175.0/qualification.json`.

JA:R 1.157 stages the local converted maps in both game modes. The MP
host browser uses the native FFA arena list and the original map previews.
Both Rend2 renderers use the same skybox face batching. The owner
confirmed that Construct loads in single-player on 1.156. JA:R 1.158
shares Construct's soundscape parser, mixes and fades with MP. Native MP
world traces match all 19 zones. JA:R 1.159 gives Construct's four map props
their authored starting hulls and masses in MP. Both loopback clients
received the furniture and observed the Physgun lift. Interactive
multiplayer play and listening remain open.
JA:R 1.160 adds multiplayer climbing through the imported ladder brush.
Shared server and client movement supports ascent, descent, stopping in
place and stepping away at the base. The stock humanoid uses its hanging
pose because its animation set lacks ladder clips. JA:R 1.161 uses a
swept crouch hull at the upper lip and a short ladder volume along the
Source dismount path. The pure-server test confirms players can climb
through the opening and exit onto the upper passage. The 1.162 test also
drives a return from the upper passage to the lower ladder and verifies
the observer sees the descent. SP now uses the same stock hanging pose
when ladder clips are absent. Interactive play in both modes remains open.
The separate multiplayer test installer stays at 1.155.0
until the owner requests a rebuild.

## Portable Vjun Sentinel and Map Sections tab

In 1.155.0, the single-player Sandbox can spawn Vjun Sentinel from the retail
`mp/ffa1` BSP. The original lifts, floor pads, fall trigger and respawning
pickups belong to the spawned instance. Thirteen portable sections are now
in the single-player Map Sections tab. The eight sections supported by MP
appear in the same tab there. Each uses its original map levelshot. See the
validation record for fixture coverage and the remaining interactive checks.

## Portable Rift Sanctuary cold breath

In 1.149, the single-player portable Rift Sanctuary uses its retail cold
breath effect within the playable BSP area. The character's position is
transformed into each visible room instance's local coordinates. Stock
resource identity and the BSP leaf bound the effect, including after a
room is moved or rotated. The host map keeps its own breath setting.
In 1.150, Classic and Rend2 also use Rift's native fog for characters
and the camera inside the playable area. Cross-boundary fog, interactive
verification and full in-game save/load remain open.

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

The stock lift replaces the single-player func_plat placeholder and adds the
card in multiplayer. Each movement step checks the deck, on-foot riders and
loose cargo together. A blocked route leaves all of them in place until clear.
Both endpoint links, travel state and auto-return survive saves and copies.

## 1.86.0 Sandbox Imperial button (2026-11-11)

The stock momentary Imperial switch replaces the single-player func_button
placeholder and adds the card in multiplayer. Both modes support USE and
Activate, two endpoint links, Context hold and activation settings, Undo,
duplicate and saved phase. The other brush-backed map movers still have their
own Sandbox work.

## 1.85.0 Sandbox sliding door parity (2026-11-11)

Single-player Sandbox doors use the stock Factory panel and a swept solid
hull. Context > Door edits travel, speed, direction and auto-close. Native
saves and assembly copies preserve motion progress, target state, remaining
wait and output links. Multiplayer clears the motor loop if travel is set to
the current extension.

## 1.84.0 Multiplayer sliding door (2026-11-11)

The MP `func_door` uses a stock Factory panel and sounds, server-side
obstruction sweeps, four configurable directions, speed, travel and
auto-close. Link supports Opened and Closed outputs. Clipboard and
world journals preserve position, target state and remaining delay.
The SP door's stock-panel appearance and matching Context controls
remain open.

## 1.83.0 Portable wall (2026-11-11)

Both modes can spawn and move the stock wall panel, toggle its collision,
and select the gizmo while it is off. Obstructions refuse reactivation
and retain Context Undo for retry. The original SP campaign brush path
is retained. Earlier SP saved walls retain their existing model and size.

## 1.82.0 Multiplayer distance trigger (2026-11-11)

The multiplayer `trigger_entdist` has the SP distance settings and all
four Link roles. Player mode checks living players on the server.
The activation consumes one check. A missing required input leaves the
trigger armed so it can be relinked. Clipboard and world saves translate
the two inputs independently of the two activation outputs.

## 1.81.0 Entity-distance trigger (2026-11-11)

The single-player `trigger_entdist` has configurable range, Player/NPC/Either,
Inside/Outside links, a movable reference, and a linked NPC subject.
It disarms after a complete check and supports Context rearming, Undo,
and assembly duplication. Authored mission triggers keep their stock
behavior. Multiplayer parity and graphical/full-save checks remain open.

## 1.80.0 Visibility trigger (2026-11-11)

Both modes support editable, reusable `trigger_visible` markers with a
Primary link. They start armed and keep their settings and armed state on
duplication. Authored SP mission triggers keep their original behavior.

## 1.79.0 Player-start markers (2026-11-11)

Both stock start IDs are directly usable through safe native on-foot entry
and teleporter links in SP and MP. SP markers are named destinations and
retain the authored campaign spawn path. MP start eligibility is off until
enabled in Context and applies to frozen, clear markers in Sandbox FFA and
Team Deathmatch. Labels, positions and MP eligibility persist. Graphical
and SP full-save gameplay acceptance remain open.

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

## 1.78.0 Map logic gizmos (2026-11-11)

Stock-texture billboards replace the green crate presentation of normally
invisible owned map logic, including supported trigger roots, target points,
timers, lights and their eligible helper markers. Native effects, world
occlusion, selection, collision, authored geometry and physical crate props
keep their existing paths. SP also recognizes markers from older saves.
Visual confirmation in both renderers remains open.

## 1.77.0 Portable one-shot trigger (2026-11-11)

The owned `trigger_always` survives its initial pulse in SP and is spawnable
with its stock card and feedback helper in MP. Activate rearms it, Context
edits the delay, and an armed copy or save retains the remaining time. An
already fired copy stays idle. Map-authored one-shots still fire and remove
themselves. Graphical and SP full-save gameplay confirmation remain open.

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

SP Context edits the label and three independent local dimensions of an owned
`trigger_location`. The oriented zone recognizes entity hull contact, can be
rotated, and retains its dimensions through Undo, Duplicate and the native
save fields. Legacy saves restore the original footprint. Authored map
triggers continue to use native brush contact. MP team chat and the team
overlay now use player hull contact with the existing owned volume bounds.
In-game menu layout and contact feel still need owner confirmation.

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

## Owner feedback disposition (questions 1-20)

| Question | Owner evidence / current disposition |
|---|---|
| 1 | LAN textures and HUD accepted. |
| 2 | Other clients joining and shared state still untested. |
| 3 | Held-Q open/release accepted; field pinning added for typing. |
| 4 | Typing was blocked by hold requirement; 0.91 fixes field pinning in both modes. |
| 5 | Both menus look good; appearance accepted. |
| 6 | All tested weapon equip/drop actions accepted. |
| 7 | Tool models, beams and sounds accepted. |
| 8 | Freeze/release gameplay still untested. |
| 9 | Wheel/flinging accepted; vertical rotation inverted in 0.91. |
| 10 | Physics accepted on LAN; other-client behavior untested. |
| 11 | Individual Select/Rotate/Gravity/Remove/Unlink tools still untested. |
| 12 | MP rope slack/floor rendering reported broken; 0.91 arc-length/terrain fix needs visual retest. |
| 13 | Thrusters reversed and stock housing added in both modes; needs gameplay retest. |
| 14 | Duplication accepted; Undo sound and top-left feedback added. |
| 15 | C text entry fixed; script slots relabeled/explained. Broader specialized MP properties remain open. |
| 16 | Desann/Mon Mothma/RebornBoss/Morgan accepted. Nulldriver class alias repaired; spawn retest pending. |
| 17 | Noghri attacks accepted. Remaining saber effect-pack screen flash removed in SP/MP; visual retest pending. |
| 18 | Shared editing is default; host can choose owned-only. Saved-world editing is disabled for owned-only hosts. |
| 19 | Retention and auto save/restore default on; host master toggle added. |
| 20 | Native MP respawn resets loadout/health; tools, build and tool settings retained. Hook fixture passes; actual death/respawn still needs testing. |

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

## Authoritative requirement checklist

| ID | Requirement | Current disposition |
| --- | --- | --- |
| CAP-01 | 16x SP capacity and controlled exhaustion | Implemented; owner accepted (1). No invented stress benchmark. |
| CAP-OWNERSHIP | Responsive ownership lookups in large single-player Sandbox scenes | 1.291 indexes entity slots and searches ordered ownership IDs, with duplicate checks, slot reuse protection and transactional restore. The native 16,382-record state fixture and lifecycle tests passed. Representative gameplay frame times and saturation remain open. |
| CAP-UNDO-HISTORY | Prune single-player Undo history efficiently when entities are removed | 1.296 indexes the creation and Context history by entity slot and preserves chronological save order. Native fixtures retire a full slot table and check partial groups, reused slots, the property-history limit and callbacks that remove earlier records during Revert. In-game cleanup timing remains open. |
| TOOL-DUP-BATCH | Paste and undo large single-player assemblies as one operation | 1.292 publishes one result, reuses each copied parent's native helpers, and rolls back uncommitted copies without touching earlier Undo history. The 72-object batch, occupied-copy rollback and native allocation fixtures passed. Interactive latency and placement acceptance remain open. |
| SB-HELPER-REMOVAL | Remove large single-player ownership groups while protecting occupied native entities | 1.293 indexes each parent's helpers for removal, grouped Undo and Duplicate. Older save ordering, orphan records, detach and reused slots have native coverage. Despawn All continues to check occupancy before cleanup. Interactive timing and gameplay remain open. |
| GFX-01 | Renderer/graphics improvements | Implemented; owner accepted (2). 1.37 exposes fifteen native classic graphics controls in MP using shared SP choice data; transaction fixtures and source audits pass. Display confirmation, MP Rend2, rendered/performance acceptance and broader parity remain open. |
| AI-01 | Optional distant/inactive AI optimization | Implemented; owner accepted (3). |
| VIDEO-01 | Custom resolution, automatic r_mode, desktop borderless | Implemented; owner accepted (4). |
| VIDEO-FOCUS | Restore brightness after Alt-Tab | 1.265 reapplies the cached hardware gamma ramp after a focused frame is presented and once after the display has had time to settle. Shared by Classic and Rend2 in SP and MP. The scheduling fixture passed. Interactive brightness confirmation remains open. |
| UI-STYLE | UI/text/console customization | Implemented; owner accepted (5). 1.34 adds MP Console settings with shared SP palette/choices, native opacity/size/height and cross-page Apply/Revert. Native menu fixtures and source audits pass; rendered acceptance and broader interface/settings parity remain open. 1.35 adds MP Interface settings, shared font/UI choices/policy and byte-identical assets; native fallback/menu fixtures pass. Rendered and pure-server acceptance, UI motion/HUD and broader settings parity remain open. 1.36 adds MP Academy/Simple HUD selection and shared SP/MP Reduced/Normal UI motion; native menu/renderer fixtures and source audits pass. Rendered HUD/motion/reload and broader parity acceptance remain open. 1.37 exposes fifteen native classic graphics controls in MP using shared SP choice data; transaction fixtures and source audits pass. Display confirmation, MP Rend2, rendered/performance acceptance and broader parity remain open. |
| HUD-01 | HUD choices | Implemented; owner accepted (6). 1.36 adds MP Academy/Simple HUD selection and shared SP/MP Reduced/Normal UI motion; native menu/renderer fixtures and source audits pass. Rendered HUD/motion/reload and broader parity acceptance remain open. |
| SABER-01 | Selectable saber textures/sounds and combat effects | Implemented; owner accepted (7). |
| UI-04 | Consistent native menus and loading descriptions | Implemented; owner accepted (8). |
| MELEE-01/02 | More melee weapons and carried choices | Implemented; owner accepted (9). |
| WEAP-CARRIED | Weapons-tab selection, firing and visible models for single-player mounted-weapon cards | 1.289 gives six cards an on-foot aim and obstructed-muzzle path and adds native turret bolt firing. 1.290 routes the probe laser through the same on-foot aim and uses stock first-person and world models for all seven empty-model weapons, including older Sandbox drops and saved hand models. Native mounts and custom model paths retain precedence. Native and asset checks passed. Rendered combat and full interactive save/reload remain open. |
| MELEE-03/FISTS-01 | Faster saber extension and repaired fists | Implemented; owner accepted (10). SP acceptance is retained. MP 1.41 adds the same Quick/Legacy and Continuous/Legacy choices, shared ignition/admission policy and native animation-duration punches; per-client userinfo/server/prediction routes are implemented. Automated native timer/menu/guard fixtures pass; remote-client combat and visible ignition acceptance remain open. |
| CAM-01 | Camera presets | Implemented; owner accepted (11). 1.33 adds shared SP/MP Legacy/Comfort/Crisp camera math and an archived MP selector with Apply/Revert; native view/menu fixtures pass, graphical feel acceptance remains open. SP accepted camera behavior is preserved, broader settings parity remains incomplete. |
| FACE-PRESET | Facial presets and speech sensitivity | Implemented; owner accepted (12). |
| FACE-01 | Sound-correlated facial variation | Implemented; owner accepted (13). |
| SB-01 | Independent Sandbox and held-Q overlay | Implemented; owner accepted (14). |
| PHY-INPUT | Inverted wheel distance, right-click freeze, fling momentum | Implemented; owner accepted (15). |
| PHY-ROT | Physgun left click + bound USE + mouse rotation | Implemented; owner accepted (16). |
| PHY-VEHICLE | Keep native vehicle seats attached during Physgun motion | 1.188 checks the vehicle hull, pilot and droid sweeps as one move in SP and synchronizes the native steering angles. Horizontal throw momentum uses the native push drift. 1.189 adds server-side swept clearance for MP hulls, pilots, passengers and droids during Physgun drag and rotation. Gameplay confirmation remains open in both modes. |
| PHY-RACK-STOCK | Keep suspended pickups with portable weapon and ammo racks | 1.269 checks shelf clearance before single-player Physgun drag and rotation, Toolgun rotation, and Context pose changes and Revert. 1.270 keeps copied pickups suspended through native item physics and carries stock through flings with swept clearance. Duplicate preserves depletion and attachment status. MP already synchronizes shelf poses. Matching the clearance checks in MP and gameplay confirmation remain open. |
| CTX-VEHICLE | Edit native single-player vehicle resources | 1.232 adds bounded hull armor, shields and installed gun/turret ammunition with conflict-aware Revert and native save fields. 1.231 preserves vehicle resources in Duplicate. 1.233 enforces shield recharge intervals and uses live shield values in the mounted HUD. 1.275 supports settled occupied hulls and makes C open the player's own vehicle resources while piloting. In-game editing, HUD, combat and full-save confirmation remain open. |
| CTX-NPC-STATUS | Edit Sandbox NPC allegiance and armor | 1.281 adds editable native team, target team and armor with field-specific Apply and Revert. Native AI drops stale combat targets after allegiance changes. Editing is scoped to living owned NPCs and honors target locks. Native client saves and Duplicate carry the values. In-game combat and full-save confirmation remain open. |
| CTX-NPC-FIRE | Edit an eligible Sandbox NPC's held-weapon ammo and fire mode | 1.282 edits the native ammo pool and alternate-fire setting, with a live weapon check and field-specific Apply and Revert. 1.283 adds a saved manual ammo supply mode and makes unlimited ammo work with native AI refills and charged attacks. Duplicate retains the supply mode. In-game firing and full-save confirmation remain open. |
| CTX-NPC-WEAPON | Equip supported hand-held weapons on living single-player Sandbox NPCs | 1.284 adds a weapon picker, transactional hand-model replacement, stock ammo grants for newly added guns with empty pools, and Context Revert. Native saves and Duplicate retain loadout state. Saber, dual-hand, vehicle and emplaced combat controllers retain their native equipment. Rendered combat and full-save confirmation remain open. |
| CTX-NPC-ORDER | Give a living single-player Sandbox NPC a wait or follow order | 1.285 adds Autonomous, Wait here and Follow player with editable distance, native controller priority, conflict-aware Context Revert and saved/copyable settings. Follow uses native AI and requires Player team alignment. Interactive navigation, combat and full-save confirmation remain open. |
| CTX-NPC-NAVIGATION | Direct a compatible NPC to a portable single-player Nav goal | 1.286 adds Go to goal, Primary Link, live destination status and Context Revert. Uses the native route and goal radius and waits for a disabled or missing destination. V56 saves and assembly copies preserve the typed links. Interactive pathfinding and full-save confirmation remain open. |
| CTX-NPC-PATROL | Direct a compatible NPC through a chain of portable single-player waypoints | 1.287 adds Patrol route, advancement through Primary links, end or loop behavior, current-waypoint status and V57 saved progress. Context Revert restores progress and Duplicate remaps it to the copied chain. Interactive pathfinding and full-save confirmation remain open. |
| CTX-SECRET | Use and edit portable single-player Secret markers | 1.235 adds native feedback, a discovery output, Once/Repeat, rearming and Enabled. Duplicate and native save fields retain discovery state. Campaign tallies keep the authored map's behavior. Interactive sound and full-save confirmation remain open. |
| CTX-REPLICATOR | Usable portable single-player Item Replicator | 1.236 keeps an idle marker, uses stock item models and bounds actor healing. Context edits cycle delay, healing and Enabled. Duplicate retains live phase; older owned replicators migrate to idle. Gameplay and full-save confirmation remain open. |
| CTX-PICKUP-QUANTITY | Edit the amount granted by spawned single-player pickups | 1.238 adds quantity to Properties for eligible armor, health, ammo, batteries and weapons with ammo. Native count persists in copies and saves. Revert checks the pickup definition and latest value. Live collection and full-save confirmation remain open. |
| CTX-PICKUP-LIFETIME | Keep or time eligible menu-spawned single-player drops | 1.239 adds a Lifetime tab, Revert, remaining-time copies and Give link suspension. Native save fields carry the timer and paused duration. Interactive expiry and full-save verification remain open. |
| CTX-DEPLOYED-SHIELD | Edit deployed single-player shield walls | 1.243 adds live phase, health and expiry feedback, bounded health and timer changes, and clearance-checked asymmetric spans and height. Revert checks the applied fields and restores the previous deadline if it is still in the future. Native fields and Duplicate carry the edits. Interactive and full-save verification remain open. |
| CTX-MOUNTED-CHARGE | Edit player-planted single-player explosives | 1.245 adds live arming and detonation status, bounded health and blast editing, and proximity expiry with conflict-aware Revert for charges on owned props. 1.247 extends the tab to standalone charges on map surfaces, including older records and pasted copies. Native save fields and Duplicate retain edits. In-game selection, detonation and full save/reload remain open. |
| CTX-MOVER-POSE | Safely edit and revert the pose of native single-player movers | 1.209 routes brush mover and portable angular mover placement through the native pusher. Translation checks scenery along the path and carries supported riders. Blocked poses roll back, and a blocked Revert can be retried. Native fixtures cover rider clearance, trajectory rebasing and frozen release. Interactive confirmation remains open. |
| PHY-PORTABLE-DECK | Carry riders and cargo during manual single-player lift and bobbing-platform movement | 1.210 uses the native push and staged cargo transaction for Physgun drag, grab-point rotation, Toolgun Rotate and Context pose edits. Supported actors, released props, seated vehicles and mounted prop charges follow an accepted move. 1.271 adds supported Sandbox racks and their shelf pickups to planned vertical and angular cargo. 1.327 uses qualified model hulls for lift and bobbing cargo sweeps, deck rotation arcs and staged prop overlap checks. A native fixture passes an obstacle within a prop box but outside its hull, blocks an obstacle against the body, and rejects a rotation blocked between two clear poses. A blocked step retains the deck and cargo state. 1.328 tests the deck against the cached prop hull before carrying it. Supported overhangs remain eligible, and a diagonal prop whose box alone touches the deck stays in place during lift and rotation. 1.329 preserves the measured vertical gap between the supported part of a prop and the deck when turning or tilting it. The fixture retains a 0.75-unit gap on a sloped overhang through both yaw and pitch. Native and solver clearance still gate the move. Interactive confirmation remains open. |
| PHY-GRAB-POINT | SP and MP Physgun grab at the clicked point and preserve pickup facing | 1.153 adds MD3 surface picking in SP and for ordinary MP props, plus posed Ghoul2 surface picking for eligible actors and props. Other entities use their native collision or controller geometry. USE rotates around the contact. 1.325 passes the multiplayer player view pitch and roll to the shared rotation code, matching the single-player camera-relative axes. The multiplayer command and solver fixture checks a tilted turn, a fixed off-center beam endpoint and an unchanged player view. Ordinary holding follows camera yaw, while looking up lifts the object and reveals its underside. Fixtures cover off-center pickup, empty mesh space, wall occlusion, camera-relative rotation, fling and blocked motion. Interactive confirmation remains open. |
| PHY-PANEL-TURN | Check clearance throughout grab-point turns of model-backed walls, doors and buttons | 1.294 checks short angular sweeps for single-player Physgun and Toolgun turns and the corresponding multiplayer native Rotate path. Tests cover a blocker between clear endpoints and a successful off-center turn. In-game clearance remains unverified. |
| PHY-VEHICLE-TURN | Turn single-player vehicles safely around the clicked point | 1.295 checks the hull, seated NPC pilot and attached droid along the complete Physgun or Toolgun turn. Level turns preserve vertical clearance. The native fixture covers yaw at floor height, a pitch arc blocked only at the pilot's intermediate position, and all-member rollback. In-game riding, turns and full saves remain open. |
| MP-PHY-INPUT | Fixed camera and inverted wheel during MP Physgun holds | 1.132 routes bound USE and mouse motion to the held entity while the camera angles remain fixed. A server-confirmed hold gates the gesture. Native and Box3D use the same accepted input and movement commands preserve the existing wire layout. Default wheel up extends and wheel down retracts through the existing bindings. Interactive client acceptance remains open. |
| SP-POINTER | Repeated Sandbox clicks at a stationary pointer | 1.132 resolves pointer focus on click-down after a refresh. A still-visible page button keeps its focus, and active text fields retain their input handling. Interactive confirmation remains open. |
| PHY-GRIP | Harmless Force-grip-style native holding | Implemented; owner accepted (17). |
| PHY-RATE | Smoother physics | Implemented; owner accepted (18). SP 1.148 scales Box3D workers with awake body count, up to eight on supported CPUs, and reuses the current frame's qualified prop set during proxy selection and pose publication. In three runs of 180 fixed steps with 4,096 initially awake props, the median of the 95th-percentile solver times fell from 10.66 ms to 2.30 ms, with identical final poses and velocities. Full-scene frame times and interactive stress checks remain open. |
| PHY-CONTACT | Mass, actor collisions and player support | Implemented; owner accepted (19). Selectable decorative poses implemented in SP 0.58; their gameplay acceptance remains separate; MP poses are implemented in 0.63-0.65. |
| PHY-SHAPE | Reduce invisible Sandbox prop collision corners | 1.137 derives tighter outer bounds from visible model geometry and caches a convex Box3D hull. 1.138 uses that hull for SP native traces, point contents, and the Box3D actor bridge. 1.139 uses the rotated hull for native translation against the host BSP and linked brush models. Rectangular brushes use full separating axes. Sloped brushes and patches use hull support along their native planes. 1.140 sweeps qualified prop hulls along the rotation for the native Physgun, Rotate and angular motion. Context and Undo check destination geometry and mounted charges. 1.142 checks both convex hulls during native prop-pair sweeps, including edge separation, Context and Undo endpoints, and conservative rotation envelopes. Native drag can back out of an allowed spawn overlap using hull depth. Shapes without a qualified hull use the existing fallback. An offline audit found that 519 of 575 frame-zero retail MD3 hulls still omit indexed vertices by more than 0.125 units. 1.143 uses the intersection of 26 model-derived support planes for Box3D and native traces. The generated hull is checked against its candidate corners before use, with the existing fallback for degenerate models. Production hulls covered 617 poses across 575 retail MD3s at the trace tolerance. 1.326 uses a coverage-checked convex hull of the original model points when the pose has up to 48 distinct points. Larger or unqualified poses retain the support-plane path. SP, MP traces and Box3D share the choice. Comparing 617 retail MD3 poses with 1.325 found 171 hulls over 1 percent smaller, up to 56.1 percent, with zero box fallbacks and all indexed vertices within trace tolerance. Three standalone 4,096-body runs completed 180 steps each using an eight-vertex posed-box hull, replacing the prior 16-vertex support hull for that fixture. Detailed coverage and solver measurements are in build/prop-coverage-1.326.0. A standalone 4,096-body Box3D load ran 180 steps. Retail Ghoul2 poses, interactive collision and live load checks remain open. |
| PHY-SOUND | Prop collision sounds | Implemented; owner accepted (20). |
| CTX-PROP-PHYSICS | Edit mass and gravity on spawned SP props | 1.151 adds a bounded mass override and per-prop gravity to Properties. The controls apply to native and Box3D physics, preserve freeze/hold and momentum, and support Undo, assembly duplication and full saves. Earlier saves default to model mass and map gravity. Interactive verification remains open. |
| PHY-LIMIT | Very high fixed prop limit | Implemented and owner accepted (21). SP and MP cap props at 4,096. SP NPCs and vehicles use the 16,384-slot native pool with its mission reserve. MP capacity expanded in 0.79. Representative load acceptance remains open. |
| SB-FALL | Remove Sandbox objects that fall out of the map | Native cleanup handles fallen props, loose drops, ownership and attached tools, including an occupancy guard. SP 1.263 restricts lower-room fall allowances to overlapping horizontal hulls. 1.273 clears incoming secondary outputs and distance endpoints during removal. 1.280 clips live room bounds to the falling object's column before choosing the allowance, including rotated and tilted rooms. In-game confirmation remains open. |
| TOOL-ALT/FX | Alternate fire, firing-only unique Toolgun effect and sound | Implemented; owner accepted (22). |
| TOOL-UI | Compact tool list, right-side properties, fixed top-left help | Implemented; owner accepted (23). |
| TOOL-DUP | Duplicate | Implemented owned assemblies, helpers, links and supported properties; owner accepted (24). Campaign scripts and native AI start fresh. SP 1.241 preserves mounted charges and their arming and timing. SP 1.242 copies deployed shields alone or with physically linked assemblies, retaining health, active or lowered state and expiry. SP 1.246 pastes standalone player-planted charges on clear world geometry with native arming and timers. SP 1.252 preserves qualified world anchors and mounted thrusters on copyable native map models. SP 1.259 preserves pending Welder pulses and Laser Arm bursts. Existing save records carry their native data. Live firing, visuals and full save/reload remain open. |
| TOOL-THRUST | Thruster | Implemented prop/native force, mover translation and non-actor torque. Owner accepted the original tool (25). The later housing request is implemented in both modes. 1.181 adds selectable force direction for new mounts in SP and MP. 1.251 adds SP Context edits and Revert for force, power and direction on existing mounts. 1.274 applies SP mount force on native vehicle movement for empty and settled occupied hulls, with separate saved drift and retained seat relationships. Interactive verification remains open. |
| TOOL-THRUST-FALLBACK | Make Thruster torque consistent for asymmetric props using native physics | 1.301 computes free-prop torque from local model extents and the world-space bounds center. Unheld fallback rotation keeps that center fixed, sweeps the adjusted body and mounted charges, and retains linear velocity. Linked assemblies use the entity-origin convention of the native constraint solver. Native tests cover centered and off-center thrust, rollback, center-preserving rotation and a world weld. Gameplay verification remains open. |
| SB-PORTAL-MONITORS | Show live portal camera views on the most prominent portable screens | In 1.302, Classic and Rend2 allocate the four distinct camera feeds by clipped viewport coverage. Screens with the same camera pose and aspect ratio share a feed. Mirrors keep a separate view. Offscreen panels and unsuccessful captures leave room for the next candidate. Native renderer fixtures cover both selection paths. In-game display and frame timing remain open. |
| CTX-PORTAL-LINKS | Show and edit single-player portal camera and aim links | 1.303 adds live target and camera power feedback, Clear link and lifetime-checked Context Revert for Primary Link and Clear link. Property Revert detects conflicting changes. Activation loop checks distinguish camera and aim links from Use outputs. Native saved fields and clipboard wiring retain their format. In-game interaction and full save/reload remain open. |
| SB-FORCE-BOON | Restore the retail Force Boon as a single-player pickup | 1.304 registers the native map classname and adds a Q Entities card with the stock model, icon and sound. The 25-second pickup extends an active timer, reduces Force costs and restores six points per normal recharge tick. The timer uses a spare native powerup slot in the existing save layout. Tatooine FFA, Taspir and other sections still require their remaining adapters. In-game pickup and save/reload remain open. |
| CTX-FORCE-BOON | Edit the duration of a spawned single-player Force Boon | 1.305 exposes whole seconds in Context Properties and uses the retail default at zero. Revert checks the native item and applied duration. Duplication and saves carry the native count. Final item setup uses the selected definition for bounds, amount and item ID, including definitions beyond a missing slot in the table. In-game editing and full save/reload remain open. |
| SB-ENLIGHTENMENT | Restore both retail Enlightenment pickups in single-player | 1.306 adds stable Entities cards and native spawn classes for Light and Dark Enlightenment. The matching side and neutral powers have at least rank three during the timed effect. SP derives alignment from learned ranks and accepts either side on a tie. Gameplay and the Force selector read effective ranks, while holocrons, scripts and campaign allocation use the base ranks. Native skill cleanup runs before the temporary ranks are removed. Context duration uses the existing editable pickup count. Native serialization and asset fixtures cover the powerups. Tatooine FFA still needs its room-local movers and pads, and in-game combat and full save/reload remain open. |
| SB-TATOOINE | Make Tatooine FFA usable as a single-player Map Section | 1.307 adds a retail levelshot card, 16 entry points, the automatic platform and its oriented touch trigger, two floor pads for five isolated piston lifts, and 40 pickups with stock respawn delays. The placement bounds account for the lowest piston position. Native fixtures exercise the retail entity data, room rotation, local targets, Duplicate state, trigger serialization and cleanup. The host retains its global music and ambience. In-game rendering, lift riding and full save/reload remain open. |
| SB-TASPIR | Make the full Taspir arena usable as a single-player Map Section | 1.308 adds the retail levelshot, 16 entry points, 84 scenery pieces, ten automatic doors, four automatic platforms, the three-floor freight lift, the timed damage volume and 42 respawning pickups. Each lift route uses only its own section's eight corners. Native checks exercise stock contact, movement, travel and stop times, Duplicate and saved fields. The host retains its global music. In-game rendering, riding and full save/reload remain open. |
| SB-FACTORY | Make Factory usable as a single-player Map Section | 1.309 adds the retail levelshot, all 29 starts, the fall trigger, four glass panes with their authored shard counts and 18 respawning pickups. Players choose a home flag with Use and can practice capturing the opposing flag. The section tracks captures separately and returns a carried flag on player death or departure. Duplicate preserves broken panes and starts a fresh flag session. Native fixtures cover transformed pane geometry, flag ownership, capture, automatic return and saved fields. Interactive shattering, flag visuals and full save/reload remain open. |
| SB-CORUSCANT | Make Coruscant Streets usable as a single-player Map Section | 1.310 adds the retail levelshot, 52 starts, six lifts, 14 local lift triggers with facing checks, three native fall brushes, 41 speakers, 67 pickups and solo flag capture. The renderer applies rooftop fog within each arena. Duplicate preserves separate trigger timers and timed flybys. Native fixtures cover rotated direction, target isolation, copy clocks and safe arrival. Classic and Rend2 fixtures cover rooftop fog routing. In-game lift riding, visuals, audio and full save/reload remain open. |
| SB-HILLTOPS | Make Yavin Hilltops usable as a single-player Map Section | 1.311 adds the retail levelshot, 55 starts, six lifts, 14 lift triggers, four fall brushes, 54 pickups, solo flag capture and 36 ambient speakers. Two lift triggers retain all three of their authored brushes. Eight speakers use the retail exterior loop and seven bird calls. Duplicate preserves independent trigger cooldowns and bird-call timers. Native fixtures cover routing, rotation, audio scheduling and isolated copies. In-game contact, visuals, audio and full save/reload remain open. |
| SB-DROPZONE | Make Imperial Drop Zone usable as a single-player Map Section | 1.312 adds the retail levelshot, 45 starts, eight sliding doors, two platforms, 18 lift triggers, five fall hazards, 14 ambient speakers, 64 pickups and solo flag capture. Duplicate preserves lift travel and local trigger timers. Native fixtures exercise rotated movers, local links, capture, safe arrival and cleanup. In-game riding, visuals, audio and full save/reload remain open. |
| SB-WASTELAND | Make Hoth Wasteland usable as a single-player Map Section | 1.313 adds the stock levelshot, 25 starts, four linked base-door leaves, four lifts, two pressure pads, four fall volumes, 54 pickups, two destructible emplaced guns, and solo flag capture. Snow uses the stock texture and three authored weather volumes, with roof traces for shelter. Classic and Rend2 route fog to the spawned room. Duplicate preserves lift, hazard and gun timers. Native fixtures cover stock admission, local door and pad links, recreation, encoded gun state, safe entry and cleanup. In-game snow, fog, lift riding, gun combat and full save and reload still need testing. |
| SB-SIEGE | Make retail Siege objectives, deliveries and map sections usable in single-player | 1.314 adds directly usable Siege item, objective and finish entities. The item card spawns a stock blue crystal and an owned delivery pedestal. The controller supports exclusive carry, the 20-second return timer after a drop, saved carrier recovery, optional Force restrictions and local delivery gates. The native fixture checks setup, matching and mismatched delivery ownership, drop and return, once-only outputs and the finish marker. The full Siege maps still need their authored objective graphs, item variants, actors, map geometry adapters and interactive save and reload testing. |
| SB-SIEGE-KORRIBAN | Recreate the stock Siege Korriban objective route inside a spawned section | Two complete retail BSPs create all 690 authored children per room and keep the objectives, enemy, effects and 30-minute timer room-local. The fixture reloads all 1,383 live section entities midway through the objective route with different registered brush-model indices. It uses the production entity-field layout, BSP registry and ownership chunks and continues with the restored gate, delayed targets, moving floor, carried crystal, timer and Rancor ownership. Entering another Siege room returns the carried item home after safe arrival. Tests cover re-entry, independent victory and timeout, stock success and failure sounds, and cleanup. In-game save/reload and rendering, assembly duplication and room movement remain open before the Map Sections card. |
| SB-LIFE | Undo, cleanup, saves and transitions | Earlier behavior owner accepted (26). Newly found version-32 save dispatch regression repaired in 0.69.1 with production writer round trips; new full in-game save/load and hub acceptance remains open. Previously omitted chunks cannot be reconstructed. 0.70 adds V33 sky ownership and eight-chunk production writer checks; 0.71 advances to V34 with a 64-slot vehicle registry and native registry round trips; new graphical save/hub acceptance remains open. 0.81 advances to V35 for shield callbacks; 0.82 to V36 for appended datapad inventory; 0.83 to V37 for rail callbacks and native motion fields. Native serialization and production writer checks pass; graphical full-save/hub acceptance remains open. Future controllers still require lifecycle/save checks. 1.45 verifies independent reconstruction, cleanup and native save fields for four stock sections. Full engine save/load remains open. 1.46 retains hazard state in native saved entity fields and verifies duplicate relocation, safe entry and cleanup. 1.49 verifies Abandoned City ownership and native saved fields after breakage, keeps source damage when an intact section is copied, and preserves reused entity slots during cleanup. 1.50 covers MP Imperial Control Room copy, JRW18 reconstruction and room-owned cleanup in fixtures. In-game restore remains open. 1.51 verifies mixed-room JRW18 save reconstruction and independent cleanup in the native lifecycle fixture. 1.52 verifies mixed-world reconstruction of the Rancor Pit disabled state and cleanup of its six local torches. 1.53 verifies rotated MP hazard reconstruction, Taspir cooldown capture/copy/JRW18 restore and owned hazard cleanup. 1.54 preserves the native Bespin cloud-car phase through room relocation, copy and JRW18 reconstruction. 1.55 adds City brush damage to JRW19 with JRW18 retained for other worlds. City copies start intact and restore reapplies saved damage. |
| WALK-01 | Walk/run leg jitter | Installed fixes accepted as satisfactory/nonproblematic (27); closed. |
| BUG-DETPACK | Hundreds of det packs crash | Owner accepted as satisfactory/nonproblematic (28); closed. |
| BUG-CROWD | Crowds mix models/textures/animations | Owner accepted as satisfactory/nonproblematic (29); closed. |
| BUG-AUDIO | Missing pistol/E-11 sound | Owner accepted as satisfactory/nonproblematic (30); closed. |
| BUG-STARTUP | OpenJK/EternalJK startup errors | Owner accepted as satisfactory/nonproblematic (31); closed. |
| BUG-REND2 | Earlier model-list double free | Owner accepted as satisfactory/nonproblematic (32); closed. |
| BUG-CUBEMAPS | SP graphics restart crashes when enabling Cubemaps | 1.74.0 installed. Both crash dumps identify reflection cleanup calling an unloaded renderer. The source regression and compiled restart ordering pass. In-game confirmation remains open. |
| BUG-SWOOP | Throwing a Swoop at the ground with the SP Physgun crashes | The available 1.217 dump resolves to the cvar read fixed in 1.219. 1.249 repairs an uninitialized client pointer in the vehicle momentum update. 1.322 cancels native movement and post-move callbacks when an impact frees or replaces the moving actor or invalidates its controller. Native fixtures cover a 1,024-unit/sec downward Swoop collision, wall contact during throw momentum, same-slot reuse and a fatal callback. Current-build gameplay reproduction remains open. |
| SB-ROOM-VIS | Bespin Courtyard, Generator Room and other portable rooms render on screen | 1.69 republishes single-player inline room models. 1.70–1.72 include portable geometry in renderer visibility and projection in both modes. Native probes reproduced the former clipped Generator Room and verified draw submission. In-game confirmation on the reported rooms remains open. |
| PHY-BEAM | Steady Physgun beam | Owner accepted earlier and preserved. |
| CTX-HEALTH | Guard single-player health edits against intervening gameplay | 1.330 refreshes the form when the selected entity health changed before Apply. Revert preserves intervening damage, healing and death and retires the conflicting health edit. No-op Apply adds no history. Native entity and client records round-trip the maximum editable health. Full in-game combat and save/reload remain open. |
| CTX-01 | Entity selection and useful type-specific context properties | Implemented C menu, selection/outline, transform/health/native actions, model mass/gravity, many portable-controller properties, literal script parameters, holocron levels, decorative poses, script tags/goals, combat-point settings 0.70 sky viewpoints and 0.75 scene-camera speed/aim/wait/loop and 0.77 live waypoint range/enabled properties, validation, Undo and saved state. MP 0.69 adds type-specific current values, typed mass/health/pickup quantity, scalar Undo, identity-bound requests and saved edits. 0.82 adds datapad pickup messages through the existing text editor, bounded validation, Undo and copies. Remaining controller families/properties and native visual/full-save acceptance open.  0.92 adds shared SP-style MP C panel/actions, world cursor selection, self fields and properties for Print/Delay/Counter/Relay/Position, with identity checks, Undo and JRW8/clipboard persistence. Specialized families and fresh visual/runtime acceptance remain open. 0.93 adds Push speed/mode/sound/enabled properties and teleporter destination labels, with Undo, copied links and JRW9 persistence. 0.94 adds typed/preset sound and effect paths, playback modes, interval/random timing, Playing/Enabled and native media Undo/copy/save state.  1.2 adds Activate/Deactivate Enabled state and link guidance; teleport help now includes consenting riders.  1.3 adds laser damage/Firing/Enabled properties with conflict-aware Undo, bounded input and lifetime-safe damage credit.  1.23 adds MP Timer interval/random/Running and Random Use Once/Armed settings, marker labels and identity-bound Undo; native fixtures pass, fresh gameplay acceptance remains open.  1.24 adds MP signed Score points, Kill mode, Give help and Enabled/Undo. 1.28 adds MP station capacity, charge, recharge, unlimited supply, Refill and Enabled controls with native-state Undo. 1.29 adds MP usable mode, visible/solid, health, reuse, USE/NPC-only, block check, Rearm and Enabled controls with identity-bound Undo. 1.30 adds typed breakable health, delay, damage/USE policy, Repair, material/chunks/splash/effects and Enabled controls with Undo. |
| CTX-LASER-EMITTER | Editable SP laser emitter | 1.118 makes the existing target_laser card a selectable stock sprite with a separate traced beam. Laser properties set integer damage, firing and enabled state. Primary Link aims at a live spawned entity, and clearing the link restores forward aim. Use and linked activation toggle firing. The beam hits eligible player slot zero and stops at shot-blocking geometry. Zero damage leaves a visible harmless beam. Undo, Duplicate with aim remapping, V53 native fields and older owned emitter migration have fixture coverage. Rendered gameplay and full in-game save/reload remain open. |
| SB-FX-BEAM-LINK | Keep the single-player Target Beam usable after a destination is removed | 1.297 idles an owned beam if its destination disappears, including before its first link callback. A linked beam uses the destination's live position and validates its cached entity. Primary can reconnect the emitter. Native fixtures cover the spawn helper, removal, a same-name survivor and lookup scope. Rendered effects and full-save acceptance remain open. |
| SB-FX-TRAIL-POSE | Keep the single-player Explosion Trail correctly aimed after tool rotation | 1.298 separates the emitter's facing, launch velocity and rendered effect direction. Primary resolves a live target at launch. Aim forward and Revert are available in Context. V58 distinguishes Euler angles while older initialized direction vectors are migrated, including held poses. Fixtures cover moving and removed destinations, launch orientation, in-flight cleanup, linked activation and Context. Rendered gameplay and full-save confirmation remain open. |
| SB-FX-TRAIL-FLIGHT | Sweep single-player Sandbox Explosion Trails against scenery and release spent flights | 1.299 moves Workshop flights through a point sweep before the brush pusher can advance them and keeps the periodic FX cadence. Impacts are one-shot, sky contact is silent, and flights are removed below the room-aware fall floor or after 30 seconds. Native fixtures cover a thin obstacle, no-impact contact, the deadline and authored-flight routing. In-game effects, damage and full-save confirmation remain open. |
| CTX-STATIONS | Editable SP ammo and shield stations | 1.119 gives the four existing stations a Station tab with capacity, live charge, refill, recharge interval, unlimited supply and enabled state. Native supply amounts and difficulty defaults apply. Recharge starts off. Native Use and linked activation dispense to the player. Disable stops sound and pauses recharge. Property Revert, controlled-frame timing, older owned-station migration and V53 entity persistence have fixture coverage. 1.227 preserves captured charge, recharge delay and Use cooldown in Duplicate. Live play and full in-game save/reload remain open. |
| CTX-BREAKABLES | Repairable model-backed SP breakables | 1.120 gives the three generic breakable cards a Breakable tab for health, fuse, weapon policy, player Use, debris, explosion effect and splash damage. Broken bodies release physical attachments and leave a selectable non-solid marker with the same output link. Repair checks the complete hull and can cancel a pending fuse. Linked blasts arm safely without recursively detonating. Undo, Duplicate and V53 native entity state preserve configured behavior and the relevant phase. Authored brush callback regression passes. In-game rendering and full save/reload remain open. |
| CTX-USABLE | Editable model-backed SP func_usable | 1.121 adds Toggle and Pulse modes, editable health, damage and direct Use policies, one-shot or timed reuse, Enabled and Rearm. Primary fires on Off or Pulse and Secondary fires on On. The Off state remains selectable and suspends physical links until the body is solid again. Linked or direct activation waits for clearance. Context solidity changes and their Undo require clearance and do not fire outputs. Duplicate remaps both links; V53 fields preserve blocked return and pulse cooldown. Authored brush callbacks pass a regression fixture. In-game rendering and full save/reload remain open. |
| CTX-ANGULAR | Editable SP rotating and pendulum movers | 1.124 adds local-axis selection, signed spin speed, swing amplitude and period, start/pause, Enable and incoming Use or Link. Motion and manual rotation use the stock crate's projected collision hull, bounded arc checks, and native rider rollback. Context edits preserve the pose. Duplicate, linked teleport, V53 fields and older owned-crate migration retain the motion settings and phase. Authored brush paths retain their original behavior. 1.125 carries released native and Box3D cargo and supported vehicles with their native pilot and droid seats through angular steps and native mover translations. Vehicles follow the deck heading with their own pitch and roll. Sweep and final hull checks can refuse the whole step, including native rider rollback. Box3D updates preserve momentum, and external attachments are checked before moving. Rendered riding and full-save gameplay remain open. |
| PHY-MOUNTED-CARGO | Safe movement of det packs and trip mines mounted on SP platform cargo | 1.126 makes spinner, pendulum, lift and bobbing movement check the mounted charge hulls before committing the platform, cargo or riders. Angular movement also checks the final hulls and staged cargo neighbors. Charges retain their local mounting, fuse and damage credit, and tripwires refresh after the group is linked. V53 mount records remain readable and use the same saved fields. Rendered gameplay and full interactive save/reload remain open. |
| PHY-MOUNTED-POSE | Clearance for SP manual prop transforms with mounted charges | 1.128 checks the rotated prop hull and attached charge destinations for Context position, orientation and retryable Undo. Discrete Rotate and native Physgun rotation sweep the charge path, and native Physgun drag checks the translation sweep. A blocked charge leaves the prop and mounts at their previous poses. A successful move updates the tripwire after relinking and retains the fuse and V53 mount fields. The solver integration and remaining gaps are tracked below. |
| PHY-MOUNTED-SOLVER | SP Box3D collision for mounted det packs and trip mines | 1.129 adds contact shapes on the prop body for live mounts. Scenery and other Box3D props can stop the charge during falling, drag or rotation. Nearby on-foot players and NPCs have a separately bounded proxy that only interacts with the mounted shape. Prop mass and the native actor bridge retain their existing roles. Existing V53 records recreate shapes from the restored mounts and entity lifetimes. The remaining post-solver correction and gameplay gaps are tracked below. |
| PHY-MOUNTED-NATIVE | SP native collision for mounted det packs and trip mines | 1.130 sweeps the prop and its mounts, selects the earliest contact, and allows one remaining slide along a charge contact plane. The native translation sweep also sees charges on other props. Portal crossings are checked on each segment before its impact response. Native spin, Rope projection and recovery, and native or mixed Weld/Hinge corrections check the mounted charge path before publishing the pose. Mount and fuse state use the existing V53 records. 1.131 checks other props' charge hulls during native spin, native Physgun rotation and exact Context transforms. |
| PHY-MOUNTED-ACTOR | SP mounted-charge clearance during actor corrections after Box3D | 1.131 stages actor pushes and rider motion, then sweeps the mounted hull from the solver pose through the proposed correction. It checks the actor and charge's relative motion before publishing either result. A failed check restores the corrected prop's prior pose and discards the staged actor motion. Successful corrections publish charges and preserve their native fuse, ownership and tripwire. Deferred actor handling after a failed linked teleport uses the same previous angles. Rendered gameplay and full interactive save/reload remain open. |
| CTX-SPEAKER | Editable stock sound playback for portable SP speakers | 1.123 adds the Speaker tab, eleven stock cues, spatial/global/activator one-shots, spatial loops and repeating playback with interval and variation. The marker supports Use, Activate and incoming Link. Enable/Disable pauses and resumes the live repeat deadline. Property Revert restores settings, and V53 native fields preserve active or paused playback. Legacy owned speakers migrate. 1.223 retains active loops and active or paused repeats with the captured delay in Duplicate. Audible gameplay and full in-game save/reload remain open. |
| CTX-LOCATION | Portable location volume label, bounds and contact | SP 1.76.0 exposes label and local dimensions with oriented hull overlap, Undo, Duplicate and legacy native save restoration. MP uses player hull overlap for native chat and overlay. Rendered menu and full-save acceptance remain open. |
| CTX-CONTACT-TRIGGERS | Editable SP Repeat and Once volumes | 1.100 adds oriented player/NPC hull contact, actor and Use filters, output delay, Repeat cooldown and variation, and rearm for spent Once triggers. Properties provides dimensions. Link and Activate use native output, and Property Revert restores edits. Duplicate preserves settings and spent state. 1.226 also retains queued outputs and Repeat cooldowns and routes copied outputs through the new assembly. Native save fields and legacy migration are covered by fixtures. Rendered interaction and full native save/reload remain open. |
| CTX-TELEPORT-CONTROLLERS | Safe editable SP teleport volumes, markers and destinations | 1.103 adds exact oriented on-foot actor contact, linked feet anchors, occupied-destination hull clearance without telefragging, configurable actor filters, view and velocity, and cooldowns. An actor must exit the arrival volume before touch can send it back. Destination labels, settings and links support Undo and Duplicate. V47 saves preserve cooldowns and arrival guards and migrate older owned sources. 1.104 adds opt-in swept contact for released unlinked props and airborne projectiles, including native and Box3D prop relocation, exit clearance, impact ordering, projectile direction and retained fuse state. V48 saves preserve the new settings, object cooldowns and arrival guards. 1.105 adds an opt-in Vehicles setting for volumes and markers. The hull and its native seated riders move together only when each arrival fits. V49 preserves the new setting and vehicle arrival guards. 1.107 adds opt-in Linked props for released prop assemblies. Native and Box3D members keep their relative poses and links, with all-or-none clearance and rope routing at the exit. V51 preserves the setting, per-hull arrivals and the resulting rope path through existing save fields and chunks. 1.108 adds airborne det packs and trip mines, rolling thermals, and loose stationary thermals and flechette rounds. Each rolling collision segment can enter a portal before its impact. Native sticking and fuse callbacks remain intact, and redirected homing rockets continue tracking from their new heading. V52 preserves arrival guards for those additional projectile types. 1.109 gives native det packs and trip mines planted on Sandbox props a saved local attachment. They follow normal prop motion and travel with loose props or linked assemblies only when all charge hulls fit. The native fuse, damage credit, and tripwire direction survive the move. V53 restores attachments with fresh entity identities and releases them when a supporting prop is removed. 1.110 extends Linked groups to released props and on-foot players or NPCs. Any eligible member can enter first. The actor filter applies to every actor in the group, and an obstructed hull or rope blocks the whole move. The translation preserves relative poses and actor views. Solo portable teleport routes respect existing links. The V53 save format is unchanged. 1.111 adds linked native vehicles, pilots and droids. The Vehicles switch admits the hull and occupied seats as one unit, including links attached to a rider. Native seat bolts, complete hull and rope clearance, and staged seat checks guard the move. Hull orientation, pilot view and optional momentum survive. V53 still stores the same links and arrival guards. 1.112 adds opt-in Frozen props for linked teleport volumes. Native and Box3D frozen props can travel with an eligible member, retain their frozen state and pose, and carry mounted charges when the exit clears. The saved setting defaults to off for older volumes. Context, Undo, Duplicate and native save fixtures cover the setting and state. 1.113 adds the Movers switch for owned portable sliding doors, lifts, bobbing platforms and rail movers. A mover can lead a swept linked crossing, and the destination preserves running phase, deadlines, rail endpoints and saved route offsets. Grounded riders must belong to the group and keep support through relocation. Native lift steps permit links internal to the carried group. V53 saves preserve the option and path state in existing fields. 1.114 admits linked single authored train, bobbing, rotating and pendulum brushes through the same Movers switch. Rotated arrival bounds are checked before committing. Authored origins, endpoints, timers and angular trajectories are translated or retained as appropriate. Train offsets persist across later path corners and V53 save restoration. 1.115 adds native brush doors, platforms and buttons, including complete teams of these binary movers. Any linked team leaf can bring the other leaves and automatic door or platform triggers. A blocked leaf stops the move. Original map visibility remains open when a relocated door leaves its authored portal, and the relocated mover stops adjusting that portal. The V53 entity save retains the detachment flag. 1.116 adds all seven qualified portable map sections to the Movers setting. Their owned brushes, bobbing platforms, hazards, sound/effect sources and entry points keep their local positions and native state. A link to an eligible child brings its section root and other children. The enclosing hull must clear the exit, and unrelated occupants or grounded riders block departure. Linked riders keep their support. Retail section fixtures cover child state; the live game remains untested. 1.117 adds opt-in World anchors for linked teleport volumes. A rope, hinge or weld can carry its world pivot to an equivalent solid surface at the destination. Missing support, a mismatched surface or a blocked rope route refuses the move. Assemblies that share only the world remain independent. The V53 save format is unchanged. Mixed or non-binary teams, active movement scripts, unqualified section profiles, rigid pivots away from solid surfaces and full in-game save/reload remain open. |
| CTX-PUSH-CONTROLLERS | Editable SP Push volume and Push marker | 1.102 adds live orientation and linked aim, bounded speed and per-subject intervals, replace/add velocity, actor and prop filters, enable state, typed aim Link with Undo, exact oriented contact and swept released-prop hulls. Native and Box3D props receive velocity without releasing frozen or held objects. Missing aim remains editable. Duplicate remaps links and starts with fresh cooldowns. V46 full saves preserve live cooldowns and migrate older owned push controllers. 1.106 adds opt-in vehicle routing for Push volumes and markers, including direct Use from a seat when aimed at an enabled marker. One hull cooldown covers mounted activation and contact. Native collision handles vehicle launch momentum independently of throttle, and V50 preserves live drift in full saves. Rendered driving, rider interaction and full in-game save/reload remain open. |
| CTX-HURT-TRIGGER | Editable SP Hurt volume with independent actor cadence | 1.101 adds oriented hull contact, player/NPC and held-Use filters, normal/falling/electrical damage, per-actor intervals, and separately armed and enabled states. Activate and links arm or disarm damage. Context edits are undoable. Duplicate retains settings and starts with fresh contact timers. V45 saves preserve remaining cooldowns for live actor/volume pairs and retain older save reads. Native and menu fixtures cover contact, edits, copy and save chunks. Rendered gameplay and a full native save/reload remain open. |
| CTX-ENABLE-DISABLE | Usable SP Activate/Deactivate markers | 1.99 adds four owned Use destinations per Enable entity or Disable entity marker, live status, link Undo and grouped Undo for direct changes. Links change the native Use flag without invoking the destination, so a trigger can disable itself through a marker. Duplicate remaps links and preserves supported Use states. Native flags and links persist in save fields. Rendered interaction and full native save/reload remain open. |
| CTX-GIVE | Usable SP Give marker with native pickups | 1.98 adds four pickup links, enable/disable and link Undo. Native pickup rules govern collection. Rejected pickups remain usable. Linked dropped pickups keep their native expiry suspended until the last Give link is removed. Save restoration recovers older rejected drops, and Duplicate remaps the pickup links. Rendered interaction and full native save/reload remain open. |
| CTX-RELAY-SCORE-KILL | Editable SP Relay, Score and Kill markers | 1.97 adds Relay delay, cooldown, Once/Repeat, Rearm and pending feedback, signed Score points with a visible player total, and native Normal/Falling/Electrical Kill modes. Context settings support Revert and Duplicate. Saved Relays retain pending output and cooldown. 1.225 retains the captured pending output and cooldown in Duplicate, using the pasted Relay as the activator for that output. Rendered interaction and full native save/reload remain open. |
| CTX-TIMER-RANDOM | Editable SP repeating Timer and usable Random outputs | 1.96 gives Timer bounded interval and variation, start/stop and output muting, with native saved deadlines. Random chooses one of four linked, eligible destinations and supports Once, Repeat, Rearm, and Undo for output edits. Duplicate rewires copied outputs and keeps Random state. 1.224 retains the captured remaining countdown and running, stopped, muted, or Physgun-frozen state. Pasted timers use themselves as the activator. Rendered interaction and full native save/reload remain open. |
| CTX-LOGIC | Editable SP Delay and Counter with linked outputs | 1.95 adds typed delay and random variation, required hits, Once/Repeat, Reset, enabled state and live feedback. Counter Complete and Progress connect through Link. Property Revert protects current progress, Duplicate copies settings and counter progress, and native saves retain a scheduled delay. 1.225 retains the remaining time on queued Delay outputs when copying. In-game links and interactive save/reload remain open. |
| CTX-MOVEMENT | Editable SP actor movement and portable gravity/friction markers | SP 1.94 adds bounded Context gravity and ground friction, return to map gravity, and Undo for eligible on-foot actors. The two marker cards apply to their activator and support editable values, enabled state, Undo, Duplicate, and native save migration. SP 1.230 preserves NPC movement overrides in Duplicate through Physgun freeze and release. MP 1.172 ports both markers and direct Movement Context with native player-state prediction, permission checks, Undo, NPC copies and JRW24 persistence. In-game movement and interactive save/reload remain open. |
| SB-ALWAYS | Spawnable reusable one-shot trigger | 1.77 adds persistent SP/MP trigger_always, owned feedback, editable delay and pending/spent restoration. Authored map behavior is preserved. Graphical and SP full-save acceptance remain open. |
| SB-GIZMO | World markers for invisible logic using the green crate | 1.78 uses one stock-texture billboard per qualifying controller in both modes. 1.122 uses a thicker amber reticle over a dark disc and scales its radius from 32 to 72 units with camera depth. Native SP/MP billboard fixtures and packaged material checks pass. Rendered gameplay confirmation remains open. |
| SB-PROBE | Spawnable reflection probes in SP and MP | SP 1.5.0 introduced the portable Reflection probe. MP 1.91 adds the matching radius, capture, and enabled controls, plus a stock marker, native Rend2 capture, Duplicate, Undo, and journal restore. Rendered confirmation remains open. |
| SB-START | Spawnable Player start and Deathmatch start | 1.79 adds safe on-foot entry, teleport links and label editing in SP and MP. MP uses the two stock cards for explicitly enabled, clear entry and respawn locations in Sandbox FFA/Team Deathmatch and saves their eligibility. Graphical and SP full-save acceptance remain open. |
| SB-VISIBLE | Spawnable sight trigger | 1.80 adds `trigger_visible` in MP and makes the SP marker reusable. Both support a Primary link, range, viewing half-angle, obstruction and Force Sight controls, disable and rearm. Native state and duplication checks pass. Graphical and SP full-save gameplay acceptance remain open. |
| SB-DISTANCE | Spawnable entity-distance trigger | 1.81 adds SP Inside/Outside, reference and NPC links, bounded range, subject selection, rearming, Undo and duplication. 1.82 adds the matching MP controller, validated independent input links, and multiplayer world save/restore. Graphical and SP full-save gameplay confirmation remain open. |
| SB-WALL | Spawnable switchable wall | 1.83 adds the stock stone wall panel in both modes, model bounds, a nonblocking gizmo when off, blocked reactivation with retryable Undo, activation settings and copy/save persistence. Graphical and SP full-save gameplay confirmation remain open. |
| SB-DOOR | Spawnable sliding door | Stock panel, configurable motion, obstruction handling, linked Opened/Closed outputs, native save/clipboard state and motor audio are implemented in SP and MP. Graphical gameplay and full interactive SP save/reload remain open. |
| SB-BUTTON | Spawnable momentary button | Stock animated Imperial switch, two links, hold/activation controls, Undo, copying and saved phase in SP and MP. Visual gameplay and full interactive SP save/reload remain open. |
| SB-LIFT | Spawnable vertical lift | Stock grate and sounds, on-foot rider, loose cargo and supported occupied vehicle transport, atomic obstruction handling, Context/Undo, Raised/Lowered links and saved progress in SP and MP. Occupied carriage passes controller fixtures and a dedicated two-client Swoop test. A Classic MP listen host and guest verified a full on-foot ascent, return and jump back onto the deck. The listen host and a connected Swoop driver also verified the full vehicle ride, dismount and occupied Undo protection. Interactive SP saves remain open. |
| SB-BOBBING | Spawnable bobbing mover | MP 1.92 and SP 1.93 use the stock crate hull, shared vertical motion math, swept rider and cargo movement, and Context amplitude, cycle length, pause/resume, and enabled controls. Undo preserves phase. Duplicate and native saves restore the pose and phase, and SP legacy owned bobbers migrate at the saved pose. A Classic MP listen host and guest verified the rider through 24 units of vertical travel. Interactive SP movement and save/reload remain open. |
| SB-CAT | Broad retail and map-specific/unused spawn catalog | Partial: 1,199 SP cards (including nineteen additional map-section variants), zero listed unavailable cards, 29 owner-approved exclusions (21 empty helpers, seven missing-content cards and terrain). Of the original 182 unavailable entries, 154 have routes and 28 were excluded; 204 of 211 entity classes are routed and seven excluded. MP 1.177 has 1,135 catalog entries, including all thirteen single-player Map Sections, both movement markers, the Directional trigger, both portable portal cards, Spinner and Pendulum. SP 1.133 adds Hoth Canyon as the eighth portable stock section, with eight wind emitters and the original canyon fall volume. SP 1.134 routes the retail Hoth backdrop through a room-owned camera in Classic and Rend2. Retail PVS and camera-submission fixtures cover area and instance isolation; rendered acceptance remains open. SP 1.135 adds Imperial Shaft as the ninth portable stock section, including both native lifts, their isolated floor triggers, the fall volume and the six scaled scenery models. Mid-travel rebase and native field serialization are fixture-verified; in-game lift riding, rendering and full save/load remain open. SP 1.136 adds Rift Sanctuary as the tenth portable stock section, with 38 room-owned respawning pickups and both fall triggers. The compound trigger preserves the gap between its brushes. Stock anchor and native lifecycle checks pass. 1.149 restores cold breath in the stock playable BSP area. 1.150 adds room-local character and camera fog in Classic and Rend2. 1.152 adds Korriban FFA as the eleventh portable stock section, including 17 entry points, eight fall triggers, 47 respawning pickups, 28 torches, seven wind emitters and room-local fog. 1.154 adds Yavin Training Grounds as the twelfth portable stock section, with 16 starts, 30 saber-only breakables, 51 scenery models, 16 damage torches, 74 dust emitters driven by the local retail timer, and a detached sky backdrop in Classic and Rend2. Cross-boundary fog, rendered gameplay, and full save/load remain open. 1.73 adds the E-11 shooter and map-model cards. 1.75 adds the native location marker/volume and skeletal map prop. Restored retail actors, vehicles, pickups, rail/camera/sky/weather/reference and other portable controllers retain their documented native and asset checks; new behavior, rendered interaction and full-save acceptance remain separate. BSP collision/render registration, lighting/fog/grid resource ownership, native child ownership/core named-reference isolation, synchronous spawn-allocation rollback, V41 resource-identity restore, and section-frame doors/buttons/platforms/bobbing with oriented automatic triggers, plus rotating/pendulum world-axis paths with rider/Physgun/save handling and an opt-in content/resource admission gate and first usable stock Vjun section controller are implemented. The fixed section retains all 60 children, 17 local entry points and four sound emitters; placement, child transform updates, independent reconstruction and cleanup have native fixture coverage. Remaining BSP work: broader admission, other class-specific geometry, direct named filters and script/camera/reference/navigation isolation, arbitrary section state/copy adapters, actor/view fog and complete gameplay/save acceptance. Terrain was explicitly excluded and hidden with owner approval on 2026-10-12; the disabled legacy system is not replaced. Listings are not completion, and no further exclusions are inferred. Historical release evidence above records each family and its validation boundaries.  MP 1.24 adds Give/Score/Kill and 18 native pickups. Three legacy neutral-flag/Harvester records lack retail models and remain open for adapters, not owner-excluded.  1.25 adds portable native Repeat/Once/Hurt trigger volumes, filters, context outlines and JRW15 pending/spent-state persistence; feature 37.  1.26 adds native MP push/teleport volumes, typed destinations, context controls, four-family vehicle/seat routes and JRW16 state; dynamic-volume client prediction and Box3D-prop pushes remain open. 1.27 adds prop push-volume contacts, real Box3D/fallback velocity and JRW16 motion restoration; feature 39. Earlier Box3D-push gap is implemented; dynamic-volume prediction and live acceptance remain open. 1.28 adds five usable native station families with charge/recharge context properties and JRW17 cooldown persistence, feature 40. 1.29 adds portable MP func_usable with native toggle/button/health callbacks, dormant physics/attachments and JRW17 pending-state persistence, feature 41; authored shader-animation and brush-specific behavior remain open. 1.30 adds three model-backed MP breakables with health/damage/effects, linked actions, dormant broken state, repair and JRW17 timers, feature 42; detailed authored-map variants remain open. 1.31 adds player prediction for portable push volumes, shared rotated geometry/aim, replay-local wait/once state, authoritative consent masks and same-frame prop cooldown publication; occupied-vehicle/teleport prediction and remote gameplay acceptance remain open, feature 43. 1.32 implements occupied-vehicle push prediction with separate pilot permission masks, native hull contact and retained momentum; four-family server/client, replay, seat-policy and codec fixtures pass. Portable teleport prediction and real remote-client acceptance remain open, feature 44. SP 1.45 qualifies Bespin Courtyard, Imperial Control Room and Rancor Pit alongside Vjun, with preserved local children, owned bobbing/torch adapters, registration preflight and incomplete-section rollback. Other section profiles, actual rendered play and full-save acceptance remain open. SP 1.46 qualifies Generator Room and Taspir Landing with proven stock hazard planes, exact sampled contact after rotation, native falling/timed damage and hazard-aware entry. Six stock sections are now usable through the menu; remaining profiles and rendered/full-save acceptance stay open. MP 1.47 adds the SP music controller with shared stock tracks, bringing MP to 1,042 choices. MP 1.50 adds exact stock admission and owned local points and speakers for Imperial Control Room. 1.49 adds Abandoned City, the seventh SP section, with 69 native breakable stone brushes, local wind and exact geometry admission. Rendered gameplay and full native save/load remain open. MP includes the stock explosive Cargo Barrel as of 1.57. MP includes a functional stock emplaced gun as of 1.58. MP includes the stock persistent E-Web as of 1.59. MP 1.60 adds directly spawnable stock exploding crates and gas tanks. 1.61 adds 20 curated prop cards and 14 direct native actor cards to MP. Kejim crate ID 1013 uses the byte-matched retail JA geometry and texture in SP and MP. 1.62 ports the three portable plasma, grenade and rocket shooters with linked aim and saved repeat timing. 1.63 ports the planted bomb and deployable beacon with stock animation and saved phase. 1.64 adds the small cargo crate with stock heavy-weapon break and medpack drop. 1.65 adds the gun rack with three stock, owned shelf pickups and saved collected slots. 1.66 adds the ammo rack with five stock pickups and persisted shelf state. 1.67 adds the stock Jabba door camera with its telescoping parts and saved pose. 1.68 adds the stock welder with native clipped sparks and consent-aware damage, Context settings and saved pulse timing. 1.69 adds the MP crystal crate and native misc_teleporter card. SP portable room geometry now goes through live snapshots at spawn and after loading an older save. In-game visibility remains to be checked. 1.70 centers new MP rooms on the aimed surface, includes visible rooms in Classic projection bounds, and gives each Rend2 brush instance independent surface submission. Owner-reported graphical visibility remains open. |
| TOOL-ALL | Tools work on every entity, including players | Basic native/player manipulation and mixed Rope/Weld/Hinge/Thruster paths implemented. Remaining special controller cases and native integration acceptance open; 0.52 adds ordinary MP actor control; 0.56 adds empty vehicle holding/freezing/rotation and native hull validation. 0.57 adds native Rope/Thruster forces, consent and attachment cleanup. 0.61 adds MP Box3D; 0.62 adds anchored rigid ropes, mounted thruster torque and tool settings. 0.66 adds MP Weld/Hinge for rigid and native/mixed graphs, with copies, JRW5 and player consent. 0.72 adds occupied-vehicle Physgun/Freeze/context with rider consent, native seat attachment and client freeze prediction. 0.73.1 implements occupied Rope/Thruster/Weld/Hinge with horizontal momentum separate from steering and vertical forces sharing native gravity. 0.74.0 adds JRW6 persistence of commanded speed and horizontal external momentum, with legacy reads and production fixtures. Human occupied driving and full native-action parity remain open. |
| TOOL-ROPE | Rope tool, including visible ropes | Partial: native/prop/player/world/mixed attachments and thick visible ropes implemented in both engines. 1.42 integrates SP scenery wrapping; 1.43 adds MP authoritative bends, replicated mesh, complete-path tension, bounded swept recovery, JRW18 persistence and copy rerouting/rollback. 1.89 adds routing around moving collision bodies, per-rope cache invalidation and temporary suspension when a body overlaps an anchor. 1.90 transfers taut-rope tension to eligible movable Sandbox props at bends, with Box3D mass and torque, freeze and hold checks, and MP edit permissions. 1.248 accounts for SP prop inertia at mixed or wrapped rope tie points and combines all bend impulses per prop. 1.250 adds Context length edits for individual attached ropes, with Revert and existing save/copy persistence. 1.260 adds independent bend offsets around staggered barriers. 1.261 adds a four-bend search for narrower offset openings and tests saved topology with the SP Workshop codec. 1.262 reduces tightening passes on retained bends and covers 32 moving ropes within the route budget in a native fixture. 1.267 finds six-bend routes through three alternating passages, retains the existing sweep limits and save format, and passes the native SP and MP fixtures and SP Workshop codec test. 1.268 finds eight-bend routes through four alternating passages and validates 180 native layouts, including rotated brushes, under the same limits and codec. Distributed rope mass, retained-route follow-ups, rendered gameplay, full in-game saves and representative remote/load acceptance remain open. The multiplayer length, tension and constraint regressions are in the priority backlog above. |
| ART-STOCK | Cheap game-provided catalog icons | Implemented stock icons/generic retail fallbacks; isometric views preferred only when cheap. No new custom catalog art. |
| SB-GMOD-MAPS | Port the original Garry's Mod Flatgrass and Construct maps to JA:R | Both converted RBSPs and their local material dependencies are installed as matching local PK3s in SP and MP. Terrain, 188 baked model placements, four movable props, baked lighting, sky camera, Construct ladder and mirror portal are included. The five scenery placements flagged solid in Source contribute nine native convex brushes. Tests match the VPhysics surfaces and preserve the sink's open space. Construct restores the original positions of its room brushes and mirror. The native USE switch toggles the original room light layer on 174 faces and plays the stock click. The converter includes the baked sunlight layer on 2,264 faces. Both maps include 19 attached decals and 11 compiled overlays, with checked texture RGBA, projection and receiving-leaf visibility. Construct's unattached source decal 506897 is recorded in the conversion report. JA:R 1.146 supports Construct's 19 authored soundscape zones and four mixes, with three-second crossfades, stereo routing and the Lake mix's nested water loop. Native collision, switch callbacks, sample conversion, soundscape selection and Classic/Rend2 lightmap ingestion pass. Remaining scripted interactions and interactive acceptance remain open. |
| SB-MAP-PICKER | Browse and start installed maps in single-player with map-derived thumbnails | The main and pause menus open a thumbnail grid with installed levelshots and generated BSP previews. 1.272 adds incremental title search, native SP difficulty and start admission, and revalidation when starting a map. Fixtures cover both preview routes, search, pagination, cancellation, eligibility changes and the native map command. Interactive visual and gameplay acceptance remain open. |
| SP-MAP-CHEATS | Enable cheats whenever a single-player map loads | 1.144 sets `helpUsObi` before game initialization and systeminfo publication in the common local server startup. The Maps picker, console commands, campaign transitions and saved games use this startup. The native resource reload mode remains selectable through the existing commands. |
| POSE-01 | Decorative model collision and selectable poses | Static frame selection implemented in SP (0.58): posed collision/Box3D bounds, context controls, copy, Revert and full-save restoration. 1.276 tests SP Apply and Revert with the selected frame's convex hull and checks mounted charges while preserving solver motion. 1.277 uses the same qualified hull for menu placement and Duplicate scenery checks. 1.278 seats ordinary copied assemblies using individual solid members and mounted charge support along the target surface normal. 1.279 adds a bounded scenery sweep for the complete free assembly when the initial placement collides. 0.63 adds MP MD3 prop poses with rotated collision/physics, context Revert and copied/saved frames. 0.64 adds all 556 available retail MD3 props and selected-hull spawn/restore validation. 0.65 adds 126 skeletal MP props, independent root poses and a direct frame field. Visual/contact/save/hub acceptance remains open. |
| CRASH-01 | Noghri staff attack crash | Dump-backed impact fixes installed with native regression checks. Owner accepted attacks in feedback question 17 on 2026-10-11; this later acceptance supplements the original 32 accepted reports. |
| NPC-REPORT | Desann, Mon Mothma, RebornBoss and Morgan Katarn spawn failures | Loaded-definition/class fixes installed; 0.80 adds the retail CLASS_MORGAN alias in SP/MP. Owner accepted these named NPCs in feedback question 16 on 2026-10-11. 1.237 distinguishes single-player native spawn failures and protects the campaign enemy tally. Nulldriver passes installed resource admission; its live spawn retest and newly restored legacy actor/rendered acceptance remain open. |
| NPC-LEGACY-SKIN | Admit compatible segmented single-player NPCs with explicit lower skins | 1.264 uses the same animation key for precaching, spawn and rendering, and requires an installed explicit custom lower skin. Native catalog tests passed, including missing assets, malformed names, and embedded default skins. In-game rendering and save/reload remain open. |
| NPC-CATALOG-PARSE | Match native NPC parsing for recognized fields and definition boundaries | 1.265 keeps adjacent compact definitions separate and handles recognized fields with native tokenization. Native regression cases passed. The retail comparison found identical metadata and admission for all 201 records in both catalog modes. |
| NPC-NATIVE-LOOKUP | Preserve NPC lookup after quoted brace values | 1.266 fixes native spawn, animation precache, model precache and template lookup together. The native production-function test verifies model, skin, animation and spawn selection after a quoted brace, with matching observed results for all 201 stock definitions. Interactive spawn with custom definitions remains open. |
| FX-FLASH | Remove missing-texture flash on saber impact | 0.91 filters the remaining saber_cut/saber_block ScreenFlash primitives in both native effect parsers, including appearance presets, while retaining sparks/smoke. Fresh SP/MP visual confirmation remains open. |
| MP-HOST-TEXTURES | LAN server type 1 makes all maps/UI show missing textures | 0.86 repairs the mismatch between the graphical client mode and public dedicated cvar. Host menu now offers a playable LAN server; separate dedicated launcher retains LAN/public modes. Production native-image and menu regressions pass; owner accepted LAN textures/HUD in feedback question 1; other-client tests remain open. |
| MP-Q | Q cannot open multiplayer spawn menu | Native held-key handoff repaired in 0.81, production catcher/cgame input regression passes. Owner accepted held-Q opening; 0.91 fixes field pinning and 0.92 retains it in the rebuilt C panel. |
| ART-01 | Application icon and cursor | Implemented; retained provenance. |
| FX-01 | Bomb/ion/random projectile variants with native semantics | Implemented; prior bounded renderer checks retained. Broader integration/campaign acceptance open. |
| RANDOM-01 | Existing randomizers, mission safety and recovery | Existing bounded implementation retained. Broad campaign/continuity acceptance open; no new randomizer scope inferred. |
| SP-HUB-RETURN | Restore the destination world and carry the current player into a revisited single-player hub | 1.323 reads the full destination save in native order, preserves the outgoing carry and campaign tiers, and merges inventory without replacing saved world ownership or joints. The arrival uses the requested view and has a fallback start. The hub checkpoint waits for client entry. Native save-layout and inventory merge checks pass. In-game return, autosave and respawn are open. |
| SP-CARRY-READ | Restore valid carried gear and Force ranks across maps and autosaves, with native fallback for damaged data | 1.324 shares the native carry layout between the writer, item precache and restore. It accepts old short lists, absent secondary sabers and native Mind Trick and Saber Offense ranks. Truncation, invalid ranks, overlong names and excess ammo entries are rejected before the affected fields are applied. In-game transitions and compatibility with the owner's saves remain unverified. |
| LAUNCH-01 | Both SP/MP executable entry points work in the installed directories | 0.74.1 installed root SP/MP launchers and automatic parent asset discovery for private runtimes. Harmless child/path tests and native dedicated default-path startup passed. Owner graphical startup confirmation pending. |
| MP-ASSETS | Keep unrelated JAPro material out of JA:R MP | 0.76.0 implements scoped exclusion of base/japro-assets.pk3 and base/japro-win-x86.pk3 while JA:R MP is selected, preserving files and unrelated packages. Production policy and private native conflicting-package fixtures pass; graphical confirmation remains open. Renamed packages/loose foreign files are outside this narrow rule. |
| MP-MENU | JA:R entry and controls on the MP main menu | 0.76.0 implements native main-menu JA:R Settings and Host JA:R Sandbox, matching pause menu, camera controls, explicit binding installation and server-validated player consent. Staged host fields apply only on valid Start. Production action/navigation/retail-asset fixtures pass; graphical click-through and owner acceptance remain open. |
| MP-UI | MP UI matches the SP JA:R design | 0.76.0 shares SP palette/cursor, uses Arial and two-column main/pause layouts, and styles all 48 other native MP menus while preserving functional widgets/art. Q/C palette and pointer match. Partial: rendered visual acceptance and remaining SP feature/settings ports are still open.  Owner accepted both menu appearances (question 5); 0.92 C panel/action parity is newly implemented and needs visual acceptance. Specialized SP context and settings ports remain open. |
| MP-01 | Hosted shared Sandbox like a normal GMod server | Partial: dedicated/client binaries with protocol 1051/game API 7/client API 8/renderer API 12; 1,106 entries; authoritative tools/context, actor/empty-vehicle grip and rotation, ownership/consent, assembly copies, group Undo, cleanup and automatic journaled world persistence. 0.61 adds Box3D props and actor/platform contacts. 0.62 adds rigid rope anchors, surface-mounted thruster torque/power and compact tool properties. 0.63 adds MP MD3 poses/context Undo and JRW4 frame persistence with JRW1/2/3 reads. 0.64 adds 132 larger props and full-hull placement/restore. 0.65 adds 126 skeletal props, posed bounds and per-instance client rendering. 0.66 adds native/rigid Weld and Hinge, constrained Physgun dragging and JRW5 frames with JRW1-4 reads. 0.68 adds map actor alias cards. 0.69 adds typed context mass/health/pickup quantities, applicable actions, current-value echoes, conflict-aware scalar Undo and lifetime-bound edits; feature 14. 0.71 adds native stock swoop color choices and expands the definition registry to 64 slots; feature 15. Production and native loopback tests are separate from human acceptance. 0.72 adds occupied Physgun/Freeze/context, seat consent/lifecycle and frozen client prediction; feature 16. 0.73.1 adds occupied attachment forces, sustained-thrust repair and replicated horizontal momentum; feature 17. 0.74.0 adds a root MP executable, private SP/MP directory discovery and JRW6 vehicle momentum persistence; feature 18. 0.79 increases capacity to 16,384 slots, 4,096 snapshot entities and 4,096 props; live load/performance acceptance remains open. 0.81 repairs native Q/C input handoff; owner accepted Q opening. Human driving, fuller context/tool parity, map catalog, presentation/combat ports and human/remote/late-join/load validation remain open; 0.91 owner-directed shared editing/persistence defaults and respawn retention are implemented. 0.92 adds controller settings and shared C actions with JRW8 persistence, feature 25. Human remote/late-join/load tests remain open. 0.93 adds native Push/Teleporter/destination routes with context settings, typed links, assembly copies and JRW9 saves (JRW1-8 reads), feature 26. Vehicle/seat teleporting is implemented in 1.0 and vehicle Push in 1.1; human driving acceptance remains open. 0.94 adds native sound/FX helpers, replicated playback, linked aim/emission and JRW10 persistence with JRW1-9 reads; feature 27. Listening/rendering and human networking acceptance remain open. 0.95 shares the four restored legacy NPC families and prop models with SP; 79 map aliases and 686 model props. New actor/rendered/client asset acceptance remains open. 0.96 restores Galak shield/melee/generator/laser behavior, original art and model-specific replicated surface/phase state; feature 28, 687 props. New actor gameplay and remote acceptance remain open. 0.98 adds four original static droid bodies and three map aliases via an MP-only exact-geometry Ghoul2 adapter; 82 aliases, feature 29. Authored team parsing is repaired. Native model/pose/spawn/restore checks remain separate from rendered and combat acceptance. 1.0 adds occupied vehicle teleporting with C activation, all-seat consent, collision preflight, native effects and movement reset; feature 30. Deterministic production fixtures pass, while rendered travel and remote driving acceptance remain open. 1.1 adds occupied/empty vehicle Push through native callbacks, replicated persistent momentum, seated C/Delay activation and JRW11 motion saves with JRW1-10 reads; feature 31. Human driving and broader parity remain open.  1.2 adds Activate/Deactivate controllers, immediate media pause/resume, typed links and saved Enabled state; feature 32. Shared public version source fixes SP/MP/package label drift.  1.3 adds native-trace laser helpers, current-position aim, permission-aware damage, property Undo and JRW12 with JRW1-11 reads; feature 33. Rendered/combat/remote acceptance remains open.  1.23 adds Timer/Random/position-marker routes, context edits and pending/spent-state copy/save restoration; feature 35 and JRW13 with legacy reads.  1.24 adds Give/Score/Kill, 18 native pickups, runtime pickup placement/registration repair and persistent consumed items; feature 36, JRW14 with legacy reads.  1.25 adds portable native Repeat/Once/Hurt trigger volumes, filters, context outlines and JRW15 pending/spent-state persistence; feature 37.  1.26 adds native MP push/teleport volumes, typed destinations, context controls, four-family vehicle/seat routes and JRW16 state; dynamic-volume client prediction and Box3D-prop pushes remain open. 1.27 adds prop push-volume contacts, real Box3D/fallback velocity and JRW16 motion restoration; feature 39. Earlier Box3D-push gap is implemented; dynamic-volume prediction and live acceptance remain open. 1.28 adds five usable native station families with charge/recharge context properties and JRW17 cooldown persistence, feature 40. 1.29 adds portable MP func_usable with native toggle/button/health callbacks, dormant physics/attachments and JRW17 pending-state persistence, feature 41; authored shader-animation and brush-specific behavior remain open. 1.30 adds three model-backed MP breakables with health/damage/effects, linked actions, dormant broken state, repair and JRW17 timers, feature 42; detailed authored-map variants remain open. 1.31 adds player prediction for portable push volumes, shared rotated geometry/aim, replay-local wait/once state, authoritative consent masks and same-frame prop cooldown publication; occupied-vehicle/teleport prediction and remote gameplay acceptance remain open, feature 43. 1.32 implements occupied-vehicle push prediction with separate pilot permission masks, native hull contact and retained momentum; four-family server/client, replay, seat-policy and codec fixtures pass. Portable teleport prediction and real remote-client acceptance remain open, feature 44. 1.33 adds shared SP/MP Legacy/Comfort/Crisp camera math and an archived MP selector with Apply/Revert; native view/menu fixtures pass, graphical feel acceptance remains open. SP accepted camera behavior is preserved, broader settings parity remains incomplete. 1.34 adds MP Console settings with shared SP palette/choices, native opacity/size/height and cross-page Apply/Revert. Native menu fixtures and source audits pass; rendered acceptance and broader interface/settings parity remain open. 1.35 adds MP Interface settings, shared font/UI choices/policy and byte-identical assets; native fallback/menu fixtures pass. Rendered and pure-server acceptance, UI motion/HUD and broader settings parity remain open. 1.36 adds MP Academy/Simple HUD selection and shared SP/MP Reduced/Normal UI motion; native menu/renderer fixtures and source audits pass. Rendered HUD/motion/reload and broader parity acceptance remain open. 1.37 exposes fifteen native classic graphics controls in MP using shared SP choice data; transaction fixtures and source audits pass. Display confirmation, MP Rend2, rendered/performance acceptance and broader parity remain open. 1.38 adds shared display choices and timed confirmation; 1.39 shares saber/sound/effect packs; 1.40 shares facial presets and speech sensitivity; 1.41 adds shared combat choices and MP native animation-cadence fists/Quick ignition. Automated evidence is recorded in the dated checkpoints. MP Rend2, remaining controllers/teleport prediction and actual remote/load/rendered acceptance remain open. 1.42 adds SP scenery wrapping; 1.43 implements MP authoritative bends, replicated rendering, JRW18 persistence, copy rerouting and bounded recovery, with feature 54 and matching protocol/API revisions. Dynamic cable contacts and broader rope parity remain open. 1.44 adds opt-in MP Rend2, two portable light cards, bounded on-foot teleport prediction and two-client native UDP smoke coverage; rendered and remote human acceptance remain open. 1.47 ports the SP music card, native cue publication, track edit/Undo and JRW18 restore. Native UDP tests cover pre-existing light/music state at late join and removal/restore seen by both peers. 1.47.1 restores empty music cues through native sound startup and covers the silent baseline in the server fixture. 1.48 implements native-client pure archive proofs and exercises both accepted and rejected proofs against a pure dedicated server using package archives. Graphical pure join, remote-human, rendered/audio and load acceptance remain open. 1.50 adds the first portable MP BSP and verifies active and late-join resource order, two instances, native speakers and Undo in the dedicated-server probe. Other SP sections and rendered room acceptance remain open. 1.51 adds Vjun and room Physgun surface anchoring. Mixed rooms, distinct resources, late-join archive proofs, native speakers and independent Undo pass on pure and non-pure loopback servers. 1.52 adds Rancor Pit and its native torch effects, loops and consent-aware damage. The mixed-room wire gate covers the third BSP registration and torch cleanup on pure and non-pure dedicated servers. 1.53 adds Generator Room and Taspir Landing with cumulative BSP model indices, native consent-aware hazard callbacks, safe entry and JRW18 cooldown restore. Pure and non-pure late-join and cleanup gates cover both resources. 1.54 adds Bespin Courtyard with a stock cloud-car mover, phase persistence and permission-aware crush rollback. The native mover and pure/non-pure two-client gates verify its collision and publication paths. 1.55 adds Abandoned City with 69 independent brush children, debris and JRW19 damage. The City/Vjun wire gate covers all 70 inline indices, late join and owned cleanup. 1.56 adds bounded solo-pilot teleport prediction for frozen owned endpoints, with native hull and pilot arrivals, exact codec coverage and replay correction. Rider-first, authored and more complex routes stay server-authoritative. Broader parity, remote human driving and representative load acceptance remain open. 1.57 ports the stock explosive Cargo Barrel with immediate Activate, the original damage fuse, native thermal FX, permissions, links and pending-fuse journal restore. Rider-first teleport contacts now defer prediction for the rest of that replay. 1.58 adds the stock native MP emplaced gun, reciprocal mount and dismount, projectile fire, tool and seat safeguards, yaw and health settings, and parked JRW restore. SP’s physical chair seat and the persistent E-Web remain open. 1.59 adds the persistent stock E-Web with its native aim rig, SP fire parameters, finite ammunition, parked restore, and repairable wreck. 1.60 adds the exploding crate and gas tank, native effects and damage, Primary death output, and journaled explosion and gas-jet state. 1.61 carries over all 20 curated props and 14 direct NPC and vehicle cards with the SP labels, icons and IDs. 1.62 adds the three portable shooters, stock native projectiles, direct and splash permissions, linked aim, and JRW18/JRW19 repeat timing. 1.63 adds the planted bomb and beacon with native MD3 animation, collision, audio, and saved phase and frame. Graphical and remote-human verification remain open. 1.64 adds the small cargo crate, one temporary native medpack, and lazy pickup art registration for already connected clients. 1.65 adds the gun rack with stock shelf rendering, native pickup and Force Pull, and persistent slot state. 1.66 adds the ammo rack with its stock contents, quantities and clip policy, shared native pickup and Force Pull, and saved slots. 1.67 adds the Jabba door camera with stock models and servo, swept extension, part selection, Context controls and saved mid-stroke state. 1.68 adds the stock welder with native clipped sparks and consent-aware damage, Context settings and saved pulse timing. 1.69 adds the MP crystal crate and native misc_teleporter card. 1.70 centers new MP rooms on the aimed surface, includes visible rooms in Classic projection bounds, and gives each Rend2 brush instance independent surface submission. Owner-reported graphical visibility remains open. |
| RELEASE-01 | Build, test, stage and verify the exact local release | Build and verify each staged ZIP through `scripts/build-jar.ps1`. Local installation requires the owner's explicit request, then the exact ZIP, backups and SHA-256 receipt. The goal's installation and gameplay acceptance gates remain open for staged changes. |

Superseded: Simple/preset-default entry and the separate primary carried selector
replaced by independent settings and the native weapon bar. Extra simultaneous
hands and independent ammo per duplicate record were not accepted promises.
Unavailable requirements require explicit owner disposition, not silent closure.

MP main-menu integration, initial SP design alignment and the two known JAPro
package exclusions are implemented in 0.76.0 with automated/native evidence; visual
acceptance is pending. Next implementation work: remaining catalog/controller
families, context properties, Rope integration and full MP tool/capacity/settings
parity. Preserve
the 32 accepted SP items. Pending new legacy-NPC/context/rope/campaign and multiplayer
acceptance is distinct from source progress. Do not launch/attach/terminate owner
gameplay without applicable authorization.



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

## Current scope and evidence (10 October 2026)

The completion goal is **active and unfinished**, rechecked with the goal tool
after the 10 October continuation. The prior paused observation is superseded.
The owner's explicit hosted multiplayer request extends the stored SP objective;
the earlier instruction not to infer MP no longer excludes this requested port.
All non-superseded objectives remain open until implemented and appropriately
qualified. Build/install evidence does not establish gameplay or performance.

The owner tested and accepted all **32 annotated SP items** as satisfactory or
nonproblematic. The [exact acceptance record](JAR-owner-acceptance-2026-10-10.md)
controls their status; unchanged accepted behavior does not need repeated owner
approval. Noghri attacks and Desann/Mon Mothma/RebornBoss spawning were not in
that list. Context property coverage, unfinished catalog routes and Rope work
also remain separate. Historical pending-test wording below is superseded.

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

### Previous 0.51.0 verification (post-package workspace entry)

**0.51.0 was installed**, completing the previously blocked workflow after the
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

### Previous 0.50.0 evidence (historical post-package workspace entry)

**0.50.0 was installed and independently verified**: **73/73 suites**, **58
installed files** byte-matched to the exact ZIP, **12 changed-file backups**
hash-verified, and **1,634 source snapshot files** matched before these receipt
entries were added to the workspace documentation. Exact packaged sources and
prior 0.49 release/rollback evidence remain preserved.

- Build: `20261010-175105-2558efd7`
- Package: `build/JediAcademyRemixed-0.50.0-win64.zip`
- ZIP SHA-256: `D9AE88BB94AA3FEBEF099105BFE18696A9E71385D587F1C1C817C7EFD319FF95`
- Receipt/backups: `build/deploy/20261010-175214-5344eb29/receipt.json`
- Qualification: `build/legacy-npcs-0.50.0/qualification.json`
- Full workflow: `build/release-0.50.0.log`
- Symbols/source: `build/symbols/20261010-175105-2558efd7.zip`

No graphical client or owner game was launched. New droid gameplay and rope
visual acceptance remain open; full multiplayer parity is unfinished. The goal
remains active and the 32 previously accepted SP items remain closed.

### 0.51.0 package preparation (historical; resolved above)

All **74/74 suites passed**. The first installation attempt was refused because
JA:R was running; **0.50.0 remains installed** until a successful deployment
receipt says otherwise. The initial candidate is retained under
`build/pickups-0.51.0/initial-candidate.zip`. The release is being repackaged to
include the final UTF-8 documentation updates; its initial source/symbol archive
`build/symbols/20261010-180313-d3f808c9.zip` remains preserved. No owner process
was stopped. Multiplayer remains a partial port, and new pickup gameplay is
not yet owner-validated.


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
constraint. The line follows both saved local anchor points. Rope slack is set in Q.

**Thruster** now applies force to native actors, pickups and models with ordinary
object motion, as well as owned props. Left attaches/toggles; right removes. The
clicked face selects direction. Native actor input/AI and gravity remain active;
forces do not replace their controllers or cause Force-grip damage. Frozen native
entities wait for Release. Native scripted/path movers still need a dedicated
thruster motion adapter and report that limitation instead of adding an inert tool.

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

## Retained 0.22.1 baseline and still-open work (2026-10-10)

**0.22.1 crash fixes (2026-10-10):** Noghri staff primary fire now handles
player shooters without an NPC record and deals 10 direct-hit damage; native NPC
spread and difficulty damage are preserved. Native prop impacts no longer read a
missing client record. Linked grenade shooters also handle missing NPC/enemy data.
Two retained 0.22 crash dumps confirmed the staff and impact faults; production-code
regression tests cover all three paths. No game was launched for this hotfix.
Save header V8, workshop wire 5, APIs and the catalog remain unchanged.



The owner accepts the beams and reports remaining **leg-animation-only walking
jitter**. Beam behavior is preserved. The default `g_noFootSlide 1` now adapts NPC
feet only; player locomotion keeps the authored animation speed. `0` disables the
feature for everyone; `2` opts back into the legacy adaptive player path. The
installed profile did not contain a saved override. The production animation
fixture covers alternating zero/nonzero player velocity, but the visual fix
still needs the owner's in-game check.

The catalog retains 1,197 searchable cards. **74 of the former 182 unavailable
entries now have spawn routes**, including 71 portable map-entity setups and three
NPC aliases. There are **108 remaining blocked entries**. Activate and Link tools
make targets/controllers usable; automatic destinations, train corners and keys
are owned and cleaned up with their parent. See [controls and substitutions](JAR-spawn-catalog.md).
New full saves use V8 `0x4a415209`; workshop wire 5 and APIs 1020 remain unchanged.

### Work still waiting

1. Confirm and, if necessary, continue diagnosing the player's walking animation.
   The owner has accepted beam appearance; do not count it as an open visual task.
2. Extend the remaining 108 entries where feasible. Missing assets/definitions and
   invisible helper meshes need suitable replacements; BSP/navigation/weather/script
   controllers and multi-entity assemblies need dedicated portable implementations.
3. Qualify the new controllers in game: spawning, linking, moving, cleanup, save/load
   and map transitions, including unusual NPCs and large vehicles. Automated spawn
   transactions do not substitute for that gameplay evidence.
4. Improve decorative GLM bounds/poses and constraints that attach to native
   entities. The 0.24 Duplicate implementation now handles connected owned assemblies,
   their properties and target/helper links; native gameplay acceptance is open. Rope/Weld/Hinge/Thruster use Box3D props. Ropes do not wrap around or collide with scenery.
5. Stress-test many actively simulated props and actor contacts. The fixed 4,096
   registry/allocation checks do not establish an active-body performance ceiling.
6. Complete broad campaign/cutscene/vehicle-camera and physical-controller acceptance,
   plus voice/face appearance across models, silence/death states and both renderers.
   Earlier bounded native tests remain useful but are not a complete campaign audit.

Additional licensed melee/weapon assets remain an expansion option. Extra hands,
independent ammunition per duplicate carried record and a multiplayer port are not
implemented promises. The former Simple/preset-default design and primary carried
selector were superseded by the owner's replacement scope, not left unfinished.

The release workflow runs 41 headless suites plus independent retail/asset checks,
then packages and installs with backups and SHA-256 verification. This update has
not launched the game. See [validation](JAR-validation.md) for the evidence boundary.

## Historical checkpoints (superseded where noted above)

> **0.21 update (2026-10-09):** the catalog now has 1,197 searchable entries,
> covering every installed retail model/definition name with explicit exceptions.
> See [the catalog guide](JAR-spawn-catalog.md). Actor capacity is 128; the fixed
> object limit stays 4,096. Physgun wheel direction is inverted, the beam uses a
> steady blue stock laser, and Toolgun has a firing sound. The remaining player
> leg stutter is addressed in foot-speed scaling. New saves use V7 `0x4a415208`;
> gameplay/visual confirmation remains outstanding. Older counts below are historical.

> **Latest owner request, 2026-10-09:** the 0.20 implementation supersedes the
> control, icon, capacity and native-hold limitations in older checkpoint entries.

## Historical status: 0.20 controls, physics and tools

Physgun wheel distance/right-click freeze/fling, distinct firing-only Toolgun FX,
per-tool secondary actions, 120 Hz interpolated physics, volume-derived mass,
actor collision/player support, collision sounds, fixed 4,096 capacity, harmless
native holds (including players/movers), stock icons, fixed top-left help, compact
tool list/properties, Duplicate/Rope/Thruster, speech-shape variation and a leg-phase
resynchronization correction are implemented. See the current controls/physics
guides and 0.20 validation entry for behavior, exact scope and evidence.

Automated checks do not establish native gameplay acceptance: this turn did not
launch the game. Full catalog/assembly cloning and arbitrary native physical
attachments remain incomplete. Physgun/direct tool transforms accept native
entities; Rope/Weld/Hinge/Thruster remain Box3D-prop attachments. Campaign/controller
acceptance and large active physics loads remain unqualified.


> **Current owner direction (2026-10-05):** the earlier Simple/preset-default,
> primary carried-inventory selector, paused Sandbox menu, small prop limit and
> blanket authored-object protection are superseded. Follow
> [the replacement Sandbox scope](JAR-sandbox-replacement.md). 0.15 was installed
> before this change and is retained as a checkpoint, not the final design.

## Historical status: 0.18 constraints

The tables and dated checkpoint entries below are historical planning records.
The replacement scope supersedes Simple/preset defaults and the primary carried
selector. Implemented now: Advanced entry; held-Q icon catalog and tool weapons;
53 qualified cards with pagination; native map-object manipulation/undo; optional
Box3D rigid props, Weld/Hinge/Unlink modes and native controller proxies; 30 additional native menu
layouts; and the original vector application icon. Earlier releases retain fist
cadence, quick extension, camera choices and the two original missing-projectile
visual repairs. See [current controls](JAR-inventory-sandbox.md) and
[physics contract](JAR-physics.md).

Remaining requested implementation: the catalog does not cover every retail
entity; arbitrary map geometry/controllers and reverse forces/damage on native
controllers are not implemented. Remaining acceptance: no physical
controller in this qualification environment, and no complete campaign
playthrough. Preset requirements UI-01/02/03 and the old primary-selector design
are superseded rather than unfinished. Local vector/icon production resolved
the icon requirement without a cloud generator or imported logo artwork.

## Historical request audit (2026-10-05)

| Requested area | Current result and exact scope |
| --- | --- |
| Physics / default | Box3D is Off by default. Settings > Sandbox, resume, hold Q, click Box3D. It simulates owned props; actor/vehicle/mover proxies push them. Props do not drive or damage native controllers. |
| Catalog / all entities | 53 cards: 20 props, 12 NPCs, 15 weapons, four supplies, two vehicles; paged and fingerprint-qualified. Full retail coverage is unfinished. |
| Toolgun / constraints | Eight modes: Select, Rotate, Freeze, Gravity, Remove, Weld, Hinge, Unlink. Weld/Hinge are new in 0.18, saved with ownership checks, Undo and bounded cleanup. |
| Native map manipulation | Eligible stationary misc_model variants move/rotate/freeze; map pickups and free NPCs select/remove. Brushes, movers, vehicles and occupied/scripted controllers retain native control. Map removal has no undo. Arbitrary map editing remains unfinished. |
| Menus / presets | Advanced opens directly. Simple/Chaos/Vanilla/Intended entry and initialization were removed under the replacement request. Existing preferences remain. Main/pause/character/saber/loading plus 30 audited native menu signatures use the new presentation; custom layouts retain their own presentation. |
| Weapon bar | Physgun/Toolgun have real IDs 29/30 and distinct names/icons. Shortblade, Energy Pike and Sith Sword have distinct bar names/icons. Native gun icons remain native; tool modes share one slot. |
| Cursor / application icon | Original 256px cursor displayed at 28-64px with tested hotspot. Original nine-size Windows icon plus SDL icon; retained source geometry. |
| Gun sounds | Earlier native event fix routes proper weapon sounds in the optional MBII flash path. This release adds no replacement sound assets. |
| Projectiles | Original bomb/ion visual gap repairs; prior native Classic/Rend2 evidence covers 78 variants and both effect packs' repaired cases. Instant beams remain instant; weapon semantics unchanged. |
| Camera / combat | Legacy/custom, Comfort and Crisp camera options, protected native camera paths, continuous fists and quick saber extension are retained. Prior 0.17 native regressions remain evidence for unchanged combat code. |
| Acceptance gaps | No physical controller hardware or complete manual campaign/cutscene playthrough has been claimed. Scripted native navigation and bounded gameplay tests are narrower evidence. |

The whole request remains open where this table says unfinished. Installation
of a verified milestone is not acceptance of every original request.

Created 2026-10-04 UTC, before implementation edits. Owner request delegated from thread `01a0f062-a1c6-742a-8919-6555e2ad80d7`.

## Authority and execution boundary

- All inspection, development, asset work, builds, tests and gameplay execution stay on **the development workstation**. No cloud workspace or remote build/test/art fallback.
- Continue `Jedi Academy Remixed (OpenJK Source)`, branch `codex/jar-capacity`. This is single-player OpenJK and is separate from FlavorJK and MCP work. Preserve the entire existing working tree.
- Baseline: installed 0.10.0, build `20261003-122624-2243cfe1`, 25 Release and 25 Debug checks, installed ZIP SHA-256 `69BBCC3B4195C8B6A3B77707D9FBA58163839029BF75599ADDB5769F0D4D6583`.
- Retain existing versions and a new pre-overhaul installed/profile rollback. Stage candidate binaries first. Use `scripts/build-jar.ps1` for the latest verified release's normal installation; verify package hashes and untouched settings, saves and other mods.
- Never terminate/interrupt a running game or deploy into an active install. Native tests use guarded launches, disposable profiles, native console commands/screenshots and scripted clean exits. Do not send global input to other applications.
- No multiplayer cheats, anti-cheat changes, untrusted binary imports, new installations, persistent access changes, destructive resets or external publishing.
- This document tracks the complete overhaul. A tested milestone is not completion of the whole overhaul.

## Complete requirements

| ID | Requirement and acceptance |
| --- | --- |
| UI-01 | Special menu starts in **Simple** mode with useful presets; **Advanced** exposes the existing detailed settings. Keyboard/controller focus order, back/cancel, apply and pending state must be coherent. |
| UI-02 | Rename **Balanced** to exactly **The way it was intended**, make it the default build preset, and define a tested preset matrix. **Chaos** enables every randomizer category. **Vanilla** disables randomizer features while preserving graphics and other preferences. Preserve seed and fixed/per-event mode on preset changes unless the user explicitly changes them. |
| UI-03 | State safety, continuity and experimental recovery behavior per preset; expose dangerous combinations. Do not silently interpret Chaos as disabling mission safeguards. Sandbox remains independently toggled. |
| UI-04 | Carry one new visual/interaction style across main, pause, loading screens/bars, character creation, lightsaber selection, mission/weapon selection, settings and other relevant native menus. Inspect actual native screens and menu definitions first. Reorganize flows instead of only recoloring. |
| ART-01 | Remake the application icon with an aesthetic relation to the Jedi Knight logo. Use an authorized local image-generation workflow for raster art where appropriate. If no local generator is available, record that blocker; do not send art production to a cloud tool. |
| FX-01 | Identify invisible/unreadable random projectile variants and provide readable original visuals. Preserve hitscan/beam versus travelling projectile semantics, ammo, cadence and damage. Qualify both renderers and stock/optional effect packs. |
| MELEE-01 | Analyze native lightsaber and `sith_sword` definitions/mechanics. Expand distinct melee weapon types using permitted existing mod assets and original unique weapons. Record provenance/license per asset; possession does not by itself establish reuse permission. |
| MELEE-02 | Expand carried/equippable inventory and selection. Distinguish more choices from simultaneous wielding; do not assume extra arms. Audit masks, indexing, UI, resource registration and save compatibility before expanding limits. |
| MELEE-03 | Inspect sluggish single-player saber extension and add a faster suitable option/default while retaining needed native behavior and testing scripts, activation and switching. |
| FISTS-01 | Diagnose continuous punching/odd moves in `iknowkungfu`; compare JA+ behavior as a reference. Reuse only licensed/permitted source/assets. No imported untrusted binaries. Test press, hold, release, movement/direction, animation completion and weapon transitions. |
| CAM-01 | Add camera presets/settings addressing sway, disorienting effects and slow chase. Include a crisp multiplayer-like option and selectable legacy behavior. Preserve collision and test scripted cameras, cutscenes, vehicles, zoom and transitions. 1.33 adds shared SP/MP Legacy/Comfort/Crisp camera math and an archived MP selector with Apply/Revert; native view/menu fixtures pass, graphical feel acceptance remains open. SP accepted camera behavior is preserved, broader settings parity remains incomplete. |
| SB-01 | Independent Sandbox toggle and a Garry's Mod-like spawn menu, default Q binding. Preserve the previous Q binding, detect conflicts and restore it safely. No change to Q while Sandbox is off. |
| SB-02 | Spawn catalog for props, entities, NPCs, weapons, vehicles and other qualified categories. Label unsupported/ineligible entries with reasons. Use native engine spawning/precache and bounded resource policies. |
| SB-03 | Original physics-gun/tool-gun equivalents with generic tool selection in the spawn menu. Initial bounded tools: select, move, rotate, freeze, remove. Do not copy proprietary GMod code/assets or promise unavailable rigid-body constraints. |
| SB-04 | Track sandbox ownership independently of authored world entities; undo/cleanup only owned objects. Sandbox off releases controls/held objects and manages owned state safely. Audit save/load, map changes, disconnects, deletion and resource limits. |

## Preset contract to implement and test

The existing Balanced action affects graphics only. New Simple build presets make gameplay behavior explicit. Existing profiles must not be overwritten merely by startup or opening the menu, including omitted CVAR_ARCHIVE_ND defaults. Only an absent profile enables new-profile defaults, using the former Balanced graphics values under the requested new name. Camera defaults to Legacy/custom; Comfort and Crisp are explicit options.

| Selection | Graphics | Randomizer categories | Seed / fixed-per-event mode | NPC mission safety | Continuity / recovery | Sandbox |
| --- | --- | --- | --- | --- | --- | --- |
| The way it was intended | Former Balanced values; default for a new profile | Off, matching the current build's conservative defaults | Keep | On | Off / Off | Keep independent selection |
| Vanilla | Keep all current graphics | All Off; restore owned live values on Apply | Keep | On (protective policy, not randomization) | Off / Off | Keep independent selection |
| Chaos | Keep graphics | All eight Entity, five Sound and six live-Cvar categories On | Keep | On | On / On, explicitly labelled experimental | Keep independent selection |
| Advanced custom | Explicit individual edits | Explicit individual edits | Explicit edits only | Explicit dangerous-Off warning | Explicit edits with limits | Independent |

Selection stages a reviewable change; Apply commits it. Placed entities still require a fresh map. Audio/future shots retain their current supported live semantics. Cvars apply explicitly without a renderer restart. Turning randomization off restores only owned live overrides. Existing save/world mapping safeguards remain.

## Implementation sequence and checkpoint gates

1. **M0 â€” inspection and durable scope (in progress):** read project/local guidance; capture baseline native special/main/pause/character/saber/loading UI where supported; map camera, projectile, melee/fists and spawn architecture; retain 0.10 rollback. Record local art capabilities.
2. **M1 â€” Simple/Advanced and presets (pending):** shared preset policy/default initialization, uncluttered Simple navigation, Advanced entry/return, exact labels, all-category Chaos/Vanilla, safety descriptions, keyboard/controller-compatible focus. Test policy matrix, no-op startup for existing profiles, staged apply/cancel, seed/mode preservation, live restore and native screens.
3. **M2 â€” safe camera and projectile readability (pending):** implement audited independently safe camera controls and concrete identified visual gaps; test production helpers, native movement/cutscenes and renderer/variant coverage. No damage or emitter-semantics change for cosmetic visibility.
4. **M3 â€” verified first release (pending):** Release/Debug checks, native Classic/Rend2, rollback/hash checks, normal install of latest verified milestone. Publish local evidence and clearly list unfinished scope.
5. **M4 â€” complete menu system and icon (pending):** reorganize all native flows using the proven visual system; loading progress, character/saber previews, consistent focus and navigation. Original icon/art subject to the local-only asset gate.
6. **M5 â€” melee, inventory and fists (pending):** provenance/mechanics audit, explicit inventory design and compatible storage, saber-extension option, fist state-machine repair, broader weapon types/assets with focused native regression fixtures.
7. **M6 â€” sandbox ownership and tools (pending):** binding lifecycle, spawn registry/menu, owned-entity/undo model, original tool/physics guns, bounded manipulation and cleanup; tests before widening categories or physics behavior.
8. **M7 â€” integration (pending):** campaign/sandbox/mode transitions, save compatibility, full keyboard/controller paths, performance and resource bounds; latest verified installed release and complete remaining-limit report.

## Initial findings and unresolved gates

- `code/ui/ui_jar.cpp` builds eleven detailed pages; Entity currently has thirteen tightly spaced controls. Native main/pause flow loads separate menu sets in `ui_main.cpp`.
- `code/qcommon/jar_graphics.h` holds the current Balanced/Quality/Maximum FPS graphics matrix. No Simple build-preset abstraction exists yet.
- Native camera defaults include camera damping 0.3 and target damping 0.5; first-person run/bob settings are separately archived. Script camera overrides must remain authoritative.
- Installed 0.10 fixed mapping and optional reroll behavior are retained requirements, not candidates for redesign.
- The exposed image-generation service is not established as local. Do not invoke it for this local-only request. Local asset-generation capability still needs inspection; icon/art remains pending if none exists.
- The local memory summary's older capacity-only/read-only scope is historical and is superseded by the current explicit overhaul request. Its warnings about cross-system capacity changes and source-only versus native validation still apply.

## Checkpoint log

- 2026-10-04: Complete owner requirements captured before implementation. Confirmed host the development workstation, preserved existing dirty branch, read AGENTS.md and local memory. No implementation code edited yet. M0 in progress.

- 2026-10-04 first-release implementation checkpoint: Simple/Advanced, all-category build presets, absent-profile default initialization and optional Comfort/Crisp cameras implemented. Existing dirty work preserved through per-file backups. Read-only review found and fixed restart ordering, omitted native defaults, Intended live-override ownership, Vanilla restore outside a mission and Wampa grab exemptions.
- Native baseline screenshots: main, special, pause, character, saber and the static loadscreen menu captured. The loadscreen capture is **not** a capture of an active loading-progress bar; full loading/flow reorganization remains M4.
- 25 Release and 25 Debug checks pass at this checkpoint. Native Classic and Rend2 menu/camera movement captures completed. Classic normal-map (`helpUsObi=0`) production menu transaction: Chaos plus a video restart retained all six owned live values, including speed 306; Vanilla restored speed 250 and all other owned baselines.
- Projectile audit: Classic executed 109 random shots covering all 78 qualified variants. Every observed travelling projectile had a registered native model or trail. Expected instantaneous beams do not create travelling missiles. No projectile emitter, damage, cadence, ammo, collision or effect was changed. Diagnostic coverage is not visual-readability certification. Identify unreadable effects with frame-level captures and optional-pack comparison before M2 visual changes; FX-01 remains open.
- Local art gate: inspected `~/.codex/skills/.system/imagegen/SKILL.md`. Its exposed generation paths are the built-in service and OpenAI API CLI, not a verified local generator. Neither was invoked. Local original vector/icon work remains possible in M4; new raster generation requires an authorized local inference workflow.
- Camera scope: Comfort uses 0.7 chase/target damping and 25% run/bob motion; Crisp uses immediate chase/target following and no run/bob motion. Legacy retains the user's values. Native collision traces, recoil, fall/step handling, script shake and scripted/entity/vehicle/zoom/creature-grab views are preserved. Full vehicle/cutscene campaign coverage and broader motion options remain open.
- Still unimplemented: coherent redesign of all native screens/loading bars, application icon, new projectile visuals, expanded melee inventory/types, saber-extension changes, fist repair, all Sandbox menus/binding/ownership/tools. No Sandbox control or Q binding is introduced by this release.

- Vanilla follow-up: future random shots now stop immediately when the current projectile toggle is Off. Catalog precache/save readiness remains tied to the qualified world; already-fired missiles are untouched. No visual/emitter/damage change. Production shot tests cover Off/invalid inputs, unchanged sequence/emission count, re-enable in a qualified world and rejection in an unqualified world.

- Rend2 full-cycle audit also completed: 110 shots covering all 78 variants, with no observed missing model/trail render path. Native fresh-profile check selected Rend2/4x anisotropic filtering; the existing-profile omitted-default check retains Classic/16x.

- Final candidate gate: both Classic and Rend2 production UI transactions passed on normal `map t1_fatal` with `helpUsObi=0`. Each retained speed 306 after the requested video restart and restored speed 250 with Vanilla. Final Debug suite: 25/25. Native projectile Off case fired five random shots (sequence 0-4), four native shots with zero random decisions while Off, then four random shots (sequence 5-8) after re-enable. Full native visual/menu redesign remains unfinished.

## Native UI and projectile follow-up, 2026-10-04

- Continue from installed 0.11.0, build 20261004-123807-93dc86f1. Retained all 28 runtime files and both current profile/save files before edits; recorded 66 base PK3 hashes.
- This bounded milestone targets a consistent native visual/layout system for main, pause, loading/progress, character and saber screens, plus evidence-backed cosmetic projectile readability. Preserve native menu actions, live previews, settings/preset transactions, seed mapping and camera behavior.
- Capture before/after with engine screenshot commands and disposable profiles; stage and test Classic and Rend2 before normal verified installation. No global input, cloud art, active game interruption or mod/profile replacement.
- Independent read-only projectile/source investigation continues. Melee, sith_sword, expanded inventory, extension/fists and sandbox stay tracked for subsequent milestones; no untested expansion in this patch.
- Icon raster generation remains blocked by absence of a verified local image generator. Proceed with original code-native UI/effects and installed retail assets only; no new third-party art is imported.

- Native follow-up source review: preserve retail actions/conditions/feeders/model transitions; remove old hover-undertext painting and put hub tab traversal in column reading order. Guard loading theme by native control names **and** TEXTSCROLL type, and retain custom loading background/bar/icon dimensions when the theme marker is absent.
- Proven projectile gap: stock vehicle bomb and ion definitions reference absent `ships/bomb` and `ships/ion_blastershot` files and no model. A nonzero configstring effect index does not prove a loaded effect; the vehicle branch returns before the generic weapon trail. Original, depth-tested core/tail effects now resolve only for these two missing keys. Existing valid, empty or malformed custom definitions retain precedence; original-key registration preserves caches/save reconstruction.
- Future melee/inventory source audit: `STAT_WEAPONS` still uses `1 << weapon` masks (`g_cmds.cpp`); normal selection ends at `MAX_PLAYER_WEAPONS = WP_STUN_BATON` (`weapons.h`). `MAX_SABERS` is two with explicit save-layout coupling (`q_shared.h`), independent of how many choices an inventory could carry. `iknowkungfu` enables debug melee and grants Force upgrades (`g_svcmds.cpp`), not merely fists. Native saber extension changes shared blade length (`cg_players.cpp`) and needs collision/script tests before adjustment. Continuous-punch diagnosis, licensed melee assets, carried inventory and sandbox ownership/tools remain unfinished.


### 0.12.0 candidate qualification

Main/pause, character, saber/saber-style and native loading now share original
code-drawn panels, typography and spacing. Hub actions are arranged in deliberate
keyboard traversal order. Native scripts, cvar conditions, preview/feeders and
campaign gates are retained. Stage progress uses the actual bounded engine stage.
Custom layouts without qualified anchors and same-name controls of the wrong
type remain untouched; native custom-content testing confirms this boundary.

25 Release and 25 Debug checks passed. Four renderer/effect-pack combinations
exercised the corrected two-primitive bomb/ion fallbacks, fixed selection and
save/load. Both normal-map renderer preset transactions preserved the Chaos
restart/live-value contract and Vanilla restoration. Native shell, pause,
loading and open-air projectile captures are retained with scripts and logs.
The old generic trail diagnostic did not establish loaded vehicle visuals;
the missing-file source audit and actual effect registration are the basis for
this bounded repair. Full-catalog visual readability remains open.

Completed through these two releases: UI-01/02/03, bounded CAM-01 presets, and
this subset of UI-04/FX-01. UI-04 still needs mission/weapon selection, remaining
settings/native flows and full keyboard/controller/campaign walkthroughs.
ART-01 remains gated on a verified local raster-generation workflow; no cloud
generator was used. MELEE-01/02/03, FISTS-01 and SB-01/02/03/04 are source-audited
future work, with no implementation claimed here. Expanded carried choices must
not be implemented by blindly increasing MAX_SABERS or weapon-mask limits.


## Melee/fists follow-up from 0.12.0, 2026-10-04

- Retained the verified installed 0.12.0 runtime/profile rollback before edits.
  This release targets reliable repeated player punches and a faster selectable
  saber-extension rate. Preserve alt-kicks/grabs, NPC behavior, scripted views,
  native maximum reach/damage/collision checks, Simple presets and saved worlds.
- Audit `iknowkungfu` as the existing developer command that enables debug melee
  and Force powers; do not turn that command into an automatic gameplay grant.
  New fist behavior must be selectable with Legacy retained and release must not
  queue additional punches. New extension behavior must have a clear default and
  leave solid swords, NPCs and protected/scripted cases on their existing path.
- Before changing inventory, document stable carried-item identities, bounded
  selection, native equip adapters, old-save migration and a versioned save
  extension. Do not change weapon enums/masks or MAX_SABERS in this milestone.
- JA+ is a behavioral reference only unless source and reuse terms are verified.
  JA++ is a separate project. No third-party binary/animation/source import is
  authorized by the mere presence of a download or installed mod.
- Remaining native UI flows, broad projectile readability, full camera campaign
  coverage and icon work remain active; the local raster-generator gate remains.
  Sandbox Q binding/restore, qualified spawn catalog, independent ownership,
  undo/cleanup and original manipulation tools are still required future work.

### 0.13.0 bounded melee implementation checkpoint

- MELEE-03/FISTS-01: Continuous humanoid primary punches and Quick held energy
  blade extension implemented with archived Legacy choices under Styles.
  Primary follows completed animations; alternate/grab, NPC/creature and
  protected saber paths remain native. Release has no queued-punch state.
- 27 Release/27 Debug checks and Classic/Rend2 native hold, release, transitions,
  save/load and single/dual/staff/sword activation fixtures completed. Independent
  read-only review's creature-scope finding was fixed before release.
- MELEE-01/02: stock sword audit and save-compatible carried-inventory design
  recorded in JAR-melee-inventory-plan.md. No inventory expansion, extra limbs,
  new melee assets or JA+ imports are claimed in this milestone.
- Next milestones remain full native UI flows, camera/campaign/controller and
  broader projectile qualification, implemented inventory and additional melee
  types, plus Sandbox SB-01 through SB-04 (independent toggle/Q binding lifecycle,
  qualified catalog, owned-object undo/cleanup and bounded original tools).
  Raster icon work remains gated on a verified authorized local generator.


### Inventory and Sandbox continuation from 0.13.0

Authorized next scope: implement a bounded carried-inventory registry and native
selector with stable IDs and compatible save migration, then original melee
choices using qualified installed assets. Keep native weapon families, possession
masks and two equipped saber slots intact. No automatic campaign grants.

Build a functional first Sandbox slice: independent toggle, conflict-aware Q
menu/binding restoration, explicitly qualified spawn categories, owned-object
registry and undo/cleanup, plus bounded original selection/manipulation tools.
Do not label a static menu or unowned console spawns as a completed Sandbox.
Each category must be usable or show its concrete qualification limitation.
Preserve 0.13.0 rollback, user settings/saves and existing working-tree changes;
verify old-save behavior, map/load lifecycles, both renderers and installed build.
The complete overhaul's other UI/projectile/camera/controller/icon work remains.


### 0.14 carried inventory and functional Sandbox slice

Implemented a 64-choice carried registry and native selector without widening
weapon IDs, masks, ammo pools or simultaneous saber slots. Old saves migrate;
new bounded versioned chunks preserve inventory and valid full-save ownership.
Map transitions and real entry autosaves transfer inventory without orphaned
spawn claims. Shortblade and energy pike are original gameplay definitions using
installed retail models; the native Sith sword remains mechanically distinct.

Sandbox now has an independent toggle, conflict-aware Q menu and restoration,
two qualified crates, E-11/repeater pickups, 32-object ownership, undo/cleanup,
aim selection, grab/move, rotation, freeze, gravity release and removal. This is
a usable bounded slice, not a complete general physics sandbox. NPCs/vehicles,
richer physics, a wider catalog, independent weapon copies/ammo, live protected
combat and physical controller/full campaign qualification remain.

Candidate checks passed both renderers, normal pickup/fire/expiry, old/new saves,
transition/entry autosaves, exact explosive selection/ammo and unavailable asset
rejection. Thirty Release/Debug checks cover state/codec, native action guards,
real menu dispatch and Q ownership. See [controls and compatibility](JAR-inventory-sandbox.md)
and the 0.14 evidence archive for exact installed-build verification.

Remaining full-overhaul work still includes other native menu flows, broader
projectile readability/performance, camera/vehicle/script/campaign/controller
coverage, more melee/catalog depth and the application icon. The local raster
generator remains unavailable. No cloud generation or JA+ code/assets were used.


### 0.15 Sandbox actor and tool milestone, 2026-10-05

Resumed on the development workstation with the 0.14 runtime, current settings/saves and existing
working tree retained. No cloud execution, raster generation, installations,
privileged helpers or multiplayer changes were used.

SB-02/03/04 now include the qualified stormtrooper, rebel, R2, Swoop and tauntaun
catalog; native gun/explosive and supply pickups; actor navigation-helper and
death-drop ownership; occupied cleanup deferral; and separate Physics gun and
Toolgun inventory choices. Mounts wait for a rider and keep native controls.
Save V2 preserves tool/drop identities and the on-demand vehicle registry by
name. This fixes a reproduced occupied-tauntaun reload crash caused by an absent
native registry entry. Old native/0.14 formats remain readable; native structures
and masks are unchanged. See JAR-inventory-sandbox.md for exact boundaries.

Release and Debug each pass 30 checks. Native candidate checks cover both
renderers, NPC death/drop and helper cleanup, full actor/vehicle saves, occupied
mount save/load/driving/exit, tool ammo isolation and release latches, and 0.14
carried-record migration. Exact installed-build evidence is recorded separately
under build/validation when packaging and installation finish.

The full overhaul remains active: remaining native mission/weapon/settings
flows, controller focus and loading presentation; broad projectile readability;
camera/fist/action-state integration; and an original local application icon.
Physical-controller use cannot be inferred from scripted input. Local vector
work remains possible; no verified local raster generator is available.
General rigid-body constraints, aircraft/boss qualification and independent
per-copy weapon ammunition are outside this bounded milestone.

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
