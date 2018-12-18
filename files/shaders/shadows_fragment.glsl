#define SHADOWS @shadows_enabled
#define MAX_SHADOW_MAP 5
//PSSM SHADOW
uniform vec2 ambientBias;
uniform float  zShadow0;
uniform float  zShadow1;
uniform float  zShadow2;
uniform float  zShadow3;
uniform float  zShadow4;

uniform sampler2DShadow shadowTexture0;
uniform sampler2DShadow shadowTexture1;
uniform sampler2DShadow shadowTexture2;
uniform sampler2DShadow shadowTexture3;
uniform sampler2DShadow shadowTexture4;

const float fTexelSize= (1.41 / float(@pssm_texture_size));
const float fZOffSet  = -0.001954; // 2^-9 good value for ATI / NVidia

float map0[MAX_SHADOW_MAP];

float unshadowedLightRatio()
{
	float shadowing = 1.0;
	#if SHADOWS
	float testZ = gl_FragCoord.z*2.0-1.0;

	map0[0] = step(testZ, zShadow0);
        if(@num_pssm_texture >1) map0[1] =  step(zShadow0,testZ)*step(testZ, zShadow1);
        if(@num_pssm_texture >2) map0[2] =  step(zShadow1,testZ)*step(testZ, zShadow2);
        if(@num_pssm_texture >3) map0[3] =  step(zShadow2,testZ)*step(testZ, zShadow3);
        if(@num_pssm_texture >4) map0[4] =  step(zShadow3,testZ)*step(testZ, zShadow4);

        float term=0.0;

        float shadowOrg0 = shadow2D( shadowTexture0, gl_TexCoord[@texture_offset +0].xyz+vec3(0.0,0.0,fZOffSet) ).r;
        float shadow00 = shadow2D( shadowTexture0, gl_TexCoord[@texture_offset +0].xyz+vec3(-fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow10 = shadow2D( shadowTexture0, gl_TexCoord[@texture_offset +0].xyz+vec3(fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow20 = shadow2D( shadowTexture0, gl_TexCoord[@texture_offset +0].xyz+vec3(fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow30 = shadow2D( shadowTexture0, gl_TexCoord[@texture_offset +0].xyz+vec3(-fTexelSize,fTexelSize,fZOffSet) ).r;
	float shadow0 = ( 2.0*shadowOrg0 + shadow00 + shadow10 + shadow20 + shadow30)/6.0;
        term+= (shadow0)*map0[0];
#if(@num_pssm_texture >1)
        float shadowOrg1 = shadow2D( shadowTexture1, gl_TexCoord[@texture_offset +1].xyz+vec3(0.0,0.0,fZOffSet) ).r;
        float shadow01 = shadow2D( shadowTexture1, gl_TexCoord[@texture_offset +1].xyz+vec3(-fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow11 = shadow2D( shadowTexture1, gl_TexCoord[@texture_offset +1].xyz+vec3(fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow21 = shadow2D( shadowTexture1, gl_TexCoord[@texture_offset +1].xyz+vec3(fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow31 = shadow2D( shadowTexture1, gl_TexCoord[@texture_offset +1].xyz+vec3(-fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow1 = ( 2.0*shadowOrg1 + shadow01 + shadow11 + shadow21 + shadow31)/6.0;
        term+= (shadow1)*map0[1];
#endif
#if(@num_pssm_texture >2)
        float shadowOrg2 = shadow2D( shadowTexture2, gl_TexCoord[@texture_offset +2].xyz+vec3(0.0,0.0,fZOffSet) ).r;
        float shadow02 = shadow2D( shadowTexture2, gl_TexCoord[@texture_offset +2].xyz+vec3(-fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow12 = shadow2D( shadowTexture2, gl_TexCoord[@texture_offset +2].xyz+vec3(fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow22 = shadow2D( shadowTexture2, gl_TexCoord[@texture_offset +2].xyz+vec3(fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow32 = shadow2D( shadowTexture2, gl_TexCoord[@texture_offset +2].xyz+vec3(-fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow2 = ( 2.0*shadowOrg2 + shadow02 + shadow12 + shadow22 + shadow32)/6.0;
        term+= (shadow2)*map0[2];
#endif
#if(@num_pssm_texture >3)
        float shadowOrg3 = shadow2D( shadowTexture3, gl_TexCoord[@texture_offset +3].xyz+vec3(0.0,0.0,fZOffSet) ).r;
        float shadow03 = shadow2D( shadowTexture3, gl_TexCoord[@texture_offset +3].xyz+vec3(-fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow13 = shadow2D( shadowTexture3, gl_TexCoord[@texture_offset +3].xyz+vec3(fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow23 = shadow2D( shadowTexture3, gl_TexCoord[@texture_offset +3].xyz+vec3(fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow33 = shadow2D( shadowTexture3, gl_TexCoord[@texture_offset +3].xyz+vec3(-fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow3 = ( 2.0*shadowOrg3 + shadow03 + shadow13 + shadow23 + shadow33)/6.0;
        term+= (shadow3)*map0[3];
#endif
#if(@num_pssm_texture >4)
        float shadowOrg4 = shadow2D( shadowTexture4, gl_TexCoord[@texture_offset +4].xyz+vec3(0.0,0.0,fZOffSet) ).r;
        float shadow04 = shadow2D( shadowTexture4, gl_TexCoord[@texture_offset +4].xyz+vec3(-fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow14 = shadow2D( shadowTexture4, gl_TexCoord[@texture_offset +4].xyz+vec3(fTexelSize,-fTexelSize,fZOffSet) ).r;
        float shadow24 = shadow2D( shadowTexture4, gl_TexCoord[@texture_offset +4].xyz+vec3(fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow34 = shadow2D( shadowTexture4, gl_TexCoord[@texture_offset +4].xyz+vec3(-fTexelSize,fTexelSize,fZOffSet) ).r;
        float shadow4 = ( 2.0*shadowOrg4 + shadow04 + shadow14 + shadow24 + shadow34)/6.0;
        term+= (shadow4)*map0[4];
#endif
return clamp(term,0.0,1.0);

#endif // SHADOWS
return 1.0;
}
