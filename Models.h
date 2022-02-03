#pragma once
#include"ProblemData.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables
void Electricy_Network_Model(bool int_vars_relaxed, bool PrintVars);
void NG_Network_Model(bool PrintVars);

double NGES_Model(bool int_vars_relaxed, bool PrintVars);
double LB_no_flow_lim_no_ramp(bool PrintVars, bool populate_inv_DVs, int** Xs, int** XestS, int** XdecS,double*** Ps);
double inv_vars_fixed_flow_equation_relaxed(bool PrintVars, int** Xs, int** XestS, int** XdecS,double*** Ps);
double UB_no_trans_consts(bool PrintVars, int** Xs, int** XestS, int** XdecS);
double UB_prods_fixed(bool PrintVars, double*** Ps);
double UB_X_var_given(bool PrintVars, int** Xs, int** XestS, int** XdecS);
double UB_X_prod_vars_given(bool PrintVars, int** Xs, int** XestS, int** XdecS, double*** Ps);