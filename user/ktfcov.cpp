#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib/ktf_run.h"
#include "lib/debug.h"
#include "kernel/unlproto.h"

using namespace std;

/* This program is a generic
 * user level application to enable/disable coverage of kernel modules.
 */

void
usage(char *progname)
{
	cerr << "Usage: " << progname << " [-e module[-m]] [-d module]\n";
}

int main (int argc, char** argv)
{
  int opt, nopts = 0;
  unsigned int cov_opts = 0;
  std::string modname = std::string();
  bool enable = false;

  testing::GTEST_FLAG(output) = "xml:ktest.xml";
  testing::InitGoogleTest(&argc,argv);

  if (argc < 3) {
	usage(argv[0]);
	return -1;
  }

  while ((opt = getopt(argc, argv, "e:d:m")) != -1) {
	switch (opt) {
	case 'e':
		nopts++;
		enable = true;
		modname = optarg;
		break;
	case 'd':
		nopts++;
		enable = false;
		modname = optarg;
		break;
	case 'm':
		cov_opts |= KTF_COV_OPT_MEM;
		break;
	default:
		cerr << "Unknown option '" << char(optopt) << "'";
		return -1;
	}
  }
  /* Either enable or disable must be specified, and -m is only valid
   * for enable.
   */
  if (modname.size() == 0 || nopts != 1 || (cov_opts && !enable)) {
	usage(argv[0]);
	return -1;
  }
  return utest::set_coverage(modname, cov_opts, enable);
}
