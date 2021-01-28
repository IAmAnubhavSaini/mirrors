/* parser.c -- This belongs to gneural_network

   gneural_network is the GNU package which implements a programmable neural network.

   Copyright (C) 2016 Jean Michel Sellier
   <jeanmichel.sellier@gmail.com>
   Copyright (C) 2016 Francesco Maria Virlinzi
   <francesco.virlinzi@gmail.com>

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

// read the input script to define the network and its task

#include <parser.h>
#include <string.h>
#include <stdlib.h>

enum main_token_id {
  _COMMENT,
  _NUMBER_OF_NEURONS,
  _NEURON,
  _NETWORK,
  _WEIGHT_MINIMUM,
  _WEIGHT_MAXIMUM,

  _LOAD_NEURAL_NETWORK,
  _SAVE_NEURAL_NETWORK,

  _ERROR_TYPE,
  _INITIAL_WEIGHTS_RANDOMIZATION,

  _NUMBER_OF_TRAINING_POINTS,
  _TRAINING_POINT,
  _TRAINING_METHOD,

  _NUMBER_OF_INPUT_POINTS,
  _NETWORK_INPUT,

  _SAVE_OUTPUT,
  _OUTPUT_FILE_NAME,
};


static const char *main_token_n[] = {
  [_COMMENT]				= "#",
  [_NUMBER_OF_NEURONS]			= "NUMBER_OF_NEURONS",
  [_NEURON]				= "NEURON",
  [_NETWORK]				= "NETWORK",
  [_WEIGHT_MINIMUM]			= "WEIGHT_MINIMUM",
  [_WEIGHT_MAXIMUM]			= "WEIGHT_MAXIMUM",
  [_LOAD_NEURAL_NETWORK]		= "LOAD_NEURAL_NETWORK",
  [_SAVE_NEURAL_NETWORK]		= "SAVE_NEURAL_NETWORK",
  [_ERROR_TYPE]				= "ERROR_TYPE",
  [_INITIAL_WEIGHTS_RANDOMIZATION]	= "INITIAL_WEIGHTS_RANDOMIZATION",
  [_NUMBER_OF_TRAINING_POINTS]		= "NUMBER_OF_TRAINING_POINTS",
  [_TRAINING_POINT]			= "TRAINING_POINT",
  [_TRAINING_METHOD]			= "TRAINING_METHOD",
  [_NUMBER_OF_INPUT_POINTS]		= "NUMBER_OF_INPUT_POINTS",
  [_NETWORK_INPUT]			= "NETWORK_INPUT",
  [_SAVE_OUTPUT]			= "SAVE_OUTPUT",
  [_OUTPUT_FILE_NAME]			= "OUTPUT_FILE_NAME",
};

enum direction_enum {
  _IN,
  _OUT,
};

static const char *direction_n[] = {
  [_IN]		= "IN",
  [_OUT]	= "OUT",
};

static const char *error_names[] = {
  [L2]	= "L2",
  [L1]	= "L1",
};

static int find_id(char *name, const char *type, const char **array, int last)
{
  int id;
  for (id = 0; id < last; ++id)
	if (strcmp(name, array[id]) == 0)
		return id;
  printf(" -> '%s' not supported as '%s' token\n", name, type);
  exit(-1);
}

static int get_positive_number(FILE *fp, const char *token_n)
{
  int ret, num;
  double tmp;

  ret = fscanf(fp, "%lf", &tmp);
  if (ret < 0)
    exit(-1);

  num = (int)(tmp);
  if (tmp != num) {
	printf("%s must be an integer number! (%f)\n", token_n, tmp);
	exit(-1);
  }
  if (num < 0) {
	printf("%s must be a positive number! (%d)\n ", token_n, num);
	exit(-1);
  }
  //printf(" -> %s = %d [OK]\n", token_n, num);
  return num;
}

static int get_strictly_positive_number(FILE *fp, const char *token_n)
{
  int num = get_positive_number(fp, token_n);
  if (num == 0) {
	printf("%s must be a strictly positive number! (%d)\n ", token_n, num);
	exit(-1);
  }
  return num;
}

static const char *switch_n[] = {
  [OFF] = "OFF",
  [ON]  = "ON",
};

static int get_switch_value(FILE *fp, const char *name)
{
  int ret;
  char s[128];
  ret = fscanf(fp, "%s", s);
  if (ret < 0)
    exit(-1);
  return find_id(s, name, switch_n, array_size(switch_n));
}

static double get_double_number(FILE *fp)
{
  double tmp;
  int ret = fscanf(fp, "%lf", &tmp);
  if (ret < 0)
    exit(-1);
  return tmp;
}

static double get_double_positive_number(FILE *fp, const char *msg)
{
  double tmp = get_double_number(fp);
  if (tmp > 0)
	return tmp;
  printf("%s must be greater than 0!", msg);
  exit(-1);
}

/*
 * sub_neuron_parser: parse the neuron attribute
 * fp: pointer to the script file
 */
static void sub_neuron_parser(network *nn, network_config *config, FILE *fp)
{
  enum neuron_sub_token_id {
	NUMBER_OF_CONNECTIONS,
	ACTIVATION,
	DISCRIMINANT,
	CONNECTION,
  };
  static const char *neuron_sub_token_n[] = {
	[NUMBER_OF_CONNECTIONS] = "NUMBER_OF_CONNECTIONS",
	[ACTIVATION]		= "ACTIVATION",
	[DISCRIMINANT]		= "DISCRIMINANT",
	[CONNECTION]		= "CONNECTION",
  };

  static const char *discriminant_names[] = {
	[LINEAR]		= "LINEAR",
	[LEGENDRE]		= "LEGENDRE",
	[LAGUERRE]		= "LAGUERRE",
	[FOURIER]		= "FOURIER",
  };

  static const char *activation_names[] = {
	[TANH]			= "TANH",
	[EXP]			= "EXP",
	[ID]			= "ID",
	[POL1]			= "POL1",
	[POL2]			= "POL2",
  };

  int sub_token, sub_num, index, ret;
  char s[128];

  index = get_positive_number(fp, "NEURON");
  if (index > (nn->num_of_neurons -1)) {
	printf("neuron id out of range!\n");
	exit(-1);
  };

  ret = fscanf(fp, "%s", s);
  if (ret < 0)
    exit(-1);
  sub_token = find_id(s, "NEURON sub-token",
	neuron_sub_token_n, array_size(neuron_sub_token_n));

  switch (sub_token) {
  case NUMBER_OF_CONNECTIONS: {
	sub_num = get_positive_number(fp, neuron_sub_token_n[sub_token]);
	network_neuron_set_connection_number(&nn->neurons[index], sub_num);
	printf("NEURON %d NUMBER_OF_CONNECTIONS = %d [OK]\n", index, sub_num);
	}
	break;
  case ACTIVATION: {
	ret = fscanf(fp, "%s", s);
	int activ = find_id(s, neuron_sub_token_n[sub_token],
		activation_names, array_size(activation_names));
	nn->neurons[index].activation = activ;
	printf("NEURON %d ACTIVATION = %s\n", index, activation_names[activ]);
	}
	break;
  case DISCRIMINANT: {
	ret = fscanf(fp, "%s", s);
	int discr = find_id(s,  neuron_sub_token_n[sub_token],
		discriminant_names, array_size(discriminant_names));
	nn->neurons[index].discriminant = discr;
	printf("NEURON %d DISCRIMINANT = %s\n", index, discriminant_names[discr]);
	};
	break;
  case CONNECTION: {
	int connection_id = get_positive_number(fp, neuron_sub_token_n[sub_token]);
	if (connection_id > (nn->num_of_neurons -1)) {
		printf("the connection index is out of range!\n");
		exit(-1);
	}
	if (connection_id > (nn->neurons[index].num_input-1) ) {
		printf("the connection index is out of range!\n");
		exit(-1);
	}

	int global_neuron_id_2 = get_positive_number(fp, neuron_sub_token_n[sub_token]);
	if (global_neuron_id_2 > (nn->num_of_neurons -1)) {
		printf("the global index of neuron #2 is out of range!\n");
		exit(-1);
	}
	printf("NEURON %d CONNECTION %d %d [OK]\n",
		index, connection_id, global_neuron_id_2);
	nn->neurons[index].connection[connection_id] = &nn->neurons[global_neuron_id_2];
	};
	break;
	/* close NEURON sub-case; */
  }
}

static void sub_network_parser(network *nn, network_config *config, FILE *fp) {
  enum network_sub_token_id {
	NUMBER_OF_LAYERS,
	LAYER,
	ASSIGN_NEURON_TO_LAYER,
  };
  static const char *network_sub_token_n[] = {
	[NUMBER_OF_LAYERS]      = "NUMBER_OF_LAYERS",
	[LAYER]                 = "LAYER",
	[ASSIGN_NEURON_TO_LAYER]= "ASSIGN_NEURON_TO_LAYER",
  };
  int ret, sub_token;
  char s[128];

  /*
   * parser NETWORK Sub-Token
   */
  ret = fscanf(fp, "%s", s);
  if (ret < 0)
    exit(-1);
  sub_token = find_id(s, "NETWORK sub-token",
	network_sub_token_n, array_size(network_sub_token_n));
  switch (sub_token) {
  case NUMBER_OF_LAYERS: {
	int num_layers = get_positive_number(fp, "NUMBER_OF_LAYERS");
	network_set_layer_number(nn, num_layers);
	printf("NETWORK NUMBER_OF_LAYERS = %d [OK]\n", num_layers);
	}
	break;
  // specify the number of neurons of a layer
  // syntax: NETWORK LAYER ind NUMBER_OF_NEURONS num
  case LAYER: {
	int ind = get_positive_number(fp, "LAYER");
	if (ind > (nn->num_of_layers - 1)) {
		printf("layer index is out of range!\n");
		exit(-1);
	}
	ret = fscanf(fp, "%s", s);
	if (strcmp(s,"NUMBER_OF_NEURONS") != 0) {
		printf("syntax error!\nNUMBER_OF_NEURONS expected!\n");
		exit(-1);
	}
	int num = get_positive_number(fp, "NUMBER_OF_NEURONS");
	if (num > nn->num_of_neurons) {
		printf("the number of neurons in the layer is grater the the total number of neurons!\n");
		printf("please check your configuration!\n");
		exit(-1);
	}
	nn->layers[ind].num_of_neurons = num;
	printf("NETWORK LAYER %d NUMBER_OF_NEURONS %d [OK]\n", ind, num);
	}
	break;
  // assigns the neurons to the layers
  // syntax: NETWORK ASSIGN_NEURON_TO_LAYER layer_id local_neuron_id global_neuron_id
  case ASSIGN_NEURON_TO_LAYER: {
	int layer_id = get_positive_number(fp, "ASSIGN_NEURON_TO_LAYER 1.");
	if (layer_id > (nn->num_of_layers - 1)) {
		printf("layer index out of range!\n");
		exit(-1);
	}
	int local_neuron_id = get_positive_number(fp, "ASSIGN_NEURON_TO_LAYER 2.");
	if (local_neuron_id > (nn->layers[layer_id].num_of_neurons -1)) {
		printf("local neuron index out of range!\n");
		exit(-1);
	}
	int global_neuron_id = get_positive_number(fp, "ASSIGN_NEURON_TO_LAYER 3.");
	if (global_neuron_id > (nn->num_of_neurons -1)) {
		printf("global neuron index out of range!\n");
		exit(-1);
	}
	printf("NETWORK ASSIGN_NEURON_TO_LAYER %d %d %d [OK]\n",
		layer_id, local_neuron_id, global_neuron_id);
	/* assign only the first-one */
	if (!nn->layers[layer_id].neurons)
	  nn->layers[layer_id].neurons = &nn->neurons[global_neuron_id];
	};
	break;
  default:
	printf("the specified network feature is unknown!\n");
	exit(-1);
	break;
  }

}

static void sub_training_method_parser(network *nn, network_config *config, FILE *fp)
{
  static const char *sub_method_token_n[] = {
	[SIMULATED_ANNEALING]   = "SIMULATED_ANNEALING",
	[RANDOM_SEARCH]         = "RANDOM_SEARCH",
	[GRADIENT_DESCENT]      = "GRADIENT_DESCENT",
	[GENETIC_ALGORITHM]     = "GENETIC_ALGORITHM",
	[MSMCO]                 = "MSMCO",
  };

  int ret, method_id;
  char s[128];
  ret = fscanf(fp, "%s", s);
  if (ret < 0)
    exit(-1);

  method_id = find_id(s, "TRAINING_METHOD", sub_method_token_n, array_size(sub_method_token_n));
  switch (method_id) {
  case SIMULATED_ANNEALING: {
	// simulated annealing
	// syntax: verbosity mmax nmax kbtmin kbtmax accuracy
	// where
	// verbosity = ON/OFF
	// mmax      = outer loop - number of effective temperature steps
	// nmax      = inner loop - number of test configurations
	// kbtmin    = effective temperature minimum
	// kbtmax    = effective temperature maximum
	// accuracy  = numerical accuracy
	int verbosity = get_switch_value(fp, "verbosity");
	int mmax = get_positive_number(fp, "simulated annealing mmax");
	if (mmax < 2) {
		printf("MMAX must be greater than 1!\n");
		exit(-1);
	}
	int nmax = get_positive_number(fp, "simulated annealing nmax");
	double kbtmin = get_double_number(fp);
	double kbtmax = get_double_number(fp);;
	if (kbtmin >= kbtmax) {
		printf("KBTMIN must be smaller then KBTMAX!\n");
		exit(-1);
	}
	double eps = get_double_positive_number(fp, "ACCURACY");
	printf("TRAINING METHOD = SIMULATED ANNEALING %d %d %g %g %g [OK]\n",
		mmax, nmax, kbtmin, kbtmax, eps);
	config->optimization_type = SIMULATED_ANNEALING;
	config->verbosity = verbosity;
	config->mmax = mmax;
	config->nmax = nmax;
	config->kbtmin = kbtmin;
	config->kbtmax = kbtmax;
	config->accuracy = eps;
	};
	break;
  // random search
  // syntax: verbosity nmax accuracy
  // verbosity = ON/OFF
  // nmax      = maximum number of random attempts
  // accuracy  = numerical accuracy
  case RANDOM_SEARCH: {
	int verbosity = get_switch_value(fp, "verbosity");
	int nmax = get_positive_number(fp, "random search nmax");
	double eps = get_double_positive_number(fp, "ACCURACY");
	printf("OPTIMIZATION METHOD = RANDOM SEARCH %d %g [OK]\n",nmax,eps);
	config->verbosity = verbosity;
	config->optimization_type = RANDOM_SEARCH;
	config->nmax = nmax;
	config->accuracy = eps;
	}
	break;
  // gradient descent
  // syntax: verbosity nxw maxiter gamma accuracy
  // where
  // verbosity = ON/OFF
  // nxw       = number of cells in one direction of the weight space
  // maxiter   = maximum number of iterations
  // gamma     = step size
  // accuracy  = numerical accuracy
  case GRADIENT_DESCENT: {
	int verbosity = get_switch_value(fp, "verbosity");
	int nxw      = get_positive_number(fp, "gradient descent nxw");
	int maxiter  = get_positive_number(fp, "gradient descent MAXITER");
	double gamma = get_double_positive_number(fp, "GAMMA");
	double eps = get_double_positive_number(fp, "ACCURACY");
	printf("OPTIMIZATION METHOD = GRADIENT DESCENT %d %d %g %g [OK]\n",
		nxw, maxiter, gamma, eps);
	config->verbosity = verbosity;
	config->optimization_type = GRADIENT_DESCENT;
	config->nxw = nxw;
	config->maxiter = maxiter;
	config->gamma = gamma;
	config->accuracy = eps;
	};
	break;
  // genetic algorithm
  // syntax: verbosity nmax npop rate accuracy
  // where:
  // verbosity = ON/OFF
  // nmax      = number of generations
  // npop      = number of individuals per generation
  // rate      = rate of change between one generation and the parent
  // accuracy  = numerical accuracy
  case GENETIC_ALGORITHM: {
	int verbosity = get_switch_value(fp, "verbosity");
	int nmax = get_positive_number(fp, "genetic algorithm nmax");
	int npop = get_positive_number(fp, "genetic algorithm npop");
	double rate = get_double_positive_number(fp, "RATE");
	double eps = get_double_positive_number(fp, "ACCURACY");
	printf("OPTIMIZATION METHOD = GENETIC ALGORITHM %d %d %g %g [OK]\n",
		nmax, npop, rate, eps);
	config->verbosity = verbosity;
	config->optimization_type = GENETIC_ALGORITHM;
	config->nmax = nmax;
	config->npop = npop;
	config->rate = rate;
	config->accuracy = eps;
	};
	break;
  // multi-scale Monte Carlo algorithm
  // syntax: verbosity mmax nmax rate
  // where:
  // verbosity = ON/OFF
  // mmax      = number of Monte Carlo outer iterations
  // nmax      = number of MC inner iterations
  // rate      = rate of change of the space of search at each iteration
  case MSMCO: {
	int verbosity = get_switch_value(fp, "verbosity");
	int mmax = get_positive_number(fp, "multi-scale Monte Carlo mmax");
	int nmax = get_positive_number(fp, "multi-scale Monte Carlo nmax");
	double rate = get_double_positive_number(fp, "RATE");
	printf("OPTIMIZATION METHOD = MULTI-SCALE MONTE CARLO OPTIMIZATION %d %d %g [OK]\n",
		mmax, nmax, rate);
	config->verbosity = verbosity;
	config->optimization_type = MSMCO;
	config->mmax = mmax;
	config->nmax = nmax;
	config->rate = rate;
	};
	break;
  default:
	break;
  }
}

void parser(network *nn, network_config *config, FILE *fp){
 int ret;
 char s[180];
 double tmp;
 unsigned int token_id;

 printf("\n\
=========================\n\
processing the input file\n\
=========================\n");
 do{
  // read the current row
  ret = fscanf(fp,"%s",s);
  if (ret < 0)
	return;

  token_id = find_id(s, "Token", main_token_n, array_size(main_token_n));

  switch (token_id) {

  case _COMMENT:
	fgets(s, 80, fp);
	printf("COMMENT ---> %s", s);
	break;

  case _NUMBER_OF_NEURONS:
	network_set_neuron_number(nn,
	    get_strictly_positive_number(fp, "TOTAL NUMBER OF NEURONS"));
	printf("TOTAL NUMBER OF NEURONS = %d [OK]\n", nn->num_of_neurons);
	break;

  case _NEURON:
	sub_neuron_parser(nn, config, fp);
	break;

  case _NETWORK:
	sub_network_parser(nn, config, fp);
	break;

  case _NUMBER_OF_TRAINING_POINTS: {
	int npoint = get_strictly_positive_number(fp, main_token_n[token_id]);
	if (npoint > MAX_TRAINING_POINTS) {
		printf("NUMBER_OF_TRAINING_POINTS is too large!\n");
		printf("please increase MAX_TRAINING_POINTS and recompile!\n");
		exit(-1);
		}
	printf("NUMBER_OF_TRAINING_POINTS = %d [OK]\n", npoint);
	config->num_points = npoint;
	}
	break;

  // specify the training points for supervised learning
  // syntax: TRAINING_POINT IN point_index neuron_index _connection_index value
  // syntax: TRAINING_POINT OUT point_index neuron_index value
  case _TRAINING_POINT: {
	ret = fscanf(fp, "%s", s);
	int direction = find_id(s, main_token_n[token_id],
		direction_n, array_size(direction_n));
	switch (direction) {
	case _IN: {
		int ind = get_positive_number(fp, "training data index");

		if (ind > (config->num_points -1)) {
			printf("training data index out of range!\n");
			exit(-1);
			}
		int neu = get_positive_number(fp, "TRAINING_POINT neuron index");

		if (neu > (nn->num_of_neurons -1)) {
			printf("TRAINING_POINT IN neuron index out of range!\n");
			exit(-1);
			}

		int conn = get_positive_number(fp, "TRAINING_POINT connection index");
		if (conn > (nn->neurons[neu].num_input -1)) {
			printf("TRAINING_POINT connection index out of range!\n");
			exit(-1);
		}
		tmp = get_double_number(fp);
		printf("TRAINING_POINT IN %d %d %d %f [OK]\n",
			ind, neu, conn, tmp);
		config->points_x[ind][neu][conn] = tmp;
		};
		break;
	case _OUT: {
		int ind = get_positive_number(fp, "training data index");
		if (ind > (config->num_points -1)) {
			printf("training data index out of range!\n");
			exit(-1);
			}
		int neu = get_positive_number(fp, "TRAINING_POINT OUT neuron index");
		if (neu > (nn->num_of_neurons -1)) {
			printf("TRAINING_POINT OUT neuron index out of range!\n");
			exit(-1);
			}
		tmp = get_double_number(fp);
		printf("TRAINING_POINT OUT %d %d %f [OK]\n", ind, neu, tmp);
		config->points_y[ind][neu] = tmp;
		}
	} /* close switch (direction) */
	};
	break;

    case _WEIGHT_MINIMUM:
	config->wmin = get_double_number(fp);;
	printf("WEIGHT_MINIMUM = %f [OK]\n", config->wmin);
	break;

    case _WEIGHT_MAXIMUM:
	config->wmax = get_double_number(fp);
	printf("WEIGHT_MAXIMUM = %f [OK]\n", config->wmax);
	break;
    // specify the training method
    // syntax: TRAINING_METHOD method values (see below)..
    case _TRAINING_METHOD:
	sub_training_method_parser(nn, config, fp);
	break;

  // specify if some output has to be saved
  // syntax: SAVE_OUTPUT ON/OFF
  case _SAVE_OUTPUT:
	config->save_output = get_switch_value(fp, "save_output");
	printf("SAVE_OUTPUT %s [OK]\n", switch_n[config->save_output]);
	break;
  
  // specify the output file name
  // syntax: OUTPUT_FILE_NAME filename
  case _OUTPUT_FILE_NAME: {
	ret = fscanf(fp, "%s", s);
	config->output_file_name = malloc(strlen(s) + 1);
	strcpy(config->output_file_name, s);
   	printf("OUTPUT FILE NAME = %s [OK]\n", config->output_file_name);
  	break;
  // specify the number of points for the output file
  // syntax: NUMBER_OF_INPUT_POINTS num
  case _NUMBER_OF_INPUT_POINTS: {
	int num = get_strictly_positive_number(fp, "NUMBER_OF_INPUT_POINTS");
	if (num > MAX_NUM_POINTS) {
		printf("NUMBER_OF_INPUT_POINTS is too large!\n");
		printf("please increase MAX_NUM_POINTS and recompile!\n");
		exit(-1);
		}
	printf("NUMBER OF INPUT POINTS = %d [OK]\n", num);
	config->num_of_points = num;
	};
  	break;
  // specify the input points for the output file
  // syntax: NETWORK_INPUT point_id neuron_id conn_id val
  case _NETWORK_INPUT: {
	int num = get_positive_number(fp, "NETWORK_INPUT point index");
	if (num > (config->num_of_points - 1)) {
		printf("NETWORK_INPUT point index out of range!\n");
		exit(-1);
		}

	int neu = get_positive_number(fp, "NETWORK_INPUT neuron index");
	if (neu > nn->layers[0].num_of_neurons - 1) {
		printf("NETWORK_INPUT neuron index out of range!\n");
		exit(-1);
		}
	int conn = get_positive_number(fp, "NETWORK_INPUT connection index");
	if (conn > (nn->neurons[neu].num_input -1)) {
		printf("NETWORK_INPUT connection index out of range!\n");
		exit(-1);
		}
	double val = get_double_number(fp);
	printf("NETWORK INPUT POINT #%d %d %d = %g [OK]\n",
		num, neu, conn, val);
	config->output_x[num][neu][conn]=val;
	};
	break;
  // save a neural network (structure and weights) in the file network.dat
  // at the end of the training process
  // syntax: SAVE_NEURAL_NETWORK
  case _SAVE_NEURAL_NETWORK: {
	ret = fscanf(fp, "%s", s);
	config->save_network_file_name = malloc(strlen(s) + 1);
	strcpy(config->save_network_file_name, s);
	config->save_neural_network = ON;
	printf("SAVE NEURAL NETWORK to %s [OK]\n", s);
  	};
	break;

  // load a neural network (structure and weights) from the file network.dat
  // at the begining of the training process
  // syntax: LOAD_NEURAL_NETWORK
  case _LOAD_NEURAL_NETWORK: {
	ret = fscanf(fp, "%s", s);
	config->load_network_file_name = malloc(strlen(s) + 1);
	strcpy(config->load_network_file_name, s);
	config->save_neural_network = ON;
	printf("LOAD NEURAL NETWORK from %s [OK]\n", s);
	};
	break;

  // perform a random initialization of the weights
  // syntax: INITIAL_WEIGHTS_RANDOMIZATION ON/OFF
  case _INITIAL_WEIGHTS_RANDOMIZATION: {
	int flag = get_switch_value(fp, main_token_n[token_id]);
	config->initial_weights_randomization = flag;
	printf("INITIAL_WEIGHTS_RANDOMIZATION = %s [OK]\n", switch_n[flag]);
	};
	break;
  // specify the error function for the training process
  // syntax: ERROR_TYPE L2/L1
  case _ERROR_TYPE: {
	ret = fscanf(fp, "%s", s);
	int errtype = find_id(s, main_token_n[token_id],
		error_names, array_size(error_names));
	config->error_type = errtype;
	printf("ERROR_TYPE = %s [OK]\n", error_names[errtype]);
	};
	break;
    } /* close switch(token_id) */
  }
  sprintf(s,""); // empty the buffer
 } while (!feof(fp));
}
