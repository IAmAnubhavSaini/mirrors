/* gneural_network.c -- This belongs to gneural_network

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

// =================================================================
// File Name        : gneural_network.c
// Version          : release 0.9.1
// Creation         : 11 Nov. 2012, Cassibile (SR), Italy
// Last Revision    : 03 May  2016, Cassibile (SR), Italy
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

#include <includes.h>
#include <defines.h>
#include <network.h>
#include <randomize.h>
#include <parser.h>
#include <load.h>
#include <save.h>

static const struct option longopts[] =
{
  { "version", no_argument, NULL, 'v' },
  { "help", no_argument, NULL, 'h' }
};

int main(int argc,char* argv[])
{
 int optc;
 int h=0,v=0,lose=0,z=0;
 char *progname;
 FILE *fp = NULL;

 network *nn = network_alloc();
 if (!nn)
	exit(-1);

 network_config *config = network_config_alloc_default();
 if (!config) {
	network_free(nn);
	exit(-1);
 }
 progname=argv[0];

 while((optc=getopt_long(argc,argv,"hv",longopts,(int *) 0))!= EOF)
  switch (optc){
   case 'v':
    v=1;
    break;
   case 'h':
    h=1;
    break;
   default:
    lose=1;
    break;
  }

  if(optind==argc-1) z=1;
  else if(lose || optind < argc){
   // dump an error message and exit.
   if (optind<argc) printf("Too many arguments\n");
   printf("Try `%s --help' for more information.\n",progname);
   exit(1);
  }

  // `help' should come first.  If `help' is requested, ignore the other options.
  if(h){
   /* Print help info and exit.  */
   /* TRANSLATORS: --help output 1
      no-wrap */
   printf("\
Gneural Network, the GNU package for neural networks.\nCopyright (C) 2016 Jean Michel Sellier.\n");
   printf ("\n");
   /* TRANSLATORS: --help output 2
      no-wrap */
   printf ("\
Usage: %s [OPTION] file...\n",progname);

   printf ("\n");
   /* TRANSLATORS: --help output 3 : options 1/2
      no-wrap */
   printf("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n");

   printf ("\n");
   /* TRANSLATORS: --help output 5 (end)
      TRANSLATORS, please don't forget to add the contact address for
      your translation!
      no-wrap */
   printf ("\
Report bugs to jeanmichel.sellier@gmail.com\n");
      exit (0);
    }

  if(v){
   /* Print version number.  */
   printf("gneural_network - Gneural Network 0.9.1\n");
   /* xgettext: no-wrap */
   printf("\n");
   printf("\
Copyright (C) %s Jean Michel Sellier.\n\n\
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.\n\
You may redistribute copies of GNU %s under the terms\n\
of the GNU General Public License.\n\
For more information about these matters, see the file named COPYING.\n",
"2016","Gneural Network");
   exit (0);
  }
  else if (z){
   // if the filename is specified then proceed with parsing the script and then run the calculations
   fp=fopen(argv[1],"r");
   // check, just in case the file does not exist...
   if(fp==NULL){
    printf("%s: fatal error in opening the input file %s\n",
           progname,argv[1]);
    exit(EXIT_FAILURE);
   }
   parser(nn, config, fp);
   fclose(fp);

   if (config->load_neural_network == OFF) {
    // assigns weights randomly for each neuron
    // before the training process
    if (config->initial_weights_randomization == ON)
     randomize(nn, config);

    // network training method
    network_run_algorithm(nn, config);

    if (config->save_neural_network == ON)
	network_save(nn, config);
   } else
	// load the neural network
	network_load(nn, config);

   if (config->save_output == ON){
    printf("saving the final curve\n");
    network_save_final_curve(nn, config);
   }
  } else {
   // filename not specified
   printf("%s: no input file\n",progname);
   exit(-1);
  }

 network_free(nn);
 network_config_free(config);
 return 0;
}
