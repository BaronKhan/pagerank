#ifdef cl_khr_fp64
    #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)
    #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#else
    #error "Double precision floating point not supported by OpenCL implementation."
#endif



__kernel void kernel_pagerank( double sum_oneAv_oneIv, double alpha, __global size_t* flattened_rows_ptr, __global int* start_each_row,  __global int* length_each_row, __global double* pr,__global double* old_pr, __global size_t* num_outgoing)
{
     
     
	    // get the index for filling pr 
	    int i = get_global_id(0);
  
            // The corresponding element of the H multiplication 
            double h = 0.0;
            
		// width of each row.
		int start_index = start_each_row[i];
                int row_length = length_each_row[i];

		for (int j = 0; j < row_length; ++j) 
		{
			int nodeIndex = flattened_rows_ptr[start_index + j];

	
			double h_v = 1.0 / (double)(num_outgoing[nodeIndex]);

			h += h_v * old_pr[nodeIndex];
		}


            pr[i] = alpha*h + sum_oneAv_oneIv;


}
