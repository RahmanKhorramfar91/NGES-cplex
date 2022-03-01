#include"Models_Funcs.h"
#include "ilcplex/ilocplex.h";
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables


void Electricy_Network_Model(bool int_vars_relaxed, bool PrintVars)
{
	auto start = chrono::high_resolution_clock::now();
	//auto start = chrono::high_resolution_clock::now();

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
	double Elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	//std::cout << "Emission tonngage for 2050: " << cplex.getValue(EV::Emit_var) << endl;
#pragma endregion


	if (PrintVars)
	{
		Print_EV(cplex, obj_val, gap, Elapsed);
	}
}


void NG_Network_Model(bool PrintVars)
{
	auto start = chrono::high_resolution_clock::now();

	IloEnv env;
	IloModel Model(env);
	IloExpr exp_NGobj(env);

	Populate_GV(Model, env);
	NG_Model(Model, env, exp_NGobj);
	Model.add(IloMinimize(env, exp_NGobj));


#pragma region Solve the model
	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, 7200);
	cplex.setParam(IloCplex::EpGap, 0.001); // 0.1%
	//cplex.exportModel("MA_LP.lp");

	//cplex.setOut(env.getNullStream());

	//cplex.setParam(IloCplex::Param::MIP::Strategy::Branch, 1); //Up/down branch selected first (1,-1),  default:automatic (0)
	//cplex.setParam(IloCplex::Param::MIP::Strategy::BBInterval, 7);// 0 to 7
	//cplex.setParam(IloCplex::Param::MIP::Strategy::NodeSelect, 2);
	//https://www.ibm.com/support/knowledgecenter/SSSA5P_12.9.0/ilog.odms.cplex.help/CPLEX/UsrMan/topics/discr_optim/mip/performance/20_node_seln.html
	//cplex.setParam(IloCplex::Param::MIP::Strategy::VariableSelect, 4);//-1 to 4
	//cplex.setParam(IloCplex::Param::RootAlgorithm, 4); /000 to 6
	if (!cplex.solve()) {
		env.error() << "Failed to optimize!!!" << endl;
		std::cout << cplex.getStatus();
		throw(-1);
	}

	float obj_val = cplex.getObjValue();
	float gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;

#pragma endregion

	if (PrintVars)
	{
		Print_GV(cplex, obj_val, gap, Elapsed);
	}

}
