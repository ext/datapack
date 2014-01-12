#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "data1.h"

#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class Test: public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(Test);
  CPPUNIT_TEST( test_unpack_inline );
  CPPUNIT_TEST( test_unpack_filename );
  CPPUNIT_TEST( test_unpack_pack );
  CPPUNIT_TEST_SUITE_END();

public:
  void test_unpack_inline(){
	  char* tmp;
	  unpack(&TEST_DATA_1, &tmp);
	  CPPUNIT_ASSERT_EQUAL(std::string(tmp), std::string("test data\n"));
	  free(tmp);
  }

  void test_unpack_filename(){
	  datapack_t handle = datapack_open(NULL);
	  if ( !handle ){
		  CPPUNIT_FAIL(std::string("unpack_open(..) failed: ") + strerror(errno));
	  }

	  char* tmp;
	  int ret = unpack_filename(handle, "data2.txt", &tmp);
	  if ( ret != 0 ){
		  CPPUNIT_FAIL(std::string("unpack_filename(..) failed: ") + strerror(ret));
	  }

	  CPPUNIT_ASSERT_EQUAL(std::string(tmp), std::string("test data\n"));
	  free(tmp);

	  datapack_close(handle);
  }

  void test_unpack_pack(){
	  datapack_t handle = datapack_open("tests/data2.pak");
	  if ( !handle ){
		  CPPUNIT_FAIL(std::string("unpack_open(..) failed: ") + strerror(errno));
	  }

	  char* tmp;
	  int ret = unpack_filename(handle, "data3.txt", &tmp);
	  if ( ret != 0 ){
		  CPPUNIT_FAIL(std::string("unpack_filename(..) failed: ") + strerror(ret));
	  }

	  CPPUNIT_ASSERT_EQUAL(std::string(tmp), std::string("test data\n"));
	  free(tmp);

	  datapack_close(handle);
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
