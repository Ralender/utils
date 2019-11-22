/// experimentation with llvm::TrailingObjects that work for any class
/// in a Hierarchy instead of just the last one.

#ifndef SG_TRAILLING_OBJECT_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/TrailingObjects.h"

namespace llvm {

/// utility class used to indicate a count of a known type.
template <typename T> struct TrailingObjCount {
  size_t Count;
  TrailingObjCount(size_t C) : Count(C) {}
};

namespace trailing_objects_internal {

/// used to interact with parameter packs.
template <typename...> struct Pack {};

template <typename...> struct MergePack {};
template <typename... Tys1, typename... Tys2>
struct MergePack<Pack<Tys1...>, Pack<Tys2...>> {
  using type = Pack<Tys1..., Tys2...>;
};

class HierarchicalTrailingObjectsImpl : TrailingObjectsBase {
protected:
  template <typename...> struct MergeTrailingPack {};
  template <typename BaseType, typename TrailingTysPack>
  struct MergeTrailingPack<BaseType, TrailingTysPack> {
    using type = typename MergePack<
        typename MergeTrailingPack<
            typename BaseType::Trailing::Parent,
            typename BaseType::Trailing::TrailingTysPack>::type,
        TrailingTysPack>::type;
  };

  template <typename TrailingTysPack>
  struct MergeTrailingPack<void, TrailingTysPack> {
    using type = TrailingTysPack;
  };

  /// Emit a pack of all type before ToFind in FirstCandidate and Others.
  template <typename ToFind, typename FirstCandidate, typename... Others>
  struct PackBeforeType {
    using type = typename MergePack<
        Pack<FirstCandidate>,
        typename PackBeforeType<ToFind, Others...>::type>::type;
  };

  template <typename ToFind, typename LastCandidate>
  struct PackBeforeType<ToFind, LastCandidate> {
    static_assert(!std::is_same<ToFind, LastCandidate>::value, "no such type");
  };

  template <typename ToFind> struct PackBeforeType<ToFind, ToFind> {
    using type = Pack<>;
  };

  template <typename ToFind, typename... Others>
  struct PackBeforeType<ToFind, ToFind, Others...> {
    using type = Pack<>;
  };

  template <typename BaseTy>
  using RecursivePack = MergeTrailingPack<BaseTy, Pack<>>;

  template <typename...> struct MaxTrailingAlign {};

  template <typename... Tys> struct MaxTrailingAlign<Pack<Tys...>> {
    static constexpr size_t value =
        llvm::trailing_objects_internal::AlignmentCalcHelper<Tys...>::Alignment;
  };

  template <typename P, typename T,
            typename std::enable_if<!std::is_final<T>::value, int>::type = 0>
  constexpr size_t getObjectSizeSFINAE(T *Ptr) {
    size_t Ret = Ptr->getObjectSize();
    assert(Ret >= sizeof(T));
    return Ret;
  }

  template <typename P, typename T,
            typename std::enable_if<std::is_final<T>::value &&
                                        !std::is_same<P, void>::value,
                                    int>::type = 0>
  constexpr size_t getObjectSizeSFINAE(T *Ptr) {
    assert(getObjectSizeSFINAE<typename P::Trailing::Parent>(
               static_cast<P *>(Ptr)) == sizeof(T) &&
           "incorrect size getter");
    return sizeof(T);
  }

  template <typename P, typename T,
            typename std::enable_if<std::is_final<T>::value &&
                                        std::is_same<P, void>::value,
                                    int>::type = 0>
  constexpr size_t getObjectSizeSFINAE(T *Ptr) {
    return sizeof(T);
  }

  static constexpr unsigned InternalComputeAlignedSize(size_t CurrentSize) {
    return CurrentSize;
  }

  template <typename First, typename... OtherTys>
  static constexpr unsigned
  InternalComputeAlignedSize(size_t CurrentSize,
                             TrailingObjCount<First> FirstTrailingObjCount,
                             OtherTys... Others) {
    return InternalComputeAlignedSize(
        (FirstTrailingObjCount.Count
             ? llvm::alignTo<alignof(First)>(CurrentSize) +
                   sizeof(First) * FirstTrailingObjCount.Count
             : CurrentSize),
        Others...);
  }

  template <typename BaseTy, typename... Tys>
  size_t ComputeSelfTrailingOffset(BaseTy *Ptr, size_t CurrentSize,
                                   Pack<Tys...>) {
    size_t Ret = InternalComputeAlignedSize(
        CurrentSize, TrailingObjCount<Tys>{Ptr->BaseTy::numTrailingObjects(
                         OverloadToken<Tys>{})}...);
    return Ret;
  }

  template <typename ParentType,
            typename std::enable_if<std::is_same<ParentType, void>::value,
                                    int>::type = 0>
  size_t getParentPrevTrailingSize(ParentType *Ptr, size_t CurrentSize) {
    return CurrentSize;
  }
  template <typename ParentType,
            typename std::enable_if<!std::is_same<ParentType, void>::value,
                                    int>::type = 0>
  size_t getParentPrevTrailingSize(ParentType *Ptr, size_t CurrentSize) {
    return Ptr->ParentType::Trailing::AddSelfTrailingSize(CurrentSize);
  }

  template <typename...> struct FirstType {};

  template <typename First, typename... Others>
  struct FirstType<First, Others...> {
    using type = First;
  };

  template <typename T, typename std::enable_if<std::is_same<T, void>::value,
                                                int>::type = 0>
  static constexpr size_t AddSelfTrailingSizeSFINAE(T *, size_t CurrentSize) {
    return CurrentSize;
  }

  template <typename T, typename std::enable_if<!std::is_same<T, void>::value,
                                                int>::type = 0>
  static constexpr size_t AddSelfTrailingSizeSFINAE(T *Prev,
                                                    size_t CurrentSize) {
    return Prev->T::Trailing::AddSelfTrailingSize(CurrentSize);
  }
};

} // namespace trailing_objects_internal

/// HierarchicalTrailingObjects has a similar purpose as TrailingObjects but can
/// handle non-final classes and hierarchy of classes with multiple trailing
/// objects. this class is used similarly to TrailingObjects but has a few
/// differences
///
/// \tparam BaseTy class inheriting from HierarchicalTrailingObjects
///
/// \tparam ParentBaseTy class BaseTy is inheriting from or void if BaseTy is
/// the first class of its Hierarchy.
///
/// \tparam TrailingTys types that can appear in the trailing space.
///
/// \code
/// struct Base { };
/// struct FirstTrailing
///     : Base,
/// // FirstTrailing has not parent with an llvm::HierarchicalTrailingObjects so
/// // ParentBaseTy is set to void.
///       private llvm::HierarchicalTrailingObjects<FirstTrailing, void, void *>
///       {
///   LLVM_TRAILING_HIERARCHY_INSERT_MEMBERS(FirstTrailing, void, void*);
///   // returns the number of void* in this trailing
///   size_t numTrailingObjects(Trailing::OverloadToken<void *>) const;
///
///   // returns the size of the current object.
///   // this can't be known at compile-time unless the class is final.
///   size_t getObjectSize() const { return sizeof(LastClass); }
/// };
///
/// struct Intermidate
///     : FirstTrailing,
///       private llvm::HierarchicalTrailingObjects<Intermidate, FirstTrailing,
///       float, char, short> {
///   LLVM_TRAILING_HIERARCHY_INSERT_MEMBERS(Intermidate, FirstTrailing, float,
///                                          char, short);
///   // returns the number of element of each type
///   size_t numTrailingObjects(Trailing::OverloadToken<float>) const;
///   size_t numTrailingObjects(Trailing::OverloadToken<char>) const;
///   size_t numTrailingObjects(Trailing::OverloadToken<short>) const;
///  static Intermidate* Create(...) {
///    Trailing::additionalSizeToAlloc(
///        llvm::TrailingObjCount<float>{0}, llvm::TrailingObjCount<char>{0},
///        llvm::TrailingObjCount<short>{0}, llvm::TrailingObjCount<void*>{0});
///    // ...
/// };
///
/// struct LastClass final // mark the last class final
///     : Intermidate,
///       private llvm::HierarchicalTrailingObjects<LastClass, Intermidate,
///       char> {
///   LLVM_TRAILING_HIERARCHY_INSERT_MEMBERS(LastClass, Intermidate, char);
///   size_t numTrailingObjects(Trailing::OverloadToken<char>) const;
///   static LastClass* Create(...) {
///     // totalSizeToAlloc can be used instead
///     Trailing::additionalSizeToAlloc(
///         llvm::TrailingObjCount<char>{0}, llvm::TrailingObjCount<float>{0},
///         llvm::TrailingObjCount<char>{0}, llvm::TrailingObjCount<short>{0},
///         llvm::TrailingObjCount<void*>{0});
///    // ...
///   }
/// };
/// size_t FirstTrailing::getObjectSize() const { return sizeof(LastClass); }
/// \endcode
///
/// remarks:
///  - it is preferable to inherit privately from
///  llvm::HierarchicalTrailingObjects. if was not inherited privateley
///  llvm::HierarchicalTrailingObjects use Trailing:: when accessing
///  totalSizeToAlloc, additionalSizeToAlloc and getTrailingObject to prevent
///  ambiguous lookup.
///  - mark classes as final when possible this allows to do more work at
///  compile time because we can know the size at compile-time.
///  - getObjectSize can often be implemented more and more efficiently as the
///  hierarchy goes deeper because less classes can inherit from the the
template <typename BaseTy, typename ParentBaseTy, typename... TrailingTys>
class HierarchicalTrailingObjects
    : trailing_objects_internal::HierarchicalTrailingObjectsImpl {
public:
  using Base = BaseTy;
  using Parent = ParentBaseTy;
  using TrailingTysPack = trailing_objects_internal::Pack<TrailingTys...>;

private:
  BaseTy *getDerived() { return static_cast<BaseTy *>(this); }

  ParentBaseTy *getDerivedParent() {
    return static_cast<ParentBaseTy *>(getDerived());
  }

  friend class HierarchicalTrailingObjectsImpl;

  size_t constexpr AddSelfTrailingSize(size_t CurrentSize) {
    return ComputeSelfTrailingOffset(
        getDerived(),
        AddSelfTrailingSizeSFINAE(getDerivedParent(), CurrentSize),
        trailing_objects_internal::Pack<TrailingTys...>{});
  }

  template <typename BaseType, typename T> T *getTrailingObjectsImpl() {
    assert(getDerived()->BaseTy::numTrailingObjects(OverloadToken<T>{}) > 0 &&
           "no element of that kind stored");
    return reinterpret_cast<T *>(llvm::alignTo<alignof(T)>(
        reinterpret_cast<uintptr_t>(getDerived()) +
        getObjectSizeSFINAE<ParentBaseTy, BaseTy>(getDerived()) +
        ComputeSelfTrailingOffset(
            getDerived(),
            getParentPrevTrailingSize<ParentBaseTy>(getDerivedParent(), 0),
            typename PackBeforeType<T, TrailingTys...>::type{})));
  }

public:
  // Make this (privately inherited) member public.
#ifndef _MSC_VER
  using trailing_objects_internal::TrailingObjectsBase::OverloadToken;
#else
  // MSVC bug prevents the above from working, at least up through CL
  // 19.10.24629.
  template <typename T>
  using OverloadToken = typename ParentType::template OverloadToken<T>;
#endif

  template <typename... Tys>
  static constexpr unsigned
  additionalSizeToAlloc(TrailingObjCount<Tys>... Counts) {
    static_assert(std::is_same<typename RecursivePack<BaseTy>::type,
                               trailing_objects_internal::Pack<Tys...>>::value,
                  "incorrect types");
    static_assert(
        MaxTrailingAlign<typename RecursivePack<BaseTy>::type>::value <=
            alignof(BaseTy),
        "alignment cannot be calculated correctly; over-align BaseTy to fixe "
        "this");
    return InternalComputeAlignedSize(0, Counts...);
  }
  template <typename... Tys>
  static constexpr unsigned totalSizeToAlloc(TrailingObjCount<Tys>... Counts) {
    return sizeof(BaseTy) + additionalSizeToAlloc(Counts...);
  }

  /// Return a pointer on the first element of type T stored in trailling space
  /// by
  template <typename T> T *getTrailingObjects() {
    return getTrailingObjectsImpl<BaseTy, T>();
  }
  template <typename T> T *getTrailingObjects() const {
    return const_cast<HierarchicalTrailingObjects<BaseTy, ParentBaseTy,
                                                  TrailingTys...> *>(this)
        ->template getTrailingObjects<T>();
  }

  template <typename T> llvm::MutableArrayRef<T> getTrailingArray() {
    return llvm::MutableArrayRef<T>(
        getTrailingObjectsImpl<BaseTy, T>(),
        getDerived()->BaseTy::numTrailingObjects(OverloadToken<T>{}));
  }
  template <typename T> llvm::ArrayRef<T> getTrailingArray() const {
    return const_cast<HierarchicalTrailingObjects<BaseTy, ParentBaseTy,
                                                  TrailingTys...> *>(this)
        ->template getTrailingArray<T>();
  }
};

#define LLVM_TRAILING_HIERARCHY_INSERT_MEMBERS(Type, ...)                      \
  using Trailing = ::llvm::HierarchicalTrailingObjects<Type, __VA_ARGS__>;     \
  friend class ::llvm::HierarchicalTrailingObjects<Type, __VA_ARGS__>;         \
  friend class ::llvm::trailing_objects_internal::                             \
      HierarchicalTrailingObjectsImpl

} // namespace llvm

#endif