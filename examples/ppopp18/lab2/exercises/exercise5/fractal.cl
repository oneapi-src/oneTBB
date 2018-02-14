/*
    Copyright (c) 2018 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




*/

__constant int redChannelOffset = 0;
__constant int greenChannelOffset = 1;
__constant int blueChannelOffset = 2;
__constant int channelsPerPixel = 4;

uchar calc_one_pixel(float x0, float y0, int max_iterations, int size_x, int size_y, float magn, float cx, float cy, int gpu)
{
	int iter;
	float fx0, fy0, xtemp, x, y, mu;

	uchar color;

	fx0 = x0 - size_x / 2.0f;
	fy0 = y0 - size_y / 2.0f;
	fx0 = fx0 / magn + cx;
	fy0 = fy0 / magn + cy;

	iter = 0; x = 0; y = 0;

	while ( ((x*x + y*y) <= 32) && (iter < max_iterations) )  {
		xtemp = x*x - y*y + fx0;
		y = 2 * x*y + fy0;
		x = xtemp;
		iter++;
	}

	if ( iter == max_iterations ) {
		// point corresponds to the Mandelbrot set
		return 0;
	}

	// compute again but with exponent calculation at each iteration
	// it's all for coloring point outside the Mandelbrot set
	iter = 0; x = 0; y = 0;
	mu = 0;

	while ( ((x*x + y*y) <= 32) && (iter < max_iterations) ) {
		xtemp = x*x - y*y + fx0;
		y = 2 * x*y + fy0;
		x = xtemp;
		mu += exp(-sqrt(x*x + y*y));
		iter++;
	}

	const uint b = convert_uint_sat(256 * mu);
	const uint g = (b / 8);
	const uint r = (g / 16);

    const uchar gray = convert_uchar_sat(0.21f * r + 0.72f * g + 0.07f * b);
    return gray;
}

__kernel void mono_mandelbrot( __global uchar* fractal_data_array, uint x0, uint x1, uint y0, uint y1, uint max_iterations, uint size_x, uint size_y, float magn, float cx, float cy) {
	const int x = get_global_id(0) + x0;
	const int y = get_global_id(1) + y0;
    
    const int pixelIndex = channelsPerPixel * (x1 - x0) * (y - y0) + channelsPerPixel * (x - x0);
    const int pixelRedChannelIndex = pixelIndex + redChannelOffset;
    const int pixelGreenChannelIndex = pixelIndex + greenChannelOffset;
    const int pixelBlueChannelIndex = pixelIndex + blueChannelOffset;

    fractal_data_array[pixelRedChannelIndex] = fractal_data_array[pixelGreenChannelIndex] = fractal_data_array[pixelBlueChannelIndex] = calc_one_pixel(x, y, max_iterations, size_x, size_y, magn, cx, cy, 255);
    fractal_data_array[pixelIndex + 3] = 255;
}
