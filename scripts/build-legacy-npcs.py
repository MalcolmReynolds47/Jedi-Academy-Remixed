"""Restore missing Academy NPC asset families from the owner's Outcast install.

Only absent resources enter the private companion PK3. Academy definitions and
shared humanoid assets are never replaced. Legacy animation sounds are converted
to the native Academy event grammar without changing timing or sound choices.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct
import zipfile
from jar_catalog_assets import LEGACY_PACK, ROOT

FAMILIES = ('glider', 'mark1', 'mark2', 'minemonster', 'galak_mech')
SOUND_FAMILIES = ('glider', 'mark1', 'mark2', 'mine', 'galak')
IMAGE_EXTENSIONS = ('.tga', '.jpg', '.png', '.jpeg', '.dds')


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Assets:
    def __init__(self, game_data):
        self.files, self.packs = {}, []
        for path in sorted((game_data / 'base').glob('assets*.pk3')):
            pack = zipfile.ZipFile(path)
            self.packs.append(pack)
            for name in pack.namelist():
                if not name.endswith('/'):
                    self.files[name.lower()] = (pack, name)
        if not self.files:
            raise FileNotFoundError('Retail assets missing: ' + str(game_data))

    def read(self, name):
        pack, entry = self.files[name.lower()]
        return pack.read(entry)

    def image(self, name):
        name = name.lower()
        stem = str(PurePosixPath(name).with_suffix('')) if PurePosixPath(name).suffix else name
        return next((p for p in [name] + [stem + ext for ext in IMAGE_EXTENSIONS] if p in self.files), None)

    def sound(self, name):
        name = name.lower()
        stem = str(PurePosixPath(name).with_suffix('')) if PurePosixPath(name).suffix else name
        return next((p for p in (name, stem + '.wav', stem + '.mp3') if p in self.files), None)


def clean(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text.replace('\r', ''), flags=re.S)


def shaders(assets):
    result = {}
    for name in sorted(assets.files):
        if not name.startswith('shaders/') or not name.endswith('.shader'):
            continue
        raw = clean(assets.read(name).decode('latin1'))
        tokens = list(re.finditer(r'"[^"\n]*"|[{}]|[^\s{}]+', raw))
        i = 0
        while i + 1 < len(tokens):
            key = tokens[i][0].strip('"').lower()
            i += 1
            if tokens[i][0] != '{':
                raise ValueError('Malformed shader: ' + name)
            start, depth = tokens[i].start(), 0
            while i < len(tokens):
                depth += (tokens[i][0] == '{') - (tokens[i][0] == '}')
                i += 1
                if not depth:
                    break
            if depth:
                raise ValueError('Unclosed shader: ' + name)
            result.setdefault(key, (name, raw[start:tokens[i-1].end()]))
    return result


def convert_events(data):
    output, count = [], 0
    for line in clean(data.decode('latin1')).splitlines():
        row = line.split()
        if not row:
            continue
        if row[0] in ('UPPERSOUNDS', 'LOWERSOUNDS'):
            output.append(row[0].replace('SOUNDS', 'EVENTS'))
        elif row == ['{'] or row == ['}']:
            output.append(row[0])
        else:
            if len(row) != 6 or not row[0].startswith(('BOTH_', 'TORSO_', 'LEGS_')):
                raise ValueError('Unsupported legacy sound event: ' + line)
            frame, low, high, chance = map(int, (row[1], *row[3:]))
            if not (frame >= 0 and 0 <= low <= high <= 16 and 0 <= chance <= 100):
                raise ValueError('Invalid legacy sound event: ' + line)
            output.append(' '.join([row[0], 'AEV_SOUND', *row[1:]]))
            count += 1
    return ('// Converted from the original Outcast animation sounds.\n' + '\n'.join(output) + '\n').encode('ascii'), count


def convert_animations(family, data):
    """Keep original frame ranges; bind removed legacy names to native states."""
    supported = set(re.findall(r'\b(?:BOTH|TORSO|LEGS|FACE)_[A-Z0-9_]+', (ROOT / 'code/game/anims.h').read_text()))
    supported &= set(re.findall(r'\b(?:BOTH|TORSO|LEGS|FACE)_[A-Z0-9_]+', (ROOT / 'codemp/game/anims.h').read_text()))
    original = {row[0]: row[1:] for line in clean(data.decode('latin1')).splitlines() if (row := line.split())}
    rows = {name: values for name, values in original.items() if name in supported}
    aliases = {}
    if family == 'glider':
        # Its only original clip used an enum absent even from released JK2.
        # The neutral ambient actor now flaps while idle/moving, using the same
        # 180 original frames and rate, with a continuous loop.
        for name in ('BOTH_STAND1', 'BOTH_STAND2', 'BOTH_WALK1', 'BOTH_RUN1'):
            rows[name] = [*original['GLIDER_FLY'][:2], '0', original['GLIDER_FLY'][3]]
            aliases[name] = 'GLIDER_FLY (looped)'
    elif family == 'mark1':
        rows['BOTH_SLEEP1'] = original['BOTH_SHUTDOWN']
        aliases['BOTH_SLEEP1'] = 'BOTH_SHUTDOWN'
    elif family == 'mark2':
        rows['BOTH_STAND1'] = original['BOTH_STAND2']
        aliases['BOTH_STAND1'] = 'BOTH_STAND2'
    elif family == 'galak_mech':
        for _, target, source in re.findall(r'#define (JAR_GALAK_\w+) (\w+) // (\w+)', (ROOT / 'shared/qcommon/jar_galak_anims.h').read_text()):
            rows[target] = original[source]
            aliases[target] = source
    return ('// Original Outcast frames adapted to Academy animation names.\n' + '\n'.join(name + ' ' + ' '.join(values) for name, values in rows.items()) + '\n').encode('ascii'), aliases


def build(game_data, outcast_data, output):
    academy, outcast = Assets(game_data), Assets(outcast_data)
    content, sources, reused, shader_defs = {}, {}, set(), {}

    def copy(name):
        if name in academy.files:
            reused.add(name)
            return
        data = outcast.read(name)
        content[name] = data
        pack, original = outcast.files[name]
        sources[name] = dict(archive=Path(pack.filename).name, entry=original, sha256=sha(data))

    for family, sound_family in zip(FAMILIES, SOUND_FAMILIES):
        model_prefix, sound_prefix = f'models/players/{family}/', f'sound/chars/{sound_family}/'
        for name in sorted(outcast.files):
            if name.startswith(model_prefix):
                copy(name)
            elif name.startswith(sound_prefix) or (family == 'galak_mech' and name.startswith('sound/weapons/galak/')):
                existing = academy.sound(name)
                if existing:
                    reused.add(existing)
                else:
                    copy(name)

    ja_shaders, jo_shaders = shaders(academy), shaders(outcast)

    def image(name):
        if name.startswith('$'):
            return
        existing = academy.image(name)
        if existing:
            reused.add(existing)
            return
        resource = outcast.image(name)
        if not resource:
            raise ValueError('Unresolved legacy material image: ' + name)
        copy(resource)

    def material(name):
        name = name.lower()
        if name in ('[nomaterial]', 'clear', '*off') or name.startswith('*'):
            return
        key = str(PurePosixPath(name).with_suffix('')) if PurePosixPath(name).suffix else name
        definition = ja_shaders.get(key) or jo_shaders.get(key)
        if not definition:
            image(name)
            return
        source, body = definition
        if key in ja_shaders:
            reused.add(source)
        else:
            shader_defs[key] = body
            pack, original = outcast.files[source]
            sources['shader:' + key] = dict(archive=Path(pack.filename).name, entry=original, sha256=sha(outcast.read(source)))
        for line in body.splitlines():
            row = line.split()
            if row and row[0].lower() in ('map', 'clampmap', 'animmap'):
                for value in row[2:] if row[0].lower() == 'animmap' else row[1:]:
                    image(value.strip('"'))

    def sound(name):
        existing = academy.sound(name)
        if existing:
            reused.add(existing)
        else:
            resource = outcast.sound(name)
            if not resource:
                raise ValueError('Missing effect sound: ' + name)
            copy(resource)

    seen_effects = set()
    def effect(name):
        path = 'effects/' + name.removeprefix('effects/').removesuffix('.efx') + '.efx'
        if path in seen_effects:
            return
        seen_effects.add(path)
        if path in academy.files:
            reused.add(path)
            return
        copy(path)
        for kind, values in re.findall(r'(\w+)\s*\[([^]]+)\]', clean(outcast.read(path).decode('latin1'))):
            for value in values.split():
                value = value.strip('"').lower()
                if kind.lower() in ('shader', 'shaders'):
                    material(value)
                elif kind.lower() in ('sound', 'sounds'):
                    sound(value)
                elif kind.lower() in ('impactfx', 'deathfx', 'emitterfx', 'playfx'):
                    effect(value)
                else:
                    raise ValueError('Unsupported legacy effect dependency: ' + kind)
    for root in ('galak/beam_warmup', 'galak/trace_beam', 'galak/explode', 'env/med_explode2', 'env/small_explode2', 'blaster/smoke_bolton'):
        effect(root)

    # MP follows the current replicated phase and cancels within 125 ms. The
    # original SP entity-attached line uses a runtime path absent from MP, so its
    # MP equivalent is traced in cg_jar_galak.inc with the same original shader.
    for name in ('beam_warmup', 'trace_beam'):
        source = 'effects/galak/' + name + '.efx'
        original = outcast.read(source)
        adapted = original
        if name == 'trace_beam':
            start = adapted.index(b'Line')
            brace = adapted.index(b'{', start)
            depth, end = 1, brace + 1
            while depth:
                depth += (adapted[end] == 123) - (adapted[end] == 125)
                end += 1
            adapted = adapted[:start] + adapted[end:]
        adapted, count = re.subn(rb'(\blife\s+)(?:1500|3000)\b', rb'\g<1>125', adapted)
        if count != (3 if name == 'beam_warmup' else 2):
            raise ValueError('Original Galak effect duration changed')
        destination = source.replace('.efx', '_follow.efx')
        content[destination] = adapted
        sources[destination] = dict(derivedFrom=source, sourceSHA256=sha(original), sha256=sha(adapted),
            transformation='125ms MP bolt-follow particles; beam line traced each frame with original shader')

    event_counts, animation_aliases = {}, {}
    for family in FAMILIES:
        prefix = f'models/players/{family}/'
        model = outcast.read(prefix + 'model.glm')
        if model[:4] != b'2LGM' or struct.unpack_from('<i', model, 4)[0] != 6:
            raise ValueError('Unexpected original model format: ' + family)
        for index in range(struct.unpack_from('<i', model, 152)[0]):
            offset = 164 + struct.unpack_from('<i', model, 164 + index * 4)[0]
            material(model[offset+68:offset+132].split(b'\0', 1)[0].decode('ascii'))
        for name in outcast.files:
            if name.startswith(prefix) and name.endswith('.skin'):
                for line in clean(outcast.read(name).decode('latin1')).splitlines():
                    row = line.split(',')
                    if len(row) == 2 and not row[0].strip().startswith('tag_'):
                        material(row[1].strip())
        config = prefix + 'animation.cfg'
        original_config = outcast.read(config)
        adapted_config, aliases = convert_animations(family, original_config)
        if config in academy.files:
            raise ValueError('Refusing to replace Academy animation configuration: ' + config)
        content[config] = adapted_config
        sources[config]['sourceSHA256'] = sources[config].pop('sha256')
        sources[config]['sha256'] = sha(adapted_config)
        sources[config]['transformation'] = 'Academy animation names; original frame ranges and rates'
        animation_aliases[family] = aliases
        old_events = prefix + 'animsounds.cfg'
        events, count = convert_events(outcast.read(old_events))
        if family == 'galak_mech':
            for target, source in aliases.items():
                events = re.sub(rb'(?m)^' + source.encode() + rb' ', target.encode() + b' ', events)
            skin = prefix + 'model_default.skin'
            adapted = outcast.read(skin).replace(b'torso_shield_off,clear', b'torso_shield_off,gfx/effects/irid_shield')
            if adapted == outcast.read(skin):
                raise ValueError('Original Galak shield skin changed')
            material('gfx/effects/irid_shield')
            content[skin] = adapted
            sources[skin]['sourceSHA256'] = sources[skin].pop('sha256')
            sources[skin]['sha256'] = sha(adapted)
            sources[skin]['transformation'] = 'Use original shield shader on shield geometry instead of invisible clear material'

        for row in (line.split() for line in clean(outcast.read(old_events).decode('latin1')).splitlines()):
            if len(row) == 6:
                low, high = int(row[3]), int(row[4])
                for path in ([row[2] % i for i in range(low, high+1)] if low and high else [row[2]]):
                    sound(path)
        event_counts[family] = count
        new_events = prefix + 'animevents.cfg'
        if new_events in academy.files:
            raise ValueError('Refusing to replace Academy animation events: ' + new_events)
        content[new_events] = events
        sources[new_events] = dict(derivedFrom=old_events, sourceSHA256=sha(outcast.read(old_events)), sha256=sha(events))

    if shader_defs:
        content['shaders/jar_legacy_npcs.shader'] = '\n'.join(key + '\n' + body for key, body in sorted(shader_defs.items())).encode('latin1')
    if set(content) & academy.files.keys():
        raise ValueError('Legacy bridge must never replace an Academy asset')
    manifest = dict(schema=1, families=FAMILIES, sourceGame='Jedi Outcast local installation',
                    scope='Missing Academy NPC assets only; no NPC definitions or shared humanoid overrides',
                    sources=sources, reusedAcademyResources=sorted(reused), animationEvents=event_counts, animationAliases=animation_aliases,
                    files={name: sha(data) for name, data in sorted(content.items())})
    content['jar/legacy-npcs-source.json'] = (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix('.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as pack:
        for name, data in sorted(content.items()):
            info = zipfile.ZipInfo(name, date_time=(2026, 10, 11, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            pack.writestr(info, data)
    temporary.replace(output)
    print(f'Legacy NPC assets: {len(FAMILIES)} families, {len(content)} files, {sum(event_counts.values())} sound events; {output}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-data', type=Path, required=True)
    parser.add_argument('--outcast-data', type=Path)
    parser.add_argument('--output', type=Path, default=LEGACY_PACK)
    args = parser.parse_args()
    source = args.outcast_data or args.game_data.parent.parent / 'Jedi Outcast/GameData'
    build(args.game_data, source, args.output)
