#include"Models.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables


void NG_Network_Model(bool PrintVars)
{
	auto start = chrono::high_resolution_clock::now();
#pragma region Fetch Data

	// set of possible existing plant types
	std::map<string, int> sym2pltType = { {"ng",0},{"dfo", 1},
{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7} };
	std::map<int, string> pltType2sym = { {0,"ng"},{1,"dfo"},
{2,"solar"},{3,"wind"},{4,"wind_offshore"},{5,"hydro"},{6,"coal"},{7,"nuclear"} };

	vector<gnode> Gnodes = Params::Gnodes;
	vector<pipe> PipeLines = Params::PipeLines;
	vector<enode> Enodes = Params::Enodes;
	vector<plant> Plants = Params::Plants;
	vector<eStore> Estorage = Params::Estorage;
	vector<branch> Branches = Params::Branches;
	int nEnode = Enodes.size();
	int nPlt = Plants.size();
	int nBr = Branches.size();
	int neSt = Estorage.size();
	int nPipe = PipeLines.size();
	vector<int> Tg = Params::Tg;
	vector<int> Te = Params::Te;
	vector<int> time_weight = Params::time_weight;
	float pi = 3.1415926535897;
	int nGnode = Gnodes.size();
	float WACC = Params::WACC;
	int trans_unit_cost = Params::trans_unit_cost;
	int trans_line_lifespan = Params::trans_line_lifespan;
	float NG_price = Params::NG_price;
	float dfo_pric = Params::dfo_pric;
	float coal_price = Params::coal_price;
	float E_curt_cost = Params::E_curt_cost;
	float G_curt_cost = Params::G_curt_cost;
	float pipe_per_mile = Params::pipe_per_mile;
	int pipe_lifespan = Params::pipe_lifespan;
	map<int, vector<int>> Lnm = Params::Lnm;
	vector<int> RepDaysCount = Params::RepDaysCount;
	float Emis_lim = Params::Emis_lim;
	float RPS = Params::RPS;
#pragma endregion

	IloEnv env;
	IloModel Model(env);
	int DV_size = 0;


#pragma region NG network DVs
	// NG vars
	NumVar2D supply(env, nGnode);
	NumVar2D curtG(env, nGnode);
	NumVar2D flowGG(env, nPipe);
	NumVar3D flowGE(env, nGnode);
	IloNumVarArray Zg(env, nPipe, 0, IloInfinity, ILOBOOL);
	for (int i = 0; i < nPipe; i++)
	{
		// uni-directional
		flowGG[i] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
	}
	for (int k = 0; k < nGnode; k++)
	{
		supply[k] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		curtG[k] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		//Zg[k] = IloNumVarArray(env, nGnode, 0, IloInfinity, ILOBOOL);
		DV_size += 2 * Tg.size() + nGnode;
		flowGE[k] = NumVar2D(env, nEnode);
		for (int n = 0; n < nEnode; n++)
		{
			flowGE[k][n] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
			DV_size += 2 * Tg.size();
		}
	}
#pragma endregion



	IloExpr exp0(env);

#pragma region NG network related costs
	for (int i = 0; i < PipeLines.size(); i++)
	{
		float s1 = std::pow(1.0 / (1 + WACC), pipe_lifespan);
		float capco = WACC / (1 - s1);
		//int fn = PipeLines[i].from_node;
		//int tn = PipeLines[i].to_node;
		float cost1 = PipeLines[i].length * pipe_per_mile;

		exp0 += capco * cost1 * Zg[i];
	}
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			exp0 += G_curt_cost * curtG[k][tau];
		}
	}
#pragma endregion

	Model.add(IloMinimize(env, exp0));

	exp0.end();


#pragma region NG Network Constraint
	// C1: flow limit for NG
	float ave_cap = 448;
	for (int i = 0; i < PipeLines.size(); i++)
	{
		int fn = PipeLines[i].from_node;
		int tn = PipeLines[i].to_node;
		int is_exist = PipeLines[i].is_exist;
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			if (is_exist == 1)
			{
				Model.add(flowGG[fn][tn][tau] <= PipeLines[i].cap + ave_cap * Zg[i]);
			}
			else
			{
				Model.add(flowGG[fn][tn][tau] <= PipeLines[i].cap * Zg[i]);
			}
		}
	}

	//C2: flow balance, NG node
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			IloExpr exp2(env);
			for (int kp : Gnodes[k].adjG) {
				exp2 += flowGG[k][kp][tau];
				//Model.add(flowGG[k][kp][tau] >= 0);
				Model.add(IloAbs(flowGG[k][kp][tau] + flowGG[kp][k][tau]) <= 1e-2);
			}

			IloExpr exp3(env);
			for (int n : Gnodes[k].adjE)
			{
				exp3 += flowGE[k][n][tau];
				Model.add(flowGE[k][n][tau]);
			}

			Model.add(supply[k][tau] - exp2 - exp3 + curtG[k][tau] == Gnodes[k].demG[tau]);
			//Model.add(supply[k][tau] - exp2 + curtG[k][tau] == Gnodes[k].demG[tau]);

		}
	}


	// C3,C4: injection (supply) limit and curtailment limit
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(supply[k][tau] >= Gnodes[k].injL);
			Model.add(supply[k][tau] <= Gnodes[k].injU);

			Model.add(curtG[k][tau] <= Gnodes[k].demG[tau]);
		}
	}


#pragma endregion






}