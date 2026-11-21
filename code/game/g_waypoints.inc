// Runtime navigation nodes. Included after the native navigator's definitions.
// Saved entity fields own the configuration; graph handles are rebuilt, never saved.
#include "g_functions.h"
static bool waypointAdmission();
#include "g_waypoint_properties.inc"
#include <algorithm>
#include <map>
#include <string>
#include <vector>
namespace {
struct LiveWaypoint {
 int entity;uint64_t room;float point[3],range;bool enabled,small;std::string name,primary,secondary;
 bool operator==(const LiveWaypoint&o)const{return entity==o.entity&&room==o.room&&VectorCompare(point,o.point)&&range==o.range&&enabled==o.enabled&&small==o.small&&name==o.name&&primary==o.primary&&secondary==o.secondary;}
};
std::vector<LiveWaypoint> liveWaypointSnapshot;
std::map<int,int> liveWaypointHandles;
std::map<int,uint64_t> liveWaypointNodeRooms;
std::map<uint64_t,std::vector<int>> liveWaypointRoomNodes;
bool liveWaypointsDirty=false;
uint64_t waypointRoom(const gentity_t& e){
 if(!e.inuse||!e.model||strcmp(e.model,jar::siegeKorriban::waypointModel))return 0;
 return G_SiegeKorribanRoom(G_SubBSPOwner(&e))?G_SubBSPNamespace(&e):0;
}
bool waypointRead(const gentity_t& e,jar::waypoint::Settings& settings){
 if(G_WaypointRead(e,settings))return true;
 if(!waypointRoom(e)||!e.classname||strcmp(e.classname,"waypoint")||!e.targetname||!e.target)return false;
 for(const auto& record:jar::siegeKorriban::waypoints)
  if(!strcmp(e.targetname,record.name)&&!strcmp(e.target,record.target)){settings={0,true};return true;}
 return false;
}
bool waypointPosition(const gentity_t&e){for(float x:e.currentOrigin)if(!std::isfinite(x)||std::fabs(x)>65535)return false;return true;}
bool waypointTrace(const vec3_t a,const vec3_t b,float radius,float height,trace_t&tr){
 vec3_t low={-radius,-radius,0},high={radius,radius,height};
 gi.trace(&tr,a,low,high,b,ENTITYNUM_NONE,CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP,(EG2_Collision)0,0);
 return !tr.startsolid&&!tr.allsolid&&tr.fraction==1;
}
bool waypointFloor(const vec3_t p,float depth){
 vec3_t end;VectorCopy(p,end);end[2]-=depth;trace_t tr;
 waypointTrace(p,end,1,1,tr);return !tr.startsolid&&!tr.allsolid&&tr.fraction<1&&tr.plane.normal[2]>=.7f;
}
void waypointClearPaths(){
 mPathUsers.clear();mPathUserIndex.fill(NAV::NULL_PATH_USER_INDEX);
 memset(&mEntityAlertList,0,sizeof(mEntityAlertList));mNearestNavSort.clear();
 for(int i=0;i<globals.num_entities;++i){g_entities[i].waypoint=0;g_entities[i].lastWaypoint=0;g_entities[i].noWaypointTime=0;}
 for(int i=0;i<level.numCombatPoints;++i)level.combatPoints[i].waypoint=NAV::GetNearestNode(level.combatPoints[i].origin);
}
// Rebuild bounded nearest-node/edge lookup cells without the map loader's
// assumptions about non-collinear points or at most 59 coincident entries.
void waypointCells(){
 mCells.clear();if(!mGraph.size_nodes())return;
 bool hasWorldNodes=false;
 for(auto it=mGraph.nodes_begin();it!=mGraph.nodes_end();++it){
  if(waypointRoomNode(it.index()))continue;
  hasWorldNodes=true;
  mCells.expand_bounds(it->mPoint[0]-64,it->mPoint[1]-64);mCells.expand_bounds(it->mPoint[0]+64,it->mPoint[1]+64);
 }
 if(!hasWorldNodes)return;
 std::vector<TGraphCells::SSortNode> nearest;nearest.reserve(NAV::NUM_EDGES);
 for(int y=0;y<NAV::NUM_CELLS;++y)for(int x=0;x<NAV::NUM_CELLS;++x){
  float x0,y0,x1,y1;mCells.get_cell_upperleft(x,y,x0,y0);mCells.get_cell_lowerright(x,y,x1,y1);const float cx=(x0+x1)*.5f,cy=(y0+y1)*.5f;
  // Include the entire cell, even on maps whose cells exceed CELL_RANGE.
  // Long explicit edges also remain candidates near either end of the segment.
  const float reach=NAV::CELL_RANGE+.5f*std::hypot(x1-x0,y1-y0);
  auto fill=[&](auto first,auto last,TGraphCells::TCellNodes&cell,auto extent,auto include){
   nearest.clear();cell.clear();for(auto it=first;it!=last;++it){
    if(!include(it.index(),*it))continue;
    const float dx=(*it)[0]-cx,dy=(*it)[1]-cy;const float d=dx*dx+dy*dy,r=reach+extent(*it);if(d<=r*r)nearest.push_back({d,short(it.index())});
   }
   const size_t n=std::min(nearest.size(),size_t(NAV::NUM_NODES_PER_CELL));std::partial_sort(nearest.begin(),nearest.begin()+n,nearest.end());for(size_t i=0;i<n;++i)cell.push_back(nearest[i].mHandle);
  };
  auto&cell=mCells.get_cell(x,y);
  fill(mGraph.nodes_begin(),mGraph.nodes_end(),cell.mNodes,[](const CWayNode&){return 0.f;},
   [](int index,const CWayNode&){return !waypointRoomNode(index);});
  fill(mGraph.edges_begin(),mGraph.edges_end(),cell.mEdges,[](const CWayEdge&e){return e.mDistance*.5f;},
   [](int,const CWayEdge& edge){return !waypointRoomNode(edge.mNodeA);});
 }
}
void waypointRegions(){
 mUser.ClearActor();mRegion.clear();mIslandRegion=mRegion.reserve();mIslandCount=0;
 for(auto it=mGraph.nodes_begin();it!=mGraph.nodes_end();++it){if(!mGraph.node_has_neighbors(it.index())){it->mFlags.set_bit(CWayNode::WN_ISLAND);mRegion.assign_region(it.index(),mIslandRegion);++mIslandCount;}else it->mFlags.clear_bit(CWayNode::WN_ISLAND);}
 // Live graphs can exceed the region and per-region edge capacities even
 // when the node/edge pools are within bounds. Use native A* directly; all
 // callers explicitly support an empty optional coarse region cache.
 mRegion.clear();
}
void waypointConnect(int a,int b){
 if(a==b||!a||!b||waypointRoomNode(a)!=waypointRoomNode(b)||mGraph.get_edge_across(a,b)||
    mGraph.size_edges()>=NAV::NUM_EDGES-1||mGraph.get_node_neighbors(a).full()||mGraph.get_node_neighbors(b).full())return;
 const auto&A=mGraph.get_node(a);const auto&B=mGraph.get_node(b);if(A.mPoint.Dist2(B.mPoint)<1||A.mPoint.Dist2(B.mPoint)>4096.f*4096.f)return;
 CWayEdge edge;mUser.setup_edge(edge,a,b,false,A,B,true);edge.mFlags.set_bit(CWayEdge::WE_PORTABLE);edge.mFlags.set_bit(CWayEdge::WE_SIZE_LARGE);
 if(A.mFlags.get_bit(CWayNode::WN_FLOATING)||B.mFlags.get_bit(CWayNode::WN_FLOATING))edge.mFlags.set_bit(CWayEdge::WE_FLYING);
 mGraph.connect_node(edge,a,b);
}
}
static uint64_t waypointRoomNode(int handle){
 const auto found=liveWaypointNodeRooms.find(handle);
 return found==liveWaypointNodeRooms.end()?0:found->second;
}
static bool waypointHasWorldNodes(){return size_t(mGraph.size_nodes())>liveWaypointNodeRooms.size();}
static NAV::TNodeHandle waypointRoomNearest(uint64_t room,const vec3_t& position){
 const auto found=liveWaypointRoomNodes.find(room);
 if(found==liveWaypointRoomNodes.end())return WAYPOINT_NONE;
 const CVec3 from(position);
 float bestDistance=NAV::VIEW_RANGE*NAV::VIEW_RANGE;
 int best=WAYPOINT_NONE;
 for(const int handle:found->second){
  const auto& node=mGraph.get_node(handle);
  if(node.mFlags.get_bit(CWayNode::WN_ISLAND))continue;
  const float distance=node.mPoint.Dist2(from);
  if(distance<bestDistance&&ViewNavTrace(from,node.mPoint)){
   best=handle;
   bestDistance=distance;
  }
 }
 return best;
}
static bool portableNavEdgeClear(const CWayEdge&edge,int size,const gentity_t* actor){
 float radius=size==CWayEdge::WE_SIZE_LARGE?NAV::SC_LARGE_RADIUS:NAV::SC_MEDIUM_RADIUS;
 float height=size==CWayEdge::WE_SIZE_LARGE?NAV::SC_LARGE_HEIGHT:NAV::SC_MEDIUM_HEIGHT;
 if(actor&&G_SiegeKorribanRancor(*actor)){
  radius=std::max({fabsf(actor->mins[0]),fabsf(actor->mins[1]),actor->maxs[0],actor->maxs[1]});
  height=actor->maxs[2]-actor->mins[2];
 }
 const auto&a=edge.PointA();const auto&b=edge.PointB();trace_t tr;
 if(!waypointTrace(a.v,b.v,radius,height,tr))return false;
 if(edge.mFlags.get_bit(CWayEdge::WE_FLYING))return true;
 // Sample the entire route, including its ends. A clear air trace alone must
 // not admit a walking route across an open pit or over a steep unsupported gap.
 const int steps=std::max(1,int(std::ceil(a.Dist(b)/32.f)));
 for(int i=0;i<=steps;++i){vec3_t p;for(int k=0;k<3;++k)p[k]=a[k]+(b[k]-a[k])*float(i)/steps;if(!waypointFloor(p,48))return false;}
 return true;
}
static bool waypointAdmission(){
 int count=0;for(int n=1;n<globals.num_entities;++n)if(G_WaypointPortable(g_entities[n])||waypointRoom(g_entities[n]))++count;
 return mGraph.size_nodes()-int(liveWaypointHandles.size())+count<NAV::NUM_NODES-1;
}
void G_WaypointReset(){
 liveWaypointSnapshot.clear();liveWaypointHandles.clear();liveWaypointNodeRooms.clear();liveWaypointRoomNodes.clear();
 liveWaypointsDirty=false;
}
void G_WaypointFrame(){
 std::vector<LiveWaypoint> next;
 for(int i=1;i<globals.num_entities;++i){auto&e=g_entities[i];jar::waypoint::Settings s;if(!waypointRead(e,s)||!waypointPosition(e))continue;
  LiveWaypoint n{};n.entity=i;n.room=waypointRoom(e);VectorCopy(e.currentOrigin,n.point);n.range=s.range;n.enabled=s.enabled;n.small=!strcmp(e.classname,"waypoint_small");n.name=e.targetname?e.targetname:"";n.primary=e.target?e.target:"";n.secondary=e.target2?e.target2:"";next.push_back(std::move(n));
 }
 if(!liveWaypointsDirty&&next==liveWaypointSnapshot)return;liveWaypointsDirty=false;
 // No NPC is being updated here: the workshop calls this between frames.
 // Authored nodes and edges keep their handles and data; only owned nodes go.
 for(const auto&entry:liveWaypointHandles)mGraph.remove_node(entry.second);
 liveWaypointHandles.clear();liveWaypointNodeRooms.clear();liveWaypointRoomNodes.clear();
 // Search behavior can retain a home node outside the ordinary path cache.
 // Keep authored homes; discard live/saved handles before pool slots are reused.
 bool authoredHome[NAV::NUM_NODES]{};for(auto it=mGraph.nodes_begin();it!=mGraph.nodes_end();++it)authoredHome[it.index()]=true;
 for(int i=0;i<globals.num_entities;++i)if(auto*npc=g_entities[i].NPC){const int home=npc->homeWp;if(home<0||home>=NAV::NUM_NODES||!authoredHome[home])npc->homeWp=WAYPOINT_NONE;}
 for(const auto&n:next){if(!n.enabled||mGraph.size_nodes()>=NAV::NUM_NODES-1)continue;
  CWayNode node{};node.mPoint=n.point;node.mPoint[2]-=20;node.mRadius=n.small?2:20;node.mType=NAV::PT_WAYNODE;node.mName=n.name.c_str();node.mFlags.clear();
  trace_t tr;if(!waypointTrace(node.mPoint.v,node.mPoint.v,NAV::SC_MEDIUM_RADIUS,NAV::SC_MEDIUM_HEIGHT,tr))continue;
  if(!waypointFloor(node.mPoint.v,96))node.mFlags.set_bit(CWayNode::WN_FLOATING);
  const int handle=mGraph.insert_node(node);
  liveWaypointHandles[n.entity]=handle;
  if(n.room){liveWaypointNodeRooms.emplace(handle,n.room);liveWaypointRoomNodes[n.room].push_back(handle);}
 }
 // Explicit links take priority over auto connections and may form cycles.
 for(const auto&n:next){auto src=liveWaypointHandles.find(n.entity);if(src==liveWaypointHandles.end())continue;for(const auto&name:{n.primary,n.secondary})if(!name.empty())for(const auto&t:next)if(t.room==n.room&&t.name==name){auto dst=liveWaypointHandles.find(t.entity);if(dst!=liveWaypointHandles.end())waypointConnect(src->second,dst->second);}}
 for(const auto&n:next){auto src=liveWaypointHandles.find(n.entity);if(src==liveWaypointHandles.end()||n.range<=0)continue;const int a=src->second;const auto&A=mGraph.get_node(a);std::vector<std::pair<float,int>> nearby;
  for(auto it=mGraph.nodes_begin();it!=mGraph.nodes_end();++it){const int b=it.index();if(a==b||waypointRoomNode(b))continue;const float d=A.mPoint.Dist2(it->mPoint);if(d<=n.range*n.range&&d>=1&&gi.inPVS(A.mPoint.v,it->mPoint.v))nearby.emplace_back(d,b);}
  std::sort(nearby.begin(),nearby.end());int added=0;for(const auto&entry:nearby){const int before=mGraph.size_edges();waypointConnect(a,entry.second);if(mGraph.size_edges()>before&&++added==8)break;}
 }
 waypointCells();waypointRegions();waypointClearPaths();liveWaypointSnapshot=std::move(next);
}
bool G_WaypointRestore(){
 int count=0;for(int i=1;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_WaypointPortable(e)&&!waypointRoom(e))continue;jar::waypoint::Settings s;if(!waypointRead(e,s)||!waypointPosition(e)||++count>NAV::NUM_NODES-1)return false;}
 if(mGraph.size_nodes()-int(liveWaypointHandles.size())+count>NAV::NUM_NODES-1)return false;
 if(!count&&liveWaypointHandles.empty())return true;
 liveWaypointsDirty=true;G_WaypointFrame();return true;
}
const char*G_WaypointStatus(const gentity_t&e){
 if(!G_WaypointPortable(e))return "Not a portable waypoint";if(e.svFlags&SVF_INACTIVE)return "Disabled";
 const auto found=liveWaypointHandles.find(e.s.number);if(found==liveWaypointHandles.end())return "Pending or blocked location";
 const auto&neighbors=mGraph.get_node_neighbors(found->second);int clear=0;for(int i=0;i<neighbors.size();++i)if(portableNavEdgeClear(mGraph.get_edge(neighbors[i].mEdge),CWayEdge::WE_SIZE_MEDIUM))++clear;
 return clear?"Connected to native navigation":"No clear connections; move or link nodes";
}
