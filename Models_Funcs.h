#pragma once
#include"ProblemData.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables
void Electricy_Network_Model(bool int_vars_relaxed, bool PrintVars);
void NG_Network_Model(bool PrintVars);

double NGES_Model(bool int_vars_relaxed, bool PrintVars);
double LB_no_flow_lim_no_ramp(bool PrintVars, bool populate_inv_DVs, int** Xs, int** XestS, int** XdecS, double*** Ps);
double inv_vars_fixed_flow_equation_relaxed(bool PrintVars, int** Xs, int** XestS, int** XdecS, double*** Ps);
double UB_no_trans_consts(bool PrintVars, int** Xs, int** XestS, int** XdecS);
double UB_prods_fixed(bool PrintVars, double*** Ps);
double UB_X_var_given(bool PrintVars, int** Xs, int** XestS, int** XdecS);
double UB_X_prod_vars_given(bool PrintVars, int** Xs, int** XestS, int** XdecS, double*** Ps);

void Read_rep_days(string name, vector<int>& Rep, vector<int>& RepCount);


void Populate_EV(bool int_vars_relaxed, IloModel& Model, IloEnv& env);
void Elec_Model(IloModel& Model, IloEnv& env);
void Populate_GV(IloModel& Model, IloEnv& env);
void NG_Model(IloModel& Model, IloEnv& env, IloExpr& exp_GVobj);

void Print_EV(IloCplex cplex, double obj, double gap, double Elapsed_time);
void Print_GV(IloCplex cplex, double obj, double gap, double Elapsed_time);

struct EV
{
	static 	NumVar2D Xest; // integer (continues) for plants
	static	NumVar2D Xdec; // integer (continues) for plants
	static	NumVar2D YeCD; // continuous: charge/discharge capacity
	static	NumVar2D YeLev; // continuous: charge/discharge level
	static NumVar2D YeStr;
	static	IloNumVarArray Ze;
	static	NumVar2D theta; // continuous phase angle
	static	NumVar2D curtE; // continuous curtailment variable
	static	NumVar3D prod;// continuous
	static	NumVar3D eSch;// power charge to storage 
	static	NumVar3D eSdis;// power discharge to storage
	static	NumVar3D eSlev;// power level at storage
	static	NumVar2D Xop; // integer (continues)
	static	NumVar2D flowE; // unlike the paper, flowE subscripts are "ntm" here
	static	IloNumVar est_cost;
	static	IloNumVar decom_cost;
	static	IloNumVar fixed_cost;
	static	IloNumVar var_cost;
	static	IloNumVar thermal_fuel_cost;
	//IloNumVar emis_cost;
	static	IloNumVar shedding_cost;
	static	IloNumVar elec_storage_cost;
	static	IloNumVar Emit_var;
};

struct GV
{

	static IloNumVarArray Xstr;
	static IloNumVarArray Xvpr;
	static NumVar2D Sstr;
	static NumVar2D Svpr;
	static NumVar2D Sliq;
	static NumVar2D supply;
	static NumVar2D curtG;
	static NumVar2D flowGG;
	static NumVar3D flowGE;
	static NumVar3D flowGL;
	static NumVar3D flowVG;
	static IloNumVarArray Zg;

	static IloNumVar strInv_cost;
	static IloNumVar pipe_cost;
	static IloNumVar gSshedd_cost;
	static IloNumVar gFixVar_cost;
	static IloNumVar NG_import_cost;
};
