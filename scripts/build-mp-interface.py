"""Package the existing SP font/UI bytes for MP, without other SP asset families."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
assert a.source.resolve() != a.output.resolve(), 'source must be preserved'
a.output.parent.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(a.source) as src:
    names = sorted(n for n in src.namelist() if n.startswith(('jar/font/', 'jar/ui/', 'jar/licenses/')))
    assert names and len(names) == len(set(names))
    manifest = {'schema': 1, 'description': 'Unmodified SP font/UI assets and license notices for MP', 'files': []}
    with zipfile.ZipFile(a.output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as dst:
        for n in names:
            data = src.read(n)
            dst.writestr(zipfile.ZipInfo(n, (2026, 10, 12, 0, 0, 0)), data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)
            manifest['files'].append({'path': n, 'sha256': hashlib.sha256(data).hexdigest()})
        dst.writestr(zipfile.ZipInfo('jar/interface-manifest.json', (2026, 10, 12, 0, 0, 0)), json.dumps(manifest, indent=2))
    with zipfile.ZipFile(a.output) as dst:
        assert set(dst.namelist()) == set(names) | {'jar/interface-manifest.json'}
        for row in manifest['files']:
            data = dst.read(row['path'])
            assert hashlib.sha256(data).hexdigest() == row['sha256']
            assert data == src.read(row['path']), row['path']
        for style in ('sans', 'serif'):
            for suffix in ('fontdat', 'tga'):
                assert len(dst.read(f'jar/font/{style}/fonts/arialnb.{suffix}')) > 0
        for style in ('up2', 'up4'):
            assert len(dst.read(f'jar/ui/{style}/ui/assets/gradientbar2.tga')) > 0
print(f'MP interface pack verified: {len(names)} exact SP font/UI/notice files; no other asset families')
