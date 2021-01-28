/* load.c -- This belongs to gneural_network

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

#include "load.h"
#include <stdio.h>
#include <stdlib.h>

void network_load(network *nn, network_config *config) {
 int output = config->verbosity;/* screen output - on/off */
 // load a network that has been previously saved
 int i,j;
 int ret;
 double tmp;
 FILE *fp;

 fp = fopen(config->load_network_file_name,"r");
 if (fp == NULL) {
  printf("cannot open file %s!\n", config->load_network_file_name);
  exit(-1);
 }

 // saves the description of every single neuron
 ret = fscanf(fp, "%lf\n", &tmp); // total number of neurons

 /* set the neuron number and alloc the required memory */
 network_set_neuron_number(nn, (unsigned int)tmp);
 
 for (i = 0; i < nn->num_of_neurons; i++) {
  neuron *ne = &nn->neurons[i];
  ret = fscanf(fp, "%lf\n", &tmp); // neuron index - useless
  ret = fscanf(fp, "%lf\n", &tmp); // number of input connections (weights)

  ne->num_input = (int)(tmp);

  for (j = 0; j < ne->num_input; j++) {
   ret = fscanf(fp,"%lf\n",&tmp);
   ne->w[j] = tmp;		// set the 'j-th'weight in the 'i-th' neuron
  }

  for (j = 0;j < ne->num_input; j++) {
   ret = fscanf(fp, "%lf\n", &tmp); // connections to other neurons
   ne->connection[j] = &nn->neurons[(int)(tmp)]; 
//   NEURON[i].connection[j]=(int)(tmp);
  }
  ret = fscanf(fp, "%lf\n", &tmp); // activation function
  ne->activation = (int)(tmp);

  ret=fscanf(fp,"%lf\n",&tmp); // discriminant function
  ne->discriminant = (int)(tmp);
 }

 // saves the network topology
 ret = fscanf(fp, "%lf\n", &tmp); // total number of layers (including input and output layers)
 network_set_layer_number(nn, (int)(tmp));

 for (i = 0; i < nn->num_of_layers; i++) {
  layer *le = &nn->layers[i];
  ret = fscanf(fp, "%lf\n", &tmp); // total number of neurons in the i-th layer
  le->num_of_neurons = (unsigned long) tmp;

  for (j = 0; j < le->num_of_neurons; j++) {
   ret = fscanf(fp,"%lf\n",&tmp); // global id neuron of every neuron in the i-th layer
   if (!j) /* set only the first neuron in the layer... the following neuron are contigues... */
     le->neurons = &nn->neurons[(int)tmp];
//   NETWORK.neuron_id[i][j]=(int)(tmp);
  }
 }

 fclose(fp);

 // screen output
 if (output == ON) {
  printf("load(NETWORK);\n");
  // neuron descriptions
  printf("\nNEURONS\n=======\n");
  printf("NNUM = %d\n", nn->num_of_neurons); // total number of neurons
  for (i = 0; i < nn->num_of_neurons; i++) {
   neuron *ne = &nn->neurons[i];
   printf("\n=======\n");
   printf("NEURON[%d].nw = %d\n",i, ne->num_input); // number of input connections (weights)
   for (j = 0; j < ne->num_input; j++)
	printf("NEURON[%d].w[%d] = %g\n", i, j, ne->w[j]); // weights

   for (j = 0; j < ne->num_input; j++)
     printf("NEURON[%d].connection[%d] = %d\n",
	 i, j, ne->connection[j]->global_id); // connections to other neurons
   printf("NEURON[%d].activation = %d\n", i, ne->activation); // activation function
   printf("NEURON[%d].discriminant = %d\n", i, ne->discriminant); // discriminant function
   printf("=======\n");
  }
  // network topology
  printf("\nNETWORK\n=======\n");
  printf("NETWORK.num_of_layers = %d\n", nn->num_of_layers); // total number of layers (including input and output layers)
  for (i = 0; i < nn->num_of_layers; i++) {
   layer *le = &nn->layers[i];
   printf("NETWORK.num_of_neurons[%d] = %d\n", i, le->num_of_neurons); // total number of neurons in the i-th layer
   for(j = 0; j < le->num_of_neurons; j++)
    printf("NETWORK.neuron_id[%d][%d] = %d\n", i, j, le->neurons[j].global_id); // global id neuron of every neuron in the i-th layer
  }
  printf("\n");
 }
}
