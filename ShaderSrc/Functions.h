float SRGBToLinear(float v)
{
	if (v <= 0.04045)
		return v * (1.0 / 12.92);
	else
		return pow(((max(v, 0.0) + 0.055) * (1.0 / 1.055)), 2.4);
}

float2 SRGBToLinear(float2 v)
{
	return float2(SRGBToLinear(v.x), SRGBToLinear(v.y));
}

float3 SRGBToLinear(float3 v)
{
	return float3(SRGBToLinear(v.x), SRGBToLinear(v.y), SRGBToLinear(v.z));
}

float LinearToSRGB(float v)
{
	if (v <= 0.0031308)
		return 12.92 * v;
	else
		return 1.055 * pow(v, 1.0 / 2.4) - 0.055;
}

float2 LinearToSRGB(float2 v)
{
	return float2(LinearToSRGB(v.x), LinearToSRGB(v.y));
}

float3 LinearToSRGB(float3 v)
{
	return float3(LinearToSRGB(v.x), LinearToSRGB(v.y), LinearToSRGB(v.z));
}

float4 ApplyFlicker(int2 coordinate, int startThreshold, int endThreshold, float4 color)
{
	int flickerTotal = dot(constants_FlickerAxis, coordinate);
	if (flickerTotal < startThreshold)
		return float4(0, 0, 0, 0);
	else if (flickerTotal >= endThreshold)
		return color;

	return float4(1, 1, 1, 1);
}

float4 ApplyDesaturation(float desaturation, float4 color)
{
	// This is intentionally done in gamma space, and keeps solid yellow
	if (desaturation == 0.0 || all(color.rgb == float3(1.0, 1.0, 0.0)))
		return color;
	
	float grayLevel = dot(color.rgb, float3(3.0, 6.0, 1.0) / 10.0);

	color.rgb = color.rgb * (1.0 - desaturation) + float3(grayLevel, grayLevel, grayLevel) * desaturation;
	return color;
}

float3 AppleRGBToSRGBLinear(float3 color)
{
	color = pow(saturate(color), 1.8);

	float3 result;
	result = color.r * float3(1.06870538834699, 0.024110476735, 0.00173499822713);
	result += color.g * float3(-0.07859532843279, 0.96007030899244, 0.02974755969275);
	result += color.b * float3(0.00988984558395, 0.01581936633364, 0.96851741859153);

	return result;
}

float3 ApplyColorSpaceTransform(float3 color)
{
#ifdef USE_ICC_PROFILE
	return saturate(AppleRGBToSRGBLinear(color));
#else
	return SRGBToLinear(color);
#endif
}
