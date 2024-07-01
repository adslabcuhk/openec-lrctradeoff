#include "ec/ECBase.hh"
#include "ec/ECDAG.hh"
#include "ec/RSCONV.hh" // should delete later
#include "common/Config.hh"
#include "inc/include.hh"

using namespace std;

void usage()
{
    cout << "Usage: ./PlacementTest code" << endl;
    cout << "  0. code (rs_9_6_op1/waslrc/ia/drc643/rsppr/drc963/rawrs/clay_6_4/butterfly_6_4)" << endl;
}

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        usage();
        exit(1);
    }

    string ecid = string(argv[1]);
    cout << "ecid: " << ecid << endl;

    string confpath = "conf/sysSetting.xml";
    Config *conf = new Config(confpath);

    // get ecpolicy
    ECPolicy *ecpolicy = conf->_ecPolicyMap[ecid];
    ECBase *ec = ecpolicy->createECClass();
    int ecn = ecpolicy->getN();
    int eck = ecpolicy->getK();
    int ecw = ecpolicy->getW();
    int opt = ecpolicy->getOpt();
    bool locality = ecpolicy->getLocality();
    vector<vector<int>> group;
    ec->Place(group);
    cout << "ecn: " << ecn << ", eck: " << eck << ", ecw: " << ecw << ", opt: " << opt << endl;

    cout << "Hierarchical settings (start)\n";
    for (int i = 0; i < group.size(); i++)
    {
        cout << "Group " << i << ": ";
        for (int j = 0; j < group[i].size(); j++)
        {
            cout << group[i][j] << " ";
        }
        cout << endl;
    }
    cout << "Hierarchical settings (end)\n";

    return 0;
}
