// NGES.cpp : This file contains the 'main' function. Program execution begins and ends there.
#include "ProblemData.h"
#include"Models_Funcs.h"
#pragma region Declaration of fields of "Setting" struct
bool Setting::print_E_vars;
bool Setting::print_NG_vars;
bool Setting::print_results_header;
bool Setting::relax_int_vars;
bool Setting::is_xi_given;
double Setting::xi_val;
bool Setting::xi_UB_obj;
bool Setting::xi_LB_obj;
double Setting::cplex_gap;
double Setting::CPU_limit;
int Setting::Num_rep_days;
double Setting::Emis_lim;
double Setting::RPS;
bool Setting::DGSP_active;
bool Setting::DESP_active;
bool Setting::Approach_1_active;
bool Setting::Approach_2_active;
double Setting::RNG_cap;
#pragma endregion


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

int main(int argc, char* argv[])
{

	auto start = chrono::high_resolution_clock::now();
	double total_ng_inj_per_year = 1.93E+09; //MMBtu
	double total_possible_emis_per_year = total_ng_inj_per_year * 0.05831;//=1.13E+08
	double total_ng_yearly_demand = 1.22E+09; // possible pollution: 7.09E+07

	if (argc > 1)
	{
		Setting::Num_rep_days = atoi(argv[1]);
		Setting::Approach_1_active = atoi(argv[2]);
		Setting::Approach_2_active = atoi(argv[3]);
		Setting::is_xi_given = atoi(argv[4]);
		Setting::xi_val = atof(argv[5]);
		Setting::Emis_lim = total_possible_emis_per_year*atof(argv[6]);
		Setting::RPS = atof(argv[7]);
		Setting::RNG_cap = atof(argv[8]);
		Setting::cplex_gap = atof(argv[9]);  // 1%
		Setting::CPU_limit = atoi(argv[10]);   // seconds
	}
	else
	{
		Setting::Num_rep_days = 7;   // 2, 7, 14, 52, 365
		Setting::Approach_1_active = true; // default = true
		Setting::Approach_2_active = false; // default = false
		Setting::is_xi_given = false;
		Setting::xi_val = 6.81119e+07;// 9.26E+07;
		Setting::Emis_lim = 2e7;    // tons
		Setting::RPS = 0.2;		    // out of 1 (=100%) Renewable Portfolio Share
		Setting::RNG_cap = 5e4;
		Setting::cplex_gap = 0.01;  // 1%
		Setting::CPU_limit = 3600;   // seconds
	}

#pragma region Problem Setting
	Setting::relax_int_vars = false; // int vars in electricity network
	Setting::print_results_header = true;
	Setting::xi_LB_obj = false; // (default = false) 
	Setting::xi_UB_obj = false;  // (default = false) 
#pragma endregion

#pragma region  Other parameters

	Params::Num_Rep_Days = Setting::Num_rep_days;
	double RNG_price = 20; // $$ per MMBtu
	double WACC = 0.05;// Weighted average cost of capital to calculate CAPEX coefficient from ATB2021
	int trans_unit_cost = 3500; // dollars per MW per mile of trans. line (ReEDS 2019)
	int trans_line_lifespan = 40; // years
	int decom_lifetime = 2035 - 2016;
	int battery_lifetime = 15; // 
	double NG_price = 4;//per MMBTu, approximated from NG price in eia.gov
	double dfo_pric = (1e6 / 1.37e5) * 3.5;//https://www.eia.gov/energyexplained/units-and-calculators/ and https://www.eia.gov/petroleum/gasdiesel/
	double coal_price = 92 / 19.26; //https://www.eia.gov/coal/ and https://www.eia.gov/tools/faqs/faq.php?id=72&t=2#:~:text=In%202020%2C%20the%20annual%20average,million%20Btu%20per%20short%20ton.
	double Nuclear_price = 0.69; // per MMBtu
	double E_curt_cost = 3e4; // $ per MWh;
	double G_curt_cost = 3e4; // & per MMBtu
	double pipe_per_mile = 7e+5;//https://www.gem.wiki/Oil_and_Gas_Pipeline_Construction_Costs
	int SVL_lifetime = 40; //https://www.hydrogen.energy.gov/pdfs/19001_hydrogen_liquefaction_costs.pdf
	int pipe_lifespan = 50; // years, https://www.popsci.com/story/environment/oil-gas-pipelines-property/#:~:text=There%20are%20some%203%20million,%2C%20power%20plants%2C%20and%20homes.&text=Those%20pipelines%20have%20an%20average%20lifespan%20of%2050%20years.
	double Ng_demand_growth_by_2050 = 0.5; // 50% https://www.eia.gov/todayinenergy/detail.php?id=42342
	double NG_emis_rate = 0.05831;  // tons of CO2 per MMBtu
#pragma endregion

#pragma region Read Data
	Read_Data();
#pragma endregion

#pragma region Set Params
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
	Params::SVL_lifetime = SVL_lifetime;
	Params::battery_lifetime = battery_lifetime;
	Params::RNG_cap = Setting::RNG_cap;
	Params::RNG_price = RNG_price;
	//Params::RPS = RPS;
	Params::NG_emis_rate = NG_emis_rate;
#pragma endregion


	int** Xs = new int* [Params::Enodes.size()];
	double*** Ps = new double** [Params::Enodes.size()];
	int** XestS = new int* [Params::Enodes.size()];
	int** XdecS = new int* [Params::Enodes.size()];


	double opt = 0; double UB1 = 0; double UB2 = 0;
	double MidSol = 0;
	// get upper and lower bound for xi by the first approach (Integrate Model)

	double xiLB1 = 0; double xiUB1 = 0;
	if (Setting::Approach_1_active)
	{
		bool ap2 = Setting::Approach_2_active;
		Setting::Approach_2_active = false;
		double total_cost = NGES_Model();
		auto end = chrono::high_resolution_clock::now();
		double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
		Print_Results(Elapsed);
		if (Setting::xi_LB_obj) { xiLB1 = NGES_Model(); }
		if (Setting::xi_UB_obj) { xiUB1 = NGES_Model(); }
		start = chrono::high_resolution_clock::now();
		Setting::Approach_2_active = ap2;
	}
	double desp_obj = 0;
	double dgsp_obj = 0; double xiLB2 = 0; double xiUB2 = 0;
	if (Setting::Approach_2_active)
	{
		Setting::Approach_1_active = false;
		desp_obj = DESP();
		dgsp_obj = DGSP();
		auto end = chrono::high_resolution_clock::now();
		double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
		Print_Results(Elapsed);
		if (Setting::xi_LB_obj) { xiLB2 = Xi_LB2(); }
		if (Setting::xi_UB_obj)
		{
			double aux_obj = AUX_UB();
			xiUB2 = Xi_UB2(aux_obj);
		}
	}


	std::cout << "xi LB1: " << xiLB1;
	std::cout << "\t xi LB2: " << xiLB2 << endl;
	std::cout << "xi UB1: " << xiUB1;
	std::cout << "\t xi UB2: " << xiUB2 << endl;


	//Electricy_Network_Model(int_vars_relaxed, print_vars);
	//NG_Network_Model(true);
	//NG_Network_Model(true);

	//FullModel(int_vars_relaxed, false);
	//LB = LB_no_flow_lim_no_ramp(true, true, Xs, XestS, XdecS, Ps);// by relaxing limit constraints on transmission flow

	//UB1 = UB_prods_fixed(false, Ps);
	//MidSol = inv_vars_fixed_flow_equation_relaxed(false, Xs, XestS, XdecS, Ps);
	//UB1 = UB_X_prod_vars_given(false,  Xs, XestS,XdecS,  Ps);
	//UB2 = UB_X_var_given(true, Xs, XestS, XdecS);
	//UB = UB_feasSol_X_Xest_Xdec_given(Xs, XestS, XdecS);
	//double gap1 = 100 * (UB2 - LB) / LB;
	//cout << "Gap ((UB-LB)/LB) (%)" << gap1 << endl;
	// system("pause");

}
