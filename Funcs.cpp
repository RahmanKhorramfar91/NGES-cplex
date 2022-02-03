#include"Funcs.h"


void Read_rep_days(string name, vector<int>& Rep, vector<int> &RepCount)
{
	ifstream fid(name);
	string line;
	while (getline(fid, line))
	{
		float re, rc;
		std::istringstream iss(line);
		iss >> re >> rc;
		Rep.push_back(re);
		RepCount.push_back(rc);
	}
}