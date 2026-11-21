"""Native vehicle catalog/capacity and color persistence in a private test world."""
import csv,json,math,struct,time
from mp_world_smoke import checksum,decode

def check_vehicle_variants(root,mod,game_data,rcon):
 rows=[]
 for line in (root/'codemp/game/jar_mp_catalog.h').read_text().splitlines():
  if not line.startswith(' {'):continue
  row=next(csv.reader([line.strip()[1:-2]]))
  if len(row)==9 and row[1]=='3' and row[2]=='4':rows.append(row)
 assert len(rows)==21
 colors={r[3] for r in rows if r[3] in {'swoop_black','swoop_blue','swoop_gold','swoop_green','swoop_purple','swoop_red','swoop_silver'}}
 assert len(colors)==7
 rcon('set jar_worldLoad ""');rcon('set jar_worldAutoSave 0');assert 'world cleared' in rcon('jar_world_clear')
 rcon('map mp/ffa3');assert 'world cleared' in rcon('jar_world_clear')
 directory=mod/'jar/worlds';assert 'Saved colors_seed revision 1' in rcon('jar_world_save colors_seed')
 seed=(directory/'colors_seed.0.jrw').read_bytes();out=bytearray(seed[:84]);struct.pack_into('<I',out,0,0x3557524a);struct.pack_into('<2I',out,12,len(rows),0)
 for i,row in enumerate(rows):
  key=int(row[0]);radius,height,bottom=map(float,row[6:9]);point=(-12000+4000*(i%5),-12000+4000*(i//5),12000)
  out+=struct.pack('<6I19fI7fI',key,1,0,0,317,0,200.,*point,0.,0.,0.,-radius,-radius,bottom,radius,radius,bottom+height,0.,0.,0.,0.,0.,0.,0,3000.,0.,0.,0.,0.,0.,1.,0)
 out+=struct.pack('<I',checksum(out));(directory/'colors.0.jrw').write_bytes(out)
 assert 'Loaded colors revision 1' in rcon('jar_world_load colors')
 def capture(slot):
  time.sleep(3.5);assert 'Saved '+slot+' revision 1' in rcon('jar_world_save '+slot)
  nodes=decode(directory/(slot+'.0.jrw'))['nodes'];assert len(nodes)==21,(slot,len(nodes))
  for i,(row,node) in enumerate(zip(rows,nodes)):
   assert node[0]==int(row[0]) and node[1]==1 and node[4]==317,(row[3],node)
   assert math.dist(node[7:10],(-12000+4000*(i%5),-12000+4000*(i//5),12000))<.05,(row[3],node)
  return nodes
 first=capture('colors_seen');assert 'world cleared' in rcon('jar_world_clear')
 assert 'Loaded colors_seen revision 1' in rcon('jar_world_load colors_seen');capture('colors_restored')
 # Map initialization discards the registry; automatic restore must reload all 21 definitions.
 rcon('set jar_worldLoad colors_seen');rcon('map mp/ffa3');capture('colors_maprestored')
 rcon('set jar_worldLoad ""');assert 'world cleared' in rcon('jar_world_clear')
 result=dict(native_module=True,vehicles=len(rows),stock_color_variants=sorted(colors),definitions_beyond_previous_limit=True,
             saved=True,restored=True,map_restored=True,cleanup=True,grid_above_retail_map=True,health=317,
             graphical_clients=0,skin_appearance_test=False,vehicle_driving_test=False,
             choices=[dict(id=int(row[0]),name=row[3]) for row in rows])
 (mod.parent/'vehicle-variant-result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
 print('Native vehicle variants: 21 definitions spawned, saved, restored and map-restored; seven stock color choices',flush=True)
