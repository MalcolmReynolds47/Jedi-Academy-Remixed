// Native NPC file loading, shared with a file-list regression fixture. GPL-2.0-only.
void NPC_LoadParms( void )
{
	int			len, totallen, npcExtFNLen, fileCnt, i;
//	const char	*filename = "ext_data/NPC2.cfg";
	char		/**buffer,*/ *holdChar, *marker;
	char		npcExtensionListBuf[65536];		// Match SP; installed mod packs can exceed the old 2 KB list.
	fileHandle_t f;
	len = 0;

	//remember where to store the next one
	totallen = len;
	marker = NPCParms+totallen;
	*marker = 0;

	//now load in the extra .npc extensions
	fileCnt = trap->FS_GetFileList("ext_data/NPCs", ".npc", npcExtensionListBuf, sizeof(npcExtensionListBuf) );

	holdChar = npcExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += npcExtFNLen + 1 )
	{
		npcExtFNLen = strlen( holdChar );

//		Com_Printf( "Parsing %s\n", holdChar );

		len = trap->FS_Open(va( "ext_data/NPCs/%s", holdChar), &f, FS_READ);

		if ( len == -1 )
		{
			Com_Printf( "error reading file\n" );
		}
		else
		{
			// Reserve both the appended newline and the trailing NUL.
			if ( len < 0 || len >= MAX_NPC_DATA_SIZE - totallen - 1 ) {
				trap->FS_Close( f );
				trap->Error( ERR_DROP, "NPC extensions (*.npc) are too large" );
			}
			trap->FS_Read(npcParseBuffer, len, f);
			npcParseBuffer[len] = 0;

			len = COM_Compress( npcParseBuffer );

			strcat( marker, npcParseBuffer );
			strcat(marker, "\n");
			len++;
			trap->FS_Close(f);

			totallen += len;
			marker = NPCParms+totallen;
			//*marker = 0; //rww - make sure this is null or strcat will not append to the correct place
			//rww  12/19/02-actually the probelm was npcParseBuffer not being nul-term'd, which could cause issues in the strcat too
		}
	}
}
