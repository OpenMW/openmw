#define SHADOWS @shadows_enabled
#define NUMTEX @num_pssm_texture
#define TEXOFFSET @texture_offset
void setupShadowCoords(vec4 viewPos)
{
#if SHADOWS
   mat4 eyePlaneMat;
   eyePlaneMat = mat4(gl_EyePlaneS[TEXOFFSET+0], gl_EyePlaneT[TEXOFFSET+0], gl_EyePlaneR[TEXOFFSET+0], gl_EyePlaneQ[TEXOFFSET+0]); gl_TexCoord[TEXOFFSET+0] = viewPos * eyePlaneMat;
#if(NUMTEX >1)
   {eyePlaneMat =mat4(gl_EyePlaneS[TEXOFFSET+1], gl_EyePlaneT[TEXOFFSET+1], gl_EyePlaneR[TEXOFFSET+1], gl_EyePlaneQ[TEXOFFSET+1]); gl_TexCoord[TEXOFFSET+1] = viewPos * eyePlaneMat;}
#endif
#if(NUMTEX >2)
   {eyePlaneMat =mat4(gl_EyePlaneS[TEXOFFSET+2], gl_EyePlaneT[TEXOFFSET+2], gl_EyePlaneR[TEXOFFSET+2], gl_EyePlaneQ[TEXOFFSET+2]); gl_TexCoord[TEXOFFSET+2] = viewPos * eyePlaneMat;}
#endif
#if(NUMTEX >3)
   {eyePlaneMat =mat4(gl_EyePlaneS[TEXOFFSET+3], gl_EyePlaneT[TEXOFFSET+3], gl_EyePlaneR[TEXOFFSET+3], gl_EyePlaneQ[TEXOFFSET+3]); gl_TexCoord[TEXOFFSET+3] = viewPos * eyePlaneMat;}
#endif
#if(NUMTEX >4)
   {eyePlaneMat =mat4(gl_EyePlaneS[TEXOFFSET+4], gl_EyePlaneT[TEXOFFSET+4], gl_EyePlaneR[TEXOFFSET+4], gl_EyePlaneQ[TEXOFFSET+4]); gl_TexCoord[TEXOFFSET+4] = viewPos * eyePlaneMat;}
#endif
#endif // SHADOWS
}
