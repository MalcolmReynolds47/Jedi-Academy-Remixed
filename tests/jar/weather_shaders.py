"""Compile production weather GLSL and read GPU transform feedback in a hidden SDL context.

No game, asset mounts, saves or user settings are opened. Requires an existing SDL2
DLL and OpenGL 3.3 driver. This is shader evidence, not native gameplay acceptance.
GPL-2.0-only.
"""
import argparse, ctypes as c, json, re, struct
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--sdl',type=Path,required=True);p.add_argument('--output',type=Path);a=p.parse_args()
root=Path(__file__).resolve().parents[2]
# Keep all three declarations in agreement; uniforms fit even the GL 3.3
# minimum vertex uniform budget (1024 components, including native uniforms).
zones_header=(root/'code/qcommon/jar_weather_zones.h').read_text()
assert re.search(r'zoneLimit=32,zoneVectors=5',zones_header)
for name in ('weather.glsl','weatherUpdate.glsl'):
 assert 'u_JarWeatherZones[160]' in (root/'shared/rd-rend2/glsl'/name).read_text()
assert '{ "u_JarWeatherZones", GLSL_VEC4, 160 }' in (root/'shared/rd-rend2/tr_glsl.cpp').read_text()
assert 'char scratchBuffer[4096]' in (root/'shared/rd-rend2/tr_local.h').read_text()
sdl=c.CDLL(str(a.sdl.resolve()))
def api(name,result,*args):
 f=getattr(sdl,name);f.restype=result;f.argtypes=args;return f
init=api('SDL_Init',c.c_int,c.c_uint);error=api('SDL_GetError',c.c_char_p)
attr=api('SDL_GL_SetAttribute',c.c_int,c.c_int,c.c_int)
window=api('SDL_CreateWindow',c.c_void_p,c.c_char_p,c.c_int,c.c_int,c.c_int,c.c_int,c.c_uint)
context=api('SDL_GL_CreateContext',c.c_void_p,c.c_void_p)
proc=api('SDL_GL_GetProcAddress',c.c_void_p,c.c_char_p)
close_context=api('SDL_GL_DeleteContext',None,c.c_void_p);close_window=api('SDL_DestroyWindow',None,c.c_void_p);quit_sdl=api('SDL_Quit',None)
U=c.c_uint;I=c.c_int;F=c.c_float;P=c.c_void_p;S=c.c_ssize_t
def gl(name,result,*args):
 address=proc(name.encode());assert address,name
 return c.WINFUNCTYPE(result,*args)(address)
def floats(values):return (F*len(values))(*values)
win=ctx=None
try:
 assert init(0x20)==0,error()
 for key,value in ((17,3),(18,3),(21,1),(5,0)):assert attr(key,value)==0,error()
 win=window(b'JA:R weather shader test',0,0,64,64,0x2|0x8);assert win,error()
 ctx=context(win);assert ctx,error()
 get_string=gl('glGetString',c.c_char_p,U);get_int=gl('glGetIntegerv',None,U,c.POINTER(I));get_error=gl('glGetError',U)
 create_shader=gl('glCreateShader',U,U);source=gl('glShaderSource',None,U,I,c.POINTER(c.c_char_p),c.POINTER(I));compile_shader=gl('glCompileShader',None,U)
 shader_iv=gl('glGetShaderiv',None,U,U,c.POINTER(I));shader_log=gl('glGetShaderInfoLog',None,U,I,c.POINTER(I),c.c_char_p)
 create_program=gl('glCreateProgram',U);attach=gl('glAttachShader',None,U,U);bind_attr=gl('glBindAttribLocation',None,U,U,c.c_char_p)
 link=gl('glLinkProgram',None,U);program_iv=gl('glGetProgramiv',None,U,U,c.POINTER(I));program_log=gl('glGetProgramInfoLog',None,U,I,c.POINTER(I),c.c_char_p)
 feedback_varyings=gl('glTransformFeedbackVaryings',None,U,I,c.POINTER(c.c_char_p),U)
 use=gl('glUseProgram',None,U);location=gl('glGetUniformLocation',I,U,c.c_char_p)
 uni1i=gl('glUniform1i',None,I,I);uni2f=gl('glUniform2f',None,I,F,F);uni3f=gl('glUniform3f',None,I,F,F,F)
 uni4f=gl('glUniform4f',None,I,F,F,F,F);uni4fv=gl('glUniform4fv',None,I,I,c.POINTER(F));matrix=gl('glUniformMatrix4fv',None,I,I,c.c_ubyte,c.POINTER(F))
 gen_buffers=gl('glGenBuffers',None,I,c.POINTER(U));bind_buffer=gl('glBindBuffer',None,U,U);buffer_data=gl('glBufferData',None,U,S,P,U);buffer_base=gl('glBindBufferBase',None,U,U,U)
 read_buffer=gl('glGetBufferSubData',None,U,S,S,P);gen_vao=gl('glGenVertexArrays',None,I,c.POINTER(U));bind_vao=gl('glBindVertexArray',None,U)
 attrib_pointer=gl('glVertexAttribPointer',None,U,I,U,c.c_ubyte,I,P);enable_attr=gl('glEnableVertexAttribArray',None,U)
 enable=gl('glEnable',None,U);disable=gl('glDisable',None,U);begin=gl('glBeginTransformFeedback',None,U);end=gl('glEndTransformFeedback',None);draw=gl('glDrawArrays',None,U,I,I)
 uniform_indices=gl('glGetUniformIndices',None,U,I,c.POINTER(c.c_char_p),c.POINTER(U));uniform_info=gl('glGetActiveUniformsiv',None,U,I,c.POINTER(U),U,c.POINTER(I))
 block_index=gl('glGetUniformBlockIndex',U,U,c.c_char_p);block_binding=gl('glUniformBlockBinding',None,U,U,U);block_info=gl('glGetActiveUniformBlockiv',None,U,U,U,c.POINTER(I))
 def checked(obj,getter,logger,flag,label):
  ok=I();getter(obj,flag,c.byref(ok));log=c.create_string_buffer(8192);logger(obj,8192,None,log);assert ok.value,(label,log.value.decode())
 def program(filename,stages=None,varyings=()):
  text=(root/'shared/rd-rend2/glsl'/filename).read_text();parts=re.split(r'/\*\[(\w+)\]\*/',text);sections=dict(zip(parts[1::2],parts[2::2]));out=create_program()
  for stage,code in sections.items():
   if stages and stage not in stages:continue
   shader=create_shader({'Vertex':0x8B31,'Fragment':0x8B30,'Geometry':0x8DD9}[stage]);encoded=('#version 330 core\n'+code).encode();strings=(c.c_char_p*1)(encoded);source(shader,1,strings,None);compile_shader(shader);checked(shader,shader_iv,shader_log,0x8B81,filename+stage);attach(out,shader)
  bind_attr(out,0,b'attr_Position');bind_attr(out,1,b'attr_Color')
  if varyings:feedback_varyings(out,len(varyings),(c.c_char_p*len(varyings))(*(v.encode() for v in varyings)),0x8C8C)
  link(out);checked(out,program_iv,program_log,0x8B82,filename+' link');return out
 full=program('weather.glsl')
 update=program('weatherUpdate.glsl',varyings=('var_Position','var_Velocity'))
 exposure=program('weather.glsl',stages=('Vertex',),varyings=('var_Culled',))
 limit=I();get_int(0x8B4A,c.byref(limit))
 report={'GLVersion':get_string(0x1F02).decode(),'Renderer':get_string(0x1F01).decode(),'VertexUniformComponents':limit.value,'HiddenWindow':True,'GameLaunched':False,'tests':[]}
 vao=U();gen_vao(1,c.byref(vao));bind_vao(vao)
 buffers=(U*3)();gen_buffers(3,buffers);input_buffer,output_buffer,scene_buffer=buffers
 # Positions are local to a chunk centered at world X=10000. A 90-degree box
 # includes the point along X and excludes the same distance along Y.
 vertices=floats((0,0,100,0,0,0,20,0,100,0,0,0,0,20,100,0,0,0))
 bind_buffer(0x8892,input_buffer);buffer_data(0x8892,c.sizeof(vertices),vertices,0x88E4)
 for index,offset in ((0,0),(1,12)):attrib_pointer(index,3,0x1406,0,24,P(offset));enable_attr(index)
 def loc(prog,name):
  result=location(prog,name.encode());assert result>=0,name;return result
 def zone(kind=0):return [10000,0,100,kind,0,1,0,8,-1,0,0,32,0,0,1,64,500,0,0,0]
 def zones(prog,values):
  use(prog);uni1i(loc(prog,'u_JarWeatherCount'),len(values))
  if values:uni4fv(loc(prog,'u_JarWeatherZones'),5*len(values),floats(sum(values,[])))
 def feedback(prog,format):
  use(prog);size=struct.calcsize(format);bind_buffer(0x8C8E,output_buffer);buffer_data(0x8C8E,size,None,0x88E9);buffer_base(0x8C8E,0,output_buffer)
  enable(0x8C89);begin(0);draw(0,0,3);end();disable(0x8C89)
  result=c.create_string_buffer(size);read_buffer(0x8C8E,0,size,result);assert get_error()==0,'GL transform feedback error';return struct.unpack(format,result.raw)
 use(update);uni2f(loc(update,'u_MapZExtents'),-1000,1000);uni3f(loc(update,'u_EnvForce'),0,0,0);uni4f(loc(update,'u_RandomOffset'),0,0,0,0)
 uni2f(loc(update,'u_ZoneOffset'),0,0);uni2f(loc(update,'u_JarWeatherCenter'),10000,0);uni1i(loc(update,'u_ChunkParticles'),3)
 block=block_index(update,b'Scene');assert block!=0xffffffff
 size=I();block_info(update,block,0x8A40,c.byref(size));idx=U();uniform_indices(update,1,(c.c_char_p*1)(b'u_deltaTime'),c.byref(idx));assert idx.value!=0xffffffff
 offset=I();uniform_info(update,1,c.byref(idx),0x8A3B,c.byref(offset));scene=bytearray(size.value);struct.pack_into('f',scene,offset.value,50)
 scene_data=(c.c_ubyte*len(scene)).from_buffer_copy(scene);bind_buffer(0x8A11,scene_buffer);buffer_data(0x8A11,len(scene),scene_data,0x88E4);buffer_base(0x8A11,0,scene_buffer);block_binding(update,block,0)
 # Exercise the full uniform capacity. Other zones are valid, far away boxes.
 all_zones=[zone()]
 for n in range(31):z=zone();z[0]=200000+n*100;all_zones.append(z)
 zones(update,all_zones);result=feedback(update,'18f')
 assert abs(result[3]-.035)<.0001 and abs(result[9]-.035)<.0001 and abs(result[15])<.0001,result
 report['tests'].append('32-zone upload and rotated local wind at world coordinates')
 zones(update,[]);result=feedback(update,'18f');assert result[3]==result[9]==result[15]==0,result
 report['tests'].append('removing zones restores zero local force')
 # Overlapping vertical winds can cross more than one map-height in a step.
 for velocity in (5000,-5000):
  extreme=[]
  for _ in range(32):z=zone();z[16]=0;z[18]=velocity;extreme.append(z)
  zones(update,extreme);use(update)
  struct.pack_into('f',scene,offset.value,500);scene_data=(c.c_ubyte*len(scene)).from_buffer_copy(scene);bind_buffer(0x8A11,scene_buffer);buffer_data(0x8A11,len(scene),scene_data,0x88E4)
  result=feedback(update,'18f');assert all(-1000<=result[i]<=1000 for i in (2,8,14)),result
 report['tests'].append('32 overlapping upward/downward winds retain bounded particle heights')
 use(exposure);uni2f(loc(exposure,'u_ZoneOffset'),10000,0);uni4f(loc(exposure,'u_ViewInfo'),1,1,0,6000)
 matrix(loc(exposure,'u_ShadowMvp'),1,0,floats((1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)))
 gen_texture=gl('glGenTextures',None,I,c.POINTER(U));bind_texture=gl('glBindTexture',None,U,U);tex_parameter=gl('glTexParameteri',None,U,U,I);tex_image=gl('glTexImage2D',None,U,I,I,I,I,I,U,U,P)
 tex=U();gen_texture(1,c.byref(tex));bind_texture(0x0DE1,tex);tex_parameter(0x0DE1,0x2801,0x2600);tex_parameter(0x0DE1,0x2800,0x2600);tex_image(0x0DE1,0,0x822E,1,1,0,0x1903,0x1406,floats((1,)))
 uni1i(loc(exposure,'u_ShadowMap'),0)
 zones(exposure,[]);assert feedback(exposure,'3i')==(1,1,1)
 zones(exposure,[zone(2)]);assert feedback(exposure,'3i')==(0,0,1)
 zones(exposure,[zone(2),zone(1)]);assert feedback(exposure,'3i')==(1,1,1)
 zones(exposure,[zone(1),zone(2)]);assert feedback(exposure,'3i')==(0,0,1)
 zones(exposure,[]);assert feedback(exposure,'3i')==(1,1,1)
 report['tests'].append('shelter/exposure, rotated membership, overlap priority and baseline restoration')
 report['tests'].append('full production vertex/geometry/fragment program links')
 assert get_error()==0
 if a.output:a.output.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2))
finally:
 if ctx:close_context(ctx)
 if win:close_window(win)
 quit_sdl()
