Iterate over members in struct, no hacks required!  |travis| |license|
======================================================================

.. |travis| image:: https://travis-ci.com/Mizuchi/ForeachMember.svg?branch=master
   :target: https://travis-ci.com/Mizuchi/ForeachMember
   :alt: Travis Build Status

.. |license| image:: https://img.shields.io/badge/license-RPL%201.5-blueviolet
   :target: https://opensource.org/licenses/RPL-1.5
   :alt: License: RPL-1.5

Usage
---------

.. code-block:: cpp

    struct T {
      T1 t1;
      T2 t2;
      T2 t3;
      ...
      TN tN;
    } t;

Then `foreachMember(t, callback);` is the equivalent to the following code:

.. code-block:: cpp

    callback(t.t1);
    callback(t.t2);
    callback(t.t3);
    ...
    callback(t.tN);

Example
-----------

Try it online: https://godbolt.org/z/5n3Zjz

.. code-block:: cpp

    struct A {
      char c;
      std::string s;
      int i;
    } a;

    a.i = 42;
    a.s = "test";
    a.c = 'c';

    foreachMember(a, [](auto&& v) {
       std::cout << v << ", ";
    });

    // this code prints "c, test, 42, "

Requirement
--------------------

1. T must be standard layout type (https://en.cppreference.com/w/cpp/named_req/StandardLayoutType)
2. T should be aggregate (https://en.cppreference.com/w/cpp/language/aggregate_initialization)
3. All non-static data member should be default constructible
4. All non-static data member must be move constructible
5. All non-static data member must not have template converting constructor
6. T must not have bit field (https://en.cppreference.com/w/cpp/language/bit_field)

Note: if T has bit field, using foreachMember results undefined behavior.
      otherwise, it should either work, or give you compile-error.

Figuratively speaking, here is an example which satisfies all requirements

.. code-block:: cpp

    class T {
     public:
      double d;
      const std::vector<U> v; // U does not matter, const is fine
      std::unique_ptr<U> p; // non-copyable is fine
      U* ptr; // pointer is fine

      U foo(std::string); // member function is fine
      void* operator&(); // overloading is fine

      struct A {
        std::optional<int> u;
      } a; // any standard layout type is fine
      std::thread t; // most types under std:: are fine, only few exceptions

     private:
      static U u; // static private is fine, it'll be ignored
    };

Here is a counterexample which does not satisfy some requirements, therefore it can't be used by foreachMember

.. code-block:: cpp

    struct T : std::set<int> // base class is not supported
    {
      T() {} // any custom ctor is not supported, even it's empty
      const int& ref; // reference is not default constructible
      virtual void foo(); // virtual function breaks standard layout
  
      std::condition_variable cv; // non-moveable is not supported
  
      // All members must be standard layout type.
      // These are counterexamples that can't be data member.
      std::exception e;
      std::fstream f;
      std::function<void()> func;
      std::tuple<int,int> t;
      struct M { virtual ~M(); } m;
  
      int x = 10; // default member initializer is not supported
  
     protected:
      int x; // private/protected non-static data member is not supported
    };

