/* genetic_algorithm.c -- This belongs to gneural_network

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

// performs a genetic search of the best weights during the training process

// initially implemented by : Jean Michel Sellier
// drastically improved by  : Nan

#include <genetic_algorithm.h>
#include <error.h>
#include <rnd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

void memswap(void* a,void* b,size_t size){
 char t[size];
 // suppose a and b never overlapped.
 if (a == b)
   return;
 memcpy(t, a, size);
 memcpy(a, b, size);
 memcpy(b, t, size);
}

void* partition(void* data,size_t element_count,size_t element_size,__compar_fn_t cmp_func){
 void* pivot =data + (element_count-1)*element_size;
 void* i = data;
 void* j = data;
 while (j < pivot){
  if (cmp_func(j, pivot) < 0){
   memswap(i, j, element_size);
   i += element_size;
  }
  j += element_size;
 }
 memswap(i, pivot, element_size);
 return i;
}

void par_qsort(void* data,size_t element_count,size_t element_size,__compar_fn_t cmp_func){
 if (element_count < PAR_QSORT_LOW_LIMIT) {
  qsort(data, element_count, element_size, cmp_func);
 } else {
  void* pivot=partition(data,element_count,element_size,cmp_func);
  size_t left=(pivot-data)/element_size;
  size_t right=element_count-left-1;
  #pragma omp parallel sections firstprivate(data,element_count,element_size,cmp_func,pivot,left,right)
  {
   #pragma omp section
   par_qsort(data,left,element_size,cmp_func);
   #pragma omp section
   par_qsort(pivot+element_size,right,element_size,cmp_func);
  }
 }
}

typedef struct{
 double error;
 double* weights;
} individual_t;

int actual_weight_count(network *nn){
 int i, j, k;

 for (k = 0, i = 0; i < nn->num_of_neurons; ++i)
  for (j = 0;j < nn->neurons[i].num_input; ++j, ++k);

 return k;
}

static void crossover(network_config *config,
    double w1,
    double w2,
    double* n1,
    double* n2){
 static int POSS = 1;
 double average = (w1 + w2) / 2;
 double delta = (config->wmax - config->wmin) / 2;
 double mid = (config->wmax + config->wmin) / 2;

 *n1 = average;
 if (average > mid)
  *n2 = average - delta;
 else if (average < mid)
  *n2 = average + delta;
 else { /* == mid*/
  if (POSS) {
   *n2 = config->wmax;
   POSS = 0;
  } else {
   *n2 = config->wmin;
   POSS = 1;
  }
 }
}

static void mutation(network_config *config,
    double* weight,
    double rate){

 double delta = config->wmax - config->wmin;

 if (rnd() > rate)
   return;
 if (rnd() > 0.5){
  /* go plus */
  *weight += (rnd() * delta / 2);
  if (*weight > config->wmax)
    *weight -= delta;
 } else {
  /* go minus */
  *weight -= (rnd() * delta / 2);
  if (*weight < config->wmin)
   *weight += delta;
 }
}

static int individual_compare(const void* a,const void* b){
 individual_t* ia = *(individual_t**)a;
 individual_t* ib = *(individual_t**)b;

 if (ia->error > ib->error)
   return 1;

 if (ia->error < ib->error)
   return -1;

 return 0;
}

static void init_individuals(unsigned long weight_cout,
    individual_t** individuals, int size){
 int n, k;
 #pragma omp parallel for shared(weight_cout, individuals) private(n, k)
 for (n = 0; n < size; ++n)
  /* for each neuron in the network... */
  for (k = 0; k < weight_cout; k++)
    individuals[n]->weights[k] = rnd();
}

static void reproduce_next_generation(
    network_config *config,
    individual_t** individuals,
    int size,
    int weight_cout,
    double rate){
 int pos = size;
 int i, j, k;
 #pragma omp parallel for shared(config, pos,individuals,size,weight_cout,rate) private(i,j,k)
 for (i = 0; i < size; ++i) {
  for (j = i + 1; j < size; ++j) {
   for (k = 0; k < weight_cout; ++k) {
    double w1 = individuals[i]->weights[k];
    double w2 = individuals[j]->weights[k];
    crossover(config, w1,w2,individuals[pos]->weights+k,individuals[pos+1]->weights+k);
    mutation(config, individuals[pos]->weights+k,rate);
    mutation(config, individuals[pos+1]->weights+k,rate);
   }
   #pragma omp atomic
   pos+=2;
  }
 }
}

static void selection(
    network *nn,
    network_config *config,
    individual_t** individuals,int size){
 int pool_size=size*size;
 int n;
 #pragma omp parallel for shared(individuals,nn,config) private(n)
 for (n = 0; n < pool_size; ++n) {
  #pragma omp critical
  {
   int i, j, k;
   /* for each neuron */
   for (k = 0, i = 0; i < nn->num_of_neurons; i++)
    /* for each input...*/
    for (j = 0; j < nn->neurons[i].num_input; j++, ++k)
     nn->neurons[i].w[j] = individuals[n]->weights[k];

   individuals[n]->error = error(nn, config);
  }
 }
 par_qsort(individuals,pool_size,sizeof(individual_t*),individual_compare);
// qsort(individuals, pool_size, sizeof(individual_t*), individual_compare);
}

void genetic_algorithm(network *nn, network_config *config) {
 int output = config->verbosity;	/* screen output - on/off */
 int nmax =   config->nmax;		/* number of generations */
 int npop =   config->npop;		/* number of individuals per generation */
 int rate =   config->rate;		/* rate of change between one generation and the parent */
 int eps  =   config->accuracy;		/* numerical accuracy */

 int i,j,k,n;

 int pool_size=npop*npop;
 individual_t** individuals=malloc(pool_size*sizeof(individual_t*));
 if(individuals==NULL){
  printf("GA: Not enough memory to allocate individual index table\n");
  exit(-1);
 }

 for(i=0;i<pool_size;++i){
  individuals[i]=malloc(sizeof(individual_t));
  if(individuals[i]==NULL){
   printf("GA: Not enough memory to allocate individuals\n");
   exit(-1);
  }
  individuals[i]->weights=malloc(nn->num_of_neurons*MAX_IN*sizeof(double));
  if(individuals[i]->weights==NULL){
   printf("GA: Not enough memory to allocate weights\n");
   exit(-1);
  }
 }

 int weight_cout = actual_weight_count(nn);
 init_individuals(weight_cout, individuals, npop);

 for (n = 0; n < nmax; ++n) {

  reproduce_next_generation(config, individuals,npop,weight_cout,rate);

  selection(nn, config, individuals,npop);

  if (output == ON)
    printf("GA2: %d %.12g\n", n, individuals[0]->error);
  if (individuals[0]->error<eps)
    break;
 }

 if (individuals[0]->error>eps && output == ON)
    printf("GA2: after %d iterations error still greater than %g\n", nmax, eps);

 /* for each neuron */
 for (k = 0, i = 0; i < nn->num_of_neurons; i++)
  /* for each input ...*/
  for (j = 0; j < nn->neurons[i].num_input; j++, ++k)
   nn->neurons[i].w[j] = individuals[0]->weights[k];

 for (i = 0; i < pool_size; ++i) {
  free(individuals[i]->weights);
  free(individuals[i]);
 }
 free(individuals);
}
