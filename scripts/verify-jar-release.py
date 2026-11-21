"""Verify ZIP contents before replacing the previous staged release."""
from hashlib import file_digest
import json
from pathlib import Path
import sys
from zipfile import ZipFile
from tempfile import TemporaryFile
from shutil import copyfileobj
from jar_compact_release import SP, MP, RECIPE, EXPAND, README, logical_hash


def verify(path, version, build_id):
    prefix = 'JediAcademy/'
    manifest_name = prefix+'JAR-SHA256SUMS.txt'
    with ZipFile(path) as archive:
        entries = {}
        transport = {}
        for entry in archive.infolist():
            name = entry.filename.replace('\\', '/')
            if name.endswith('/'):
                continue
            if name in (RECIPE, EXPAND, README):
                if name in transport: raise ValueError('Duplicate transport member')
                transport[name] = entry
                continue
            if not name.startswith(prefix) or name in entries:
                raise ValueError(f'Invalid archive entry {name}')
            entries[name] = entry
        if transport:
            if set(transport) != {RECIPE, EXPAND, README} or prefix+MP in entries or transport[RECIPE].file_size > 16*1024*1024:
                raise ValueError('Invalid compact release layout')
            if archive.read(transport[EXPAND]).decode('utf-8-sig').splitlines() != Path(__file__).with_name(EXPAND).read_text(encoding='utf-8-sig').splitlines():
                raise ValueError('Incorrect materializer')
            with TemporaryFile() as source, archive.open(entries[prefix+SP]) as compressed:
                copyfileobj(compressed, source)
                mp_digest = logical_hash(source,json.loads(archive.read(transport[RECIPE])))
            entries[prefix+MP] = None
        info = json.loads(archive.read(entries[prefix+'JediAcademyRemixed/build-info.json']).decode('utf-8-sig'))
        if (info['Version'], info['BuildId']) != (version, build_id):
            raise ValueError('Build identity mismatch')
        checksums = archive.read(entries[manifest_name]).decode('ascii').splitlines()
        if len(checksums) != len(entries)-1:
            raise ValueError('Incorrect number of package checksums')
        expected = {}
        for row in checksums:
            digest, name = row.split('  ', 1)
            expected[prefix+name] = digest
        if expected.keys() != entries.keys()-{manifest_name}:
            raise ValueError('Missing or duplicate package checksum')
        for name, digest in expected.items():
            if entries[name] is None:
                if digest != mp_digest: raise ValueError('Interface manifest mismatch')
                continue
            with archive.open(entries[name]) as file:
                if file_digest(file, 'sha256').hexdigest() != digest:
                    raise ValueError(f'Checksum mismatch for {name}')
    print(f'Verified {len(entries)} files in the staged JA:R {version} archive.')


if __name__ == '__main__':
    verify(Path(sys.argv[1]), sys.argv[2], sys.argv[3])
