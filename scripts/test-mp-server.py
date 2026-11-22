"""Isolated loopback-only dedicated-server smoke; never touches an existing server."""
import argparse
import json
import re
from pathlib import Path
import secrets
import shutil
import socket
import subprocess
import tempfile
import time

parser = argparse.ArgumentParser()
parser.add_argument('--game-data', required=True)
parser.add_argument('--build', default='build/jar/Release')
parser.add_argument('--map', choices=('gm_construct', 'gm_flatgrass'), help='Host a local converted map with its packaged media')
parser.add_argument('--asset-isolation', action='store_true', help='Validate the deliberate JAPro override fixture and unrelated custom pack')
parser.add_argument('--default-paths', action='store_true', help='Use installed executable defaults for fs_basepath and fs_game')
parser.add_argument('--network-clients', action='store_true', help='Exercise real loopback UDP clients without graphics, using the source-built codec')
parser.add_argument('--network-lights', action='store_true', help='Also spawn and undo both portable lights through a UDP client and verify their snapshots')
parser.add_argument('--network-music', action='store_true', help='Activate portable music and verify configstrings across late join and Undo')
parser.add_argument('--network-pure', action='store_true', help='Use isolated copies of retail and packaged MP archives and verify native pure proofs')
parser.add_argument('--network-section', action='store_true', help='Verify mixed portable BSP resources, native speakers and torches, late join and Undo')
parser.add_argument('--network-hazard-sections', action='store_true', help='Verify portable stock hazard sections with multiple inline models')
parser.add_argument('--network-city-section', action='store_true', help='Verify Abandoned City and its native breakable brushes alongside Vjun')
parser.add_argument('--network-sentinel-section', action='store_true', help='Verify portable Vjun Sentinel and its stock children')
parser.add_argument('--network-hoth-section', action='store_true', help='Verify portable Hoth Canyon and its stock wind emitters')
parser.add_argument('--network-rift-section', action='store_true', help='Verify portable Rift Sanctuary and its pickups')
parser.add_argument('--network-shaft-section', action='store_true', help='Verify portable Imperial Shaft lifts and scenery')
parser.add_argument('--network-korriban-section', action='store_true', help='Verify portable Korriban FFA and its stock children')
parser.add_argument('--network-yavin-section', action='store_true', help='Verify Yavin Training Grounds brushes, scenery and dust')
parser.add_argument('--network-movement', action='store_true', help='Verify gravity and friction marker activation over the native player-state wire')
parser.add_argument('--network-space', action='store_true', help='Verify portable space exposure and gravity recovery over the native player-state wire')
parser.add_argument('--network-directional', action='store_true', help='Verify forward and reverse crossings from native client movement')
parser.add_argument('--network-searchlight', action='store_true', help='Verify searchlight occlusion, private detection, and Undo on the native wire')
parser.add_argument('--network-portals', action='store_true', help='Verify portable camera panel state and Undo on the native wire')
parser.add_argument('--network-angular', action='store_true', help='Verify portable angular movers on the native wire')
parser.add_argument('--network-crowd', action='store_true', help='Join a running 96-NPC fight with simulated props and compare client snapshots for 30 simulated seconds')
parser.add_argument('--network-console-npcs', action='store_true', help='Verify native console NPC admission, overlap, Undo and cleanup')
parser.add_argument('--wire-probe', type=Path, help='Path to jar_mp_transport_tests.exe for --network-clients')
parser.add_argument('--vehicle-motion', action='store_true', help='Qualify JRW6 native vehicle momentum save/restore')
parser.add_argument('--autosave', action='store_true', help='Qualify automatic world saves, map restore, journal recovery and orderly quit')
parser.add_argument('--joints', action='store_true', help='Qualify Weld/Hinge native and rigid graphs, immutable frames and saves')
parser.add_argument('--aliases', action='store_true', help='Spawn and restore every map NPC/vehicle alias in bounded native batches')
parser.add_argument('--curated-catalog', action='store_true', help='Qualify SP curated props, NPCs and vehicles through the native world loader')
parser.add_argument('--vehicle-variants', action='store_true', help='Qualify all 21 native vehicle definitions and seven stock colors together')
parser.add_argument('--skeletal', action='store_true', help='Qualify all retail skeletal props, native posed bounds, instances and saves')
parser.add_argument('--poses', action='store_true', help='Qualify retail prop poses, collision bounds and saved frames')
parser.add_argument('--attachments', action='store_true', help='Qualify mounted thrusters and anchored rigid ropes on a retail map')
parser.add_argument('--rigid', action='store_true', help='Qualify retail-map rigid floor/stack, spin persistence and solver lifecycle')
parser.add_argument('--capacity', action='store_true', help='Verify prop exhaustion, actor admission above 512, and large native saves')
parser.add_argument('--world', action='store_true', help='Exercise native world save/load and map-start restore before connecting bots')
parser.add_argument('--vehicles', action='store_true', help='Also qualify frozen native vehicle families in the isolated world test')
parser.add_argument('--forces', action='store_true', help='Qualify native actor/vehicle rope and thruster movement in the isolated world')
args = parser.parse_args()
section_scenarios = (args.network_section, args.network_hazard_sections, args.network_city_section,
                     args.network_sentinel_section, args.network_hoth_section, args.network_rift_section,
                     args.network_shaft_section, args.network_korriban_section, args.network_yavin_section)
interactive_scenarios = (args.network_directional, args.network_searchlight, args.network_portals,
                         args.network_console_npcs, args.network_space, args.network_crowd)
section_interaction = (args.network_sentinel_section or args.network_shaft_section or args.network_rift_section or
                       args.network_korriban_section or args.network_yavin_section or any(interactive_scenarios))
fixture_count = sum(section_scenarios) + sum(interactive_scenarios) + args.network_angular
section_fixture = fixture_count > 0
if args.network_lights or args.network_music or args.network_pure or args.network_movement or section_fixture:
    args.network_clients = True
if fixture_count > 1:
    parser.error('Run each open fixture scenario separately')
if section_fixture and (args.network_music or args.network_lights):
    parser.error('Portable BSP scenarios use a separate open fixture map from the retail-map wire checks')
if section_fixture and args.curated_catalog:
    parser.error('Curated catalog qualification uses the retail mp/ffa3 map')
if args.network_pure and args.default_paths:
    parser.error('--network-pure uses a copied archive layout, so it cannot test installed executable defaults')
if args.map and (section_fixture or args.network_lights or args.network_music or args.curated_catalog):
    parser.error('Use converted maps with the basic network smoke')
root = Path(__file__).resolve().parents[1]
build = (root / args.build).resolve()
protocol_match = re.search(r'^#define\s+PROTOCOL_VERSION\s+(\d+)\b', (root/'codemp/qcommon/qcommon.h').read_text(), re.M)
assert protocol_match, 'Missing MP protocol declaration'
protocol = int(protocol_match[1])
wire_probe = args.wire_probe or (build / 'jar_mp_transport_tests.exe')
if args.default_paths and not args.wire_probe:
    wire_probe = root / 'build/jar/Release/jar_mp_transport_tests.exe'
run = root / 'build' / ('mp-smoke-' + time.strftime('%Y%m%d-%H%M%S-') + secrets.token_hex(3))
mod = run / 'JediAcademyRemixedMP'
mod.mkdir(parents=True)
map_name = args.map or 'mp/ffa3'
if section_fixture:
    from mp_network_smoke import SECTION_MAP, write_section_map, write_section_map_pack
    if not args.network_pure:
        write_section_map(mod)
    map_name = SECTION_MAP
if (args.world or args.vehicles or args.forces or args.rigid or args.attachments) and not section_fixture:
    from mp_network_smoke import write_section_map
    write_section_map(mod)
server_game_data = Path(args.game_data).resolve()
client_game_data = None
pure_layout = None
if args.network_pure:
    if build.name != 'JediAcademyRemixedMP' or not wire_probe.is_file():
        parser.error('--network-pure needs --build <packaged JediAcademyRemixedMP directory> and --wire-probe <jar_mp_transport_tests.exe>')
    pure_layout = tempfile.TemporaryDirectory(prefix='pure-', dir=run)
    layout = Path(pure_layout.name)
    client_game_data = layout / 'client'
    server_copy = layout / 'server'
    stock_packs = [server_game_data / 'base' / f'assets{i}.pk3' for i in range(4)]
    mod_packs = [build / name for name in ('jar-effects.pk3', 'jar-interface.pk3', 'jar-legacy-npcs.pk3',
                                          'jar-mp-legacy-droids.pk3', 'jar-sandbox-icons.pk3', 'jar-yt1300.pk3')]
    if (build / 'jar-mp-player-models.pk3').is_file():
        mod_packs.append(build / 'jar-mp-player-models.pk3')
    if args.map:
        mod_packs.append(build / 'jar-gmod-maps.pk3')
    if section_fixture:
        fixture_pack = layout / 'jar-wire-section.pk3'
        write_section_map_pack(fixture_pack)
        mod_packs.append(fixture_pack)
    for copied_root in (server_copy, client_game_data):
        for folder, packs in (('base', stock_packs), (build.name, mod_packs)):
            target = copied_root / folder
            target.mkdir(parents=True)
            for pack in packs:
                shutil.copy2(pack, target)
    shutil.copy2(build / 'jampgamex86_64.dll', server_copy / build.name)
    server_game_data = server_copy
else:
    shutil.copy2(build / 'jampgamex86_64.dll', mod)
# Use matching generated assets when testing a source build; installed runs use fs_basepath.
if build == (root/'build/jar/Release').resolve():
    for pack in ('jar-legacy-npcs.pk3','jar-mp-legacy-droids.pk3','jar-yt1300.pk3'):
        shutil.copy2(root/'build'/pack,mod/pack)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('127.0.0.1', 0))
sock.settimeout(.35)
probe = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
probe.bind(('127.0.0.1', 0))
port = probe.getsockname()[1]
probe.close()
password = secrets.token_hex(16)
command = [str(build / 'jar_server.x86_64.exe')]
settings = dict(fs_basepath=str(server_game_data), fs_homepath=str(run),
                fs_game='JediAcademyRemixedMP', dedicated='1', net_ip='127.0.0.1',
                net_port=str(port), net_enabled='1', sv_master1='', sv_master2='',
                rconPassword=password, logfile='2', sv_pure=str(int(args.network_pure)), sv_maxclients='4',
                sv_fps='40', g_gametype='0', fraglimit='0', timelimit='0', bot_enable=str(int(not section_fixture and not args.map)))
if args.network_clients:
    # The real first-connect profile menu normally leaves players spectating.
    # The wire-only probe has no UI to dismiss it, so use native auto-join.
    settings['g_teamAutoJoin'] = '1'
if section_interaction:
    # These private fixtures drive stock cheat teleports to precise test
    # positions. Test mover and pickup timing independently of chat flooding.
    settings['sv_floodProtect'] = '0'
if args.default_paths:
    assert build.name == 'JediAcademyRemixedMP' and build.parent == Path(args.game_data).resolve()
    del settings['fs_basepath']
    del settings['fs_game']
for key, value in settings.items():
    command += ['+set', key, value]
# The Construct wire probes use the stock positioning command to reach the furniture room.
command += ['+devmap' if args.map == 'gm_construct' or section_interaction else '+map', map_name]
map_response = f'\\mapname\\{map_name}'.encode('ascii')
response = b''
shutdown_check = None
def send(text):
    sock.sendto(b'\xff\xff\xff\xff' + text.encode() + b'\n', ('127.0.0.1', port))

def rcon(text):
    # Drain old replies and respect the engine's RCON rate limit.
    while True:
        try: sock.recvfrom(65535)
        except socket.timeout: break
    time.sleep(.6)
    send(f'rcon {password} {text}')
    result = b''
    deadline = time.monotonic() + 8
    while time.monotonic() < deadline:
        try: result += sock.recvfrom(65535)[0]
        except socket.timeout:
            if result: break
    with (run / 'rcon.txt').open('ab') as capture:
        capture.write(text.encode() + b'\n' + result + b'\n')
    assert result, f'No server reply to {text}'
    return result.decode('utf-8', errors='replace')
with (run / 'stdout.log').open('wb') as log:
    process = subprocess.Popen(command, cwd=run, stdout=log, stderr=subprocess.STDOUT,
                               creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    try:
        deadline = time.monotonic() + 45
        while time.monotonic() < deadline and process.poll() is None:
            send('getstatus jar-smoke')
            try:
                response, _ = sock.recvfrom(65535)
                if b'statusResponse' in response and map_response in response:
                    break
            except (socket.timeout, ConnectionResetError):
                pass
            time.sleep(.15)
        assert map_response in response, 'Dedicated server did not answer with the loaded map'
        assert f'\\protocol\\{protocol}\\'.encode('ascii') in response, 'Wrong multiplayer protocol'
        assert b'\\jar_sandbox\\1' in response, 'Sandbox game module not active'
        feature = re.search(r'Cvar_Set\("jar_mpVersion","(\d+)"\)', (root/'codemp/game/g_jar_sandbox.c').read_text())
        assert feature and ('"'+feature[1]+'"') in re.sub(r'\^[0-9]', '', rcon('jar_mpVersion')), 'Wrong context feature version'
        if args.default_paths:
            assert str(Path(args.game_data).resolve()) in rcon('fs_basepath')
            assert 'JediAcademyRemixedMP' in rcon('fs_game')
        if args.asset_isolation:
            search = rcon('path').lower().replace('\\', '/')
            assert 'base/japro-assets.pk3 (' not in search and 'base/japro-win-x86.pk3 (' not in search
            assert 'base/jar-custom-map.pk3 (' in search
            rcon('exec jar-isolation-allowed.cfg')
            assert '"42"' in re.sub(r'\^[0-9]', '', rcon('jar_asset_probe'))
            (run / 'asset-isolation-result.json').write_text(json.dumps(dict(KnownJAProBasePackagesSkipped=True,UnrelatedCustomPakLoaded=True,CustomConfigExecuted=True,GraphicalUIValidated=False),indent=2)+'\n')
        if args.map == 'gm_construct':
            from struct import unpack_from
            from zipfile import ZipFile
            maps = (build if args.network_pure else server_game_data / 'JediAcademyRemixedMP') / 'jar-gmod-maps.pk3'
            with ZipFile(maps) as archive:
                bsp = archive.read('maps/gm_construct.bsp')
            model_offset = unpack_from('<i', bsp, 8 + 7 * 8)[0]
            authored_bottom = unpack_from('<3f', bsp, model_offset)[2]
            # The engine pads collision model bounds by one unit.
            bottom = re.search(r'"(-?[0-9.]+)"', re.sub(r'\^[0-9]', '', rcon('sv_jarMapBottom')))
            assert bottom and abs(float(bottom[1]) - (authored_bottom - 1)) < .01
            assert rcon('entitylist').count('jar_construct_furniture') == 4
            assert 'Box3D active' in rcon('jar_physics_status')
            assert '0 objects, 0 attachments' in rcon('jar_world_save construct_baseline')
            assert 'map-authored entities preserved' in rcon('jar_world_clear')
            assert '0 objects, 0 attachments' in rcon('jar_world_load construct_baseline')
            assert rcon('entitylist').count('jar_construct_furniture') == 4
            from mp_world_smoke import check_construct_overlap
            check_construct_overlap(run, mod, rcon)
        if args.network_clients:
            from mp_network_smoke import check_network_clients
            lights = None
            music = None
            section = None
            physgun = None
            if args.network_lights or args.network_music or section_fixture:
                catalog = (root/'codemp/game/jar_mp_catalog.h').read_text()

                def controller_id(kind, tab=3):
                    match = re.search(rf'\{{(\d+),5,{tab},"{re.escape(kind)}",', catalog)
                    assert match, f'Missing portable controller catalog entry: {kind}'
                    return int(match[1])

            if args.network_lights:
                lights = {kind: controller_id(kind) for kind in ('light', 'misc_dlight')}
            if args.network_music:
                music = controller_id('target_play_music')
            if args.network_section:
                from mp_network_smoke import IMPERIAL_RESOURCE, RANCOR_RESOURCE, VJUN_RESOURCE
                section = {IMPERIAL_RESOURCE: controller_id('Imperial Control Room', 5),
                           VJUN_RESOURCE: controller_id('Vjun Fuel Processing', 5),
                           RANCOR_RESOURCE: controller_id('Rancor Pit', 5)}
            if args.network_hazard_sections:
                from mp_network_smoke import BESPIN_RESOURCE, GENERATOR_RESOURCE, RANCOR_RESOURCE, TASPIR_RESOURCE
                section = {GENERATOR_RESOURCE: controller_id('Generator Room', 5),
                           TASPIR_RESOURCE: controller_id('Taspir Landing', 5),
                           RANCOR_RESOURCE: controller_id('Rancor Pit', 5),
                           BESPIN_RESOURCE: controller_id('Bespin Courtyard', 5)}
            if args.network_city_section:
                from mp_network_smoke import CITY_RESOURCE, VJUN_RESOURCE
                section = {CITY_RESOURCE: controller_id('Abandoned City', 5),
                           VJUN_RESOURCE: controller_id('Vjun Fuel Processing', 5)}
            if args.network_sentinel_section:
                from mp_network_smoke import SENTINEL_RESOURCE
                section = {SENTINEL_RESOURCE: controller_id('Vjun Sentinel', 5)}
            if args.network_hoth_section:
                from mp_network_smoke import HOTH_RESOURCE
                section = {HOTH_RESOURCE: controller_id('Hoth Canyon', 5)}
            if args.network_rift_section:
                from mp_network_smoke import RIFT_RESOURCE
                section = {RIFT_RESOURCE: controller_id('Rift Sanctuary', 5)}
            if args.network_shaft_section:
                from mp_network_smoke import SHAFT_RESOURCE
                section = {SHAFT_RESOURCE: controller_id('Imperial Shaft', 5)}
            if args.network_korriban_section:
                from mp_network_smoke import KORRIBAN_RESOURCE
                section = {KORRIBAN_RESOURCE: controller_id('Korriban FFA', 5)}
            if args.network_yavin_section:
                from mp_network_smoke import YAVIN_RESOURCE
                section = {YAVIN_RESOURCE: controller_id('Yavin Training Grounds', 5)}
            if args.map == 'gm_construct':
                catalog = (root/'codemp/game/jar_mp_catalog.h').read_text()
                physgun = int(re.search(r'\{(\d+),4,2,"weapon_physgun",', catalog)[1])
            check_network_clients(port, wire_probe.resolve(), run, protocol, lights, music, client_game_data, section,
                                  map_name=map_name, physgun_id=physgun,
                                  section_save=(rcon, mod / 'jar/worlds') if section_interaction else None,
                                  movement_markers=args.network_movement, directional_trigger=args.network_directional,
                                  searchlight=args.network_searchlight, portals=args.network_portals,
                                  angular=args.network_angular, console_npcs=args.network_console_npcs,
                                  space_volume=args.network_space, crowd_session=args.network_crowd)
            if args.network_sentinel_section:
                from mp_world_smoke import check_sentinel_journal
                check_sentinel_journal(run, mod, rcon)
            if args.network_yavin_section:
                from mp_world_smoke import check_yavin_journal
                check_yavin_journal(run, mod, rcon)
            if args.network_shaft_section or args.network_rift_section or args.network_korriban_section:
                from mp_world_smoke import check_section_timers
                name = 'shaft' if args.network_shaft_section else 'rift' if args.network_rift_section else 'korriban'
                check_section_timers(run, mod, rcon, name)
        else:
            # Lightweight version check for runs without the source-built codec.
            # Uppercase CONNECT skips the legacy lowercase compression prefix.
            while True:
                try: sock.recvfrom(65535)
                except socket.timeout: break
            send(f'CONNECT "\\protocol\\{protocol - 1}\\name\\old-protocol-test"')
            mismatch, _ = sock.recvfrom(65535)
            (run / 'protocol-rejection.txt').write_bytes(mismatch)
            assert f'Server uses protocol version {protocol} (yours is {protocol - 1})'.encode('ascii') in mismatch, mismatch

        if args.world or args.vehicles or args.forces:
            from mp_world_smoke import check_world
            check_world(root, mod, Path(args.game_data), rcon, vehicles=args.vehicles, forces=args.forces)
        if args.capacity:
            from mp_capacity_smoke import check_capacity
            check_capacity(root, mod, Path(args.game_data), rcon)
        if args.rigid:
            from mp_rigid_smoke import check_rigid
            check_rigid(root, mod, Path(args.game_data), rcon)
        if args.attachments:
            from mp_attachment_smoke import check_attachments
            check_attachments(root, mod, Path(args.game_data), rcon)
        if args.poses:
            from mp_pose_smoke import check_poses
            check_poses(root, mod, Path(args.game_data), rcon)
        if args.skeletal:
            from mp_skeletal_smoke import check_skeletal
            check_skeletal(root, mod, Path(args.game_data), rcon)
        if args.joints:
            from mp_joint_smoke import check_joints
            check_joints(root, mod, Path(args.game_data), rcon)
        if args.aliases:
            from mp_alias_smoke import check_aliases
            check_aliases(root, mod, Path(args.game_data), rcon)
        if args.curated_catalog:
            from mp_curated_catalog_smoke import check_curated_catalog
            check_curated_catalog(root, mod, Path(args.game_data), rcon)
        if args.vehicle_variants:
            from mp_vehicle_variant_smoke import check_vehicle_variants
            check_vehicle_variants(root, mod, Path(args.game_data), rcon)
        if args.vehicle_motion:
            from mp_vehicle_motion_smoke import check_vehicle_motion
            check_vehicle_motion(root, mod, Path(args.game_data), rcon)
        if args.autosave:
            from mp_autosave_smoke import check_autosave, latest
            auto_slot = check_autosave(root, mod, Path(args.game_data), rcon)
        if not section_fixture and not args.map:
            send(f'rcon {password} status')
            time.sleep(.3)
            # Bots execute the native client connection/spawn path without a graphical client.
            send(f'rcon {password} addbot Alora 1')
            time.sleep(1)
            send(f'rcon {password} addbot Cultist 1')
            time.sleep(1)
            # Drain prior RCON replies, then obtain a fresh status.
            while True:
                try:
                    data, _ = sock.recvfrom(65535)
                    with (run / 'rcon.txt').open('ab') as capture: capture.write(data + b'\n')
                except socket.timeout: break
            send('getstatus jar-smoke-bots')
            response, _ = sock.recvfrom(65535)
            assert b'alora' in response.lower() and b'cultist' in response.lower(), 'Two native bot clients did not connect'
        (run / 'status.txt').write_bytes(response)
        (run / 'result.json').write_text(json.dumps(dict(protocol=protocol, map=map_name,
            capacity=args.capacity, sandbox=True, bot_clients=0 if section_fixture or args.map else 2, graphical_clients=0, loopback_only=True, default_install_paths=args.default_paths, pure_server=args.network_pure, vehicle_motion=args.vehicle_motion, map_spawner_aliases=args.aliases, vehicle_variants=args.vehicle_variants,
            world_persistence=args.world or args.vehicles or args.forces or args.autosave or args.rigid or args.attachments or args.poses or args.skeletal or args.joints or args.aliases or args.curated_catalog or args.vehicle_variants, curated_catalog=args.curated_catalog, frozen_vehicle_families=args.vehicles, native_forces=args.forces, automatic_world_persistence=args.autosave, rigid_physics=args.rigid, mounted_attachments=args.attachments, decorative_poses=args.poses, skeletal_props=args.skeletal, weld_hinge=args.joints, old_protocol_rejected=True, udp_clients=2 if args.network_clients else 0), indent=2))
        if args.autosave:
            generation = latest(mod / 'jar/worlds', auto_slot)[1]['generation']
            assert 'world cleared' in rcon('jar_world_clear')
            shutdown_check = (auto_slot, generation)
        print(f'Dedicated map, module and protocol passed: {run}' if section_fixture or args.map else
              f'Dedicated map, module, protocol and two bot clients passed: {run}')
    finally:
        if process.poll() is None:
            send(f'rcon {password} quit')
            try: process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                # Only this test-owned child, never a separately running user process.
                process.terminate()
                process.wait(timeout=5)
        sock.close()
        if pure_layout:
            pure_layout.cleanup()
        # The password is disposable; omit it from durable logs/configuration.
        for path in run.rglob('*'):
            if path.is_file() and path.suffix in ('.log', '.cfg'):
                contents = path.read_bytes()
                if password.encode() in contents:
                    path.write_bytes(contents.replace(password.encode(), b'<test-password-removed>'))

# Assert after the owned native process exits: G_ShutdownGame must write the
# intentional clear before freeing fake clients. No timer or manual save does it.
if shutdown_check:
    slot, generation = shutdown_check
    saved = latest(mod / 'jar/worlds', slot)[1]
    assert saved['generation'] > generation and not saved['nodes'] and not saved['ropes'], saved
    evidence = run / 'autosave-result.json'
    result = json.loads(evidence.read_text(encoding='utf-8'))
    result.update(orderly_quit_saved_clear=True, quit_generation=saved['generation'])
    evidence.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print('Native orderly shutdown autosave passed:', evidence)
