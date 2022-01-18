#pragma once
#include"ProblemData.h"

void Electricy_Network_Model(bool int_vars_relaxed, bool PrintVars);

void FullModel(bool int_vars_relaxed, bool PrintVars);
double LB_no_flow_lim(bool PrintVars, int** Xs, int** XestS, int** XdecS);
double UB_no_trans_consts(bool PrintVars, int** Xs, int** XestS, int** XdecS);
double UB_feasSol_X_var_given(int** Xs);
double UB_feasSol_X_Xest_Xdec_given(int** Xs, int** XestS, int** XdecS);