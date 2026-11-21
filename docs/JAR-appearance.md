# JA:R 0.4.1 display and appearance

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

## 0.20 speech variation

Facial accents now vary lip spread, jaw opening and asymmetry using smooth,
speaker-specific phases plus changes in the actual voice-energy envelope. This
avoids an identical open/close shape every syllable without consuming the gameplay
randomizer's RNG. It is an amplitude-correlated accent, not phoneme recognition.
Stock remains the default, and silence/death/rig-change cleanup remains active.
Bounds/controller tests pass; no new in-game artistic acceptance is claimed.


## Speech-driven facial expressions (0.19.0)

Open **Settings > Styles** from the main or pause menu. Select **Facial
expressions**, then **Apply**. This setting and **Speech sensitivity** apply live;
no renderer/audio restart or level reload is needed. Stock is the default.

| Preset | Added movement at maximum speech strength |
| --- | --- |
| Stock | Native facial animation; remove JA:R accents. |
| Expressive | Up to 8 degrees of additional jaw opening, subtle lip/brow accents. |
| Exaggerated | Up to 22 degrees of additional jaw opening, larger lip/brow accents. |
| Extreme | Up to 45 degrees of additional jaw opening and intentionally strong stretching. |

Sensitivity choices are Subtle (0.5x), Normal (1x), Sensitive (1.5x) and Very
sensitive (2x). These are bounded visual gains; they do not change audible volume.
Accents follow pre-mix voice energy with a short attack and slower release.
Speaker/master volume, music and effects do not drive the face. Original voice
states, script waits and FACE_TALK animation selection remain unchanged.
The standard 64-bit DMA backend supplies a continuous sample-energy signal;
legacy OpenAL builds use smoothed native talking levels.

Supported rigs have the humanoid `face` hierarchy with `jaw`, lip and eyebrow
bones. Missing bones are skipped. Blinking and independently controlled bone
angles are retained. Stock, silence and death remove the added accents; saved
accent ownership also allows Stock to clean up after save/load or video reload.
The accents use the actual jaw pivot and lower lips follow the extra jaw opening.
They affect skeletal poses, not mesh topology or audio. Extreme poses may expose
skin-weight, mouth-interior or teeth limitations on individual models.

Cvars: `jar_facePreset` (0-3) and `jar_faceSensitivity` (0.5-2). Both persist in
configuration, are included in diagnostic snapshots and are preserved by the
Balanced/Quality/Maximum FPS graphics presets. The engine/game interface is now
1018 for the voice-energy callback; the renderer interface and save layout are unchanged.
Automated audio, deformation, controller and menu checks do not establish visual
quality; no game was launched while implementing this update.


Open **JA:R Settings** from the main menu or pause menu. Each selection is pending
until **Apply**. Graphics presets leave display dimensions and appearance choices
alone. **Revert pending** discards edits; **Back / Cancel** keeps existing settings.

## Resolution and fullscreen

- **Display → Custom width / Custom height** accepts pixel dimensions and selects
  `r_mode -1` automatically when applied in windowed or exclusive fullscreen mode.
- **Use desktop borderless** selects `r_mode -2` and `r_fullscreen 2`. SDL queries
  the monitor's desktop size and creates desktop fullscreen without changing its
  display mode. The detected size and refresh rate appear in the menu.
- Window mode also offers ordinary windowed (`0`) and exclusive fullscreen (`1`).
  Window decoration controls ordinary window borders independently.
- A display change starts a **20-second confirmation** after video reload. Choose
  **Keep this display**, or press Escape / wait to restore the previous display.
  Temporary display settings are excluded from automatic configuration saves.
- Custom dimensions must be at least 320×200, at most 16384 per axis, and no more
  than 64 megapixels total. Driver limits still apply. The engine retains its safe
  fallback mode when a requested mode cannot be created.

Video changes require a renderer reload. Borderless always follows the desktop;
typed dimensions apply to the other window modes. On first launch the primary
monitor is used; while recreating an existing window, its monitor is queried.

## Interface and console

The initial defaults for the new settings are **Crisp Sans** and **2x smooth**.
Existing graphics, window, sound and saber settings are preserved.

| Control | Choices and behavior |
| --- | --- |
| HUD layout | Academy gauges or OpenJK Simple numeric. Applies after video reload and persists. Custom HUD paths are preserved unless this selector is changed. MBII multiplayer HUDs require its game code and are not offered as SP layouts. |
| Text appearance | Retail, Crisp Sans, Crisp Serif. Proportional glyphs are rendered from vector fonts into 2048×2048 atlases. Original glyph advance, bounds and baseline values are preserved to keep menu layout and hit targets stable. |
| UI artwork | Retail, 2x smooth, 4x sharpened. Covers retail menu/HUD/icon/console textures and their texture animation frames under `gfx/menus`, `gfx/hud`, `gfx/2d`, `gfx/interface`, `menu/art`, and `ui/assets`. Textures are capped at 4096 pixels per axis. |
| Font variant detail | Selects higher-resolution variants when an installed font pack provides OpenJK `_sharp` variants. |
| UI motion | Normal or Reduced. Reduced freezes 2D material animation, stops menu orbits and completes rectangle transitions immediately. World/combat clocks and story cinematics are unchanged. |
| Console background | Retail, Slate, Amber, Black. The flat-color choices remain crisp at any resolution. |
| Console opacity | 25%, 50%, 75%, 100%. The disconnected/fullscreen console remains opaque. |
| Console text size | 100%, 150%, 200%, 300%, 400%. Console characters use a high-resolution monospace atlas with either crisp font selection. |
| Console height | 25%, 50%, 75%, 100%. |

Latin, Cyrillic and Polish font tables receive new vector glyphs. Control symbols,
the fictional alphabet and Asian font artwork retain their original character
designs; their bitmaps are resampled. This is not a complete vector redraw of the
retail interface. Texture upscaling smooths and optionally sharpens existing art;
it cannot recover missing source detail. 4x artwork uses more GPU texture memory.

The crisp glyph sources are Noto Sans, Noto Serif and Noto Sans Mono, from
[Google Fonts](https://github.com/google/fonts). Their OFL 1.1 notices and source
hashes are included in `jar/licenses/` inside the appearance archive. Sources and
the generation script are retained in this project.

## Saber and combat styles

The **Styles** page has independent lists; no gameplay or damage values change.

- **Saber blades:** Retail, Cinematic narrow, Soft glow, Installed Saber GFX.
  Cinematic and Soft are generated blade profiles. Saber GFX imports the selected
  texture add-on already present in this owner's MBII directory.
- **Saber sounds:** Retail, MBII, MBII Original Trilogy, MBII Battlefront,
  MBII Force Unleashed, MBII Plasma. These correspond to the installed MBII
  `saber`, `saberot`, `BFsaber`, `saber_tfu`, and `saber_plasma` sample folders.
  Only matching retail sample names are substituted; missing samples use retail.
- **Combat effects:** Retail or MBII combat. The importer adapts compatible saber,
  blaster and pistol effects, plus bowcaster bolts/impacts, repeater primary and
  secondary projectiles/impacts, DEMP2 ion effects, flechette impacts, disruptor
  effects, rocket trails/explosions, thermal/tripmine/detpack explosions, and
  concussion explosions. Dependencies use private names. Firing audio remains the
  correct retail SP primary/alternate sound; importing an MP muzzle flash cannot
  remove or double that audio. The asset manifest records each effect adaptation. MBII-specific gameplay effects are not enabled. More
  elaborate effects can cost frame time, especially in crowded scenes.

Apply reloads visual assets and restarts sound when its pack changes. Selections
persist. A pack absent from the generated installation is rejected before other
pending settings are applied. Individual missing replacement files fall back to
the original game or currently mounted mod. Third-party packs may need a map
reload if their assets are retained by game-side effect caches.

## Additional local packs

The engine mounts `GameData/JediAcademyRemixed/` and
`<fs_homepath>/appearance/` for namespaced appearance assets. Put custom PK3s in
the latter location. Do not replace retail files. A pack uses these paths:

```text
jar/<category>/<id>/pack.txt
jar/<category>/<id>/<original asset path>
jar/catalog/<unique filename>.jap
```

Category is `font`, `ui`, `saber`, `sound`, or `fx`. IDs use 1–16 lowercase letters,
digits or underscores. For example, a replacement HUD image can be stored as
`jar/ui/mytheme/gfx/hud/ammo_tic_1.tga`. Fonts need matching `.fontdat` UV tables
and atlases. Preserve the original sound extension/encoding. Shader definitions
must have unique names; putting a stock-named shader in an archive affects all
styles and bypasses selection. Effects need all referenced resources.

A `.jap` catalog descriptor contains exactly these three plain-text lines:

```text
ui
mytheme
My HUD Theme
```

After restarting video, that label appears in the corresponding list. Labels
allow up to 36 printable ASCII characters. Duplicate and invalid entries are
ignored. An empty `pack.txt` is sufficient as the availability marker. For direct
console selection, use `jar_uiPack mytheme` then `vid_restart`; the corresponding
cvars are `jar_fontPack`, `jar_saberPack`, `jar_soundPack`, and `jar_fxPack`.
For sound changes, run `snd_restart` followed by `vid_restart`.

## Packaging, provenance and diagnostics

`scripts/build-jar.ps1` generates `JediAcademyRemixed/jar-appearance.pk3` from this
owner's installed retail and MBII assets, validates it, packages it with the
runtime and deploys that exact release with backups and SHA-256 verification.
It requires Python with Pillow; use `-PythonExecutable` on another machine.
The builder caches a generated archive against its script, source fonts and
source-archive metadata. No source game/mod archive is modified.

`jar/appearance-manifest.json` inside the PK3 records source entries, SHA-256
hashes, imported counts and dependency validation. These locally derived game
and MBII assets retain their original ownership; this package does not assert
permission to redistribute them. MBII code and multiplayer features are not
included.

Diagnostic snapshots include all new settings. Display creation and confirmation
are logged at normal verbosity. `jar_logLevel 2` additionally records selected
asset and sound replacements. The renderer's existing warnings identify failed
asset decoding and shader/effect parsing.

Validation includes the real menu controller, rejection of invalid inputs and
unavailable packs, confirmation timeout/keep behavior, asset-category isolation,
WAV/MP3 selection, archive integrity, font metric preservation and shader image
dependency closure. These checks do not replace in-game visual, audio or monitor
testing. The game is not launched by the build or validation scripts.
