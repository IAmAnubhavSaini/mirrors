/* defines.h -- This belongs to gneural_network

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
#ifndef _DEFINE_H
#define _DEFINE_H

// returns the size (in particular, the number of elements) of an array
#define array_size(foo) (sizeof(foo)/sizeof(foo[0]))

// maximum number of training point
#define MAX_TRAINING_POINTS 8

// maximum allowed number of input connections per neuron
#define MAX_IN 16

// maximum total number of neurons
#define MAX_NUM_NEURONS 32

// maximum number of layers
#define MAX_NUM_LAYERS 16

// maximum number of points in output file
#define MAX_NUM_POINTS 64

// low limit for the genetic algorithm
#define PAR_QSORT_LOW_LIMIT 1024

// definition of various internal types
enum switch_flag {
	OFF = (0 == 1),
	ON = (1 == 1),
};

// activation functions
enum activation_function {
	TANH,
	EXP,
	ID,
	POL1,
	POL2,
};

// discriminant functions
enum discriminant_function {
	LINEAR,
	LEGENDRE,
	LAGUERRE,
	FOURIER,
};

// error/cost/target functions
enum error_function {
	L2,
	L1,
};

// optimization methods
enum optimization_method {
	SIMULATED_ANNEALING,
	RANDOM_SEARCH,
	GRADIENT_DESCENT,
	GENETIC_ALGORITHM,
	MSMCO,
};

#endif
