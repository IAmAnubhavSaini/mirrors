/* network.c -- This belongs to gneural_network

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

// =================================================================
// File Name        : network.c
// Version          : release 0.9.0
// Creation         : 18 Apr. 2016, Catania (CT), Italy
// Last Revision    : 18 Apr. 2016, Catania (CT), Italy
// =================================================================

/*
   Programmable Neural Network. Every neuron consists of
   n-inputs and only one output. The activation
   and discriminant functions are defined by the user (see below).
   Every network is made of layers which forward propagate
   information from left to right. Every layer can have
   an arbitrary amount of neurons which are connected
   with each other.

   Every neuron has a global ID number and a local ID number
   (inside the layer). This simplifies the description of
   connections in a network.

*/

#ifdef _DEBUG
# define d_print(fmt, args...)  printf("%s: " fmt, __func__ , ## args)
#else
# define d_print(fmt, args...)
#endif

#include <network.h>
#include <defines.h>
#include <msmco.h>
#include <random_search.h>
#include <simulated_annealing.h>
#include <gradient_descent.h>
#include <genetic_algorithm.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * network* API
 */
network *network_alloc()
{
  network * nn = (network *)malloc(sizeof(*nn));
  if (nn)
	memset(nn, 0, sizeof(*nn));
  return nn;
};

static void _free_neuron_data(neuron* ne)
{
  if (!ne)
	return;

  if (ne->connection)
	free(ne->connection);

  if (ne->w)
	free(ne->w);
}

void network_free(network *nn)
{
 if (!nn)
	return;

 if (nn->neurons) {
	int i;

	for (i = 0; i < nn->num_of_neurons; ++i)
		_free_neuron_data(&nn->neurons[i]);

	free(nn->neurons);
	}

 if (nn->layers)
	free(nn->layers);

 free(nn);
}

int network_set_neuron_number(network *nn, unsigned long nr)
{
  int i;

  if (!nn || !nr)
	return -1;

  if (nr == nn->num_of_neurons) /* do nothing... */
	return 0;

  if (nn->neurons)
    	/* free the previous array if any */
	free(nn->neurons);

  nn->neurons = (neuron *)malloc(sizeof(neuron) * nr);
  if (!nn->neurons) {
	printf("No memory available to allocate the neurons array!\n");
	exit(-1);
  }

  memset(nn->neurons, 0, sizeof(neuron) * nr);
  nn->num_of_neurons = nr;

  for (i = 0; i < nr; ++i) {
	nn->neurons[i].global_id = i; /* set the global is once */
	nn->neurons[i].output = 0.0;
  }

  d_print("neurons array allocated: size for %d elements\n", nr);

  return 0;
}

int network_set_layer_number(network *nn, unsigned long nr)
{
  if (!nn || !nr)
	return -1;

  if (nr == nn->num_of_layers) /* do nothing... */
	return 0;

  if (nn->layers)
	/* free the previous array if any */
	free(nn->layers);

  nn->layers = (layer *) malloc(sizeof(layer) * nr);
  if (!nn->layers) {
	printf("No memory available to allocate the layers array!\n");
	exit(-1);
  }
  memset(nn->layers, 0, sizeof(layer) * nr);

  nn->num_of_layers = nr;
  d_print("layers array allocated: size for %d elements\n", nr);

  return 0;
}

int network_neuron_set_connection_number(neuron* ne, unsigned int nr)
{
  int i;
  if (!ne || !nr)
	return -1;

  if (ne->num_input == nr)
  	/* do nothing */
  	return 0;

  if (ne->connection)
  	free(ne->connection);
  if (ne->w)
	free(ne->w);

  ne->connection = (neuron **)malloc(sizeof(neuron *) * nr);
  if (!ne->connection) {
	printf("No memory available to allocate the connetcion array!\n");
	exit(-1);
  }

  memset(ne->connection, 0, sizeof(neuron *) * nr);

  ne->w = (double *)malloc(sizeof(double) * nr);
  if (!ne->w) {
	printf("No memory available to allocate the y array!\n");
	exit(-1);
  }

  for (i = 0; i < nr; ++i)
    ne->w[i] = 0.0;

  ne->num_input = nr;
  return 0;
}

/*
 * __network_check:
 * - check network consistensy
 */
static int __network_check(network *nn)
{
  int i, j, k;

  if (!nn) {
    printf("Network can not be null");
    return -1;
  }
  if (!nn->num_of_neurons) {
    printf("Error: Network with No neurons!!");
    return -1;
  }
  if (!nn->num_of_layers) {
    printf("Error: Network with No layers!!");
    return -1;
  }

  /* 1. check the total neurons in the layers are equal
   *    to the neuron in the network
   */
  for (j = 0, i = 0; i < nn->num_of_layers; ++i)
	j += nn->layers[i].num_of_neurons;
  if (j != nn->num_of_neurons) {
	printf("Error: The total neuron in the layers is not equal "
	    "to the total neurons the network has (%d vs %d)\n", j, nn->num_of_neurons);
	return -1;
  }

  /* 2. check the layers do not overlap and there is no hole
   */
  for (i = 0; i < nn->num_of_layers -1; ++i)
    if ((nn->layers[i].neurons + nn->layers[i].num_of_neurons) != nn->layers[i + 1].neurons) {
	printf("Error: There is layers error setting between layer %d and %d\n",
	    i, i + 1);
	return -1;
    }

  /*
   * 3. neuron connetction can not be NULL
   */
  for (i = 1; i < nn->num_of_layers; ++i)
   for (j = 0; j < nn->layers[i].num_of_neurons; ++j)
    for (k = 0; k < nn->layers[i].neurons[j].num_input; ++k)
     if (nn->layers[i].neurons[j].connection[k] == NULL) {
       printf("Error: Neuron %d has a NULL connection %d\n", nn->layers[i].neurons[j].global_id, j);
       return -1;
     }

  return 0;
}

void network_print(network *nn)
{
  int i, j, k;

  if (!nn)
    return;

  printf("========\nNetwork:\n");
  printf("-> Number of neurons: %d\n-> Number of layers: %d\n",
      nn->num_of_neurons, nn->num_of_layers);
  for (i = 0; i < nn->num_of_layers; ++i) {
	printf("-> Layer: #%d has %d neurons\n", i, nn->layers[i].num_of_neurons);
	if (i == 0) {
	  printf("    ");
	  for (j = 0; j < nn->layers[i].num_of_neurons; ++j)
	    printf("n[%d]\t", nn->layers[i].neurons[j].global_id);
	} else {
	  for (j = 0; j < nn->layers[i].num_of_neurons; ++j) {
	    printf("    ");
	    printf("n[%d] (%d inputs)\n",
		nn->layers[i].neurons[j].global_id, nn->layers[i].neurons[j].num_input);
	    for (k = 0; k < nn->layers[i].neurons[j].num_input; ++k)
		printf("\tn[%d] (%.2f)",
		  nn->layers[i].neurons[j].connection[k]->global_id,
		  nn->layers[i].neurons[j].w[k]);
	    printf("\n");
	  }
	}
	printf("\n");
  }
  printf("========\n");
}

void network_run_algorithm(network *nn, network_config *config)
{
  static const
  void (*supported_optimization_methods[])(network *, network_config *) = {
	[SIMULATED_ANNEALING]	= simulated_annealing,
	[RANDOM_SEARCH]		= random_search,
	[GRADIENT_DESCENT]	= gradient_descent,
	[GENETIC_ALGORITHM]	= genetic_algorithm,
	[MSMCO]			= msmco,
  };

  /*
   * Before the algorith... we check the network topology
   */
  if (__network_check(nn) < 0) {
    /* print the network topology in case of error... */
  	network_print(nn);
	exit(-1);
  }

  /* network_print(nn); */

  supported_optimization_methods[config->optimization_type](nn, config);
}

/*
 * network_config*  API
 */
network_config *network_config_alloc()
{
  network_config *config = (network_config *)malloc(sizeof(*config));
  if (config)
	memset(config, 0, sizeof(*config));
  return config;
}

void network_config_free(network_config *config)
{
  if (!config)
	return;

  if (config->load_network_file_name)
	free(config->load_network_file_name);

  if (config->save_network_file_name)
	free(config->save_network_file_name);

  free(config);
}

void network_config_set_default(network_config *config)
{
  // some defaults are defined before parsing the input script
  config->save_output = OFF;
  config->load_neural_network = OFF;
  config->save_neural_network = OFF;
  config->initial_weights_randomization = ON;
  config->error_type = L2;
}

network_config *network_config_alloc_default()
{
  network_config *config = network_config_alloc();
  if (config)
	network_config_set_default(config);
  return config;
}
