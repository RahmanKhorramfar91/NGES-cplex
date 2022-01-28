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
	//int fips;
	//int fips_order;
	vector<int> adj_buses;
	//vector<float> adj_dist;
	vector<string> Init_plt_types;
	vector<int> Init_plt_ind;
	vector<float*> Init_plt_prod_lim; // [Pmin,Pmax]
	vector<int> Init_plt_count;
	vector<float> demand;
	enode(int n)
	{
		this->num = n;
	}

	static vector<enode> read_bus_data(string FileName);
	static void read_adj_data(string FileName, vector<enode>& Enodes);
	static void read_exist_plt_data(string FileName, vector<enode>& Enodes);
	static void read_demand_data(string FileName, vector<enode>& Enodes);
};

//struct Fips
//{
//	int fips_num;
//	int bus_count;
//
//	vector<float> demE;
//	Fips(int f, int bc)
//	{
//		this->fips_num = f;
//		this->bus_count = bc;
//	}
//
//	static vector<Fips> read_fips_data(string FileName);
//	static void read_demand_Data(string FileName, vector<Fips>& all_FIPS);
//};

struct plant
{
	string type;
	int num;  //0:'ng',1:'dfo',2:'solar',3:'wind',4:'wind_offshore', 5:'hydro',6:'coal',7:'nuclear'
	int is_exis; // 1=the plant existing type (PowerSim Data), 0= the plant is new type (ATB)
	int Umax = 500; // maximum number of plants in a Enode
	int capex; // dollar per MW
	int fix_cost;//dollar per count of type i per year
	int var_cost;//dollar per MWh
	float emis_rate;//ton/MMBTU
	float heat_rate;//MMBTU/MWh
	int lifetime; // year
	int decom_cost;
	float Pmax;  // MW
	float Pmin;
	float rampU;
	float rampD;
	int emis_cost; //$/ton
	// revisit these paramters laters


	vector<float> prod_profile;

	plant(string t, int n, int ise, int cap, int f, int v, float emi, float hr, int lt, int dec,
		float pmax,float pmin, float ru, float rd, int emic)
	{
		this->type = t;
		this->num = n;
		this->is_exis = ise;
		this->capex = cap;
		this->fix_cost = f;
		this->var_cost = v;
		this->emis_rate = emi;
		this->heat_rate = hr;
		this->lifetime = lt;
		this->decom_cost = dec;
		this->Pmax = pmax;
		this->Pmin = pmin;
		this->rampU = ru;
		this->rampD = rd;
		this->emis_cost = emic;
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
		map<int, vector<int>>& Le);

};

struct eStore
{
	int energy_cost;// &/MWh
	int power_cost; // &/MW
	float eff_ch;  // charge efficiency
	float eff_disCh; // discharge efficiency
	eStore(int en, int pow, float ch, float dis)
	{
		this->energy_cost = en;
		this->power_cost = pow;
		this->eff_ch = ch;
		this->eff_disCh = dis;
	}
	static vector<eStore> read_elec_storage_data(string FileName);

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
	static vector<pipe> read_pipe_data(string FileName, map<int, vector<int>> &Lg);
};


struct Params
{
	// General
	static float WACC;
	static int trans_unit_cost;
	static int trans_line_lifespan;
	static float NG_price;
	static float Emis_lim;
	static float RPS;

	static float dfo_pric;
	static float coal_price;
	static float E_curt_cost;
	static float G_curt_cost;
	static float pipe_per_mile;
	static int pipe_lifespan;
	static vector<int> RepDaysCount;


	// natural gas data
	static vector<int> Tg;
	static vector<gnode> Gnodes;
	static vector<pipe> PipeLines;
	static map<int, vector<int>> Lg;


	// electricity data
	static map<int, vector<int>> Le;
	static vector<int> Te;
	static vector<int> time_weight;
	static vector<enode> Enodes;
	static vector<plant> Plants;
	static vector<branch> Branches;
	static vector<eStore> Estorage;
};