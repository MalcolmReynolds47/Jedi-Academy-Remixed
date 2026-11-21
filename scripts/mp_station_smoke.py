"""Station geometry and recharge through the existing private native MP session."""
import csv
import json
import struct
import time
from mp_world_smoke import checksum


def check_stations(root, mod, game_data, rcon):
    names = ('misc_ammo_floor_unit', 'misc_shield_floor_unit',
             'misc_model_shield_power_converter', 'misc_model_ammo_power_converter',
             'misc_model_health_power_converter')
    catalog = {}
    for line in (root / 'codemp/game/jar_mp_catalog.h').read_text().splitlines():
        if line.startswith(' {'):
            row = next(csv.reader([line.strip()[1:-2]]))
            if len(row) == 9 and row[3] in names:
                catalog[row[3]] = row
    assert len(catalog) == 5
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Saved station_seed revision 1' in rcon('jar_world_save station_seed')
    directory = mod / 'jar/worlds'
    seed = (directory / 'station_seed.0.jrw').read_bytes()
    assert len(seed) == 88 and seed[:4] == b'JRWI'
    data = bytearray(seed[:84]);struct.pack_into('<2I', data, 12, 5, 0)
    for i, name in enumerate(names):
        row = catalog[name];record = bytearray(284)
        struct.pack_into('<6If', record, 0, int(row[0]), 1, 0, 1, 0, 0, 200.)
        struct.pack_into('<3f', record, 28, -8000 + 2000 * i, -8000., 12000.)
        struct.pack_into('<3f', record, 40, 0., 15. * i, 0.)
        struct.pack_into('<6f', record, 52, -1., -1., -1., 1., 1., 1.)
        struct.pack_into('<f', record, 104, 3000.);struct.pack_into('<f', record, 128, 1.)
        struct.pack_into('<4I', record, 164, 20, 200, 100, int(i == 1))
        struct.pack_into('<I', record, 180, 100);struct.pack_into('<I', record, 188, int(i == 2))
        struct.pack_into('<I', record, 280, 60000)
        data += record
    data += struct.pack('<I', checksum(data));(directory / 'stations.0.jrw').write_bytes(data)
    audit = []
    for slot in ('stations', 'stations_seen'):
        assert 'Loaded ' + slot in rcon('jar_world_load ' + slot)
        time.sleep(.5)
        assert '5 objects' in rcon('jar_world_status')
        assert 'Box3D active' in rcon('jar_physics_status')
        target = slot + '_seen'
        assert 'Saved ' + target + ' revision 1' in rcon('jar_world_save ' + target)
        saved = (directory / (target + '.0.jrw')).read_bytes()
        assert saved[:4] == b'JRWI' and len(saved) == 88 + 5 * 284
        assert checksum(saved[:-4]) == struct.unpack_from('<I', saved, len(saved) - 4)[0]
        for i, name in enumerate(names):
            offset = 84 + i * 284
            ident = struct.unpack_from('<I', saved, offset)[0]
            charge, capacity, pending, inactive = struct.unpack_from('<4I', saved, offset + 164)
            cooldown = struct.unpack_from('<I', saved, offset + 280)[0]
            bounds = struct.unpack_from('<6f', saved, offset + 52)
            assert ident == int(catalog[name][0]) and capacity == 200 and inactive == (i == 1)
            assert charge == 20 if inactive else 20 < charge <= 200
            assert 0 <= pending <= 100 and 0 < cooldown < 60000
            assert all(bounds[k] < bounds[k + 3] for k in range(3)) and bounds != (-1., -1., -1., 1., 1., 1.)
            audit.append(dict(slot=slot, name=name, charge=charge, inactive=bool(inactive), cooldown=cooldown, bounds=bounds))
        assert 'world cleared' in rcon('jar_world_clear')
    result = dict(nativeModule=True, spawnedCards=5, restoredCards=5, realModelBounds=True,
                  nativeRecharge=True, inactivePreserved=True, useCooldownPreserved=True,
                  cleanup=True, graphicalClients=0, dispensingGameplayValidated=False, records=audit)
    (mod.parent / 'stations-result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Native MP station spawn/recharge/restore checks passed:', mod.parent)
