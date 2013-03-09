float DecodeHeightmap(float4 heightmap)
{
    float4 table = float4(1.0, -1.0, 0.0, 0.0);
	return dot(heightmap, table);
}

float DecodeHeightmap(shTexture2D HeightmapSampler, float2 texcoord)
{
	float4 heightmap = shSample(HeightmapSampler, texcoord);
	return DecodeHeightmap(heightmap);
}

float4 EncodeHeightmap(float fHeight)
{
	float h = fHeight;
	float positive = fHeight > 0.0 ? fHeight : 0.0;
	float negative = fHeight < 0.0 ? -fHeight : 0.0;

	float4 color = float4(0,0,0,0);

	color.r = positive;
	color.g = negative;

	return color;
}
