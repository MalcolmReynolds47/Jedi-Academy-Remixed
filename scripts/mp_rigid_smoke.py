"""Real retail-map rigid physics in the private test-owned dedicated server."""
import json
import math
import re
import struct
import time
import zipfile
from mp_world_smoke import checksum, decode, spawn_points


def check_rigid(root, mod, game_data, rcon):
    rcon('set jar_worldLoad ""')
    rcon('set jar_worldAutoSave 0')
    assert 'world cleared' in rcon('jar_world_clear')
    rcon('map mp/ffa3')
    assert 'world cleared' in rcon('jar_world_clear')
    directory = mod / 'jar/worlds'
    assert 'Saved rigid_seed revision 1' in rcon('jar_world_save rigid_seed')
    seed = (directory / 'rigid_seed.0.jrw').read_bytes()
    name = 'models/chunks/crate/crate2_1.md3'
    catalog = (root / 'codemp/game/jar_mp_catalog.h').read_text(encoding='utf-8')
    ident = int(re.search(r'\{(\d+),1,\d+,"'+re.escape(name)+'"', catalog)[1])
    bounds = None
    for archive in sorted((game_data / 'base').glob('assets*.pk3')):
        with zipfile.ZipFile(archive) as z:
            if name in z.namelist():
                data = z.read(name)
                bounds = struct.unpack_from('<6f', data, struct.unpack_from('<I', data, 92)[0])
    assert bounds
    point = spawn_points(game_data, 'maps/mp/ffa3.bsp')[0]

    def node(position, frozen=0, gravity=0, spin=0):
        return struct.pack('<6I19f', ident, frozen, 0, gravity, 0, 0, 250., *position,
                           0., 0., 0., *bounds, 0., 0., 0., 0., 0., spin)

    def fixture(slot, records):
        data = bytearray(seed[:84])
        struct.pack_into('<I', data, 0, 0x3257524a)
        struct.pack_into('<2I', data, 12, len(records), 0)
        data += b''.join(records)
        data += struct.pack('<I', checksum(data))
        (directory / (slot+'.0.jrw')).write_bytes(data)
        assert 'Loaded '+slot+' revision 1' in rcon('jar_world_load '+slot)

    def save(slot):
        assert 'Saved '+slot+' revision 1' in rcon('jar_world_save '+slot)
        return decode(directory / (slot+'.0.jrw'))['nodes']

    start = (*point[:2], point[2]+16)
    fixture('rigid_drop', [node(start)])
    time.sleep(3)
    dropped = save('rigid_floor')[0]
    assert start[2]-128 < dropped[9] < start[2]-10, dropped
    assert math.dist(dropped[19:22], (0, 0, 0)) < 1, dropped
    status = rcon('jar_physics_status')
    match = re.search(r'Box3D active; (\d+) triangles; (\d+) fixed steps', status)
    assert match and int(match[1]) > 100 and int(match[2]) > 300, status
    assert 'world cleared' in rcon('jar_world_clear')
    # A splinter legitimately rolls off a second splinter. Use a flat-topped
    # retail crate on known clear geometry to exercise actual stacked contact.
    from mp_network_smoke import SECTION_MAP
    rcon('map '+SECTION_MAP)
    assert 'Saved stack_seed revision 1' in rcon('jar_world_save stack_seed')
    seed = (directory/'stack_seed.0.jrw').read_bytes()
    point = (0., 0., 192.)
    name = 'models/map_objects/nar_shaddar/crate_xplode.md3'
    ident = int(re.search(r'\{(\d+),1,\d+,"'+re.escape(name)+'"', catalog)[1])
    for archive in sorted((game_data/'base').glob('assets*.pk3')):
        with zipfile.ZipFile(archive) as z:
            if name in z.namelist():
                model = z.read(name)
                bounds = struct.unpack_from('<6f', model, struct.unpack_from('<I', model, 92)[0])
                break
    else:
        raise AssertionError(name)
    base = (*point[:2], point[2]+20)
    fixture('rigid_stack', [node(base, frozen=1), node((*base[:2], base[2]+bounds[5]-bounds[2]+8))])
    time.sleep(4)
    stacked = save('rigid_stacked')
    assert len(stacked)==2 and stacked[0][1]==1 and stacked[1][1]==0
    expected = base[2]+bounds[5]-bounds[2]
    assert abs(stacked[1][9]-expected) < 1 and math.dist(stacked[1][19:22], (0, 0, 0)) < 1, stacked
    assert 'world cleared' in rcon('jar_world_clear')
    fixture('rigid_spin', [node(point, gravity=1, spin=1)])
    time.sleep(2)
    spinning = save('rigid_spinning')[0]
    assert abs(spinning[11]) > 10 and .1 < spinning[24] < 1 and spinning[6]==250, spinning
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded rigid_spinning revision 1' in rcon('jar_world_load rigid_spinning')
    time.sleep(1)
    resumed = save('rigid_resumed')[0]
    assert abs(resumed[11]-spinning[11]) > 10 and resumed[24] > .05 and resumed[3]==1, resumed
    rcon('set jar_mpBox3D 0')
    parked = save('rigid_disabled')[0]
    assert parked[1]==1 and math.dist(parked[19:25], (0,)*6)==0, parked
    assert 'Box3D inactive' in rcon('jar_physics_status')
    rcon('set jar_mpBox3D 1')
    assert 'Box3D active' in rcon('jar_physics_status')
    assert 'world cleared' in rcon('jar_world_clear')
    (mod.parent / 'rigid-result.json').write_text(json.dumps(dict(native_module=True,
        map='mp/ffa3', triangles=int(match[1]), fixed_steps=int(match[2]), protocol=1045,
        real_map_floor=True, real_prop_stacking=True, saved_angular_motion=True,
        safe_disable_reenable=True, drop_start=start, drop_final=dropped[7:10],
        stack_height=stacked[1][9]-base[2], expected_stack_height=expected-base[2],
        spin_before=spinning[22:25], spin_restored=resumed[22:25], graphical_clients=0), indent=2)+'\n', encoding='utf-8')
