/* binom.c -- This belongs to gneural_network

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

// returns the binomial coefficient of two numbers (n over k)

#include <binom.h>

// originally programmed by : Jean Michel Sellier
// improved by              : Gergo Barany

inline int binom(int n,int k){
 double res=1;
 register int i;
 for(i=1;i<=k;i++){
  res*=(double)(n+1-i)/i;
 }
 return(round(res));
}
