"""Package the existing SP font/UI bytes for MP, without other SP asset families."""
import argparse
import copy
import hashlib
import json
import struct
from pathlib import Path
import zipfile
from jar_zip_members import append_member


def copy_deflated(src, dst, name):
    """Reuse a verified, locally generated compressed member in CPython's ZIP writer."""
    info = copy.copy(src.getinfo(name))
    if info.compress_type != zipfile.ZIP_DEFLATED or info.flag_bits or info.file_size >= 2**31:
        # Accept other source ZIP layouts through the normal stdlib writer.
        dst.writestr(zipfile.ZipInfo(name, (2026, 10, 12, 0, 0, 0)), src.read(name),
                     compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)
        return
    src.fp.seek(info.header_offset)
    local = src.fp.read(30)
    if local[:4] != b'PK\x03\x04' or len(local) != 30:
        raise zipfile.BadZipFile(name)
    name_size, extra_size = struct.unpack_from('<HH', local, 26)
    src.fp.seek(name_size + extra_size, 1)
    info.date_time = (2026, 10, 12, 0, 0, 0)
    def blocks():
        left = info.compress_size
        while left:
            block = src.fp.read(min(left, 1024 * 1024))
            if not block:
                raise zipfile.BadZipFile(name)
            yield block
            left -= len(block)
    append_member(dst, info, blocks())

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
            copy_deflated(src, dst, n)
            manifest['files'].append({'path': n, 'sha256': hashlib.sha256(data).hexdigest()})
        dst.writestr(zipfile.ZipInfo('jar/interface-manifest.json', (2026, 10, 12, 0, 0, 0)), json.dumps(manifest, indent=2))
    with zipfile.ZipFile(a.output) as dst:
        assert set(dst.namelist()) == set(names) | {'jar/interface-manifest.json'}
        for row in manifest['files']:
            data = dst.read(row['path'])
            assert hashlib.sha256(data).hexdigest() == row['sha256']
        for style in ('sans', 'serif'):
            for suffix in ('fontdat', 'tga'):
                assert len(dst.read(f'jar/font/{style}/fonts/arialnb.{suffix}')) > 0
        for style in ('up2', 'up4'):
            assert len(dst.read(f'jar/ui/{style}/ui/assets/gradientbar2.tga')) > 0
print(f'MP interface pack verified: {len(names)} exact SP font/UI/notice files; no other asset families')
