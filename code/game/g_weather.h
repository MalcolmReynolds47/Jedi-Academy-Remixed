// Portable global weather ownership stays in native saved entity fields.
#ifndef G_WEATHER_H
#define G_WEATHER_H
#include "qcommon/jar_weather_zones.h"
bool G_WeatherPortable(const gentity_t&);
bool G_WeatherRead(const gentity_t&,jar::weather::Settings&);
bool G_WeatherSet(gentity_t&,const jar::weather::Settings&,bool prioritize=true);
bool G_WeatherSpawnPortable(gentity_t*);
bool G_WeatherHasControllers();
int G_WeatherZoneCount(const gentity_t*exclude=nullptr);
jar::weather::Zones G_WeatherZones(float scale);
void G_WeatherRestore();
jar::weather::Frame G_WeatherFrame(float scale);
void CG_JAR_WeatherSync();
void CG_JAR_WeatherClear();
#endif
