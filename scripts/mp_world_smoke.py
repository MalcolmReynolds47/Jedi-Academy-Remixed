"""Native persistence qualification inside test-mp-server's private process/profile."""
import json
import math
import re
import struct
import time
import zipfile


def checksum(data):
    value = 2166136261
    for byte in data:
        value = ((value ^ byte) * 16777619) & 0xffffffff
    return value


def legacy_header(seed):
    data = bytearray(seed[:84])
    struct.pack_into('<I', data, 0, 0x3157524a)
    return data


def decode(path):
    data = path.read_bytes()
    magic, generation, bsp, count, ropes = struct.unpack_from('<5I', data)
    revisions = '123456789ABCDEFGHIJKLMNOP'
    assert data[:3] == b'JRW' and chr(magic >> 24) in revisions
    version = revisions.index(chr(magic >> 24)) + 1
    stride = (88,100,132,136,136,156,180,260,264,268,268,272,272,276,276,280,284,284,284,284,284,284,284,284,284)[version-1]
    link_stride = 172 if version >= 18 else 72 if version >= 5 else 36
    assert checksum(data[:-4]) == struct.unpack_from('<I', data, len(data) - 4)[0]
    nodes = [struct.unpack_from('<6I16f' if stride == 88 else '<6I19f', data, 84 + stride * i) + (struct.unpack_from('<I7f', data, 184 + stride*i) if stride >= 132 else ()) + (struct.unpack_from('<I', data, 216 + stride*i) if stride >= 136 else ()) + (struct.unpack_from('<I4f', data, 220 + stride*i) if stride >= 156 else ()) for i in range(count)]
    links = [struct.unpack_from('<ii7f', data, 84 + stride * count + link_stride * i) + (struct.unpack_from('<I8f', data, 120 + stride * count + link_stride*i) if link_stride >= 72 else ()) for i in range(ropes)]
    paths = [struct.unpack_from('<I24f', data, 156 + stride * count + link_stride*i) for i in range(ropes)] if version >= 18 else []
    offset = 84 + stride * count + link_stride * ropes
    sentinels = []
    shafts = []
    rifts = []
    korribans = []
    yavins = []
    for node in nodes:
        if version >= 19 and node[0] == 88805698:
            offset += 18
        if version >= 20 and node[0] == 89854302:
            lifts = list(struct.iter_unpack('<BH', data[offset:offset+30]))
            pads = list(struct.iter_unpack('<HH', data[offset+30:offset+50]))
            pickups = struct.unpack_from('<45H', data, offset+50)
            sentinels.append(dict(lifts=lifts, pads=pads, pickups=pickups))
            offset += 140
        if version >= 21 and node[0] == 91270120:
            lifts = list(struct.iter_unpack('<BH', data[offset:offset+6]))
            pads = list(struct.iter_unpack('<HH', data[offset+6:offset+14]))
            shafts.append(dict(lifts=lifts, pads=pads))
            offset += 14
        if version >= 21 and node[0] == 88177980:
            rifts.append(struct.unpack_from('<38H', data, offset))
            offset += 76
        if version >= 22 and node[0] == 100481508:
            korribans.append(struct.unpack_from('<47H', data, offset))
            offset += 94
        if version >= 23 and node[0] == 95133242:
            damage = [amount for pair in data[offset:offset+15] for amount in (pair & 15, pair >> 4)]
            dust_ms, = struct.unpack_from('<H', data, offset+15)
            yavins.append(dict(damage=damage, dust_ms=dust_ms))
            offset += 17
    movement = {}
    if version >= 24:
        movement_format = '<HHH' if version >= 25 else '<HHB'
        for node, gravity, friction in struct.iter_unpack(movement_format, data[offset:-4]):
            assert node < count and node not in movement and (gravity or friction)
            assert gravity <= (32767 if version >= 25 else 3201)
            assert friction <= (32767 if version >= 25 else 11)
            movement[node] = (gravity - 1, friction - 1)
        assert movement
        offset = len(data) - 4
    assert offset + 4 == len(data)
    # Existing physics fixtures consume the stable prefix through vehicle motion.
    # Controller fields are appended after that prefix from JRW7 onward.
    return dict(version=version, generation=generation, bsp=bsp, map=data[20:84].split(b'\0')[0].decode(), nodes=nodes,
                ropes=links, rope_paths=paths, sentinels=sentinels, shafts=shafts, rifts=rifts,
                korribans=korribans, yavins=yavins, movement=movement)


def check_construct_overlap(run, mod, rcon):
    directory = mod / 'jar/worlds'
    seed = (directory / 'construct_baseline.0.jrw').read_bytes()
    # The sofa is a linked, solid model. A frozen crate fits inside its hull
    # but is well above the native floor brush. Test both positions.
    crate = 16940136  # models/chunks/crate/crate2_1.md3
    sofa_center = (-3020., -1096., -79.)
    inside_floor = (*sofa_center[:2], -110.)
    for slot, position in (('overlap', sofa_center), ('floor', inside_floor)):
        data = legacy_header(seed)
        struct.pack_into('<2I', data, 12, 1, 0)
        data += struct.pack('<6I16f', crate, 1, 1, 0, 0, 0, 50., *position,
                            0., 0., 0., -7., -7., -3., 7., 7., 3., 0., 0., 0.)
        data += struct.pack('<I', checksum(data))
        (directory / (slot + '.0.jrw')).write_bytes(data)
    result = rcon('jar_world_load overlap')
    assert 'Loaded overlap revision 1: 1 objects' in result, result
    assert 'Saved checked_overlap revision 1: 1 objects' in rcon('jar_world_save checked_overlap')
    prop, = decode(directory / 'checked_overlap.0.jrw')['nodes']
    assert prop[0] == crate and prop[1] == 1 and prop[7:10] == sofa_center, prop
    assert 'world cleared' in rcon('jar_world_clear')
    result = rcon('jar_world_load floor')
    assert 'Not enough clear space' in result, result
    assert 'Saved checked_floor revision 1: 0 objects' in rcon('jar_world_save checked_floor')
    assert rcon('entitylist').count('jar_construct_furniture') == 4
    (run / 'construct-overlap.json').write_text(json.dumps(dict(
        overlapping_native_sofa=sofa_center, blocked_by_floor=inside_floor,
        rejected_placement_created_objects=0, authored_furniture_preserved=4), indent=2) + '\n')


def check_sentinel_journal(run, mod, rcon):
    directory = mod / 'jar/worlds'
    original = directory / 'sentinel_active.0.jrw'
    saved, = decode(original)['sentinels']

    # The real captured save should still have an active lift at the upper
    # stop and a collected weapon after loading into a fresh entity slot.
    assert 'Loaded sentinel_active revision 1: 1 objects' in rcon('jar_world_load sentinel_active')
    assert 'Saved sentinel_resumed revision 1: 1 objects' in rcon('jar_world_save sentinel_resumed')
    resumed, = decode(directory / 'sentinel_resumed.0.jrw')['sentinels']
    for field, position in (('lifts', 1), ('pads', 1)):
        before = max(timer[position] for timer in saved[field])
        after = max(timer[position] for timer in resumed[field])
        assert 0 < after < before, (field, before, after)
    assert 0 < max(resumed['pickups']) < max(saved['pickups']), (saved, resumed)
    assert 'world cleared' in rcon('jar_world_clear')

    # Rehash a payload within the generic timer limit but beyond the first
    # lift's actual travel time. This must roll back the entire section.
    raw = bytearray(original.read_bytes())
    trailer = len(raw) - 144
    struct.pack_into('<BH', raw, trailer, 2, 2050)
    struct.pack_into('<I', raw, len(raw)-4, checksum(raw[:-4]))
    (directory / 'sentinel_bad.0.jrw').write_bytes(raw)
    assert 'rolled back' in rcon('jar_world_load sentinel_bad')
    assert 'World: 0 objects, 0 attachments' in rcon('jar_world_status')
    assert 'jar_section_' not in rcon('entitylist')

    moving = bytearray(original.read_bytes())
    struct.pack_into('<BH', moving, trailer, 2, 225)
    struct.pack_into('<H', moving, trailer+30+4, 400)
    struct.pack_into('<H', moving, trailer+30+6, 9200)
    struct.pack_into('<I', moving, len(moving)-4, checksum(moving[:-4]))
    (directory / 'sentinel_moving.0.jrw').write_bytes(moving)
    assert 'Loaded sentinel_moving revision 1: 1 objects' in rcon('jar_world_load sentinel_moving')
    assert 'Saved sentinel_queued revision 1: 1 objects' in rcon('jar_world_save sentinel_queued')
    queued, = decode(directory / 'sentinel_queued.0.jrw')['sentinels']
    # At the first save the restored ascent has finished, and the previously
    # pending pad has started the independent second lift group.
    assert queued['lifts'][0][0] == 1
    assert any(phase == 2 for phase, _ in queued['lifts']), queued
    assert queued['pads'][1][0] == 0 and 0 < queued['pads'][1][1] < 9200
    assert 'world cleared' in rcon('jar_world_clear')

    # Version 1.165 wrote the root in JRW18. It should load the stock pose.
    legacy = bytearray(original.read_bytes()[:trailer])
    legacy[3] = ord('I')
    legacy += struct.pack('<I', checksum(legacy))
    (directory / 'sentinel_legacy.0.jrw').write_bytes(legacy)
    assert 'Loaded sentinel_legacy revision 1: 1 objects' in rcon('jar_world_load sentinel_legacy')
    assert 'Saved sentinel_stock revision 1: 1 objects' in rcon('jar_world_save sentinel_stock')
    stock, = decode(directory / 'sentinel_stock.0.jrw')['sentinels']
    assert not any(phase or timer for phase, timer in stock['lifts'])
    assert not any(pending or timer for pending, timer in stock['pads']) and not any(stock['pickups'])
    assert 'world cleared' in rcon('jar_world_clear')
    (run / 'sentinel-journal.json').write_text(json.dumps(dict(
        version=20, saved=saved, resumed=resumed, queued=queued,
        legacy_defaults=True, malformed_travel_rolled_back=True),
        indent=2) + '\n', encoding='utf-8')


def check_section_timers(run, mod, rcon, name):
    directory = mod / 'jar/worlds'
    active = directory / f'{name}_active.0.jrw'
    group = f'{name}s'
    saved, = decode(active)[group]
    assert f'Loaded {name}_active revision 1: 1 objects' in rcon(f'jar_world_load {name}_active')
    assert f'Saved {name}_resumed revision 1: 1 objects' in rcon(f'jar_world_save {name}_resumed')
    resumed, = decode(directory / f'{name}_resumed.0.jrw')[group]
    if name == 'shaft':
        phase, timer = resumed['lifts'][0]
        upper_wait = saved['lifts'][0][1]
        if phase == 1:
            assert 0 < timer < upper_wait
            elapsed = upper_wait - timer
        else:
            # The 266-unit lift can start its descent before the RCON readback.
            assert phase == 3 and 50 <= timer <= 583, resumed
            elapsed = upper_wait + timer - 50
        assert abs(resumed['pads'][0][1] - max(0, saved['pads'][0][1] - elapsed)) <= 50
        assert resumed['lifts'][1] == resumed['pads'][1] == (0, 0)
    elif name == 'rift':
        assert sum(timer > 0 for timer in resumed) == 1 and 0 < max(resumed) < max(saved)
    else:
        weapon, = [i for i, timer in enumerate(saved) if 0 < timer <= 5000]
        assert 0 < resumed[weapon] < saved[weapon]
        assert all(0 <= after <= before for before, after in zip(saved, resumed))
    assert 'world cleared' in rcon('jar_world_clear')

    raw = active.read_bytes()
    trailer = {'shaft': 14, 'rift': 76, 'korriban': 94}[name]
    legacy = bytearray(raw[:-trailer-4])
    legacy[3] = ord('I')
    legacy += struct.pack('<I', checksum(legacy))
    (directory / f'{name}_legacy.0.jrw').write_bytes(legacy)
    assert f'Loaded {name}_legacy revision 1: 1 objects' in rcon(f'jar_world_load {name}_legacy')
    assert f'Saved {name}_stock revision 1: 1 objects' in rcon(f'jar_world_save {name}_stock')
    stock, = decode(directory / f'{name}_stock.0.jrw')[group]
    assert stock == dict(lifts=[(0, 0)]*2, pads=[(0, 0)]*2) if name == 'shaft' else not any(stock)
    assert 'world cleared' in rcon('jar_world_clear')

    (run / f'{name}-journal.json').write_text(json.dumps(dict(version=22 if name == 'korriban' else 21,
                                                            saved=saved, resumed=resumed,
                                                            legacy_defaults=True), indent=2) + '\n',
                                             encoding='utf-8')


def check_yavin_journal(run, mod, rcon):
    directory = mod / 'jar/worlds'
    original = directory / 'yavin_active.0.jrw'
    saved, = decode(original)['yavins']
    assert not any(saved['damage']) and 0 < saved['dust_ms'] <= 8000, saved

    def write_state(slot, brush_pair, dust_ms):
        raw = bytearray(original.read_bytes())
        struct.pack_into('<B', raw, len(raw)-21, brush_pair)
        struct.pack_into('<H', raw, len(raw)-6, dust_ms)
        struct.pack_into('<I', raw, len(raw)-4, checksum(raw[:-4]))
        (directory / f'{slot}.0.jrw').write_bytes(raw)

    # Seed one fully destroyed brush and a second with partial damage.
    write_state('yavin_damaged', 0x28, 7500)
    loaded = rcon('jar_world_load yavin_damaged')
    assert 'Loaded yavin_damaged revision 1: 1 objects' in loaded, loaded
    assert 'Saved yavin_resumed revision 1: 1 objects' in rcon('jar_world_save yavin_resumed')
    resumed, = decode(directory / 'yavin_resumed.0.jrw')['yavins']
    assert resumed['damage'] == [8, 2] + [0]*28 and 0 < resumed['dust_ms'] < 7500, resumed
    assert 'world cleared' in rcon('jar_world_clear')

    for slot, brush_pair, dust_ms in (('yavin_bad_damage', 0x2f, 7500), ('yavin_bad_timer', 0x28, 8001)):
        write_state(slot, brush_pair, dust_ms)
        result = rcon(f'jar_world_load {slot}')
        assert 'No valid world revision found' in result, result
        assert 'World: 0 objects, 0 attachments' in rcon('jar_world_status')

    (run / 'yavin-journal.json').write_text(json.dumps(dict(
        version=23, saved=saved, resumed=resumed, invalid_damage_and_timer_rejected=True), indent=2) + '\n',
        encoding='utf-8')


def spawn_points(game_data, map_name, airspace=False):
    # Use retail deathmatch spawn positions, not guessed coordinates.
    entities = None
    for archive in sorted((game_data / 'base').glob('assets*.pk3')):
        with zipfile.ZipFile(archive) as pk3:
            if map_name in pk3.namelist():
                bsp = pk3.read(map_name)
                offset, size = struct.unpack_from('<ii', bsp, 8)
                entities = bsp[offset:offset + size].decode('latin1')
                break
    assert entities, map_name
    points = []
    for block in re.findall(r'\{([^}]*)\}', entities):
        entity = dict(re.findall(r'"([^"]*)"\s+"([^"]*)"', block))
        if entity.get('classname') == 'info_player_deathmatch':
            points.append(tuple(map(float, entity['origin'].split())))
    assert len(points) >= 2
    if airspace:
        # Large non-opaque BSP leaf boxes provide candidate airspace. These
        # boxes are only candidates: the production hull trace must still pass.
        offset, size = struct.unpack_from('<ii', bsp, 8 + 4 * 8)
        candidates = []
        for leaf in struct.iter_unpack('<12i', bsp[offset:offset + size]):
            low, high = leaf[2:5], leaf[5:8]
            span = [high[a] - low[a] for a in range(3)]
            if leaf[0] >= 0 and min(span) >= 384 and max(span) <= 32000:
                candidates.append((math.prod(span), tuple((low[a] + high[a]) * .5 for a in range(3))))
        points = [point for _, point in sorted(candidates, reverse=True)[:12]] + points
    return points


def check_world(root, mod, game_data, rcon, vehicles=False, forces=False):
    assert 'Saved seed revision 1: 0 objects, 0 attachments' in rcon('jar_world_save seed')
    directory = mod / 'jar/worlds'
    seed = (directory / 'seed.0.jrw').read_bytes()
    assert len(seed) == 88
    points = spawn_points(game_data, 'maps/mp/ffa3.bsp')
    catalog = (root / 'codemp/game/jar_mp_catalog.h').read_text(encoding='utf-8')
    def entry(name):
        match = re.search(r'\{(\d+),(\d+),\d+,"' + re.escape(name) + r'","[^"]*","[^"]*",([^,]+),([^,]+),([^}]+)\}', catalog)
        assert match, name
        ident, kind = map(int, match.groups()[:2])
        radius, height, bottom = map(float, match.groups()[2:])
        return ident, kind, (-radius, -radius, bottom), (radius, radius, bottom + height)
    prop = entry('models/chunks/crate/crate2_1.md3')
    npc = entry('stormtrooper2')
    def fixture(slot, points, seed):
        prop_record = struct.pack('<6I16f', prop[0], 1, 1, 1, 0, 0, 250., *points[0], 0., 25., 0., *prop[2], *prop[3], 12., 34., 0.)
        npc_record = struct.pack('<6I16f', npc[0], 1, 0, 0, 173, 0, 80., *points[1], 0., 90., 0., *npc[2], *npc[3], 0., 0., 0.)
        internal = struct.pack('<ii7f', 0, 1, 1., 2., 3., 4., 5., 6., math.dist(points[0], points[1]) + 100)
        anchor = (*points[0][:2], points[0][2] + 64)
        external = struct.pack('<ii7f', 0, -1, 0., 0., 0., *anchor, 64.)
        data = legacy_header(seed)
        struct.pack_into('<2I', data, 12, 2, 2)
        data += prop_record + npc_record + internal + external
        data += struct.pack('<I', checksum(data))
        (directory / (slot+'.0.jrw')).write_bytes(data)
        return anchor
    fixture('blocked_route', points, seed)
    rejected = rcon('jar_world_load blocked_route')
    assert 'No clear rope route' in rejected and 'rolled back' in rejected, rejected
    assert 'World: 0 objects, 0 attachments' in rcon('jar_world_status')
    from mp_network_smoke import SECTION_MAP
    rcon('map '+SECTION_MAP)
    assert 'Saved open_seed revision 1' in rcon('jar_world_save open_seed')
    points = [(0., 0., 128.), (128., 0., 128.)]
    anchor = fixture('fixture', points, (directory/'open_seed.0.jrw').read_bytes())
    result = rcon('jar_world_load fixture')
    assert 'Loaded fixture revision 1: 2 objects, 2 attachments' in result, result
    # Longer than the pending-native timeout: verifies actual NPC_Begin callback.
    time.sleep(4)
    assert 'World: 2 objects, 2 attachments, 0 pending native members' in rcon('jar_world_status')
    assert 'Saved roundtrip revision 1: 2 objects, 2 attachments' in rcon('jar_world_save roundtrip')
    roundtrip = decode(directory / 'roundtrip.0.jrw')
    by_id = {node[0]: node for node in roundtrip['nodes']}
    assert by_id[prop[0]][1:7] == (1, 1, 1, 0, 0, 250.)
    assert by_id[prop[0]][7:10] == points[0]
    assert by_id[prop[0]][19:22] == (12., 34., 0.)
    assert by_id[npc[0]][1] == 1 and by_id[npc[0]][4] == 173
    assert all(abs(a-b) < .1 for a, b in zip(by_id[npc[0]][7:10], points[1]))
    assert roundtrip['ropes'][1][1] == -1 and roundtrip['ropes'][1][5:8] == anchor
    assert 'Save/clear the existing Sandbox world first' in rcon('jar_world_load roundtrip')
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'World: 0 objects, 0 attachments' in rcon('jar_world_status')
    assert 'Loaded roundtrip revision 1' in rcon('jar_world_load roundtrip')
    time.sleep(4)
    assert 'World: 2 objects, 2 attachments, 0 pending native members' in rcon('jar_world_status')
    # Map-start restore uses a fresh module with no connected players.
    rcon('set jar_worldLoad roundtrip')
    rcon('map '+SECTION_MAP)
    time.sleep(4)
    assert 'World: 2 objects, 2 attachments, 0 pending native members' in rcon('jar_world_status')
    assert 'world cleared' in rcon('jar_world_clear')
    (mod.parent / 'world-result.json').write_text(json.dumps(dict(
        native_module=True, connected_players_during_restore=0, catalog_ids=[prop[0], npc[0]],
        native_npc_health=173, native_npc_freeze=True, internal_and_world_ropes=True,
        save_reload=True, map_start_restore=True, preserved_prop_velocity=True,
        map=roundtrip['map'], bsp_checksum=roundtrip['bsp']), indent=2) + '\n', encoding='utf-8')
    rcon('set jar_worldLoad ""')
    rcon('map mp/ffa3')
    if vehicles or forces:
        # Taspir has outdoor airspace for the native fighter hull. Keep the
        # smaller actor/world regression above on its original Tatooine map.
        rcon('set jar_worldLoad ""')
        rcon('devmap mp/ffa5' if forces else 'map mp/ffa5')
        assert 'Saved airspace revision 1: 0 objects, 0 attachments' in rcon('jar_world_save airspace')
        seed = (directory / 'airspace.0.jrw').read_bytes()
        points = spawn_points(game_data, 'maps/mp/ffa5.bsp', airspace=True)
        results = []
        for name in (('swoop_mp', 'tie-fighter', 'wildtauntaun', 'atst_vehicle') if vehicles else ()):
            vehicle = entry(name)
            loaded = False
            # The real hull trace decides which retail spawn area has room;
            # every rejected placement must leave the world empty.
            for source, lift in ((p, h) for h in (0, 32, 64, 128) for p in points):
                position = (source[0], source[1], source[2] + lift)
                record = struct.pack('<6I16f', vehicle[0], 1, 0, 0, 230, 0, 500., *position, 20., 45., 15., *vehicle[2], *vehicle[3], 0., 0., 0.)
                data = legacy_header(seed);struct.pack_into('<2I', data, 12, 1, 0)
                data += record;data += struct.pack('<I', checksum(data))
                (directory / 'vehicle.0.jrw').write_bytes(data)
                response = rcon('jar_world_load vehicle')
                if 'Loaded vehicle revision 1' in response:
                    loaded = True
                    break
                assert 'World: 0 objects, 0 attachments' in rcon('jar_world_status')
            assert loaded, f'No clear retail spawn area for {name}'
            time.sleep(4)
            assert 'World: 1 objects, 0 attachments, 0 pending native members' in rcon('jar_world_status'), name
            slot = 'checked_' + name.replace('-', '_')
            assert '1 objects, 0 attachments' in rcon('jar_world_save ' + slot)
            saved = decode(directory / (slot + '.0.jrw'))['nodes'][0]
            assert saved[0] == vehicle[0] and saved[1] == 1 and saved[4] == 230, (name, saved)
            assert all(abs(a-b) < .1 for a, b in zip(saved[7:10], position)), (name, saved)
            assert saved[10:13] == (20., 45., 15.), (name, saved)
            results.append(dict(name=name, id=vehicle[0], frozen=True, health=230, position=position, angles=saved[10:13]))
            assert 'world cleared' in rcon('jar_world_clear')
        (mod.parent / 'vehicle-result.json').write_text(json.dumps(dict(native_module=True, map='mp/ffa5', connected_players=0, families=results), indent=2) + '\n', encoding='utf-8')
        if forces:
            from mp_force_smoke import check_forces
            check_forces(mod, rcon, seed, points[0], entry)
        rcon('map mp/ffa3')
