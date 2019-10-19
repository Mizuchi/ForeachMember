# Iterate over members in struct, no hacks required!

## Usage

    struct T {
      T1 t1;
      T2 t2;
      T2 t3;
      ...
      TN tN;
    } t;

Then `foreachMember(t, callback);` is the equivalent to the following code:

    callback(t.t1);
    callback(t.t2);
    callback(t.t3);
    ...
    callback(t.tN);

## Example

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

## Requirement

1. T must be standard layout type (https://fburl.com/587498861)
2. T should be aggregate (https://fburl.com/587478985)
3. All non-static data member should be default constructible
4. All non-static data member must be move constructible
5. All non-static data member must not have template converting constructor
6. T must not have bit field (https://fburl.com/340998699604242)

Note: if T has bit field, using foreachMember results undefined behavior.
      otherwise, it should either work, or give you compile-error.

Figuratively speaking, here is an example which satisfies all requirements

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

