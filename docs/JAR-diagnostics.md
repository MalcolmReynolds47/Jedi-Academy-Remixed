# JA:R diagnostics (0.2.1)

Logging is automatic. The default profile is **Documents/My Games/JediAcademyRemixed** (Windows may redirect Documents to OneDrive). Logs are in its **diagnostics** folder. A custom `fs_homepath` changes this location. `jar_diagnostics` prints the active file, level and write status and captures a settings snapshot.

The recorder writes JSON Lines with schema, session identity, sequence, elapsed milliseconds, process/thread IDs, frame, server time, category, event and detail. Each record is written synchronously without a user-space buffer, so an abrupt process exit need not lose the last operation. This is not a guarantee against power loss or storage failure. File-open/write failures disable the recorder without terminating the game.

There are eight rotating files, `jar-diagnostics-0.jsonl` (current) through `-7.jsonl`, each at most 4 MiB. Startup also rotates, preserving previous-session context. Rotation bounds total JSON logs at 32 MiB; it does not retain every session indefinitely. Up to 2,000 ordinary events per second are recorded; a `rate_limit` event reports omitted events at the next active window, and session end reports outstanding omissions. Errors bypass this rate limit. High-volume traces can evict older history sooner.

## Controls

| Console command | Effect |
| --- | --- |
| `jar_logLevel 0` | Errors, session/build metadata and explicit snapshots only. |
| `jar_logLevel 1` | Default: console output, relevant cvar changes, renderer/model lifecycle, NPC spawn/weapon attachment, map/save transitions and five-second performance/drop summaries. |
| `jar_logLevel 2` | Adds entity allocation/free, bone-cache lifetime, projectiles and explosion/chain events. |
| `jar_logLevel 3` | Adds individual sound/effect requests and deferred NPC AI decisions. Use for short reproductions; synchronous tracing has a performance cost. |
| `jar_mark before spawning stormtrooper` | Adds a searchable marker and settings snapshot. |
| `jar_diagnostics` | Reports logging status and records a snapshot. |

No renderer restart is required to change the log level. Logging does not select a graphics preset, enable reduced AI or alter random-number state. Only relevant gameplay/graphics/audio cvars and selected map/NPC/save/restart commands receive structured tracing. Console output is also captured, including text printed by mods.

## Recorded context

- Build version and unique build ID; profile, game and base paths; capacity constants.
- Active and pending graphics cvars, renderer loads/module addresses, video restart boundaries, GPU details printed by renderer initialization.
- Ghoul2 slot IDs/generations, availability, model filenames, owner/vector/cache pointers, sizes/capacities and before/after mutation events in both renderers. These identify the last completed step; they are not a complete CRT heap allocation history.
- NPC type/entity and weapon model attachment, entity population/high-water summaries, capacity rejection, optional entity allocation/free traces.
- Cumulative cosmetic effect, scheduled effect, loop, sound-channel drop/replacement and local-entity recycling counters; skeletal surface pressure; snapshot overflow errors.
- Frame interval mean/worst over five seconds. These include frame caps, VSync, pauses and loading; they are not isolated CPU/GPU timings or a benchmark.
- Save/map transitions, gameplay cvar changes, optional projectile/explosion events, and AI deferral details.

`jar-native-exceptions.log` records selected native exception codes and instruction addresses through a minimal Windows vectored handler. It continues the normal exception chain and never attempts recovery. A handled exception may appear here without a crash. Fail-fast termination can bypass handlers; the JSON trail and Windows crash dump are still needed. The previous native-exception file is retained as `.previous.log`. This release does not change Windows Error Reporting registry settings or promise a dump for every crash.

## Collecting a crash

After the crash, run the installed `JediAcademyRemixed/tools/collect-jar-diagnostics.ps1` in PowerShell. It creates a local ZIP under the profile's `diagnostic-bundles` folder with the retained logs, build information, installed binary hashes and newest available `jar_sp.x86_64.exe.*.dmp` from `%LOCALAPPDATA%/CrashDumps`. The collection manifest preserves source timestamps so an old dump is distinguishable from the current crash. Use `-NoDump` for logs only, and `-ProfilePath` / `-GameDataPath` for custom paths. It does not launch the game or upload anything. Crash dumps can contain process memory; inspect the bundle before sharing it externally.

Builds retain a separate ZIP under the source workspace's `build/symbols/`, keyed by build ID, containing matching optimized binaries/PDBs, runtime hashes and source provenance. Keep that ZIP when reporting crashes from an older build. Symbols are not installed into GameData. Match build IDs and module hashes; Debug symbols cannot be substituted for Release symbols.

## Future feature instrumentation

The engine's `JAR_Log(level, category, event, format, ...)` is exposed through the game and renderer import tables. Use paired `*_begin` / `*_end` events around risky operations, with entity/model handles and stable decision IDs. Future randomizer events should use category `randomizer`, and record feature/subsection, seed, decision index, original asset/value, chosen replacement, eligibility/exclusion reason and whether a value was applied or latched. Log the already-selected result; never call an RNG merely to produce a log. Pickup randomization now uses these hooks; see `JAR-randomizer.md`. Other randomizer categories remain deferred.

Logging and symbol retention improve diagnosis. They do not fix the previously observed Rend2 model-list double free.

## Native loading-frame capture (0.12.0)

`set jar_captureLoading 100` arms one native `screenshots/jar_loading.png` capture
on the hundredth loading-screen refresh of the next map. Values 1-1024 are
bounded counters, not loading stages; 0 disables. This setting is not archived.
The command runs after native drawing and before renderer EndFrame. It does not
pause the game, inject input or alter loading progress. Rend2 capture is asynchronous;
allow subsequent frames before quitting. Use a disposable profile for validation.

`fx/missing_media_fallback` records the original effect key, private fallback
file, registered template ID and primitive count. Generic weapon trail pointers
in `fx/projectile_visual` do not certify a vehicle effect: vehicle rendering can
return before invoking that weapon callback. Validate the actual effect and image.
