
/*
 * =====================================================================================
 *
 *       Filename:  DW_banded.c
 *
 *    Description:  A banded version for the O(ND) greedy sequence alignment algorithm
 *
 *        Version:  0.1
 *        Created:  07/20/2013 17:00:00
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jason Chin,
 *        Company:
 *
 * =====================================================================================

 #################################################################################$$
 # Copyright (c) 2011-2014, Pacific Biosciences of California, Inc.
 #
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted (subject to the limitations in the
 # disclaimer below) provided that the following conditions are met:
 #
 #  * Redistributions of source code must retain the above copyright
 #  notice, this list of conditions and the following disclaimer.
 #
 #  * Redistributions in binary form must reproduce the above
 #  copyright notice, this list of conditions and the following
 #  disclaimer in the documentation and/or other materials provided
 #  with the distribution.
 #
 #  * Neither the name of Pacific Biosciences nor the names of its
 #  contributors may be used to endorse or promote products derived
 #  from this software without specific prior written permission.
 #
 # NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 # GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY PACIFIC
 # BIOSCIENCES AND ITS CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 # WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 # OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 # DISCLAIMED. IN NO EVENT SHALL PACIFIC BIOSCIENCES OR ITS
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 # SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 # LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 # USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 # ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 # OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 # OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 # SUCH DAMAGE.
 #################################################################################$$


*/

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include "shimmer.h"

int compare_d_path(const void * a, const void * b)
{
    const d_path_data2 * arg1 = a;
    const d_path_data2 * arg2 = b;
    if (arg1->d - arg2->d == 0) {
        return  arg1->k - arg2->k;
    } else {
        return arg1->d - arg2->d;
    }
}


void d_path_sort( d_path_data2 * base, unsigned long max_idx) {
    qsort(base, max_idx, sizeof(d_path_data2), compare_d_path);
}

d_path_data2 * get_dpath_idx( seq_coor_t d, seq_coor_t k, 
		unsigned long max_idx, d_path_data2 * base) {
    d_path_data2 d_tmp;
    d_path_data2 *rtn;
    d_tmp.d = d;
    d_tmp.k = k;
    rtn = (d_path_data2 *)  bsearch( &d_tmp, base, max_idx, sizeof(d_path_data2), compare_d_path);
    //printf("dp %ld %ld %ld %ld %ld %ld %ld\n", (rtn)->d, (rtn)->k, (rtn)->x1, (rtn)->y1, (rtn)->x2, (rtn)->y2, (rtn)->pre_k);

    return rtn;

}

void print_d_path(  d_path_data2 * base, unsigned long max_idx) {
    unsigned long idx;
    for (idx = 0; idx < max_idx; idx++){
        printf("dp %ld %d %d %d %d %d %d %d\n",idx, 
				(base+idx)->d, (base+idx)->k, 
				(base+idx)->x1, (base+idx)->y1, 
				(base+idx)->x2, (base+idx)->y2, (base+idx)->pre_k);
    }
}


alignment * align(char * query_seq, seq_coor_t q_len,
                  char * target_seq, seq_coor_t t_len,
                  seq_coor_t band_tolerance) {
    seq_coor_t * V;
    seq_coor_t * U;  // array of matched bases for each "k"
    seq_coor_t k_offset;
    seq_coor_t d;
    seq_coor_t k, k2;
    seq_coor_t best_m;  // the best "matches" for each d
    seq_coor_t min_k, new_min_k;
    seq_coor_t max_k, new_max_k;
    seq_coor_t pre_k;
    seq_coor_t x, y;
    seq_coor_t cd;
    seq_coor_t ck;
    seq_coor_t cx, cy, nx, ny;
    seq_coor_t max_d;
    seq_coor_t band_size;
	bool start = false;
    unsigned long d_path_idx = 0;
    unsigned long max_idx = 0;

    d_path_data2 * d_path;
    d_path_data2 * d_path_aux;
    path_point * aln_path;
    seq_coor_t aln_path_idx;
    alignment * align_rtn;
    seq_coor_t aln_pos;
    seq_coor_t i;
    bool aligned = false;

    //printf("debug: %ld %ld\n", q_len, t_len);
    //printf("%s\n", query_seq);

    max_d = (int) (0.3*(q_len + t_len));

    band_size = band_tolerance * 2;

    V = calloc( max_d * 2 + 1, sizeof(seq_coor_t) );
    U = calloc( max_d * 2 + 1, sizeof(seq_coor_t) );

    k_offset = max_d;

    // We should probably use hashmap to store the backtracing information to save memory allocation time
    // This O(MN) block allocation scheme is convient for now but it is slower for very long sequences
    d_path = calloc( max_d * (band_size + 1 ) * 2 + 1, sizeof(d_path_data2) );

    aln_path = calloc( q_len + t_len + 1, sizeof(path_point) );

    align_rtn = calloc( 1, sizeof(alignment));
    align_rtn->t_aln_str = calloc( q_len + t_len + 1, sizeof(char));
    align_rtn->q_aln_str = calloc( q_len + t_len + 1, sizeof(char));
    align_rtn->aln_str_size = 0;
    align_rtn->aln_q_s = 0;
    align_rtn->aln_q_e = 0;
    align_rtn->aln_t_s = 0;
    align_rtn->aln_t_e = 0;

    //printf("max_d: %lu, band_size: %lu\n", max_d, band_size);
    best_m = -1;
    min_k = 0;
    max_k = 0;
    d_path_idx = 0;
    max_idx = 0;
    for (d = 0; d < max_d; d ++ ) {
        if (max_k - min_k > band_size) {
            break;
        }

        for (k = min_k; k <= max_k;  k += 2) {

            if ( (k == min_k) || ((k != max_k) && (V[ k - 1 + k_offset ] < V[ k + 1 + k_offset])) ) {
                pre_k = k + 1;
                x = V[ k + 1 + k_offset];
            } else {
                pre_k = k - 1;
                x = V[ k - 1 + k_offset] + 1;
            }
            y = x - k;
            d_path[d_path_idx].d = d;
            d_path[d_path_idx].k = k;
            d_path[d_path_idx].x1 = x;
            d_path[d_path_idx].y1 = y;

            while ( x < q_len && y < t_len && query_seq[x] == target_seq[y] ){
                x++;
                y++;
            }

            d_path[d_path_idx].x2 = x;
            d_path[d_path_idx].y2 = y;
			if ( (x - d_path[d_path_idx].x1 > 16) && (start == false) ) {
				align_rtn->aln_q_s = d_path[d_path_idx].x1;
				align_rtn->aln_t_s = d_path[d_path_idx].y1;
				start = true;
			}
            d_path[d_path_idx].pre_k = pre_k;
            d_path_idx ++;

            V[ k + k_offset ] = x;
            U[ k + k_offset ] = x + y;

            if ( x + y > best_m) {
                best_m = x + y;
            }

            if ( x >= q_len || y >= t_len) {
                aligned = true;
                max_idx = d_path_idx;
                break;
            }
        }

        // For banding
        new_min_k = max_k;
        new_max_k = min_k;

        for (k2 = min_k; k2 <= max_k;  k2 += 2) {
            if (U[ k2 + k_offset] >= best_m - band_tolerance ) {
                if ( k2 < new_min_k ) {
                    new_min_k = k2;
                }
                if ( k2 > new_max_k ) {
                    new_max_k = k2;
                }
            }
        }

        max_k = new_max_k + 1;
        min_k = new_min_k - 1;

        if (aligned == true) {
            align_rtn->aln_q_e = x;
            align_rtn->aln_t_e = y;
            align_rtn->dist = d;
            align_rtn->aln_str_size = (x + y + d) / 2;
            //d_path_sort(d_path, max_idx);
            //print_d_path(d_path, max_idx);
            break;
        } 
    }
	if (aligned == false) {
		align_rtn->aln_q_s = 0;
		align_rtn->aln_t_s = 0;
	}

    free(V);
    free(U);
    free(d_path);
    free(aln_path);
    return align_rtn;
}


void free_alignment(alignment * aln) {
    free(aln->q_aln_str);
    free(aln->t_aln_str);
    free(aln);
}
