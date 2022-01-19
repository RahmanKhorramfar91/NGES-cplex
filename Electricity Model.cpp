#include"Models.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables


void Electricy_Network_Model(bool int_vars_relaxed, bool PrintVars)
{
	auto start = chrono::high_resolution_clock::now();
#pragma region Fetch Data
	std::map<string, int> sym2pltType = { {"ng",0},{"dfo", 1},
{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7} };
	std::map<int, string> pltType2sym = { {0,"ng"},{1,"dfo"},
{2,"solar"},{3,"wind"},{4,"wind_offshore"},{5,"hydro"},{6,"coal"},{7,"nuclear"} };
	int Rn[] = { 2,3,5 };

	vector<gnode> Gnodes = Params::Gnodes;
	vector<pipe> PipeLines = Params::PipeLines;
	vector<enode> Enodes = Params::Enodes;
	vector<Fips> all_FIPS = Params::all_FIPS;
	vector<plant> Plants = Params::Plants;
	vector<branch> Branches = Params::Branches;
	int nEnode = Enodes.size();
	int nPlt = Plants.size();
	int nBr = Branches.size();
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
#pragma region Electricity network DVs

	NumVar2D Xest(env, nEnode); // integer (continues) for plants
	NumVar2D Xdec(env, nEnode); // integer (continues) for plants
	NumVar2D Ze(env, nEnode); // binary for trasmission lines
	NumVar2D theta(env, nEnode); // continuous phase angle
	NumVar2D curtE(env, nEnode); // continuous curtailment variable

	NumVar3D prod(env, nEnode);// continuous
	NumVar2D X(env, nEnode); // integer (continues)
	NumVar2D flowE(env, nBr); // unlike the paper, flowE subscripts are "ntm" here
	for (int b = 0; b < nBr; b++)
	{
		flowE[b] = IloNumVarArray(env, Te.size(), -IloInfinity, IloInfinity, ILOFLOAT);
	}
	for (int n = 0; n < nEnode; n++)
	{
		if (int_vars_relaxed)
		{
			Xest[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOFLOAT);
			Xdec[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOFLOAT);
			X[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOFLOAT);
			DV_size += 3 * nPlt;
		}
		else
		{
			Xest[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
			Xdec[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
			X[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
			DV_size += 3 * nPlt;
		}


		Ze[n] = IloNumVarArray(env, Enodes[n].adj_buses.size(), 0, IloInfinity, ILOBOOL);
		theta[n] = IloNumVarArray(env, Te.size(), -pi, pi, ILOFLOAT);
		curtE[n] = IloNumVarArray(env, Te.size(), 0, IloInfinity, ILOFLOAT);

		prod[n] = NumVar2D(env, Te.size());

		//flowE[n] = NumVar2D(env, Te); // ntm

		DV_size += 4 * Te.size() + Enodes[n].adj_buses.size();
		for (int t = 0; t < Te.size(); t++)
		{
			prod[n][t] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOFLOAT);
			//	flowE[n][t] = IloNumVarArray(env, Enodes[n].adj_buses.size(), -IloInfinity, IloInfinity, ILOFLOAT);
			DV_size += nPlt + Enodes[n].adj_buses.size();
		}
	}
#pragma endregion

#pragma region Objective Function
	IloExpr exp0(env);

	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			// Investment/decommission cost of plants
			float s1 = std::pow(1.0 / (1 + WACC), Plants[i].lifespan);
			float capco = WACC / (1 - s1);
			exp0 += capco * Plants[i].capex * Xest[n][i] + Plants[i].decom_cost * Xdec[n][i];
		}

		// fixed cost
		for (int i = 0; i < nPlt; i++)
		{
			exp0 += Plants[i].fix_cost * X[n][i];
		}
		// var+fuel costs of plants
		for (int t = 0; t < Te.size(); t++)
		{
			for (int i = 0; i < nPlt; i++)
			{
				// var cost
				exp0 += time_weight[t] * Plants[i].var_cost * prod[n][t][i];

				// fuel price to be updated later (dollar per thousand cubic feet=MMBTu)
				if (Plants[i].fuel_type == "ng")
				{
					exp0 += time_weight[t] * NG_price * Plants[i].heat_rate * prod[n][t][i];
				}
				else if (Plants[i].fuel_type == "dfo")
				{
					exp0 += time_weight[t] * dfo_pric * Plants[i].heat_rate * prod[n][t][i];
				}
				else if (Plants[i].fuel_type == "coal")
				{
					exp0 += time_weight[t] * coal_price * Plants[i].heat_rate * prod[n][t][i];
				}

				// emission cost (to be added later)
				exp0 += time_weight[t] * Plants[i].emis_cost * Plants[i].emis_rate * prod[n][t][i];
			}

			// load curtailment cost
			exp0 += time_weight[t] * E_curt_cost * curtE[n][t];
		}
	}

	// investment cost of transmission lines
	for (int b = 0; b < Branches.size(); b++)
	{
		float s1 = std::pow(1.0 / (1 + WACC), trans_line_lifespan);
		float capco = WACC / (1 - s1);
		int fb = Branches[b].from_bus;
		int tb = Branches[b].to_bus;
		// find the index of tb in the adj_buses of the fb
		int tbi = std::find(Enodes[fb].adj_buses.begin(), Enodes[fb].adj_buses.end(), tb) - Enodes[fb].adj_buses.begin();
		exp0 += capco * trans_unit_cost * Branches[b].length * Ze[fb][tbi];
	}

	Model.add(IloMinimize(env, exp0));

	exp0.end();
#pragma endregion


#pragma region Constraints
	int const_size = 0;

	// C1: generation capacity in each node
	int existP = 0;
	for (int n = 0; n < nEnode; n++)
	{
		int j = 0;
		bool if_off_shore = std::find(Enodes[n].Init_plt_types.begin(), Enodes[n].Init_plt_types.end(), "wind_offshore") != Enodes[n].Init_plt_types.end();

		for (int i = 0; i < nPlt; i++)
		{
			// wind_offshore plants are only allowed in buses that already have one

			if (!if_off_shore)
			{
				Model.add(Xest[n][4] == 0);
			}

			bool isInd = std::find(Enodes[n].Init_plt_ind.begin(), Enodes[n].Init_plt_ind.end(), i) != Enodes[n].Init_plt_ind.end();
			if (isInd)
			{
				// find the index of plant in the set of Init_plants of the node
				string tp = pltType2sym[i];
				int ind1 = std::find(Enodes[n].Init_plt_types.begin(), Enodes[n].Init_plt_types.end(), tp) - Enodes[n].Init_plt_types.begin();
				Model.add(X[n][i] == Enodes[n].Init_plt_count[ind1] - Xdec[n][i] + Xest[n][i]);
				const_size++;
				j++;
			}
			else
			{
				Model.add(X[n][i] == -Xdec[n][i] + Xest[n][i]);
				const_size++;
			}
		}
	}

	// C2:
	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			Model.add(X[n][i] <= Plants[i].Umax);
			const_size++;
		}
	}

	//C3, C4: production limit, ramping
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			for (int i = 0; i < nPlt; i++)
			{
				Model.add(prod[n][t][i] >= Plants[i].Pmin * X[n][i]);
				Model.add(prod[n][t][i] <= Plants[i].Pmax * X[n][i]);
				const_size += 2;

				if (t > 0)
				{
					Model.add(-Plants[i].rampD * X[n][i] <= prod[n][t][i] - prod[n][t - 1][i]);
					Model.add(prod[n][t][i] - prod[n][t - 1][i] <= Plants[i].rampU * X[n][i]);
					const_size += 2;

				}

			}
		}
	}

	// C5, C6: flow limit for electricity
	for (int br = 0; br < nBr; br++)
	{
		int fb = Branches[br].from_bus;
		int tb = Branches[br].to_bus;
		int tbi = std::find(Enodes[fb].adj_buses.begin(), Enodes[fb].adj_buses.end(), tb) - Enodes[fb].adj_buses.begin();
		Model.add(Ze[fb][tbi]); const_size++;
		int fbi = std::find(Enodes[tb].adj_buses.begin(), Enodes[tb].adj_buses.end(), fb) - Enodes[tb].adj_buses.begin();
		Model.add(Ze[tb][fbi]); const_size++;
		for (int t = 0; t < Te.size(); t++)
		{
			//Model.add(IloAbs(flowE[fb][t][tbi] + flowE[tb][t][fbi]) <= 10e-3);
			if (Branches[br].is_exist == 1)
			{
				//Model.add(IloAbs(flowE[br][t]) <= Branches[br].maxFlow);
				Model.add(flowE[br][t] <= Branches[br].maxFlow);
				Model.add(-Branches[br].maxFlow <= flowE[br][t]);
				const_size++;
			}
			else
			{
				//Model.add(IloAbs(flowE[br][t]) <= Branches[br].maxFlow * Ze[fb][tbi]);
				Model.add(flowE[br][t] <= Branches[br].maxFlow * Ze[fb][tbi]);
				Model.add(-Branches[br].maxFlow * Ze[fb][tbi] <= flowE[br][t]);
				const_size++;
			}
		}
	}

	//C7: flow balance
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			IloExpr exp1(env);
			for (int i = 0; i < nPlt; i++)
			{
				exp1 += prod[n][t][i];
			}
			IloExpr exp2(env);
			for (int m = 0; m < Enodes[n].adj_buses.size(); m++)
			{
				int key1 = n * 200 + Enodes[n].adj_buses[m];
				// check if this key exist, if not, the other order exisit
				if (Lnm.count(key1) == 0)
				{
					key1 = Enodes[n].adj_buses[m] * 200 + n;
				}
				int l1 = Lnm[key1][0]; //each Lnm contains only one value
				Model.add(flowE[l1][t]); const_size++;
				exp2 += flowE[l1][t];
			}
			float dem = 0;
			if (all_FIPS[Enodes[n].fips_order].bus_count > 0)
			{
				dem = all_FIPS[Enodes[n].fips_order].demE[Te[t]] / all_FIPS[Enodes[n].fips_order].bus_count;
			}
			//Model.add(exp1 + curtE[n][t] == dem); // ignore transmission
			Model.add(exp1 - exp2 + curtE[n][t] == dem); const_size++;
		}
	}
	// C8: flow equation
	int ebr = 0;
	for (int br = 0; br < Branches.size(); br++)
	{
		int fb = Branches[br].from_bus;
		int tb = Branches[br].to_bus;
		int tbi = std::find(Enodes[fb].adj_buses.begin(), Enodes[fb].adj_buses.end(), tb) - Enodes[fb].adj_buses.begin();
		int fbi = std::find(Enodes[tb].adj_buses.begin(), Enodes[tb].adj_buses.end(), fb) - Enodes[tb].adj_buses.begin();

		for (int t = 0; t < Te.size(); t++)
		{
			if (Branches[br].is_exist == 1)
			{
				//Model.add(IloAbs(flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t])) <= 1e-2);
				Model.add(flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t]) <= 1e-2);
				Model.add(-1e-2 <= flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t]));
			}
			else
			{
				//Model.add(IloAbs(flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t])) <= 1e-2 + Branches[br].suscep * 24 * (1 - Ze[fb][tbi]));
				Model.add(flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t]) <= 1e-2 + Branches[br].suscep * 24 * (1 - Ze[fb][tbi]));
				Model.add(-1e-2 - Branches[br].suscep * 24 * (1 - Ze[fb][tbi]) <= flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t]));
			}
			//Model.add(flowE[fb][t][tbi] == Branches[br].suscep * (theta[tb][t] - theta[fb][t]));
			//Model.add(flowE[tb][t][fbi] == Branches[br].suscep * (theta[fb][t] - theta[tb][t]));
			//Model.add(IloAbs(flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t])) <= 1e-2);
			//Model.add(IloAbs(flowE[tb][t][fbi] - Branches[br].suscep * (theta[fb][t] - theta[tb][t])) <= 1e-2);
			const_size += 2;
		}
	}

	// C9: phase angle (theta) limits. already applied in the definition of the variable
	for (int n = 0; n < nEnode; n++)
	{
		Model.add(theta[n][0] == 0); const_size++;
		for (int t = 1; t < Te.size(); t++)
		{
			//Model.add(IloAbs(theta[n][t]) <= pi);
			Model.add(theta[n][t] <= pi);
			Model.add(-pi <= theta[n][t]);
			const_size++;
		}
	}

	// C10: VRE production profile
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			for (int r : Rn)
			{
				Model.add(prod[n][t][r] <= Plants[r].prod_profile[Te[t]] * X[n][r]);
				const_size++;
			}
		}
	}


	// C11: demand curtainlment constraint
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			float dem = 0;
			if (all_FIPS[Enodes[n].fips_order].bus_count > 0)
			{
				dem = all_FIPS[Enodes[n].fips_order].demE[Te[t]] / all_FIPS[Enodes[n].fips_order].bus_count;
			}
			Model.add(curtE[n][t] <= dem); const_size++;
		}
	}

	//C12, C13: Emission limit and RPS constraints
	IloExpr exp2(env); IloExpr exp3(env);
	float total_demand = 0;
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			if (all_FIPS[Enodes[n].fips_order].bus_count > 0)
			{
				total_demand += all_FIPS[Enodes[n].fips_order].demE[Te[t]] / all_FIPS[Enodes[n].fips_order].bus_count;
			}
			for (int i = 0; i < nPlt; i++)
			{
				exp2 += time_weight[t] * Plants[i].emis_rate * prod[n][t][i];
				exp3 += prod[n][t][i];
			}
		}
	}
	IloNumVar Emit_var(env, 0, IloInfinity, ILOFLOAT);
	Model.add(exp2 == Emit_var);
	Model.add(exp2 <= Emis_lim);
	Model.add(exp3 >= RPS * total_demand);
#pragma endregion

#pragma region Solve the model
	std::cout << DV_size << endl;
	std::cout << const_size << endl;
	IloCplex cplex(Model);
	//cplex.setParam(IloCplex::TiLim, 7200);
	//cplex.setParam(IloCplex::EpGap, 0.04); // 4%
	//cplex.exportModel("MA_LP.lp");

	//cplex.setOut(env.getNullStream());

	//cplex.setParam(IloCplex::Param::MIP::Strategy::Branch, 1); //Up/down branch selected first (1,-1),  default:automatic (0)
	//cplex.setParam(IloCplex::Param::MIP::Strategy::BBInterval, 7);// 0 to 7
	//cplex.setParam(IloCplex::Param::MIP::Strategy::NodeSelect, 2);
	//https://www.ibm.com/support/knowledgecenter/SSSA5P_12.9.0/ilog.odms.cplex.help/CPLEX/UsrMan/topics/discr_optim/mip/performance/20_node_seln.html
	//cplex.setParam(IloCplex::Param::MIP::Strategy::VariableSelect, 4);//-1 to 4
	//cplex.setParam(IloCplex::Param::RootAlgorithm, 4); /000 to 6

	//cplex.setParam(IloCplex::Param::Benders::Strategy,3);// IloCplex::BendersFull
	//cplex.setParam(IloCplex::Param::Benders::Strategy, IloCplex::BendersFull);// 

	IloCplex::LongAnnotation
		decomp = cplex.newLongAnnotation(IloCplex::BendersAnnotation,
			CPX_BENDERS_MASTERVALUE + 1);
	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			cplex.setAnnotation(decomp, X[n][i], CPX_BENDERS_MASTERVALUE);
			cplex.setAnnotation(decomp, Xest[n][i], CPX_BENDERS_MASTERVALUE);
			cplex.setAnnotation(decomp, Xdec[n][i], CPX_BENDERS_MASTERVALUE);
		}
		for (int i = 0; i < Enodes[n].adj_buses.size(); i++)
		{
			cplex.setAnnotation(decomp, Ze[n][i], CPX_BENDERS_MASTERVALUE);
		}
	}
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
	std::cout << "Emission tonngage for"<<Te.size()<< " hours:" << cplex.getValue(Emit_var) << endl;
#pragma endregion

	if (PrintVars)
	{
#pragma region print Electricity Network variables
		ofstream fid;
		fid.open("DVe.txt");
		fid << "Elapsed time: " << Elapsed << endl;
		fid << "\t RMP Obj Value:" << obj_val << endl;
		fid << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
		fid << "Number of DVs: " << DV_size << endl;
		fid << "Number of constraints: " << const_size << endl;
		float** XestS = new float* [nEnode];
		float** XdecS = new float* [nEnode];
		float** Xs = new float* [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			XestS[n] = new float[nPlt]();
			XdecS[n] = new float[nPlt]();
			Xs[n] = new float[nPlt]();
			for (int i = 0; i < nPlt; i++)
			{
				XestS[n][i] = cplex.getValue(Xest[n][i]);
				XdecS[n][i] = cplex.getValue(Xdec[n][i]);
				Xs[n][i] = cplex.getValue(X[n][i]);
				if (XestS[n][i] > 0)
				{
					//std::cout << "Xest[" << n << "][" << i << "] = " << XestS[n][i] << endl;
					fid << "Xest[" << n << "][" << i << "] = " << XestS[n][i] << endl;
				}
				if (XdecS[n][i] > 0)
				{
					//std::cout << "Xdec[" << n << "][" << i << "] = " << XdecS[n][i] << endl;
					fid << "Xdec[" << n << "][" << i << "] = " << XdecS[n][i] << endl;
				}
				if (Xs[n][i] > 0)
				{
					//cout << "X[" << n << "][" << i << "] = " << Xs[n][i] << endl;
					fid << "X[" << n << "][" << i << "] = " << Xs[n][i] << endl;
				}
			}
		}

		float** fs = new float* [nBr];
		for (int b = 0; b < nBr; b++)
		{
			fs[b] = new float[Te.size()]();
			for (int t = 0; t < Te.size(); t++)
			{
				fs[b][t] = cplex.getValue(flowE[b][t]);
				if (std::abs(fs[b][t]) > 10e-3)
				{
					//std::cout << "flowE[" << n << "][" << t << "][" << Enodes[n].adj_buses[m] << "] = " << fs[n][t][m] << endl;
					fid << "flowE[" << b << "][" << t << "] = " << fs[b][t] << endl;
				}
			}
		}



		//float** Zs = new float* [nEnode];
		//for (int n = 0; n < nEnode; n++)
		//{
		//	Zs[n] = new float[Enodes[n].adj_buses.size()]();
		//	for (int m = 0; m < Enodes[n].adj_buses.size(); m++)
		//	{
		//		Zs[n][m] = cplex.getValue(Z[n][m]);
		//		if (Zs[n][m] > 10e-3)
		//		{
		//			//std::cout << "Z[" << n << "][" << Enodes[n].adj_buses[m] << "] = " << Zs[n][m] << endl;
		//			fid << "Z[" << n << "][" << Enodes[n].adj_buses[m] << "] = " << Zs[n][m] << endl;
		//		}
		//	}
		//}

		float*** prodS = new float** [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			prodS[n] = new float* [Te.size()];
			for (int t = 0; t < Te.size(); t++)
			{
				if (t > 10) { break; }

				prodS[n][t] = new float[nPlt]();
				for (int i = 0; i < nPlt; i++)
				{
					prodS[n][t][i] = cplex.getValue(prod[n][t][i]);
					if (prodS[n][t][i] > 10e-3)
					{
						//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
						fid << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
					}
				}
			}
		}

		float** curtS = new float* [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			curtS[n] = new float[Te.size()]();
			for (int t = 0; t < Te.size(); t++)
			{
				if (t > 10) { break; }
				curtS[n][t] = cplex.getValue(curtE[n][t]);
				if (curtS[n][t] > 10e-3)
				{
					//	std::cout << "curtE[" << n << "][" << t << "] = " << curtS[n][t] << endl;
					fid << "curtE[" << n << "][" << t << "] = " << curtS[n][t] << endl;
				}
			}
		}



		fid.close();
#pragma endregion

	}
}

