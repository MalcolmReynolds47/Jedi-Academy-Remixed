// Shared SP/MP console palette and native menu choices. GPL-2.0-only.
#ifndef JAR_CONSOLE_STYLE_H
#define JAR_CONSOLE_STYLE_H
#define JAR_CONSOLE_STYLE_CHOICES {{"Retail","0"},{"Slate","1"},{"Amber","2"},{"Black","3"}}
#define JAR_CONSOLE_FRACTION_CHOICES {{"25%","0.25"},{"50%","0.5"},{"75%","0.75"},{"100%","1"}}
#define JAR_CONSOLE_SCALE_CHOICES {{"100%","1"},{"150%","1.5"},{"200%","2"},{"300%","3"},{"400%","4"}}
// Caller supplies the native clamped opacity. Full-screen consoles stay opaque;
// style zero and negative custom values retain the native retail texture path.
static int JAR_ConsoleColor(int style,float fraction,float opacity,float color[4]){
 if(style<=0)return 0;
 color[0]=color[1]=color[2]=0;color[3]=fraction<1.0f?opacity:1.0f;
 if(style==1){color[0]=0.025f;color[1]=0.045f;color[2]=0.075f;}
 if(style==2){color[0]=0.085f;color[1]=0.045f;color[2]=0.01f;}
 return 1;
}
#endif
