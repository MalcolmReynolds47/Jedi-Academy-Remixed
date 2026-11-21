"""Verify ZIP contents before replacing the previous staged release."""
from hashlib import file_digest
import json
from pathlib import Path
import sys
from zipfile import ZipFile


def verify(path, version, build_id):
    prefix = 'JediAcademy/'
    manifest_name = prefix+'JAR-SHA256SUMS.txt'
    with ZipFile(path) as archive:
        entries = {}
        for entry in archive.infolist():
            name = entry.filename.replace('\\', '/')
            if name.endswith('/'):
                continue
            if not name.startswith(prefix) or name in entries:
                raise ValueError(f'Invalid archive entry {name}')
            entries[name] = entry
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
            with archive.open(entries[name]) as file:
                if file_digest(file, 'sha256').hexdigest() != digest:
                    raise ValueError(f'Checksum mismatch for {name}')
    print(f'Verified {len(entries)} files in the staged JA:R {version} archive.')


if __name__ == '__main__':
    verify(Path(sys.argv[1]), sys.argv[2], sys.argv[3])
