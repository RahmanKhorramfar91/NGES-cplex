#include"Models.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables

NumVar2D EV::Xest; // integer (continues) for plants
NumVar2D EV::Xdec; // integer (continues) for plants
NumVar2D EV::YeCD; // continuous: charge/discharge capacity
NumVar2D EV::YeLev; // continuous: charge/discharge level
IloNumVarArray EV::Ze;
NumVar2D EV::theta; // continuous phase angle
NumVar2D EV::curtE; // continuous curtailment variable
NumVar3D EV::prod;// continuous
NumVar3D EV::eSch;// power charge to storage 
NumVar3D EV::eSdis;// power discharge to storage
NumVar3D EV::eSlev;// power level at storage
NumVar2D EV::X; // integer (continues)
NumVar2D EV::flowE; // unlike the paper, flowE subscripts are "ntm" here
IloNumVar EV::est_cost;
IloNumVar EV::decom_cost;
IloNumVar EV::fixed_cost;
IloNumVar EV::var_cost;
IloNumVar EV::fuel_cost;
IloNumVar EV::shedding_cost;
IloNumVar EV::elec_storage_cost;
IloNumVar EV::Emit_var;



void Populate_EV(bool int_vars_relaxed, IloModel& Model, IloEnv& env)
{

#pragma region Fetch Data

	// set of possible existing plant types
	std::map<string, int> sym2pltType = { {"ng",0},{"dfo", 1},
{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7} };
	std::map<int, string> pltType2sym = { {0,"ng"},{1,"dfo"},
{2,"solar"},{3,"wind"},{4,"wind_offshore"},{5,"hydro"},{6,"coal"},{7,"nuclear"} };
	//int Rn[] = { 2,3,5 };

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
	vector<int> RepDaysCount = Params::RepDaysCount;
	float Emis_lim = Params::Emis_lim;
	float RPS = Params::RPS;
#pragma endregion

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
			Xest[n] = IloNumVarArray(env, nPlt);
			X[n] = IloNumVarArray(env, nPlt);
			Xdec[n] = IloNumVarArray(env, nPlt);
			for (int i = 0; i < nPlt; i++)
			{
				if (Plants[i].type == "solar" || Plants[i].type == "solar-UPV" || Plants[i].type == "wind" || Plants[i].type == "wind-new")
				{
					Xest[n][i] = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
					Xdec[n][i] = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
					X[n][i] = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
				}
				else
				{
					Xest[n][i] = IloNumVar(env, 0, IloInfinity, ILOINT);
					Xdec[n][i] = IloNumVar(env, 0, IloInfinity, ILOINT);
					X[n][i] = IloNumVar(env, 0, IloInfinity, ILOINT);
				}
			}
		}


		//Ze[n] = IloNumVarArray(env, Enodes[n].adj_buses.size(), 0, IloInfinity, ILOBOOL);
		theta[n] = IloNumVarArray(env, Te.size(), -pi, pi, ILOFLOAT);
		curtE[n] = IloNumVarArray(env, Te.size(), 0, IloInfinity, ILOFLOAT);
		YeCD[n] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
		YeLev[n] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);

		prod[n] = NumVar2D(env, Te.size());
		eSch[n] = NumVar2D(env, Te.size());
		eSdis[n] = NumVar2D(env, Te.size());
		eSlev[n] = NumVar2D(env, Te.size());

		//flowE[n] = NumVar2D(env, Te); // ntm

		for (int t = 0; t < Te.size(); t++)
		{
			prod[n][t] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOFLOAT);
			eSch[n][t] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
			eSdis[n][t] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
			eSlev[n][t] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
			//	flowE[n][t] = IloNumVarArray(env, Enodes[n].adj_buses.size(), -IloInfinity, IloInfinity, ILOFLOAT);
		}
	}
	IloNumVar est_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar decom_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar fixed_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar var_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar fuel_cost(env, 0, IloInfinity, ILOFLOAT);
	//IloNumVar emis_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar shedding_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar elec_storage_cost(env, 0, IloInfinity, ILOFLOAT);
	IloNumVar Emit_var(env, 0, IloInfinity, ILOFLOAT);


	EV::curtE = curtE;
	EV::decom_cost = decom_cost;
	EV::elec_storage_cost = elec_storage_cost;
	EV::eSch = eSch;
	EV::eSdis = eSdis;
	EV::eSlev = eSlev;
	EV::est_cost = est_cost;
	EV::fixed_cost = fixed_cost;
	EV::flowE = flowE;
	EV::fuel_cost = fuel_cost;
	EV::prod = prod;
	EV::shedding_cost = shedding_cost;
	EV::theta = theta;
	EV::var_cost = var_cost;
	EV::X = X;
	EV::Xdec = Xdec;
	EV::Xest = Xest;
	EV::YeCD = YeCD;
	EV::YeLev = YeLev;
	EV::Ze = Ze;

#pragma endregion


}

void Elec_Model(IloModel& Model, IloEnv& env)
{
	//auto start = chrono::high_resolution_clock::now();
#pragma region Fetch Data

	// set of possible existing plant types
	std::map<string, int> sym2pltType = { {"ng",0},{"dfo", 1},
{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7} };
	std::map<int, string> pltType2sym = { {0,"ng"},{1,"dfo"},
{2,"solar"},{3,"wind"},{4,"wind_offshore"},{5,"hydro"},{6,"coal"},{7,"nuclear"} };
	//int Rn[] = { 2,3,5 };

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
	vector<int> RepDaysCount = Params::RepDaysCount;
	float Emis_lim = Params::Emis_lim;
	float RPS = Params::RPS;
#pragma endregion


#pragma region Set some variables
	// 1) existing types can not be established because there are new equivalent types
	// 2) new types cannot be decommissioned
	// 3) no new wind-offshore can be established as all buses are located in-land
	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			//// do not allow decommissioning of hydro plants
			//Model.add(EV::Xdec[n][5] == 0);
			if (Plants[i].type == "wind-offshore-new" || Plants[i].type == "hydro-new")
			{
				Model.add(EV::EV::Xest[n][i] == 0);
			}
			if (Plants[i].is_exis == 1)
			{
				Model.add(EV::EV::Xest[n][i] == 0);
			}
			else
			{
				Model.add(EV::Xdec[n][i] == 0);
			}
		}
	}
#pragma endregion


#pragma region Objective Function
	IloExpr ex_est(env);
	IloExpr ex_decom(env);
	IloExpr ex_fix(env);
	IloExpr ex_var(env);
	IloExpr ex_fuel(env);
	//IloExpr ex_emis(env);
	IloExpr ex_shedd(env);
	IloExpr ex_trans(env);
	IloExpr ex_elec_str(env);
	IloExpr exp0(env);

	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			// Investment/decommission cost of plants
			float s1 = std::pow(1.0 / (1 + WACC), Plants[i].lifetime);
			float capco = WACC / (1 - s1);
			ex_est += capco * Plants[i].capex * EV::Xest[n][i];
			ex_decom += Plants[i].decom_cost * EV::Xdec[n][i];
		}

		// fixed cost
		for (int i = 0; i < nPlt; i++)
		{
			ex_fix += Plants[i].fix_cost * EV::X[n][i];
		}
		// var+fuel costs of plants
		for (int t = 0; t < Te.size(); t++)
		{
			for (int i = 0; i < nPlt; i++)
			{
				// var cost
				ex_var += time_weight[t] * Plants[i].var_cost * EV::prod[n][t][i];

				// fuel price to be updated later (dollar per thousand cubic feet=MMBTu)
				if (Plants[i].type == "ng" || Plants[i].type == "CT" || Plants[i].type == "CC" || Plants[i].type == "CC-CCS")
				{
					ex_fuel += time_weight[t] * NG_price * Plants[i].heat_rate * EV::prod[n][t][i];
				}
				else if (Plants[i].type == "dfo")
				{
					ex_fuel += time_weight[t] * dfo_pric * Plants[i].heat_rate * EV::prod[n][t][i];
				}
				else if (Plants[i].type == "coal")
				{
					ex_fuel += time_weight[t] * coal_price * Plants[i].heat_rate * EV::prod[n][t][i];
				}

				// emission cost (to be added later)
				//ex_emis += time_weight[t] * Plants[i].emis_cost * Plants[i].emis_rate * EV::prod[n][t][i];

			}

			// load curtailment cost
			ex_shedd += time_weight[t] * E_curt_cost * EV::curtE[n][t];
		}


		// storage cost
		for (int r = 0; r < neSt; r++)
		{
			ex_elec_str += Estorage[r].power_cost * EV::YeCD[n][r] + Estorage[r].energy_cost * EV::YeLev[n][r];
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
		ex_trans += capco * trans_unit_cost * Branches[b].maxFlow * Branches[b].length * EV::Ze[b];
	}
	exp0 = ex_est + ex_decom + ex_fix + ex_var + ex_fuel + ex_shedd + ex_trans + ex_elec_str;
	Model.add(IloMinimize(env, exp0));
	Model.add(EV::est_cost == ex_est);
	Model.add(EV::decom_cost == ex_decom);
	Model.add(EV::fixed_cost == ex_fix);
	Model.add(EV::var_cost == ex_var);
	Model.add(EV::fuel_cost == ex_fuel);
	//Model.add(emis_cost == ex_emis);
	Model.add(EV::shedding_cost == ex_shedd);
	Model.add(EV::elec_storage_cost == ex_elec_str);

	exp0.end();
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
				Model.add(EV::X[n][i] == Enodes[n].Init_plt_count[ind1] - EV::Xdec[n][i] + EV::Xest[n][i]);
				const_size++;
				j++;
			}
			else
			{
				Model.add(EV::X[n][i] == -EV::Xdec[n][i] + EV::Xest[n][i]);
				const_size++;
			}
		}
	}

	// C2: maximum number of each plant type
	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			Model.add(EV::X[n][i] <= Plants[i].Umax);
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
				Model.add(EV::prod[n][t][i] >= Plants[i].Pmin * EV::X[n][i]);
				Model.add(EV::prod[n][t][i] <= Plants[i].Pmax * EV::X[n][i]);
				const_size += 2;

				if (t > 0)
				{
					Model.add(-Plants[i].rampD * EV::X[n][i] <= EV::prod[n][t][i] - EV::prod[n][t - 1][i]);
					Model.add(EV::prod[n][t][i] - EV::prod[n][t - 1][i] <= Plants[i].rampU * EV::X[n][i]);
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
		//Model.add(EV::Ze[fb][tbi]); 
		const_size++;
		int fbi = std::find(Enodes[tb].adj_buses.begin(), Enodes[tb].adj_buses.end(), fb) - Enodes[tb].adj_buses.begin();
		//Model.add(EV::Ze[tb][fbi]); 
		const_size++;
		for (int t = 0; t < Te.size(); t++)
		{
			//Model.add(IloAbs(EV::flowE[fb][t][tbi] + EV::flowE[tb][t][fbi]) <= 10e-3);
			if (Branches[br].is_exist == 1)
			{
				//Model.add(IloAbs(EV::flowE[br][t]) <= Branches[br].maxFlow);
				Model.add(EV::flowE[br][t] <= Branches[br].maxFlow);
				Model.add(-Branches[br].maxFlow <= EV::flowE[br][t]);
				const_size++;
			}
			else
			{
				//Model.add(IloAbs(EV::flowE[br][t]) <= Branches[br].maxFlow * EV::Ze[fb][tbi]);
				Model.add(EV::flowE[br][t] <= Branches[br].maxFlow * EV::Ze[br]);
				Model.add(-Branches[br].maxFlow * EV::Ze[br] <= EV::flowE[br][t]);
				const_size++;
			}
		}
	}

	//C7: energy balance
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			IloExpr exp_prod(env);
			for (int i = 0; i < nPlt; i++)
			{
				exp_prod += EV::prod[n][t][i];
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
					Model.add(EV::flowE[l2][t]);
					const_size++;
					exp_trans += EV::flowE[l2][t];
				}
			}
			IloExpr ex_store(env);
			for (int r = 0; r < neSt; r++)
			{
				ex_store += EV::eSdis[n][t][r] - EV::eSch[n][t][r];
			}
			float dem = Enodes[n].demand[Te[t]];
			//Model.add(exp1 + EV::curtE[n][t] == dem); // ignore transmission
			//Model.add(exp_prod + ex_store + EV::curtE[n][t] == dem); const_size++;
			Model.add(exp_prod + exp_trans + ex_store + EV::curtE[n][t] == dem); const_size++;
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
				Model.add(EV::flowE[br][t] - Branches[br].suscep * (EV::theta[tb][t] - EV::theta[fb][t]) <= 1e-2);
				Model.add(-1e-2 <= EV::flowE[br][t] - Branches[br].suscep * (EV::theta[tb][t] - EV::theta[fb][t]));
			}
			else
			{
				Model.add(EV::flowE[br][t] - Branches[br].suscep * (EV::theta[tb][t] - EV::theta[fb][t]) <= 1e-2 + Branches[br].suscep * 24 * (1 - EV::Ze[br]));
				Model.add(-1e-2 - Branches[br].suscep * 24 * (1 - EV::Ze[br]) <= EV::flowE[br][t] - Branches[br].suscep * (EV::theta[tb][t] - EV::theta[fb][t]));
			}
			//Model.add(EV::flowE[fb][t][tbi] == Branches[br].suscep * (EV::theta[tb][t] - EV::theta[fb][t]));
			//Model.add(EV::flowE[tb][t][fbi] == Branches[br].suscep * (EV::theta[fb][t] - EV::theta[tb][t]));
			//Model.add(IloAbs(EV::flowE[br][t] - Branches[br].suscep * (EV::theta[tb][t] - EV::theta[fb][t])) <= 1e-2);
			//Model.add(IloAbs(EV::flowE[tb][t][fbi] - Branches[br].suscep * (EV::theta[fb][t] - EV::theta[tb][t])) <= 1e-2);
			const_size += 2;
		}
	}

	// C9: phase angle (theta) limits. already applied in the definition of the variable
	for (int n = 0; n < nEnode; n++)
	{
		Model.add(EV::theta[n][0] == 0); const_size++;
		for (int t = 1; t < Te.size(); t++)
		{
			//Model.add(IloAbs(EV::theta[n][t]) <= pi);
			Model.add(EV::theta[n][t] <= pi);
			Model.add(-pi <= EV::theta[n][t]);
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
					Model.add(EV::prod[n][t][i] <= Plants[i].prod_profile[Te[t]] * EV::X[n][i]);
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
			Model.add(EV::curtE[n][t] <= dem); const_size++;
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
				exp2 += time_weight[t] * Plants[i].emis_rate * EV::prod[n][t][i];
				if (Plants[i].type == "solar" || Plants[i].type == "wind" || Plants[i].type == "hydro" || Plants[i].type == "solar-UPV" || Plants[i].type == "wind-new" || Plants[i].type == "hydro-new")
				{
					exp3 += EV::prod[n][t][i]; // production from VRE
				}

			}
		}
	}

	//Model.add(exp2 == EV::Emit_var[0]);
	//Model.add(exp2 <= Emis_lim);
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
					Model.add(EV::eSlev[n][t][r] == 0);
				}
				else
				{
					Model.add(EV::eSlev[n][t][r] == EV::eSlev[n][t - 1][r] + Estorage[r].eff_ch * EV::eSch[n][t][r] - (EV::eSdis[n][t][r] / Estorage[r].eff_disCh));
				}
				Model.add(EV::YeCD[n][r] >= EV::eSdis[n][t][r]);
				Model.add(EV::YeCD[n][r] >= EV::EV::eSch[n][t][r]);

				Model.add(EV::YeLev[n][r] >= EV::eSlev[n][t][r]);
			}
		}
	}
#pragma endregion

}



