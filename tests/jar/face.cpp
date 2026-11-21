#include "qcommon/jar_face.h"
#include "test_support.h"
#include <limits>

static bool Near(float a,float b) { return std::fabs(a-b)<0.0001f; }
int main() {
    return RunTests("facial envelope, smoothing and pivot-safe deformation", [] {
        CHECK(jar::VoiceEnergy(0,128)==0);
        CHECK(jar::VoiceEnergy(100,0)==0);
        CHECK(jar::VoiceEnergy(std::numeric_limits<float>::quiet_NaN(),128)==0);
        CHECK(jar::VoiceEnergy(100,128)<jar::VoiceEnergy(400,128));
        CHECK(jar::VoiceEnergy(400,128)<jar::VoiceEnergy(1600,128));
        CHECK(jar::VoiceEnergy(100000,128)==1);
        CHECK(jar::FaceTarget(0.2f,1.5f)>jar::FaceTarget(0.2f,1));
        CHECK(jar::FaceTarget(1,2)==1);
        CHECK(jar::SmoothFace(0,1,0)==0);
        CHECK(jar::SmoothFace(0,1,-100)==0);
        float a=0,b=0;
        for(int i=0;i<10;++i)a=jar::SmoothFace(a,1,10);
        for(int i=0;i<20;++i)b=jar::SmoothFace(b,1,5);
        CHECK(Near(a,b)); CHECK(a>0.9f && a<1);
        for(int i=0;i<100;++i)a=jar::SmoothFace(a,0,10);
        CHECK(a<0.001f);
        const float jaw[]={0.04f,-0.53f,60.78f}, lip[]={0.57f,-3.87f,60.03f};
        float previousOpening=0;
        for(int preset=0;preset<4;++preset) {
            float m[3][4]; jar::FaceMatrix(m,0,jaw,jaw,1,preset);
            // A jaw accent rotates about the jaw, not the 60-unit-high model origin.
            for(int r=0;r<3;++r) {
                float p=m[r][3];for(int c=0;c<3;++c)p+=m[r][c]*jaw[c];
                CHECK(Near(p,jaw[r]));
            }
            float opening=jaw[2]-(m[2][1]*(jaw[1]-3.3f)+m[2][2]*jaw[2]+m[2][3]);
            if(preset)CHECK(opening>previousOpening);
            previousOpening=opening;
            CHECK(Near(m[1][1]*m[2][2]-m[1][2]*m[2][1],1));
            float lower[3][4];jar::FaceMatrix(lower,1,lip,jaw,1,preset);
            CHECK(lower[2][3]<=0);
            for(int role=0;role<6;++role) {
                jar::FaceMatrix(m,role,lip,jaw,0,preset);
                for(int r=0;r<3;++r)for(int c=0;c<4;++c)CHECK(Near(m[r][c],r==c?1.0f:0.0f));
            }
        }
        const auto shape=jar::FaceShape(1,1000,.7f,.1f),same=jar::FaceShape(1,1000,.7f,.1f),other=jar::FaceShape(2,1000,.7f,.1f);
        CHECK(shape.spread==same.spread&&shape.asymmetry==same.asymmetry);
        CHECK(shape.spread!=other.spread);CHECK(jar::FaceShape(1,1100,.7f,.1f).spread!=shape.spread);
        float spread[3][4],rounded[3][4];jar::FaceMatrix(spread,3,lip,jaw,.8f,2,1,.4f);jar::FaceMatrix(rounded,3,lip,jaw,.8f,2,-1,-.4f);
        CHECK(spread[0][3]!=rounded[0][3]);
        for(int i=0;i<10000;++i){const auto v=jar::FaceShape(i%32,i*7,.5f,.1f);CHECK(std::isfinite(v.spread)&&std::fabs(v.spread)<=1&&std::fabs(v.asymmetry)<=1);}
        float stock[3][4];jar::FaceMatrix(stock,0,jaw,jaw,1,999);
        CHECK(Near(stock[1][1],1)&&Near(stock[1][3],0));
    });
}
