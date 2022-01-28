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



#pragma region NG network related costs
	IloExpr exp0(env);
	IloExpr exp_pipe(env);
	IloExpr exp_curt(env);
	IloExpr exp_store(env);

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
	exp0 = exp_pipe + exp_curt + exp_store;
	IloNumVar pipe_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar gSshedd_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar gStore_cost(env, 0, IloInfinity, ILOFLOAT);
	Model.add(pipe_cost == exp_pipe);
	Model.add(gSshedd_cost == exp_curt);
	Model.add(gStore_cost == exp_store);
#pragma endregion

	Model.add(IloMinimize(env, exp0));

	exp0.end();


#pragma region NG Network Constraint
	// C1, C2: flow limit for NG
	float ave_cap = 448;
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
			IloExpr exp_gg_flow(env);
			IloExpr exp_ge_flow(env);
			IloExpr exp_store(env);
			for (int kp : Gnodes[k].adjG)
			{
				int key1 = k * 200 + kp;
				// check if this key exist, if not, the other order exisit
				if (Le.count(key1) == 0) { key1 = k * 200 + kp; }
				for (int l2 : Lg[key1]) { exp_gg_flow += flowGG[l2][tau]; }
			}
			for (int n : Gnodes[k].adjE) { exp_ge_flow += flowGE[k][n][tau]; }
			Model.add(supply[k][tau] + exp_gg_flow - exp_ge_flow + curtG[k][tau] == Gnodes[k].demG[Tg[tau]]);
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
		fid2 << "\t Pipeline Establishment Cost: " << cplex.getValue(pipe_cost) << endl;
		fid2 << "\t NG Storage Cost: " << cplex.getValue(gStore_cost) << endl;
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
		for (int i = 0; i < nPipe; i++)
		{
			float s1 = cplex.getValue(Zg[i]);
			if (s1 > 0.01)
			{
				fid2 << "Zg[" << PipeLines[i].from_node << "][" << PipeLines[i].to_node << "] = " << s1 << endl;
			}
		}

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

		fid2.close();
#pragma endregion
	}




}