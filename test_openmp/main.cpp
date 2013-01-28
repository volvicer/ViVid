#include "opencv2\opencv.hpp"
#ifdef _WIN32
#include "omp.h"
#else
#include "omp_unix.h"
#endif
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;
#include <immintrin.h>
#include "xmmintrin.h"
#include <malloc.h>

void cosine_filter(
	float* fr_data, float* fb_array, 
	const int height, const int width, 
	const int filter_h, const int filter_w, 
	const int n_filters, float* out_data);

static char* exampleImagePath = "..\\..\\..\\media\\kewell1.jpg";

int main(int argc, char* argv[])
{
	cv::Mat exampleImage = cv::imread(exampleImagePath, 0);

	//convert to float
	exampleImage.convertTo(exampleImage, CV_32FC1);

	//pull the data
	float* f_imData= (float*) exampleImage.data;

	const int height= exampleImage.size().height;
	const int width= exampleImage.size().width;

	//create a random filterbank
	const int num_filters = 100;
	const int filter_dim = 3;

	//float* filter_bank = _mm_malloc(num_filters * filter_dim * filter_dim*sizeof(float), 256);
	float* filter_bank = new float[num_filters * filter_dim * filter_dim];

	for (int i = 0; i < num_filters * filter_dim * filter_dim; i++)
	{
		filter_bank[i] = float( std::rand() ) / RAND_MAX;
	}

	//C Reference
	float* retvalC = new float[2 * height * width];
	cosine_filter(f_imData, filter_bank, height, width, filter_dim, filter_dim, num_filters, retvalC);

	std::ofstream test_out_c("testc.out", std::ios_base::out);
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			test_out_c << retvalC[j * width + i] << ", ";
		}

		test_out_c << std::endl;
	}

	test_out_c << std::endl << std::endl << std::endl;

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			test_out_c << retvalC[height * width + j * width + i] << ", ";
		}
		test_out_c << std::endl;
	}
	test_out_c.close();


	delete[] filter_bank;
	delete[] retvalC;

	return 0;
}

void cosine_filter(
	float* fr_data, float* fb_array, 
	const int height, const int width, 
	const int filter_h, const int filter_w, 
	const int n_filters, float* out_data)
{
	//do convolution
	const int apron_y = filter_h / 2;
	const int apron_x = filter_w / 2;

	const int filter_size = filter_h * filter_w;

	const int filter_bank_size = filter_size * n_filters;

	int *pixel_offsets=(int*) malloc(sizeof(int)*filter_size);

	int oi = 0;
	for (int ii=-apron_y; ii<=apron_y; ii++){
		for (int jj=-apron_y; jj<=apron_y; jj++){
			pixel_offsets[oi] = ii * width + jj;
			oi++;
		}
	}
	// 100 filters, each 9 values
	

	double tic = omp_get_wtime();
	for(int i=0; i<1; i++) {

		int n_threads = 1; //omp_get_num_procs();
		int valid_height = height - 2 * apron_y;
		int height_step = valid_height / n_threads + 1;

//#pragma omp parallel for
		for (int tid=0; tid<n_threads; tid++){
			int start_y = apron_y + tid * height_step;
			int end_y = min(start_y + height_step, height - apron_y);
		//	float *image_cache=(float*) malloc(sizeof(float)*filter_size);    
			__m256 image_cache[9]; // filter size is 9, data type is float

			for (int i=start_y; i<end_y; i++){
				float* fr_ptr = fr_data + i * width + apron_x;
				float* ass_out = out_data + i * width + apron_x;
				float* wgt_out = ass_out + height * width;

				for (int j=apron_x; j<(width - apron_x); j++ ){

					
					for (int ii=0; ii< filter_size; ii++){
						// copy each pixel to all elements of vector
						image_cache[ii] = _mm256_broadcast_ss(&fr_ptr[pixel_offsets[ii]]);
					} 

					float max_sim = -1e6;
					float best_ind = -1.0f;

					int fi=0;
					int filter_ind = 0;
					float temp_sum;
					// 96 filters, 9 values each
					while (fi<((n_filters/8)*8)*filter_size)
					{
						__m256 temp_sum = _mm256_set1_ps(0.0f);

						// no fused multiply add :(
						// current value of 8 filters
						__m256 curr_filter = _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[0], curr_filter), temp_sum);

						curr_filter= _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[1], curr_filter), temp_sum);

						curr_filter = _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[2], curr_filter), temp_sum);

						curr_filter= _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[3], curr_filter), temp_sum);

						curr_filter= _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[4], curr_filter), temp_sum);

						curr_filter= _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[5], curr_filter), temp_sum);

						curr_filter= _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[6], curr_filter), temp_sum);

						curr_filter= _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[7], curr_filter), temp_sum);

						curr_filter= _mm256_load_ps(&fb_array[fi]);
						fi+=8;
						temp_sum = _mm256_add_ps(_mm256_mul_ps(image_cache[8], curr_filter), temp_sum);
						// printf("response calculated\n");
						/*
						for (int ii=0; ii < filter_size; ii++){
							temp_sum += fb_array[fi++] * image_cache[ii];
						}*/
						
						//temp_sum = fabs(temp_sum);
						// calculating absolute value by clearing the last digit
						__m256 mask = _mm256_set1_ps(0x7fffffff);
						temp_sum = _mm256_and_ps(mask, temp_sum);
						// extract 128 and use SSE
						__m128 sum1 = _mm256_extractf128_ps(temp_sum, 0);
						__m128 sum2 = _mm256_extractf128_ps(temp_sum, 1);
						__m128 max_fil = _mm_load_ss(&max_sim);

						// first half 
						// if filter value greater than max
						if(_mm_comigt_ss(sum1,max_fil)) {
							_mm_move_ss(max_fil, sum1);
							best_ind = filter_ind*8+7;
						}
						// permute to second element of vector
						sum1 = _mm_permute_ps(sum1, 0x1);
						/*
						// if filter value greater than max
						if(_mm_comigt_ss(sum1,max_fil)) {
							_mm_move_ss(max_fil, sum1);
							best_ind = filter_ind*8+6;
						}
						// permute to second element of vector
						sum1 = _mm_permute_ps(sum1, 0x2);

						// if filter value greater than max
						if(_mm_comigt_ss(sum1,max_fil)) {
							_mm_move_ss(max_fil, sum1);
							best_ind = filter_ind*8+5;
						}
						// permute to second element of vector
						sum1 = _mm_permute_ps(sum1, 0x3);
						// if filter value greater than max
						if(_mm_comigt_ss(sum1,max_fil)) {
							_mm_move_ss(max_fil, sum1);
							best_ind = filter_ind*8+4;
						}
						
						// second half 
						// if filter value greater than max
						if(_mm_comigt_ss(sum2,max_fil)) {
							_mm_move_ss(max_fil, sum2);
							best_ind = filter_ind*8+3;
						}
						// permute to second element of vector
						sum2 = _mm_permute_ps(sum2, 0x1);
						
						// if filter value greater than max
						if(_mm_comigt_ss(sum2,max_fil)) {
							_mm_move_ss(max_fil, sum2);
							best_ind = filter_ind*8+2;
						}
						// permute to second element of vector
						sum2 = _mm_permute_ps(sum2, 0x2);

						// if filter value greater than max
						if(_mm_comigt_ss(sum2,max_fil)) {
							_mm_move_ss(max_fil, sum1);
							best_ind = filter_ind*8+1;
						}
						// permute to second element of vector
						sum2 = _mm_permute_ps(sum2, 0x3);
						// if filter value greater than max
						if(_mm_comigt_ss(sum2,max_fil)) {
							_mm_move_ss(max_fil, sum2);
							best_ind = filter_ind*8+0;
						}
						*/
						/*
						if (temp_sum > max_sim){
							max_sim = temp_sum;
							best_ind = filter_ind;
						}
						*/

						filter_ind += 8;
					}
					
					*ass_out = best_ind;
					*wgt_out = max_sim;

					fr_ptr++;
					ass_out++;
					wgt_out++;
				}
			}
		}
		//#pragma omp barrier
	}
	double toc = omp_get_wtime();
	std::cout << "openmp filter time: " << toc - tic << std::endl;
}