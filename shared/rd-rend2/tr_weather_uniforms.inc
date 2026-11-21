// Share exact zone packing between simulation and draw, with no GL mutation.
static void JAR_WeatherUniforms(UniformDataWriter&writer){
#ifdef REND2_SP
 const auto&z=tr.weatherSystem->sandboxZones;writer.SetUniformInt(UNIFORM_JAR_WEATHER_COUNT,z.count);
 if(z.count)writer.SetUniformVec4(UNIFORM_JAR_WEATHER_ZONES,z.values[0].v,z.count*jar::weather::zoneVectors);
#else
 writer.SetUniformInt(UNIFORM_JAR_WEATHER_COUNT,0);
#endif
}
