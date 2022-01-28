#include"Models.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables

void NGES_Model(bool int_vars_relaxed, bool PrintVars)
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


#pragma region Decision Variables

#pragma region Electricity network DVs

	NumVar2D Xest(env, nEnode); // integer (continues) for plants
	NumVar2D Xdec(env, nEnode); // integer (continues) for plants
	NumVar2D YeCD(env, nEnode); // continuous: charge/discharge capacity
	NumVar2D YeLev(env, nEnode); // continuous: charge/discharge level
	IloNumVarArray Ze(env, nBr, 0, IloInfinity, ILOBOOL);
	NumVar2D theta(env, nEnode); // continuous phase angle
	NumVar2D curtE(env, nEnode); // continuous curtailment variable

	NumVar3D prod(env, nEnode);// continuous
	NumVar3D eSch(env, nEnode);// power charge to storage 
	NumVar3D eSdis(env, nEnode);// power discharge to storage
	NumVar3D eSlev(env, nEnode);// power level at storage

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
		}
		else
		{
			Xest[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
			Xdec[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
			X[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOINT);
		}

		theta[n] = IloNumVarArray(env, Te.size(), -pi, pi, ILOFLOAT);
		curtE[n] = IloNumVarArray(env, Te.size(), 0, IloInfinity, ILOFLOAT);
		YeCD[n] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
		YeLev[n] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);

		prod[n] = NumVar2D(env, Te.size());
		eSch[n] = NumVar2D(env, Te.size());
		eSdis[n] = NumVar2D(env, Te.size());
		eSlev[n] = NumVar2D(env, Te.size());

		for (int t = 0; t < Te.size(); t++)
		{
			prod[n][t] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOFLOAT);
			eSch[n][t] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
			eSdis[n][t] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
			eSlev[n][t] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
		}
	}
#pragma endregion

#pragma region NG network DVs
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
		flowGE[k] = NumVar2D(env, nEnode);
		for (int n = 0; n < nEnode; n++)
		{
			flowGE[k][n] = IloNumVarArray(env, Tg.size(), 0, IloInfinity, ILOFLOAT);
		}
	}
#pragma endregion

#pragma endregion


#pragma region Objective Function
	IloExpr exp0(env);
#pragma region Electricity Network Costs
	IloExpr ex_est(env);
	IloExpr ex_decom(env);
	IloExpr ex_fix(env);
	IloExpr ex_var(env);
	IloExpr ex_fuel(env);
	IloExpr ex_emis(env);
	IloExpr ex_shedd(env);
	IloExpr ex_trans(env);
	IloExpr ex_elec_str(env);

	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			// Investment/decommission cost of plants
			float s1 = std::pow(1.0 / (1 + WACC), Plants[i].lifetime);
			float capco = WACC / (1 - s1);
			ex_est += capco * Plants[i].capex * Xest[n][i];
			ex_decom += Plants[i].decom_cost * Xdec[n][i];
		}

		// fixed cost
		for (int i = 0; i < nPlt; i++)
		{
			ex_fix += Plants[i].fix_cost * X[n][i];
		}
		// var+fuel costs of plants
		for (int t = 0; t < Te.size(); t++)
		{
			for (int i = 0; i < nPlt; i++)
			{
				// var cost
				ex_var += time_weight[t] * Plants[i].var_cost * prod[n][t][i];

				// fuel price to be updated later (dollar per thousand cubic feet=MMBTu)
				if (Plants[i].type == "ng" || Plants[i].type == "CT" || Plants[i].type == "CC" || Plants[i].type == "CC-CCS")
				{
					ex_fuel += time_weight[t] * NG_price * Plants[i].heat_rate * prod[n][t][i];
				}
				else if (Plants[i].type == "dfo")
				{
					ex_fuel += time_weight[t] * dfo_pric * Plants[i].heat_rate * prod[n][t][i];
				}
				else if (Plants[i].type == "coal")
				{
					ex_fuel += time_weight[t] * coal_price * Plants[i].heat_rate * prod[n][t][i];
				}

				// emission cost (to be added later)
				ex_emis += time_weight[t] * Plants[i].emis_cost * Plants[i].emis_rate * prod[n][t][i];
			}

			// load curtailment cost
			ex_shedd += time_weight[t] * E_curt_cost * curtE[n][t];
		}


		// storage cost
		for (int r = 0; r < neSt; r++)
		{
			ex_elec_str += Estorage[r].power_cost * YeCD[n][r] + Estorage[r].energy_cost * YeLev[n][r];
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
		ex_trans += capco * trans_unit_cost * Branches[b].length * Ze[b];
	}
	exp0 = ex_est + ex_decom + ex_fix + ex_var + ex_fuel + ex_emis + ex_shedd + ex_trans + ex_elec_str;

	IloNumVar est_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar decom_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar fixed_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar var_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar fuel_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar emis_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar shedding_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar elec_storage_cost(env, 0, IloInfinity, ILOFLOAT);
	Model.add(est_cost == ex_est);
	Model.add(decom_cost == ex_decom);
	Model.add(fixed_cost == ex_fix);
	Model.add(var_cost == ex_var);
	Model.add(fuel_cost == ex_fuel);
	Model.add(emis_cost == ex_emis);
	Model.add(shedding_cost == ex_shedd);
	Model.add(elec_storage_cost == ex_elec_str);

#pragma endregion


#pragma region NG network related costs
	IloExpr exp_pipe(env);
	IloExpr exp_curt(env);
	IloExpr exp_store(env);
	exp_store += 0;
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
	exp0 += exp_pipe + exp_curt + exp_store;
	IloNumVar pipe_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar gSshedd_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar gStore_cost(env, 0, IloInfinity, ILOFLOAT);
	Model.add(pipe_cost == exp_pipe);
	Model.add(gSshedd_cost == exp_curt);
	Model.add(gStore_cost == exp_store);
#pragma endregion
	Model.add(IloMinimize(env, exp0));
#pragma endregion


#pragma region Constraints


#pragma region Fix some Electricity Network variables
	// 1) existing types can not be established because there are new equivalent types
	// 2) new types cannot be decommissioned
	// 3) no new wind-offshore can be established as all buses are located in-land
	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			//// do not allow decommissioning of hydro plants
			//Model.add(Xdec[n][5] == 0);
			if (Plants[i].type == "wind-offshore-new")
			{
				Model.add(Xest[n][i] == 0);
			}
			if (Plants[i].is_exis == 1)
			{
				Model.add(Xest[n][i] == 0);
			}
			else
			{
				Model.add(Xdec[n][i] == 0);
			}
		}
	}
#pragma endregion


#pragma region Electricity Network Constraints
	int const_size = 0;

	// C1: number of generation units at each node
	int existP = 0;
	for (int n = 0; n < nEnode; n++)
	{
		int j = 0;
		bool if_off_shore = std::find(Enodes[n].Init_plt_types.begin(), Enodes[n].Init_plt_types.end(), "wind_offshore") != Enodes[n].Init_plt_types.end();

		for (int i = 0; i < nPlt; i++)
		{
			// is plant type i part of initial plants types of node n:
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

	// C2: maximum number of each plant type
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
		//Model.add(Ze[fb][tbi]); 
		const_size++;
		int fbi = std::find(Enodes[tb].adj_buses.begin(), Enodes[tb].adj_buses.end(), fb) - Enodes[tb].adj_buses.begin();
		//Model.add(Ze[tb][fbi]); 
		const_size++;
		for (int t = 0; t < Te.size(); t++)
		{
			if (Branches[br].is_exist == 1)
			{
				Model.add(flowE[br][t] <= Branches[br].maxFlow);
				Model.add(-Branches[br].maxFlow <= flowE[br][t]);
				const_size++;
			}
			else
			{
				Model.add(flowE[br][t] <= Branches[br].maxFlow * Ze[br]);
				Model.add(-Branches[br].maxFlow * Ze[br] <= flowE[br][t]);
				const_size++;
			}
		}
	}

	//C7: flow balance
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			IloExpr exp_prod(env);
			for (int i = 0; i < nPlt; i++)
			{
				exp_prod += prod[n][t][i];
			}
			IloExpr exp_trans(env);
			for (int m : Enodes[n].adj_buses)
			{
				int key1 = n * 200 + m;
				// check if this key exist, if not, the other order exisit
				if (Le.count(key1) == 0)
				{
					key1 = m * 200 + n;
				}
				//each Le can contains multiple lines
				for (int l2 : Le[key1])
				{
					Model.add(flowE[l2][t]);
					const_size++;
					exp_trans += flowE[l2][t];
				}
			}
			IloExpr ex_store(env);
			for (int r = 0; r < neSt; r++)
			{
				ex_store += eSdis[n][t][r] - eSch[n][t][r];
			}
			float dem = Enodes[n].demand[Te[t]];
			//Model.add(exp1 + curtE[n][t] == dem); // ignore transmission
			Model.add(exp_prod + exp_trans + ex_store + curtE[n][t] == dem); const_size++;
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
				Model.add(flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t]) <= 1e-2);
				Model.add(-1e-2 <= flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t]));
			}
			else
			{
				Model.add(flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t]) <= 1e-2 + Branches[br].suscep * 24 * (1 - Ze[br]));
				Model.add(-1e-2 - Branches[br].suscep * 24 * (1 - Ze[br]) <= flowE[br][t] - Branches[br].suscep * (theta[tb][t] - theta[fb][t]));
			}
		}
	}

	// C9: phase angle (theta) limits. already applied in the definition of the variable
	for (int n = 0; n < nEnode; n++)
	{
		Model.add(theta[n][0] == 0); const_size++;
		for (int t = 1; t < Te.size(); t++)
		{
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
			for (int i = 0; i < nPlt; i++)
			{
				if (Plants[i].type == "solar" || Plants[i].type == "wind" || Plants[i].type == "hydro" || Plants[i].type == "solar-UPV" || Plants[i].type == "wind-new" || Plants[i].type == "hydro-new")
				{
					Model.add(prod[n][t][i] <= Plants[i].prod_profile[Te[t]] * X[n][i]);
					const_size++;
				}
			}
		}
	}


	// C11: demand curtainlment constraint
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			float dem = Enodes[n].demand[Te[t]];
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
			total_demand += Enodes[n].demand[Te[t]];

			for (int i = 0; i < nPlt; i++)
			{
				exp2 += time_weight[t] * Plants[i].emis_rate * prod[n][t][i];
				if (Plants[i].type == "solar" || Plants[i].type == "wind" || Plants[i].type == "hydro" || Plants[i].type == "solar-UPV" || Plants[i].type == "wind-new" || Plants[i].type == "hydro-new")
				{
					exp3 += prod[n][t][i]; // production from VRE
				}

			}
		}
	}
	IloNumVar Emit_var(env, 0, IloInfinity, ILOFLOAT);
	Model.add(exp2 == Emit_var);
	Model.add(exp2 <= Emis_lim);
	Model.add(exp3 >= RPS * total_demand);


	// C14,C15,C16
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			for (int r = 0; r < neSt; r++)
			{
				if (t == 0)
				{
					Model.add(eSlev[n][t][r] == 0);
				}
				else
				{
					Model.add(eSlev[n][t][r] == eSlev[n][t - 1][r] + Estorage[r].eff_ch * eSch[n][t][r] - (eSdis[n][t][r] / Estorage[r].eff_disCh));
				}
				Model.add(YeCD[n][r] >= eSdis[n][t][r]);
				Model.add(YeCD[n][r] >= eSch[n][t][r]);

				Model.add(YeLev[n][r] >= eSlev[n][t][r]);
			}
		}
	}
#pragma endregion

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

#pragma region Coupling Constraints
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			for (int n : Gnodes[k].adjE)
			{
				IloExpr exp2(env);
				for (int t = tau * 24; t < (tau + 1) * 24; t++)
				{
					for (int i = 0; i < nPlt; i++)
					{
						if (Plants[i].type == "ng" || Plants[i].type == "CT" || Plants[i].type == "CC" || Plants[i].type == "CC-CCS")
						{
							exp2 += Plants[i].heat_rate * prod[n][t][i];
						}
					}
				}
				Model.add(flowGE[k][n][tau] - exp2 <= 1e-2);
				Model.add(exp2 - flowGE[k][n][tau] <= 1e-2);
			}
		}
	}
#pragma endregion

#pragma endregion



#pragma region Solve the model
	IloCplex cplex(Model);
	//cplex.setParam(IloCplex::TiLim, 7200);
	cplex.setParam(IloCplex::EpGap, 0.01); // 4%
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

	/*IloCplex::LongAnnotation
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
	}*/
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
	std::cout << "\tNGES Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	std::cout << "Emission tonngage for 2050: " << cplex.getValue(Emit_var) << endl;
#pragma endregion

	if (PrintVars)
	{
#pragma region print Electricity Network variables
		int periods2print = 20;
		ofstream fid;
		fid.open("DVe.txt");
		fid << "Elapsed time: " << Elapsed << endl;
		fid << "\t Electricity Network Obj Value:" << obj_val << endl;
		fid << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
		fid << "\n \t Number of constraints: " << const_size << endl;
		fid << "\n \t Establishment Cost: " << cplex.getValue(est_cost);
		fid << "\n \t Decommissioning Cost: " << cplex.getValue(decom_cost);
		fid << "\n \t Fixed Cost: " << cplex.getValue(fixed_cost);
		fid << "\n \t Variable Cost: " << cplex.getValue(var_cost);
		fid << "\n \t Emission Cost: " << cplex.getValue(emis_cost);
		fid << "\n \t Fuel Cost: " << cplex.getValue(fuel_cost);
		fid << "\n \t Load Shedding Cost: " << cplex.getValue(shedding_cost);
		fid << "\n \t Storage Cost: " << cplex.getValue(elec_storage_cost) << "\n\n";

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
				if (t > periods2print) { break; }

				fs[b][t] = cplex.getValue(flowE[b][t]);
				if (std::abs(fs[b][t]) > 10e-3)
				{
					//std::cout << "flowE[" << n << "][" << t << "][" << Enodes[n].adj_buses[m] << "] = " << fs[n][t][m] << endl;
					fid << "flowE[" << b << "][" << t << "] = " << fs[b][t] << endl;
				}
			}
		}



		float* Zs = new float[nBr]();
		for (int b = 0; b < nBr; b++)
		{
			Zs[b] = cplex.getValue(Ze[b]);
			if (Zs[b] > 10e-3)
			{
				//std::cout << "Z[" << n << "][" << Enodes[n].adj_buses[m] << "] = " << Zs[n][m] << endl;
				fid << "Z[" << b << "] = " << Zs[b] << endl;
			}
		}

		float*** prodS = new float** [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			prodS[n] = new float* [Te.size()];
			for (int t = 0; t < Te.size(); t++)
			{
				if (t > periods2print) { break; }

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
				if (t > periods2print) { break; }
				curtS[n][t] = cplex.getValue(curtE[n][t]);
				if (curtS[n][t] > 10e-3)
				{
					//	std::cout << "curtE[" << n << "][" << t << "] = " << curtS[n][t] << endl;
					fid << "curtE[" << n << "][" << t << "] = " << curtS[n][t] << endl;
				}
			}
		}

		// Storage variables

		float** YeCDs = new float* [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			YeCDs[n] = new float[neSt]();
			for (int r = 0; r < neSt; r++)
			{
				YeCDs[n][r] = cplex.getValue(YeCD[n][r]);
				if (YeCDs[n][r] > 10e-3)
				{
					fid << "Ye_ch[" << n << "][" << r << "] = " << YeCDs[n][r] << endl;
				}
			}
		}
		float** YeLevs = new float* [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			YeLevs[n] = new float[neSt]();
			for (int r = 0; r < neSt; r++)
			{
				YeLevs[n][r] = cplex.getValue(YeLev[n][r]);
				if (YeLevs[n][r] > 10e-3)
				{
					fid << "Ye_lev[" << n << "][" << r << "] = " << YeLevs[n][r] << endl;
				}
			}
		}


		float*** eSchS = new float** [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			eSchS[n] = new float* [Te.size()];
			for (int t = 0; t < Te.size(); t++)
			{
				if (t > periods2print) { break; }

				eSchS[n][t] = new float[nPlt]();
				for (int i = 0; i < neSt; i++)
				{
					eSchS[n][t][i] = cplex.getValue(eSch[n][t][i]);
					if (eSchS[n][t][i] > 10e-3)
					{
						//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
						fid << "eS_ch[" << n << "][" << t << "][" << i << "] = " << eSchS[n][t][i] << endl;
					}
				}
			}
		}
		float*** eSdisS = new float** [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			eSdisS[n] = new float* [Te.size()];
			for (int t = 0; t < Te.size(); t++)
			{
				if (t > periods2print) { break; }

				eSdisS[n][t] = new float[nPlt]();
				for (int i = 0; i < neSt; i++)
				{
					eSdisS[n][t][i] = cplex.getValue(eSdis[n][t][i]);
					if (eSdisS[n][t][i] > 10e-3)
					{
						//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
						fid << "eS_dis[" << n << "][" << t << "][" << i << "] = " << eSdisS[n][t][i] << endl;
					}
				}
			}
		}

		float*** eSlevS = new float** [nEnode];
		for (int n = 0; n < nEnode; n++)
		{
			eSlevS[n] = new float* [Te.size()];
			for (int t = 0; t < Te.size(); t++)
			{
				if (t > periods2print) { break; }

				eSlevS[n][t] = new float[nPlt]();
				for (int i = 0; i < neSt; i++)
				{
					eSlevS[n][t][i] = cplex.getValue(eSlev[n][t][i]);
					if (eSlevS[n][t][i] > 10e-3)
					{
						//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
						fid << "eS_lev[" << n << "][" << t << "][" << i << "] = " << eSlevS[n][t][i] << endl;
					}
				}
			}
		}
		fid.close();
#pragma endregion

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















