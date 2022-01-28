// NGES.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "ProblemData.h"
#include"Models.h"

//HOW TO SET UP CPLEX FOR C++  :
//https://bzdww.com/article/134619/
//https://www.youtube.com/watch?v=Hbn1pGWLeaA
//1) go to IBM directory in the program files in driver C
//2) Go to concert folder -> include -> "ilconcert" folder, copy the directory somewhere
//3) Goto cplex folder -> include and when you see the "ilcplex" folder, copy the directory somewhere
//4) In the solution Explorer tab, click on the project name and select properties
//5) Go to C/C++ general ->" additional include directories" -> paste the two directories:
//C:\Program Files\IBM\ILOG\CPLEX_Studio129\concert\include
//C:\Program Files\IBM\ILOG\CPLEX_Studio129\cplex\include
//
//6) Go to C/C++ general ->"Preprocessors" and add these words:
//WIN32
//_CONSOLE
//IL_STD
//_CRT_SECURE_NO_WARNINGS
//
//Or
//NDEBUG
//_CONSOLE
//IL_STD
//
//7)  In the Project1 property page, select: "c/c++" - "code generation" - "runtime library",
//set to "multithreaded DLL (/MD)". determine.
//
//8) In the Project1 property page, select: "Linker" - "Input" - "Additional Dependencies",
//and then enter the path of the following three files:
//C:\Program Files\IBM\ILOG\CPLEX_Studio129\cplex\lib\x64_windows_vs2017\stat_mda\cplex1290.lib
//C:\Program Files\IBM\ILOG\CPLEX_Studio129\cplex\lib\x64_windows_vs2017\stat_mda\ilocplex.lib
//C:\Program Files\IBM\ILOG\CPLEX_Studio129\concert\lib\x64_windows_vs2017\stat_mda\concert.lib
//
//9) if you're using visual studio 2017 with cplex 12.8, you may encounter an error which you then may
//follow this link: https://www-01.ibm.com/support/docview.wss?uid=ibm10718671

int main()
{
	// NOTE: prints for t<10 for prod[n][t][i]
	auto start = chrono::high_resolution_clock::now();
#pragma region Problem Setting
	const int nRepDays = 2;
	int PP = 24 * nRepDays;  // hours of planning for electricity network
	vector<int> Tg;  // days of planning
	vector<int> RepDays{ 243, 321 };
	vector<int> RepDaysCount{ 121, 244 };

	vector<int> Te;
	vector<int> time_weight;
	for (int i = 0; i < nRepDays; i++)
	{
		for (int j = 0; j < 24; j++)
		{
			Te.push_back(24 * RepDays[i] + j);
			time_weight.push_back(RepDaysCount[i]);
			if (Te.size() >= PP) { break; }
		}
	}
	for (int i = 0; i < nRepDays; i++)
	{
		Tg.push_back(RepDays[i]);
	}


	bool int_vars_relaxed = false;
	 
	// Uppper bound of Emission, 2018 emission for New England was 147 million metric tons(mmt)
	// emission for 2 hours (out of 48 rep. hours) is 1.9e6, yearly 23 (mmt) in this model.
	float Emis_lim = 4e6;
	float RPS = 0.2;   // Renewable Portfolio Share
	float WACC = 0.065;// Weighted average cost of capital to calculate CAPEX coefficient from ATB2021
	int trans_unit_cost = 1800; // dollars per mile of trans. line
#pragma endregion


#pragma region  parameters to be revised later
	int trans_line_lifespan = 10; // years
	float NG_price = 15;//per MMBTu, approximately: https://www.eia.gov/dnav/ng/hist/n3010us3M.htm
	float dfo_pric = (1e6 / 1.37e5) * 3.5;//https://www.eia.gov/energyexplained/units-and-calculators/ and https://www.eia.gov/petroleum/gasdiesel/
	float coal_price = 92 / 19.26; //https://www.eia.gov/coal/ and https://www.eia.gov/tools/faqs/faq.php?id=72&t=2#:~:text=In%202020%2C%20the%20annual%20average,million%20Btu%20per%20short%20ton.
	float E_curt_cost = 2e5; // $ per MW;
	float G_curt_cost = 1.5e6;
	float pipe_per_mile = 7e+5;//https://www.gem.wiki/Oil_and_Gas_Pipeline_Construction_Costs
	int pipe_lifespan = 50; // years, https://www.popsci.com/story/environment/oil-gas-pipelines-property/#:~:text=There%20are%20some%203%20million,%2C%20power%20plants%2C%20and%20homes.&text=Those%20pipelines%20have%20an%20average%20lifespan%20of%2050%20years.
	float Ng_demand_growth_by_2050 = 0.5; // 50% https://www.eia.gov/todayinenergy/detail.php?id=42342
#pragma endregion

#pragma region Read Data
	// Read Natural Gas Data
	std::map<int, vector<int>> Lg; //key: from_ng_node*200+to_ng_node, 200 is up_lim for number of buses
	vector<gnode> Gnodes = gnode::read_gnode_data("ng_nodes.txt");
	gnode::read_adjG_data("ng_adjG.txt", Gnodes);
	gnode::read_adjE_data("ng_adjE.txt", Gnodes);
	gnode::read_ng_demand_data("ng_daily_dem.txt", Gnodes);
	vector<pipe> PipeLines = pipe::read_pipe_data("g2g_br.txt",Lg);
	int nGnode = Gnodes.size();

	// Read Electricity Data
	// better to use std::unordered_map which is more efficient and faster
	std::map<int, vector<int>> Le; //key: from_bus*200+to_bus, 200 is up_lim for number of buses
	vector<eStore> Estorage = eStore::read_elec_storage_data("storage_elec.txt");
	vector<enode> Enodes = enode::read_bus_data("bus_num.txt");
	enode::read_adj_data("bus_adj_Nodes.txt", Enodes);
	enode::read_exist_plt_data("existing_plants.txt", Enodes);
	enode::read_demand_data("elec_dem_per_zone_per_hour.txt", Enodes);
	int nEnode = Enodes.size();

	vector<plant> Plants = plant::read_new_plant_data("plant_data.txt");
	plant::read_VRE_profile("profile_hydro_hourly.txt",
		"profile_wind_hourly.txt", "profile_solar_hourly.txt", Plants);

	vector<branch> Branches = branch::read_branch_data(nEnode, "b2b_br_per_node.txt", "b2b_br.txt", "b2b_br_dist.txt",
		"b2b_br_is_existing.txt", "b2b_br_maxFlow.txt", "b2b_br_Suscept.txt", Le);


#pragma endregion

#pragma region Populate Params
	Params::Branches = Branches;
	Params::Enodes = Enodes;
	Params::Gnodes = Gnodes;
	Params::PipeLines = PipeLines;
	Params::Plants = Plants;
	Params::Estorage = Estorage;
	Params::Tg = Tg;
	Params::Te = Te;
	Params::time_weight = time_weight;
	Params::RepDaysCount = RepDaysCount;
	Params::WACC = WACC;
	Params::trans_unit_cost = trans_unit_cost;
	Params::trans_line_lifespan = trans_line_lifespan;
	Params::NG_price = NG_price;
	Params::dfo_pric = dfo_pric;
	Params::coal_price = coal_price;
	Params::E_curt_cost = E_curt_cost;
	Params::G_curt_cost = G_curt_cost;
	Params::pipe_per_mile = pipe_per_mile;
	Params::pipe_lifespan = pipe_lifespan;
	Params::Le = Le;
	Params::Lg = Lg;
	Params::Emis_lim = Emis_lim;
	Params::RPS = RPS;
#pragma endregion

	int** Xs = new int* [Params::Enodes.size()];
	int** XestS = new int* [Params::Enodes.size()];
	int** XdecS = new int* [Params::Enodes.size()];

	double LB = 0; double UB = 0;
	//Electricy_Network_Model(false, true);
	//NG_Network_Model(true);
	NGES_Model(false, true);
	//FullModel(int_vars_relaxed, false);
	//LB = LB_no_flow_lim(true, Xs, XestS, XdecS);// by relaxing limit constraints on transmission flow
	//UB = UB_no_trans_consts(false, Xs, XestS, XdecS);
	//UB = UB_feasSol_X_Xest_Xdec_given(Xs, XestS, XdecS);
	//UB = UB_feasSol_X_var_given(Xs);
	// system("pause");
	return 0;
}
