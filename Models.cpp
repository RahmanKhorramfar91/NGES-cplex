#include"Models_Funcs.h"
#include "ilcplex/ilocplex.h"
typedef IloArray<IloNumVarArray> NumVar2D; // to define 2-D decision variables
typedef IloArray<NumVar2D> NumVar3D;  // to define 3-D decision variables
typedef IloArray<NumVar3D> NumVar4D;  // to define 4-D decision variables


void Electricy_Network_Model()
{
	auto start = chrono::high_resolution_clock::now();
	//auto start = chrono::high_resolution_clock::now();

	IloEnv env;
	IloModel Model(env);
	IloExpr exp_Eobj(env);
	Populate_EV(Model, env);
	Elec_Model(Model, env, exp_Eobj);
	Model.add(IloMinimize(env, exp_Eobj));
#pragma region Solve the model

	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, Setting::CPU_limit);
	cplex.setParam(IloCplex::EpGap, Setting::cplex_gap);
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

	double obj_val = cplex.getObjValue();
	double gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	//std::cout << "Emission tonngage for 2050: " << cplex.getValue(EV::Emit_var) << endl;
#pragma endregion


	if (Setting::print_E_vars)
	{
		Get_EV_vals(cplex, obj_val, gap, Elapsed);
	}
}


void NG_Network_Model()
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
	cplex.setParam(IloCplex::TiLim, Setting::CPU_limit);
	cplex.setParam(IloCplex::EpGap, Setting::cplex_gap);
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

	double obj_val = cplex.getObjValue();
	double gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;

#pragma endregion

	if (Setting::print_NG_vars)
	{
		Get_GV_vals(cplex, obj_val, gap, Elapsed);
	}

}


double NGES_Model()
{
	auto start = chrono::high_resolution_clock::now();

	IloEnv env;
	IloModel Model(env);
	IloExpr exp_NGobj(env);
	IloExpr exp_Eobj(env);
	Populate_EV(Model, env);
	Elec_Model(Model, env, exp_Eobj);

	Populate_GV(Model, env);
	NG_Model(Model, env, exp_NGobj);

	// coupling constraints
	IloExpr ex_xi(env);
	IloExpr ex_NG_emis(env);
	IloExpr ex_E_emis(env);
	Coupling_Constraints(Model, env, ex_xi, ex_NG_emis, ex_E_emis);
	if (Setting::is_xi_given)
	{
		cout << "\n\n if xi given" << endl;
		Model.add(ex_xi == Setting::xi_val);
		Model.add(CV::xi == ex_xi);
	}
	else
	{
		cout << "else xi given" << endl;
		Model.add(CV::xi == ex_xi);
	}
	Model.add(ex_E_emis + ex_NG_emis <= Setting::Emis_lim);
	Model.add(ex_E_emis == CV::E_emis);
	Model.add(ex_NG_emis == CV::NG_emis);

	if (Setting::xi_LB_obj)
	{
		Model.add(IloMinimize(env, CV::xi));
	}
	else if (Setting::xi_UB_obj)
	{
		Model.add(IloMaximize(env, CV::xi));
	}
	else
	{
		Model.add(IloMinimize(env, exp_Eobj + exp_NGobj));
	}




#pragma region Solve the model
	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, Setting::CPU_limit);
	cplex.setParam(IloCplex::EpGap, Setting::cplex_gap); // 0.1%	
	if (!cplex.solve()) {
		cout << "Failed to optimize!!!" << endl;
		std::cout << cplex.getStatus();
		throw(-1);
	}

	double obj_val = cplex.getObjValue();
	double gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	std::cout << "\t xi value = " << cplex.getValue(CV::xi) << endl;
	std::cout << "\t NG emission = " << cplex.getValue(CV::NG_emis) << endl;
	std::cout << "\t E emission = " << cplex.getValue(CV::E_emis) << endl;
#pragma endregion


	Get_GV_vals(cplex, obj_val, gap, Elapsed);
	Get_EV_vals(cplex, obj_val, gap, Elapsed);




	Print_Results(Elapsed);
	return obj_val;

}

double DESP()
{
	auto start = chrono::high_resolution_clock::now();

	IloEnv env;
	IloModel Model(env);
	IloExpr exp_Eobj(env);
	Populate_EV(Model, env); Populate_GV(Model, env);
	Elec_Model(Model, env, exp_Eobj);


	// coupling constraints
	IloExpr ex_xi(env); IloExpr ex_NG_emis(env); IloExpr ex_E_emis(env);
	Coupling_Constraints(Model, env, ex_xi, ex_NG_emis, ex_E_emis);

	// Coupling 1
	Model.add(CV::xi == ex_xi);
	Model.add(ex_xi == Setting::xi_val);

	// Coupling 2
	Model.add(ex_E_emis <= Setting::Emis_lim);
	//Model.add(ex_NG_emis <= 2.6e6);
	Model.add(ex_E_emis == CV::E_emis);
	Model.add(CV::NG_emis);


	Model.add(IloMinimize(env, exp_Eobj));


#pragma region Solve the model

	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, Setting::CPU_limit);
	cplex.setParam(IloCplex::EpGap, Setting::cplex_gap);
	if (!cplex.solve()) {
		env.error() << "Failed to optimize DESP!!!" << endl;
		std::cout << cplex.getStatus();
		throw(-1);
	}

	double obj_val = cplex.getObjValue();
	double gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t DESP Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	//std::cout << "Emission tonngage for 2050: " << cplex.getValue(EV::Emit_var) << endl;
#pragma endregion
	CV::used_emis_cap = cplex.getValue(CV::E_emis);

	Get_EV_vals(cplex, obj_val, gap, Elapsed);

	return obj_val;
}

double DGSP()
{
	auto start = chrono::high_resolution_clock::now();

	IloEnv env;
	IloModel Model(env);
	IloExpr exp_NGobj(env);
	Populate_GV(Model, env);
	Populate_EV(Model, env);
	NG_Model(Model, env, exp_NGobj);
	Setting::Approach_2_active = true;
	Setting::DGSP_active = true; Setting::DESP_active = false;
	// coupling constraints
	IloExpr ex_xi(env); IloExpr ex_NG_emis(env); IloExpr ex_E_emis(env);
	Coupling_Constraints(Model, env, ex_xi, ex_NG_emis, ex_E_emis);

	// Coupling 1: xi must be given
	Model.add(CV::xi == ex_xi);
	Model.add(ex_xi == Setting::xi_val);


	// Coupling 2
	double rhs = Setting::Emis_lim - CV::used_emis_cap;
	rhs = std::max(rhs, 0 + 0.001);
	Model.add(ex_NG_emis <= rhs);
	Model.add(CV::E_emis);
	Model.add(ex_NG_emis == CV::NG_emis);

	Model.add(IloMinimize(env, exp_NGobj));

#pragma region Solve the model

	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, Setting::CPU_limit);
	cplex.setParam(IloCplex::EpGap, Setting::cplex_gap); // 0.1%
	//cplex.exportModel("MA_LP.lp");

	//cplex.setOut(env.getNullStream());

	//cplex.setParam(IloCplex::Param::MIP::Strategy::Branch, 1); //Up/down branch selected first (1,-1),  default:automatic (0)
	//cplex.setParam(IloCplex::Param::MIP::Strategy::BBInterval, 7);// 0 to 7
	//cplex.setParam(IloCplex::Param::MIP::Strategy::NodeSelect, 2);
	//https://www.ibm.com/support/knowledgecenter/SSSA5P_12.9.0/ilog.odms.cplex.help/CPLEX/UsrMan/topics/discr_optim/mip/performance/20_node_seln.html
	//cplex.setParam(IloCplex::Param::MIP::Strategy::VariableSelect, 4);//-1 to 4
	//cplex.setParam(IloCplex::Param::RootAlgorithm, 4); /000 to 6
	if (!cplex.solve()) {
		env.error() << "Failed to Optimize DGSP!!!" << endl;
		std::cout << cplex.getStatus();
		throw(-1);
	}
	//CV::eta_val = cplex.getValue(CV::NG_emis);

	double obj_val = cplex.getObjValue();
	double gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = (int)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t DGSP Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	std::cout << "\t xi value = " << cplex.getValue(CV::xi) << endl;
	std::cout << "\t NG emission = " << cplex.getValue(CV::NG_emis) << endl;
	std::cout << "\t E emission = " << cplex.getValue(CV::E_emis) << endl;
#pragma endregion
	Get_GV_vals(cplex, obj_val, gap, Elapsed);
	return obj_val;
}


double Xi_LB2()
{
	auto start = chrono::high_resolution_clock::now();

	IloEnv env;
	IloModel Model(env);
	IloExpr exp_NGobj(env);
	IloExpr exp_Eobj(env);
	Populate_GV(Model, env);
	Populate_EV(Model, env);
	Elec_Model(Model, env, exp_Eobj);

	// coupling constraints
	IloExpr ex_xi(env); IloExpr ex_NG_emis(env); IloExpr ex_E_emis(env);
	Coupling_Constraints(Model, env, ex_xi, ex_NG_emis, ex_E_emis);
	// xi is a variable to be minimized
	Model.add(CV::xi == ex_xi);


	// Coupling 2
	Model.add(ex_E_emis <= Setting::Emis_lim);
	Model.add(ex_E_emis == CV::E_emis);
	Model.add(CV::NG_emis);


	Model.add(IloMinimize(env, CV::xi));


#pragma region Solve the model

	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, Setting::CPU_limit);
	cplex.setParam(IloCplex::EpGap, Setting::cplex_gap);

	if (!cplex.solve()) {
		env.error() << "Failed to optimize!!!" << endl;
		std::cout << cplex.getStatus();
		throw(-1);
	}

	double obj_val = cplex.getObjValue();
	double gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	//std::cout << "Emission tonngage for 2050: " << cplex.getValue(EV::Emit_var) << endl;
#pragma endregion


	return obj_val;
}


double AUX_UB()
{
	auto start = chrono::high_resolution_clock::now();

	IloEnv env;
	IloModel Model(env);
	IloExpr exp_NGobj(env);
	Populate_GV(Model, env);
	Populate_EV(Model, env);
	NG_Model(Model, env, exp_NGobj);
	Setting::Approach_2_active = true;
	Setting::DGSP_active = true; Setting::DESP_active = false;
	// coupling constraints
	IloExpr ex_xi(env); IloExpr ex_NG_emis(env); IloExpr ex_E_emis(env);
	Coupling_Constraints(Model, env, ex_xi, ex_NG_emis, ex_E_emis);
	Model.add(CV::xi == ex_xi);

	Model.add(ex_NG_emis <= Setting::Emis_lim);
	Model.add(ex_NG_emis == CV::NG_emis);
	Model.add(CV::E_emis);

	Model.add(IloMaximize(env, CV::xi));


#pragma region Solve the model
	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, Setting::CPU_limit);
	cplex.setParam(IloCplex::EpGap, Setting::cplex_gap);

	if (!cplex.solve()) {
		env.error() << "Failed to optimize AUX!!!" << endl;
		std::cout << cplex.getStatus();
		throw(-1);
	}

	double obj_val = cplex.getObjValue();
	double gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
#pragma endregion

	if (Setting::print_NG_vars)
	{
		Get_GV_vals(cplex, obj_val, gap, Elapsed);
	}
	return obj_val;
}


double Xi_UB2(double aux_obj)
{
	auto start = chrono::high_resolution_clock::now();

	IloEnv env;
	IloModel Model(env);
	IloExpr exp_NGobj(env);
	IloExpr exp_Eobj(env);
	Populate_GV(Model, env);
	Populate_EV(Model, env);
	Elec_Model(Model, env, exp_Eobj);

	// coupling constraints
	IloExpr ex_xi(env); IloExpr ex_NG_emis(env); IloExpr ex_E_emis(env);
	Coupling_Constraints(Model, env, ex_xi, ex_NG_emis, ex_E_emis);
	// xi is a variable to be minimized
	Model.add(CV::xi <= aux_obj);
	Model.add(CV::xi == ex_xi);


	// Coupling 2
	Model.add(ex_E_emis <= Setting::Emis_lim);
	Model.add(ex_E_emis == CV::E_emis);
	Model.add(CV::NG_emis);


	Model.add(IloMaximize(env, CV::xi));


#pragma region Solve the model

	IloCplex cplex(Model);
	cplex.setParam(IloCplex::TiLim, Setting::CPU_limit);
	cplex.setParam(IloCplex::EpGap, Setting::cplex_gap);

	if (!cplex.solve()) {
		env.error() << "Failed to optimize!!!" << endl;
		std::cout << cplex.getStatus();
		throw(-1);
	}

	double obj_val = cplex.getObjValue();
	double gap = cplex.getMIPRelativeGap();
	auto end = chrono::high_resolution_clock::now();
	double Elapsed = (double)chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000; // seconds
	std::cout << "Elapsed time: " << Elapsed << endl;
	std::cout << "\t Obj Value:" << obj_val << endl;
	std::cout << "\t Gap: " << gap << " Status:" << cplex.getStatus() << endl;
	//std::cout << "Emission tonngage for 2050: " << cplex.getValue(EV::Emit_var) << endl;
#pragma endregion


	return obj_val;



}