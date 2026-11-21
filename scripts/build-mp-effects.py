"""Reuse SP cosmetic media in MP; native MP weapon code owns muzzle audio."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('jar_appearance_builder', ROOT / 'scripts/build-appearance.py')
appearance = importlib.util.module_from_spec(spec)
spec.loader.exec_module(appearance)


def remove_primitives(data, kinds):
    text = data.decode('latin1')
    removed = 0
    for kind, start, end in reversed(appearance.effect_blocks(text)):
        if kind in kinds:
            text = text[:start] + text[end:]
            removed += 1
    return text.encode('latin1'), removed


def build(source, output, game_data):
    assert source.resolve() != output.resolve()
    weapon_source = (ROOT / 'codemp/cgame/cg_weaponinit.c').read_text(encoding='utf-8')
    muzzles = {'jar/fx/mbii/effects/' + n + '.efx' for n in re.findall(
        r'(?:muzzleEffect|altMuzzleEffect)\s*=\s*trap->FX_RegisterEffect\(\s*"([^"]+)"', weapon_source)}
    output.parent.mkdir(parents=True, exist_ok=True)
    manifest = dict(schema=1, description='SP cosmetic media for MP; native MP muzzle sounds retained in code', files=[])
    with zipfile.ZipFile(source) as src, zipfile.ZipFile(output, 'w') as dst:
        names = sorted(n for n in src.namelist() if n.startswith((
            'jar/saber/', 'jar/sound/', 'jar/fx/mbii/', 'jar/m/', 'effects/jar/', 'jar/licenses/')) or n == 'shaders/jar_mbii.shader')
        assert names and len(names) == len(set(names))
        for n in names:
            original = data = src.read(n)
            removed = 0
            if n in muzzles:
                data, removed = remove_primitives(original, ('sound',))
            dst.writestr(zipfile.ZipInfo(n, (2026, 10, 12, 0, 0, 0)), data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=1)
            manifest['files'].append(dict(path=n, sha256=hashlib.sha256(data).hexdigest(), source_sha256=hashlib.sha256(original).hexdigest(), removed_sound_primitives=removed))
        retail = 'effects/blaster/muzzle_flash.efx'
        for path in sorted((game_data / 'base').glob('assets*.pk3'), reverse=True):
            with zipfile.ZipFile(path) as pack:
                if retail in pack.namelist():
                    original = pack.read(retail)
                    break
        else:
            raise FileNotFoundError(f'Missing stock E-11 muzzle effect {retail}')
        destination = 'effects/jar/sandbox/e11_muzzle.efx'
        data, removed = remove_primitives(original, ('sound', 'forcefeedback'))
        assert destination not in names and removed == 2
        dst.writestr(zipfile.ZipInfo(destination, (2026, 10, 12, 0, 0, 0)), data,
                     compress_type=zipfile.ZIP_DEFLATED, compresslevel=1)
        manifest['retail_muzzle'] = dict(path=destination, source_path=retail, source_archive=path.name,
                                        sha256=hashlib.sha256(data).hexdigest(),
                                        source_sha256=hashlib.sha256(original).hexdigest(),
                                        removed_primitives=['sound', 'forcefeedback'])
        source_manifest = json.loads(src.read('jar/appearance-manifest.json'))
        manifest['sources'] = [row for row in source_manifest['sources'] if row['destination'] in names]
        manifest['notice'] = source_manifest['notice']
        dst.writestr(zipfile.ZipInfo('jar/mp-effects-manifest.json', (2026, 10, 12, 0, 0, 0)), json.dumps(manifest, indent=2))
    print(f'MP effects pack built: {len(names)} shared files and an adapted stock E-11 muzzle')


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--game-data', type=Path, required=True)
    a = p.parse_args()
    build(a.source, a.output, a.game_data)
