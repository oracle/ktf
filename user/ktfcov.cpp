#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ktf_run.h"
#include "debug.h"

using namespace std;

/* This program is a generic
 * user level application to enable/disable coverage of kernel modules.
 */

int main (int argc, char** argv)
{
  int opt;
  int ret = 0;

  testing::GTEST_FLAG(output) = "xml:ktest.xml";
  testing::InitGoogleTest(&argc,argv);

  if (argc < 3) {
	cerr << "Usage: " << argv[0] << " [-e module] [-d module]";
	return -1;
  }

  while ((opt = getopt(argc, argv, "e:d:")) != -1) {
	switch (opt) {
	case 'e':
		ret = utest::set_coverage(optarg, true);
		break;
	case 'd':
		ret = utest::set_coverage(optarg, false);
		break;
	default:
		cerr << "Unknown option '" << char(optopt) << "'";
		return -1;
	}
  }
  return ret;
}
