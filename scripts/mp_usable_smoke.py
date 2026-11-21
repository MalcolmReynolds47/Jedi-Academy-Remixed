"""Portable usable state through the existing private native MP server session."""
import csv
import json
import struct
import time
from mp_world_smoke import checksum


def check_usable(root, mod, rcon):
    rows = [next(csv.reader([line.strip()[1:-2]])) for line in
            (root / 'codemp/game/jar_mp_catalog.h').read_text().splitlines()
            if line.startswith(' {')]
    row = next(row for row in rows if len(row) == 9 and row[3] == 'func_usable')
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Saved usable_seed revision 1' in rcon('jar_world_save usable_seed')
    directory = mod / 'jar/worlds'
    seed = (directory / 'usable_seed.0.jrw').read_bytes()
    assert len(seed) == 88 and seed[:4] == b'JRWI'
    data = bytearray(seed[:84]);struct.pack_into('<2I', data, 12, 6, 0)
    states = ((0, 0, 0), (0, 1, 0), (1, 2, 0), (1, 3, 0), (1, 2, 1), (1, 2, 0))
    for i, (active, pending, inactive) in enumerate(states):
        record = bytearray(284)
        struct.pack_into('<6If', record, 0, int(row[0]), 1, 0, 1, 0, 0, 200.)
        struct.pack_into('<3f', record, 28, -8000 + 2000 * i, -8000., 12000.)
        struct.pack_into('<3f', record, 40, 0., i * 15., 0.)
        struct.pack_into('<6f', record, 52, -1., -1., -1., 1., 1., 1.)
        struct.pack_into('<f', record, 104, 3000.);struct.pack_into('<f', record, 128, 1.)
        struct.pack_into('<4I', record, 164, 75, 100, 50 if pending == 1 or i == 5 else 60000 if pending == 2 else 0, inactive)
        struct.pack_into('<4I', record, 180, 60000 if active else 0, 0, 88 if active else 80, pending)
        struct.pack_into('<I', record, 264, active)
        data += record
    data += struct.pack('<I', checksum(data));(directory / 'usable.0.jrw').write_bytes(data)
    audit = []
    for slot in ('usable', 'usable_seen'):
        assert 'Loaded ' + slot in rcon('jar_world_load ' + slot)
        time.sleep(.4)
        assert '6 objects' in rcon('jar_world_status')
        assert 'Box3D active' in rcon('jar_physics_status')
        target = slot + '_seen'
        assert 'Saved ' + target + ' revision 1' in rcon('jar_world_save ' + target)
        saved = (directory / (target + '.0.jrw')).read_bytes()
        assert saved[:4] == b'JRWI' and len(saved) == 88 + 6 * 284
        assert checksum(saved[:-4]) == struct.unpack_from('<I', saved, len(saved) - 4)[0]
        for i, (_, pending, inactive) in enumerate(states):
            offset = 84 + i * 284
            ident = struct.unpack_from('<I', saved, offset)[0]
            health, maximum, remaining, disabled = struct.unpack_from('<4I', saved, offset + 164)
            mode = struct.unpack_from('<I', saved, offset + 192)[0]
            visible = struct.unpack_from('<I', saved, offset + 264)[0]
            bounds = struct.unpack_from('<6f', saved, offset + 52)
            assert ident == int(row[0]) and (health, maximum, disabled) == (75, 100, inactive)
            assert visible == (i != 0) and mode == (0 if i in (1, 5) else pending)
            assert (0 < remaining < 60000) if i in (2, 4) else remaining == 0
            assert all(bounds[k] < bounds[k + 3] for k in range(3)) and bounds != (-1., -1., -1., 1., 1., 1.)
            audit.append(dict(slot=slot, index=i, visible=visible, pending=mode,
                              remaining=remaining, disabled=disabled, health=health, bounds=bounds))
        assert 'world cleared' in rcon('jar_world_clear')
    result = dict(nativeModule=True, spawnedStates=6, restoredStates=6,
                  nativeReturnThink=True, nativeCooldownThink=True, cooldownPreserved=True, spentPreserved=True,
                  hiddenPreserved=True, inactivePreserved=True, realModelBounds=True,
                  cleanup=True, graphicalClients=0, directUseGameplayValidated=False, records=audit)
    (mod.parent / 'usable-result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Native MP usable spawn/state/restore checks passed:', mod.parent)
