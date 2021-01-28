/* save.c -- This belongs to gneural_network

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

#include "save.h"
#include "feedforward.h"
#include <stdio.h>
#include <stdlib.h>

void network_save(network *nn, network_config *config) {
 int output = config->verbosity; /* screen output - on/off */
 // saves all information related to the network
 int i,j;
 FILE *fp;

 fp = fopen(config->save_network_file_name,"w");
 if (fp == NULL) {
  printf("cannot save file %s!\n", config->save_network_file_name);
  exit(-1);
 }

 // saves the description of every single neuron
 fprintf(fp,"%d\n", nn->num_of_neurons); // total number of neurons
 for (i = 0; i < nn->num_of_neurons; i++){
  fprintf(fp,"%d\n",i); // neuron index
  fprintf(fp,"%d\n", nn->neurons[i].num_input); // number of input connections (weights)
  for (j = 0; j < nn->neurons[i].num_input; j++)
    fprintf(fp,"%g\n",nn->neurons[i].w[j]); // weights
  for (j = 0; j < nn->neurons[i].num_input; j++)
    fprintf(fp,"%d\n", nn->neurons[i].connection[j]->global_id); // connections to other neurons
  fprintf(fp,"%d\n", nn->neurons[i].activation); // activation function
  fprintf(fp,"%d\n", nn->neurons[i].discriminant); // discriminant function
 }

 // saves the network topology
 fprintf(fp,"%d\n", nn->num_of_layers); // total number of layers (including input and output layers)
 for (i = 0; i < nn->num_of_layers; i++){
  fprintf(fp,"%d\n", nn->layers[i].num_of_neurons); // total number of neurons in the i-th layer
  for (j = 0; j < nn->layers[i].num_of_neurons;j++)
   fprintf(fp,"%d\n", nn->layers[i].neurons[j].global_id); // global id neuron of every neuron in the i-th layer
 }

 fclose(fp);

 // screen output
 if(output==ON){
  printf("save(NETWORK);\n");
  // neuron descriptions
  printf("\nNEURONS\n=======\n");
  printf("NNUM = %d\n", nn->num_of_neurons); // total number of neurons
  for (i = 0; i < nn->num_of_neurons; i++){
   printf("\n=======\n");
   printf("NEURON[%d].nw = %d\n",i, nn->neurons[i].num_input); // number of input connections (weights)
   for (j = 0; j < nn->neurons[i].num_input; j++)
     printf("NEURON[%d].w[%d] = %g\n",i, j, nn->neurons[i].w[j]); // weights
   for (j = 0; j < nn->neurons[i].num_input; j++)
     printf("NEURON[%d].connection[%d] = %d\n",
	 i, j, nn->neurons[i].connection[j]->global_id); // connections to other neurons
   printf("NEURON[%d].activation = %d\n",
       i, nn->neurons[i].activation); // activation function
   printf("NEURON[%d].discriminant = %d\n",
       i, nn->neurons[i].discriminant); // discriminant function
   printf("=======\n");
  }
  // network topology
  printf("\nNETWORK\n=======\n");
  printf("NETWORK.num_of_layers = %d\n", nn->num_of_layers); // total number of layers (including input and output layers)
  for (i = 0; i < nn->num_of_layers; i++){
   printf("NETWORK.num_of_neurons[%d] = %d\n",
       i, nn->layers[i].num_of_neurons); // total number of neurons in the i-th layer
   for (j = 0; j < nn->layers[i].num_of_neurons; j++)
    printf("NETWORK.neuron_id[%d][%d] = %d\n",
	i, j, nn->layers[i].neurons[j].global_id); // global id neuron of every neuron in the i-th layer
  }
  printf("\n");
 }
}

void network_save_final_curve(network *nn, network_config *config)
{
  int n;
  FILE* fp = fopen(config->output_file_name, "w");

  if (fp == NULL) {
	printf("cannot save file %s!\n", config->output_file_name);
	exit(-1);
  }

  for (n = 0; n < config->num_of_points; n++) {
	int i, j;
	double y;
	/* for each neuron in layers[0] i.e: the input layer */
	for (i = 0; i < nn->layers[0].num_of_neurons; ++i) {
#if 0
	  nn->layers[0].neurons[i].output = config->output_x[n][i][0];
	  fprintf(fp,"%g ", nn->layers[0].neurons[i].output);
#else
	  for (j = 0; j < nn->layers[0].neurons[i].num_input; ++j) {
			nn->layers[0].neurons[i].output = config->output_x[n][i][j];
			fprintf(fp,"%g ", nn->layers[0].neurons[i].output);
		}
#endif
	}

	feedforward(nn);
	/* for each neuron in the last layer */
	for (i = 0; i < nn->layers[nn->num_of_layers -1].num_of_neurons; ++i) {
		y = nn->layers[nn->num_of_layers -1].neurons[i].output;
		fprintf(fp, "%g ", y);
	}
	fprintf(fp,"\n");
  }
  fclose(fp);
}

