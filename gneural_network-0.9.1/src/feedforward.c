/* feedforward.c -- This belongs to gneural_network

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

// feedforward propagation of information through the neural network

#include "feedforward.h"
#include "includes.h"
#include "defines.h"
#include "binom.h"
#include "fact.h"
#include "activation.h"
#include <math.h>

#define PI M_PI

void feedforward(network *nn){
 register int i,j;
 register int l,n;

 /*
  * scan all the layers (execpt the first)
  */
 for (l = 1; l < nn->num_of_layers; l++){
  /*
   * scan all the neurons in layer 'l'
   */
  for (n = 0; n < nn->layers[l].num_of_neurons; n++) {

   neuron *ne = &nn->layers[l].neurons[n];

   double x = 0.;
   double a, tmp;

   /* fmv no more required */
   //for (i=0;i<NEURON[id].nw;i++) NEURON[id].x[i]=NEURON[NEURON[id].connection[i]].output;
   switch (ne->discriminant) {
    case LINEAR:
     for (i = 0; i < ne->num_input; i++)
      x += ne->connection[i]->output * ne->w[i]; // linear product between w[] and x[]
     break;
    case LEGENDRE:
     for (i = 0; i < ne->num_input; i++) {
      for (tmp = 0., j = 0;j <= i; j++)
       	tmp += pow(ne->connection[i]->output,j)*binom(i,j)*binom((i+j-1)/2,j);
      tmp *= pow(2, i) * ne->w[i];
      x+=tmp;
     }
     break;
    case LAGUERRE:
     for (i = 0; i < ne->num_input; i++) {
      for (tmp = 0., j = 0;j <= i;j++)
	tmp += binom(i,j) * pow(ne->connection[i]->output, j)*pow(-1,j)/fact(j);
      tmp *= ne->w[i];
      x += tmp;
     }
     break;
    case FOURIER:
     for (i = 0; i < ne->num_input; i++) {
      for(tmp = 0., j = 0;j <= i; j++)
	tmp += sin(2. * j *PI *ne->connection[i]->output);
      tmp *= ne->w[i];
      x += tmp;
     }
     break;
    default:
     break;
   }
   ne->output = activation(ne->activation, x);
  }
 }
}
