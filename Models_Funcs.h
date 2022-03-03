#pragma once
#include"ProblemData.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables
void Electricy_Network_Model();
void NG_Network_Model();

double NGES_Model();

double DGSP();
double DESP();

double XiBounds2();


void Read_rep_days(string name, vector<int>& Rep, vector<int>& RepCount);


void Populate_EV(IloModel& Model, IloEnv& env);
void Elec_Model(IloModel& Model, IloEnv& env, IloExpr& exp_Eobj);
void Populate_GV(IloModel& Model, IloEnv& env);
void NG_Model(IloModel& Model, IloEnv& env, IloExpr& exp_GVobj);

void Coupling_Constraints(IloModel& Model, IloEnv& env);

void Print_EV(IloCplex cplex, double obj, double gap, double Elapsed_time);
void Print_GV(IloCplex cplex, double obj, double gap, double Elapsed_time);



struct Setting
{
	static bool print_E_vars;
	static bool print_NG_vars;
	static bool relax_int_vars;

	static bool is_xi_given;
	static double xi_val;
	static bool xi_UB_obj;
	static bool xi_LB_obj;
	static float cplex_gap;
	static float CPU_limit;

	static int Num_rep_days;
	static float Emis_lim;
	static float RPS;
	static bool Approach_1_active;
	static bool Approach_2_active;

	static bool DGSP_active;
	static bool DESP_active;
};


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
	static IloNumVar e_system_cost;
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
	static IloNumVar gShedd_cost;
	static IloNumVar gStrFOM_cost;
	static IloNumVar NG_import_cost;
	static IloNumVar NG_system_cost;
};

struct CV
{
	static IloNumVar xi; // flow from NG to E network (NG consumed by NG-fired plants)
	static IloNumVar NG_emis; // emission from NG network
	static IloNumVar E_emis; // emission from Electricity network. 


	static float used_emis_cap; // eta value from the DGSP of the Approach 2
};