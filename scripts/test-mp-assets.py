"""Test known conflicting packages in an isolated base; never move owner mod files."""
import argparse,datetime,hashlib,json,os,subprocess,sys,zipfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--game-data',required=True);p.add_argument('--build',default='build/jar/Release');a=p.parse_args()
root=Path(__file__).resolve().parents[1];run=root/'build'/('mp-asset-fixture-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f'));base=run/'base';base.mkdir(parents=True)
for source in sorted((Path(a.game_data)/'base').glob('assets*.pk3')):os.link(source,base/source.name)
assert (base/'assets0.pk3').is_file() and (base/'assets1.pk3').is_file()
for name in ['japro-assets.pk3','japro-win-x86.pk3']:
 with zipfile.ZipFile(base/name,'w') as z:
  z.writestr('maps/mp/ffa3.bsp',b'Invalid map override: this package must never mount in JA:R.')
  z.writestr('ui/jamp/main.menu',b'foreign menu override')
  z.writestr('autoexec.cfg',b'echo JAPro_OVERRIDE_SHOULD_NOT_EXECUTE\n')
with zipfile.ZipFile(base/'jar-custom-map.pk3','w') as z:z.writestr('jar-isolation-allowed.cfg',b'set jar_asset_probe 42\n')
def hashes():return {p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in base.glob('*.pk3') if not p.name.startswith('assets')}
before=hashes();result=subprocess.run([sys.executable,str(root/'scripts/test-mp-server.py'),'--game-data',str(run),'--build',a.build,'--asset-isolation'],cwd=root,capture_output=True,text=True)
(run/'native-output.txt').write_text(result.stdout+result.stderr,encoding='utf-8');print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr);assert result.returncode==0,result.returncode
assert hashes()==before
native=Path(result.stdout.strip().split('passed: ')[-1]);e=json.loads((native/'asset-isolation-result.json').read_text());assert e['CustomConfigExecuted'] and e['KnownJAProBasePackagesSkipped']
log=(native/'stdout.log').read_text(encoding='utf-8',errors='replace');assert 'JAPro_OVERRIDE_SHOULD_NOT_EXECUTE' not in log
for name in before:
 if name.startswith('japro'):assert 'ignoring unrelated base package '+name in log
(run/'result.json').write_text(json.dumps(dict(NativeRun=str(native),FixturePackagesPreserved=before,RetailFilesLinkedReadOnlyByTest=True,OwnerModFilesModified=False,PrivateLoopback=True,ProcessExited=True),indent=2)+'\n',encoding='utf-8');print('Asset isolation passed:',run/'result.json')
