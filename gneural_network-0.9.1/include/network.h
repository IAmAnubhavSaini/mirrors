/* network. -- This belongs to gneural_network

   gneural_network is the GNU package which implements a programmable neural network.

   Copyright (C) 2016 Jean Michel Sellier
   <jeanmichel.sellier@gmail.com>

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

#ifndef NETWORK_H
#define NETWORK_H

#include <defines.h>

typedef struct _neuron{
 unsigned int global_id;	/* a unique global id for each neuron */
 unsigned int num_input;	/* how many inputs the neuron has */
 // connection[i] is the identification number of the neuron which
 // output is connected to the i-th input branch of the neuron
 struct _neuron* *connection;	/* which neurons is connected */
 enum activation_function activation;		// type of activation function
 enum discriminant_function discriminant;	// type of discriminant function
// double x[MAX_IN]; // n inputs FMV: no more required...
 double *w;		// n weights
 double output;		// one output
} neuron;

typedef struct _layer {
   unsigned int num_of_neurons;   // number of neurons in the layers;
   neuron *neurons;               // pointer to the first neuron of the layers;
} layer;

typedef struct _network{
 unsigned int num_of_neurons;// total number of neurons
 unsigned int num_of_layers; // total number of layers
 layer *layers;
 neuron *neurons;
} network;

typedef struct _network_config {
  unsigned char verbosity;
  unsigned char initial_weights_randomization;
  enum optimization_method optimization_type;
  enum error_function error_type;

  unsigned char load_neural_network;
  char *load_network_file_name;

  unsigned char save_neural_network;
  char *save_network_file_name;

  unsigned char save_output;
  char *output_file_name;

  unsigned int num_of_points;
  double output_x[MAX_NUM_POINTS][MAX_NUM_NEURONS][MAX_IN];

  double rate;
  int nmax, mmax;
  int npop;
  int nxw;
  int maxiter;
  double accuracy;
  double gamma;
  double kbtmin, kbtmax;
  double wmin, wmax;

  /* training fields */
  unsigned int num_points;
  double points_x[MAX_TRAINING_POINTS][MAX_NUM_NEURONS][MAX_IN];
  double points_y[MAX_TRAINING_POINTS][MAX_NUM_NEURONS];
} network_config;
/*
 * network* API
 */

/*
 * network_alloc:
 * - alloc a network object
 */
network * network_alloc();
/*
 * network_free:
 * - free a network object
 */
void network_free(network *);

/*
 * network_run_algorithm:
 * - run a training algorith on a network
 */
void network_run_algorithm(network *, network_config *);

/*
 * network_set_neuron_number:
 * - set the number of neuron the network has and alloc the required memory
 */
int network_set_neuron_number(network *, unsigned long);
/*
 * network_set_layer_number:
 * - set the number of layers the network has and alloc the required memory
 */
int network_set_layer_number(network *, unsigned long);
/*
 * network_set_neuron_connection_number
 * -  set the the number of input a neuron has
 *    and alloc the required memory
 */
int network_neuron_set_connection_number(neuron* ne, unsigned int nr);

/*
 * network_set_neuron_connection_number
 * -  show the network topology
 */
void network_print(network *);

/*
 * network_config* API
 */

/*
 * network_config_alloc:
 * - alloc a network_config object
 */
network_config *network_config_alloc();

void network_config_set_default(network_config *);

network_config *network_config_alloc_default();
/*
 * network_config_free:
 * - free a network_config object
 */
void network_config_free(network_config *);
#endif
