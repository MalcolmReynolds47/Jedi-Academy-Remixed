// Reconcile after load and each rendered game frame. No native weather clears.
// GPL-2.0-only.
#include "game/g_weather.h"
extern void cgi_R_WorldEffectCommand(const char*);
static std::string weatherSent,weatherZonesSent;
void CG_JAR_WeatherClear(){weatherZonesSent.clear();cgi_R_WorldEffectCommand(jar::weather::Encode(jar::weather::Zones{}).c_str());weatherSent.clear();cgi_R_WorldEffectCommand(jar::weather::Encode(jar::weather::Frame{}).c_str());}
void CG_JAR_WeatherSync(){const auto zones=jar::weather::Encode(G_WeatherZones(gi.cvar("r_weatherScale","1",CVAR_ARCHIVE)->value));if(zones!=weatherZonesSent){cgi_R_WorldEffectCommand(zones.c_str());weatherZonesSent=zones;}const auto frame=G_WeatherFrame(gi.cvar("r_weatherScale","1",CVAR_ARCHIVE)->value);const auto command=jar::weather::Encode(frame);if(command!=weatherSent){cgi_R_WorldEffectCommand(command.c_str());weatherSent=command;}}
