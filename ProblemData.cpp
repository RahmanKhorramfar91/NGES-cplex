#include "ProblemData.h"
vector<gnode> Params::Gnodes;
vector<pipe> Params::PipeLines;
vector<enode> Params::Enodes;
vector<Fips> Params::all_FIPS;
vector<plant> Params::Plants;
vector<branch> Params::Branches;
vector<int> Params::Tg;
vector<int> Params::Te;
vector<int> Params::RepDaysCount;
vector<int> Params::time_weight;
float Params::WACC;
int Params::trans_unit_cost;
int Params::trans_line_lifespan;
float Params::NG_price;
float Params::dfo_pric;
float Params::coal_price;
float Params::E_curt_cost;
float Params::G_curt_cost;
float Params::pipe_per_mile;
float Params::Emis_lim;
float Params::RPS;
int Params::pipe_lifespan;
map<int, vector<int>> Params::Lnm;	




vector<enode> enode::read_bus_data(string name)
{
	vector<enode> Buses;


	ifstream fid(name);
	string line;
	while (getline(fid, line))
	{
		std::istringstream iss(line);
		float bus_num, fips_id, fips_order;
		iss >> bus_num >> fips_id >> fips_order;
		enode bs((int)bus_num, (int)fips_id, (int)fips_order);
		Buses.push_back(bs);
	}

	fid.close();
	return Buses;
}

void  enode::read_adj_data(string name, vector<enode>& Enodes)
{
	ifstream fid(name);
	string line;
	int j = -1;
	while (getline(fid, line))
	{
		string temp = "";
		j++;
		int if_two_tab = 0;
		for (int i = 0; i < line.length(); i++)
		{
			if (line[i] == '\t' || i == line.length() - 1)
			{
				if_two_tab++;
				if (if_two_tab > 1)
				{
					break;
				}
				Enodes[j].adj_buses.push_back(std::stof(temp));

				temp = "";
			}
			else
			{
				temp.push_back(line[i]);
				if_two_tab = 0;
			}
		}
	}


}

void enode::read_exist_plt_data(string FileName, vector<enode>& Enodes)
{

	std::map<string, int> sym2ind = { {"ng",0},{"dfo", 1},
{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7} };


	ifstream fid(FileName);
	string line;
	while (getline(fid, line))
	{
		std::istringstream iss(line);
		string type;
		float bus_num, Pmax, Pmin, g1, g2, g3, count;
		iss >> bus_num >> type >> Pmax >> Pmin >> g1 >> g2 >> g3 >> count;

		Enodes[(int)bus_num].Init_plt_types.push_back(type);
		int ind1 = sym2ind[type];
		Enodes[(int)bus_num].Init_plt_ind.push_back(ind1);
		float* plim = new float[2]{ Pmin,Pmax };
		Enodes[(int)bus_num].Init_plt_prod_lim.push_back(plim);
		Enodes[(int)bus_num].Init_plt_count.push_back((int)count);


	}
}


vector<Fips> Fips::read_fips_data(string name)
{
	vector<Fips> all_FIPS;
	ifstream fid(name);
	string line;
	while (getline(fid, line))
	{
		std::istringstream iss(line);
		float f, s;
		iss >> f >> s;
		Fips fips((int)f, (int)s);
		all_FIPS.push_back(fips);
	}
	fid.close();
	return all_FIPS;
}

void Fips::read_demand_Data(string name, vector<Fips>& all_FIPS)
{
	ifstream fid(name);
	string line;
	while (getline(fid, line))
	{
		//vector<string> v;
		string temp = "";
		int j = 0;
		for (int i = 0; i < line.length(); i++)
		{
			if (line[i] == '\t' || i == line.length() - 1)
			{
				all_FIPS[j].demE.push_back(std::stof(temp));
				j++;
				temp = "";
			}
			else
			{
				temp.push_back(line[i]);
			}
		}
	}
	fid.close();
}



vector<plant> plant::read_new_plant_data(string name)
{
	vector<plant> NewPlants;
	ifstream fid(name);
	string line;
	while (getline(fid, line))
	{
		std::istringstream iss(line);
		float n, capex,pmax, pmin,ru,rd, fix_cost, var_cost, decom_cost, emis_cost, lifespan;
		string type;
		float h, emis_rate;
		iss >> type >> n >> capex >>pmax>>pmin>>ru>>rd>> fix_cost >> var_cost >> h >> emis_rate >> decom_cost >> emis_cost >> lifespan;
		plant np(type, (int)n, (int)capex,pmax,pmin,ru,rd, (int)fix_cost, (int)var_cost, h, emis_rate, (int)decom_cost, (int)emis_cost, (int)lifespan);
		NewPlants.push_back(np);
	}
	fid.close();
	return NewPlants;
}
void plant::read_VRE_profile(string FN1, string FN2, string FN3, vector<plant> &Plants)
{
	std::map<string, int> sym2pltType = { {"ng",0},{"dfo", 1},
{"solar", 2},{"wind", 3},{"wind_offshore", 4},{"hydro", 5},{"coal",6},{"nuclear",7} };

	ifstream fid1("profile_hydro_hourly.txt");
	ifstream fid2("profile_wind_hourly.txt");
	ifstream fid3("profile_solar_hourly.txt");
	string line;
	while (getline(fid1, line))
	{
		std::istringstream iss(line);
		float pp;
		iss >> pp;
		Plants[sym2pltType["hydro"]].prod_profile.push_back(pp);
	}

	while (getline(fid2, line))
	{
		std::istringstream iss(line);
		float pp;
		iss >> pp;
		Plants[sym2pltType["wind"]].prod_profile.push_back(pp);
	}
	while (getline(fid3, line))
	{
		std::istringstream iss(line);
		float pp;
		iss >> pp;
		Plants[sym2pltType["solar"]].prod_profile.push_back(pp);
	}

}


vector<branch> branch::read_branch_data(int nBus, string FN,
	string FN0, string FN1, string FN2, string FN3, string FN4, map<int, vector<int>>& Lnm)
{

	vector<branch> Branches;
	ifstream fid(FN); // branch per node
	ifstream fid0(FN0); // branch
	ifstream fid1(FN1); // length
	ifstream fid2(FN2); // is_exist
	ifstream fid3(FN3); // max flow
	ifstream fid4(FN4); // Susceptance

	string line, line0, line1, line2, line3, line4;
	int lc = 0;
	for (int i = 0; i < nBus; i++)
	{
		getline(fid, line);// branch per node
		getline(fid0, line0);// branch
		getline(fid1, line1);// length
		getline(fid2, line2); // is_exist
		getline(fid3, line3); // max flow
		getline(fid4, line4); // suscept

		std::istringstream iss(line);
		std::istringstream iss0(line0);
		std::istringstream iss1(line1);
		std::istringstream iss2(line2);
		std::istringstream iss3(line3);
		std::istringstream iss4(line4);

		float sn;
		iss >> sn;
		if ((int)sn == 0) { continue; }


		for (int j = 0; j < (int)sn; j++)
		{
			float s0, s1, s2, s3, s4;
			iss0 >> s0;
			iss1 >> s1;
			iss2 >> s2;
			iss3 >> s3;
			iss4 >> s4;

			branch nb(i, (int)s0, s1, s1, (int)s2, s3, s4);
			//Lnm[i*200+ (int)s0] = vector<int>();
			Lnm[i*200+ (int)s0].push_back(lc);
			lc++;
			Branches.push_back(nb);
		}
	}
	fid0.close();
	fid1.close();
	fid2.close();
	fid3.close();

	return Branches;

}

vector<gnode> gnode::read_gnode_data(string name)
{
	vector<gnode>Gnodes;
	ifstream fid2(name);
	string line;
	while (getline(fid2, line))
	{
		float num, fips, capU;
		std::istringstream iss(line);

		iss >> num >> fips >> capU;
		gnode gn((int)num, (int)fips, capU);
		Gnodes.push_back(gn);
	}
	return Gnodes;
}
void gnode::read_adjG_data(string FileName, vector<gnode>& Gnodes)
{
	ifstream fid2(FileName);
	string line;
	int j = -1;
	while (getline(fid2, line))
	{
		string temp = "";
		j++;
		int if_two_tab = 0;
		for (int i = 0; i < line.length(); i++)
		{
			if (line[i] == '\t' || i == line.length() - 1)
			{
				if_two_tab++;
				if (if_two_tab > 1) // end of number
				{
					break;
				}
				Gnodes[j].adjG.push_back(std::stoi(temp));
				temp = "";
			}
			else
			{
				temp.push_back(line[i]);
				if_two_tab = 0;
			}
		}
	}

}


void gnode::read_adjE_data(string FileName, vector<gnode>& Gnodes)
{
	ifstream fid2(FileName);
	string line;
	int j = -1;
	while (getline(fid2, line))
	{
		string temp = "";
		j++;
		int if_two_tab = 0;
		for (int i = 0; i < line.length(); i++)
		{
			if (line[i] == '\t' || i == line.length() - 1)
			{
				if_two_tab++;
				if (if_two_tab > 1) // end of number
				{
					break;
				}
				Gnodes[j].adjE.push_back(std::stoi(temp));
				temp = "";
			}
			else
			{
				temp.push_back(line[i]);
				if_two_tab = 0;
			}
		}
	}

}


vector<pipe> pipe::read_pipe_data(string name)
{
	vector<pipe> PipeLines;
	ifstream fid2(name);
	string line;
	while (getline(fid2, line))
	{
		float f, t, exi, le, ca;
		std::istringstream iss(line);
		iss >> f >> t >> exi >> le >> ca;
		pipe p2((int)f, (int)t, (int)exi, le, ca);
		PipeLines.push_back(p2);
	}

	return PipeLines;
}


void gnode::read_ng_demand_data(string name, vector<gnode>& Gnodes)
{
	int nGnode = Gnodes.size();

	ifstream fid2(name);
	string line;
	while (getline(fid2, line))
	{
		float dem2;
		std::istringstream iss(line);
		for (int i = 0; i < nGnode; i++)
		{
			iss >> dem2;
			Gnodes[i].demG.push_back(dem2);
		}
	}

}
