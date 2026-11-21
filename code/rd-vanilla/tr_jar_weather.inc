// Independent Sandbox slots; native map commands never clear these. GPL-2.0-only.
static CParticleCloud jarWeatherClouds[3];
static void JAR_WeatherReset(){for(auto&cloud:jarWeatherClouds)cloud.Reset();jarWeather={};}
static void JAR_WeatherApply(const jar::weather::Frame&frame){
 for(int i=0;i<3;++i){const auto&layer=frame.layers[i];if(jar::weather::Same(layer,jarWeather.layers[i]))continue;
  auto&c=jarWeatherClouds[i];if(!layer.count){c.Reset();continue;}
  const auto r=jar::weather::Describe(i,layer);c.Initialize(layer.count,r.texture,i==jar::weather::Rain?3:4);
  c.mGravity=r.gravity;c.mWidth=r.width;c.mHeight=r.height;c.mBlendMode=r.additive?1:0;
  c.mWaterParticles=r.water;c.mOrientWithVelocity=r.oriented;c.mFade=i==jar::weather::Rain?100:10;
  for(int a=0;a<4;++a)c.mColor[a]=r.color[a];
  if(i==jar::weather::Rain)c.mFilterMode=1;
  if(i==jar::weather::Puff){c.mMass.mMin=10;c.mMass.mMax=30;c.mSpawnRange.mMins[2]=-150;c.mSpawnRange.mMaxs[2]=150;}
 }
 jarWeather=frame;
}
