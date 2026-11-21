// Classic renders each selected camera feed into its own color target.
namespace {
PFNGLBINDFRAMEBUFFERPROC bindMonitorFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC deleteMonitorFramebuffers;
PFNGLDELETERENDERBUFFERSPROC deleteMonitorRenderbuffers;
GLuint monitorFramebuffers[JAR_PORTAL_CAMERA_FEEDS];
GLuint monitorDepth;
}

void R_InitPortalMonitor() {
 bindMonitorFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(ri.GL_GetProcAddress("glBindFramebuffer"));
 deleteMonitorFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(ri.GL_GetProcAddress("glDeleteFramebuffers"));
 deleteMonitorRenderbuffers = reinterpret_cast<PFNGLDELETERENDERBUFFERSPROC>(ri.GL_GetProcAddress("glDeleteRenderbuffers"));
 const auto genFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(ri.GL_GetProcAddress("glGenFramebuffers"));
 const auto genRenderbuffers = reinterpret_cast<PFNGLGENRENDERBUFFERSPROC>(ri.GL_GetProcAddress("glGenRenderbuffers"));
 const auto bindRenderbuffer = reinterpret_cast<PFNGLBINDRENDERBUFFERPROC>(ri.GL_GetProcAddress("glBindRenderbuffer"));
 const auto renderbufferStorage = reinterpret_cast<PFNGLRENDERBUFFERSTORAGEPROC>(ri.GL_GetProcAddress("glRenderbufferStorage"));
 const auto attachRenderbuffer = reinterpret_cast<PFNGLFRAMEBUFFERRENDERBUFFERPROC>(ri.GL_GetProcAddress("glFramebufferRenderbuffer"));
 const auto attachTexture = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(ri.GL_GetProcAddress("glFramebufferTexture2D"));
 const auto checkFramebuffer = reinterpret_cast<PFNGLCHECKFRAMEBUFFERSTATUSPROC>(ri.GL_GetProcAddress("glCheckFramebufferStatus"));
 if (!bindMonitorFramebuffer || !deleteMonitorFramebuffers || !deleteMonitorRenderbuffers ||
     !genFramebuffers || !genRenderbuffers || !bindRenderbuffer || !renderbufferStorage ||
     !attachRenderbuffer || !attachTexture || !checkFramebuffer) return;

 static const byte empty[JAR_PORTAL_CAMERA_SIZE * JAR_PORTAL_CAMERA_SIZE * 4]{};
 genFramebuffers(JAR_PORTAL_CAMERA_FEEDS, monitorFramebuffers);
 genRenderbuffers(1, &monitorDepth);
 bindRenderbuffer(GL_RENDERBUFFER, monitorDepth);
 renderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, JAR_PORTAL_CAMERA_SIZE, JAR_PORTAL_CAMERA_SIZE);
 for (int i = 0; i < JAR_PORTAL_CAMERA_FEEDS; ++i) {
  if (!monitorFramebuffers[i] || !monitorDepth) continue;
  tr.portalMonitorImages[i] = R_CreateImage(va("*jarCamera%d", i), empty, JAR_PORTAL_CAMERA_SIZE,
   JAR_PORTAL_CAMERA_SIZE, GL_RGBA, qfalse, qfalse, qfalse, GL_CLAMP);
  bindMonitorFramebuffer(GL_FRAMEBUFFER, monitorFramebuffers[i]);
  attachTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tr.portalMonitorImages[i]->texnum, 0);
  attachRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, monitorDepth);
  if (checkFramebuffer(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
   deleteMonitorFramebuffers(1, &monitorFramebuffers[i]);
   monitorFramebuffers[i] = 0;
  }
 }
 bindRenderbuffer(GL_RENDERBUFFER, 0);
 bindMonitorFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_ShutdownPortalMonitor() {
 if (monitorDepth) deleteMonitorRenderbuffers(1, &monitorDepth);
 if (deleteMonitorFramebuffers) deleteMonitorFramebuffers(JAR_PORTAL_CAMERA_FEEDS, monitorFramebuffers);
 memset(monitorFramebuffers, 0, sizeof(monitorFramebuffers));
 monitorDepth = 0;
}

bool R_PortalMonitorReady(int slot) { return monitorFramebuffers[slot] != 0; }

static void RB_DrawPortalMonitor(const drawSurfsCommand_t& command) {
 bindMonitorFramebuffer(GL_FRAMEBUFFER, monitorFramebuffers[command.viewParms.portalMonitor - 1]);
 RB_RenderDrawSurfList(command.drawSurfs, command.numDrawSurfs);
 bindMonitorFramebuffer(GL_FRAMEBUFFER, 0);
}
