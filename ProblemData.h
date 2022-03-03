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
void Read_rep_days(string name, vector<int>& Rep, vector<int>& RepCount);
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
	int Umax; // maximum number of plants in a Enode
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
		float pmax, float pmin, float ru, float rd, int emic, int max_num)
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
		this->Umax = max_num;
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
	float FOM; // fixed operating and maintanence cost
	eStore(int en, int pow, float ch, float dis,float fom)
	{
		this->energy_cost = en;
		this->power_cost = pow;
		this->eff_ch = ch;
		this->eff_disCh = dis;
		this->FOM = fom;
	}
	static vector<eStore> read_elec_storage_data(string FileName);

};

struct gnode
{
	int num;
	int fips;
	vector<float> demG;
	int out_dem;
	int injU;
	int injL = 0.0;
	vector<int> Lexp;
	vector<int> Limp;
	vector<int> adjE;
	vector<int> adjS;
	gnode(int n, int f, int od, int inj, vector<int> as)
	{
		this->num = n;
		this->fips = f;
		this->out_dem = od;
		this->injU = inj;
		this->adjS = as;
	}

	static vector<gnode> read_gnode_data(string FileName);
	static void read_Lexp_data(string FileName, vector<gnode>& Gnodes);
	static void read_Limp_data(string FileName, vector<gnode>& Gnodes);
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
	static vector<pipe> read_pipe_data(string FileName, map<int, vector<int>>& Lg);
};

struct exist_gSVL
{
	int num;
	int store_cap; // max storage capacity
	int vap_cap; // max vaporization capacity
	int liq_cap; // max liquefaction capacity

	exist_gSVL(int n, int sc, int vc, int lc)
	{
		this->num = n;
		this->store_cap = sc;
		this->vap_cap = vc;
		this->liq_cap = lc;
	}
	static vector<exist_gSVL> read_exist_SVL_data(string FL);
};

struct SVL
{
	// SVL 0 : Storage facility
	// SVL 1: Vaporization facility
	// SVL 2: Liquefaction facility
	int Capex;
	int FOM;
	float eff_ch;  // charge efficiency
	float eff_disCh; // discharge efficiency
	float BOG = 6e-6; // Boil-off gas, Dharik: The boiled off gas is sent to the pipeline network. You can convert this to an hourly rate and model this as the minimum discharge of the tank in each hour
	SVL(int cp, int fm, float ch, float dis)
	{
		this->Capex = cp;
		this->FOM = fm;
		this->eff_ch = ch;
		this->eff_disCh = dis;
	}

	static vector<SVL> read_SVL_data(string FL);
};

struct Params
{
	// General
	static int Num_Rep_Days;
	static float WACC;
	static int trans_unit_cost;
	static int trans_line_lifespan;
	static float NG_price;
	//static float Emis_lim;
	//static float RPS;

	static float dfo_pric;
	static float coal_price;
	static float nuclear_price;
	static float E_curt_cost;
	static float G_curt_cost;
	static float pipe_per_mile;
	static int pipe_lifespan;
	static int battery_lifetime;
	static int SVL_lifetime;
	static vector<int> RepDaysCount;
	static float NG_emis_rate;


	// natural gas data
	static vector<int> Tg;
	static vector<gnode> Gnodes;
	static vector<pipe> PipeLines;
	static map<int, vector<int>> Lg;
	static vector<exist_gSVL> Exist_SVL;
	static vector<SVL> SVLs;


	// electricity data
	static map<int, vector<int>> Le;
	static vector<int> Te;
	static vector<int> time_weight;
	static vector<enode> Enodes;
	static vector<plant> Plants;
	static vector<branch> Branches;
	static vector<eStore> Estorage;
};


void Read_Data();


