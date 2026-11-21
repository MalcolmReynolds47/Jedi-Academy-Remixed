// Native image lookup shared with the hosting regression fixture. GPL-2.0-only.
image_t	*R_FindImageFile( const char *name, qboolean mipmap, qboolean allowPicmip, qboolean allowTC, int glWrapClampMode ) {
	image_t	*image;
	int		width, height;
	byte	*pic;

	if (!name || ri.Cvar_VariableIntegerValue( "dedicated" ) )	// stop ghoul2 horribleness as regards image loading from server
	{
		return NULL;
	}

	// need to do this here as well as in R_CreateImage, or R_FindImageFile_NoLoad() may complain about
	//	different clamp parms used...
	//
	if(glConfig.clampToEdgeAvailable && glWrapClampMode == GL_CLAMP) {
		glWrapClampMode = GL_CLAMP_TO_EDGE;
	}

	image = R_FindImageFile_NoLoad(name, mipmap, allowPicmip, allowTC, glWrapClampMode );
	if (image) {
		return image;
	}

	//
	// load the pic from disk
	//
	R_LoadImage( name, &pic, &width, &height );
	if ( pic == NULL ) {                                    // if we dont get a successful load
		return NULL;                                        // bail
	}


	// refuse to find any files not power of 2 dims...
	//
	if ( (width&(width-1)) || (height&(height-1)) )
	{
		ri.Printf( PRINT_ALL, "Refusing to load non-power-2-dims(%d,%d) pic \"%s\"...\n", width,height,name );
		return NULL;
	}

	image = R_CreateImage( ( char * ) name, pic, width, height, GL_RGBA, mipmap, allowPicmip, allowTC, glWrapClampMode );
	Z_Free( pic );
	return image;
}
