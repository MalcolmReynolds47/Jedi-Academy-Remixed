gfx/jar/saber_rgb_glow
{
    cull disable
    {
        clampmap gfx/effects/sabers/blurglow.jpg
        blendFunc GL_ONE GL_ONE
        rgbGen entity
    }
}

gfx/jar/saber_rgb_core
{
    cull disable
    {
        clampmap gfx/effects/sabers/blurcore.jpg
        blendFunc GL_ONE GL_ONE
        rgbGen identity
    }
}
