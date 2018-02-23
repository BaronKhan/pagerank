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
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>
#include <math.h>
#include <string>
#include <cstring>
#include <limits>

#include "table.h"

#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"
#include "tbb/blocked_range.h"
using namespace tbb;


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

// finds the sum of 2 vectors

struct SumTwoVec {

    double value;
    double value2;

    double* myPr;
    size_t* mynum_outgoing;

    SumTwoVec(double* pr, size_t* num_outgoing) : value(0) ,value2(0) {myPr = pr; mynum_outgoing = num_outgoing; }

    SumTwoVec( SumTwoVec& s, split )
    {
        value = 0;
        value2 = 0;
        myPr = s.myPr;
        mynum_outgoing = s.mynum_outgoing;
    }

    void operator()( const blocked_range<int>& r ) 
    {
	// do NOT initialize to 0.
        double temp = value;
        double temp2 = value2;
 
	int len = r.end();
        for( int i=r.begin(); i!=len ; ++i )
        {
            temp += myPr[i];
            
            
            temp2 += (mynum_outgoing[i] == 0) * myPr[i];
            
        }

        value = temp;
        value2 = temp2;
    }

    void join( SumTwoVec& rhs ) 
    {
        value += rhs.value;
        value2 += rhs.value2;
    }
};



// finds the sum of 1 vexctor
struct SumOneVec {

    double value;
    double* myPr;

    SumOneVec(double* pr) : value(0) {myPr = pr;}

    SumOneVec( SumOneVec& s, split )
    {
        value = 0;
        myPr = s.myPr;
    }

    void operator()( const blocked_range<int>& r ) 
    {
	// do NOT initialize to 0.
        double temp = value;
        for( int i=r.begin(),len = r.end(); i!=len ; ++i )
        {
            temp += myPr[i];         
        }

        value = temp;
   
    }

    void join( SumOneVec& rhs ) 
    {
        value += rhs.value;
    }
};






void Table::pagerank() {

   
    size_t i;
    double sum_pr;       // sum of current pagerank vector elements
    double dangling_pr; // sum of current pagerank vector elements for dangling
    			// nodes
    double diff = 1;

    unsigned long num_iterations = 0;
    vector<double> old_pr;
	
    size_t num_rows = rows.size();
    
    if (num_rows == 0) {
        return;
    }
    
    pr.resize(num_rows);

    pr[0] = 1;

    if (trace) {
        print_pagerank();
    }

    double one_Av,one_Iv, sum_oneAv_oneIv;
    double num_cols = num_rows; // this is just for better understanding...
    double oneminusalpha_factor = (1 - alpha) / num_rows ;
    double alpha_factor = alpha/ num_rows ;

    while (diff > convergence && num_iterations < max_iterations) {
     
        
	// ===== normalization ======

	/*
        sum_pr = 0;
        dangling_pr = 0;
        double cpr;
        for (size_t k = 0; k < num_cols ; ++k)
        {
            cpr = pr[k];
            sum_pr += cpr;

            if (num_outgoing[k] == 0)
            {
                dangling_pr += cpr;
            }
        }
        */
      
       SumTwoVec total( &pr[0] , &num_outgoing[0]);
       tbb::parallel_reduce( tbb::blocked_range<int>( 0, num_rows ), total );
       sum_pr = total.value;
       dangling_pr = total.value2;
      
	
        if (num_iterations == 0) {
            old_pr = pr;
        } else {
            /* Normalize so that we start with sum equal to one */
            for (i = 0; i < num_cols; ++i) {
                old_pr[i] = pr[i] / sum_pr;
            }

        }

        /*
         * After normalisation the elements of the pagerank vector sum
         * to one
         */
        sum_pr = 1;
        
       

        /* An element of the A x I vector; all elements are identical */
        one_Av = alpha_factor * dangling_pr;

        /* An element of the 1 x I vector; all elements are identical */
        one_Iv = oneminusalpha_factor * sum_pr ;
        sum_oneAv_oneIv = one_Av + one_Iv;
	
        /* The difference to be checked for convergence */
       
       
       std::vector<double> diff_vec(num_rows);
	
	
        //for (i = 0; i < num_rows; ++i) {
        tbb::parallel_for(size_t(0), num_rows, [&](size_t i){

            /* The corresponding element of the H multiplication */
            double h = 0.0;
  
            for (vector<size_t>::iterator ci = rows[i].begin(), cend = rows[i].end() ; ci != cend; ++ci) 
            {
                /* The current element of the H vector */
                double h_v = 1.0 / (num_outgoing[*ci]);

                if (num_iterations == 0 && trace) {cout << "h[" << i << "," << *ci << "]=" << h_v << endl;}

                h += h_v * old_pr[*ci];
            }

            pr[i] = alpha*h + sum_oneAv_oneIv;

            //diff += fabs(pr[i] - old_pr[i]);
            diff_vec[i] = fabs(pr[i] - old_pr[i]);
             
        } );

        /*
        diff = 0;
        for(int k = 0 ; k < diff_vec.size() ; ++k)
        {
            diff += diff_vec[k];
        }
        */
	
        SumOneVec total2( &diff_vec[0]);
        tbb::parallel_reduce( tbb::blocked_range<int>( 0, num_rows ), total2 );
        diff = total2.value;
         
        ++num_iterations;

        if (trace) { cout << num_iterations << ": "; print_pagerank();}

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
