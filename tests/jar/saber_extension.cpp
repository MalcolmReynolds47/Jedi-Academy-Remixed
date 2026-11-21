#include "qcommon/jar_saber_extension.h"
#include "test_support.h"
#include <limits>
#include <cmath>
int main(){return RunTests("production saber ignition duration and protected paths",[]{
    for(float maximum:{1.0f,40.0f,44.0f,60.0f}){
        CHECK(jar::ExtendSaber(0,maximum,250,1,false)==maximum);
        CHECK(std::fabs(jar::ExtendSaber(0,maximum,250,0,false)-maximum*.25f)<.001f);
        for(int mode:{-1,0,2,999})CHECK(jar::ExtendSaber(0,maximum,100,mode,false)==maximum/10);
        CHECK(jar::ExtendSaber(0,maximum,100,1,true)==maximum/10);
        for(int frame:{7,16,33,100})for(int mode:{0,1}){
            float length=0;int elapsed=0;
            while(length<maximum && elapsed<2000){const float before=length;length=jar::ExtendSaber(length,maximum,frame,mode,false);
                CHECK(length>=before && length<=maximum);elapsed+=frame;}
            const int duration=mode==1?250:1000;CHECK(elapsed>=duration && elapsed<=duration+frame);
            CHECK(length==maximum && jar::ExtendSaber(length,maximum,100,mode,false)==maximum);
        }
        CHECK(jar::ExtendSaber(maximum*.5f,maximum,125,1,false)==maximum); // saved partial ignition
        CHECK(jar::ExtendSaber(0,maximum,0,1,false)==0);
        CHECK(jar::ExtendSaber(0,maximum,-10,1,false)==0);
        CHECK(jar::ExtendSaber(0,maximum,99999,1,false)==maximum);
    }
    CHECK(jar::ExtendSaber(7,0,16,1,false)==7);
    CHECK(jar::ExtendSaber(7,std::numeric_limits<float>::infinity(),16,1,false)==7);
});}
