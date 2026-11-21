"""Mounted thrusters and rigid world ropes in an isolated native server."""
import json
import math
import re
import struct
import time
import zipfile
from mp_world_smoke import checksum, decode, spawn_points


def endpoint(node, local):
    pitch, yaw, roll = map(math.radians, node[10:13])
    sp, cp, sy, cy, sr, cr = math.sin(pitch), math.cos(pitch), math.sin(yaw), math.cos(yaw), math.sin(roll), math.cos(roll)
    axes = ((cp*cy, cp*sy, -sp), (sr*sp*cy-cr*sy, sr*sp*sy+cr*cy, sr*cp), (cr*sp*cy+sr*sy, cr*sp*sy-sr*cy, cr*cp))
    return tuple(node[7+k]+sum(local[a]*axes[a][k] for a in range(3)) for k in range(3))


def check_attachments(root, mod, game_data, rcon):
    rcon('set jar_worldLoad ""')
    rcon('set jar_worldAutoSave 0')
    assert 'world cleared' in rcon('jar_world_clear')
    rcon('map mp/ffa3')
    assert 'world cleared' in rcon('jar_world_clear')
    directory = mod / 'jar/worlds'
    assert 'Saved attach_seed revision 1' in rcon('jar_world_save attach_seed')
    seed = (directory / 'attach_seed.0.jrw').read_bytes()
    version = decode(directory/'attach_seed.0.jrw')['version']
    assert version >= 6
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
    local = (bounds[3], (bounds[1]+bounds[4])/2, (bounds[2]+bounds[5])/2)

    def node(thruster=0, gravity=0):
        return struct.pack('<6I19fI7f', ident, 0, thruster, gravity, 0, 0, 250., *point,
                           0., 0., 0., *bounds, 0., 0., 0., 0., 0., 0.,
                           1, 450., *local, 0., 0., 1.)

    def fixture(slot, record, ropes=b'', version=3):
        data = bytearray(seed[:84])
        struct.pack_into('<I', data, 0, 0x3057524a+version*0x1000000)
        struct.pack_into('<2I', data, 12, 1, len(ropes)//36)
        data += record+ropes
        data += struct.pack('<I', checksum(data))
        (directory/(slot+'.0.jrw')).write_bytes(data)
        assert 'Loaded '+slot+' revision 1' in rcon('jar_world_load '+slot)

    def save(slot):
        assert 'Saved '+slot+' revision 1' in rcon('jar_world_save '+slot)
        return decode(directory/(slot+'.0.jrw'))

    anchor = (point[0]+local[0], point[1]+local[1], point[2]+local[2]+50)
    rope = struct.pack('<ii7f', 0, -1, *local, *anchor, 50.)
    fixture('attach_rope', node(), rope)
    time.sleep(3)
    saved = save('attach_hanging')
    hung = saved['nodes'][0]
    distance = math.dist(endpoint(hung, local), anchor)
    assert distance < 51 and hung[9] > point[2]-32, (distance, hung)
    assert abs(hung[10])+abs(hung[12]) > 5 and len(saved['ropes'])==1, saved
    assert 'Box3D active' in rcon('jar_physics_status')
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded attach_hanging revision 1' in rcon('jar_world_load attach_hanging')
    time.sleep(1)
    resumed = save('attach_rope_restored')['nodes'][0]
    assert math.dist(endpoint(resumed, local), anchor) < 51
    assert 'world cleared' in rcon('jar_world_clear')
    fixture('attach_thruster', node(thruster=1, gravity=1))
    time.sleep(1)
    moving = save('attach_spinning')['nodes'][0]
    assert moving[25]==1 and moving[26]==450 and moving[27:30]==local and moving[30:33]==(0.,0.,1.), moving
    spin = math.dist(moving[22:25], (0,0,0))
    assert .05 < spin <= 8.01 and math.dist(moving[19:22], (0,0,0)) <= 1500.01, moving
    assert 'world cleared' in rcon('jar_world_clear')
    assert 'Loaded attach_spinning revision 1' in rcon('jar_world_load attach_spinning')
    restored = save('attach_mount_restored')['nodes'][0]
    assert restored[25:33]==moving[25:33] and restored[2]==1
    assert 'world cleared' in rcon('jar_world_clear')
    # Legacy unmounted thrust now pushes down. Measure it in free flight,
    # before a floor impact can produce real spin on the irregular crate shard.
    from mp_network_smoke import SECTION_MAP
    rcon('map '+SECTION_MAP)
    assert 'Saved legacy_air revision 1' in rcon('jar_world_save legacy_air')
    seed = (directory/'legacy_air.0.jrw').read_bytes()
    point = (0., 0., 1024.)
    legacy = bytearray(node(thruster=1, gravity=1)[:100])
    struct.pack_into('<f', legacy, 24, 1500.)
    fixture('attach_legacy', legacy, version=2)
    older = save('attach_legacy_restored')['nodes'][0]
    assert older[25]==0 and older[26]==3000 and math.dist(older[22:25], (0,0,0)) < .001, older
    assert older[21] < -1, older
    assert 'world cleared' in rcon('jar_world_clear')
    (mod.parent/'attachment-result.json').write_text(json.dumps(dict(native_module=True, map='mp/ffa3',
        rigid_world_rope=True, rope_length=50, measured_anchor_distance=distance,
        off_center_rotation=True, mounted_thruster_power=450, mounted_thruster_spin=spin,
        mounted_frame_restored=True, rope_restored=True, legacy_thruster_no_added_torque=True, world_write_version=version,
        graphical_clients=0), indent=2)+'\n', encoding='utf-8')
