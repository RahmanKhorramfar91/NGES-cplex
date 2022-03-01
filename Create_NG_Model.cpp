#include"Models_Funcs.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables


IloNumVarArray GV::Xstr;
IloNumVarArray GV::Xvpr;
NumVar2D GV::Sstr;
NumVar2D GV::Svpr;
NumVar2D GV::Sliq;
NumVar2D GV::supply;
NumVar2D GV::curtG;
NumVar2D GV::flowGG;
NumVar3D GV::flowGE;
NumVar3D GV::flowGL;
NumVar3D GV::flowVG;
IloNumVarArray GV::Zg;

IloNumVar GV::strInv_cost;
IloNumVar GV::pipe_cost;
IloNumVar GV::gSshedd_cost;
IloNumVar GV::gFixVar_cost;
IloNumVar GV::NG_import_cost;


void Populate_GV(IloModel& Model, IloEnv& env)
{

#pragma region Fetch Data

	vector<gnode> Gnodes = Params::Gnodes;
	vector<pipe> PipeLines = Params::PipeLines;
	vector<enode> Enodes = Params::Enodes;
	vector<plant> Plants = Params::Plants;
	vector<eStore> Estorage = Params::Estorage;
	vector<exist_gSVL> Exist_SVL = Params::Exist_SVL;
	int nSVL = Exist_SVL.size();
	vector<SVL> SVLs = Params::SVLs;
	vector<branch> Branches = Params::Branches;
	int nEnode = Enodes.size();
	int nPlt = Plants.size();
	int nBr = Branches.size();
	int neSt = Estorage.size();
	int nPipe = PipeLines.size();
	vector<int> Tg = Params::Tg;
	//vector<int> Te = Params::Te;
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
	map<int, vector<int>> Le = Params::Le;
	map<int, vector<int>> Lg = Params::Lg;
	vector<int> RepDaysCount = Params::RepDaysCount;
	float Emis_lim = Params::Emis_lim;
	float RPS = Params::RPS;
#pragma endregion


#pragma region NG network DVs

	// NG vars
	GV::Xstr = IloNumVarArray(env, nSVL, 0, IloInfinity, ILOFLOAT);
	GV::Xvpr = IloNumVarArray(env, nSVL, 0, IloInfinity, ILOFLOAT);

	GV::Sstr = NumVar2D(env, nSVL);
	GV::Svpr = NumVar2D(env, nSVL);
	GV::Sliq = NumVar2D(env, nSVL);
	GV::supply = NumVar2D(env, nGnode);
	GV::curtG = NumVar2D(env, nGnode);
	GV::flowGG = NumVar2D(env, nPipe);
	GV::flowGE = NumVar3D(env, nGnode);
	GV::flowGL = NumVar3D(env, nGnode);
	GV::flowVG = NumVar3D(env, nSVL);
	GV::Zg = IloNumVarArray(env, nPipe, 0, IloInfinity, ILOBOOL);
	GV::strInv_cost = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	GV::pipe_cost = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	GV::gSshedd_cost = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	GV::gFixVar_cost = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
	GV::NG_import_cost = IloNumVar(env, 0, IloInfinity, ILOFLOAT);



	for (int i = 0; i < nPipe; i++)
	{
		// uni-directional
		GV::GV::flowGG[i] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
	}
	for (int j = 0; j < nSVL; j++)
	{
		GV::GV::Sstr[j] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		GV::GV::Svpr[j] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		GV::GV::Sliq[j] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		GV::GV::flowVG[j] = NumVar2D(env, nGnode);
		for (int k = 0; k < nGnode; k++)
		{
			GV::GV::flowVG[j][k] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		}
	}

	for (int k = 0; k < nGnode; k++)
	{
		GV::GV::supply[k] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		GV::GV::curtG[k] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		GV::GV::flowGE[k] = NumVar2D(env, nEnode);
		GV::GV::flowGL[k] = NumVar2D(env, nSVL);
		for (int j = 0; j < nSVL; j++)
		{
			GV::GV::flowGL[k][j] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);

		}
		for (int n = 0; n < nEnode; n++)
		{
			GV::GV::flowGE[k][n] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		}
	}
#pragma endregion

}

void NG_Model(IloModel& Model, IloEnv& env, IloExpr& exp_GVobj)
{

#pragma region Fetch Data

	vector<gnode> Gnodes = Params::Gnodes;
	vector<pipe> PipeLines = Params::PipeLines;
	vector<enode> Enodes = Params::Enodes;
	vector<plant> Plants = Params::Plants;
	vector<eStore> Estorage = Params::Estorage;
	vector<exist_gSVL> Exist_SVL = Params::Exist_SVL;
	int nSVL = Exist_SVL.size();
	vector<SVL> SVLs = Params::SVLs;
	vector<branch> Branches = Params::Branches;
	int nEnode = Enodes.size();
	int nPlt = Plants.size();
	int nBr = Branches.size();
	int neSt = Estorage.size();
	int nPipe = PipeLines.size();
	vector<int> Tg = Params::Tg;
	//vector<int> Te = Params::Te;
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
	map<int, vector<int>> Le = Params::Le;
	map<int, vector<int>> Lg = Params::Lg;
	vector<int> RepDaysCount = Params::RepDaysCount;
	float Emis_lim = Params::Emis_lim;
	float RPS = Params::RPS;
#pragma endregion


#pragma region NG network related costs
	//	IloExpr exp_GVobj(env);
	IloExpr exp_NG_import_cost(env);
	IloExpr exp_invG(env);
	IloExpr exp_pipe(env);
	IloExpr exp_curt(env);
	IloExpr exp_FixVar(env);
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			exp_NG_import_cost += GV::GV::supply[k][tau] * 0;
		}
	}
	for (int j = 0; j < nSVL; j++)
	{
		float s1 = std::pow(1.0 / (1 + WACC), 25);//check 25 later
		float capco = WACC / (1 - s1);
		exp_invG += capco * (SVLs[0].Capex * GV::Xstr[j] + SVLs[1].Capex * GV::Xvpr[j]);
	}
	for (int i = 0; i < nPipe; i++)
	{
		float s1 = std::pow(1.0 / (1 + WACC), pipe_lifespan);
		float capco = WACC / (1 - s1);
		float cost1 = PipeLines[i].length * pipe_per_mile;

		exp_pipe += capco * cost1 * GV::Zg[i];
	}
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			exp_curt += time_weight[tau] * G_curt_cost * GV::curtG[k][tau];
		}
	}

	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			exp_FixVar += SVLs[0].FOM * GV::Sstr[j][tau] + SVLs[1].FOM * GV::Svpr[j][tau];
		}
	}

	exp_GVobj += exp_NG_import_cost + exp_invG + exp_pipe + exp_curt + exp_FixVar;

	Model.add(GV::strInv_cost == exp_invG);
	Model.add(GV::pipe_cost == exp_pipe);
	Model.add(GV::gSshedd_cost == exp_curt);
	Model.add(GV::gFixVar_cost == exp_FixVar);
	Model.add(GV::NG_import_cost == exp_NG_import_cost);
#pragma endregion

	//Model.add(IloMinimize(env, exp0));

	//exp0.end();


#pragma region NG Network Constraint
	// C1, C2: flow limit for NG
	for (int i = 0; i < nPipe; i++)
	{
		int is_exist = PipeLines[i].is_exist;
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			if (is_exist == 1)
			{
				Model.add(GV::flowGG[i][tau] <= PipeLines[i].cap);
			}
			else
			{
				Model.add(GV::flowGG[i][tau] <= PipeLines[i].cap * GV::Zg[i]);
			}
		}
	}

	//C3: flow balance, NG node
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			IloExpr exp_gg_exp(env);
			IloExpr exp_gg_imp(env);
			IloExpr exp_ge_flow(env);
			IloExpr exp_store(env);
			for (int l : Gnodes[k].Lexp) { exp_gg_exp += GV::flowGG[l][tau]; }
			for (int l : Gnodes[k].Limp) { exp_gg_imp += GV::flowGG[l][tau]; }


			for (int n : Gnodes[k].adjE) { exp_ge_flow += GV::flowGE[k][n][tau]; }
			for (int j : Gnodes[k].adjS)
			{
				exp_store += GV::flowVG[j][k][tau] - GV::flowGL[k][j][tau];
			}

			//Model.add(exp_ge_flow == 0);
			//Model.add(GV::supply[k][tau] + exp_gg_flow - exp_store + GV::curtG[k][tau] == Gnodes[k].demG[Tg[tau]] + Gnodes[k].out_dem);
			Model.add(GV::supply[k][tau] - exp_gg_exp + exp_gg_imp - exp_ge_flow + exp_store + GV::curtG[k][tau] == Gnodes[k].demG[Tg[tau]]);
		}
	}


	// C3,C4: injection (supply) limit and curtailment limit
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(GV::supply[k][tau] >= Gnodes[k].injL);
			Model.add(GV::supply[k][tau] <= Gnodes[k].injU);

			Model.add(GV::curtG[k][tau] <= Gnodes[k].demG[Tg[tau]]);
		}
	}

	//C5: storage balance
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			if (tau == 0)
			{
				//Model.add(GV::Sstr[j][tau] == Exist_SVL[j].store_cap*0.5);
				Model.add(GV::Sstr[j][tau] == Exist_SVL[j].store_cap * 0 + GV::Sliq[j][tau] - GV::Svpr[j][tau] / SVLs[1].eff_disCh);
				//Model.add(GV::Sstr[j][tau] == GV::Sliq[j][tau] - GV::Svpr[j][tau] / SVLs[1].eff_disCh);
				continue;
			}
			Model.add(GV::Sstr[j][tau] == (1 - SVLs[0].BOG) * GV::Sstr[j][tau - 1] + GV::Sliq[j][tau] - GV::Svpr[j][tau] / SVLs[1].eff_disCh);
		}
	}

	//C6: calculate Sliq
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			IloExpr exp(env);
			// get the ng nodes adjacent to SVL j
			vector<int> NG_adj;
			for (int k = 0; k < nGnode; k++)
			{
				for (int j2 : Gnodes[k].adjS)
				{
					if (j2 == j)
					{
						NG_adj.push_back(k);
					}
				}
			}
			for (int k : NG_adj)
			{
				exp += GV::flowGL[k][j][tau];
			}
			Model.add(GV::Sliq[j][tau] == exp);
		}
	}

	//C6: Sliq limit
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(GV::Sliq[j][tau] <= Exist_SVL[j].liq_cap);
		}
	}

	//C7: calculate Svpr
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			IloExpr exp(env);
			// get the ng nodes adjacent to SVL j
			vector<int> NG_adj;
			for (int k = 0; k < nGnode; k++)
			{
				for (int j2 : Gnodes[k].adjS)
				{
					if (j2 == j)
					{
						NG_adj.push_back(k);
					}
				}
			}
			for (int k : NG_adj)
			{
				exp += GV::flowVG[j][k][tau];
			}
			Model.add(GV::Svpr[j][tau] == exp);
		}
	}

	//C8: Svpr limit
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(GV::Svpr[j][tau] <= Exist_SVL[j].vap_cap + GV::Xvpr[j]);
		}
	}

	//C8: Sstr limit
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(GV::Sstr[j][tau] <= Exist_SVL[j].store_cap + GV::Xstr[j]);
		}
	}

#pragma endregion

}




