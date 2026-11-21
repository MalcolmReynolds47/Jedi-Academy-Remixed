// Rebuild pointer focus for Sandbox click-down after a menu refresh.
static void UI_JAR_ClickFocus(int key,qboolean down,int x,int y){
 if(down&&(key==A_MOUSE1||key==A_MOUSE2)&&UI_JAR_OverlayActive()&&!UI_JAR_OverlayTyping())
  Display_MouseMove(nullptr,x,y);
}
