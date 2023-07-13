float N21(float idx, float idy) {
	float v = (sin(idx * 100.0 + idy * 6574.0) + 1.0) * 5674;
	v = v - floor(v);
	return v;
}
float SmoothNoise(float cx, float cy) {
	float scaleFactor = 5.;
	float lvx = (cx * scaleFactor) - floor(cx * scaleFactor);
	float lvy = (cy * scaleFactor) - floor(cy * scaleFactor);

	float idx = floor(cx * scaleFactor);
	float idy = floor(cy * scaleFactor);

	lvx = lvx * lvx * (3. - 2. * lvx);
	lvy = lvy * lvy * (3. - 2. * lvy);
	//lvx = lvx * lvx * (3. - 3. * lvx);
	//lvy = lvy * lvy * (3. - 3. * lvy);

	float bl = N21(idx, idy);
	float br = N21(idx + 1., idy);
	float b = mix(bl, br, lvx);

	float tl = N21(idx, idy + 1.);
	float tr = N21(idx + 1., idy + 1.);
	float t = mix(tl, tr, lvx);
	
	return mix(b, t, lvy);
}
uchar3 cyanToMagPaletteDefault(float v) {
	uchar3 col = (uchar3)(255 * v, 255 * (1 - v), 255);
	return col;
}
uchar3 cyanToMagPalette(float v) {
	float3 a = (float3)(0.5f, 0.5f, 0.5f);
	float3 b = (float3)(0.5f, 0.5f, 0.5f);
	float3 c = (float3)(0.5f, 0.5f, 0.5f);
	float3 d = (float3)(0.5, 0.17, 0.83);

	float3 colf = a + b * cos(6.2831853f * (c * v + d));
	colf = colf * 255.f;
	uchar3 col = (uchar3)((uchar)colf.x, (uchar)colf.y, (uchar)colf.z);
	return col;
}
uchar3 brownToWhitePalette(float v) {
	float3 a = (float3)(0.5f, 0.5f, 0.5f);
	float3 b = (float3)(0.5f, 0.5f, 0.5f);
	float3 c = (float3)(1.0f, 1.0f, 1.0f);
	float3 d = (float3)(0.00, 0.10, 0.20);
	
	float3 colf = a + b * cos(6.2831853f * (c * v + d));
	colf = colf * 255.f;
	uchar3 col = (uchar3) ((uchar)colf.x, (uchar)colf.y, (uchar)colf.z);
	return col;
}
uchar3 cyanMagYelPalette(float v) {
	float3 a = (float3)(0.5f, 0.5f, 0.5f);
	float3 b = (float3)(0.5f, 0.5f, 0.5f);
	float3 c = (float3)(2.0, 1.0, 0.0);
	float3 d = (float3)(0.50, 0.20, 0.25);

	float3 colf = a + b * cos(6.2831853f * (c * v + d));
	colf = colf * 255.f;
	uchar3 col = (uchar3)((uchar)colf.x, (uchar)colf.y, (uchar)colf.z);
	return col;
}
uchar3 pinkLimePalette(float v) {
	float3 a = (float3)(0.8, 0.5, 0.4);
	float3 b = (float3)(0.2, 0.4, 0.2);
	float3 c = (float3)(2.0, 1.0, 1.0);
	float3 d = (float3)(0.00, 0.25, 0.25);

	float3 colf = a + b * cos(6.2831853f * (c * v + d));
	colf = colf * 255.f;
	uchar3 col = (uchar3)((uchar)colf.x, (uchar)colf.y, (uchar)colf.z);
	return col;
}
uchar3 blueGoldPalette(float v) {
	float3 a = (float3)(0.5f, 0.5f, 0.5f);
	float3 b = (float3)(0.5f, 0.5f, 0.5f);
	float3 c = (float3)(1.0, 0.7, 0.4);
	float3 d = (float3)(0.00, 0.15, 0.20);

	float3 colf = a + b * cos(6.2831853f * (c * v + d));
	colf = colf * 255.f;
	uchar3 col = (uchar3)((uchar)colf.x, (uchar)colf.y, (uchar)colf.z);
	return col;
}
uchar3 rainbowPalette(float v) {
	float3 a = (float3)(0.5f, 0.5f, 0.5f);
	float3 b = (float3)(0.5f, 0.5f, 0.5f);
	float3 c = (float3)(1.0, 1.0, 1.0);
	float3 d = (float3)(0.00, 0.33, 0.67);

	float3 colf = a + b * cos(6.2831853f * (c * v + d));
	colf = colf * 255.f;
	uchar3 col = (uchar3)((uchar)colf.x, (uchar)colf.y, (uchar)colf.z);
	return col;
}
uchar3 testPalette(float v) {
	float3 a = (float3)(0.5f, 0.5f, 0.5f);
	float3 b = (float3)(0.5f, 0.5f, 0.5f);
	float3 c = (float3)(0.5f, 0.5f, 0.5f);
	float3 d = (float3)(0.5, 0.17, 0.83);

	float3 colf = a + b * cos(6.2831853f * (c * v + d));
	colf = colf * 255.f;
	uchar3 col = (uchar3)((uchar)colf.x, (uchar)colf.y, (uchar)colf.z);
	return col;
}

uchar3 paletteCaller(int maxIter, int i, int colorDiv, int colorPaletteNum) {
	uchar3 col = (uchar3)(0, 0, 0);
	int iNorm = 0;
	if (i < maxIter) {
		iNorm = i % (colorDiv * 2);
		iNorm = iNorm < colorDiv ? iNorm : 2 * colorDiv - iNorm;

		switch (colorPaletteNum)
		{
			case 0:
				col = cyanToMagPaletteDefault(iNorm/ (float)colorDiv);
				break;
			case 1:
				col = cyanToMagPalette(iNorm/ (float)colorDiv);
				break;
			case 2:
				col = brownToWhitePalette(iNorm/ (float)colorDiv);
				break;
			case 3:
				col = cyanMagYelPalette(iNorm/ (float)colorDiv);
				break;
			case 4:
				col = pinkLimePalette(iNorm/ (float)colorDiv);
				break;
			case 5:
				col = blueGoldPalette(iNorm/ (float)colorDiv);
				break;
			case 6:
				col = rainbowPalette(iNorm/ (float)colorDiv);
				break;
			case 7:
				col = testPalette(iNorm/ (float)colorDiv);
				break;
			default:
				break;
		}
	}
	return col;
}

__kernel void fractalSet(__global const double* input_X,
                        __global const double* input_Y,
                        __global const int* options,
						__global const double* mouseCords,
						__global uchar* output
						)
{
	int indx = get_global_id(0);

	int maxIter = options[0];
	int size_X = options[1];
	int colorDiv = options[2];
	int juliaMode = options[3];
	int textureMode = options[4];
	int powAInt = options[5];
	float powA = *(float*) &powAInt;
	int colorPaletteNum = options[6];

	int indx_X = indx % size_X;
	int indx_Y = indx / size_X;

	int i = maxIter;

	const double threshold = 5.0;		
	double x;
	double y;
	double nx;
	double ny;
	double cx;
	double cy;
	double x2;
	double y2;

		
	if (juliaMode) {
		x = input_X[indx_X];
		y = input_Y[indx_Y];
		nx = 0.0;
		cx = mouseCords[0];
		cy = mouseCords[1];
	} else {
		x = 0.0;
		y = 0.0;
		nx = 0.0;
		cx = input_X[indx_X];
		cy = input_Y[indx_Y];
	}
	if (textureMode == 1) {											// mandel set
		while(i-- && ((x2 = x*x) + (y2 = y*y) <= threshold)) {
			nx = (x2 - y2) + cx;
			y = (2 * x * y) + cy;
			x = nx;
		}
	} else if (textureMode == 2) {									// burning ship
		while(i-- && ((x2 = x*x) + (y2 = y*y) <= threshold)) {
			nx = (x2 - y2) + cx;
			y = fabs(2.0 * x * y) + cy;
			x = nx;
		}
	} else if (textureMode == 3) {									// mandel with adjustable variable
		while(i-- && ((x2 = x*x) + (y2 = y*y) <= threshold)) {
			nx = (x2 - y2) + cx;
			y = (powA * x * y) + cy;
			x = nx;
		}
	} else if (textureMode == 4) {									// spinner
		while (i-- && ((x2 = x * x) + (y2 = y * y) <= threshold)) {
			nx = (x2 - y2) + cx;
			x = (2.0 * y * x) + cy;
			y = nx;
		}
	} else if (textureMode == 5) {									// random noise
		float c = SmoothNoise(cx, cy);
		c += SmoothNoise(cx * 2., cy * 2.) * .5;
		c += SmoothNoise(cx * 4., cy * 4.) * .25;
		c += SmoothNoise(cx * 8., cy * 8.) * .125;
		c += SmoothNoise(cx * 16., cy * 16.) * .0625;

		c /= 2.0;
		i = (int)(c * maxIter);

	} else if (textureMode == 6) {									// metlaxi
		double vx = (sin(cx) + 1.0) / 2.0;
		double vy = (sin(cy) + 1.0) / 2.0;
		i = (int)(sqrt((vx * vx  + vy * vy) * .5) * maxIter);
	} else if (textureMode == 9) {									// mandel old
		while(i-- && (x*x + y*y <= 5)) {
			nx = (x*x - y*y) + cx;
			y = (2 * x * y) + cy;
			x = nx;
		}
	}
	
	i = maxIter - i - 1;
	uchar3 col = paletteCaller(maxIter, i, colorDiv, colorPaletteNum);

	output[4 * indx + 0] = col.x;
	output[4 * indx + 1] = col.y;
	output[4 * indx + 2] = col.z;
	output[4 * indx + 3] = 255;
}