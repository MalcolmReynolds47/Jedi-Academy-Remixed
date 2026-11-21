gfx/jar/portal_panel
{
    portal
    surfaceparm nomarks
    {
        map $whiteimage
        blendFunc GL_ZERO GL_ONE
        depthWrite
    }
}

gfx/jar/portal_monitor
{
    cull none
    sort opaque
    surfaceparm nomarks
    nomipmaps
    {
        map $jarCamera
        rgbGen identity
        depthWrite
    }
}

gfx/jar/portal_monitor1
{
    cull none
    sort opaque
    surfaceparm nomarks
    nomipmaps
    {
        map $jarCamera1
        rgbGen identity
        depthWrite
    }
}

gfx/jar/portal_monitor2
{
    cull none
    sort opaque
    surfaceparm nomarks
    nomipmaps
    {
        map $jarCamera2
        rgbGen identity
        depthWrite
    }
}

gfx/jar/portal_monitor3
{
    cull none
    sort opaque
    surfaceparm nomarks
    nomipmaps
    {
        map $jarCamera3
        rgbGen identity
        depthWrite
    }
}
