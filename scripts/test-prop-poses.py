"""Read-only retail pose metadata qualification. Does not launch/render the game."""
import argparse
import json
import math
import re
import struct
import zipfile
from jar_catalog_assets import catalog_asset_paths
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-data', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    files, packs = {}, []
    for path in catalog_asset_paths(args.game_data):
        pack = zipfile.ZipFile(path)
        packs.append(pack)
        for name in pack.namelist():
            files[name.lower()] = (pack, name)

    def data(name):
        pack, entry = files[name.lower()]
        return pack.read(entry)

    generated = (ROOT / 'code/qcommon/jar_catalog_generated.h').read_text(encoding='utf-8')
    models = set(re.findall(r'\{\d+u,1,\d+,"[^"]+","([^"]+)"', generated))
    source = (ROOT / 'code/game/g_workshop.cpp').read_text(encoding='utf-8')
    legacy = set(re.findall(r'\{10\d\d,"[^"]+","([^"]+)"', source))
    missing_legacy = sorted(legacy - files.keys())
    models.update(legacy & files.keys())
    blocked = json.loads((ROOT / 'docs/JAR-spawn-catalog.json').read_text(encoding='utf-8'))['blocked']
    models.difference_update(row['name'] for row in blocked)
    assert models
    renderer = (ROOT / 'code/rd-vanilla/tr_model.cpp').read_text(encoding='utf-8')
    fake = re.search(r'FakeGLAFile\[\]\s*=\s*\{([^}]+)', renderer).group(1)
    default = bytes(int(v, 16) for v in re.findall(r'0x([0-9A-Fa-f]{2})', fake))
    results = []
    for name in sorted(models):
        blob = data(name)
        if name.endswith('.md3'):
            assert blob[:4] == b'IDP3' and struct.unpack_from('<i', blob, 4)[0] == 15, name
            frames = struct.unpack_from('<i', blob, 76)[0]
            offset, end = struct.unpack_from('<i', blob, 92)[0], struct.unpack_from('<i', blob, 104)[0]
            assert 0 < frames <= 65536 and 108 <= offset <= end <= len(blob) and frames <= (end-offset)//56, name
            rejected = 0
            for frame in range(frames):
                lohi = struct.unpack_from('<6f', blob, offset+frame*56)
                valid = all(math.isfinite(v) and abs(v) <= 8192 for v in lohi)
                valid &= all(lohi[i] <= lohi[i+3] for i in range(3))
                rejected += not valid
            assert rejected < frames, name
            results.append(dict(model=name, frames=frames, rejectedBounds=rejected))
        else:
            assert name.endswith('.glm') and blob[:4] == b'2LGM', name
            anim = blob[72:136].split(b'\0', 1)[0].decode('ascii')
            gla = default if anim == '*default' else data(anim+'.gla')
            assert gla[:4] == b'2LGA', name
            frames, _, bones, _, _, end = struct.unpack_from('<6i', gla, 76)
            assert 0 < frames <= 65536 and 0 < bones <= 512 and 100+bones*4 <= end <= len(gla), name
            roots = []
            for bone in range(bones):
                offset = struct.unpack_from('<i', gla, 100+bone*4)[0]
                assert bones*4 <= offset <= end-100-172, name
                bone_name = gla[100+offset:164+offset]
                parent = struct.unpack_from('<i', gla, 168+offset)[0]
                assert -1 <= parent < bones and b'\0' in bone_name and bone_name[0], name
                if parent == -1:
                    roots.append(bone_name.split(b'\0', 1)[0].decode('ascii'))
            assert roots, name
            results.append(dict(model=name, animation=anim, frames=frames, roots=roots))
    summary = dict(Models=len(results), MultiFrameModels=sum(row['frames'] > 1 for row in results),
                   MaximumFrames=max(row['frames'] for row in results), GraphicalValidation=False, MissingLegacyModels=missing_legacy,
                   Scope='Retail frame/header/root metadata; posed GLM geometry is covered by renderer fixtures, not this script')
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(dict(**summary, models=results), indent=2)+'\n', encoding='utf-8')
    print(json.dumps(summary))
    for pack in packs:
        pack.close()


if __name__ == '__main__':
    main()
