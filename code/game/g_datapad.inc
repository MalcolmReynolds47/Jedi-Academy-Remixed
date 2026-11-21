// The unused retail pickup opens the existing mission/equipment screen.
// No mission objective or campaign variable is replaced. GPL-2.0-only.
static bool G_UseDatapad(gentity_t*ent){
 if(!ent||!ent->inuse||ent->s.number!=0||!ent->client||ent->health<=0||in_camera||
    ent->client->ps.viewEntity||ent->client->ps.inventory[INV_DATAPAD]<=0)return false;
 gi.SendConsoleCommand("datapad\n");return true;
}
