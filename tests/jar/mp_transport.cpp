// Exercise production transport, including complete messages beyond 64 KiB.
#include "mp_message_support.inc"
#include "../../codemp/qcommon/net_chan.cpp"
#include <memory>
#include <vector>
static std::vector<std::vector<byte>> packets;
cvar_t*Cvar_Get(const char*,const char*,uint32_t,const char*){return &quiet;}
qboolean Sys_StringToAdr(const char*,netadr_t*){return qfalse;}
void Sys_SendPacket(int n,const void*data,const netadr_t*){const byte*p=(const byte*)data;packets.emplace_back(p,p+n);}
static std::vector<byte> output(MAX_MSGLEN+4);
static bool Receive(netchan_t*c,const std::vector<byte>&p,int capacity=MAX_MSGLEN+4){
 assert(p.size()<=output.size());memcpy(output.data(),p.data(),p.size());msg_t m;MSG_InitOOB(&m,output.data(),capacity);m.cursize=int(p.size());
 return Netchan_Process(c,&m)==qtrue;
}
#include "mp_network_probe.inc"
int main(int argc,char**argv){
 showpackets=showdrop=qport=&quiet;quiet.integer=0;
 if(argc==2&&!strcmp(argv[1],"--network-probe"))return NetworkProbe();
 assert(argc==1);
 auto tx=std::make_unique<netchan_t>(),rx=std::make_unique<netchan_t>();netadr_t adr{};adr.type=NA_IP;
 std::vector<byte> payload(MAX_MSGLEN);for(int i=0;i<MAX_MSGLEN;++i)payload[i]=byte((i*73)^(i>>16));
 for(netsrc_t side:{NS_CLIENT,NS_SERVER})for(int length:{0,1,FRAGMENT_SIZE-1,FRAGMENT_SIZE,FRAGMENT_SIZE*2,65535,65536,200000,MAX_MSGLEN}){
  Netchan_Setup(side,tx.get(),&adr,0);Netchan_Setup(side==NS_CLIENT?NS_SERVER:NS_CLIENT,rx.get(),&adr,0);packets.clear();
  Netchan_Transmit(tx.get(),length,payload.data());while(tx->unsentFragments)Netchan_TransmitNextFragment(tx.get());
  for(size_t i=0;i<packets.size();++i){bool complete=Receive(rx.get(),packets[i]);assert(complete==(i+1==packets.size()));}
  const int header=length>=FRAGMENT_SIZE?4:side==NS_CLIENT?6:4;assert(!memcmp(output.data()+header,payload.data(),length));assert(rx->incomingSequence==1);
  for(const auto&p:packets)assert(!Receive(rx.get(),p));
  if(packets.size()>2){Netchan_Setup(rx->sock,rx.get(),&adr,0);assert(!Receive(rx.get(),packets[1]));assert(!Receive(rx.get(),packets[0]));
   for(size_t i=1;i<packets.size();++i)assert(Receive(rx.get(),packets[i])==(i+1==packets.size()));assert(rx->incomingSequence==1);
  }
 }
 for(int bad=0;bad<7;++bad){Netchan_Setup(NS_CLIENT,rx.get(),&adr,0);byte b[1400]{};msg_t m;MSG_InitOOB(&m,b,sizeof b);MSG_WriteLong(&m,int(0x80000001u));
  MSG_WriteLong(&m,bad==0?-1:bad==1?MAX_MSGLEN+1:0);MSG_WriteShort(&m,bad==2?FRAGMENT_SIZE+1:2);MSG_WriteByte(&m,8);MSG_WriteByte(&m,9);
  std::vector<byte> p(b,b+m.cursize);if(bad==3)p.resize(3);if(bad==4)p.resize(8);if(bad==5)p.pop_back();
  assert(!Receive(rx.get(),p,bad==6?5:MAX_MSGLEN+4));assert(rx->incomingSequence==0);
 }
 Netchan_Setup(NS_SERVER,rx.get(),&adr,0);assert(!Receive(rx.get(),std::vector<byte>{1,0,0,0}));assert(rx->incomingSequence==0);
 for(int invalid:{-1,MAX_MSGLEN+1}){bool rejected=false;try{Netchan_Transmit(tx.get(),invalid,payload.data());}catch(const std::runtime_error&){rejected=true;}assert(rejected);}
 memset(loopbacks,0,sizeof loopbacks);netadr_t from{};byte b[1400]{};msg_t m;
 NET_SendLoopPacket(NS_CLIENT,MAX_PACKETLEN+1,payload.data());MSG_InitOOB(&m,b,sizeof b);assert(!NET_GetLoopPacket(NS_SERVER,&from,&m));
 for(int i=0;i<MAX_LOOPBACK+3;++i){b[0]=byte(i);NET_SendLoopPacket(NS_CLIENT,1,b);}
 for(int i=3;i<MAX_LOOPBACK+3;++i){MSG_InitOOB(&m,b,sizeof b);assert(NET_GetLoopPacket(NS_SERVER,&from,&m)&&m.cursize==1&&b[0]==i);}
 assert(!NET_GetLoopPacket(NS_SERVER,&from,&m));NET_SendLoopPacket(NS_CLIENT,10,b);MSG_InitOOB(&m,b,2);assert(!NET_GetLoopPacket(NS_SERVER,&from,&m));
 puts("MP native transport: both directions, 4 MiB payloads, replay, reorder, malformed headers, bounds and loopback passed.");
}
