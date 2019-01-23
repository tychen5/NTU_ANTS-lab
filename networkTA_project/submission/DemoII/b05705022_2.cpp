#include <string.h>
#include <cstdlib>
#include <cassert>
#include "cmdParser.h"
#include "client.h"
using namespace std;

int main(int argc, char** argv) {
   auto cmd = CmdParser<Client>();
   cmd.readCmd();
   cout << endl;
   return 0;
}
