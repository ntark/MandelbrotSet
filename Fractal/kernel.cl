float N21(float idx, float idy) {
	float v = (sin(idx * 100.0 + idy * 6574.0) + 1.0) * 5674;
	v = v - floor(v);
	return v;
}
float SmoothNoise(float cx, float cy) {
	float lvx = (cx * 10.) - floor(cx * 10.);
	float lvy = (cy * 10.) - floor(cy * 10.);

	float idx = floor(cx * 10.);
	float idy = floor(cy * 10.);

	lvx = lvx * lvx * (3. - 2. * lvx);
	lvy = lvy * lvy * (3. - 2. * lvy);

	float bl = N21(idx, idy);
	float br = N21(idx + 1., idy);
	float b = mix(bl, br, lvx);

	float tl = N21(idx, idy + 1.);
	float tr = N21(idx + 1., idy + 1.);
	float t = mix(tl, tr, lvx);
	
	return mix(b, t, lvy);
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

	i = maxIter - i;
	int iNorm = 0;
	if (i > maxIter) {
		output[4 * indx] = 0;
		output[4 * indx + 1] = 0;
		output[4 * indx + 2] = 0;
	} else {
		int scaledI = i * colorDiv;
		iNorm = (int)scaledI % 511;
		iNorm = iNorm <= 255 ? iNorm : (511 - iNorm);
		output[4 * indx + 0] = iNorm;
		output[4 * indx + 1] = 255 - iNorm;
		output[4 * indx + 2] = 255;			
	}
	output[4 * indx + 3] = 255;
}