# Owner acceptance and multiplayer direction, 10 October 2026

The owner reports testing all 32 annotated items below and being satisfied with
them or finding that they do not cause a problem. This closes their outstanding
owner-acceptance requests for the installed single-player experience. It does not
invent benchmark numbers, exhaustive campaign coverage, or multiplayer evidence.

| Annotation | Accepted item |
| --- | --- |
| 1 | 16x capacity and controlled exhaustion |
| 2 | Renderer/graphics improvements |
| 3 | Optional distant/inactive AI optimization |
| 4 | Custom resolution, automatic r_mode and desktop borderless |
| 5 | UI/text/console customization |
| 6 | HUD choices |
| 7 | Selectable saber textures/sounds and combat effects |
| 8 | Native menu redesign and loading descriptions |
| 9 | More melee weapons and carried choices |
| 10 | Faster saber extension and repaired fists |
| 11 | Camera presets |
| 12 | Facial presets and speech sensitivity |
| 13 | Sound-correlated facial variation |
| 14 | Independent Sandbox and held-Q overlay |
| 15 | Physgun wheel inversion, distance, freeze and flinging |
| 16 | Physgun USE-key rotation |
| 17 | Harmless Force-grip-style holding |
| 18 | Smoother physics |
| 19 | Prop mass, actor collisions and player support |
| 20 | Collision sounds |
| 21 | Very high fixed prop limit |
| 22 | Toolgun alternate fire, firing-only effect and sound |
| 23 | Compact tool list, right-side properties and top-left help |
| 24 | Duplicate tool |
| 25 | Thruster tool |
| 26 | Undo, cleanup, saving and transitions |
| 27 | Walking/running leg jitter |
| 28 | Hundreds of det packs crash |
| 29 | Crowds mix models/textures/animations |
| 30 | Pistol/E-11 missing sound |
| 31 | OpenJK/EternalJK startup errors |
| 32 | Earlier Rend2 model-list double free |

Walking and the annotated historical crash/startup reports are accepted as
nonproblematic; do not keep asking the owner to retest unchanged implementations.
The Noghri attack and Desann/Mon Mothma/RebornBoss spawn reports were not among
these annotations. Their prior open status remains. The partial catalog, context
property coverage and Rope implementation were also not accepted by this list.

New explicit direction supersedes the old single-player-only project/goal scope:
JA:R must support a hosted, shared multiplayer sandbox like a normal GMod server.
Use a matching JA:R client and dedicated/listen server with authoritative shared
world state, per-player tools/ownership and connection lifecycle. Keep the
accepted single-player build available. Merely enabling stock MP builds is not
completion of this request. Campaign co-op is not inferred from sandbox hosting.

Ropes must be visibly rendered. Existing thin, one-millisecond effect lines do
not satisfy this reported requirement. Preserve the owner-accepted gun beams.
