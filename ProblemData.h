#pragma once
#include <iostream>
#include <vector> 
#include<random>
#include <string>
#include <fstream> // to read and write from/on files
#include <cstdio>  // for puts and printf
#include <cstdlib> // for string converstion, rand srand, dynamic memory management (C++ header)
#include <iterator>
#include<sstream>
#include<chrono> // for high resolution timing (elapsed time in microseconds and so on)
#include<array>
#include<list>
#include<cmath> // power operator and others
#include<numeric>
#include<map>  // To define dictionary data types

using namespace std;

struct enode
{
	int num;
	int fips;
	int fips_order;
	vector<int> adj_buses;
	//vector<float> adj_dist;
	vector<string> Init_plt_types;
	vector<int> Init_plt_ind;
	vector<float*> Init_plt_prod_lim; // [Pmin,Pmax]
	vector<int> Init_plt_count;

	enode(int n, int f, int fo)
	{
		this->num = n;
		this->fips = f;
		this->fips_order = fo;
	}

	static vector<enode> read_bus_data(string FileName);
	static void read_adj_data(string FileName, vector<enode>& Enodes);
	static void read_exist_plt_data(string FileName, vector<enode>& Enodes);
};

struct Fips
{
	int fips_num;
	int bus_count;

	vector<float> demE;
	Fips(int f, int bc)
	{
		this->fips_num = f;
		this->bus_count = bc;
	}

	static vector<Fips> read_fips_data(string FileName);
	static void read_demand_Data(string FileName, vector<Fips>& all_FIPS);
};

struct plant
{
	string fuel_type;
	int num;  //0:'ng',1:'dfo',2:'solar',3:'wind',4:'wind_offshore', 5:'hydro',6:'coal',7:'nuclear'
	int Umax = 10; // maximum number of plants in a Enode
	int capex; // dollar per MW
	int fix_cost;//dollar per MW per year
	int var_cost;//dollar per MWh
	float heat_rate;//MMBTU/MWh
	float emis_rate;//ton/MMBTU
	int decom_cost;
	int emis_cost; //$/ton
	int lifespan; // year
	// revisit these paramters laters
	float Pmin;
	float Pmax;  // MW
	float rampU;
	float rampD;

	vector<float> prod_profile;

	plant(string t, int n, int cap, float pmax, float pmin, float ru, float rd, int f, int v, float h, float emi, int dec, int emic, int ls)
	{
		this->fuel_type = t;
		this->num = n;
		this->capex = cap;
		this->Pmax = pmax;
		this->Pmin = pmin;
		this->rampU = ru;
		this->rampD = rd;
		this->fix_cost = f;
		this->var_cost = v;
		this->heat_rate = h;
		this->emis_rate = emi;
		this->decom_cost = dec;
		this->emis_cost = emic;
		this->lifespan = ls;
	}
	static vector<plant> read_new_plant_data(string FileName);
	static void read_VRE_profile(string FileName1, string FileName2, string FileName3, vector<plant>& Plants);
};

struct branch
{
	int from_bus;
	int to_bus;
	float suscep;
	float maxFlow;
	float length;
	float cost;
	int is_exist;
	branch(int f, int t, float l, float c, int ie, float m, float s)
	{
		this->from_bus = f;
		this->to_bus = t;
		this->length = l;
		this->cost = c;
		this->is_exist = ie;
		this->maxFlow = m;
		this->suscep = s;
	}
	static vector<branch> read_branch_data(int nBus, string FN,
		string FN0, string FN1, string FN2, string FN3, string FN4,
		map<int, vector<int>>& Lnm);

};

struct gnode
{
	int num;
	int fips;
	vector<float> demG;
	float injU;
	float injL = 0.0;
	vector<int> adjG;
	vector<int> adjE;
	gnode(int n, int f, int inj)
	{
		this->num = n;
		this->fips = f;
		this->injU = inj;
	}

	static vector<gnode> read_gnode_data(string FileName);
	static void read_adjG_data(string FileName, vector<gnode>& Gnodes);
	static void read_adjE_data(string FileName, vector<gnode>& Gnodes);
	static void read_ng_demand_data(string FileName, vector<gnode>& Gnodes);
};

struct pipe
{
	int from_node;
	int to_node;
	int is_exist;
	float length;
	float cap;

	pipe(int f, int t, int exi, float le, float ca)
	{
		this->from_node = f;
		this->to_node = t;
		this->is_exist = exi;
		this->length = le;
		this->cap = ca;
	}
	static vector<pipe> read_pipe_data(string FileName);
};


struct Params
{
	// General
	static float WACC;
	static int trans_unit_cost;
	static int trans_line_lifespan;
	static float NG_price;

	static float dfo_pric;
	static float coal_price;
	static float E_curt_cost;
	static float G_curt_cost;
	static float pipe_per_mile;
	static int pipe_lifespan;


	// natural gas data
	static int Tg;
	static vector<gnode> Gnodes;
	static vector<pipe> PipeLines;


	// electricity data
	static map<int, vector<int>> Lnm;
	static int Te;
	static vector<enode> Enodes;
	static vector<Fips> all_FIPS;
	static vector<plant> Plants;
	static vector<branch> Branches;
};