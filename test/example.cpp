#define BOOST_TEST_MODULE test_module
#include <ForeachMember.h>
#include <boost/test/included/unit_test.hpp>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

BOOST_AUTO_TEST_CASE(example) {
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
    BOOST_TEST(sout.str() == "c, test, 42, ");
}

BOOST_AUTO_TEST_CASE(simple) {
    struct {
    } a;
    foreachMember(a, [] { throw std::logic_error(""); });
}

BOOST_AUTO_TEST_CASE(plain_struct) {
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
            BOOST_TEST(idx == 0);
            BOOST_TEST(i == 100);
            i = 42; // member can be modified
            idx++;
        }
        void operator()(std::string &s) {
            if (idx == 1) {
                BOOST_TEST(s == "hello");
            } else {
                BOOST_TEST(idx == 5);
                BOOST_TEST(s == "world");
            }
            idx++;
        }
        void operator()(char &c) {
            BOOST_TEST(idx == 2);
            BOOST_TEST(c == 'a');
            idx++;
        }
        void operator()(std::vector<int> &v) {
            BOOST_TEST(idx == 3);
            BOOST_TEST(v == std::vector<int>({3, 4, 5}));
            idx++;
        }
        void operator()(double &d) {
            BOOST_TEST(idx == 4);
            BOOST_TEST(d == 5);
            idx++;
        }

        int idx = 0;
    } checker;

    foreachMember(a, checker);
    BOOST_TEST(checker.idx == 6);
    BOOST_TEST(a.i == 42);
}

static auto isSame = [](const auto &x, const auto &y) {
    BOOST_CHECK(typeid(x) == typeid(y));
    BOOST_CHECK(static_cast<const void *>(&x) == static_cast<const void *>(&y));
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

BOOST_AUTO_TEST_CASE(documentation) {
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
    BOOST_TEST(idx == 6);
}
} // namespace documentation_example

BOOST_AUTO_TEST_CASE(correctness) {
    struct A {
        int i;
        std::thread t;
    };
    A a{};
    foreachMember(a, [](auto &&v) {
        static_assert(
            !std::is_const<std::remove_reference_t<decltype(v)>>::value);
    });
    const A ca{};
    foreachMember(ca, [](auto &&v) {
        static_assert(
            std::is_const<std::remove_reference_t<decltype(v)>>::value);
    });
}

BOOST_AUTO_TEST_CASE(has_const_member) {
    struct A {
        int i;
        const std::thread t;
    };
    A a{};
    foreachMember(a, [](auto &&v) {
        // if T has const data member, it'll always pass const value to callback
        static_assert(
            std::is_const<std::remove_reference_t<decltype(v)>>::value);
    });
    const A ca{};
    foreachMember(ca, [](auto &&v) {
        static_assert(
            std::is_const<std::remove_reference_t<decltype(v)>>::value);
    });
}
