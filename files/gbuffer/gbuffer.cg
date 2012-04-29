void RenderScene_vs(in float4 position : POSITION
	,in float2 uv :TEXCOORD0
	,uniform float4x4 wvp
	,out float4 oPosition : POSITION
	,out float2 oUV :TEXCOORD0)
{
	oPosition = mul(wvp, position);
	oUV = uv;
}

void RenderScene_ps(in float4 position : POSITION
	,in float2 uv :TEXCOORD0
	,uniform sampler2D tex1 : TEXUNIT0
	,out float4 oColor : COLOR)
{
	float4 scene =tex2D(tex1, uv);
	oColor= scene;
}
