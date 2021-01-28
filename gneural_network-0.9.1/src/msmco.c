/* msmco.c -- This belongs to gneural_network

   gneural_network is the GNU package which implements a programmable neural network.

   Copyright (C) 2016 Jean Michel Sellier
   <jeanmichel.sellier@gmail.com>
   Copyright (C) 2016 Francesco Maria Virlinzi
   <francesco.virlinzi@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

// perform a modified version of the multi-scale Monte Carlo optimization to train the network

#include <msmco.h>
#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <rnd.h>

void msmco(network *nn, network_config *config) {
 int output = config->verbosity;/* screen output - on/off */
 int mmax =config->mmax;        /* number of MC outer iterations */
 int nmax = config->nmax;    /* number of MC inner iterations */
 double gamma = config->gamma;/* rate to reduce the space of search at every iteration */
 register int i, j, k;
 int m, n;
 double e0, err;
 double *wbest;
 double delta = config->wmax - config->wmin;

 e0=1.e8; // just a big number

 wbest = malloc((nn->num_of_neurons*MAX_IN+1)*sizeof(*wbest));
 if(wbest==NULL){
  printf("MSMCO: Not enough memory to allocate\ndouble *wbest]\n");
  exit(0);
 }

 #pragma omp parallel for private(n,i,j,err)
 for (m = 0; m < mmax; m++) {
  for (n = 0; n < nmax; n++) {
   // random weights
   if (m == 0) {
    for (i = 0; i < nn->num_of_neurons; i++){
     for (j = 0; j < nn->neurons[i].num_input; ++j) {
      nn->neurons[i].w[j] = 0.5 * delta +
		(0.5 - rnd()) * 0.5 * delta ;
     }
    }
   } else {
    for (k = 0, i = 0; i < nn->num_of_neurons; i++)
     for (j = 0; j < nn->neurons[i].num_input; ++j, ++k)
      nn->neurons[i].w[j] = wbest[k] + (0.5 - rnd()) * 0.5 * delta * pow(gamma,m);
   }
   // update error
   err = error(nn, config);
   if (err < e0) {
    // update/store the new best weights
    e0 = err;
    for (k = 0, i = 0; i < nn->num_of_neurons; i++)
     for (j = 0; j < nn->neurons[i].num_input; ++j, ++k)
      wbest[k] = nn->neurons[i].w[j];
   }
  } // end of n-loop
  if (output==ON)
    printf("MSMCO: %d %g\n",m,e0);
 } // end of m-loop

 // update the weights of the network with the best found solution
 for (k = 0, i = 0; i < nn->num_of_neurons; i++)
  for (j = 0; j < nn->neurons[i].num_input; ++j, ++k) 
   nn->neurons[i].w[j] = wbest[k];

 free(wbest);
}
