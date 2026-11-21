// Deterministic filesystem boundary; production validates the native IBI header.
#ifndef JAR_TEST_SCRIPTRUNNER_FILES_H
#define JAR_TEST_SCRIPTRUNNER_FILES_H
static bool scriptMissing=false,scriptBadHeader=false,scriptShortRead=false;
static unsigned scriptOpens=0,scriptCloses=0;
static void ScriptFiles(){
 gi.FS_FOpenFile=[](const char*path,fileHandle_t*out,fsMode_t mode){CHECK(mode==FS_READ&&strstr(path,"scripts/")==path&&strstr(path,".ibi"));if(scriptMissing){*out=0;return -1;}++scriptOpens;*out=17;return 80;};
 gi.FS_Read=[](void*data,int length,fileHandle_t file){CHECK(length==8&&file==17);const unsigned char header[]={'I','B','I',0,0xc3,0xf5,0xc8,0x3f};memcpy(data,header,8);if(scriptBadHeader)static_cast<char*>(data)[0]='X';return scriptShortRead?7:8;};
 gi.FS_FCloseFile=[](fileHandle_t file){CHECK(file==17);++scriptCloses;};
}
#endif
