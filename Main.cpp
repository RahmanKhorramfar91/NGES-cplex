// NGES.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "ProblemData.h"
#include"Models_Funcs.h"

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
	int Num_rep_days = 2;
	Params::Num_Rep_Days = Num_rep_days;
	bool int_vars_relaxed = false;
	bool print_vars = true;
#pragma endregion

#pragma region  Other parameters
	// Uppper bound of Emission, 2018 emission for New England was 147 million metric tons(mmt)
// emission for 2 hours (out of 48 rep. hours) is 1.9e6, yearly 23 (mmt) in this model.
	float Emis_lim = 4e6;
	float RPS = 0.2;   // Renewable Portfolio Share
	float WACC = 0.05;// Weighted average cost of capital to calculate CAPEX coefficient from ATB2021
	int trans_unit_cost = 3500; // dollars per MW per mile of trans. line (ReEDS 2019)
	int trans_line_lifespan = 40; // years
	int decom_lifetime = 2035 - 2016;
	int battery_lifetime = 15; // 
	float NG_price = 4;//per MMBTu, approximated from NG price in eia.gov
	float dfo_pric = (1e6 / 1.37e5) * 3.5;//https://www.eia.gov/energyexplained/units-and-calculators/ and https://www.eia.gov/petroleum/gasdiesel/
	float coal_price = 92 / 19.26; //https://www.eia.gov/coal/ and https://www.eia.gov/tools/faqs/faq.php?id=72&t=2#:~:text=In%202020%2C%20the%20annual%20average,million%20Btu%20per%20short%20ton.
	float Nuclear_price = 0.69; // per MMBtu
	float E_curt_cost = 5e4; // $ per MWh;
	float G_curt_cost = 1e5; // & per MMBtu
	float pipe_per_mile = 7e+5;//https://www.gem.wiki/Oil_and_Gas_Pipeline_Construction_Costs
	int pipe_lifespan = 50; // years, https://www.popsci.com/story/environment/oil-gas-pipelines-property/#:~:text=There%20are%20some%203%20million,%2C%20power%20plants%2C%20and%20homes.&text=Those%20pipelines%20have%20an%20average%20lifespan%20of%2050%20years.
	float Ng_demand_growth_by_2050 = 0.5; // 50% https://www.eia.gov/todayinenergy/detail.php?id=42342
#pragma endregion

#pragma region Read Data
	// Read Natural Gas Data
	Read_Data();
#pragma endregion

#pragma region Populate Params
	Params::WACC = WACC;
	Params::trans_unit_cost = trans_unit_cost;
	Params::trans_line_lifespan = trans_line_lifespan;
	Params::NG_price = NG_price;
	Params::dfo_pric = dfo_pric;
	Params::coal_price = coal_price;
	Params::nuclear_price = Nuclear_price;
	Params::E_curt_cost = E_curt_cost;
	Params::G_curt_cost = G_curt_cost;
	Params::pipe_per_mile = pipe_per_mile;
	Params::pipe_lifespan = pipe_lifespan;
	Params::battery_lifetime = battery_lifetime;
	Params::Emis_lim = Emis_lim;
	Params::RPS = RPS;
#pragma endregion

	int** Xs = new int* [Params::Enodes.size()];
	double*** Ps = new double** [Params::Enodes.size()];
	int** XestS = new int* [Params::Enodes.size()];
	int** XdecS = new int* [Params::Enodes.size()];

	double LB = 0; double opt = 0; double UB1 = 0; double UB2 = 0;
	double MidSol = 0;
	Electricy_Network_Model(int_vars_relaxed, print_vars);
	//NG_Network_Model(true);
	//opt = NGES_Model(false, true);
	//FullModel(int_vars_relaxed, false);
	//LB = LB_no_flow_lim_no_ramp(true, true, Xs, XestS, XdecS, Ps);// by relaxing limit constraints on transmission flow

	//UB1 = UB_prods_fixed(false, Ps);
	//MidSol = inv_vars_fixed_flow_equation_relaxed(false, Xs, XestS, XdecS, Ps);
	//UB1 = UB_X_prod_vars_given(false,  Xs, XestS,XdecS,  Ps);
	//UB2 = UB_X_var_given(true, Xs, XestS, XdecS);
	//UB = UB_feasSol_X_Xest_Xdec_given(Xs, XestS, XdecS);
	double gap1 = 100 * (UB2 - LB) / LB;
	cout << "Gap ((UB-LB)/LB) (%)" << gap1 << endl;
	// system("pause");
	return 0;
}
