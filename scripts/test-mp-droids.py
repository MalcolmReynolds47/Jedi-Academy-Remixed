"""Original droid actors through an isolated native MP server and JRW restore.

No graphical client or owner's server/profile is used. This qualifies loading,
teams, model poses and lifecycle; it does not claim rendered/combat acceptance.
"""
import argparse
import csv
import json
import math
from pathlib import Path
import re
import secrets
import shutil
import socket
import struct
import subprocess
import time
import zipfile
from mp_world_smoke import checksum
from mp_station_smoke import check_stations
from mp_usable_smoke import check_usable
from mp_breakable_smoke import check_breakables

p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);p.add_argument('--build',type=Path,default=Path('build/jar/Release'));a=p.parse_args()
root=Path(__file__).resolve().parents[1];build=a.build.resolve()
run=root/'build'/('mp-droids-'+time.strftime('%Y%m%d-%H%M%S'));mod=run/'JediAcademyRemixedMP';mod.mkdir(parents=True)
for path in (build/'jampgamex86_64.dll',root/'build/jar-mp-legacy-droids.pk3',root/'build/jar-legacy-npcs.pk3'):shutil.copy2(path,mod/path.name)
catalog={}
for line in (root/'codemp/game/jar_mp_catalog.h').read_text().splitlines():
 if line.startswith(' {'):
  row=next(csv.reader([line.strip()[1:-2]]))
  if len(row)==9 and row[3] in ('mouse','remote_sp','remote','seeker') and int(row[1])==2:catalog[int(row[0])]=row
assert len(catalog)==7
sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);sock.bind(('127.0.0.1',0));sock.settimeout(.3)
probe=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);probe.bind(('127.0.0.1',0));port=probe.getsockname()[1];probe.close();password=secrets.token_hex(16)
command=[str(build/'jar_server.x86_64.exe')]
settings=dict(fs_basepath=str(a.game_data.resolve()),fs_homepath=str(run),fs_game='JediAcademyRemixedMP',dedicated='1',net_ip='127.0.0.1',net_port=str(port),net_enabled='1',sv_master1='',sv_master2='',rconPassword=password,sv_pure='0',sv_maxclients='2',sv_fps='40',g_gametype='0',jar_worldAutoSave='0',jar_worldAutoLoad='0',logfile='2')
for k,v in settings.items():command+=['+set',k,v]
command+=['+map','mp/ffa3']
def send(s):sock.sendto(b'\xff\xff\xff\xff'+s.encode()+b'\n',('127.0.0.1',port))
def rcon(s):
 while True:
  try:sock.recvfrom(65535)
  except socket.timeout:break
 time.sleep(.6);send('rcon '+password+' '+s);result=b'';until=time.monotonic()+8
 while time.monotonic()<until:
  try:result+=sock.recvfrom(65535)[0]
  except socket.timeout:
   if result:break
 with (run/'rcon.txt').open('ab') as f:f.write(s.encode()+b'\n'+result+b'\n')
 assert result,s
 return re.sub(r'\^[0-9]','',result.decode(errors='replace'))

teams={'mouse':(3,3),'remote_sp':(1,2),'remote':(0,0),'seeker':(2,1)}
teamtext=(root/'codemp/game/teams.h').read_text();classlist=re.findall(r'\b(CLASS_\w+)\b',re.sub(r'//[^\n]*|/\*[\s\S]*?\*/','',teamtext));classes={name:i for i,name in enumerate(dict.fromkeys(classlist))}
expectedclass={'mouse':classes['CLASS_MOUSE'],'remote_sp':classes['CLASS_REMOTE'],'remote':classes['CLASS_SEEKER'],'seeker':classes['CLASS_SEEKER']}
original={}
for path in sorted((a.game_data/'base').glob('assets*.pk3')):
 with zipfile.ZipFile(path) as z:
  for source in ('mouse','remote_sp'):
   name='models/players/'+source+'/lower.md3'
   if name not in z.namelist():continue
   data=z.read(name);offset=struct.unpack_from('<i',data,100)[0];points=[]
   for i in range(struct.unpack_from('<i',data,84)[0]):
    nv=struct.unpack_from('<i',data,offset+80)[0];ov=struct.unpack_from('<i',data,offset+100)[0]
    points += [tuple(v/64 for v in struct.unpack_from('<3h',data,offset+ov+8*k)) for k in range(nv)]
    offset+=struct.unpack_from('<i',data,offset+104)[0]
   original[source]=tuple(min(p[k] for p in points) for k in range(3))+tuple(max(p[k] for p in points) for k in range(3))

audit=[]
with (run/'stdout.log').open('wb') as log:
 process=subprocess.Popen(command,cwd=run,stdout=log,stderr=subprocess.STDOUT,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
 try:
  response=b'';until=time.monotonic()+45
  while process.poll() is None and time.monotonic()<until:
   send('getstatus jar-droids')
   try:
    response=sock.recvfrom(65535)[0]
    if b'\\mapname\\mp/ffa3' in response:break
   except (socket.timeout,ConnectionResetError):pass
  assert b'\\mapname\\mp/ffa3' in response
  expected_feature=re.search(r'Cvar_Set\("jar_mpVersion","(\d+)"\)',(root/'codemp/game/g_jar_sandbox.c').read_text())
  assert expected_feature and ('"'+expected_feature[1]+'"') in rcon('jar_mpVersion')
  assert 'world cleared' in rcon('jar_world_clear')
  assert 'Saved droid_seed revision 1' in rcon('jar_world_save droid_seed')
  directory=mod/'jar/worlds';seed=(directory/'droid_seed.0.jrw').read_bytes()
  data=bytearray(seed[:84]);struct.pack_into('<I',data,0,0x3557524a);struct.pack_into('<2I',data,12,7,0)
  for i,(key,row) in enumerate(catalog.items()):
   radius,height,bottom=map(float,row[6:9]);point=(-12000+3000*(i%4),-12000+3000*(i//4),12000)
   data+=struct.pack('<6I19fI7fI',key,1,0,0,173,30 if row[3] in ('remote','seeker') else 0,200.,*point,0.,0.,0.,-radius,-radius,bottom,radius,radius,bottom+height,0.,0.,0.,0.,0.,0.,0,3000.,0.,0.,0.,0.,0.,1.,0)
  data+=struct.pack('<I',checksum(data));(directory/'droids.0.jrw').write_bytes(data)
  for slot in ('droids','droids_seen'):
   assert 'Loaded '+slot in rcon('jar_world_load '+slot);time.sleep(3)
   assert '7 objects' in rcon('jar_world_status')
   for key,row in catalog.items():
    reply=rcon('jar_actor_info '+str(key))
    m=re.search(r'Actor (\d+) entity (\d+) class (\d+) team (-?\d+) enemy (-?\d+) gravity (\d+) health (\d+) ammo (\d+) model (\S+) gla (\S+) pose (\d+) frames (\d+) bounds ([^\r\n]+)',reply)
    assert m,(row,reply)
    name=row[3];bounds=tuple(map(float,m[13].split()))
    assert int(m[3])==expectedclass[name] and (int(m[4]),int(m[5]))==teams[name],(name,m.groups())
    assert int(m[6])==(800 if name=='mouse' else 0) and int(m[7])==173,(name,m.groups())
    assert m[9]==row[4] and m[10]==row[4].removesuffix('.glm') and (m[11],m[12])==('1','1'),m.groups()
    assert math.dist(bounds,original['mouse' if name=='mouse' else 'remote_sp'])<.001,(name,bounds)
    audit.append(dict(slot=slot,id=key,name=name,model=m[9],team=int(m[4]),enemy=int(m[5]),gravity=int(m[6]),bounds=bounds))
   assert 'Saved '+slot+'_seen revision 1' in rcon('jar_world_save '+slot+'_seen')
   saved=(directory/(slot+'_seen.0.jrw')).read_bytes();assert struct.unpack_from('<I',saved)[0]==0x4957524a and len(saved)==88+7*284
   assert checksum(saved[:-4])==struct.unpack_from('<I',saved,len(saved)-4)[0]
   assert {struct.unpack_from('<I',saved,84+284*i)[0] for i in range(7)}==catalog.keys()
   assert 'world cleared' in rcon('jar_world_clear')
  assert '0 objects' in rcon('jar_world_status')
  check_stations(root,mod,a.game_data,rcon)
  check_usable(root,mod,rcon)
  check_breakables(root,mod,rcon)
 finally:
  if process.poll() is None:
   send('rcon '+password+' quit')
   try:process.wait(timeout=8)
   except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=5)
  sock.close()
  for path in run.rglob('*'):
   if path.is_file() and path.suffix in ('.log','.cfg'):path.write_bytes(path.read_bytes().replace(password.encode(),b'<test-password-removed>'))
assert len(audit)==14
(run/'result.json').write_text(json.dumps(dict(nativeModule=True,spawnedCards=7,restoredCards=7,actualGhoul2PosesMatchMD3=True,teamsAndGravityMatchDefinitions=True,cleanup=True,graphicalClients=0,combatValidated=False,records=audit),indent=2)+'\n')
print('Native MP droid spawn/restore/geometry checks passed:',run)
