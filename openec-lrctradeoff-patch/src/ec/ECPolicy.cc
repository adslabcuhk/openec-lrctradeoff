#include "ECPolicy.hh"

// ECPolicy::ECPolicy(string id, string classname, int n, int k, int w, bool locality, int opt, vector<string> param) {
ECPolicy::ECPolicy(string id, string classname, int n, int k, int w, int opt, vector<string> param)
{
  _id = id;
  _classname = classname;
  _n = n;
  _k = k;
  _w = w;
  //  _locality = locality;
  _opt = opt;
  _param = param;
}

ECBase *ECPolicy::createECClass()
{
  ECBase *toret;
  if (_classname == "RSCONV")
  {
    //    toret = new RSCONV(_n, _k, _w, _locality, _opt, _param);
    toret = new RSCONV(_n, _k, _w, _opt, _param);
  }
  else if (_classname == "AzureLRCFlat")
  {
    //    toret = new AzureLRCFlat(_n, _k, _w, _locality, _opt, _param);
    toret = new AzureLRCFlat(_n, _k, _w, _opt, _param);
  }
  else if (_classname == "AzureLRCTradeoff")
  {
    //    toret = new AzureLRCTradeoff(_n, _k, _w, _locality, _opt, _param);
    toret = new AzureLRCTradeoff(_n, _k, _w, _opt, _param);
  }
  else if (_classname == "AzureLRCOptR1022")
  {
    //    toret = new AzureLRCOptR1022(_n, _k, _w, _locality, _opt, _param);
    toret = new AzureLRCOptR1022(_n, _k, _w, _opt, _param);
  }
  else if (_classname == "AzureLRCOptM1022")
  {
    //    toret = new AzureLRCOptM1022(_n, _k, _w, _locality, _opt, _param);
    toret = new AzureLRCOptM1022(_n, _k, _w, _opt, _param);
  }
  else
  {
    cout << "unrecognized code, use default RSCONV" << endl;
    //    toret = new RSCONV(_n, _k, _w, _locality, _opt, _param);
    toret = new RSCONV(_n, _k, _w, _opt, _param);
  }
  return toret;
}

string ECPolicy::getPolicyId()
{
  return _id;
}

int ECPolicy::getN()
{
  return _n;
}

int ECPolicy::getK()
{
  return _k;
}

int ECPolicy::getW()
{
  return _w;
}

bool ECPolicy::getLocality()
{
  return _locality;
}

int ECPolicy::getOpt()
{
  return _opt;
}

string ECPolicy::getClassName()
{
  return _classname;
}

vector<string> ECPolicy::getParams()
{
  return _param;
}
