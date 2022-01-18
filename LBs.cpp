//#include"Models.h"
//#include "ilcplex/ilocplex.h";
//typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
//typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
//typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables
//
//
//
//double LB_no_flow_lim(bool PrintVars, int** Xs, int** XestS, int** XdecS)
//{
//	auto start = chrono::high_resolution_clock::now();
//#pragma region Fetch Data
//	std::map<string, int> sym2pltType = { {"ng",0},{"dfo", 1},
//{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7} };
//	std::map<int, string> pltType2sym = { {0,"ng"},{1,"dfo"},
//{2,"solar"},{3,"wind"},{4,"wind_offshore"},{5,"hydro"},{6,"coal"},{7,"nuclear"} };
//	int Rn[] = { 2,3,5 };
//
//	vector<gnode> Gnodes = Params::Gnodes;
//	vector<pipe> PipeLines = Params::PipeLines;
//	vector<enode> Enodes = Params::Enodes;
//	vector<Fips> all_FIPS = Params::all_FIPS;
//	vector<plant> Plants = Params::Plants;
//	vector<branch> Branches = Params::Branches;
//	int nEnode = Enodes.size();
//	int nPlt = Plants.size();
//	int Tg = Params::Tg;
//	int Te = Params::Te;
//	float pi = 3.1415926535897;
//	int nGnode = Gnodes.size();
//	float WACC = Params::WACC;
//	int trans_unit_cost = Params::trans_unit_cost;
//	int trans_line_lifespan = Params::trans_line_lifespan;
//	float NG_price = Params::NG_price;
//	float dfo_pric = Params::dfo_pric;
//	float coal_price = Params::coal_price;
//	float E_curt_cost = Params::E_curt_cost;
//	float G_curt_cost = Params::G_curt_cost;
//	float pipe_per_mile = Params::pipe_per_mile;
//	int pipe_lifespan = Params::pipe_lifespan;
//
//#pragma endregion
//
//	IloEnv env;
//	IloModel Model(env);
//
//#pragma region Decision Variables
//
//	int DV_size = 0;
//#pragma region Electricity network DVs
//
//	NumVar2D Xest(env, nEnode); // integer (continues) for plants
//	NumVar2D Xdec(env, nEnode); // integer (continues) for plants
//	NumVar2D Ze(env, nEnode); // binary for trasmission lines
//	NumVar2D theta(env, nEnode); // continuous phase angle
//	NumVar2D curtE(env, nEnode); // continuous curtailment variable
//
//	NumVar3D prod(env, nEnode);// continuous
//	NumVar2D X(env, nEnode); // integer (continues)
//	NumVar3D flowE(env, nEnode); // unlike the paper, flowE subscripts are "ntm" here
//
//	for (int n = 0; n < nEnode; n++)
//	{
//
//		Xest[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
//		Xdec[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
//		X[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
//		DV_size += 3 * nPlt;
//
//		Ze[n] = IloNumVarArray(env, Enodes[n].adj_buses.size(), 0, IloInfinity, ILOBOOL);
//		theta[n] = IloNumVarArray(env, Te, -pi, pi, ILOFLOAT);
//		curtE[n] = IloNumVarArray(env, Te, 0, IloInfinity, ILOFLOAT);
//
//		prod[n] = NumVar2D(env, Te);
//
//		flowE[n] = NumVar2D(env, Te); // ntm
//
//		DV_size += 4 * Te + Enodes[n].adj_buses.size();
//		for (int t = 0; t < Te; t++)
//		{
//			prod[n][t] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOFLOAT);
//			flowE[n][t] = IloNumVarArray(env, Enodes[n].adj_buses.size(), -IloInfinity, IloInfinity, ILOFLOAT);
//			DV_size += nPlt + Enodes[n].adj_buses.size();
//		}
//	}
//#pragma endregion
//
//#pragma region NG network DVs
//	// NG vars
//	NumVar2D supply(env, nGnode);
//	NumVar2D curtG(env, nGnode);
//	NumVar3D flowGG(env, nGnode);
//	NumVar3D flowGE(env, nGnode);
//	NumVar2D Zg(env, nGnode);
//	for (int k = 0; k < nGnode; k++)
//	{
//		supply[k] = IloNumVarArray(env, Tg, 0, IloInfinity, ILOFLOAT);
//		curtG[k] = IloNumVarArray(env, Tg, 0, IloInfinity, ILOFLOAT);
//		Zg[k] = IloNumVarArray(env, nGnode, 0, IloInfinity, ILOBOOL);
//		DV_size += 2 * Tg + nGnode;
//		flowGG[k] = NumVar2D(env, nGnode);
//		flowGE[k] = NumVar2D(env, nEnode);
//		for (int kp = 0; kp < nGnode; kp++)
//		{
//			flowGG[k][kp] = IloNumVarArray(env, Tg, -IloInfinity, IloInfinity, ILOFLOAT);
//			DV_size += 2 * Tg;
//		}
//		for (int n = 0; n < nEnode; n++)
//		{
//			flowGE[k][n] = IloNumVarArray(env, Tg, 0, IloInfinity, ILOFLOAT);
//		}
//	}
//#pragma endregion
//
//
//
//#pragma endregion
//
//#pragma region Objective Function
//	IloExpr exp0(env);
//
//#pragma region Electricity network related costs
//	for (int n = 0; n < nEnode; n++)
//	{
//		for (int i = 0; i < nPlt; i++)
//		{
//			// Investment/decommission cost of plants
//			float s1 = std::pow(1.0 / (1 + WACC), Plants[i].lifespan);
//			float capco = WACC / (1 - s1);
//			exp0 += capco * Plants[i].capex * Xest[n][i] + Plants[i].decom_cost * Xdec[n][i];
//		}
//		// fix+var+fuel costs of plants
//		for (int t = 0; t < Te; t++)
//		{
//			for (int i = 0; i < nPlt; i++)
//			{
//				// fixed cost is initially given annually, but cpp reads is hourly. 
//				exp0 += Te * Plants[i].fix_cost * X[n][i] + Plants[i].var_cost * prod[n][t][i];
//
//				// fuel price to be updated later (dollar per thousand cubic feet=MMBTu)
//				if (Plants[i].fuel_type == "ng")
//				{
//					exp0 += NG_price * Plants[i].heat_rate * prod[n][t][i];
//				}
//				else if (Plants[i].fuel_type == "dfo")
//				{
//					exp0 += dfo_pric * Plants[i].heat_rate * prod[n][t][i];
//				}
//				else if (Plants[i].fuel_type == "coal")
//				{
//					exp0 += coal_price * Plants[i].heat_rate * prod[n][t][i];
//				}
//
//				// emission cost (to be added later)
//				exp0 += Plants[i].emis_cost * Plants[i].emis_rate * prod[n][t][i];
//			}
//
//			// load curtailment cost
//			exp0 += E_curt_cost * curtE[n][t];
//		}
//	}
//
//	// investment cost of transmission lines
//	for (int b = 0; b < Branches.size(); b++)
//	{
//		float s1 = std::pow(1.0 / (1 + WACC), trans_line_lifespan);
//		float capco = WACC / (1 - s1);
//		int fb = Branches[b].from_bus;
//		int tb = Branches[b].to_bus;
//		// find the index of tb in the adj_buses of the fb
//		int tbi = std::find(Enodes[fb].adj_buses.begin(), Enodes[fb].adj_buses.end(), tb) - Enodes[fb].adj_buses.begin();
//		exp0 += capco * trans_unit_cost * Branches[b].length * Ze[fb][tbi];
//	}
//
//#pragma endregion
//
//#pragma region NG network related costs
//	for (int i = 0; i < PipeLines.size(); i++)
//	{
//		float s1 = std::pow(1.0 / (1 + WACC), pipe_lifespan);
//		float capco = WACC / (1 - s1);
//		int fn = PipeLines[i].from_node;
//		int tn = PipeLines[i].to_node;
//		float cost1 = PipeLines[i].length * pipe_per_mile;
//
//		exp0 += capco * cost1 * Zg[fn][tn];
//	}
//	for (int k = 0; k < nGnode; k++)
//	{
//		for (int tau = 0; tau < Tg; tau++)
//		{
//			exp0 += G_curt_cost * curtG[k][tau];
//		}
//	}
//#pragma endregion
//
//	Model.add(IloMinimize(env, exp0));
//
//	exp0.end();
//
//#pragma endregion
//
//
//#pragma region Constraints
//	int const_size = 0;
//
//#pragma region Electricity Network Constraints
//
//	// C1: generation capacity in each node
//	int existP = 0;
//	for (int n = 0; n < nEnode; n++)
//	{
//		int j = 0;
//		bool if_off_shore = std::find(Enodes[n].Init_plt_types.begin(), Enodes[n].Init_plt_types.end(), "wind_offshore") != Enodes[n].Init_plt_types.end();
//
//		for (int i = 0; i < nPlt; i++)
//		{
//			// wind_offshore plants are only allowed in buses that already have one
//
//			if (!if_off_shore)
//			{
//				Model.add(Xest[n][4] == 0);
//			}
//
//			bool isInd = std::find(Enodes[n].Init_plt_ind.begin(), Enodes[n].Init_plt_ind.end(), i) != Enodes[n].Init_plt_ind.end();
//			if (isInd)
//			{
//				// find the index of plant in the set of Init_plants of the node
//				string tp = pltType2sym[i];
//				int ind1 = std::find(Enodes[n].Init_plt_types.begin(), Enodes[n].Init_plt_types.end(), tp) - Enodes[n].Init_plt_types.begin();
//				Model.add(X[n][i] == Enodes[n].Init_plt_count[ind1] - Xdec[n][i] + Xest[n][i]);
//				const_size++;
//				j++;
//			}
//			else
//			{
//				Model.add(X[n][i] == -Xdec[n][i] + Xest[n][i]);
//				const_size++;
//			}
//		}
//	}
//	// C2:
//	for (int n = 0; n < nEnode; n++)
//	{
//		for (int i = 0; i < nPlt; i++)
//		{
//			Model.add(X[n][i] <= Plants[i].Umax);
//			const_size++;
//		}
//	}
//
//	//C3, C4: production limit, ramping
//	for (int n = 0; n < nEnode; n++)
//	{
//		for (int t = 0; t < Te; t++)
//		{
//			for (int i = 0; i < nPlt; i++)
//			{
//				Model.add(prod[n][t][i] >= Plants[i].Pmin * X[n][i]);
//				Model.add(prod[n][t][i] <= Plants[i].Pmax * X[n][i]);
//				const_size += 2;
//
//				if (t > 0)
//				{
//					Model.add(-Plants[i].rampD * X[n][i] <= prod[n][t][i] - prod[n][t - 1][i]);
//					Model.add(prod[n][t][i] - prod[n][t - 1][i] <= Plants[i].rampU * X[n][i]);
//					const_size += 2;
//
//				}
//
//			}
//		}
//	}
//
//	// C5: flow limit for electricity
//	/*for (int br = 0; br < Branches.size(); br++)
//	{
//		int fb = Branches[br].from_bus;
//		int tb = Branches[br].to_bus;
//		int tbi = std::find(Enodes[fb].adj_buses.begin(), Enodes[fb].adj_buses.end(), tb) - Enodes[fb].adj_buses.begin();
//		Model.add(Ze[fb][tbi]); const_size++;
//		int fbi = std::find(Enodes[tb].adj_buses.begin(), Enodes[tb].adj_buses.end(), fb) - Enodes[tb].adj_buses.begin();
//		Model.add(Ze[tb][fbi]); const_size++;
//		for (int t = 0; t < Te; t++)
//		{
//			Model.add(IloAbs(flowE[fb][t][tbi] + flowE[tb][t][fbi]) <= 10e-3);
//			if (Branches[br].is_exist == 1)
//			{
//				Model.add(IloAbs(flowE[fb][t][tbi]) <= Branches[br].maxFlow * (1 + Ze[fb][tbi]));
//				const_size++;
//			}
//			else
//			{
//				Model.add(IloAbs(flowE[fb][t][tbi]) <= Branches[br].maxFlow * Ze[fb][tbi]);
//				const_size++;
//			}
//		}
//	}*/
//
//	//C6: flow balance
//	for (int n = 0; n < nEnode; n++)
//	{
//		for (int t = 0; t < Te; t++)
//		{
//			IloExpr exp1(env);
//			for (int i = 0; i < nPlt; i++)
//			{
//				exp1 += prod[n][t][i];
//			}
//			IloExpr exp2(env);
//			for (int m = 0; m < Enodes[n].adj_buses.size(); m++)
//			{
//				Model.add(flowE[n][t][m]); const_size++;
//				exp2 += flowE[n][t][m];
//			}
//			float dem = 0;
//			if (all_FIPS[Enodes[n].fips_order].bus_count > 0)
//			{
//				dem = all_FIPS[Enodes[n].fips_order].demE[t] / all_FIPS[Enodes[n].fips_order].bus_count;
//			}
//			//Model.add(exp1 + curtE[n][t] == dem); // ignore transmission
//			Model.add(exp1 - exp2 + curtE[n][t] == dem); const_size++;
//		}
//	}
//	// C8: flow equation
//	int ebr = 0;
//	for (int br = 0; br < Branches.size(); br++)
//	{
//		int fb = Branches[br].from_bus;
//		int tb = Branches[br].to_bus;
//		int tbi = std::find(Enodes[fb].adj_buses.begin(), Enodes[fb].adj_buses.end(), tb) - Enodes[fb].adj_buses.begin();
//		int fbi = std::find(Enodes[tb].adj_buses.begin(), Enodes[tb].adj_buses.end(), fb) - Enodes[tb].adj_buses.begin();
//
//		for (int t = 0; t < Te; t++)
//		{
//			//Model.add(flowE[fb][t][tbi] == Branches[br].suscep * (theta[tb][t] - theta[fb][t]));
//			//Model.add(flowE[tb][t][fbi] == Branches[br].suscep * (theta[fb][t] - theta[tb][t]));
//			Model.add(IloAbs(flowE[fb][t][tbi] - Branches[br].suscep * (theta[tb][t] - theta[fb][t])) <= 1e-2);
//			Model.add(IloAbs(flowE[tb][t][fbi] - Branches[br].suscep * (theta[fb][t] - theta[tb][t])) <= 1e-2);
//			const_size += 2;
//		}
//	}
//
//	// C9: phase angle (theta) limits. already applied in the definition of the variable
//	for (int n = 0; n < nEnode; n++)
//	{
//		Model.add(theta[n][0] == 0); const_size++;
//		for (int t = 1; t < Te; t++)
//		{
//			Model.add(IloAbs(theta[n][t]) <= pi); const_size++;
//		}
//	}
//
//	// C10: VRE production profile
//	for (int n = 0; n < nEnode; n++)
//	{
//		for (int t = 0; t < Te; t++)
//		{
//			for (int r : Rn)
//			{
//				Model.add(prod[n][t][r] <= Plants[r].prod_profile[t] * X[n][r]);
//				const_size++;
//			}
//		}
//	}
//
//
//	// C11: demand curtainlment constraint
//	for (int n = 0; n < nEnode; n++)
//	{
//		for (int t = 0; t < Te; t++)
//		{
//			float dem = 0;
//			if (all_FIPS[Enodes[n].fips_order].bus_count > 0)
//			{
//				dem = all_FIPS[Enodes[n].fips_order].demE[t] / all_FIPS[Enodes[n].fips_order].bus_count;
//			}
//			Model.add(curtE[n][t] <= dem); const_size++;
//		}
//	}
//#pragma endregion
//
//#pragma region NG Network Constraint
//	// C1: flow limit for NG
//	float ave_cap = 448;
//	for (int i = 0; i < PipeLines.size(); i++)
//	{
//		int fn = PipeLines[i].from_node;
//		int tn = PipeLines[i].to_node;
//		int is_exist = PipeLines[i].is_exist;
//		for (int tau = 0; tau < Tg; tau++)
//		{
//			if (is_exist == 1)
//			{
//				Model.add(flowGG[fn][tn][tau] <= PipeLines[i].cap + ave_cap * Zg[fn][tn]);
//			}
//			else
//			{
//				Model.add(flowGG[fn][tn][tau] <= PipeLines[i].cap * Zg[fn][tn]);
//			}
//		}
//	}
//
//	//C2: flow balance, NG node
//	for (int k = 0; k < nGnode; k++)
//	{
//		for (int tau = 0; tau < Tg; tau++)
//		{
//			IloExpr exp2(env);
//			for (int kp : Gnodes[k].adjG) {
//				exp2 += flowGG[k][kp][tau];
//				//Model.add(flowGG[k][kp][tau] >= 0);
//				Model.add(IloAbs(flowGG[k][kp][tau] + flowGG[kp][k][tau]) <= 1e-2);
//			}
//
//			IloExpr exp3(env);
//			for (int n : Gnodes[k].adjE)
//			{
//				exp3 += flowGE[k][n][tau];
//				Model.add(flowGE[k][n][tau]);
//			}
//
//			Model.add(supply[k][tau] - exp2 - exp3 + curtG[k][tau] == Gnodes[k].demG[tau]);
//			//Model.add(supply[k][tau] - exp2 + curtG[k][tau] == Gnodes[k].demG[tau]);
//
//		}
//	}
//
//
//	// C3,C4: injection (supply) limit and curtailment limit
//	for (int k = 0; k < nGnode; k++)
//	{
//		for (int tau = 0; tau < Tg; tau++)
//		{
//			Model.add(supply[k][tau] >= Gnodes[k].injL);
//			Model.add(supply[k][tau] <= Gnodes[k].injU);
//
//			Model.add(curtG[k][tau] <= Gnodes[k].demG[tau]);
//		}
//	}
//
//
//#pragma endregion
//
//
//#pragma region Coupling Constraint
//	for (int k = 0; k < nGnode; k++)
//	{
//		for (int tau = 0; tau < Tg; tau++)
//		{
//			for (int n : Gnodes[k].adjE)
//			{
//				IloExpr exp2(env);
//				for (int t = tau * 24; t < (tau + 1) * 24; t++)
//				{
//					exp2 += Plants[0].heat_rate * prod[n][t][0];
//				}
//				Model.add(IloAbs(flowGE[k][n][tau] - exp2) <= 1e-2);
//			}
//		}
//	}
//#pragma endregion
//
//
//#pragma endregion
//
//#pragma region Solve the model
//	std::cout << DV_size << endl;
//	std::cout << const_size << endl;
//	IloCplex cplex(Model);
//	cplex.setParam(IloCplex::TiLim, 7200);
//	cplex.setParam(IloCplex::EpGap, 0.001); // 0.1%
//	//cplex.exportModel("MA_LP.lp");
//
//	//cplex.setOut(env.getNullStream());
//
//	//cplex.setParam(IloCplex::Param::MIP::Strategy::Branch, 1); //Up/down branch selected first (1,-1),  default:automatic (0)
//	//cplex.setParam(IloCplex::Param::MIP::Strategy::BBInterval, 7);// 0 to 7
//	//cplex.setParam(IloCplex::Param::MIP::Strategy::NodeSelect, 2);
//	//https://www.ibm.com/support/knowledgecenter/SSSA5P_12.9.0/ilog.odms.cplex.help/CPLEX/UsrMan/topics/discr_optim/mip/performance/20_node_seln.html
//	//cplex.setParam(IloCplex::Param::MIP::Strategy::VariableSelect, 4);//-1 to 4
//	//cplex.setParam(IloCplex::Param::RootAlgorithm, 4); /000 to 6
//	if (!cplex.solve()) {
//		env.error() << "Failed to optimize!!!" << endl;
//		std::cout << cplex.getStatus();
//		throw(-1);
//	}
//
//	float obj_val = cplex.getObjValue();
//	float gap = cplex.getMIPRelativeGap();
//	auto end = chrono::high_resolution_clock::now();
//	auto Elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
//	std::cout << "Elapsed time: " << Elapsed << endl;
//	std::cout << "\t LB Value:" << obj_val << endl;
//	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
//
//#pragma endregion
//
//	for (int n = 0; n < nEnode; n++)
//	{
//		Xs[n] = new int[nPlt]();
//		XestS[n] = new int[nPlt]();
//		XdecS[n] = new int[nPlt]();
//		for (int i = 0; i < nPlt; i++)
//		{
//			Xs[n][i] =std::round(cplex.getValue(X[n][i]));
//			Xs[n][i] = std::max(0, Xs[n][i]);
//			XestS[n][i] = std::round(cplex.getValue(Xest[n][i]));
//			Xs[n][i] = std::max(0, XestS[n][i]);
//			XdecS[n][i] = std::round(cplex.getValue(Xdec[n][i]));
//			Xs[n][i] = std::max(0, XdecS[n][i]);
//		}
//	}
//
//
//	if (PrintVars)
//	{
//#pragma region print Electricity Network variables
//		ofstream fid;
//		fid.open("DVe.txt");
//		fid << "Elapsed time: " << Elapsed << endl;
//		fid << "\t RMP Obj Value:" << obj_val << endl;
//		fid << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
//		fid << "Number of DVs: " << DV_size << endl;
//		fid << "Number of constraints: " << const_size << endl;
//		float** XestS = new float* [nEnode];
//		float** XdecS = new float* [nEnode];
//		float** Xs = new float* [nEnode];
//		for (int n = 0; n < nEnode; n++)
//		{
//			XestS[n] = new float[nPlt]();
//			XdecS[n] = new float[nPlt]();
//			Xs[n] = new float[nPlt]();
//			for (int i = 0; i < nPlt; i++)
//			{
//				XestS[n][i] = cplex.getValue(Xest[n][i]);
//				XdecS[n][i] = cplex.getValue(Xdec[n][i]);
//				Xs[n][i] = cplex.getValue(X[n][i]);
//				if (XestS[n][i] > 0)
//				{
//					//std::cout << "Xest[" << n << "][" << i << "] = " << XestS[n][i] << endl;
//					fid << "Xest[" << n << "][" << i << "] = " << XestS[n][i] << endl;
//				}
//				if (XdecS[n][i] > 0)
//				{
//					//std::cout << "Xdec[" << n << "][" << i << "] = " << XdecS[n][i] << endl;
//					fid << "Xdec[" << n << "][" << i << "] = " << XdecS[n][i] << endl;
//				}
//				if (Xs[n][i] > 0)
//				{
//					//cout << "X[" << n << "][" << i << "] = " << Xs[n][i] << endl;
//					fid << "X[" << n << "][" << i << "] = " << Xs[n][i] << endl;
//				}
//			}
//		}
//
//		float*** fs = new float** [nEnode];
//
//		for (int n = 0; n < nEnode; n++)
//		{
//			fs[n] = new float* [Te];
//			if (n == 9)
//			{
//				int gg = 0;
//			}
//			for (int t = 0; t < Te; t++)
//			{
//				fs[n][t] = new float[Enodes[n].adj_buses.size()]();
//				for (int m = 0; m < Enodes[n].adj_buses.size(); m++)
//				{
//					fs[n][t][m] = cplex.getValue(flowE[n][t][m]);
//					if (std::abs(fs[n][t][m]) > 10e-3)
//					{
//						//std::cout << "flowE[" << n << "][" << t << "][" << Enodes[n].adj_buses[m] << "] = " << fs[n][t][m] << endl;
//						fid << "flowE[" << n << "][" << t << "][" << Enodes[n].adj_buses[m] << "] = " << fs[n][t][m] << endl;
//					}
//				}
//			}
//		}
//		//float** Zs = new float* [nEnode];
//		//for (int n = 0; n < nEnode; n++)
//		//{
//		//	Zs[n] = new float[Enodes[n].adj_buses.size()]();
//		//	for (int m = 0; m < Enodes[n].adj_buses.size(); m++)
//		//	{
//		//		Zs[n][m] = cplex.getValue(Z[n][m]);
//		//		if (Zs[n][m] > 10e-3)
//		//		{
//		//			//std::cout << "Z[" << n << "][" << Enodes[n].adj_buses[m] << "] = " << Zs[n][m] << endl;
//		//			fid << "Z[" << n << "][" << Enodes[n].adj_buses[m] << "] = " << Zs[n][m] << endl;
//		//		}
//		//	}
//		//}
//
//		float*** prodS = new float** [nEnode];
//		for (int n = 0; n < nEnode; n++)
//		{
//			prodS[n] = new float* [Te];
//			for (int t = 0; t < Te; t++)
//			{
//				if (t > 10) { break; }
//
//				prodS[n][t] = new float[nPlt]();
//				for (int i = 0; i < nPlt; i++)
//				{
//					prodS[n][t][i] = cplex.getValue(prod[n][t][i]);
//					if (prodS[n][t][i] > 10e-3)
//					{
//						//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
//						fid << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
//					}
//				}
//			}
//		}
//
//		float** curtS = new float* [nEnode];
//		for (int n = 0; n < nEnode; n++)
//		{
//			curtS[n] = new float[Te]();
//			for (int t = 0; t < Te; t++)
//			{
//				if (t > 10) { break; }
//				curtS[n][t] = cplex.getValue(curtE[n][t]);
//				if (curtS[n][t] > 10e-3)
//				{
//					//	std::cout << "curtE[" << n << "][" << t << "] = " << curtS[n][t] << endl;
//					fid << "curtE[" << n << "][" << t << "] = " << curtS[n][t] << endl;
//				}
//			}
//		}
//
//
//
//		fid.close();
//#pragma endregion
//
//#pragma region print NG network variables
//		ofstream fid2;
//		fid2.open("DVg.txt");
//
//		//float** supS = new float* [nGnode];
//		for (int k = 0; k < nGnode; k++)
//		{
//			for (int tau = 0; tau < Tg; tau++)
//			{
//				float s1 = cplex.getValue(supply[k][tau]);
//				if (s1 > 0)
//				{
//					fid2 << "supply[" << k << "][" << tau << "] = " << s1 << endl;
//				}
//			}
//		}
//
//		for (int k = 0; k < nGnode; k++)
//		{
//			for (int tau = 0; tau < Tg; tau++)
//			{
//				float s1 = cplex.getValue(curtG[k][tau]);
//				if (s1 > 0)
//				{
//					fid2 << "CurtG[" << k << "][" << tau << "] = " << s1 << endl;
//				}
//			}
//		}
//		for (int i = 0; i < PipeLines.size(); i++)
//		{
//			int fn = PipeLines[i].from_node;
//			int tn = PipeLines[i].to_node;
//			float s1 = cplex.getValue(Zg[fn][tn]);
//			if (s1 > 0.01)
//			{
//				fid2 << "Zg[" << fn << "][" << tn << "] = " << s1 << endl;
//			}
//		}
//
//
//
//		for (int k = 0; k < nGnode; k++)
//		{
//			for (int kp : Gnodes[k].adjG)
//			{
//				for (int tau = 0; tau < Tg; tau++)
//				{
//					float s1 = cplex.getValue(flowGG[k][kp][tau]);
//					if (std::abs(s1) > 0.1)
//					{
//						fid2 << "flowGG[" << k << "][" << kp << "][" << tau << "] = " << s1 << endl;
//					}
//				}
//			}
//		}
//
//		for (int k = 0; k < nGnode; k++)
//		{
//			for (int kp : Gnodes[k].adjE)
//			{
//				for (int tau = 0; tau < Tg; tau++)
//				{
//					float s1 = cplex.getValue(flowGE[k][kp][tau]);
//					if (std::abs(s1) > 0.1)
//					{
//						fid2 << "flowGE[" << k << "][" << kp << "][" << tau << "] = " << s1 << endl;
//					}
//				}
//			}
//		}
//
//		fid2.close();
//#pragma endregion
//	}
//
//	return obj_val;
//}
//
