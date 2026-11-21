// Shared native classic-renderer controls for SP/MP. GPL-2.0-only.
#ifndef JAR_CLASSIC_GRAPHICS_H
#define JAR_CLASSIC_GRAPHICS_H
#define JAR_GFX_R_EXT_MULTISAMPLE_CHOICES {{"Off","0"},{"2x","2"},{"4x","4"},{"8x","8"}}
#define JAR_GFX_R_DYNAMICGLOW_CHOICES {{"Off","0"},{"On","1"}}
#define JAR_GFX_R_SWAPINTERVAL_CHOICES {{"Off","0"},{"On","1"}}
#define JAR_GFX_COM_MAXFPS_CHOICES {{"60","60"},{"120","120"},{"125","125"},{"144","144"},{"240","240"},{"Unlimited","0"}}
#define JAR_GFX_CG_DRAWFPS_CHOICES {{"Off","0"},{"On","1"}}
#define JAR_GFX_R_PICMIP_CHOICES {{"Full","0"},{"Half","1"},{"Quarter","2"},{"Eighth","3"}}
#define JAR_GFX_R_TEXTUREMODE_CHOICES {{"Bilinear","GL_LINEAR_MIPMAP_NEAREST"},{"Trilinear","GL_LINEAR_MIPMAP_LINEAR"}}
#define JAR_GFX_R_EXT_TEXTURE_FILTER_ANISOTROPIC_CHOICES {{"Off","1"},{"2x","2"},{"4x","4"},{"8x","8"},{"16x","16"}}
#define JAR_GFX_R_LODBIAS_CHOICES {{"High","0"},{"Medium","1"},{"Low","2"}}
#define JAR_GFX_R_SUBDIVISIONS_CHOICES {{"High","4"},{"Medium","12"},{"Low","20"}}
#define JAR_GFX_CG_SHADOWS_CHOICES {{"Off","0"},{"Blob","1"},{"Stencil","2"},{"Projected","3"}}
#define JAR_GFX_R_DYNAMICLIGHT_CHOICES {{"Off","0"},{"On","1"}}
#define JAR_GFX_R_SURFACESPRITES_CHOICES {{"Off","0"},{"On","1"}}
#define JAR_GFX_CG_MARKS_CHOICES {{"Off","0"},{"On","1"}}
#define JAR_GFX_R_FASTSKY_CHOICES {{"Off","0"},{"On","1"}}
// id, cvar, label, initial, video restart, MP page, shared choices
#define JAR_CLASSIC_GRAPHICS_ROWS(X) \
 X(r_ext_multisample,"r_ext_multisample","MSAA","0",1,0,JAR_GFX_R_EXT_MULTISAMPLE_CHOICES) \
 X(r_dynamicGlow,"r_dynamicGlow","Dynamic glow","0",1,0,JAR_GFX_R_DYNAMICGLOW_CHOICES) \
 X(r_swapInterval,"r_swapInterval","VSync","0",0,0,JAR_GFX_R_SWAPINTERVAL_CHOICES) \
 X(com_maxfps,"com_maxfps","Frame limit","125",0,0,JAR_GFX_COM_MAXFPS_CHOICES) \
 X(cg_drawFPS,"cg_drawFPS","Show FPS","0",0,0,JAR_GFX_CG_DRAWFPS_CHOICES) \
 X(r_picmip,"r_picmip","Texture detail","0",1,1,JAR_GFX_R_PICMIP_CHOICES) \
 X(r_textureMode,"r_textureMode","Texture filtering","GL_LINEAR_MIPMAP_NEAREST",0,1,JAR_GFX_R_TEXTUREMODE_CHOICES) \
 X(r_ext_texture_filter_anisotropic,"r_ext_texture_filter_anisotropic","Anisotropic filter","16",1,1,JAR_GFX_R_EXT_TEXTURE_FILTER_ANISOTROPIC_CHOICES) \
 X(r_lodbias,"r_lodbias","Model detail","0",0,1,JAR_GFX_R_LODBIAS_CHOICES) \
 X(r_subdivisions,"r_subdivisions","Curved surfaces","4",1,1,JAR_GFX_R_SUBDIVISIONS_CHOICES) \
 X(cg_shadows,"cg_shadows","Character shadows","1",0,2,JAR_GFX_CG_SHADOWS_CHOICES) \
 X(r_dynamiclight,"r_dynamiclight","Dynamic lights","1",0,2,JAR_GFX_R_DYNAMICLIGHT_CHOICES) \
 X(r_surfaceSprites,"r_surfaceSprites","Grass and foliage","1",0,2,JAR_GFX_R_SURFACESPRITES_CHOICES) \
 X(cg_marks,"cg_marks","Impact marks","1",0,2,JAR_GFX_CG_MARKS_CHOICES) \
 X(r_fastsky,"r_fastsky","Simple sky","0",0,2,JAR_GFX_R_FASTSKY_CHOICES)
#endif
