#include"Models_Funcs.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables


void Electricy_Network_Model(bool int_vars_relaxed, bool PrintVars)
{
	auto start = chrono::high_resolution_clock::now();
	//auto start = chrono::high_resolution_clock::now();
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

	IloEnv env;
	IloModel Model(env);

	Populate_EV(int_vars_relaxed, Model, env);
	Elec_Model(Model, env);
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
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	//std::cout << "Emission tonngage for 2050: " << cplex.getValue(EV::Emit_var) << endl;
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
		fid << "\n \t Establishment Cost: " << cplex.getValue(EV::est_cost);
		fid << "\n \t Decommissioning Cost: " << cplex.getValue(EV::decom_cost);
		fid << "\n \t Fixed Cost: " << cplex.getValue(EV::fixed_cost);
		fid << "\n \t Variable Cost: " << cplex.getValue(EV::var_cost);
		//fid << "\n \t Emission Cost: " << cplex.getValue(emis_cost);
		fid << "\n \t (dfo, coal, and nuclear) Fuel Cost: " << cplex.getValue(EV::thermal_fuel_cost);
		fid << "\n \t Load Shedding Cost: " << cplex.getValue(EV::shedding_cost);
		fid << "\n \t Storage Cost: " << cplex.getValue(EV::elec_storage_cost) << "\n\n";

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
}

