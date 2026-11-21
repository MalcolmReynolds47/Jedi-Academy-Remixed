// Grant only catalog weapons, respecting the same server rules as pickups.
extern int G_ItemDisabled(gitem_t*);
static qboolean EquipWeapon(gentity_t*p,unsigned id){
 int index;const jarMPEntry_t*entry=Entry(id,&index);gitem_t*item=NULL;
 if(!entry||entry->tab!=2||entry->kind!=4)return qfalse;
 for(int i=1;i<bg_numItems;++i)if(bg_itemlist[i].classname&&!strcmp(bg_itemlist[i].classname,entry->name)){item=&bg_itemlist[i];break;}
 if(!item||item->giType!=IT_WEAPON||item->giTag<=WP_NONE||item->giTag>=WP_NUM_WEAPONS)return qfalse;
 int weapon=item->giTag,ammo=weaponData[weapon].ammoIndex;
 int disabled=(level.gametype==GT_DUEL||level.gametype==GT_POWERDUEL)?g_duelWeaponDisable.integer:g_weaponDisable.integer;
 if((level.gametype!=GT_JEDIMASTER&&(disabled&(1<<weapon)))||G_ItemDisabled(item)){Tell(p,"This weapon is disabled by the server's rules.");return qfalse;}
 if(ammo<AMMO_NONE||ammo>=AMMO_MAX)return qfalse;
 playerState_t*ps=&p->client->ps;
 if(ps->saberInFlight||ps->saberLockTime>level.time||ps->emplacedIndex||ps->forceHandExtend!=HANDEXTEND_NONE){Tell(p,"Finish the current held or locked action before equipping.");return qfalse;}
 RegisterItem(item);SaveRegisteredItems();Drop(p->s.number,qfalse);
 ps->stats[STAT_WEAPONS]|=1<<weapon;
 if(ammo!=AMMO_NONE)Add_Ammo(p,ammo,item->quantity>0?item->quantity:50);
 ps->weapon=weapon;ps->weaponTime=0;ps->weaponstate=WEAPON_READY;
 ps->saberMove=LS_READY;ps->saberBlocked=BLOCKED_NONE;
 if(weapon==WP_SABER)ps->saberHolstered=0;
 p->s.weapon=weapon;p->client->pers.cmd.weapon=weapon;
 trap->SendServerCommand(p->s.number,va("jarequip %d",weapon));
 Tell(p,"Weapon granted and equipped.");return qtrue;
}
