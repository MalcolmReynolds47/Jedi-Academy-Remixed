"""Verify the staged player model pack, saber selection, and RGB userinfo over UDP."""
import argparse
from contextlib import ExitStack
import json
from pathlib import Path
import re
import secrets
import shutil
import socket
import subprocess
import time
from zipfile import ZipFile

from mp_network_smoke import OOB, await_state, open_wire_client, wire_value


def check():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-data', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--media', type=Path, required=True)
    args = parser.parse_args()
    build = args.build.resolve()
    root = Path(__file__).resolve().parents[1]
    protocol = int(re.search(r'^#define\s+PROTOCOL_VERSION\s+(\d+)',
                             (root/'codemp/qcommon/qcommon.h').read_text(), re.M)[1])
    run = root/'build'/f'mp-smoke-{time.strftime("%Y%m%d-%H%M%S")}-{secrets.token_hex(3)}'
    mod = run/'JediAcademyRemixedMP'
    mod.mkdir(parents=True)
    for path in (build/'jampgamex86_64.dll', args.media/'jar-mp-player-models.pk3',
                 args.media/'jar-sandbox-icons.pk3'):
        shutil.copy2(path, mod/path.name)
    with ZipFile(mod/'jar-mp-player-models.pk3') as pak:
        manifest = json.loads(pak.read('jar/player-models.json'))

    with ExitStack() as stack:
        probe = stack.enter_context(socket.socket(socket.AF_INET, socket.SOCK_DGRAM))
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as reserved:
            reserved.bind(('127.0.0.1', 0))
            port = reserved.getsockname()[1]
        probe.settimeout(.3)
        address = ('127.0.0.1', port)
        password = secrets.token_hex(16)
        config = dict(fs_basepath=str(args.game_data.resolve()), fs_homepath=str(run), fs_game=mod.name,
                      net_ip=address[0], net_port=port, dedicated=1, sv_pure=0, bot_enable=0,
                      g_teamAutoJoin=1, d_perPlayerGhoul2=1, sv_maxclients=32, sv_maxOOBRateIP=0,
                      sv_maxOOBRate=0, g_maxConnPerIP=32, sv_zombietime=0,
                      rconPassword=password, sv_master1='', sv_master2='')
        command = [str(build/'jar_server.x86_64.exe')]
        for key, value in config.items():
            command.extend(['+set', key, str(value)])
        log = stack.enter_context((run/'stdout.log').open('wb'))
        server = subprocess.Popen([*command, '+map','mp/ffa3'], cwd=run, stdout=log,
                                  stderr=subprocess.STDOUT, creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        rows = []
        try:
            deadline = time.monotonic()+25
            while time.monotonic() < deadline:
                probe.sendto(OOB+b'getstatus appearance\n', address)
                try:
                    if b'\\mapname\\mp/ffa3' in probe.recv(65535):
                        break
                except (socket.timeout, ConnectionResetError):
                    pass
            else:
                raise AssertionError('Dedicated appearance server did not start')
            for index, (model, record) in enumerate(manifest['adapted'].items()):
                assert max(record['bounds'][:2]) <= 52 and record['bounds'][2] <= 62
                saber = ('desann', 'droid', 'jar_pike', 'sith_sword')[index % 4]
                color = '#' + f'{0x12ab20 + index:06x}'
                setting = (model+'/default', saber, color)
                with ExitStack() as client:
                    sock, codec, state = open_wire_client(client, port, build/'jar_mp_transport_tests.exe',
                                                          run, protocol, f'Appearance{index}', appearance=setting)
                    sock.send(codec.packet('enter'))
                    frame = await_state(sock, codec, 'snapshot')
                    assert int(frame[4]) > 0
                    value = wire_value(codec.request(f'playerinfo {state["slot"]}').split()[1])
                    pairs = value.lstrip('\\').split('\\')
                    info = dict(zip(pairs[::2], pairs[1::2]))
                    assert (info['model'], info['st'], info['c1']) == setting, (setting, info)
                    rows.append(dict(model=model, saber=saber, rgb=color))
                    sock.send(codec.packet('disconnect'))
            (run/'appearance-result.json').write_text(json.dumps(rows, indent=2)+'\n')
            print(f'Native MP player appearance passed for {len(rows)} adapted models, four hilts, and RGB userinfo: {run}')
        finally:
            if server.poll() is None:
                probe.sendto(OOB+f'rcon {password} quit\n'.encode(), address)
                try:
                    server.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    server.terminate()
                    server.wait(timeout=5)


if __name__ == '__main__':
    check()
