#include"Models_Funcs.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables

NumVar2D EV::Xest; // integer (continues) for plants
NumVar2D EV::Xdec; // integer (continues) for plants
NumVar2D EV::YeCD; // continuous: charge/discharge capacity
NumVar2D EV::YeLev; // continuous: charge/discharge level
NumVar2D EV::YeStr;
IloNumVarArray EV::Ze;
NumVar2D EV::theta; // continuous phase angle
NumVar2D EV::curtE; // continuous curtailment variable
NumVar3D EV::prod;// continuous
NumVar3D EV::eSch;// power charge to storage 
NumVar3D EV::eSdis;// power discharge to storage
NumVar3D EV::eSlev;// power level at storage
NumVar2D EV::Xop; // integer (continues)
NumVar2D EV::flowE; // unlike the paper, flowE subscripts are "ntm" here
IloNumVar EV::est_cost;
IloNumVar EV::decom_cost;
IloNumVar EV::fixed_cost;
IloNumVar EV::var_cost;
IloNumVar EV::thermal_fuel_cost;
IloNumVar EV::shedding_cost;
IloNumVar EV::elec_storage_cost;
IloNumVar EV::Emit_var;

void Populate_EV(bool int_vars_relaxed, IloModel& Model, IloEnv& env)
{

#pragma region Fetch Data

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
	NumVar2D YeStr(env, nEnode); // (binary) if a storage is established
	IloNumVarArray Ze(env, nBr, 0, IloInfinity, ILOBOOL);
	NumVar2D theta(env, nEnode); // continuous phase angle
	NumVar2D curtE(env, nEnode); // continuous curtailment variable

	NumVar3D prod(env, nEnode);// continuous
	NumVar3D eSch(env, nEnode);// power charge to storage 
	NumVar3D eSdis(env, nEnode);// power discharge to storage
	NumVar3D eSlev(env, nEnode);// power level at storage

	NumVar2D Xop(env, nEnode); // integer (continues)
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
			Xop[n] = IloNumVarArray(env, nPlt, 0, IloInfinity, ILOFLOAT);
		}
		else
		{
			Xest[n] = IloNumVarArray(env, nPlt);
			Xop[n] = IloNumVarArray(env, nPlt);
			Xdec[n] = IloNumVarArray(env, nPlt);
			for (int i = 0; i < nPlt; i++)
			{
				if (Plants[i].type == "solar-UPV" || Plants[i].type == "wind-new")
				{
					Xest[n][i] = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
					Xdec[n][i] = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
					Xop[n][i] = IloNumVar(env, 0, IloInfinity, ILOFLOAT);
				}
				else
				{
					Xest[n][i] = IloNumVar(env, 0, IloInfinity, ILOINT);
					Xdec[n][i] = IloNumVar(env, 0, IloInfinity, ILOINT);
					Xop[n][i] = IloNumVar(env, 0, IloInfinity, ILOINT);
				}
			}
		}


		//Ze[n] = IloNumVarArray(env, Enodes[n].adj_buses.size(), 0, IloInfinity, ILOBOOL);
		theta[n] = IloNumVarArray(env, Te.size(), -pi, pi, ILOFLOAT);
		curtE[n] = IloNumVarArray(env, Te.size(), 0, IloInfinity, ILOFLOAT);
		YeCD[n] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
		YeLev[n] = IloNumVarArray(env, neSt, 0, IloInfinity, ILOFLOAT);
		YeStr[n] = IloNumVarArray(env, neSt, 0, 1, ILOBOOL);

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
	IloNumVar thermal_fuel_cost(env, 0, IloInfinity, ILOFLOAT);
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
	EV::thermal_fuel_cost = thermal_fuel_cost;
	EV::prod = prod;
	EV::shedding_cost = shedding_cost;
	EV::theta = theta;
	EV::var_cost = var_cost;
	EV::Xop = Xop;
	EV::Xdec = Xdec;
	EV::Xest = Xest;
	EV::YeCD = YeCD;
	EV::YeLev = YeLev;
	EV::YeStr = YeStr;
	EV::Ze = Ze;

#pragma endregion


}

void Elec_Model(IloModel& Model, IloEnv& env)
{
	//auto start = chrono::high_resolution_clock::now();
#pragma region Fetch Data

	std::map<string, int> sym2pltType = { {"ng",0},{"dfo", 1},
{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7},
		{"Ct",8},{"CC",9},{"CC-CCS",10},{"solar-UPV",11},{"wind-new",12},
		{"wind-offshore-new",13},{"hydro-new",14},{"nuclear-new",15} };
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
	float nuclear_price = Params::nuclear_price;
	float E_curt_cost = Params::E_curt_cost;
	float G_curt_cost = Params::G_curt_cost;
	float pipe_per_mile = Params::pipe_per_mile;
	int pipe_lifespan = Params::pipe_lifespan;
	int battery_lifetime = Params::battery_lifetime;
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
			//// do not allow establishment off-shore wind and hydro plants
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
	IloExpr ex_thermal_fuel(env);
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

		// fixed cost (annual, so no iteration over time)
		for (int i = 0; i < nPlt; i++)
		{
			ex_fix += Plants[i].fix_cost * EV::Xop[n][i];
		}
		// var+fuel costs of plants
		for (int t = 0; t < Te.size(); t++)
		{
			for (int i = 0; i < nPlt; i++)
			{
				// var cost
				ex_var += time_weight[t] * Plants[i].var_cost * EV::prod[n][t][i];

				// fuel price to be updated later (dollar per thousand cubic feet=MMBTu)
				// NG fuel is handle in the NG network
				if (Plants[i].type == "dfo")
				{
					ex_thermal_fuel += time_weight[t] * dfo_pric * Plants[i].heat_rate * EV::prod[n][t][i];
				}
				else if (Plants[i].type == "coal")
				{
					ex_thermal_fuel += time_weight[t] * coal_price * Plants[i].heat_rate * EV::prod[n][t][i];
				}
				else if (Plants[i].type == "nuclear" || Plants[i].type == "nuclear-new")
				{
					ex_thermal_fuel += time_weight[t] * nuclear_price * Plants[i].heat_rate * EV::prod[n][t][i];
				}
			}

			// load curtailment cost
			ex_shedd += time_weight[t] * E_curt_cost * EV::curtE[n][t];
		}


		// storage cost
		for (int r = 0; r < neSt; r++)
		{
			float s1 = std::pow(1.0 / (1 + WACC), battery_lifetime);
			float capco = WACC / (1 - s1);
			ex_elec_str += capco * Estorage[r].power_cost * EV::YeCD[n][r] + capco * Estorage[r].energy_cost * EV::YeLev[n][r];
			ex_elec_str += Estorage[r].FOM * EV::YeStr[n][r]; // fixed cost per kw per year
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
	exp0 = ex_est + ex_decom + ex_fix + ex_var + ex_thermal_fuel + ex_shedd + ex_trans + ex_elec_str;
	Model.add(IloMinimize(env, exp0));
	Model.add(EV::est_cost == ex_est);
	Model.add(EV::decom_cost == ex_decom);
	Model.add(EV::fixed_cost == ex_fix);
	Model.add(EV::var_cost == ex_var);
	Model.add(EV::thermal_fuel_cost == ex_thermal_fuel);
	//Model.add(emis_cost == ex_emis);
	Model.add(EV::shedding_cost == ex_shedd);
	Model.add(EV::elec_storage_cost == ex_elec_str);

	exp0.end();
#pragma endregion

#pragma region Electricity Network Constraints
	int const_size = 0;

	// C1, C2: number of generation units at each node
	int existP = 0;
	for (int n = 0; n < nEnode; n++)
	{
		for (int i = 0; i < nPlt; i++)
		{
			// is plant type i part of initial plants types of node n:
			bool isInd = std::find(Enodes[n].Init_plt_ind.begin(), Enodes[n].Init_plt_ind.end(), i) != Enodes[n].Init_plt_ind.end();
			if (isInd)
			{
				// find the index of plant in the set of Init_plants of the node
				string tp = pltType2sym[i];
				int ind1 = std::find(Enodes[n].Init_plt_types.begin(), Enodes[n].Init_plt_types.end(), tp) - Enodes[n].Init_plt_types.begin();
				Model.add(EV::Xop[n][i] == Enodes[n].Init_plt_count[ind1] - EV::Xdec[n][i] + EV::Xest[n][i]);
				const_size++;

			}
			else
			{
				Model.add(EV::Xop[n][i] == -EV::Xdec[n][i] + EV::Xest[n][i]);
				const_size++;
			}

			//C2: maximum number of each plant type
			Model.add(EV::Xop[n][i] <= Plants[i].Umax); const_size++;
		}
	}

	//C3, C4: production limit, ramping
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			for (int i = 0; i < nPlt; i++)
			{
				Model.add(EV::prod[n][t][i] >= Plants[i].Pmin * EV::Xop[n][i]);
				Model.add(EV::prod[n][t][i] <= Plants[i].Pmax * EV::Xop[n][i]);
				const_size += 2;

				if (t > 0)
				{
					Model.add(-Plants[i].rampD * EV::Xop[n][i] <= EV::prod[n][t][i] - EV::prod[n][t - 1][i]);
					Model.add(EV::prod[n][t][i] - EV::prod[n][t - 1][i] <= Plants[i].rampU * EV::Xop[n][i]);
					const_size += 2;
				}
			}
		}
	}

	// C5, C6: flow limit for electricity
	for (int br = 0; br < nBr; br++)
	{
		//int fb = Branches[br].from_bus;
		//int tb = Branches[br].to_bus;
		//int tbi = std::find(Enodes[fb].adj_buses.begin(), Enodes[fb].adj_buses.end(), tb) - Enodes[fb].adj_buses.begin();
		//Model.add(EV::Ze[fb][tbi]); 
		const_size++;
		//int fbi = std::find(Enodes[tb].adj_buses.begin(), Enodes[tb].adj_buses.end(), fb) - Enodes[tb].adj_buses.begin();
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
		//int tbi = std::find(Enodes[fb].adj_buses.begin(), Enodes[fb].adj_buses.end(), tb) - Enodes[fb].adj_buses.begin();
		//int fbi = std::find(Enodes[tb].adj_buses.begin(), Enodes[tb].adj_buses.end(), fb) - Enodes[tb].adj_buses.begin();

		for (int t = 0; t < Te.size(); t++)
		{
			if (Branches[br].is_exist == 1)
			{
				Model.add(EV::flowE[br][t] - Branches[br].suscep * (EV::theta[tb][t] - EV::theta[fb][t]) <= 1e-2);
				Model.add(-1e-2 <= EV::flowE[br][t] - Branches[br].suscep * (EV::theta[tb][t] - EV::theta[fb][t]));
			}
			else
			{
				// Big M = Branches[br].suscep * 24
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
				if (Plants[i].type == "solar" || Plants[i].type == "wind" ||
					Plants[i].type == "hydro" || Plants[i].type == "solar-UPV" ||
					Plants[i].type == "wind-new" || Plants[i].type == "hydro-new")
				{
					Model.add(EV::prod[n][t][i] <= Plants[i].prod_profile[Te[t]] * EV::Xop[n][i]);
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

	//C12: RPS constraints
	//IloExpr exp2(env);
	IloExpr exp3(env);
	float total_demand = 0;
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			total_demand += time_weight[t] * Enodes[n].demand[Te[t]];

			for (int i = 0; i < nPlt; i++)
			{
				//exp2 += time_weight[t] * Plants[i].emis_rate * EV::prod[n][t][i];
				if (Plants[i].type == "solar" || Plants[i].type == "wind" ||
					Plants[i].type == "hydro" || Plants[i].type == "solar-UPV" ||
					Plants[i].type == "wind-new" || Plants[i].type == "hydro-new")
				{
					exp3 += time_weight[t] * EV::prod[n][t][i]; // production from VRE
				}

			}
		}
	}

	//Model.add(exp2 == EV::Emit_var[0]);
	//Model.add(exp2 <= Emis_lim);
	Model.add(exp3 >= RPS * total_demand);

	// C14,C15,C16 storage constraints
	for (int n = 0; n < nEnode; n++)
	{
		for (int t = 0; t < Te.size(); t++)
		{
			for (int r = 0; r < neSt; r++)
			{
				if (t == Te.size()-1)
				{
					Model.add(EV::eSlev[n][t][r] <= EV::eSlev[n][0][r]);
				}
				else if (t == 0)
				{
					Model.add(EV::eSlev[n][t][r] ==
						Estorage[r].eff_ch * EV::eSch[n][t][r] -
						(EV::eSdis[n][t][r] / Estorage[r].eff_disCh));
				}
				else
				{
					Model.add(EV::eSlev[n][t][r] == EV::eSlev[n][t - 1][r] +
						Estorage[r].eff_ch * EV::eSch[n][t][r] -
						(EV::eSdis[n][t][r] / Estorage[r].eff_disCh));
				}
				Model.add(EV::YeCD[n][r] >= EV::eSdis[n][t][r]);
				Model.add(EV::YeCD[n][r] >= EV::EV::eSch[n][t][r]);

				Model.add(EV::YeLev[n][r] >= EV::eSlev[n][t][r]);
			}
		}
	}

	// C17: if an storage established or not
	for (int n = 0; n < nEnode; n++)
	{
		for (int r = 0; r < neSt; r++)
		{
			Model.add(EV::YeLev[n][r] <= 10e10 * EV::YeStr[n][r]);
		}
	}
#pragma endregion

}



