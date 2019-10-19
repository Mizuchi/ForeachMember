#pragma once
#include <memory>
#include <type_traits>

namespace detail {

template <class T, class Member> class IsMemberConst {
private:
  // T is not aggregate, otherwise `T{Detector{}}` must call aggregate
  // initialization. However, `T{Detector{}}` actually called copy constructor
  // instead
  static_assert(
      !std::is_same<Member, std::remove_const_t<T>>::value,
      "T must be aggregate "
      "(https://en.cppreference.com/w/cpp/language/aggregate_initialization). "
      "It's not allowed to have "
      "(1) private/protected data member "
      "(2) user-defined constructor "
      "(3) base class "
      "(4) virtual function ");

  // if Member is not move constructible,
  // `Member member = Detector{};` is not valid
  static_assert(std::is_move_constructible<Member>::value,
                "All non-static data members must be move constructible");

  // Note: standard layout type requires that all non-static data members must
  // be standard layout type. For the following structure,
  //
  // struct T { T1 t1; T2 t2; };
  //
  // If T1 or T2 is not standard layout type, then T isn't standard layout type.
  // However, since standard guarantees that (void*)&t1 < (void*)&t2. It does
  // not make much sense for compiler to add more than necessary padding between
  // adjacent data members (even it is allowed to do so). So in real world, I
  // believe it should still work if we remove the following static_assert.
  static_assert(std::is_standard_layout<Member>::value,
                "All non-static data members must be standard layout type "
                "(https://en.cppreference.com/w/cpp/concept/"
                "StandardLayoutType), otherwise T is not standard layout");

  // T can not have data member that has template converting function like this
  // struct Member {
  //   Member() {}
  //   template<class U> Member(U&&) {}
  // };
  // Otherwise `Member member = Detector{};` is ambiguous since it could call
  // both `Member::Member(Detector)` and `Detector::operator Member()`.
  struct SimpleDetector {
    template <class U>
    /* implicit */ operator U();
  };
  static int dummyCheck(Member);
  static_assert(
      sizeof(dummyCheck(SimpleDetector{})),
      "All non-static data members must not have template converting "
      "constructor "
      "(https://en.cppreference.com/w/cpp/language/converting_constructor)");

public:
  // If T is const or T has non-static const data member, we want to forward
  // member to callback function as const reference. Otherwise we could just
  // forward as reference so that the callback could modify the input.
  //
  // so, how to check whether T has non-static const data member?
  // 1) If T has const data member, it will not be move assignable by default.
  // 2) If T has user-defined move assignment, it won't be move constructible.
  // 3) If T has user-defined move constructor, it can't be aggregate.
  static constexpr bool value = std::is_const<T>::value ||
                                !std::is_move_assignable<T>::value ||
                                !std::is_move_constructible<T>::value;
};

template <class T, class MemberTypeCallback> struct Detector {
  MemberTypeCallback &memberTypeCallback;
  template <class Member, bool kIsConst = IsMemberConst<T, Member>::value>
  /* implicit */ operator Member() {
    memberTypeCallback(
        static_cast<std::conditional_t<kIsConst, const Member *, Member *>>(
            nullptr));
    return {};
  }

  template <int... I> void call(...) {
    // T is not aggregate, otherwise `T{Detector{}}` is valid or T is empty
    static_assert(
        sizeof...(I) > 0 || std::is_empty<T>::value,
        "T must be aggregate (https://en.cppreference.com/w/cpp/language/"
        "aggregate_initialization). "
        "It's not allowed to have "
        "(1) private/protected data member "
        "(2) user-defined constructor "
        "(3) base class "
        "(4) virtual function");

    // This is aggregate initialization
    // (https://en.cppreference.com/w/cpp/language/aggregate_initialization),
    // C++ Standard guarantees that the arguments will be evaluated sequentially
    // (https://en.cppreference.com/w/cpp/language/eval_order). Detector has
    // "operator U();", which means Detector has implicit conversion to any
    // type. In order to initialize T, the compiler will call
    // "Detector::operator U()" with T's member type in memory layout's order,
    // which means we could record type of each member and use it to compute the
    // offsets.
    T{(I, *this)...};
  }

  template <int... I> auto call(int) -> decltype(T{*this, (I, *this)...}) {
    call<0, I...>(0);
    return {};
  }
};

template <class T, class Callback> void foreachMemberType(Callback &&callback) {
  Detector<std::remove_reference_t<T>, decltype(callback)>{callback}.call(0);
}
} // namespace detail

template <class T, class Callback>
void foreachMember(T &&t, Callback &&callback) {
  detail::foreachMemberType<T>([&t, &callback, lastUnusedOffset = 0](
                                   auto *dummyMember) mutable {
    using Member = std::remove_reference_t<decltype(*dummyMember)>;
    constexpr auto memAlignment = alignof(Member);

    // `offset` is the minimum number which satisfies
    // 1. offset >= lastUnusedOffset
    // 2. offset % memAlignment == 0
    auto offset =
        (lastUnusedOffset + memAlignment - 1) / memAlignment * memAlignment;
    lastUnusedOffset = offset + sizeof(Member);

    const char *addr =
        static_cast<const char *>(static_cast<const void *>(std::addressof(t)));
    const Member &member =
        *static_cast<const Member *>(static_cast<const void *>(addr + offset));

    // It is possible to provide such interface:
    //   callback(Member&& member, std::integral_constant<int, pos>);
    // though I am not sure whether it's useful.
    callback(const_cast<Member &>(member));
  });

  // This should be no-op in C++14 since if T is aggregate and all non-static
  // data members are standard layout type, T should be standard layout type.
  // However, this is not the case in C++17. In C++17, aggregate could have base
  // type. Without this check, we don't know whether compiler performed empty
  // base optimization.
  static_assert(std::is_standard_layout<std::remove_reference_t<T>>::value);
  static_assert(!std::is_volatile<std::remove_reference_t<T>>::value);
}
