#ifdef __ANDROID__
	// This doesn't work on AMD Windows drivers
	#define GP_GL_SHADER_CODE_MEDIUM_PRECISION_PREFIX "precision mediump float;\n"
	#define GP_GL_SHADER_CODE_HIGH_PRECISION_PREFIX "precision highp float;\n"
#else
	#define GP_GL_SHADER_CODE_MEDIUM_PRECISION_PREFIX ""
	#define GP_GL_SHADER_CODE_HIGH_PRECISION_PREFIX	""
#endif

#define GP_GL_SHADER_CODE_FUNCTIONS_H	"vec3 pow3(vec3 v, float ex)\n"\
"{\n"\
"	return vec3(pow(v.x, ex), pow(v.y, ex), pow(v.z, ex));\n"\
"}\n"\
"\n"\
"float SRGBToLinear(float v)\n"\
"{\n"\
"	if (v <= 0.04045)\n"\
"		return v * (1.0 / 12.92);\n"\
"	else\n"\
"		return pow(((v + 0.055) * (1.0 / 1.055)), 2.4);\n"\
"}\n"\
"\n"\
"vec2 SRGBToLinear(vec2 v)\n"\
"{\n"\
"	return vec2(SRGBToLinear(v.x), SRGBToLinear(v.y));\n"\
"}\n"\
"\n"\
"vec3 SRGBToLinear(vec3 v)\n"\
"{\n"\
"	return vec3(SRGBToLinear(v.x), SRGBToLinear(v.y), SRGBToLinear(v.z));\n"\
"}\n"\
"\n"\
"float LinearToSRGB(float v)\n"\
"{\n"\
"	if (v <= 0.0031308)\n"\
"		return 12.92 * v;\n"\
"	else\n"\
"		return 1.055 * pow(v, 1.0 / 2.4) - 0.055;\n"\
"}\n"\
"\n"\
"vec2 LinearToSRGB(vec2 v)\n"\
"{\n"\
"	return vec2(LinearToSRGB(v.x), LinearToSRGB(v.y));\n"\
"}\n"\
"\n"\
"vec3 LinearToSRGB(vec3 v)\n"\
"{\n"\
"	return vec3(LinearToSRGB(v.x), LinearToSRGB(v.y), LinearToSRGB(v.z));\n"\
"}\n"\
"\n"\
"vec4 ApplyFlicker(vec2 flickerAxis, vec2 coordinate, float startThreshold, float endThreshold, vec4 color)\n"\
"{\n"\
"	float flickerTotal = dot(flickerAxis, coordinate);\n"\
"	if (flickerTotal < startThreshold)\n"\
"		return vec4(0, 0, 0, 0);\n"\
"	else if (flickerTotal >= endThreshold)\n"\
"		return vec4(color.rgb, 1);\n"\
"\n"\
"	return vec4(1, 1, 1, 1);\n"\
"}\n"\
"\n"\
"vec4 ApplyDesaturation(float desaturation, vec4 color)\n"\
"{\n"\
"	// This is intentionally done in gamma space\n"\
"	if (desaturation == 0.0 || (color.r == 1.0 && color.g == 1.0 && color.b == 0.0))\n"\
"		return color;\n"\
"\n"\
"	float grayLevel = dot(color.rgb, vec3(3.0, 6.0, 1.0) / 10.0);\n"\
"\n"\
"	color.rgb = color.rgb * (1.0 - desaturation) + vec3(grayLevel, grayLevel, grayLevel) * desaturation;\n"\
"	return color;\n"\
"}\n"\
"\n"\
"float saturate(float v)\n"\
"{\n"\
"	return min(1.0, max(0.0, v));\n"\
"}\n"\
"\n"\
"vec2 saturate(vec2 v)\n"\
"{\n"\
"	return vec2(saturate(v.x), saturate(v.y));\n"\
"}\n"\
"\n"\
"vec3 saturate(vec3 v)\n"\
"{\n"\
"	return vec3(saturate(v.x), saturate(v.y), saturate(v.z));\n"\
"}\n"\
"\n"\
"vec3 AppleRGBToSRGBLinear(vec3 color)\n"\
"{\n"\
"	color = pow3(saturate(color), 1.8);\n"\
"\n"\
"	vec3 result;\n"\
"	result = color.r * vec3(1.06870538834699, 0.024110476735, 0.00173499822713);\n"\
"	result += color.g * vec3(-0.07859532843279, 0.96007030899244, 0.02974755969275);\n"\
"	result += color.b * vec3(0.00988984558395, 0.01581936633364, 0.96851741859153);\n"\
"\n"\
"	return result;\n"\
"}\n"\
"\n"\
"vec3 ApplyColorSpaceTransform(vec3 color)\n"\
"{\n"\
"#ifdef USE_ICC_PROFILE\n"\
"	return saturate(AppleRGBToSRGBLinear(color));\n"\
"#else\n"\
"	return SRGBToLinear(color);\n"\
"#endif\n"\
"}\n"\
"\n"
