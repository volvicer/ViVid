#include "BlockHistogramWrapper.hpp"
#include "BlockHistogram.hpp"
#include <boost/python.hpp>
#include "NumPyWrapper.hpp"

#ifdef _WIN32
#include "omp.h"
#else
#include "omp_unix.h"
#endif

#define PY_ARRAY_UNIQUE_SYMBOL tb
#define NO_IMPORT
#include <numpy/arrayobject.h>

using namespace boost::python;

object cell_histogram_dense_c(object& input_mat, object& weight_mat, 
                             const int max_bin, const int cell_size,
                             const int margin_top, const int margin_left,
                             const int margin_bottom, const int margin_right)
{
    NumPyMatrix id_m(input_mat);
    NumPyMatrix wgts(weight_mat);
    const float* id_data = (float*) id_m.data();
    const float* wt_data = (float*) wgts.data();

    //find out the input size
    const int im_height = id_m.height();
    const int im_width = wgts.width();

    const int margin_v = margin_top + margin_bottom;
    const int margin_h = margin_left + margin_right;

    int n_parts_y = (int) (im_height - margin_v) / cell_size;
    int n_parts_x = (int) (im_width - margin_h) / cell_size;

    npy_intp dims[3] = {n_parts_y, n_parts_x, max_bin};
    
    PyObject* arr = PyArray_SimpleNew(3, dims, PyArray_FLOAT);

    float* out_data = (float*)PyArray_DATA(arr);
    
    memset(out_data, 0, sizeof(float) * n_parts_y * n_parts_x * max_bin);

    int start_i = (int) margin_top;
    int start_j = (int) margin_left;

    double tic = omp_get_wtime();

    #pragma omp parallel for
    for (int write_i=0; write_i<n_parts_y; write_i++){
        for (int write_j=0; write_j<n_parts_x; write_j++){
            int out_ind = (write_i*n_parts_x + write_j) * max_bin;

            int read_i = (start_i + (write_i * cell_size)) * im_width;
            for (int i=0; i<cell_size; i++){
                int read_j = start_j + write_j * cell_size ;

                for (int j=0; j<cell_size; j++){
                    int bin_ind = (int)id_data[read_i+read_j+j];
                    assert((bin_ind >= 0) && (bin_ind < max_bin));
                    float weight = wt_data[read_i+read_j+j];
                    out_data[out_ind + bin_ind] += weight;
                }
                read_i += im_width;
            }
        }
    }
    handle<> temp_out(arr);

    double toc = omp_get_wtime();

    //printf("Loop time: %.8f\n",(toc-tic) * 1000000.0f);

    return boost::python::object(temp_out);
}

object cell_histogram_dense(object& input_mat, object& weight_mat, 
                             const int max_bin, const int cell_size, 
                             object& start_inds, object& stop_inds)
{
    NumPyMatrix id_m(input_mat);
    NumPyMatrix wgts(weight_mat);
    const float* id_data = (float*) id_m.data();
    const float* wt_data = (float*) wgts.data();

    NumPyArray start_arr(start_inds);
    NumPyArray stop_arr(stop_inds);

    int n_parts_y = (int)(stop_arr.data()[0] - start_arr.data()[0] ) / cell_size;
    int n_parts_x = (int)(stop_arr.data()[1] - start_arr.data()[1] ) / cell_size;

    npy_intp dims[3] = {n_parts_y, n_parts_x, max_bin};
    
    PyObject* arr = PyArray_SimpleNew(3, dims, PyArray_FLOAT);
    float* out_data = (float*)PyArray_DATA(arr);
    
    memset(out_data, 0, sizeof(float) * n_parts_y * n_parts_x * max_bin);

    int start_i = (int) start_arr.data()[0];
    int start_j = (int) start_arr.data()[1];

    //double tic = omp_get_wtime();

    double tic = omp_get_wtime();

    const int im_width = id_m.width();
    const int im_height = id_m.height();

    #pragma omp parallel for
    for (int write_i=0; write_i<n_parts_y; write_i++){
        for (int write_j=0; write_j<n_parts_x; write_j++){
            int out_ind = (write_i*n_parts_x + write_j) * max_bin;

            int read_i = (start_i + (write_i * cell_size)) * im_width;
            for (int i=0; i<cell_size; i++){
                int read_j = start_j + write_j * cell_size ;

                for (int j=0; j<cell_size; j++){
                    int bin_ind = (int)id_data[read_i+read_j+j];
                    assert((bin_ind >= 0) && (bin_ind < max_bin));
                    float weight = wt_data[read_i+read_j+j];
                    out_data[out_ind + bin_ind] += weight;
                }
                read_i += im_width;
            }
        }
    }
    
    handle<> temp_out(arr);

    double toc = omp_get_wtime();

    //printf("Loop time: %.8f\n",(toc-tic) * 1000000.0f);

    return boost::python::object(temp_out);
}



void export_BlockHistogram()
{
    def("cell_histogram_dense", cell_histogram_dense);
    def("cell_histogram_dense_c", cell_histogram_dense_c);

    def<DeviceMatrix3D::Ptr (
        const DeviceMatrix::Ptr&, const DeviceMatrix::Ptr&,
        const int, const int, 
        boost::python::object&, boost::python::object&) >
        ("cell_histogram_dense_cuda", cell_histogram_dense_cuda);
	
	def<DeviceMatrixCL3D::Ptr (
							 const DeviceMatrixCL::Ptr&, const DeviceMatrixCL::Ptr&,
							 const int, const int, 
							 boost::python::object&, boost::python::object&) >
	("cell_histogram_dense_cl", cell_histogram_dense_cl);
}
