/* error.c -- This belongs to gneural_network

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

// compute the specified error of a (training) neural network

#include <error.h>
#include <math.h>
#include <feedforward.h>

double error(network *nn, network_config *config){
 register int n;
 double err;
 double y;

 err = 0.;

 switch (config->error_type) {
 // L1 norm
 case L1:
   err = -1.e8;
   for (n = 0; n < config->num_points; n++) {
    int i, j;
    // assign training input
    for (i = 0; i < nn->layers[0].num_of_neurons; i++) {
     neuron *ne = &nn->layers[0].neurons[i];
#if 1
     ne->output = config->points_x[n][ne->global_id][0];
#else
     for (j = 0; j < ne->num_input; j++) 
      ne->output = config->points_x[n][i][j];
#endif
    }
    feedforward(nn);
    // compute the L1 error comparing with training output
    double tmp = 0.;
    for (j = 0; j < nn->layers[nn->num_of_layers-1].num_of_neurons; j++){
     neuron *ne = &nn->layers[nn->num_of_layers-1].neurons[j];
     y = ne->output;
     tmp += fabs(y-config->points_y[n][ne->global_id]);
    }
    err += tmp;
   }
   return err;

   break;
  // L2 norm
  case L2:
   for (n = 0; n < config->num_points; n++){
    int i, j;
    // assign training input
    for (i = 0; i < nn->layers[0].num_of_neurons; i++){
     neuron *ne = &nn->layers[0].neurons[i];
#if 1
     ne->output = config->points_x[n][ne->global_id][0];
#else
     for (j = 0; j < ne->num_input; j++)
      ne->output = config->points_x[n][i][j];
#endif
    }
    feedforward(nn);
    // compute the L2 error comparing with the training output
    double tmp = 0.;
    for (j = 0; j < nn->layers[nn->num_of_layers-1].num_of_neurons; j++){
     neuron *ne = &nn->layers[nn->num_of_layers-1].neurons[j];
     y = ne->output;;
     tmp += pow(y - config->points_y[n][ne->global_id], 2);
    }
    err += tmp;
   }
   return sqrt(err);
   break;
  default:
   return 0.;
   break;
 }
}
