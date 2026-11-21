# JA:R single-player randomizer

Open **JA:R Settings** and select **Randomizer**, **Sound** or **Cvars**. All three pages share the seed and
**Randomizer mode**. **Seed-fixed mapping** (`jar_randomMode 0`) is the new default;
**Per-event reroll** (`jar_randomMode 1`) retains the earlier varying selections.
The category toggles and eligible destination pools are shared by both modes.

### Fixed source identities

| Category | Fixed identity and behavior |
| --- | --- |
| Effects, UI, loops, dialogue | Canonical logical audio path; ASCII case/slashes and the native WAV/MP3 suffix aliases normalize to one source. The same source shares its replacement across enabled roles. |
| Music | Resolved native music file, including a map's default explore track; same audio namespace as sound files. |
| Embedded movie audio | Canonical ROQ filename; one choice pinned through the playback and rewind, preserving the original movie clock. |
| Projectiles and beams | Held weapon name plus primary/alternate fire mode. Holding a charge longer does not reroll the replacement. Destination charge tiers remain in the complete pool. |
| NPC spawns | Resolved original NPC template name, after native class wrappers. Mission safeguards and source allegiance still apply. |
| Pickups | Original qualified item classname; conservative categories and the wide pool have separate namespaces. |
| Decorative props | Original fingerprinted model path; only fitting, unlinked candidates qualify. |
| Bonus arsenal | Three stable player-arsenal slots, removing each selected gun before choosing the next; no duplicate guns, independent of map. |
| Live cvars | Target cvar name. Each seed selects one bounded value, or one stable fraction of the authored gravity/speed baseline. Reapplying does not compound. |

Fixed choices do not use map names, spawn ordinals, entity numbers, playback
order, wall-clock time, gameplay RNG or process-dependent hashes. A versioned
namespace and explicit unsigned byte hashing define them. Entity/projectile
candidates are ranked by canonical name with a lexical tie-break; audio uses a
canonical sorted/deduplicated catalog. Reordering input catalogs does not change
fixed results. Changing eligibility, installed assets or a pool option can change
the mapping. Changing a seed recomputes choices; small pools and hash collisions
mean a particular replacement can stay the same. The mapping is not a bijection.

### When changes take effect and what saves preserve

Placed entities and bonus arsenal use applied settings on a fresh mission or the
confirmed **Restart level with applied settings** action. Full saves retain their
serialized pickups, NPCs, identities and inventory. Static props, which the
engine rebuilds from the BSP, use the saved `jar_worldSeed` and `jar_worldMode`.
They therefore keep the loaded world's appearance even if future settings differ.
Saves predating 0.10 use the legacy world-mode fallback for rebuilt props.

Future shots and sound events use the current seed/mode. Existing one-shots finish
naturally; a movie keeps its pinned choice until the next playback. Music refreshes
on menu Apply (`jar_audio_refresh` from the console); active loop lookups use the
current settings. Initial projectile activation still requires a qualified applied world. Turning
projectile randomization Off stops future random shots immediately; existing
missiles retain their native behavior and qualified resources remain available.
Live cvars require **Apply live roll**; loading resumes saved overrides once.

New cvars are archived and saved; old profiles/saves lacking `jar_randomMode`
default to fixed mode without rewriting unrelated preferences. Explicit mode
choices persist in new saves/profiles. Existing categories stay enabled/disabled
as selected; a new install starts with all category toggles Off. **NPC mission
safety defaults On**. Entity presets retain seed/mode, reset mission safety On,
and reset named-ally continuity/recovery Off. Pickups enables the three ordinary
pickup categories and All Entity enables the Entity features. Sound and Cvars have
their own section controls and restore actions. Apply commits pending changes for
the current settings. **Back / Cancel** discards pending selections. Sandbox is
available from its own button in JA:R Settings.

## Complete mounted audio catalog

`jar_randomEffects`, `jar_randomUI`, `jar_randomLoops`, `jar_randomMusic`, and
`jar_randomVoices` select source roles independently. Every enabled role draws
from the same catalog: character dialogue, effects, interface sounds, ambience,
loop files, music and embedded ROQ movie soundtracks. There are no role-specific
destination whitelists. Dialogue / movie audio controls voice events and audible
embedded cinematic streams; sidecar sounds retain their native channel role.
**Sound > All on > Apply** enables dialogue too.

Discovery traverses mounted PK3s and loose files without the old 4,096-file list
limit. Native filesystem precedence resolves duplicate names. WAV PCM (8/16-bit,
mono/stereo, 8–96 kHz) and MP3 accepted by the native decoder are converted to
mono PCM at the mixer rate. Validated ID3v2.2/2.3/2.4 metadata is skipped before
the native MPEG probe. Native mono/stereo ROQ DPCM audio is extracted while video
chunks are skipped in bounded buffers. Unsupported encodings/extensions,
malformed files, WAV/MP3 files over 32 MiB, ROQ files over 512 MiB, and decoded
sounds over 64 MiB have explicit rejection reasons. Packed ROQ containers are
unsupported. Silent ROQs report no embedded audio; they do not become silence
destinations.
The 128-slot, 128-MiB cache pins active sounds; pressure falls back to native audio
instead of freeing a playing sound. OpenAL does not support this PCM substitution
path and retains native playback; the normal SDL/DMA backend is supported.

The 0.9 mounted the development workstation audit discovered **15,024** unique entries and decoded
**15,017**, including all 13 audible stock movie soundtracks. One malformed WAV
(`sound/nomad/szico_nomadflush.wav`, odd byte count for mono 16-bit data) is rejected.
Six ROQs have no embedded audio (three holonews textures, two opening crawls and
`video/racepack10/phoony.roq`). The previously rejected tagged
`sound/chars/jedi_spanki/misc/combat1.mp3` now decodes; the installed ID3v2.2 music
file also retains support. The earlier 0.7 stock-only audit covered 13,588 WAV/MP3
files; it did not include ROQs. Mounted content and filesystem precedence determine
other users' counts. Adding movie destinations changes seeded selections relative
to 0.7/0.8; runs remain deterministic with the same build, settings and catalog.

In fixed mode, each canonical source starts the same deterministic candidate
walk. Decoder/format rejections advance that walk; transient cache pressure
returns native playback without selecting a different replacement. Both modes
retain the bounded 64-attempt fallback and 128-slot/128-MiB cache.

In per-event mode, one-shots use the existing seeded full-cycle permutation,
visiting every catalog slot before repetition (unsupported slots are skipped).
Its saved audio counter continues across full saves; event order and enabled
roles affect that sequence. As before, loops and music keep stable source choices
and movies pin one choice, avoiding a reroll on every mixer packet/frame.

Dialogue keeps the authored speaker handle, captions, script-control identity and
duration. Long replacements are cut at that deadline; short clips have a silent
tail. Cutscene stop commands stop their logical source without freeing PCM used
by another channel. `sound/null.wav` remains silence. Musical replacements loop a
PCM selection and honor silence; authored explore/action/boss transitions are
replaced rather than reproduced. Blank campaign music fields resolve through the
native map-default set; invalid labels and absent files retain native fallback.
Original soundtrack behavior returns on restore.

Embedded movie audio uses one seeded choice per playback, pinned for its lifetime.
Rewinds retain that choice; separate playbacks of the same movie use its stable
mapping. Cumulative sample conversion preserves original packet duration, including
uneven chunks. A short clip ends in silence and a long clip stops with the movie.
Native silent-video flags, video clocks, held frames and sidecar playback remain
native. Restoring sound roles returns subsequent embedded packets to original audio.
Sound restart releases pinned PCM safely and uses native audio for the rest of an
already playing movie.

`jar_audio_catalog` writes `jar-audio/catalog.tsv`, with supported/pending/rejected
states and reasons. `jar_audio_validate 32` audits the next 32 entries (repeat to
complete a catalog). `jar_audio_restore` disables all five roles and refreshes
native music; current one-shots finish naturally. No renderer restart is needed.

## Per-shot projectile and beam catalog

`jar_randomProjectiles` maps each eligible held weapon/fire mode to one emitter
in fixed mode, or chooses a new emitter per shot in per-event mode. On the tested install,
**78 variants** cover pistol charge tiers, blaster modes, bowcaster volleys,
repeater, DEMP charge levels, flechette, ordinary/guided rockets, concussion,
disruptor beams/charges, thermal modes, trip mines, thrown detpacks, AT-ST,
emplaced and bot bolts, Tusken rifle, scepter, remote/seeker/probe/sentry/Mark
payloads, Noghri gas, turbolasers and **12 vehicle weapon definitions**.

All eligible held guns use the same destination pool. The per-event mode's
seeded permutation visits all entries before repetition. Held ammo costs, cadence, inventory, animations
and AI alerts remain native. The temporary emitter/charge/lock state is restored
after each event. Guided variants acquire a live hostile under the native crosshair
trace instantly; without one they fly unguided. Neutral targets are excluded.
Beams are explicitly included; saber/melee swings, mounted/vehicle-controlled
players and remote detpack detonation remain their native control actions.

Two discovered entries are unavailable in this installation: Jawa ion fire lacks
a complete native item/render model, and the vehicle mine definition has zero
projectile speed. Aliased TIE/rapid-fire gun modes reuse their native emplaced or
repeater emitter; they are not advertised as distinct physics. Logs record every
admitted/rejected entry. Vehicle registry indices retain native precache order;
saved configstring resources are rebound after loading.

`jar_projectileSequence` and `jar_projectileSequenceSeed` preserve per-event
progress through full saves, including load and immediate resave in a new process.
Fresh maps or a changed seed reset that cycle. Fixed shots do not consume the
counter. Installed definitions and eligibility can affect either mapping.

## Broad NPC replacements and mission safety

`jar_randomEnemies` draws from loaded NPC definitions, after native class wrappers
resolve the source template. It changes the template at existing spawn locations;
it does not scatter new actors. Native AI/model/loadout behavior comes from the
replacement. The current installation supplies 246 definitions, with 193 viable candidates.
The catalog applies native first-definition precedence, validates
class/team/bounds and model availability, and rejects missing explicit custom
skins. Default GLM embedded materials remain supported (stock sentry has no
separate default skin). Vehicles, player templates and unsupported MD3-only
templates are excluded with reasons.

With **`jar_npcSafety 1` (default)**, mission metadata, actor script names,
parameters, key messages, callbacks, flags, allies and neutral sources remain
protected. Replacement dimensions must fit the source. With **0**, marked
**Experimental OFF** in the menu, named/scripted and allied sources may change
and larger creatures may be selected. This can break routes, animations, boss
mechanics or mission logic even though names remain intact.

Spawner trigger names and actor `NPC_targetname` / `targetname` /
`script_targetname` relationships, parameters, targets and camera groups remain
native. Source allegiance is retained, including native class-derived enemy-team
rules. Packed existing saved flags preserve allegiance without changing entity
or save layouts. Fixed decisions use the original template identity; per-event
mode retains the seed/map/resolved-spawner-ordinal algorithm. Neither consumes
gameplay RNG; actor AI itself remains nondeterministic.

**`jar_actorContinuity 1`** optionally prevents unplanned lethal damage to eligible
living, named allied NPCs before death can discard their ICARUS/navigation state.
Authored script kills, death callbacks, objective death targets, vehicles and
scheduled removals retain their normal behavior. It does **not** resurrect
already dead/deleted actors, override authored removals, complete arbitrary tasks
or skip scenes. Retroactive reconstruction of actors already absent before recovery enrollment remains unsupported.
`jar_npc_status` provides read-only camera/actor identity, health and team evidence.

## Missing ally script recovery (0.8.0, experimental)

`jar_actorRecovery 1` must be enabled **before the NPC spawns**. Default is Off;
presets leave it Off. It is independent of `jar_actorContinuity` (damage prevention).
An enrolled actor that later dies or is directly deleted keeps its native entity
slot, ICARUS owner, navigation controller, model animation state, names, parms,
relationships and pending task IDs. Its visible/physical actor is removed; the
controller becomes an invisible, untargetable, unarmed stand-in. It continues the
actual ground-navigation, animation and dialogue callbacks, including authored
duration waits, without falsely completing arbitrary task groups or skipping scenes.
Turning the option Off does not discard already retained controllers.

The first contract covers named, allied ground humanoids with valid script owners
and native temporary navigation goals. Original spawner death scripts/targets and
later death callbacks remain authoritative. Authored ICARUS KILL, REMOVE, zero
HEALTH, positive DAMAGEENTITY and SAFE_REMOVE bypass recovery. Unenrolled actors,
corpses already missing before opt-in, enemies, vehicles, mounted/held actors,
flying/custom-gravity templates, player-clone weapon-matching controllers,
player-controlled NPCs, squads/troops, pending behavior/resize gates, disintegration
and dismemberment are outside this contract. No reconstruction from guessed names
or blanket resurrection occurs.

Stand-ins do not touch triggers, activate mover contact callbacks, kick other
actors or damage breakables through impact. World collision remains enabled so
authored navigation still has geometry to follow. Scriptable supported states are CINEMATIC and DEFAULT. Existing internal WAIT
states retain their native pause behavior and temporary/default-state precedence;
DEFAULT otherwise remains within the bounded cinematic controller. BS_WAIT is
not exposed by the native ICARUS name table and is not added as a new script API. A new unsupported combat/jump behavior request is
logged as `actor_recovery_unsupported` and retains its pending gate; it is not
silently declared successful. Arbitrary scripts can therefore still stall, and
campaign-wide softlock freedom is not claimed. A scene may show an absent actor
while retaining its dialogue, captions and script progression.

### Save contract

Entity enrollment/stand-in markers use existing serialized flag bits; no entity
layout changes. Saves carrying recovery state use header `0x4a415201` (v1), which
0.7 and earlier reject before reading the game state. New builds also accept the
native header (101); saves without recovery state retain it. Full saves restore
the original controllers/tasks and reapply their noncombat constraints. Partially
played audio samples are not serialized; the native unloaded-voice completion rule
still applies after loading. Native
map shutdown/load cleanup explicitly releases controllers.

The x64 native save writer previously nulled a four-byte ICARUS handle with an
eight-byte pointer write, erasing the adjacent voice task ID. The writer now uses
an explicit integer field rule. This fixes newly written saves for normal actors
as well as stand-ins. It cannot safely infer a voice ID already lost in an older
save. Keep older saves and the retained 0.7 runtime when testing this experimental
feature. Autosave/map-transition behavior follows the native game's reconstruction
rules; full saves are the qualified way to retain active controllers and tasks.

`jar_npc_status` reports camera state, identity, recovery state, ICARUS owner,
individual pending tasks, animation timers, friction and first script parameter.
`g_ICARUSDebug 4` adds native task-completion and group save/load diagnostics.
`jar_test_actor_delete <exact targetname>` is a single-player, cheat-gated test
command that exercises unplanned deletion of that exact NPC.

### OpenAL status

The shipped Windows x64 SP target excludes `USE_OPENAL`; setting `s_UseOpenAL`
cannot enable it. The installed AMD64 OpenAL Soft 1.23.1 DLL provides a possible
local development dependency, but its device API has not been qualified here.
This is a backend implementation gap, not a missing-package request.

The existing SP backend must be ported/enabled for x64, isolate legacy EAX and
device selection, retain bounded shared PCM for the unified sound/music catalog,
detach queued/finished buffers safely, and implement authored voice deadlines and
source-clock lipsync before it can satisfy the unified all-audio scope. An OpenAL
output transport behind the existing software mixer is another possible direction,
but would not provide native per-source OpenAL spatialization. Neither is claimed
as implemented or validated. Unified randomization currently uses the software
mixer/SDL path on both renderers; no new dependency was installed.

## Existing pickup and decoration behavior

Weapons, ammo and supplies retain conservative script/target/quantity/model/bounds
checks; keys, sabers and mission tools remain authored. Cross-category pickups
allow ordinary items to become fitting guns, explosives, ammo or supplies. Bonus
arsenal adds three seeded ranged weapons at level entry while retaining existing
inventory and unarmed/training protections. Full-save loading does not refill it.

Decorative props remain a separate, bounded pool of 14 fingerprinted static MD3
models. Linked/scripted/scaled props, architecture, collision, breakables,
switches and objectives are not arbitrary substitution candidates.

No multiplayer, anti-cheat or network behavior is changed. Campaign-wide
softlock freedom and arbitrary installed-mod compatibility are not established.

## Cvars section

Select features, then **Apply live roll** during a mission. Fixed mode uses the
seed and target name; its fraction/value is independent of map and apply order.
Gravity/speed remain relative to that map's authored baseline for progression
safety, so different baselines can produce different absolute values.

Per-event mode rerolls on each successful explicit Apply transaction, using a
saved seed/counter plus map and target. Changing the seed restarts that sequence;
loading restores the selected values without rolling again. Both modes reuse
owned baseline snapshots, so repeated applies never compound. There is no
periodic enforcement and no protected/restart-required cvar enters the pool.

| Toggle | Live target / bounds |
| --- | --- |
| `jar_randomGravity` | `g_gravity`: 75-100% of baseline, within 100-1200 |
| `jar_randomSpeed` | `g_speed`: 100-125% of baseline, capped at 425 |
| `jar_randomBrightness` | `jar_sceneBrightness`: 0.85-1.25 (scene dim/white lift) |
| `jar_randomFov` | `cg_fov`: 80-105 |
| `jar_randomSky` | `r_fastsky`: 0 or 1 |
| `jar_randomLights` | `r_dynamiclight`: 0 or 1 |

Gravity never exceeds the current authored baseline. Movement speed does not
alter `timescale`, cinematic skipping or Force-speed timing. Unsupported/custom
baseline values are skipped. Both Classic and Rend2 consume the selected graphics
settings live. Brightness is a bounded scene fade before the HUD, using the shared
2D draw path. It does not rely on hardware gamma or rebuild textures; native
`r_gamma` remains the user's preference. HDR, SSAO, framebuffer formats, resolution, shadow pipelines and
other restart-required controls are outside this pool; runtime latch/init flags
also prevent accidental selection. Apply never queues a renderer or game restart.

**Restore originals** restores only values still owned by the randomizer and
disables the section's toggles. Later script/manual changes take precedence.
Current values are displayed separately from feature selections. Original graphics
preferences are written to the config while a temporary roll is active. Saves
store only active restore snapshots; ordinary saves do not acquire new ownership
of graphics preferences. Saved rolls resume once after normal game initialization,
with the graphics restore baseline rebased to current preferences. This works in
normal non-cheat games; no cheat flag, multiplayer code or anti-cheat behavior is
changed. A normal new map may reset authored physics; Apply explicitly to roll it.
Console: `jar_cvar_apply`, `jar_cvar_restore`, `jar_cvar_status`.

All category toggles default Off. Legacy saves clear missing expansion switches
and snapshots; explicit saved switches remain. Per-event mode retains the v1
pickup/Chaos selection algorithms and their qualified pools. Fixed mode
intentionally changes those selections to follow source identity. Full campaign completion across all seed combinations is
not claimed; protected boundaries deliberately leave unsupported entities alone.

## Audio diagnostics

`jar_audio_capture <name> <seconds>` records 1-20 seconds from JA:R's own software
mixer as stereo 16-bit PCM under the active profile's `jar-audio/<name>.wav`.
Names contain only letters, numbers, underscores or hyphens; existing files are
not overwritten. `jar_audio_capture_stop` finalizes an early capture. This captures
game audio only, not microphone/system/other-application audio. It is an optional
diagnostic; OpenAL does not feed the software mixer capture. No recording is active
by default.

## Campaign qualification and limits

The 0.9 static audit covers all 34 shipped single-player BSPs, 1,986 NPC spawners,
and 2,595 mounted compiled scripts (zero parse failures). Its per-map script graph
starts from entity script fields and the map directory, then follows script-valued
references, including shared/common scripts. It is a conservative reference audit,
not an execution trace. All effective shipped BSPs match stock; no loose BSP/IBI
override was found. 1,395 spawners carry nonordinary metadata that the conservative
production policy rejects before ordinary replacement. Other spawners still face
native template, allegiance, vehicle and bounds checks. See JAR-campaign-audit.csv.
Potential lifecycle-operation counts include SET_HEALTH (including healing),
death-hook assignments, KILL/REMOVE and related setters; they are not counts of
actual deaths. The T1 Sour native test verifies both real Chewie spawners remain
original and observes intro doubles; it does not test Chewie death recovery.

| Scope | Implemented / directly qualified | Remaining limit |
| --- | --- | --- |
| NPC mission protection | Default On; production decisions tested on Vjun II, Taspir II, Korriban II and T1 Sour, including named allies/bosses and ordinary replacements | Static audit spans 34 maps; full mission traversal and every mod combination are not tested |
| Unexpected ally loss | Opt-in before spawn; native Taspir Rosh deletion and Korriban Luke death retain exact controllers; recovery-bearing full saves reload | Eligible ground allies only; supported script tasks only; no universal softlock recovery |
| Authored death/removal | Actual Korriban boss death runs its ending script; actual Rosh stand-in still obeys REMOVE; Vjun Kyle's death-hook actor remains excluded | A stand-in cannot guess or fulfill an unsupported combat/mechanical task |
| Older already-missing actors | Native old-format save/load remains supported; tested unexpected and authored losses stay absent even after opt-in | G_FreeEntity discards entity/NPC/client/ICARUS identity and task ownership; ordinary saves record only surviving entities. BSP references do not prove current story state, so no retrospective resurrection is attempted |
| Available audio routes | DMA effects, UI, loops, music, voices, audible ROQ packets and existing sidecars share the supported catalog; default map music and movie destinations tested natively | One malformed mounted WAV; six silent ROQs; unsupported codecs/packed containers and configured resource bounds retain explicit fallback |
| Audio backends | Current Windows x64 SP target uses SDL/DMA; both Classic and Rend2 tested | OpenAL is compiled out of this target. Its installed DLL does not make it an available playback path; porting it is separate work, not required for current audio coverage |

No new user decision is required for this release. Recovery remains experimental
and Off by default; mission safety remains On. Existing profile choices are retained.
