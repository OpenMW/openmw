ps.1.4
  // conversion from Cg generated ARB_fragment_program to ps.1.4 by NFZ
  // command line args: -profile arbfp1 -entry main_fp
  // program main_fp
  // c0 : distortionRange
  // c1 : tintColour
  // testure 0 : noiseMap
  // texture 1 : reflectMap
  // texture 2 : refractMap
  // v0.x : fresnel 
  // t0.xyz : noiseCoord
  // t1.xyw : projectionCoord 

def c2, 2, 1, 0, 0

  // Cg:	distort.x = tex3D(noiseMap, noiseCoord).x;
  // arbfp1:	TEX R0.x, fragment.texcoord[0], texture[0], 3D;
  // sample noise map using noiseCoord in TEX unit 0 

texld r0, t0.xyz

  // get projected texture coordinates from TEX coord 1
  // will be used in phase 2

texcrd r1.xy, t1_dw.xyw
mov r1.z, c2.y

  // Cg:	distort.y = tex3D(noiseMap, noiseCoord + yoffset).x;
  // arbfp1:	ADD R1.xyz, fragment.texcoord[0], c1;
  // arbfp1:	TEX R1.x, R1, texture[0], 3D;
  // arbfp1:	MOV R0.y, R1.x;

  // Cg:	distort = (distort * 2 - 1) * distortionRange;
  // arbfp1:	MAD R0.xy, R0, c0.x, -c0.y;
  // arbfp1:	MUL R0.xy, R0, u0.x;
  // (distort * 2 - 1) same as 2*(distort -.5) so use _bx2


  // Cg:	final = projectionCoord.xy / projectionCoord.w;
  // Cg:	final += distort;
  // arbfp1:	RCP R0.w, fragment.texcoord[1].w;
  // arbfp1:	MAD R0.xy, fragment.texcoord[1], R0.w, R0;
  // 	final = (distort *  projectionCoord.w) + projectionCoord.xy
  // for ps.1.4 have to re-arrange things a bit to perturb projected texture coordinates

mad r0.xyz, r0_bx2, c0.x, r1

phase

  // do dependant texture reads
  // Cg:	reflectionColour = tex2D(reflectMap, final);
  // arbfp1:	TEX R0, R0, texture[1], 2D;
  // sampe reflectMap using dependant read : texunit 1 

texld r1, r0.xyz

  // Cg:	refractionColour = tex2D(refractMap, final) + tintColour;
  // arbfp1:	TEX R1, R0, texture[2], 2D;
  // sample refractMap : texunit 2 

texld r2, r0.xyz

  // adding tintColour that is in global c1
  // arbfp1:	ADD R1, R1, u1;

add r2, r2, c1

  // Cg:	col = lerp(refractionColour, reflectionColour, fresnel);
  // arbfp1:	ADD R0, R0, -R1;
  // arbfp1:	MAD result.color, fragment.color.primary.x, R0, R1;

lrp r0, v0.x, r1, r2
