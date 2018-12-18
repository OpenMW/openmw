#define SHADOWS @shadows_enabled
#define NUMTEX @num_pssm_texture
#define TEXOFFSET @texture_offset
void setupShadowCoords(vec4 viewPos)
{
#if SHADOWS
   mat4 eyePlaneMat;
   for(int i=TEXOFFSET ;i<TEXOFFSET + NUMTEX;++i)
   {
         eyePlaneMat = mat4(gl_EyePlaneS[i], gl_EyePlaneT[i], gl_EyePlaneR[i], gl_EyePlaneQ[i]);
         gl_TexCoord[i].xyzw = viewPos * eyePlaneMat;
   }
#endif // SHADOWS
}
