// Speech-driven facial accents. GPL-2.0-only.
#ifndef JAR_FACE_H
#define JAR_FACE_H
#include <algorithm>
#include <cmath>

namespace jar {
inline float FaceClamp(float value, float low, float high) {
    return std::isfinite(value) ? std::max(low, std::min(high, value)) : low;
}
struct FacePreset { float jawDegrees, lipSpread, upperLift, browLift; };
inline FacePreset FacialPreset(int preset) {
    switch (preset) {
    case 1: return {8.0f, 0.08f, 0.08f, 0.06f};
    case 2: return {22.0f, 0.25f, 0.25f, 0.18f};
    case 3: return {45.0f, 0.65f, 0.55f, 0.45f};
    default: return {0,0,0,0};
    }
}
// Continuous RMS-like envelope from the same pre-mix samples used by legacy
// lipsync. Keep the old -1/0/1..4 script/animation signal completely separate.
inline float VoiceEnergy(float meanSquare, float peak) {
    if (!std::isfinite(meanSquare) || !std::isfinite(peak) || meanSquare <= 0 || peak <= 0) return 0;
    const float relative = std::sqrt(meanSquare) / std::max(1.0f, peak);
    return FaceClamp((relative - 0.025f) * 2.6f, 0, 1);
}
struct SpeechShape {float spread,asymmetry;};
inline SpeechShape FaceShape(int entity,int time,float energy,float change){
    const float phase=float(time%600000)*.001f,seed=float((unsigned(entity)*2654435761u)%997)*.01f;
    const float spread=.48f*std::sin(phase*13.1f+seed)+.22f*std::sin(phase*23.7f+seed*2)+FaceClamp(change*3,-.3f,.3f);
    return {FaceClamp(spread,-1,1),.4f*std::sin(phase*5.7f+seed)*FaceClamp(energy,0,1)};
}
inline float FaceTarget(float energy, float sensitivity) {
    return FaceClamp(FaceClamp(energy,0,1)*FaceClamp(sensitivity,0.5f,2.0f),0,1);
}
inline float SmoothFace(float previous, float target, int elapsedMs) {
    previous=FaceClamp(previous,0,1); target=FaceClamp(target,0,1);
    const float elapsed=float(std::max(0,std::min(elapsedMs,100)));
    const float response=target>previous ? 35.0f : 110.0f;
    return previous+(target-previous)*(1.0f-std::exp(-elapsed/response));
}
// Model-space axes of the supported humanoid face: +X left, +Z up.
// Rotation is around the actual jaw pivot, never the model/world origin.
inline void FaceMatrix(float out[3][4], int role, const float pivot[3],
                       const float jawPivot[3], float amount, int preset, float shape=0, float asymmetry=0) {
    for(int r=0;r<3;++r) for(int c=0;c<4;++c) out[r][c]=r==c ? 1.0f : 0.0f;
    const auto p=FacialPreset(preset); amount=FaceClamp(amount,0,1);
    shape=FaceClamp(shape,-1,1);asymmetry=FaceClamp(asymmetry,-1,1);
    const float angle=p.jawDegrees*amount*(1-.18f*shape)*0.01745329252f;
    const float s=std::sin(angle), c=std::cos(angle);
    if(role==0) {
        out[1][1]=c; out[1][2]=-s; out[2][1]=s; out[2][2]=c;
        out[1][3]=jawPivot[1]*(1-c)+jawPivot[2]*s;
        out[2][3]=jawPivot[2]*(1-c)-jawPivot[1]*s;
    } else if(role>=1 && role<=4) {
        out[0][3]=(role==1 || role==3 ? 1.0f : -1.0f)*p.lipSpread*amount*(1+.65f*shape)+p.lipSpread*amount*.18f*asymmetry;
        if(role<=2) {
            const float y=pivot[1]-jawPivot[1], z=pivot[2]-jawPivot[2];
            out[1][3]=y*(c-1)-z*s;
            out[2][3]=y*s+z*(c-1);
        } else out[2][3]=p.upperLift*amount;
    } else if(role==5) out[2][3]=p.browLift*amount*(1+.3f*shape);
}
}
// Engine implementation exposed to the game DLL through game_import_t.
float S_JAR_VoiceEnergy(int entity);
#endif
