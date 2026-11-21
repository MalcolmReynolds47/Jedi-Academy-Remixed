"""Transport an ordinary pair of private PK3s without storing shared deflate streams twice."""
import base64
import hashlib
import json
import struct
import sys
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

SP = 'JediAcademyRemixed/jar-appearance.pk3'
MP = 'JediAcademyRemixedMP/jar-interface.pk3'
RECIPE = 'JAR-interface.json'
EXPAND = 'Expand-JA-R.ps1'
README = 'READ-ME-FIRST.txt'


def offset(stream, entry):
    stream.seek(entry.header_offset)
    header = stream.read(30)
    if header[:4] != b'PK\x03\x04':
        raise ValueError('Invalid local ZIP header')
    name_len, extra_len = struct.unpack_from('<HH', header, 26)
    return entry.header_offset + 30 + name_len + extra_len


def recipe(source, target):
    ops = []
    copied = 0
    with source.open('rb') as a, target.open('rb') as b, ZipFile(source) as az, ZipFile(target) as bz:
        last = 0
        for entry in sorted(bz.infolist(), key=lambda e: e.header_offset):
            if entry.filename not in az.NameToInfo:
                continue
            other = az.getinfo(entry.filename)
            if (other.CRC, other.compress_size, other.file_size) != (entry.CRC, entry.compress_size, entry.file_size):
                continue
            pos, src = offset(b, entry), offset(a, other)
            a.seek(src); b.seek(pos)
            if a.read(other.compress_size) != b.read(entry.compress_size):
                continue
            b.seek(last)
            ops.append(base64.b64encode(b.read(pos-last)).decode('ascii'))
            ops.append([src, entry.compress_size])
            last = pos + entry.compress_size
            copied += entry.compress_size
        b.seek(last)
        ops.append(base64.b64encode(b.read()).decode('ascii'))
        b.seek(0)
        digest = hashlib.file_digest(b, 'sha256').hexdigest()
    if copied < target.stat().st_size * .95:
        raise ValueError('Private packs no longer share their compressed UI streams')
    return dict(schema=1, size=target.stat().st_size, sha256=digest, segments=ops)


def logical_hash(source, plan):
    digest, size = hashlib.sha256(), 0
    if plan['schema'] != 1 or not 0 < plan['size'] < 2**31:
        raise ValueError('Invalid compact interface recipe')
    for op in plan['segments']:
        if isinstance(op, str):
            data = base64.b64decode(op, validate=True)
            digest.update(data); size += len(data)
        else:
            offset, count = op
            if offset < 0 or count < 0 or size+count > plan['size']:
                raise ValueError('Invalid source range')
            source.seek(offset)
            while count:
                block = source.read(min(count, 1048576))
                if not block:
                    raise ValueError('Truncated shared archive')
                count -= len(block); size += len(block); digest.update(block)
    if size != plan['size'] or digest.hexdigest() != plan['sha256']:
        raise ValueError('Materialized interface checksum mismatch')
    return digest.hexdigest()


def pack(root, output):
    plan = recipe(root/SP, root/MP)
    with ZipFile(output, 'x', ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(root.rglob('*')):
            if path.is_file() and path != root/MP:
                archive.write(path, 'JediAcademy/'+path.relative_to(root).as_posix())
        archive.writestr(RECIPE, json.dumps(plan, separators=(',', ':')))
        archive.write(Path(__file__).with_name(EXPAND), EXPAND)
        archive.writestr(README, 'Extract this ZIP to an empty folder. Run Expand-JA-R.ps1 there with PowerShell. '
                        'It restores and verifies the multiplayer interface. Then copy the contents of JediAcademy '
                        'to your Jedi Academy GameData folder. Source users: deploy-jar-release.ps1 handles this automatically.\r\n')
    print('Shared compressed bytes stored once:', sum(op[1] for op in plan['segments'] if isinstance(op,list)))


if __name__ == '__main__':
    pack(Path(sys.argv[1]), Path(sys.argv[2]))
