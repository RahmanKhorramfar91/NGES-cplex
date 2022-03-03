#include"Models_Funcs.h"

void Print_EV(IloCplex cplex, double obj, double gap, double Elapsed_time)
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
#pragma endregion



#pragma region print Electricity Network variables
	int periods2print = 20;
	ofstream fid;
	if (Setting::DESP_active)
	{
		fid.open("DESP.txt");
	}
	else
	{
		fid.open("DVe.txt");
	}

	fid << "Elapsed time: " << Elapsed_time << endl;
	fid << "\t Total cost for both networks:" << obj << endl;
	fid << "\t Electricity Network Obj Value:" << cplex.getValue(EV::e_system_cost) << endl;
	fid << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	fid << "\n \t Establishment Cost: " << cplex.getValue(EV::est_cost);
	fid << "\n \t Decommissioning Cost: " << cplex.getValue(EV::decom_cost);
	fid << "\n \t Fixed Cost: " << cplex.getValue(EV::fixed_cost);
	fid << "\n \t Variable Cost: " << cplex.getValue(EV::var_cost);
	//fid << "\n \t Emission Cost: " << cplex.getValue(emis_cost);
	fid << "\n \t (dfo, coal, and nuclear) Fuel Cost: " << cplex.getValue(EV::thermal_fuel_cost);
	fid << "\n \t Load Shedding Cost: " << cplex.getValue(EV::shedding_cost);
	fid << "\n \t Storage Cost: " << cplex.getValue(EV::elec_storage_cost) << "\n";
	fid << "\t E Emission: " << cplex.getValue(CV::E_emis) << "\n\n";

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
			XestS[n][i] = cplex.getValue(EV::Xest[n][i]);
			XdecS[n][i] = cplex.getValue(EV::Xdec[n][i]);
			Xs[n][i] = cplex.getValue(EV::Xop[n][i]);
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
				fid << "Xop[" << n << "][" << i << "] = " << Xs[n][i] << endl;
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

			fs[b][t] = cplex.getValue(EV::flowE[b][t]);
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
		Zs[b] = cplex.getValue(EV::Ze[b]);
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
				prodS[n][t][i] = cplex.getValue(EV::prod[n][t][i]);
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
			curtS[n][t] = cplex.getValue(EV::curtE[n][t]);
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
			YeCDs[n][r] = cplex.getValue(EV::YeCD[n][r]);
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
			YeLevs[n][r] = cplex.getValue(EV::YeLev[n][r]);
			if (YeLevs[n][r] > 10e-3)
			{
				fid << "Ye_lev[" << n << "][" << r << "] = " << YeLevs[n][r] << endl;
			}
		}
	}

	float** YeStr = new float* [nEnode];
	for (int n = 0; n < nEnode; n++)
	{
		YeStr[n] = new float[neSt]();
		for (int r = 0; r < neSt; r++)
		{
			YeStr[n][r] = cplex.getValue(EV::YeStr[n][r]);
			if (YeStr[n][r] > 10e-3)
			{
				fid << "Ye_Str[" << n << "][" << r << "] = " << YeStr[n][r] << endl;
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
				eSchS[n][t][i] = cplex.getValue(EV::eSch[n][t][i]);
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
				eSdisS[n][t][i] = cplex.getValue(EV::eSdis[n][t][i]);
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
				eSlevS[n][t][i] = cplex.getValue(EV::eSlev[n][t][i]);
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
}



void Print_GV(IloCplex cplex, double obj, double gap, double Elapsed_time)
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
	int nSVL = Exist_SVL.size();
	vector<SVL> SVLs = Params::SVLs;
	vector<branch> Branches = Params::Branches;
	int nEnode = Enodes.size();
	int nPlt = Plants.size();
	int nBr = Branches.size();
	int neSt = Estorage.size();
	int nPipe = PipeLines.size();
	vector<int> Tg = Params::Tg;
	//vector<int> Te = Params::Te;
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
#pragma endregion

#pragma region print NG network variables
	ofstream fid2;
	if (Setting::DGSP_active)
	{
		fid2.open("DGSP.txt");
	}
	else
	{
		fid2.open("DVg.txt");
	}

	fid2 << "Elapsed time: " << Elapsed_time << endl;
	fid2 << "\t Total cost for both networks:" << obj << endl;
	fid2 << "\t NG Network Obj Value:" << cplex.getValue(GV::NG_system_cost) << endl;
	fid2 << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	fid2 << "\t NG import Cost: " << cplex.getValue(GV::NG_import_cost) << endl;
	fid2 << "\t Pipeline Establishment Cost: " << cplex.getValue(GV::pipe_cost) << endl;
	fid2 << "\t Storatge Investment Cost: " << cplex.getValue(GV::strInv_cost) << endl;
	fid2 << "\t NG Storage Cost: " << cplex.getValue(GV::gStrFOM_cost) << endl;
	fid2 << "\t NG Load Shedding Cost: " << cplex.getValue(GV::gShedd_cost) << endl;
	fid2 << "\t NG Emission: " << cplex.getValue(CV::NG_emis) << endl;
	fid2 << endl;
	//float** supS = new float* [nGnode];
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			float s1 = cplex.getValue(GV::supply[k][tau]);
			if (s1 > 0)
			{
				fid2 << "supply[" << k << "][" << tau << "] = " << s1 << endl;
			}
		}
	}
	fid2 << endl;
	for (int k = 0; k < nGnode; k++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			float s1 = cplex.getValue(GV::curtG[k][tau]);
			if (s1 > 0)
			{
				fid2 << "CurtG[" << k << "][" << tau << "] = " << s1 << endl;
			}
		}
	}
	fid2 << endl;
	for (int i = 0; i < nPipe; i++)
	{
		float s1 = cplex.getValue(GV::Zg[i]);
		if (s1 > 0.01)
		{
			fid2 << "Zg[" << PipeLines[i].from_node << "][" << PipeLines[i].to_node << "] = " << s1 << endl;
		}
	}
	fid2 << endl;
	for (int i = 0; i < nPipe; i++)
	{
		for (int tau = 0; tau < Tg.size(); tau++)
		{
			float s1 = cplex.getValue(GV::flowGG[i][tau]);
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
				float s1 = cplex.getValue(GV::flowGE[k][kp][tau]);
				if (std::abs(s1) > 0.1)
				{
					fid2 << "flowGE[" << k << "][" << kp << "][" << tau << "] = " << s1 << endl;
				}
			}
		}
	}

	fid2 << endl;
	for (int k = 0; k < nGnode; k++)
	{
		for (int kp : Gnodes[k].adjS)
		{
			for (int tau = 0; tau < Tg.size(); tau++)
			{
				float s1 = cplex.getValue(GV::flowGL[k][kp][tau]);
				if (std::abs(s1) > 0.1)
				{
					fid2 << "flowGL[" << k << "][" << kp << "][" << tau << "] = " << s1 << endl;
				}
			}
		}
	}
	fid2 << endl;
	for (int k = 0; k < nGnode; k++)
	{
		for (int kp : Gnodes[k].adjS)
		{
			for (int tau = 0; tau < Tg.size(); tau++)
			{
				float s1 = cplex.getValue(GV::flowVG[kp][k][tau]);
				if (std::abs(s1) > 0.1)
				{
					fid2 << "flowVG[" << kp << "][" << k << "][" << tau << "] = " << s1 << endl;
				}
			}
		}
	}

	fid2 << endl;
	for (int j = 0; j < nSVL; j++)
	{
		float s1 = cplex.getValue(GV::Xstr[j]);
		if (s1 > 0.001)
		{
			fid2 << "Xstr[" << j << "]= " << s1 << endl;
		}
	}
	fid2 << endl;
	for (int j = 0; j < nSVL; j++)
	{
		float s1 = cplex.getValue(GV::Xvpr[j]);
		if (s1 > 0.001)
		{
			fid2 << "Xvpr[" << j << "]= " << s1 << endl;
		}
	}


	fid2 << endl;
	for (int j = 0; j < nSVL; j++)
	{
		for (int t = 0; t < Tg.size(); t++)
		{
			float s1 = cplex.getValue(GV::Sstr[j][t]);
			if (s1 > 0.001)
			{
				fid2 << "Sstr[" << j << "][" << t << "]= " << s1 << endl;
			}
		}
	}
	fid2 << endl;
	for (int j = 0; j < nSVL; j++)
	{
		for (int t = 0; t < Tg.size(); t++)
		{
			float s1 = cplex.getValue(GV::Svpr[j][t]);
			if (s1 > 0.001)
			{
				fid2 << "Svpr[" << j << "][" << t << "]= " << s1 << endl;
			}
		}
	}
	fid2 << endl;
	for (int j = 0; j < nSVL; j++)
	{
		for (int t = 0; t < Tg.size(); t++)
		{
			float s1 = cplex.getValue(GV::Sliq[j][t]);
			if (s1 > 0.001)
			{
				fid2 << "Sliq[" << j << "][" << t << "]= " << s1 << endl;
			}
		}
	}
	fid2.close();
#pragma endregion
}





