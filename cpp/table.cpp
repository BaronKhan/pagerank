/* Copyright (c) 2010-2011, Panos Louridas, GRNET S.A.
 
   All rights reserved.
  
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
 
   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
 
   * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.
 
   * Neither the name of GRNET S.A, nor the names of its contributors
   may be used to endorse or promote products derived from this
   software without specific prior written permission.
  
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <iostream>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include <math.h>
#include <string>
#include <cstring>
#include <limits>

#include "table.h"



#define __CL_ENABLE_EXCEPTIONS 
#include "CL/cl.hpp"


std::string LoadSource(const char *fileName)
{
	
	std::string fullName=fileName;
	
	// Open a read-only binary stream over the file
	std::ifstream src(fullName, std::ios::in | std::ios::binary);
	if(!src.is_open())
		throw std::runtime_error("LoadSource : Couldn't load cl file from '"+fullName+"'.");
	
	// Read all characters of the file into a string
	return std::string(
		(std::istreambuf_iterator<char>(src)), // Node the extra brackets.
        std::istreambuf_iterator<char>()
	);
}



void Table::reset() {
    num_outgoing.clear();
    rows.clear();
    nodes_to_idx.clear();
    idx_to_nodes.clear();
    pr.clear();
}

Table::Table(double a, double c, size_t i, bool t, bool n, string d)
    : trace(t),
      alpha(a),
      convergence(c),
      max_iterations(i),
      delim(d),
      numeric(n) {
}

void Table::reserve(size_t size) {
    num_outgoing.reserve(size);
    rows.reserve(size);
}

const size_t Table::get_num_rows() {
    return rows.size();
}

void Table::set_num_rows(size_t num_rows) {
    num_outgoing.resize(num_rows);
    rows.resize(num_rows);
}

const void Table::error(const char *p,const char *p2) {
    cerr << p <<  ' ' << p2 <<  '\n';
    exit(1);
}

const double Table::get_alpha() {
    return alpha;
}

void Table::set_alpha(double a) {
    alpha = a;
}

const unsigned long Table::get_max_iterations() {
    return max_iterations;
}

void Table::set_max_iterations(unsigned long i) {
    max_iterations = i;
}

const double Table::get_convergence() {
    return convergence;
}

void Table::set_convergence(double c) {
    convergence = c;
}

const vector<double>& Table::get_pagerank() {
    return pr;
}

const string Table::get_node_name(size_t index) {
    if (numeric) {
        stringstream s;
        s << index;
        return s.str();
    } else {
        return idx_to_nodes[index];
    }
}

const map<size_t, string>& Table::get_mapping() {
    return idx_to_nodes;
}

const bool Table::get_trace() {
    return trace;
}

void Table::set_trace(bool t) {
    trace = t;
}

const bool Table::get_numeric() {
    return numeric;
}

void Table::set_numeric(bool n) {
    numeric = n;
}

const string Table::get_delim() {
    return delim;
}

void Table::set_delim(string d) {
    delim = d;
}






/*
 * From a blog post at: http://bit.ly/1QQ3hv
 */
void Table::trim(string &str) {

    size_t startpos = str.find_first_not_of(" \t");

    if (string::npos == startpos) {
        str = "";
    } else {
        str = str.substr(startpos, str.find_last_not_of(" \t") - startpos + 1);
    }
}

size_t Table::insert_mapping(const string &key) {

    size_t index = 0;
    map<string, size_t>::const_iterator i = nodes_to_idx.find(key);
    if (i != nodes_to_idx.end()) {
        index = i->second;
    } else {
        index = nodes_to_idx.size();
        nodes_to_idx.insert(pair<string, size_t>(key, index));
        idx_to_nodes.insert(pair<size_t, string>(index, key));;
    }

    return index;
}

int Table::read_file(const string &filename) {

    pair<map<string, size_t>::iterator, bool> ret;

    reset();
    
    istream *infile;

    if (filename.empty()) {
      infile = &cin;
    } else {
      infile = new ifstream(filename.c_str());
      if (!infile) {
          error("Cannot open file", filename.c_str());
      }
    }
    
    size_t delim_len = delim.length();
    size_t linenum = 0;
    string line; // current line
    while (getline(*infile, line)) {
        string from, to; // from and to fields
        size_t from_idx, to_idx; // indices of from and to nodes
        size_t pos = line.find(delim);
        if (pos != string::npos) {
            from = line.substr(0, pos);
            trim(from);
            if (!numeric) {
                from_idx = insert_mapping(from);
            } else {
                from_idx = strtol(from.c_str(), NULL, 10);
            }
            to = line.substr(pos + delim_len);
            trim(to);
            if (!numeric) {
                to_idx = insert_mapping(to);
            } else {
                to_idx = strtol(to.c_str(), NULL, 10);
            }
            add_arc(from_idx, to_idx);
        }

        linenum++;

	       
	if (linenum && ((linenum % 100000) == 0)) {
            cerr << "read " << linenum << " lines, "
                 << rows.size() << " vertices" << endl;
        }
        

        from.clear();
        to.clear();
        line.clear();
    }

    cerr << "read " << linenum << " lines, " << rows.size() << " vertices" << endl;

    nodes_to_idx.clear();

    if (infile != &cin) {
        delete infile;
    }
    reserve(idx_to_nodes.size());
    
    return 0;
}

/*
 * Taken from: M. H. Austern, "Why You Shouldn't Use set - and What You Should
 * Use Instead", C++ Report 12:4, April 2000.
 */
template <class Vector, class T>
bool Table::insert_into_vector(Vector& v, const T& t) {
    typename Vector::iterator i = lower_bound(v.begin(), v.end(), t);
    if (i == v.end() || t < *i) {
        v.insert(i, t);
        return true;
    } else {
        return false;
    }
}

bool Table::add_arc(size_t from, size_t to) {

    bool ret = false;
    size_t max_dim = max(from, to);
    if (trace) {
        cout << "checking to add " << from << " => " << to << endl;
    }
    if (rows.size() <= max_dim) {
        max_dim = max_dim + 1;
        if (trace) {
            cout << "resizing rows from " << rows.size() << " to "
                 << max_dim << endl;
        }
        rows.resize(max_dim);
        if (num_outgoing.size() <= max_dim) {
            num_outgoing.resize(max_dim);
        }
    }

    ret = insert_into_vector(rows[to], from);

    if (ret) {
        num_outgoing[from]++;
        if (trace) {
            cout << "added " << from << " => " << to << endl;
        }
    }

    return ret;
}




//=====================================


long findTotalElements(const std::vector<std::vector<size_t> >& rows)
{

    long sum_elements =0 ; 

    for(int i = 0, len = rows.size() ; i < len ; ++i)
    {

       sum_elements += (rows.at(i)).size();

    }

    return sum_elements;
}

// input - rows
// output - flattened pointer.
// 	  - start of each row 
//        - length of each row.
void flattenRows(const std::vector<std::vector<size_t> >& rows, size_t*& flattened_rows_ptr, std::vector<int>&  start_each_row, std::vector<int>& length_each_row, long total_elements)
{

    flattened_rows_ptr = new size_t[total_elements];

    int num_rows = rows.size();
    length_each_row.resize(num_rows);
    start_each_row.resize(num_rows);


    int current_start = 0 ;
    long k = 0 ;

    for(int i = 0 ; i < num_rows ; ++i)
    {

        int row_length = (rows.at(i)).size();


	for(int j = 0 ; j < row_length ; ++j)
	{
	    flattened_rows_ptr[k] =  (rows.at(i)).at(j);
            ++k;
	}

        // calculate the start of each row.
	start_each_row[i] = current_start ;
	length_each_row[i] = row_length;
        current_start = current_start + row_length;

    }

}



/*
// Sequential version of openCL kernel (used mainly to verify logic) 
void sequential_pagerank(double sum_oneAv_oneIv, double alpha, int num_rows, size_t* flattened_rows_ptr, int* start_each_row, int* length_each_row, double* pr, double* old_pr, size_t* num_outgoing)
{

	for (int i = 0; i < num_rows; ++i) 
        {
            // The corresponding element of the H multiplication 
            double h = 0.0;

	 	// FIND START INDEX OF EACH ROW. ALSO FIND LENGTH OF EACH ROW.

		// width of each row.
		int start_index = start_each_row[i];
                int row_length = length_each_row[i];

		for (int j = 0; j < row_length; ++j) 
		{
			int nodeIndex = flattened_rows_ptr[start_index + j];


			double h_v = 1.0 / (num_outgoing[nodeIndex]);

			if(num_outgoing[nodeIndex] == 0)
			{
                          std::cout << "num_outgoing is 0" << std::endl;
			}

			h += h_v * old_pr[nodeIndex];
		}
 
            pr[i] = alpha*h + sum_oneAv_oneIv;

        } 



}
*/





void Table::pagerank() {


    size_t i;
    double sum_pr;       // sum of current pagerank vector elements
    double dangling_pr; // sum of current pagerank vector elements for dangling
    			// nodes
    double diff = 1;

    unsigned long num_iterations = 0;
    size_t num_rows = rows.size();


    if (num_rows == 0) {
        return;
    }

    std::vector<double> old_pr;
    old_pr.resize(num_rows);
    pr.resize(num_rows);

    pr[0] = 1;


    double one_Av,one_Iv, sum_oneAv_oneIv;
    double num_cols = num_rows; // this is just for better understanding...
    double oneminusalpha_factor = (1 - alpha) / num_rows ;
    double alpha_factor = alpha/ num_rows ;



    // =============== OpenCL setup code ====================


       //1) Choosing a Platform ....
	std::vector<cl::Platform> platforms;

	cl::Platform::get(&platforms);
	if(platforms.size()==0)
	{
		throw std::runtime_error("No OpenCL platforms found.");
	}


	int selectedPlatform=0;
	cl::Platform platform=platforms.at(selectedPlatform);    




       // 2) Choosing a Device ....
	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
	if(devices.size()==0)
	{
		throw std::runtime_error("No opencl devices found.\n");
	}



	int selectedDevice=0;
	cl::Device device=devices.at(selectedDevice);



	// 3) Creating a context ......
	// Notice how it takes a device as an input.
        cl::Context context(devices);



	// 4) Fetch kernelcode from source and build/compile it.
	// Note- We have not actually created a kernel object yet, just fetched the source code.
	// In simple words : program = context + kernel.

	std::string kernelSource=LoadSource("table.cl");

	cl::Program::Sources sources;	// A vector of (data,length) pairs
	sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1));	// push on our single string

        cl::Program program(context, sources);


	// To catch compilation errors of kernel code. Note that this is runtime compilation of kernel code.
	try
	{
		program.build(devices);
	}
	catch(...)
	{
		for(unsigned i=0;i<devices.size();i++)
		{
			std::cerr<<"Log for device "<<devices[i].getInfo<CL_DEVICE_NAME>()<<":\n\n";
			std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i])<<"\n\n";
		}
		throw;
	}



   //==============================================

       size_t* flattened_rows_ptr;
       std::vector<int> start_each_row;
       std::vector<int> length_each_row;
       long total_elements = findTotalElements(rows);

       flattenRows(rows, flattened_rows_ptr, start_each_row, length_each_row, total_elements);


	// Step 1 After successful compilation - 
	//	Create GPU buffers. 
        size_t cbBuffFlatten =    8*total_elements;
	size_t cbBuffer_32   =    4*num_rows;
        size_t cbBuffer_64   =    8*num_rows;
        size_t cbNumOutgoing_32 = 8*num_outgoing.size(); // might be redundant...


        cl::Buffer buffFlattenedRowsPtr(context, CL_MEM_READ_ONLY, cbBuffFlatten); // rows
	cl::Buffer buffStartEachRow(context, CL_MEM_READ_ONLY, cbBuffer_32);  //start_each_row
	cl::Buffer buffLengthEachRow(context, CL_MEM_READ_ONLY, cbBuffer_32); //length_each_row

        cl::Buffer buffPr(context, CL_MEM_WRITE_ONLY, cbBuffer_64); // pr - NOTE: WRITE ACCESS
        cl::Buffer buffOldPr(context, CL_MEM_READ_ONLY, cbBuffer_64); //old_pr

        cl::Buffer buffNumOutgoing(context, CL_MEM_READ_ONLY, cbNumOutgoing_32); //num_outgoing


	// Step 2 - create kernel object.
        cl::Kernel kernel(program, "kernel_pagerank");


        // Step 3 - Bind  rguments to kernel.
	kernel.setArg(0, sum_oneAv_oneIv);
	kernel.setArg(1, alpha);
	kernel.setArg(2, buffFlattenedRowsPtr);
	kernel.setArg(3, buffStartEachRow);
	kernel.setArg(4, buffLengthEachRow);
	kernel.setArg(5, buffPr);
	kernel.setArg(6, buffOldPr);
	kernel.setArg(7, buffNumOutgoing);


	// Step 3 -  Create command queue for a single device.
	//Notice how a cl::Context can span over multiple devices but a CommandQueue can't.
	cl::CommandQueue queue(context, device);


	// Step 4 - Copy over FIXED stuff before loop begins
        // This copy is synchronous.
 	queue.enqueueWriteBuffer(buffFlattenedRowsPtr, CL_TRUE, 0, cbBuffFlatten, &flattened_rows_ptr[0]);
	queue.enqueueWriteBuffer(buffStartEachRow, CL_TRUE, 0, cbBuffer_32, &start_each_row[0]);
	queue.enqueueWriteBuffer(buffLengthEachRow, CL_TRUE, 0, cbBuffer_32, &length_each_row[0]);
	queue.enqueueWriteBuffer(buffNumOutgoing, CL_TRUE, 0, cbNumOutgoing_32, &num_outgoing[0]);



        // Finally set up the iteration space for kernel execution ...
	cl::NDRange offset(0);				// Always start iterations at x=0, y=0
	cl::NDRange globalSize(num_rows);	// Global size must match the original loops
	cl::NDRange localSize=cl::NullRange;	// We don't care about local size



    // Main loop

    while (diff > convergence && num_iterations < max_iterations) {

	// ===== normalization ======

        sum_pr = 0;
        dangling_pr = 0;

        for (size_t k = 0; k < num_cols ; ++k)
        {
            sum_pr += pr[k];


            if (num_outgoing[k] == 0)
            {
                dangling_pr += pr[k];
            }
        }


        if (num_iterations == 0) 
        {
	    for(i=0; i < num_cols; ++i)
	    {
	        old_pr[i] = pr[i];
	    }

        } else {
            /* Normalize so that we start with sum equal to one */
            for (i = 0; i < num_cols; ++i) {
                old_pr[i] = pr[i] / sum_pr;
            }

        }



         //==== After old_pr is calculated from current pr, write old_pr to GPU memory =====
	cl::Event evCopiedState;
	queue.enqueueWriteBuffer(buffOldPr, CL_FALSE, 0, cbBuffer_64, &old_pr[0], NULL, &evCopiedState);




        //After normalisation the elements of the pagerank vector sum to one
        sum_pr = 1;


        one_Av = alpha_factor * dangling_pr;
        one_Iv = oneminusalpha_factor * sum_pr ;
        sum_oneAv_oneIv = one_Av + one_Iv;

	kernel.setArg(0, sum_oneAv_oneIv);


        /* The difference to be checked for convergence */
        diff = 0;


       // execute NDRange kernel 
	std::vector<cl::Event> kernelDependencies(1, evCopiedState);
	cl::Event evExecutedKernel;
	queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize, &kernelDependencies, &evExecutedKernel);


         // Call to sequential version of openCL kernel.
         //sequential_pagerank(sum_oneAv_oneIv, alpha, num_rows, flattened_rows_ptr, &start_each_row[0], &length_each_row[0] , &pr[0],  &old_pr[0], &num_outgoing[0]);




	// ====== Read pr from GPU memory ===========
	std::vector<cl::Event> copyBackDependencies(1, evExecutedKernel);
        queue.enqueueReadBuffer(buffPr, CL_TRUE, 0, cbBuffer_64, &pr[0], &copyBackDependencies);




	for(int i = 0 ; i < num_rows ; ++i)
	{
	    diff += fabs(pr[i] - old_pr[i]);
	}

        ++num_iterations;


    }

}

const void Table::print_params(ostream& out) {
    out << "alpha = " << alpha << " convergence = " << convergence
        << " max_iterations = " << max_iterations
        << " numeric = " << numeric
        << " delimiter = '" << delim << "'" << endl;
}

const void Table::print_table() {
    vector< vector<size_t> >::iterator cr;
    vector<size_t>::iterator cc; // current column

    size_t i = 0;
    for (cr = rows.begin(); cr != rows.end(); cr++) {
        cout << i << ":[ ";
        for (cc = cr->begin(); cc != cr->end(); cc++) {
            if (numeric) {
                cout << *cc << " ";
            } else {
                cout << idx_to_nodes[*cc] << " ";
            }
        }
        cout << "]" << endl;
        i++;
    }
}

const void Table::print_outgoing() {
    vector<size_t>::iterator cn;

    cout << "[ ";
    for (cn = num_outgoing.begin(); cn != num_outgoing.end(); cn++) {
        cout << *cn << " ";
    }
    cout << "]" << endl;

}

const void Table::print_pagerank() {

    vector<double>::iterator cr;
    double sum = 0;

    cout.precision(numeric_limits<double>::digits10);
    
    cout << "(" << pr.size() << ") " << "[ ";
    for (cr = pr.begin(); cr != pr.end(); cr++) {
        cout << *cr << " ";
        sum += *cr;
        cout << "s = " << sum << " ";
    }
    cout << "] "<< sum << endl;
}

const void Table::print_pagerank_v() {

    size_t i;
    size_t num_rows = pr.size();
    double sum = 0;
    
    cout.precision(numeric_limits<double>::digits10);

    for (i = 0; i < num_rows; i++) {
        if (!numeric) {
            cout << idx_to_nodes[i] << " = " << pr[i] << endl;
        } else {
            cout << i << " = " << pr[i] << endl;
        }
        sum += pr[i];
    }
    cerr << "s = " << sum << " " << endl;
}
