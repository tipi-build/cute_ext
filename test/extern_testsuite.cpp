#include <cute/cute.h>
#include <exception>

class TestClass final
{
public:
  void Test1_pass()
  {
    ASSERT(true);
  }

  void Test2_fail()
  {
    ASSERT(false);
  }

  void Test3_throws()
  {
    int f = std::stoi("ABBA");  // Throws: no conversion    
  }
};

cute::suite make_suite()
{
  cute::suite s{};
  s.push_back(CUTE_SMEMFUN(TestClass, Test1_pass));
  s.push_back(CUTE_SMEMFUN(TestClass, Test2_fail));
  s.push_back(CUTE_SMEMFUN(TestClass, Test3_throws));
  return s;
}


