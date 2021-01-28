/* simulated_annealing.c -- This belongs to gneural_network

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

// train the neural network by means of simulated annealing to optimize the weights

#include <simulated_annealing.h>
#include <randomize.h>
#include <rnd.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void simulated_annealing(network *nn, network_config *config) {
 int output = config->verbosity;	/* screen output - on/off */
 int mmax = config->mmax;		/* outer loop - number of effective temperature steps*/
 int nmax = config->nmax;		/* inner loop - number of test configurations */
 double kbtmin = config->kbtmin;	/* effective temperature minimum */
 double kbtmax = config->kbtmax;	/* effective temperature maximum */
 double eps = config->accuracy;
 register int m,n;
 int i;
 double err;
 double e0;
 double de;
 double kbt;
 double e_best;
 double **w_total;
 double **wbackup;
 double **wbest;
 void *tmp;

 w_total = malloc(nn->num_of_neurons * sizeof(double*) * 2);

 if (w_total == NULL){
	printf("SA: Not enough memory to allocate\ndouble **w_total\n");
	exit(-1);
 }

 wbackup = w_total;
 wbest   = w_total + nn->num_of_neurons;

 for (i = 0; i < nn->num_of_neurons; ++i) {
	unsigned long size_ = nn->neurons[i].num_input * sizeof(double);
	wbackup[i] = malloc(size_);
	wbest[i]   = malloc(size_);
	if (!wbackup[i] || !wbest[i]) {
		printf("SA: Not enough memory to allocate\ndouble *wbackup/*wbest #%d\n", i);
		exit(-1);
	}
 }

 e_best = err = e0 = 1.e8; // just a big number

 for (m = 0;(m < mmax) && (e0 > eps); m++){

  kbt = kbtmax-m*(kbtmax-kbtmin)/(mmax-1);

  for (n = 0;(n < nmax) && (e0 > eps);n++){
   // backup the old weights
   /* for each neurons */
   for (i = 0; i < nn->num_of_neurons; i++) {
	/* for each input in the neuron swap the weight buffers */
	tmp = nn->neurons[i].w;
	nn->neurons[i].w = wbackup[i];
	wbackup[i] = tmp;
   }
   // new random configuration
   randomize(nn, config);
   // compute the error
   err = error(nn, config);
   // update energy landscape
   de = err - e0;
   // decides what configuration to keep
   if (de > 0.) {
    // if(de<=0.) just accept the new configuration
    // otherwise treat it probabilistically
    double p = exp(-e0/kbt);
    if (rnd() < p)
	// accept the new configuration
	e0 = err;
    else
     // reject the new configuration
     /* for each neurons */
     for (i = 0; i < nn->num_of_neurons; i++) {
	/* swap the weights */
	tmp = wbackup[i];
	wbackup[i] = nn->neurons[i].w;
	nn->neurons[i].w = tmp;
     }
   } else
	// accept the new configuration
	e0 = err;

   if (e_best > e0) {
    /* for each neurons */
    for (i = 0; i < nn->num_of_neurons; i++) {
      tmp = wbest[i];
      wbest[i] = nn->neurons[i].w;
      nn->neurons[i].w = tmp;
    }
    e_best = e0;
   }
  }
  if (output == ON)
    printf("SA: %d %g %g\n",m,kbt,e_best);
 }

 // keep the best solution found
 /* for each neurons */
 for (i = 0; i < nn->num_of_neurons; i++) {
   tmp = nn->neurons[i].w;
   nn->neurons[i].w = wbest[i];
   wbest[i] = tmp;
 }

  if (output == ON)
    printf("\n");
  for (i = 0; i < nn->num_of_neurons * 2; i++)
    free(w_total[i]);

   free(w_total);
}
