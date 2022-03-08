#include"Models_Funcs.h"

void Get_EV_vals(IloCplex cplex, double obj, double gap, double Elapsed_time)
{

#pragma region Fetch Data
	vector<gnode> Gnodes = Params::Gnodes;
	vector<pipe> PipeLines = Params::PipeLines;
	vector<enode> Enodes = Params::Enodes;
	vector<plant> Plants = Params::Plants;
	vector<eStore> Estorage = Params::Estorage;
	vector<branch> Branches = Params::Branches;
	int nEnode = (int)Enodes.size();
	int nPlt = (int)Plants.size();
	int nBr = (int)Branches.size();
	int neSt = (int)Estorage.size();
	vector<int> Tg = Params::Tg;
	vector<int> Te = Params::Te;
	vector<int> time_weight = Params::time_weight;
#pragma endregion

#pragma region get Electricity Network variables
	//int periods2print = 20;
	/*ofstream fid;
	if (Setting::DESP_active)
	{
		fid.open("DESP.txt");
	}
	else
	{
		fid.open("DVe.txt");
	}*/
	EV::val_est_cost = cplex.getValue(EV::est_cost);
	EV::val_decom_cost = cplex.getValue(EV::decom_cost);
	EV::val_fixed_cost = cplex.getValue(EV::fixed_cost);
	EV::val_var_cost = cplex.getValue(EV::var_cost);
	EV::val_thermal_fuel_cost = cplex.getValue(EV::thermal_fuel_cost);
	EV::val_shedding_cost = cplex.getValue(EV::shedding_cost);
	EV::val_elec_storage_cost = cplex.getValue(EV::elec_storage_cost);
	EV::val_Emit_var = cplex.getValue(CV::E_emis);
	EV::val_e_system_cost = cplex.getValue(EV::e_system_cost);

	CV::val_E_emis = cplex.getValue(CV::E_emis);
	CV::val_xi = cplex.getValue(CV::xi);


	//fid << "Elapsed time: " << Elapsed_time << endl;
	//fid << "\t Total cost for both networks:" << obj << endl;
	//fid << "\t Electricity Network Obj Value:" << cplex.getValue(EV::e_system_cost) << endl;
	//fid << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	//fid << "\n \t Establishment Cost: " << cplex.getValue(EV::est_cost);
	//fid << "\n \t Decommissioning Cost: " << cplex.getValue(EV::decom_cost);
	//fid << "\n \t Fixed Cost: " << cplex.getValue(EV::fixed_cost);
	//fid << "\n \t Variable Cost: " << cplex.getValue(EV::var_cost);
	////fid << "\n \t Emission Cost: " << cplex.getValue(emis_cost);
	//fid << "\n \t (dfo, coal, and nuclear) Fuel Cost: " << cplex.getValue(EV::thermal_fuel_cost);
	//fid << "\n \t Load Shedding Cost: " << cplex.getValue(EV::shedding_cost);
	//fid << "\n \t Storage Cost: " << cplex.getValue(EV::elec_storage_cost) << "\n";
	//fid << "\t E Emission: " << cplex.getValue(CV::E_emis) << "\n\n";

	//double** XestS = new double* [nEnode];
	//double** XdecS = new double* [nEnode];
	//double** Xs = new double* [nEnode];
	EV::val_num_est = new double[nPlt]();
	EV::val_num_decom = new double[nPlt]();
	for (int n = 0; n < nEnode; n++)
	{
		//XestS[n] = new double[nPlt]();
		//XdecS[n] = new double[nPlt]();
		//Xs[n] = new double[nPlt]();
		for (int i = 0; i < nPlt; i++)
		{
			EV::val_num_est[i] += cplex.getValue(EV::Xest[n][i]);
			EV::val_num_decom[i] += cplex.getValue(EV::Xdec[n][i]);
			//XestS[n][i] = cplex.getValue(EV::Xest[n][i]);
			//XdecS[n][i] = cplex.getValue(EV::Xdec[n][i]);
			//Xs[n][i] = cplex.getValue(EV::Xop[n][i]);
			//if (XestS[n][i] > 0)
			//{
			//	//std::cout << "Xest[" << n << "][" << i << "] = " << XestS[n][i] << endl;
			//	fid << "Xest[" << n << "][" << i << "] = " << XestS[n][i] << endl;
			//}
			//if (XdecS[n][i] > 0)
			//{
			//	//std::cout << "Xdec[" << n << "][" << i << "] = " << XdecS[n][i] << endl;
			//	fid << "Xdec[" << n << "][" << i << "] = " << XdecS[n][i] << endl;
			//}
			//if (Xs[n][i] > 0)
			//{
			//	//cout << "X[" << n << "][" << i << "] = " << Xs[n][i] << endl;
			//	fid << "Xop[" << n << "][" << i << "] = " << Xs[n][i] << endl;
			//}
		}
	}

	double** fs = new double* [nBr];
	for (int b = 0; b < nBr; b++)
	{
		fs[b] = new double[Te.size()]();
		for (int t = 0; t < Te.size(); t++)
		{
			//if (t > periods2print) { break; }
			EV::val_total_flow += time_weight[t] * cplex.getValue(EV::flowE[b][t]);
			fs[b][t] = cplex.getValue(EV::flowE[b][t]);
			//if (std::abs(fs[b][t]) > 10e-3)
			//{
			//	//std::cout << "flowE[" << n << "][" << t << "][" << Enodes[n].adj_buses[m] << "] = " << fs[n][t][m] << endl;
			//	fid << "flowE[" << b << "][" << t << "] = " << fs[b][t] << endl;
			//}
		}
	}

	double* Zs = new double[nBr]();
	for (int b = 0; b < nBr; b++)
	{
		EV::val_num_est_trans += cplex.getValue(EV::Ze[b]);
		Zs[b] = cplex.getValue(EV::Ze[b]);

		//if (Zs[b] > 10e-3)
		//{
		//	//std::cout << "Z[" << n << "][" << Enodes[n].adj_buses[m] << "] = " << Zs[n][m] << endl;
		//	fid << "Z[" << b << "] = " << Zs[b] << endl;
		//}
	}

	//double*** prodS = new double** [nEnode];
	EV::val_total_prod = new double[nPlt]();
	for (int n = 0; n < nEnode; n++)
	{
		//prodS[n] = new double* [Te.size()];
		for (int t = 0; t < Te.size(); t++)
		{
			//if (t > periods2print) { break; }

			//prodS[n][t] = new double[nPlt]();
			for (int i = 0; i < nPlt; i++)
			{
				EV::val_total_prod[i] += time_weight[t] * cplex.getValue(EV::prod[n][t][i]);
				//prodS[n][t][i] = cplex.getValue(EV::prod[n][t][i]);
				//if (prodS[n][t][i] > 10e-3)
				//{
				//	//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
				//	fid << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
				//}
			}
		}
	}

	double** curtS = new double* [nEnode];
	for (int n = 0; n < nEnode; n++)
	{
		curtS[n] = new double[Te.size()]();
		for (int t = 0; t < Te.size(); t++)
		{
			EV::val_total_curt += time_weight[t] * cplex.getValue(EV::curtE[n][t]);
			//if (t > periods2print) { break; }
			//curtS[n][t] = cplex.getValue(EV::curtE[n][t]);
			//if (curtS[n][t] > 10e-3)
			//{
			//	//	std::cout << "curtE[" << n << "][" << t << "] = " << curtS[n][t] << endl;
			//	fid << "curtE[" << n << "][" << t << "] = " << curtS[n][t] << endl;
			//}
		}
	}

	// Storage variables
	double** YeCDs = new double* [nEnode];
	for (int n = 0; n < nEnode; n++)
	{
		YeCDs[n] = new double[neSt]();
		for (int r = 0; r < neSt; r++)
		{
			EV::val_storage_cap += cplex.getValue(EV::YeCD[n][r]);
			YeCDs[n][r] = cplex.getValue(EV::YeCD[n][r]);
			/*if (YeCDs[n][r] > 10e-3)
			{
				fid << "Ye_ch[" << n << "][" << r << "] = " << YeCDs[n][r] << endl;
			}*/
		}
	}
	double** YeLevs = new double* [nEnode];
	for (int n = 0; n < nEnode; n++)
	{
		YeLevs[n] = new double[neSt]();
		for (int r = 0; r < neSt; r++)
		{
			EV::val_storage_lev += cplex.getValue(EV::YeLev[n][r]);
			YeLevs[n][r] = cplex.getValue(EV::YeLev[n][r]);
			/*if (YeLevs[n][r] > 10e-3)
			{
				fid << "Ye_lev[" << n << "][" << r << "] = " << YeLevs[n][r] << endl;
			}*/
		}
	}

	//double** YeStr = new double* [nEnode];
	for (int n = 0; n < nEnode; n++)
	{
		//YeStr[n] = new double[neSt]();
		for (int r = 0; r < neSt; r++)
		{
			EV::val_num_storage += cplex.getValue(EV::YeStr[n][r]);
			//YeStr[n][r] = cplex.getValue(EV::YeStr[n][r]);
			/*if (YeStr[n][r] > 10e-3)
			{
				fid << "Ye_Str[" << n << "][" << r << "] = " << YeStr[n][r] << endl;
			}*/
		}
	}


	//double*** eSchS = new double** [nEnode];
	//for (int n = 0; n < nEnode; n++)
	//{
	//	eSchS[n] = new double* [Te.size()];
	//	for (int t = 0; t < Te.size(); t++)
	//	{
	//		if (t > periods2print) { break; }

	//		eSchS[n][t] = new double[nPlt]();
	//		for (int i = 0; i < neSt; i++)
	//		{
	//			eSchS[n][t][i] = cplex.getValue(EV::eSch[n][t][i]);
	//			if (eSchS[n][t][i] > 10e-3)
	//			{
	//				//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
	//				fid << "eS_ch[" << n << "][" << t << "][" << i << "] = " << eSchS[n][t][i] << endl;
	//			}
	//		}
	//	}
	//}
	//double*** eSdisS = new double** [nEnode];
	//for (int n = 0; n < nEnode; n++)
	//{
	//	eSdisS[n] = new double* [Te.size()];
	//	for (int t = 0; t < Te.size(); t++)
	//	{
	//		if (t > periods2print) { break; }

	//		eSdisS[n][t] = new double[nPlt]();
	//		for (int i = 0; i < neSt; i++)
	//		{
	//			eSdisS[n][t][i] = cplex.getValue(EV::eSdis[n][t][i]);
	//			if (eSdisS[n][t][i] > 10e-3)
	//			{
	//				//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
	//				fid << "eS_dis[" << n << "][" << t << "][" << i << "] = " << eSdisS[n][t][i] << endl;
	//			}
	//		}
	//	}
	//}

	//double*** eSlevS = new double** [nEnode];
	//for (int n = 0; n < nEnode; n++)
	//{
	//	eSlevS[n] = new double* [Te.size()];
	//	for (int t = 0; t < Te.size(); t++)
	//	{
	//		if (t > periods2print) { break; }

	//		eSlevS[n][t] = new double[nPlt]();
	//		for (int i = 0; i < neSt; i++)
	//		{
	//			eSlevS[n][t][i] = cplex.getValue(EV::eSlev[n][t][i]);
	//			if (eSlevS[n][t][i] > 10e-3)
	//			{
	//				//	std::cout << "prod[" << n << "][" << t << "][" << i << "] = " << prodS[n][t][i] << endl;
	//				fid << "eS_lev[" << n << "][" << t << "][" << i << "] = " << eSlevS[n][t][i] << endl;
	//			}
	//		}
	//	}
	//}
	//fid.close();
#pragma endregion
}


void Get_GV_vals(IloCplex cplex, double obj, double gap, double Elapsed_time)
{

#pragma region Fetch Data

	// set of possible existing plant types
//	std::map<string, int> sym2pltType = { {"ng",0},{"dfo", 1},
//{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7} };
//	std::map<int, string> pltType2sym = { {0,"ng"},{1,"dfo"},
//{2,"solar"},{3,"wind"},{4,"wind_offshore"},{5,"hydro"},{6,"coal"},{7,"nuclear"} };

	vector<gnode> Gnodes = Params::Gnodes;
	vector<pipe> PipeLines = Params::PipeLines;
	vector<enode> Enodes = Params::Enodes;
	vector<plant> Plants = Params::Plants;
	vector<eStore> Estorage = Params::Estorage;
	vector<exist_gSVL> Exist_SVL = Params::Exist_SVL;
	int nSVL = (int)Exist_SVL.size();
	vector<SVL> SVLs = Params::SVLs;
	vector<branch> Branches = Params::Branches;
	int nEnode = (int)Enodes.size();
	int nPlt = (int)Plants.size();
	int nBr = (int)Branches.size();
	int neSt = (int)Estorage.size();
	int nPipe = (int)PipeLines.size();
	vector<int> Tg = Params::Tg;
	//vector<int> Te = Params::Te;
	vector<int> time_weight = Params::time_weight;
	double pi = 3.1415;
	int nGnode = (int)Gnodes.size();
	double WACC = Params::WACC;
	int trans_unit_cost = Params::trans_unit_cost;
	int trans_line_lifespan = Params::trans_line_lifespan;
	double NG_price = Params::NG_price;
	double dfo_pric = Params::dfo_pric;
	double coal_price = Params::coal_price;
	double E_curt_cost = Params::E_curt_cost;
	double G_curt_cost = Params::G_curt_cost;
	double pipe_per_mile = Params::pipe_per_mile;
	int pipe_lifespan = Params::pipe_lifespan;
	map<int, vector<int>> Le = Params::Le;
	map<int, vector<int>> Lg = Params::Lg;
	vector<int> RepDaysCount = Params::RepDaysCount;
#pragma endregion

#pragma region print NG network variables
	/*ofstream fid2;
	if (Setting::DGSP_active)
	{
		fid2.open("DGSP.txt");
	}
	else
	{
		fid2.open("DVg.txt");
	}*/

	GV::val_NG_system_cost = cplex.getValue(GV::NG_system_cost);
	GV::val_pipe_cost = cplex.getValue(GV::pipe_cost);
	GV::val_NG_import_cost = cplex.getValue(GV::NG_import_cost);
	GV::val_strInv_cost = cplex.getValue(GV::strInv_cost);
	GV::val_gStrFOM_cost = cplex.getValue(GV::gStrFOM_cost);
	GV::val_ngShedd_cost = cplex.getValue(GV::gShedd_cost);
	GV::val_rngShedd_cost = cplex.getValue(GV::rngShedd_cost);

	CV::val_NG_emis = cplex.getValue(CV::NG_emis);


	/*fid2 << "Elapsed time: " << Elapsed_time << endl;
	fid2 << "\t Total cost for both networks:" << obj << endl;
	fid2 << "\t NG Network Obj Value:" << cplex.getValue(GV::NG_system_cost) << endl;
	fid2 << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	fid2 << "\t NG import Cost: " << cplex.getValue(GV::NG_import_cost) << endl;
	fid2 << "\t Pipeline Establishment Cost: " << cplex.getValue(GV::pipe_cost) << endl;
	fid2 << "\t Storatge Investment Cost: " << cplex.getValue(GV::strInv_cost) << endl;
	fid2 << "\t NG Storage Cost: " << cplex.getValue(GV::gStrFOM_cost) << endl;
	fid2 << "\t NG Load Shedding Cost: " << cplex.getValue(GV::gShedd_cost) << endl;
	fid2 << "\t NG Emission: " << cplex.getValue(CV::NG_emis) << endl;*/
	//fid2 << endl;
	//double** supS = new double* [nGnode];
	GV::val_supply = new double[nGnode]();
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			GV::val_supply[k] = RepDaysCount[k] * cplex.getValue(GV::supply[k][tau]);
			/*if (s1 > 0)
			{
				fid2 << "supply[" << k << "][" << tau << "] = " << s1 << endl;
			}*/
		}
	}
	//	fid2 << endl;
	//GV::val_g_curt = new double[nGnode]();
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			GV::val_ng_curt += RepDaysCount[k] * cplex.getValue(GV::curtG[k][tau]);
			GV::val_rng_curt += RepDaysCount[k] * cplex.getValue(GV::curtRNG[k][tau]);
			/*if (s1 > 0)
			{
				fid2 << "CurtG[" << k << "][" << tau << "] = " << s1 << endl;
			}*/
		}
	}
	//fid2 << endl;
	for (int i = 0; i < nPipe; i++)
	{
		GV::val_num_est_pipe += cplex.getValue(GV::Zg[i]);
		/*if (s1 > 0.01)
		{
			fid2 << "Zg[" << PipeLines[i].from_node << "][" << PipeLines[i].to_node << "] = " << s1 << endl;
		}*/
	}
	//fid2 << endl;
	for (int i = 0; i < nPipe; i++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			GV::val_total_flowGG += cplex.getValue(GV::flowGG[i][tau]);
			/*if (s1 > 0.001)
			{
				fid2 << "flowGG[" << PipeLines[i].from_node << "][" << PipeLines[i].to_node << "][" << tau << "]= " << s1 << endl;
			}*/
		}
	}
	//fid2 << endl;
	for (int k = 0; k < nGnode; k++)
	{
		for (int kp : Gnodes[k].adjE)
		{
			for (int tau = 0; tau < Tg.size(); tau++)
			{
				GV::val_total_flowGE += cplex.getValue(GV::flowGE[k][kp][tau]);
				/*if (std::abs(s1) > 0.1)
				{
					fid2 << "flowGE[" << k << "][" << kp << "][" << tau << "] = " << s1 << endl;
				}*/
			}
		}
	}

	//fid2 << endl;
	for (int k = 0; k < nGnode; k++)
	{
		for (int kp : Gnodes[k].adjS)
		{
			for (int tau = 0; tau < Tg.size(); tau++)
			{
				GV::val_total_flowGL += cplex.getValue(GV::flowGL[k][kp][tau]);
				/*if (std::abs(s1) > 0.1)
				{
					fid2 << "flowGL[" << k << "][" << kp << "][" << tau << "] = " << s1 << endl;
				}*/
			}
		}
	}
	//fid2 << endl;
	for (int k = 0; k < nGnode; k++)
	{
		for (int kp : Gnodes[k].adjS)
		{
			for (int tau = 0; tau < Tg.size(); tau++)
			{
				GV::val_total_flowVG += cplex.getValue(GV::flowVG[kp][k][tau]);
				/*if (std::abs(s1) > 0.1)
				{
					fid2 << "flowVG[" << kp << "][" << k << "][" << tau << "] = " << s1 << endl;
				}*/
			}
		}
	}

	//fid2 << endl;
	GV::val_storage = new double[nSVL]();
	for (int j = 0; j < nSVL; j++)
	{
		GV::val_storage[j] += cplex.getValue(GV::Xstr[j]);
		/*if (s1 > 0.001)
		{
			fid2 << "Xstr[" << j << "]= " << s1 << endl;
		}*/
	}
	//fid2 << endl;
	GV::val_vapor = new double[nSVL]();
	for (int j = 0; j < nSVL; j++)
	{
		GV::val_vapor[j] += cplex.getValue(GV::Xvpr[j]);
		/*if (s1 > 0.001)
		{
			fid2 << "Xvpr[" << j << "]= " << s1 << endl;
		}*/
	}


	//fid2 << endl;
	//for (int j = 0; j < nSVL; j++)
	//{
	//	for (int t = 0; t < Tg.size(); t++)
	//	{
	//		double s1 = cplex.getValue(GV::Sstr[j][t]);
	//		if (s1 > 0.001)
	//		{
	//			fid2 << "Sstr[" << j << "][" << t << "]= " << s1 << endl;
	//		}
	//	}
	//}
	//fid2 << endl;
	//for (int j = 0; j < nSVL; j++)
	//{
	//	for (int t = 0; t < Tg.size(); t++)
	//	{
	//		double s1 = cplex.getValue(GV::Svpr[j][t]);
	//		if (s1 > 0.001)
	//		{
	//			fid2 << "Svpr[" << j << "][" << t << "]= " << s1 << endl;
	//		}
	//	}
	//}
	//fid2 << endl;
	//for (int j = 0; j < nSVL; j++)
	//{
	//	for (int t = 0; t < Tg.size(); t++)
	//	{
	//		double s1 = cplex.getValue(GV::Sliq[j][t]);
	//		if (s1 > 0.001)
	//		{
	//			fid2 << "Sliq[" << j << "][" << t << "]= " << s1 << endl;
	//		}
	//	}
	//}
	//fid2.close();
#pragma endregion

}

void Print_Results(double Elapsed_time)
{
#pragma region Print in CSV file
	ofstream fid;
	string name = "NGES_Results.csv";
	fid.open(name, std::ios::app);
	if (Setting::print_results_header)
	{
		// problem setting
		fid << "\nNum_Rep_days" << ",";
		fid << "Approach 1" << ",";
		fid << "Approach 2" << ",";
		fid << "xi_given" << ",";
		fid << "xi_val" << ",";
		fid << "Emis_lim" << ",";
		fid << "RPS" << ",";
		fid << "RNG_cap" << ",";

		// electricity
		fid << "Total_E_Cost" << ",";
		fid << "Est_cost" << ",";
		fid << "Decom_cost" << ",";
		fid << "Fixed_cost" << ",";
		fid << "Variable_cost" << ",";
		fid << "Cola_dfo_fuel_cost" << ",";
		fid << "Shedding_cost" << ",";
		fid << "Storage_cost" << ",";
		fid << "Num_storage" << ",";
		fid << "Storage_level" << ",";
		fid << "Storage_cap" << ",";
		fid << "Total_curt" << ",";
		fid << "Num_new_trans" << ",";
		fid << "Total_flow" << ",";
		fid << "E_emis" << ",,";

		// natural gas
		fid << "Total_E_cost" << ",";
		fid << "Import_cost" << ",";
		fid << "Pipe_est_cost" << ",";
		fid << "Str_est_cost" << ",";
		fid << "Str_fix_cost" << ",";
		fid << "NG_shedding_cost" << ",";
		fid << "RNG_shedding_cost" << ",";
		fid << "Num_est_pipe" << ",";
		fid << "Total_NG_shedding" << ",";
		fid << "Total_RNG_shedding" << ",";
		fid << "flowGG" << ",";
		fid << "flowGE" << ",";
		fid << "flowGL" << ",";
		fid << "flowVG" << ",";
		fid << "NG_emis" << ",";
		// vector variables of both networks
		fid << ",,";
		fid << "num_est(nplt)" << ","; // vector of nplt
		fid << "num_decom(nplt)" << ","; // vector of nplt
		fid << "prod(nplt)" << ","; // vector of nplt

		fid << "supply(nGnode)" << ",";
		fid << "storage(nSLV)" << ",";
		fid << "vapor(nSLV)" << ",";


	}
	fid << "\n" << Setting::Num_rep_days << ",";
	fid << Setting::Approach_1_active << ",";
	fid << Setting::Approach_2_active << ",";
	fid << Setting::is_xi_given << ",";
	fid << CV::val_xi << ",";
	fid << Setting::Emis_lim << ",";
	fid << Setting::RPS << ",";
	fid << Setting::RNG_cap << ",";
	fid << EV::val_e_system_cost << ",";
	fid << EV::val_est_cost << ",";
	fid << EV::val_decom_cost << ",";
	fid << EV::val_fixed_cost << ",";
	fid << EV::val_var_cost << ",";
	fid << EV::val_thermal_fuel_cost << ",";
	fid << EV::val_shedding_cost << ",";
	fid << EV::val_elec_storage_cost << ",";
	fid << EV::val_num_storage << ",";
	fid << EV::val_storage_lev << ",";
	fid << EV::val_storage_cap << ",";
	fid << EV::val_total_curt << ",";
	fid << EV::val_num_est_trans << ",";
	fid << EV::val_total_flow << ",";
	fid << CV::val_E_emis << ",,";

	// NG
	fid << GV::val_NG_system_cost << ",";
	fid << GV::val_NG_import_cost << ",";
	fid << GV::val_pipe_cost << ",";
	fid << GV::val_strInv_cost << ",";
	fid << GV::val_gStrFOM_cost << ",";
	fid << GV::val_ngShedd_cost << ",";
	fid << GV::val_rngShedd_cost << ",";
	fid << GV::val_num_est_pipe << ",";
	fid << GV::val_ng_curt << ",";
	fid << GV::val_rng_curt << ",";
	fid << GV::val_total_flowGG << ",";
	fid << GV::val_total_flowGE << ",";
	fid << GV::val_total_flowGL << ",";
	fid << GV::val_total_flowVG << ",";
	fid << CV::val_NG_emis << ",";

	fid << ",";
	for (int i = 0; i < Params::Plants.size(); i++)
	{
		fid << "," << EV::val_num_est[i];
	}
	fid << ",";
	for (int i = 0; i < Params::Plants.size(); i++)
	{
		fid << "," << EV::val_num_decom[i];
	}
	fid << ",";
	for (int i = 0; i < Params::Plants.size(); i++)
	{
		fid << "," << EV::val_total_prod[i];
	}
	fid << ",";
	for (int j = 0; j < Params::Gnodes.size(); j++)
	{
		fid << "," << GV::supply[j];
	}
	fid << ",";
	for (int j = 0; j < Params::SVLs.size(); j++)
	{
		fid << "," << GV::val_storage[j];
	}
	fid << ",";
	for (int j = 0; j < Params::SVLs.size(); j++)
	{
		fid << "," << GV::val_vapor[j];
	}
	fid.close();
#pragma endregion

#pragma region Print in a text file
	ofstream fid2;
	fid2.open("NGES_Results.txt", std::ios::app);
	// problem setting
	fid2 << "\nNum_Rep_days: " << Setting::Num_rep_days;
	fid2 << "\t Approach 1: " << Setting::Approach_1_active;
	fid2 << "\tApproach 2: " << Setting::Approach_2_active;
	fid2 << "\txi_given: " << Setting::is_xi_given;
	fid2 << "\txi_val: " << Setting::xi_val;
	fid2 << "\tEmis_lim: " << Setting::Emis_lim;
	fid2 << "\tRPS: " << Setting::RPS;
	fid2 << "\tRNG_cap: " << Setting::RNG_cap;
	

	fid2 << "\n\t Elapsed time: " << Elapsed_time << endl;
	fid2 << "\t Total cost for both networks:" << EV::val_e_system_cost + GV::val_NG_system_cost << endl;
	fid2 << "\n \t Establishment Cost: " << EV::val_est_cost;
	fid2 << "\n \t Decommissioning Cost: " << EV::val_decom_cost;
	fid2 << "\n \t Fixed Cost: " << EV::val_fixed_cost;
	fid2 << "\n \t Variable Cost: " << EV::val_var_cost;
	fid2 << "\n \t (dfo, coal, and nuclear) Fuel Cost: " << EV::val_thermal_fuel_cost;
	fid2 << "\n \t Load Shedding Cost: " << EV::val_shedding_cost;
	fid2 << "\n \t Storage Cost: " << EV::val_elec_storage_cost;
	fid2 << "\t E Emission: " << CV::val_E_emis << "\n";


	fid2 << "\n\n\t NG Network Obj Value:" << GV::val_NG_system_cost << endl;
	fid2 << "\t NG import Cost: " << GV::val_NG_import_cost << endl;
	fid2 << "\t Pipeline Establishment Cost: " << GV::val_pipe_cost << endl;
	fid2 << "\t Storatge Investment Cost: " << GV::val_strInv_cost << endl;
	fid2 << "\t NG Storage Cost: " << GV::val_gStrFOM_cost << endl;
	fid2 << "\t NG Load NG Shedding Cost: " << GV::val_ngShedd_cost << endl;
	fid2 << "\t NG Load RNG Shedding Cost: " << GV::val_rngShedd_cost << endl;
	fid2 << "\t NG Emission: " << CV::val_NG_emis << endl;
	fid2 << "\n\n\n";
	fid2.close();
#pragma endregion

}




