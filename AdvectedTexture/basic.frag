#version 430

const int size = 1024;

uniform sampler2D tex;

in perVert
{
	vec3 pos;
};
out vec4 FragColor;



float getNoise(int n)
{
	n = (n >> 13) ^ n;
	int nn = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
	float ans = 1.0f - (nn / 1073741824.0f);
	return ans;
}

float interLinear(float x0, float x1, float a)
{
	return mix(x0, x1, a);
}

float interSmooth(float x0, float x1, float a)
{
	return smoothstep(x0, x1, a);
}

void main() 
{
	vec2 tpos = vec2((pos.x + 1.0f)/2, (pos.y + 1.0f)/2);
	FragColor = texture(tex, tpos);
	FragColor.w = 1.0f;
}