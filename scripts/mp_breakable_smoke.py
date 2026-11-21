"""Portable breakables in the existing private native MP server session."""
import csv
import json
import struct
import time
from mp_world_smoke import checksum


def check_cargo_barrels(mod, directory, seed, cargo_id, rcon):
    assert cargo_id == 91859725  # Keep the single-player card's stable ID.
    # Count, maximum, remaining fuse, disabled, radius, active state, damage.
    states = (
        ('armed', (31, 40, 0, 0, 200, 1, 100)),
        ('disabled pending', (0, 55, 350, 1, 321, 2, 123)),
        ('enabled pending', (0, 62, 500, 0, 150, 2, 75)),
    )
    # Retail Imperial barrels.md3 frame 0, rotated to match each record's yaw.
    hulls = (
        (-16.4375, -32.765625, -.21875, 19.78125, 32.75, 48.),
        (-32.75, -16.4375, -.21875, 32.765625, 19.78125, 48.),
        (-19.78125, -32.75, -.21875, 16.4375, 32.765625, 48.),
    )
    data = bytearray(seed[:84])
    struct.pack_into('<2I', data, 12, len(states), 0)
    for i, (_, state) in enumerate(states):
        record = bytearray(284)
        struct.pack_into('<6If', record, 0, cargo_id, 1, 0, 1, 0, 0, 200.)
        struct.pack_into('<6f', record, 28, -12000 + 2000 * i, -12000., 12000., 0., 90. * i, 0.)
        struct.pack_into('<6f', record, 52, -1., -1., -1., 1., 1., 1.)
        struct.pack_into('<f', record, 104, 3000.)
        struct.pack_into('<f', record, 128, 1.)
        struct.pack_into('<4I', record, 164, *state[:4])
        struct.pack_into('<3I', record, 260, *state[4:])
        data += record
    data += struct.pack('<I', checksum(data))
    (directory / 'cargo.0.jrw').write_bytes(data)

    def capture(slot, expired):
        saved = (directory / (slot + '.0.jrw')).read_bytes()
        count = len(states) - expired
        assert len(saved) == 88 + count * 284 and saved[:4] == b'JRWI'
        assert struct.unpack_from('<2I', saved, 12) == (count, 0)
        assert checksum(saved[:-4]) == struct.unpack_from('<I', saved, len(saved) - 4)[0]
        result = []
        for i, (label, expected) in enumerate(states[:count]):
            offset = 84 + i * 284
            assert struct.unpack_from('<I', saved, offset)[0] == cargo_id
            assert struct.unpack_from('<6f', saved, offset + 28) == (
                -12000 + 2000 * i, -12000., 12000., 0., 90. * i, 0.)
            bounds = struct.unpack_from('<6f', saved, offset + 52)
            assert all(abs(actual - retail) < .001 for actual, retail in zip(bounds, hulls[i]))
            state = (struct.unpack_from('<4I', saved, offset + 164)
                     + struct.unpack_from('<3I', saved, offset + 260))
            if expired and label == 'disabled pending':
                assert 0 < state[2] <= 100  # Disabled native think rechecks at FRAMETIME.
                assert state[:2] + state[3:] == expected[:2] + expected[3:]
            else:
                assert state == expected
            assert saved[offset + 156:offset + 164] == bytes(8)  # no helper links
            assert saved[offset + 180:offset + 260] == bytes(80)
            assert saved[offset + 272:offset + 284] == bytes(12)
            result.append(dict(slot=slot, state=label, health=state[0], maximum=state[1],
                               remaining=state[2], disabled=state[3], radius=state[4],
                               active=state[5], damage=state[6], bounds=bounds))
        return result

    audit = []
    for source, target in (('cargo', 'cargo_seen'), ('cargo_seen', 'cargo_reloaded')):
        # RCON throttling exceeds the 500 ms fuse. Queue load and capture in
        # the same server frame so the live pending record can be read back.
        config = target + '.cfg'
        (mod / config).write_text(f'jar_world_load {source}\njar_world_save {target}\n', encoding='ascii')
        assert 'execing ' + config in rcon('exec ' + config)
        saved = directory / (target + '.0.jrw')
        deadline = time.monotonic() + 5
        while not saved.exists() and time.monotonic() < deadline:
            time.sleep(.05)
        assert saved.is_file(), target
        audit.extend(capture(target, expired=False))
        deadline = time.monotonic() + 5
        while 'World: 2 objects' not in rcon('jar_world_status'):
            assert time.monotonic() < deadline, 'Enabled cargo fuse did not expire'
        after = target + '_expired'
        assert 'Saved ' + after + ' revision 1: 2 objects, 0 attachments' in rcon('jar_world_save ' + after)
        audit.extend(capture(after, expired=True))
        assert 'world cleared' in rcon('jar_world_clear')
    assert 'World: 0 objects, 0 attachments' in rcon('jar_world_status')
    return audit


def check_breakables(root, mod, rcon):
    rows = [next(csv.reader([line.strip()[1:-2]])) for line in
            (root / 'codemp/game/jar_mp_catalog.h').read_text().splitlines()
            if line.startswith(' {')]
    names = ('func_breakable', 'misc_model_breakable', 'func_glass')
    ids = {name: int(next(row for row in rows if len(row) == 9 and row[3] == name)[0]) for name in names}
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Saved break_seed revision 1' in rcon('jar_world_save break_seed')
    directory = mod / 'jar/worlds'
    seed = (directory / 'break_seed.0.jrw').read_bytes()
    assert len(seed) == 88 and seed[:4] == b'JRWI'
    data = bytearray(seed[:84]); struct.pack_into('<2I', data, 12, 12, 0)
    for i in range(12):
        name = names[i // 4]; state = i % 4
        active = 1 if state == 0 else 0 if state == 1 else 2
        record = bytearray(284)
        struct.pack_into('<6If', record, 0, ids[name], 1, 0, 1, 0, 0, 200.)
        struct.pack_into('<3f', record, 28, -12000 + 2000 * i, -12000., 12000.)
        struct.pack_into('<3f', record, 40, 0., i * 15., 0.)
        struct.pack_into('<6f', record, 52, -1., -1., -1., 1., 1., 1.)
        struct.pack_into('<f', record, 104, 3000.); struct.pack_into('<f', record, 128, 1.)
        struct.pack_into('<4I', record, 164, 75 if active == 1 else 0, 100,
                         60000 if state == 3 else 1 if state == 2 else 0, int(state == 3))
        struct.pack_into('<4I', record, 180, 60000, 60000, 128, 1 if name == 'func_glass' else 8 if name == 'misc_model_breakable' else 0)
        struct.pack_into('<I', record, 264, active)
        struct.pack_into('<2I', record, 276, 100, 60000)
        data += record
    data += struct.pack('<I', checksum(data)); (directory / 'breakables.0.jrw').write_bytes(data)
    audit = []
    for slot in ('breakables', 'breakables_seen'):
        assert 'Loaded ' + slot in rcon('jar_world_load ' + slot)
        time.sleep(.4)
        assert '12 objects' in rcon('jar_world_status')
        assert 'Box3D active' in rcon('jar_physics_status')
        target = slot + '_seen'
        assert 'Saved ' + target + ' revision 1' in rcon('jar_world_save ' + target)
        saved = (directory / (target + '.0.jrw')).read_bytes()
        assert saved[:4] == b'JRWI' and len(saved) == 88 + 12 * 284
        assert checksum(saved[:-4]) == struct.unpack_from('<I', saved, len(saved) - 4)[0]
        for i in range(12):
            offset = 84 + i * 284; state = i % 4
            ident = struct.unpack_from('<I', saved, offset)[0]
            health, maximum, remaining, disabled = struct.unpack_from('<4I', saved, offset + 164)
            active = struct.unpack_from('<I', saved, offset + 264)[0]
            pain = struct.unpack_from('<I', saved, offset + 280)[0]
            bounds = struct.unpack_from('<6f', saved, offset + 52)
            assert ident == ids[names[i // 4]] and (health, maximum, disabled) == (75 if state == 0 else 0, 100, int(state == 3))
            assert active == (1 if state == 0 else 2 if state == 3 else 0)
            assert (0 < remaining <= 60000) if state == 3 else remaining == 0
            assert 0 < pain < 60000
            assert all(bounds[k] < bounds[k + 3] for k in range(3)) and bounds != (-1., -1., -1., 1., 1., 1.)
            audit.append(dict(slot=slot, index=i, active=active, remaining=remaining,
                              disabled=disabled, health=health, bounds=bounds, painRemaining=pain))
        assert 'world cleared' in rcon('jar_world_clear')
    cargo_id = int(next(row for row in rows if len(row) == 9 and row[3] == 'object_cargo_barrel1')[0])
    cargo = check_cargo_barrels(mod, directory, seed, cargo_id, rcon)
    result = dict(nativeModule=True, spawnedStates=12, restoredStates=12,
                  nativeDestructionThink=True, destroyedPreserved=True, inactivePendingPreserved=True,
                  realModelBounds=True, painCooldownPreserved=True, cleanup=True,
                  graphicalClients=0, weaponDamageGameplayValidated=False, records=audit,
                  cargoJournalVersion=18, cargoRecords=cargo)
    (mod.parent / 'breakable-result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Native MP breakable spawn/state/restore checks passed:', mod.parent)
