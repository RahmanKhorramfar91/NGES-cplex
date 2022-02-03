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

	IloEnv env;
	IloModel Model(env);
	int DV_size = 0;


#pragma region NG network DVs

	// NG vars
	IloNumVarArray Xstr(env, nSVL, 0, IloInfinity, ILOFLOAT);
	IloNumVarArray Xvpr(env, nSVL, 0, IloInfinity, ILOFLOAT);

	NumVar2D Sstr(env, nSVL);
	NumVar2D Svpr(env, nSVL);
	NumVar2D Sliq(env, nSVL);
	NumVar2D supply(env, nGnode);
	NumVar2D curtG(env, nGnode);
	NumVar2D flowGG(env, nPipe);
	NumVar3D flowGE(env, nGnode);
	NumVar3D flowGL(env, nGnode);
	NumVar3D flowVG(env, nSVL);
	IloNumVarArray Zg(env, nPipe, 0, IloInfinity, ILOBOOL);
	for (int i = 0; i < nPipe; i++)
	{
		// uni-directional
		flowGG[i] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
	}
	for (int j = 0; j < nSVL; j++)
	{
		Sstr[j] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		Svpr[j] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		Sliq[j] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		flowVG[j] = NumVar2D(env, nGnode);
		for (int k = 0; k < nGnode; k++)
		{
			flowVG[j][k] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		}
	}

	for (int k = 0; k < nGnode; k++)
	{
		supply[k] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		curtG[k] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		DV_size += 2 * Tg.size() + nGnode;
		flowGE[k] = NumVar2D(env, nEnode);
		flowGL[k] = NumVar2D(env, nSVL);
		for (int j = 0; j < nSVL; j++)
		{
			flowGL[k][j] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);

		}
		for (int n = 0; n < nEnode; n++)
		{
			flowGE[k][n] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
			DV_size += 2 * Tg.size();
		}
	}
#pragma endregion


	IloExpr exp0(env);

#pragma region NG network related costs
	IloExpr exp_NG_import_cost(env);
	IloExpr exp_invG(env);
	IloExpr exp_pipe(env);
	IloExpr exp_curt(env);
	IloExpr exp_FixVar(env);
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			exp_NG_import_cost += supply[k][tau] * 0;
		}
	}
	for (int j = 0; j < nSVL; j++)
	{
		float s1 = std::pow(1.0 / (1 + WACC), 25);//check 25 later
		float capco = WACC / (1 - s1);
		exp_invG += capco * (SVLs[0].Capex * Xstr[j] + SVLs[1].Capex * Xvpr[j]);
	}
	for (int i = 0; i < nPipe; i++)
	{
		float s1 = std::pow(1.0 / (1 + WACC), pipe_lifespan);
		float capco = WACC / (1 - s1);
		float cost1 = PipeLines[i].length * pipe_per_mile;

		exp_pipe += capco * cost1 * Zg[i];
	}
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			exp_curt += time_weight[tau] * G_curt_cost * curtG[k][tau];
		}
	}

	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			exp_FixVar += SVLs[0].FOM * Sstr[j][tau] + SVLs[1].FOM * Svpr[j][tau];
		}
	}

	exp0 += exp_NG_import_cost + exp_invG + exp_pipe + exp_curt + exp_FixVar;
	IloNumVar strInv_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar pipe_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar gSshedd_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar gFixVar_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar NG_import_cost(env, 0, IloInfinity, ILOFLOAT);
	Model.add(strInv_cost == exp_invG);
	Model.add(pipe_cost == exp_pipe);
	Model.add(gSshedd_cost == exp_curt);
	Model.add(gFixVar_cost == exp_FixVar);
	Model.add(NG_import_cost == exp_NG_import_cost);
#pragma endregion

	Model.add(IloMinimize(env, exp0));

	exp0.end();


#pragma region NG Network Constraint
	// C1, C2: flow limit for NG
	for (int i = 0; i < nPipe; i++)
	{
		int is_exist = PipeLines[i].is_exist;
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			if (is_exist == 1)
			{
				Model.add(flowGG[i][tau] <= PipeLines[i].cap);
			}
			else
			{
				Model.add(flowGG[i][tau] <= PipeLines[i].cap * Zg[i]);
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
			for (int l :Gnodes[k].Lexp){ exp_gg_exp += flowGG[l][tau]; }
			for (int l : Gnodes[k].Limp) { exp_gg_imp += flowGG[l][tau]; }
				
			
			for (int n : Gnodes[k].adjE) { exp_ge_flow += flowGE[k][n][tau]; }
			for (int j : Gnodes[k].adjS) { exp_store += flowVG[j][k][tau] - flowGL[k][j][tau]; }

			//Model.add(exp_ge_flow == 0);
			//Model.add(supply[k][tau] + exp_gg_flow - exp_store + curtG[k][tau] == Gnodes[k].demG[Tg[tau]] + Gnodes[k].out_dem);
			Model.add(supply[k][tau]- exp_gg_exp+ exp_gg_imp - exp_ge_flow + exp_store + curtG[k][tau] == Gnodes[k].demG[Tg[tau]]);
		}
	}


	// C3,C4: injection (supply) limit and curtailment limit
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(supply[k][tau] >= Gnodes[k].injL);
			Model.add(supply[k][tau] <= Gnodes[k].injU);

			Model.add(curtG[k][tau] <= Gnodes[k].demG[Tg[tau]]);
		}
	}

	//C5: storage balance
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			if (tau == 0)
			{
				//Model.add(Sstr[j][tau] == Exist_SVL[j].store_cap*0.5);
				Model.add(Sstr[j][tau] == Exist_SVL[j].store_cap * 0 + Sliq[j][tau] - Svpr[j][tau] / SVLs[1].eff_disCh);
				//Model.add(Sstr[j][tau] == Sliq[j][tau] - Svpr[j][tau] / SVLs[1].eff_disCh);
				continue;
			}
			Model.add(Sstr[j][tau] == (1-SVLs[0].BOG) * Sstr[j][tau - 1] + Sliq[j][tau] - Svpr[j][tau] / SVLs[1].eff_disCh);
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
				exp += flowGL[k][j][tau];
			}
			Model.add(Sliq[j][tau] == exp);
		}
	}

	//C6: Sliq limit
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(Sliq[j][tau] <= Exist_SVL[j].liq_cap);
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
				exp += flowVG[j][k][tau];
			}
			Model.add(Svpr[j][tau] == exp);
		}
	}

	//C8: Svpr limit
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(Svpr[j][tau] <= Exist_SVL[j].vap_cap + Xvpr[j]);
		}
	}

	//C8: Sstr limit
	for (int j = 0; j < nSVL; j++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			Model.add(Sstr[j][tau] <= Exist_SVL[j].store_cap + Xstr[j]);
		}
	}

#pragma endregion


#pragma region Solve the model
	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, 7200);
	cplex.setParam(IloCplex::EpGap, 0.001); // 0.1%
	//cplex.exportModel("MA_LP.lp");

	//cplex.setOut(env.getNullStream());

	//cplex.setParam(IloCplex::Param::MIP::Strategy::Branch, 1); //Up/down branch selected first (1,-1),  default:automatic (0)
	//cplex.setParam(IloCplex::Param::MIP::Strategy::BBInterval, 7);// 0 to 7
	//cplex.setParam(IloCplex::Param::MIP::Strategy::NodeSelect, 2);
	//https://www.ibm.com/support/knowledgecenter/SSSA5P_12.9.0/ilog.odms.cplex.help/CPLEX/UsrMan/topics/discr_optim/mip/performance/20_node_seln.html
	//cplex.setParam(IloCplex::Param::MIP::Strategy::VariableSelect, 4);//-1 to 4
	//cplex.setParam(IloCplex::Param::RootAlgorithm, 4); /000 to 6
	if (!cplex.solve()) {
		env.error() << "Failed to optimize!!!" << endl;
		std::cout << cplex.getStatus();
		throw(-1);
	}

	float obj_val = cplex.getObjValue();
	float gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	auto Elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;

#pragma endregion

	if (PrintVars)
	{
#pragma region print NG network variables
		ofstream fid2;
		fid2.open("DVg.txt");
		fid2 << "Elapsed time: " << Elapsed << endl;
		fid2 << "\t NG Network Obj Value:" << obj_val << endl;
		fid2 << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
		fid2 << "\t NG import Cost: " << cplex.getValue(NG_import_cost) << endl;
		fid2 << "\t Pipeline Establishment Cost: " << cplex.getValue(pipe_cost) << endl;
		fid2 << "\t Storatge Investment Cost: " << cplex.getValue(strInv_cost) << endl;
		fid2 << "\t NG Storage Cost: " << cplex.getValue(gFixVar_cost) << endl;
		fid2 << "\t NG Load Shedding Cost: " << cplex.getValue(gSshedd_cost) << endl;
		//float** supS = new float* [nGnode];
		for (int k = 0; k < nGnode; k++)
		{
			for (int tau = 0; tau < Tg.size(); tau++)
			{
				float s1 = cplex.getValue(supply[k][tau]);
				if (s1 > 0)
				{
					fid2 << "supply[" << k << "][" << tau << "] = " << s1 << endl;
				}
			}
		}
		fid2 << endl;
		for (int k = 0; k < nGnode; k++)
		{
			for (int tau = 0; tau < Tg.size(); tau++)
			{
				float s1 = cplex.getValue(curtG[k][tau]);
				if (s1 > 0)
				{
					fid2 << "CurtG[" << k << "][" << tau << "] = " << s1 << endl;
				}
			}
		}
		fid2 << endl;
		for (int i = 0; i < nPipe; i++)
		{
			float s1 = cplex.getValue(Zg[i]);
			if (s1 > 0.01)
			{
				fid2 << "Zg[" << PipeLines[i].from_node << "][" << PipeLines[i].to_node << "] = " << s1 << endl;
			}
		}
		fid2 << endl;
		for (int i = 0; i < nPipe; i++)
		{
			for (int tau = 0; tau < Tg.size(); tau++)
			{
				float s1 = cplex.getValue(flowGG[i][tau]);
				if (s1 > 0.001)
				{
					fid2 << "flowGG[" << PipeLines[i].from_node << "][" << PipeLines[i].to_node << "][" << tau << "]= " << s1 << endl;
				}
			}
		}
		fid2 << endl;
		for (int k = 0; k < nGnode; k++)
		{
			for (int kp : Gnodes[k].adjE)
			{
				for (int tau = 0; tau < Tg.size(); tau++)
				{
					float s1 = cplex.getValue(flowGE[k][kp][tau]);
					if (std::abs(s1) > 0.1)
					{
						fid2 << "flowGE[" << k << "][" << kp << "][" << tau << "] = " << s1 << endl;
					}
				}
			}
		}

		fid2 << endl;
		for (int k = 0; k < nGnode; k++)
		{
			for (int kp : Gnodes[k].adjS)
			{
				for (int tau = 0; tau < Tg.size(); tau++)
				{
					float s1 = cplex.getValue(flowGL[k][kp][tau]);
					if (std::abs(s1) > 0.1)
					{
						fid2 << "flowGL[" << k << "][" << kp << "][" << tau << "] = " << s1 << endl;
					}
				}
			}
		}
		fid2 << endl;
		for (int k = 0; k < nGnode; k++)
		{
			for (int kp : Gnodes[k].adjS)
			{
				for (int tau = 0; tau < Tg.size(); tau++)
				{
					float s1 = cplex.getValue(flowVG[kp][k][tau]);
					if (std::abs(s1) > 0.1)
					{
						fid2 << "flowVG[" << kp << "][" << k << "][" << tau << "] = " << s1 << endl;
					}
				}
			}
		}

		fid2 << endl;
		for (int j = 0; j < nSVL; j++)
		{
			float s1 = cplex.getValue(Xstr[j]);
			if (s1 > 0.001)
			{
				fid2 << "Xstr[" << j << "]= " << s1 << endl;
			}
		}
		fid2 << endl;
		for (int j = 0; j < nSVL; j++)
		{
			float s1 = cplex.getValue(Xvpr[j]);
			if (s1 > 0.001)
			{
				fid2 << "Xvpr[" << j << "]= " << s1 << endl;
			}
		}


		fid2 << endl;
		for (int j = 0; j < nSVL; j++)
		{
			for (int t = 0; t < Tg.size(); t++)
			{
				float s1 = cplex.getValue(Sstr[j][t]);
				if (s1 > 0.001)
				{
					fid2 << "Sstr[" << j << "][" << t << "]= " << s1 << endl;
				}
			}
		}
		fid2 << endl;
		for (int j = 0; j < nSVL; j++)
		{
			for (int t = 0; t < Tg.size(); t++)
			{
				float s1 = cplex.getValue(Svpr[j][t]);
				if (s1 > 0.001)
				{
					fid2 << "Svpr[" << j << "][" << t << "]= " << s1 << endl;
				}
			}
		}
		fid2 << endl;
		for (int j = 0; j < nSVL; j++)
		{
			for (int t = 0; t < Tg.size(); t++)
			{
				float s1 = cplex.getValue(Sliq[j][t]);
				if (s1 > 0.001)
				{
					fid2 << "Sliq[" << j << "][" << t << "]= " << s1 << endl;
				}
			}
		}
		fid2.close();
#pragma endregion
	}




}