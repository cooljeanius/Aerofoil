cbuffer SDrawQuadPixelConstants : register(b0)
{
	float4 constants_Modulation;
	
	int2 constants_FlickerAxis;
	int constants_FlickerStartThreshold;
	int constants_FlickerEndThreshold;
	
	float constants_Desaturation;
	float3 constants_Unused;
};
