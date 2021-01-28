/* randomize.c -- This belongs to gneural_network

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

// assigns weights randomly for each neuron

#include "randomize.h"
#include "rnd.h"

void randomize(network *nn, network_config *config){
 register int i, n;

 /* for each neuron in the network */
 for (n = 0; n < nn->num_of_neurons; n++)
  /* for each input in the neuron */
  for (i = 0; i < nn->neurons[n].num_input; i++) {
   nn->neurons[n].w[i] = config->wmin + rnd()*(config->wmax - config->wmin);
 }
}
