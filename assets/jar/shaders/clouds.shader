// Portable clouds reuse unedited textures/skies/cloudlayer2.png from retail.
// Native haze scrolling/fading parameters, under independent shader names.
jar/world/cloud
{
 cull twosided
 {
  map textures/skies/cloudlayer2
  blendFunc GL_ONE GL_ONE
  glow
  rgbGen exactVertex
  tcMod scale 0.006 0.004
  tcMod scroll 0.1 0
 }
}
jar/world/cloud_alt
{
 cull twosided
 {
  map textures/skies/cloudlayer2
  blendFunc GL_ONE GL_ONE
  glow
  rgbGen exactVertex
  tcMod scale 0.008 0.007
  tcMod scroll 0.08 0.09
 }
}
