/****************************************************************************
  FileName     [ main.cpp ]
  PackageName  [ main ]
  Synopsis     [ Define main() function ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2007-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#include <string.h>
#include <cstdlib>
#include <cassert>
#include <string>
#include <sstream>
#include "cmdParser.h"
#include "client.h"
using namespace std;

int main(int argc, char** argv) {
   auto cmd = CmdParser<Client>();
   cmd.readCmd();  // press "Ctrl-d" to break
   cout << endl;
   return 0;
}
