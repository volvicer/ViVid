#include "FlexibleFilter.hpp"
#include "FlexibleFilterLocal.hpp"

#include <omp.h>

#include <cuda_runtime.h>

#define PY_ARRAY_UNIQUE_SYMBOL tb
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#define MAX_FILTERBANK_SIZE 10000
#define N_MAX_
#define N_MAX_CHANNELS 10

int update_filter_bank_internal_cl(float* new_filter, int filter_size){
		
    if (filter_size > MAX_FILTERBANK_SIZE){
        printf("ERROR: Filterbank too large\n");
        return 1;
    }
    else {
        //std::cout << "Loading the filterbank" << std::endl;
        //printf("Value in:%05f\n",new_filter[0]);
		
		TheContext* tc = new TheContext();
		cl_context GPUContext = tc->getMyContext()->getContextCL();
		cl_device_id cdDevice = tc->getMyContext()->getDeviceCL();
		
		MyKernels *kernels = new MyKernels(GPUContext,cdDevice);
		
		cl_int err;
		
		cl_mem filter_mem =  clCreateBuffer(GPUContext, CL_MEM_READ_ONLY, sizeof(float) * filter_size,     
											NULL, &err);

		err |= clEnqueueWriteBuffer(tc->getMyContext()->cqCommandQueue, filter_mem, CL_TRUE, 0, 
									  sizeof(float) * filter_size, new_filter, 0, NULL,  NULL);
		
        if (err != 0)
            std::cout << "Error loading the filterbank: CL error: " << err << std::endl;

		kernels->getMyKernels()->c_FilterBank=filter_mem;
		
        return 0;
    }
	
}

int set_filter_bank_cuda(float* filter_bank, int size){
    return update_filter_bank_internal(filter_bank,size); 
}

int set_filter_bank_cl(float* filter_bank, int size){
    return update_filter_bank_internal_cl(filter_bank,size); 
}

DeviceMatrixCL3D::Ptr filter_frame_cl_3_batch(const boost::python::object& npy_array,
        const int dim_t, const int nchannels, const int optype)
{
    PyObject* contig
        = PyArray_FromAny(npy_array.ptr(), PyArray_DescrFromType(PyArray_FLOAT),
                      3, 3, NPY_CARRAY, NULL);
    boost::python::handle<> temp(contig);
    boost::python::object arr(temp);

    const int d1 = PyArray_DIM(npy_array.ptr(), 0);
    std::cout << d1 << std::endl;
    const int d2 = PyArray_DIM(npy_array.ptr(), 1);
    std::cout << d2 << std::endl;
    const int d3 = PyArray_DIM(npy_array.ptr(), 2);
    std::cout << d3 << std::endl;

    DeviceMatrixCL3D::Ptr out = makeDeviceMatrixCL3D(2,100,100);
    std::cout << "Everything is fine" << std::endl;

    return out;
}

DeviceMatrix3D::Ptr filter_frame_cuda_3(const DeviceMatrix::Ptr& frame,
                                    const int dim_t, const int nchannels,
                                    const int optype){

    DeviceMatrix3D::Ptr out = makeDeviceMatrix3D(2, frame->height, frame->width / nchannels);

    dist_filter2_d3(frame.get(), dim_t, nchannels, out.get(), optype);
    return out;
}

DeviceMatrix3D::Ptr filter_frame_cuda_5(const DeviceMatrix::Ptr& frame,
                                    const int dim_t, const int nchannels,
                                    const int optype){

    DeviceMatrix3D::Ptr out = makeDeviceMatrix3D(2, frame->height, frame->width / nchannels);

    dist_filter2_d5(frame.get(), dim_t, nchannels, out.get(), optype);
    return out;
}

DeviceMatrix3D::Ptr filter_frame_cuda_7(const DeviceMatrix::Ptr& frame,
                                    const int dim_t, const int nchannels,
                                    const int optype){

    DeviceMatrix3D::Ptr out = makeDeviceMatrix3D(2, frame->height, frame->width / nchannels);
    dist_filter2_d7(frame.get(), dim_t, nchannels, out.get(), optype);

    return out;
}
DeviceMatrix3D::Ptr filter_frame_cuda_noargmin(const DeviceMatrix::Ptr& frame,
                                    const int dim_t, const int dim_y, const int dim_x, const int nchannels,
                                    const int optype){

    DeviceMatrix3D::Ptr out = makeDeviceMatrix3D(frame->height, frame->width / nchannels, dim_t);
    
    dist_filter_noargmin(frame.get(), dim_t, dim_y, dim_x, nchannels, out.get(), optype);

    return out;
}

DeviceMatrix3D::Ptr get_cell_histograms_cuda(const DeviceMatrix3D::Ptr& inds_and_weights,
                                             const int cell_size,
                                             const int offset_y, const int offset_x,
                                             const int n_bins){
#ifndef CUDA_NO_SM_11_ATOMIC_INTRINSICS
//	printf("WARNING! Not using atomics!\n");
#endif

    int frame_height = inds_and_weights->dim_y;
    int frame_width = inds_and_weights->dim_x;

    int n_cells_y = ( frame_height - offset_y ) / cell_size;
    int n_cells_x = ( frame_width - offset_x ) / cell_size;

    DeviceMatrix3D::Ptr out = makeDeviceMatrix3D(n_cells_y, n_cells_x, n_bins);
    out->zero();

    hist_all_cells(inds_and_weights.get(), out.get(), cell_size, offset_y, offset_x, n_bins);

    return out;
}

/* OPENCL FUNCTIONS */

DeviceMatrixCL3D::Ptr filter_frame_cl_3(const DeviceMatrixCL::Ptr& frame,
										const int dim_t, const int nchannels,
										const int optype){
	
    DeviceMatrixCL3D::Ptr out = makeDeviceMatrixCL3D(2, frame->height, frame->width / nchannels);
	
    dist_filter2_d3_cl(frame.get(), dim_t, nchannels, out.get(), optype);
    return out;
}



DeviceMatrixCL3D::Ptr filter_frame_cl_5(const DeviceMatrixCL::Ptr& frame,
										const int dim_t, const int nchannels,
										const int optype){
	
    DeviceMatrixCL3D::Ptr out = makeDeviceMatrixCL3D(2, frame->height, frame->width / nchannels);
	
    dist_filter2_d5_cl(frame.get(), dim_t, nchannels, out.get(), optype);
    return out;
}

DeviceMatrixCL3D::Ptr filter_frame_cl_7(const DeviceMatrixCL::Ptr& frame,
										const int dim_t, const int nchannels,
										const int optype){
	
    DeviceMatrixCL3D::Ptr out = makeDeviceMatrixCL3D(2, frame->height, frame->width / nchannels);
    dist_filter2_d7_cl(frame.get(), dim_t, nchannels, out.get(), optype);
	
    return out;
}

DeviceMatrixCL3D::Ptr filter_frame_cl_noargmin(const DeviceMatrixCL::Ptr& frame,
											   const int dim_t, const int dim_y, const int dim_x, const int nchannels,
											   const int optype){
	
    DeviceMatrixCL3D::Ptr out = makeDeviceMatrixCL3D(frame->height, frame->width / nchannels, dim_t);
    
    dist_filter_noargmin_cl(frame.get(), dim_t, dim_y, dim_x, nchannels, out.get(), optype);
	
    return out;
}

DeviceMatrixCL3D::Ptr get_cell_histograms_cl(const DeviceMatrixCL3D::Ptr& inds_and_weights,
                                             const int cell_size,
                                             const int offset_y, const int offset_x,
                                             const int n_bins){
#ifndef CUDA_NO_SM_11_ATOMIC_INTRINSICS
	//	printf("WARNING! Not using atomics!\n");
#endif
	
    int frame_height = inds_and_weights->dim_y;
    int frame_width = inds_and_weights->dim_x;
	
    int n_cells_y = ( frame_height - offset_y ) / cell_size;
    int n_cells_x = ( frame_width - offset_x ) / cell_size;
	
    DeviceMatrixCL3D::Ptr out = makeDeviceMatrixCL3D(n_cells_y, n_cells_x, n_bins);
    out->zero();
	
    hist_all_cells_cl(inds_and_weights.get(), out.get(), cell_size, offset_y, offset_x, n_bins);
	
    return out;
}


#define BLOCK_MULT 2

/* AUXILIARY OPENCL FUNCTIONS THAT WERE DEFINED IN CL FILE */

/* OPENCL */

cl_int parameters_blockwise_distance_kernel(
        cl_kernel theKernel,const DeviceMatrixCL* matrix,
        DeviceMatrixCL3D* output,
        const int frame_width, const int frame_height, 
        const int FD, const int optype, 
        cl_mem filter, const int n_filters){
	cl_int err=0;
	
    err |= clSetKernelArg(theKernel, 0, sizeof (cl_mem), &matrix->dataMatrix);
	err |= clSetKernelArg(theKernel, 1, sizeof (int), &matrix->width);
    err |= clSetKernelArg(theKernel, 2, sizeof (int), &matrix->height);
    err |= clSetKernelArg(theKernel, 3, sizeof (int), &matrix->pitch);
	err |= clSetKernelArg(theKernel, 4, sizeof (cl_mem), &output->dataMatrix);
    err |= clSetKernelArg(theKernel, 5, sizeof (int), &output->dim_x);
    err |= clSetKernelArg(theKernel, 6, sizeof (int), &output->dim_y);
    err |= clSetKernelArg(theKernel, 7, sizeof (int), &output->dim_t);
	err |= clSetKernelArg(theKernel, 8, sizeof (int), &output->pitch_y);
    err |= clSetKernelArg(theKernel, 9, sizeof (int), &output->pitch_t);
	err |= clSetKernelArg(theKernel, 10, sizeof (const int), &frame_width);
	err |= clSetKernelArg(theKernel, 11, sizeof (const int), &frame_height);
	err |= clSetKernelArg(theKernel, 12, sizeof (const int), &FD);
	
	const int BM = BLOCK_MULT;
	const int BS = BLOCK_SIZE;
	err |= clSetKernelArg(theKernel, 13, sizeof (const int), &BM);
	err |= clSetKernelArg(theKernel, 14, sizeof (const int), &BS);
	err |= clSetKernelArg(theKernel, 15, sizeof (const int), &optype);
	err |= clSetKernelArg(theKernel, 16, sizeof (cl_mem), &filter);
	err |= clSetKernelArg(theKernel, 17, sizeof (const int), &n_filters);
	return err;
}

cl_int parameters_blockwise_filter_kernel(cl_kernel theKernel,const DeviceMatrixCL* matrix,
				DeviceMatrixCL3D* output,const int frame_width,const int frame_height,
				const int apron_lo_y, const int apron_lo_x,
				const int apron_hi_y, const int apron_hi_x,
				const int nchannels,const int optype,cl_mem filter){
	cl_int err=0;
	
    err |= clSetKernelArg(theKernel, 0, sizeof (cl_mem), &matrix->dataMatrix);
	err |= clSetKernelArg(theKernel, 1, sizeof (int), &matrix->width);
    err |= clSetKernelArg(theKernel, 2, sizeof (int), &matrix->height);
    err |= clSetKernelArg(theKernel, 3, sizeof (int), &matrix->pitch);
	err |= clSetKernelArg(theKernel, 4, sizeof (cl_mem), &output->dataMatrix);
    err |= clSetKernelArg(theKernel, 5, sizeof (int), &output->dim_x);
    err |= clSetKernelArg(theKernel, 6, sizeof (int), &output->dim_y);
    err |= clSetKernelArg(theKernel, 7, sizeof (int), &output->dim_t);
	err |= clSetKernelArg(theKernel, 8, sizeof (int), &output->pitch_y);
    err |= clSetKernelArg(theKernel, 9, sizeof (int), &output->pitch_t);
	err |= clSetKernelArg(theKernel, 10, sizeof (const int), &frame_width);
	err |= clSetKernelArg(theKernel, 11, sizeof (const int), &frame_height);
	err |= clSetKernelArg(theKernel, 12, sizeof (const int), &apron_lo_y);
	err |= clSetKernelArg(theKernel, 13, sizeof (const int), &apron_lo_x);
	err |= clSetKernelArg(theKernel, 14, sizeof (const int), &apron_hi_y);
	err |= clSetKernelArg(theKernel, 15, sizeof (const int), &apron_hi_x);
	err |= clSetKernelArg(theKernel, 16, sizeof (const int), &nchannels);
	
	const int BS = BLOCK_SIZE;
	err |= clSetKernelArg(theKernel, 17, sizeof (const int), &BS);
	err |= clSetKernelArg(theKernel, 18, sizeof (const int), &optype);
	err |= clSetKernelArg(theKernel, 19, sizeof (cl_mem), &filter);
	
	return err;
}

cl_int parameters_histogram(cl_kernel theKernel,const DeviceMatrixCL3D* matrix,
										  DeviceMatrixCL3D* output,const int cell_size,const int offset_y,
										  const int offset_x, const int max_bin){
	cl_int err=0;
	
    err |= clSetKernelArg(theKernel, 0, sizeof (cl_mem), &matrix->dataMatrix);
	err |= clSetKernelArg(theKernel, 1, sizeof (int), &matrix->dim_x);
    err |= clSetKernelArg(theKernel, 2, sizeof (int), &matrix->dim_y);
	err |= clSetKernelArg(theKernel, 3, sizeof (int), &matrix->dim_t);
    err |= clSetKernelArg(theKernel, 4, sizeof (int), &matrix->pitch_y);
	err |= clSetKernelArg(theKernel, 5, sizeof (int), &matrix->pitch_t);
	err |= clSetKernelArg(theKernel, 6, sizeof (cl_mem), &output->dataMatrix);
    err |= clSetKernelArg(theKernel, 7, sizeof (int), &output->dim_x);
    err |= clSetKernelArg(theKernel, 8, sizeof (int), &output->dim_y);
    err |= clSetKernelArg(theKernel, 9, sizeof (int), &output->dim_t);
	err |= clSetKernelArg(theKernel, 10, sizeof (int), &output->pitch_y);
    err |= clSetKernelArg(theKernel, 11, sizeof (int), &output->pitch_t);
	err |= clSetKernelArg(theKernel, 12, sizeof (const int), &cell_size);
	err |= clSetKernelArg(theKernel, 13, sizeof (const int), &offset_y);
	err |= clSetKernelArg(theKernel, 14, sizeof (const int), &offset_x);
	err |= clSetKernelArg(theKernel, 15, sizeof (const int), &max_bin);

	const int BS = BLOCK_SIZE;
	const int B8 = BLOCK_8;
	err |= clSetKernelArg(theKernel, 16, sizeof (const int), &B8);
	err |= clSetKernelArg(theKernel, 17, sizeof (const int), &BS);

	
	return err;
}

void dist_filter2_d3_cl(const DeviceMatrixCL* frame,
						const int dim_t, const int nchannels,
						DeviceMatrixCL3D* output,
						const int optype)
{
	const int frame_width = int(frame->width);
	const int frame_height = int(frame->height);
	
	const int valid_region_h = frame_height - 3 + 1;
	const int valid_region_w = frame_width - 3 + 1;
	
	const size_t local_work_size[2] = {BLOCK_SIZE, BLOCK_SIZE}; 
	const int n_blocks_x = (valid_region_h / (BLOCK_SIZE * BLOCK_MULT) + 1)* local_work_size[0];
	const int n_blocks_y = (valid_region_w / (BLOCK_SIZE * BLOCK_MULT) + 1)* local_work_size[1];	
	const size_t global_work_size[2] = {n_blocks_x, n_blocks_y};
	
	TheContext* tc = new TheContext();
	
    cl_context GPUContext = tc->getMyContext()->getContextCL();
    cl_device_id cdDevice = tc->getMyContext()->getDeviceCL();
	
    // Creates the program
    // Uses NVIDIA helper functions to get the code string and it's size (in bytes)
  	
	MyKernels *kernels = new MyKernels(GPUContext,cdDevice);
	
	cl_kernel theKernel= kernels->getBlockWiseDistanceKernel();
	
	cl_int err;
	err=0;
	
    err =  parameters_blockwise_distance_kernel(theKernel, frame, output,
												frame_width,frame_height,3,optype,
												kernels->getMyKernels()->c_FilterBank,
                                                dim_t);	
  	
    if (err != CL_SUCCESS) {
        printf("Error: Failed to set kernel arguments 3! %d\n", err);
        exit(1);
    }
	
	
	err = clEnqueueNDRangeKernel(tc->getMyContext()->cqCommandQueue, 
								 theKernel, 2, NULL, 
								 global_work_size, local_work_size, 0, NULL, NULL);
	
    if (err) {
        printf("Error: Failed to execute kernel! %d\n", err);
        exit(1);
    }
}

void dist_filter2_d5_cl(const DeviceMatrixCL* frame,
					 const int dim_t, const int nchannels,
					 DeviceMatrixCL3D* output,
					 const int optype)
{
 /*   const int frame_width = int(frame->width);
    const int frame_height = int(frame->height);
	
    const int valid_region_h = frame_height - 5 + 1;
    const int valid_region_w = frame_width - 5 + 1;
	
    int grid_ry = valid_region_h / (BLOCK_SIZE * BLOCK_MULT) + 1;
    int grid_cx = valid_region_w / (BLOCK_SIZE * BLOCK_MULT) + 1;
	
    dim3 dimBlock(BLOCK_SIZE, BLOCK_SIZE);
	
    dim3 dimGrid(grid_cx, grid_ry);
	
    blockwise_distance_kernel<5><<<dimGrid, dimBlock>>>(*frame,
														*output,
														frame_width, frame_height,
														dim_t,
														optype);*/
	const int frame_width = int(frame->width);
	const int frame_height = int(frame->height);
	
	const int valid_region_h = frame_height - 5 + 1;
	const int valid_region_w = frame_width - 5 + 1;
	
	const size_t local_work_size[2] = {BLOCK_SIZE, BLOCK_SIZE}; 
	
	
	const int n_blocks_x = (valid_region_h / (BLOCK_SIZE * BLOCK_MULT) + 1)* local_work_size[0];
	
	const int n_blocks_y = (valid_region_w / (BLOCK_SIZE * BLOCK_MULT) + 1)* local_work_size[1];
    
    const size_t global_work_size[2] = {n_blocks_x, n_blocks_y};
	
	TheContext* tc = new TheContext();
	
    cl_context GPUContext = tc->getMyContext()->getContextCL();
    cl_device_id cdDevice = tc->getMyContext()->getDeviceCL();
	
    // Creates the program
    // Uses NVIDIA helper functions to get the code string and it's size (in bytes)
  	
	MyKernels *kernels = new MyKernels(GPUContext,cdDevice);
	
	cl_kernel theKernel= kernels->getBlockWiseDistanceKernel();
	
	cl_int err;
	err=0;
	
	
    err =  parameters_blockwise_distance_kernel(theKernel, frame, output,
												frame_width,frame_height,5,optype,
												kernels->getMyKernels()->c_FilterBank,
                                                dim_t);	
  	
    if (err != CL_SUCCESS) {
        printf("Error: Failed to set kernel arguments 3! %d\n", err);
        exit(1);
    }
	
	
	err = clEnqueueNDRangeKernel(tc->getMyContext()->cqCommandQueue, 
								 theKernel, 2, NULL, 
								 global_work_size, local_work_size, 0, NULL, NULL);
	
    if (err) {
        printf("Error: Failed to execute kernel! %d\n", err);
        exit(1);
    }
	
}

void dist_filter2_d7_cl(const DeviceMatrixCL* frame,
					 const int dim_t, const int nchannels,
					 DeviceMatrixCL3D* output,
					 const int optype)
{
	const int frame_width = int(frame->width);
	const int frame_height = int(frame->height);
	
	const int valid_region_h = frame_height - 7 + 1;
	const int valid_region_w = frame_width - 7 + 1;
	
	const size_t local_work_size[2] = {BLOCK_SIZE, BLOCK_SIZE}; 
	
	
	const int n_blocks_x = (valid_region_h / (BLOCK_SIZE * BLOCK_MULT) + 1)* local_work_size[0];
	
	const int n_blocks_y = (valid_region_w / (BLOCK_SIZE * BLOCK_MULT) + 1)* local_work_size[1];
    
    const size_t global_work_size[2] = {n_blocks_x, n_blocks_y};
	
	TheContext* tc = new TheContext();
	
    cl_context GPUContext = tc->getMyContext()->getContextCL();
    cl_device_id cdDevice = tc->getMyContext()->getDeviceCL();
	
    // Creates the program
    // Uses NVIDIA helper functions to get the code string and it's size (in bytes)
	MyKernels *kernels = new MyKernels(GPUContext,cdDevice);
	
	cl_kernel theKernel= kernels->getBlockWiseDistanceKernel();
	
	cl_int err;
	err=0;
	
    err =  parameters_blockwise_distance_kernel(theKernel, frame, output,
												frame_width,frame_height,7,optype,
												kernels->getMyKernels()->c_FilterBank,
                                                dim_t);	
  	
    if (err != CL_SUCCESS) {
        printf("Error: Failed to set kernel arguments 3! %d\n", err);
        exit(1);
    }
	
	
	err = clEnqueueNDRangeKernel(tc->getMyContext()->cqCommandQueue, 
								 theKernel, 2, NULL, 
								 global_work_size, local_work_size, 0, NULL, NULL);
	
    if (err) {
        printf("Error: Failed to execute kernel! %d\n", err);
        exit(1);
    }
}

//new dist filter implementation
void dist_filter_noargmin_cl(const DeviceMatrixCL* frame,
						  const int dim_t, const int dim_y, const int dim_x, const int nchannels,
						  DeviceMatrixCL3D* output,
						  const int optype)
{

	
	const int frame_width = float(frame->width) / (nchannels);
    const int frame_height = float(frame->height);
	
    const int apron_hi_y = dim_y / 2;
    const int apron_hi_x = dim_x / 2;
	
    const int apron_lo_y = dim_y - apron_hi_y - 1;
    const int apron_lo_x = dim_x - apron_hi_x - 1;
	
	const size_t local_work_size[2] = {BLOCK_SIZE, BLOCK_SIZE}; 
	
	
	const int n_blocks_x = ((frame_height) / (local_work_size[0]-dim_y+1) + 1)* local_work_size[0];
	
	const int n_blocks_y = ((frame_width ) / (local_work_size[1]-dim_x+1) + 1)* local_work_size[1];
    
    const size_t global_work_size[2] = {n_blocks_x, n_blocks_y};
	
	
	
	TheContext* tc = new TheContext();
	
    cl_context GPUContext = tc->getMyContext()->getContextCL();
    cl_device_id cdDevice = tc->getMyContext()->getDeviceCL();
	
    // Creates the program
    // Uses NVIDIA helper functions to get the code string and it's size (in bytes)
  	
	MyKernels *kernels = new MyKernels(GPUContext,cdDevice);
	
	cl_kernel theKernel= kernels->getBlockWiseFilterKernel();
	
	cl_int err;
	err=0;
	
    err =  parameters_blockwise_filter_kernel(theKernel, frame, output,frame_width,
											  frame_height,apron_lo_y,apron_lo_x,
											  apron_hi_y,apron_hi_x,nchannels,optype,
											  kernels->getMyKernels()->c_FilterBank);
	
  	
    if (err != CL_SUCCESS) {
        printf("Error: Failed to set kernel arguments 3! %d\n", err);
        exit(1);
    }
	
	
	err = clEnqueueNDRangeKernel(tc->getMyContext()->cqCommandQueue, 
								 theKernel, 2, NULL, 
								 global_work_size, local_work_size, 0, NULL, NULL);
	
    if (err) {
        printf("Error: Failed to execute kernel! %d\n", err);
        exit(1);
    }
}

void hist_all_cells_cl(const DeviceMatrixCL3D* inds_and_weights,
                    DeviceMatrixCL3D* output,
                    const int cell_size,
                    const int offset_y,
                    const int offset_x,
                    const int max_bin)
{
    const int frame_height = inds_and_weights->dim_y;
    const int frame_width = inds_and_weights->dim_x;
    
    const size_t local_work_size[2] = {BLOCK_8, BLOCK_8}; 
    
    
    const int n_blocks_x = ((frame_height - offset_y) / cell_size + 1)* local_work_size[0];
    
    const int n_blocks_y = ((frame_width  - offset_x) / cell_size + 1)* local_work_size[1];
    
    const size_t global_work_size[2] = {n_blocks_x, n_blocks_y};
    
    TheContext* tc = new TheContext();
    
    cl_context GPUContext = tc->getMyContext()->getContextCL();
    cl_device_id cdDevice = tc->getMyContext()->getDeviceCL();
    
    // Creates the program
    // Uses NVIDIA helper functions to get the code string and it's size (in bytes)
    
    MyKernels *kernels = new MyKernels(GPUContext,cdDevice);
    
    cl_kernel theKernel= kernels->getCellHistogramKernel();
    
    cl_int err;
    err=0;
    
    err =  parameters_histogram(theKernel, inds_and_weights, output,
    							 cell_size, offset_y,offset_x, 
    							 max_bin);	
    
    if (err != CL_SUCCESS) {
        printf("Error: Failed to set kernel arguments 3! %d\n", err);
        exit(1);
    }
    
    
    err = clEnqueueNDRangeKernel(tc->getMyContext()->cqCommandQueue, 
    							 theKernel, 2, NULL, 
    							 global_work_size, local_work_size, 0, NULL, NULL);
    
    if (err) {
        printf("Error: Failed to execute kernel! %d\n", err);
        exit(1);
    }
}





