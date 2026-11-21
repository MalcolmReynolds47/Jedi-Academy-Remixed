"""Qualify real MP cosmetic bytes, native event routes and dependency closure."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--pack', type=Path, required=True)
p.add_argument('--game-data', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parents[2]
base = {}
archives = []
for path in sorted((a.game_data / 'base').glob('assets*.pk3')):
    z = zipfile.ZipFile(path)
    archives.append(z)
    for n in z.namelist():
        base[n.lower()] = (z, n)


def clean(data):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', data.decode('latin1'), flags=re.S)


def blocks(data):
    # Independent balanced-brace scan; retain each exact top-level body.
    text = data.decode('latin1')
    parsed = re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: ' ' * len(m[0]), text, flags=re.S)
    depth = 0
    result = []
    for m in re.finditer(r'[{}]', parsed):
        if m[0] == '{':
            if depth == 0:
                kind = re.search(r'([A-Za-z]+)\s*$', parsed[:m.start()])
                assert kind
                start = kind.start()
            depth += 1
        else:
            depth -= 1
            assert depth >= 0
            if depth == 0:
                result.append((kind[1].lower(), text[start:m.end()]))
    assert not depth
    return result


with zipfile.ZipFile(a.source) as sp, zipfile.ZipFile(a.pack) as mp:
    names = set(mp.namelist())
    expected = {n for n in sp.namelist() if n.startswith(('jar/saber/', 'jar/sound/', 'jar/fx/mbii/', 'jar/m/', 'effects/jar/', 'jar/licenses/')) or n == 'shaders/jar_mbii.shader'}
    muzzle_path = 'effects/jar/sandbox/e11_muzzle.efx'
    assert names == expected | {'jar/mp-effects-manifest.json', muzzle_path}
    assert len(names) == len(mp.infolist()) and mp.testzip() is None
    manifest = json.loads(mp.read('jar/mp-effects-manifest.json'))
    assert {row['path'] for row in manifest['files']} == expected
    muzzle = manifest['retail_muzzle']
    assert muzzle['path'] == muzzle_path and muzzle['source_path'] == 'effects/blaster/muzzle_flash.efx'
    stock_pack, stock_key = base[muzzle['source_path']]
    stock_muzzle, adapted_muzzle = stock_pack.read(stock_key), mp.read(muzzle_path)
    assert muzzle['source_archive'] == Path(stock_pack.filename).name
    assert muzzle['source_sha256'] == hashlib.sha256(stock_muzzle).hexdigest()
    assert muzzle['sha256'] == hashlib.sha256(adapted_muzzle).hexdigest()
    removed = {'sound', 'forcefeedback'}
    assert set(muzzle['removed_primitives']) == removed
    stock_blocks = blocks(stock_muzzle)
    assert {kind for kind, _ in stock_blocks if kind in removed} == removed
    assert blocks(adapted_muzzle) == [(kind, body) for kind, body in stock_blocks if kind not in removed]
    weapon = (root / 'codemp/cgame/cg_weaponinit.c').read_text(encoding='utf-8')
    muzzles = {'jar/fx/mbii/effects/' + n + '.efx' for n in re.findall(r'(?:muzzleEffect|altMuzzleEffect)\s*=\s*trap->FX_RegisterEffect\(\s*"([^"]+)"', weapon)}
    changed = []
    for row in manifest['files']:
        n = row['path']
        left, right = sp.read(n), mp.read(n)
        assert hashlib.sha256(left).hexdigest() == row['source_sha256']
        assert hashlib.sha256(right).hexdigest() == row['sha256']
        if n in muzzles:
            assert blocks(right) == [(kind, body) for kind, body in blocks(left) if kind != 'sound'], n
            assert row['removed_sound_primitives'] == sum(kind == 'sound' for kind, _ in blocks(left))
        else:
            assert left == right and not row['removed_sound_primitives'], n
        if left != right:
            changed.append(n)
    assert changed and {'jar/fx/mbii/effects/bryar/muzzle_flash.efx', 'jar/fx/mbii/effects/blaster/muzzle_flash.efx'} <= set(changed)
    for category, styles in [('saber', ('cinematic', 'soft', 'saber_gfx')), ('sound', ('mbii', 'ot', 'bf', 'tfu', 'plasma')), ('fx', ('mbii',))]:
        for style in styles:
            assert mp.read(f'jar/{category}/{style}/pack.txt')
    shaders = set()
    shader_text = ''
    for n in base:
        if n.startswith('shaders/') and n.endswith('.shader'):
            z, key = base[n]
            shaders.update(re.findall(r'(?m)^\s*([^\s{}]+/[^\s{}]+)\s*\{', clean(z.read(key)).lower()))
    for n in names:
        if n.endswith('.shader'):
            shader_text += clean(mp.read(n)).lower() + '\n'
    private_shaders = set(re.findall(r'(?m)^\s*([^\s{}]+/[^\s{}]+)\s*\{', shader_text))
    assert private_shaders and all(n.startswith('jar/m/') for n in private_shaders)
    shaders |= private_shaders
    for n in names:
        if not n.endswith(('.efx', '.shader')):
            continue
        for ref in re.findall(r'[^\s{}\[\]"]+/[^\s{}\[\]"]+', clean(mp.read(n)).lower()):
            candidates = [ref] + [ref + ext for ext in ('.tga', '.jpg', '.png', '.wav', '.mp3')] + ['effects/' + ref + '.efx']
            if ref.endswith(('.wav', '.mp3')):
                candidates += [ref[:-4] + ext for ext in ('.wav', '.mp3')]
            assert ref in shaders or any(n in names or n in base for n in candidates), (n, ref)
    # Actual source guards and registrations remain authoritative for event audio,
    # complete asset cache reload and the previously requested contact-flash removal.
    cg = (root / 'codemp/cgame/cg_weapons.c').read_text()
    assert 'weap->flashSound[c]' in cg and 'weap->altFlashSound[c]' in cg
    main = (root / 'codemp/cgame/cg_main.c').read_text()
    assert 'trap->FX_FreeSystem();' in main and 'trap->FX_InitSystem(&cg.refdef);' in main
    assert 'type==ScreenFlash && JAR_SaberContactEffect(file)' in (root / 'codemp/client/FxScheduler.cpp').read_text()
    print(f'MP effects verified: {len(expected)} shared files; {len(changed)} muzzle adaptations; all other bytes exact; EFX/shader/image/sound dependencies closed')
for z in archives:
    z.close()
