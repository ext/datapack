#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <cstdlib>
#include "data1.h"

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class Test: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(Test);
  CPPUNIT_TEST( test_unpack );
  CPPUNIT_TEST( test_unpack_filename );
  CPPUNIT_TEST_SUITE_END();

public:
  void test_unpack(){
	  char* tmp;
	  unpack(&TEST_DATA_1, &tmp);
	  CPPUNIT_ASSERT_EQUAL(std::string(tmp), std::string("test data\n"));
	  free(tmp);
  }

  void test_unpack_filename(){
	  char* tmp;
	  unpack_filename("data2.txt", &tmp);
	  CPPUNIT_ASSERT_EQUAL(std::string(tmp), std::string("test data\n"));
	  free(tmp);
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

int main(int argc, const char* argv[]){
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

  CppUnit::TextUi::TestRunner runner;

  runner.addTest( suite );
  runner.setOutputter(new CppUnit::CompilerOutputter(&runner.result(), std::cerr ));

  return runner.run() ? 0 : 1;
}
