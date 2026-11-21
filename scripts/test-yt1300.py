"""YT-1300 native loader, bolts, turret transforms and persisted world fixture.

Runs only its own hidden loopback server in a private home directory. Rendered
appearance, driving and human network acceptance remain separate.
"""
import argparse,csv,json,math,re,secrets,shutil,socket,struct,subprocess,time,zipfile
from pathlib import Path
from mp_world_smoke import checksum,decode
p=argparse.ArgumentParser();p.add_argument('--game-data',type=Path,required=True);p.add_argument('--build',type=Path,default=Path('build/jar/Release'));a=p.parse_args()
root=Path(__file__).resolve().parents[1];build=a.build.resolve();run=root/'build'/('yt1300-native-'+time.strftime('%Y%m%d-%H%M%S'));mod=run/'JediAcademyRemixedMP';mod.mkdir(parents=True)
for path in [build/'jampgamex86_64.dll']+[root/'build'/n for n in ('jar-yt1300.pk3','jar-legacy-npcs.pk3','jar-mp-legacy-droids.pk3')]:shutil.copy2(path,mod/path.name)
rows=[]
for line in (root/'codemp/game/jar_mp_catalog.h').read_text().splitlines():
 if line.startswith(' {'):
  row=next(csv.reader([line.strip()[1:-2]]))
  if len(row)==9 and row[4].lower()=='models/players/yt-1300/model.glm':rows.append(row)
assert len(rows)==2 and {r[1] for r in rows}=={'1','3'}
vehicle=next(r for r in rows if r[1]=='3');prop=next(r for r in rows if r[1]=='1')
with zipfile.ZipFile(root/'build/jar-yt1300.pk3') as z:manifest=json.loads(z.read('jar-yt1300.json'))
sock=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);sock.bind(('127.0.0.1',0));sock.settimeout(.3)
probe=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);probe.bind(('127.0.0.1',0));port=probe.getsockname()[1];probe.close();password=secrets.token_hex(16)
command=[str(build/'jar_server.x86_64.exe')];settings=dict(fs_basepath=str(a.game_data.resolve()),fs_homepath=str(run),fs_game='JediAcademyRemixedMP',dedicated='1',net_ip='127.0.0.1',net_port=str(port),net_enabled='1',sv_master1='',sv_master2='',rconPassword=password,sv_pure='0',sv_maxclients='2',sv_fps='40',g_gametype='0',jar_worldAutoSave='0',jar_worldAutoLoad='0',logfile='2')
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
audit=[]
with (run/'stdout.log').open('wb') as log:
 process=subprocess.Popen(command,cwd=run,stdout=log,stderr=subprocess.STDOUT,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
 try:
  response=b'';until=time.monotonic()+45
  while process.poll() is None and time.monotonic()<until:
   send('getstatus jar-yt1300')
   try:
    response=sock.recvfrom(65535)[0]
    if b'\\mapname\\mp/ffa3' in response:break
   except (socket.timeout,ConnectionResetError):pass
  assert b'\\mapname\\mp/ffa3' in response
  assert 'world cleared' in rcon('jar_world_clear');assert 'Saved yt_seed revision 1' in rcon('jar_world_save yt_seed');directory=mod/'jar/worlds';seed=(directory/'yt_seed.0.jrw').read_bytes()
  data=bytearray(seed[:84]);struct.pack_into('<I',data,0,0x3557524a);struct.pack_into('<2I',data,12,len(rows),0)
  for i,row in enumerate(rows):
   radius,height,bottom=map(float,row[6:9]);point=(4000*i,0,12000)
   data+=struct.pack('<6I19fI7fI',int(row[0]),1,0,0,5000,0,200.,*point,0.,0.,0.,-radius,-radius,bottom,radius,radius,bottom+height,0.,0.,0.,0.,0.,0.,0,3000.,0.,0.,0.,0.,0.,1.,0)
  data+=struct.pack('<I',checksum(data));(directory/'yt.0.jrw').write_bytes(data)
  for slot in ('yt','yt_seen'):
   assert 'Loaded '+slot in rcon('jar_world_load '+slot);time.sleep(1);assert '2 objects' in rcon('jar_world_status')
   actor=rcon('jar_actor_info '+vehicle[0]);assert 'health 5000' in actor and 'model models/players/YT-1300/model.glm' in actor and 'pose 1 frames 17' in actor,actor
   reply=rcon('jar_vehicle_rig '+vehicle[0]);assert 'Vehicle rig copy released' in reply,reply
   bolts={}
   for m in re.finditer(r'Rig (\d+) phase (\d+) (\S+) at ([^\r\n]+) dir ([^\r\n]+)',reply):
    assert m[1]==vehicle[0];bolts[(int(m[2]),m[3])]=tuple(map(float,(m[4]+' '+m[5]).split()))
   assert len(bolts)==24,(len(bolts),reply)
   for row in manifest['attachments']:
    tag=row['name']
    if (0,tag) not in bolts:continue
    bind=row['bind'];point=(-bind[7],bind[3],bind[11]);assert math.dist(bolts[0,tag][:3],point)<.01,(tag,bolts[0,tag],point)
    expected=(-1,0,0) if tag.startswith('*exhaust') else (1,0,0);assert math.dist(bolts[0,tag][3:],expected)<.001,(tag,bolts[0,tag])
    if tag in ('*muzzle1','*muzzle2','*muzzle3','*muzzle4','*turretview_t','*turretview_b'):
     assert math.dist(bolts[1,tag][3:],(0,math.cos(math.pi/6),-.5))<.001,(tag,bolts[1,tag])
    else:assert math.dist(bolts[1,tag],bolts[0,tag])<.001,(tag,bolts[1,tag])
   poses=[]
   for frame in (0,8,16):
    response=rcon('jar_model_info '+prop[0]+' '+str(frame));m=re.search(r'Model \d+ frame \d+ of 17: ([^\r\n]+)',response);assert m,response;poses.append(tuple(map(float,m[1].split())))
   assert poses[0]==poses[2] and poses[1][2]>poses[0][2]+10 and poses[1][:2]==poses[0][:2] and poses[1][3:]==poses[0][3:],poses
   assert 'Saved '+slot+'_seen revision 1' in rcon('jar_world_save '+slot+'_seen');saved=decode(directory/(slot+'_seen.0.jrw'));assert {n[0] for n in saved['nodes']}=={int(r[0]) for r in rows}
   audit.append(dict(slot=slot,actor=actor,bolts=[dict(phase=k[0],tag=k[1],values=v) for k,v in bolts.items()],poses=poses));assert 'world cleared' in rcon('jar_world_clear')
  assert '0 objects' in rcon('jar_world_status')
 finally:
  if process.poll() is None:
   send('rcon '+password+' quit')
   try:process.wait(timeout=8)
   except subprocess.TimeoutExpired:process.terminate();process.wait(timeout=5)
  sock.close()
  for path in run.rglob('*'):
   if path.is_file() and path.suffix in ('.log','.cfg'):path.write_bytes(path.read_bytes().replace(password.encode(),b'<test-password-removed>'))
(run/'result.json').write_text(json.dumps(dict(nativeModule=True,spawnedVehicleAndProp=True,restored=True,cleanup=True,NativeBolts=12,TurretYawPitch=True,LandingGearPoses=True,graphicalClients=0,drivingValidated=False,SPNativeSpawnValidated=False,records=audit),indent=2)+'\n')
print('Native YT-1300 spawn, rig, poses and restore checks passed:',run)
