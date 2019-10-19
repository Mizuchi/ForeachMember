#include "ForeachMember.h"

#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

TEST(simple, example) {
  struct A {
    char c;
    std::string s;
    int i;
  } a;

  a.i = 42;
  a.s = "test";
  a.c = 'c';

  std::ostringstream sout;
  foreachMember(a, [&sout](auto &&v) { sout << v << ", "; });
  EXPECT_EQ(sout.str(), "c, test, 42, ");
}

TEST(simple, empty) {
  struct {
  } a;
  foreachMember(a, [] { throw std::logic_error(""); });
}

TEST(simple, plain_struct) {
  struct A {
    int i;
    std::string s;
    char c;
    std::vector<int> v;
    double d;
    std::string s2;
  } a;

  a.i = 100;
  a.c = 'a';
  a.s = "hello";
  a.v = {3, 4, 5};
  a.s2 = "world";
  a.d = 5;

  struct {
    void operator()(int &i) {
      EXPECT_EQ(idx, 0);
      EXPECT_EQ(i, 100);
      i = 42; // member can be modified
      idx++;
    }
    void operator()(std::string &s) {
      if (idx == 1) {
        EXPECT_EQ(s, "hello");
      } else {
        EXPECT_EQ(idx, 5);
        EXPECT_EQ(s, "world");
      }
      idx++;
    }
    void operator()(char &c) {
      EXPECT_EQ(idx, 2);
      EXPECT_EQ(c, 'a');
      idx++;
    }
    void operator()(std::vector<int> &v) {
      EXPECT_EQ(idx, 3);
      EXPECT_EQ(v, std::vector<int>({3, 4, 5}));
      idx++;
    }
    void operator()(double &d) {
      EXPECT_EQ(idx, 4);
      EXPECT_EQ(d, 5);
      idx++;
    }

    int idx = 0;
  } checker;

  foreachMember(a, checker);
  EXPECT_EQ(checker.idx, 6);
  EXPECT_EQ(a.i, 42);
}

static auto isSame = [](const auto &x, const auto &y) {
  EXPECT_EQ(typeid(x), typeid(y));
  EXPECT_EQ(static_cast<const void *>(&x), static_cast<const void *>(&y));
};

namespace documentation_example {
using U = std::runtime_error;
// please sync with the documentation in header file
class T {
public:
  double d;
  const std::vector<U> v; // U does not matter, const is fine
  std::unique_ptr<U> p;   // non-copyable is fine
  U *ptr;                 // pointer is fine

  U foo(std::string); // member function is fine
  void *operator&();  // overloading is fine

  struct A {
    std::optional<int> u;
  } a;           // any standard layout type is fine
  std::thread t; // most types under std:: are fine, only few exceptions

private:
  static U u; // static private is fine, it'll be ignored
};

U T::u("");

TEST(documentation, example) {
  int idx = 0;
  T t;
  foreachMember(t, [&t, &idx](auto &&v) {
    switch (idx++) {
    case 0:
      isSame(v, t.d);
      break;
    case 1:
      isSame(v, t.v);
      break;
    case 2:
      isSame(v, t.p);
      break;
    case 3:
      isSame(v, t.ptr);
      break;
    case 4:
      isSame(v, t.a);
      break;
    case 5:
      isSame(v, t.t);
      break;
    }
  });
  EXPECT_EQ(idx, 6);
}
} // namespace documentation_example

TEST(constness, normal) {
  struct A {
    int i;
    std::thread t;
  };
  A a{};
  foreachMember(a, [](auto &&v) {
    static_assert(!std::is_const<std::remove_reference_t<decltype(v)>>::value);
  });
  const A ca{};
  foreachMember(ca, [](auto &&v) {
    static_assert(std::is_const<std::remove_reference_t<decltype(v)>>::value);
  });
}

TEST(constness, has_const_member) {
  struct A {
    int i;
    const std::thread t;
  };
  A a{};
  foreachMember(a, [](auto &&v) {
    // if T has const data member, it'll always pass const value to callback
    static_assert(std::is_const<std::remove_reference_t<decltype(v)>>::value);
  });
  const A ca{};
  foreachMember(ca, [](auto &&v) {
    static_assert(std::is_const<std::remove_reference_t<decltype(v)>>::value);
  });
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
