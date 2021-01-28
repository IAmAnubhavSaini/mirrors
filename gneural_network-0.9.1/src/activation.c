/* activation.c -- This belongs to gneural_network

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

// returns the specified activation function given the input x

#include <activation.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

inline double activation(enum activation_function type,double x){
 switch (type) {
  case TANH:
   return tanh(x);
   break;
  case EXP:
   return 1./(1. + exp(-x));
   break;
  case ID:
   return x;
   break;
  case POL1:
   return 1. + x;
   break;
  case POL2:
   return 1. + x + x * x ;
   break;
  default:
   printf("unknown activation function!\n");
   exit(-1);
 }
}

