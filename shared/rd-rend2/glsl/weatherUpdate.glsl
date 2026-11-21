/*[Vertex]*/

layout(std140) uniform Scene
{
	vec4 u_PrimaryLightOrigin;
	vec3 u_PrimaryLightAmbient;
	int  u_globalFogIndex;
	vec3 u_PrimaryLightColor;
	float u_PrimaryLightRadius;
	float u_frameTime;
	float u_deltaTime;
};

uniform int u_JarWeatherCount;
uniform vec4 u_JarWeatherZones[160];
bool JarWeatherContains(int i,vec3 point){
 int b=i*5;vec3 d=point-u_JarWeatherZones[b].xyz;
 return abs(dot(d,u_JarWeatherZones[b+1].xyz))<=u_JarWeatherZones[b+1].w+0.001 &&
        abs(dot(d,u_JarWeatherZones[b+2].xyz))<=u_JarWeatherZones[b+2].w+0.001 &&
        abs(dot(d,u_JarWeatherZones[b+3].xyz))<=u_JarWeatherZones[b+3].w+0.001;
}
uniform vec2 u_JarWeatherCenter;
uniform vec2 u_MapZExtents;
uniform vec3 u_EnvForce;
uniform vec4 u_RandomOffset;
uniform vec2 u_ZoneOffset[9];
uniform int u_ChunkParticles;

in vec3 attr_Position;
in vec3 attr_Color;

out vec3 var_Position;
out vec3 var_Velocity;

const float CHUNK_EXTENDS = 2000.0;
const float HALF_CHUNK_EXTENDS = CHUNK_EXTENDS * 0.5;
const float THREE_HALF_CHUNK_EXTENDS = 3.0 * HALF_CHUNK_EXTENDS;

vec3 NewParticleZPosition( in vec3 in_position )
{
	vec3 position = in_position;
	position.xy += u_RandomOffset.xy;
	position.z += u_MapZExtents.y - u_MapZExtents.x;

	return position;
}

void main()
{
 int zone = gl_VertexID / u_ChunkParticles;
 vec3 worldPoint=vec3(attr_Position.xy+u_ZoneOffset[zone]*CHUNK_EXTENDS+u_JarWeatherCenter,attr_Position.z);
 vec3 localForce=vec3(0.0);
 for(int i=0;i<u_JarWeatherCount;++i)if(u_JarWeatherZones[i*5].w==0.0&&JarWeatherContains(i,worldPoint))localForce+=u_JarWeatherZones[i*5+4].xyz*0.0007;
	var_Velocity = attr_Color;
	var_Velocity = mix(var_Velocity, u_EnvForce+localForce, u_deltaTime * 0.002);
	var_Position = attr_Position;
	var_Position += var_Velocity * u_deltaTime;

	if (var_Position.z < u_MapZExtents.x)
	{
		var_Position = NewParticleZPosition(var_Position);
		var_Velocity.xy = u_EnvForce.xy+localForce.xy;
	}

 if(var_Position.z<u_MapZExtents.x||var_Position.z>u_MapZExtents.y){float height=u_MapZExtents.y-u_MapZExtents.x;if(height>0.0)var_Position.z=u_MapZExtents.x+mod(var_Position.z-u_MapZExtents.x,height);}
	vec2 zoneOffset = u_ZoneOffset[zone] * CHUNK_EXTENDS;
	vec2 sim_Position = var_Position.xy + zoneOffset;

	if (any(greaterThan(abs(sim_Position).xy, vec2(THREE_HALF_CHUNK_EXTENDS))))
	{
		vec2 signs = sign(sim_Position.xy);
		vec2 absPos = abs(sim_Position.xy) + vec2(THREE_HALF_CHUNK_EXTENDS);
		sim_Position.xy = -signs * (THREE_HALF_CHUNK_EXTENDS - mod(absPos, 3.0 * CHUNK_EXTENDS));

		var_Position.xy = sim_Position - zoneOffset;
	}
}
