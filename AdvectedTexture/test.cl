
float getNoise(int x, int y)
{
	int n = x + y * 57;
	n = (n << 13) ^ n;
	return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

kernel void genNoise(global write_only float4 * dat)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		sizex = get_global_size(0),
		sizey = get_global_size(1);
	const int id = idy * sizex + idx;

	float noise = getNoise(idx, idy);
	float gradx = getNoise(idx + sizex, idy + sizey);
	float grady = getNoise(idx * sizey + idy, idy + 1024576);
	float pstep = (idx + idy) * 1.0f / (sizex + sizey);
	dat[id] = (float4)(gradx, grady, noise, 1.0f);
}

kernel void genColorful(global write_only float4 * dat)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		sizex = get_global_size(0),
		sizey = get_global_size(1);
	const int id = idy * sizex + idx;

	float pstep = (idx + idy) * 1.0f / (sizex + sizey);
	float4 tryc = (float4)(idx * 1.0f / sizex, idy * 1.0f / sizey, pstep, 1.0f);

	dat[id] = tryc;
}


float InterCosine(const float x0, const float x1, const float w)
{
	const float f = (1 - cospi(w))* 0.5;
	return mix(x0, x1, f);
}


kernel void genStepNoise(int level, global write_only float4 * dst)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		sizex = get_global_size(0),
		sizey = get_global_size(1);
	const int id = idy * sizex + idx;
	const int stp = pown(2.0f, level);

	const int rsizex = sizex / stp,
		rsizey = sizey / stp;
	float rx = 1.0f * rsizex * idx / sizex,
		ry = 1.0f * rsizey * idy / sizey;
	const int x0 = (int)rx, y0 = (int)ry;
	int x1 = x0 + 1, y1 = y0 + 1;

	rx -= x0, ry -= y0;
	const float w00 = getNoise(x0, y0),
		w10 = getNoise(x1, y0),
		w01 = getNoise(x0, y1),
		w11 = getNoise(x1, y1);
	const float w0 = mix(w00, w10, rx),
		w1 = mix(w01, w11, rx);
	const float val = mix(w0, w1, ry);

	dst[id] = (float4)(val, val, val, 1.0f);
}


kernel void genMultiNoise(int level, global write_only float4 * dst)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		sizex = get_global_size(0),
		sizey = get_global_size(1);
	const int id = idy * sizex + idx;

	//const int lsizex = get_local_size(0);
	//float4 obj = (0, 0, 0, 1);
	/*if(idx > lsizex)
	{
		dst[id] += (float4)(1, 0, 0, 1);
		return;
	}*/

	float val = 0.0f;
	int stp = 1;
	float amp = 1 / pown(2.0f, level);
	for (int a = level; a-- > 0; amp *= 2, stp *= 2)
	{
		const int rsizex = sizex / stp,
			rsizey = sizey / stp;
		float rx = 1.0f * rsizex * idx / sizex,
			ry = 1.0f * rsizey * idy / sizey;
		const int x0 = (int)rx, y0 = (int)ry;
		int x1 = x0 + 1, y1 = y0 + 1;

		const float w00 = getNoise(x0, y0),
			w10 = getNoise(x1, y0),
			w01 = getNoise(x0, y1),
			w11 = getNoise(x1, y1);
		rx -= x0, ry -= y0;
		const float w0 = mix(w00, w10, rx),
			w1 = mix(w01, w11, rx);
		val += mix(w0, w1, ry) * amp;
	}
	dst[id] = (float4)(val, val, val, 1.0f);
}



kernel void genMultiNoiseCos(int level, global write_only float4 * dst)
{
	const int idx = get_global_id(0),
		idy = get_global_id(1),
		sizex = get_global_size(0),
		sizey = get_global_size(1);
	const int id = idy * sizex + idx;

	float val = 0.0f;
	int stp = 1;
	float amp = 1 / pown(2.0f, level);
	for (int a = level; a-- > 0; amp *= 2, stp *= 2)
	{
		const int rsizex = sizex / stp,
			rsizey = sizey / stp;
		float rx = 1.0f * rsizex * idx / sizex,
			ry = 1.0f * rsizey * idy / sizey;
		const int x0 = floor(rx), y0 = floor(ry);
		int x1 = x0 + 1, y1 = y0 + 1;

		const float w00 = getNoise(x0, y0),
			w10 = getNoise(x1, y0),
			w01 = getNoise(x0, y1),
			w11 = getNoise(x1, y1);
		rx -= x0, ry -= y0;
		const float w0 = InterCosine(w00, w10, rx),
			w1 = InterCosine(w01, w11, rx);
		val += InterCosine(w0, w1, ry) * amp;
	}
	dst[id] = (float4)(val, val, val, 1.0f);
}