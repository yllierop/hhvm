/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-present Facebook, Inc. (http://www.facebook.com)  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#ifndef incl_HPHP_VM_CLASS_INL_H_
#error "class-inl.h should only be included by class.h"
#endif

#include "hphp/runtime/base/runtime-error.h"
#include "hphp/runtime/base/strings.h"

namespace HPHP {
///////////////////////////////////////////////////////////////////////////////

inline bool Class::isZombie() const {
  return !m_cachedClass.bound();
}


inline bool Class::validate() const {
#ifndef NDEBUG
  assertx(m_magic == kMagic);
#endif
  assertx(name()->checkSane());
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Class::PropInitVec.

inline Class::PropInitVec::PropInitVec() : m_data(nullptr),
                                           m_size(0),
                                           m_capacity(0) {
}

inline Class::PropInitVec::iterator Class::PropInitVec::begin() {
  return m_data;
}

inline Class::PropInitVec::iterator Class::PropInitVec::end() {
  return m_data + m_size;
}

inline size_t Class::PropInitVec::size() const {
  return m_size;
}

inline TypedValueAux& Class::PropInitVec::operator[](size_t i) {
  assertx(i < m_size);
  return m_data[i];
}

inline const TypedValueAux& Class::PropInitVec::operator[](size_t i) const {
  assertx(i < m_size);
  return m_data[i];
}

inline bool Class::PropInitVec::reqAllocated() const {
  return m_capacity < 0;
}

///////////////////////////////////////////////////////////////////////////////
// Pre- and post-allocations.

inline LowPtr<Func>* Class::funcVec() const {
  return reinterpret_cast<LowPtr<Func>*>(
    reinterpret_cast<uintptr_t>(this) -
    m_funcVecLen * sizeof(LowPtr<Func>)
  );
}

inline void* Class::mallocPtr() const {
  return reinterpret_cast<void*>(
    reinterpret_cast<uintptr_t>(funcVec()) & ~(alignof(Class) - 1)
  );
}

inline const void* Class::mallocEnd() const {
  return reinterpret_cast<const char*>(this)
         + Class::classVecOff()
         + classVecLen() * sizeof(*classVec());
}

inline const LowPtr<Class>* Class::classVec() const {
  return m_classVec;
}

inline Class::veclen_t Class::classVecLen() const {
  return m_classVecLen;
}

///////////////////////////////////////////////////////////////////////////////
// Ancestry.

inline bool Class::classofNonIFace(const Class* cls) const {
  assertx(!(cls->attrs() & AttrInterface));
  if (m_classVecLen >= cls->m_classVecLen) {
    return (m_classVec[cls->m_classVecLen-1] == cls);
  }
  return false;
}

inline bool Class::classof(const Class* cls) const {
  // If `cls' is an interface, we can simply check to see if cls is in
  // this->m_interfaces.  Otherwise, if `this' is not an interface, the
  // classVec check will determine whether it's an instance of cls (including
  // the case where this and cls are the same trait).  Otherwise, `this' is an
  // interface, and `cls' is not, so we need to return false.  But the classVec
  // check can never return true in that case (cls's classVec contains only
  // non-interfaces, while this->classVec is either empty, or contains
  // interfaces).
  if (UNLIKELY(cls->attrs() & AttrInterface)) {
    return this == cls ||
      m_interfaces.lookupDefault(cls->m_preClass->name(), nullptr) == cls;
  }
  return classofNonIFace(cls);
}

inline bool Class::ifaceofDirect(const StringData* name) const {
  return m_interfaces.contains(name);
}

///////////////////////////////////////////////////////////////////////////////
// Basic info.

inline const StringData* Class::name() const {
  return m_preClass->name();
}

inline const PreClass* Class::preClass() const {
  return m_preClass.get();
}

inline Class* Class::parent() const {
  return m_parent.get();
}

inline StrNR Class::nameStr() const {
  return m_preClass->nameStr();
}

inline StrNR Class::parentStr() const {
  return m_preClass->parentStr();
}

inline Attr Class::attrs() const {
  assertx(Attr(m_attrCopy) == m_preClass->attrs());
  return Attr(m_attrCopy);
}

inline bool Class::rtAttribute(RuntimeAttribute a) const {
  return m_RTAttrs & a;
}

inline void Class::initRTAttributes(uint8_t a) {
  m_RTAttrs |= a;
}

inline bool Class::isUnique() const {
  return attrs() & AttrUnique;
}

inline bool Class::isPersistent() const {
  return attrs() & AttrPersistent;
}

inline bool Class::isDynamicallyConstructible() const {
  return attrs() & AttrDynamicallyConstructible;
}

///////////////////////////////////////////////////////////////////////////////
// Magic methods.

inline const Func* Class::getCtor() const {
  return m_ctor;
}

inline const Func* Class::getToString() const {
  return m_toString;
}

inline const Func* Class::get86pinit() const {
  return m_pinitVec.back();
}

inline const Func* Class::get86sinit() const {
  return m_sinitVec.back();
}

inline const Func* Class::get86linit() const {
  return m_linitVec.back();
}

///////////////////////////////////////////////////////////////////////////////
// Builtin classes.

inline bool Class::isBuiltin() const {
  return attrs() & AttrBuiltin;
}

template <bool Unlocked>
inline BuiltinCtorFunction Class::instanceCtor() const {
  return Unlocked ? m_extra->m_instanceCtorUnlocked : m_extra->m_instanceCtor;
}

inline BuiltinDtorFunction Class::instanceDtor() const {
  return m_extra->m_instanceDtor;
}

///////////////////////////////////////////////////////////////////////////////
// Object release.

inline ObjReleaseFunc Class::releaseFunc() const {
  return m_release;
}

///////////////////////////////////////////////////////////////////////////////
// Methods.

inline size_t Class::numMethods() const {
  return m_methods.size();
}

inline Func* Class::getMethod(Slot idx) const {
  auto funcVec = (LowPtr<Func>*)this;
  return funcVec[-((int32_t)idx + 1)];
}

inline void Class::setMethod(Slot idx, Func* func) {
  auto funcVec = (LowPtr<Func>*)this;
  funcVec[-((int32_t)idx + 1)] = func;
}

inline Func* Class::lookupMethod(const StringData* methName) const {
  Slot* idx = m_methods.find(methName);
  if (!idx) return nullptr;
  return getMethod(*idx);
}

///////////////////////////////////////////////////////////////////////////////
// Property metadata.

inline size_t Class::numDeclProperties() const {
  return m_declProperties.size();
}

inline size_t Class::numStaticProperties() const {
  return m_staticProperties.size();
}

inline uint32_t Class::declPropNumAccessible() const {
  return m_declPropNumAccessible;
}

inline folly::Range<const Class::Prop*> Class::declProperties() const {
  return m_declProperties.range();
}

inline folly::Range<const Class::SProp*> Class::staticProperties() const {
  return m_staticProperties.range();
}

inline Slot Class::lookupDeclProp(const StringData* propName) const {
  return m_declProperties.findIndex(propName);
}

inline Slot Class::lookupSProp(const StringData* sPropName) const {
  return m_staticProperties.findIndex(sPropName);
}

inline Slot Class::lookupReifiedInitProp() const {
  return m_declProperties.findIndex(s_86reified_prop.get());
}

inline bool Class::hasReifiedGenerics() const {
  return m_hasReifiedGenerics;
}

inline bool Class::hasReifiedParent() const {
  return m_hasReifiedParent;
}

inline RepoAuthType Class::declPropRepoAuthType(Slot index) const {
  return m_declProperties[index].repoAuthType;
}

inline RepoAuthType Class::staticPropRepoAuthType(Slot index) const {
  return m_staticProperties[index].repoAuthType;
}

inline const TypeConstraint& Class::declPropTypeConstraint(Slot index) const {
  return m_declProperties[index].typeConstraint;
}

inline const TypeConstraint& Class::staticPropTypeConstraint(Slot index) const {
  return m_staticProperties[index].typeConstraint;
}

inline bool Class::hasDeepInitProps() const {
  return m_hasDeepInitProps;
}

inline bool Class::forbidsDynamicProps() const {
  return attrs() & AttrForbidDynamicProps;
}

inline bool Class::serialize() const {
  if (m_serialized) return false;
  m_serialized = true;
  return true;
}

inline bool Class::wasSerialized() const {
  return m_serialized;
}

///////////////////////////////////////////////////////////////////////////////
// Property initialization.

inline bool Class::needInitialization() const {
  return m_needInitialization;
}

inline bool Class::maybeRedefinesPropTypes() const {
  return m_maybeRedefsPropTy;
}

inline bool Class::needsPropInitialValueCheck() const {
  return m_needsPropInitialCheck;
}

inline const Class::PropInitVec& Class::declPropInit() const {
  return m_declPropInit;
}

inline const VMFixedVector<const Func*>& Class::pinitVec() const {
  return m_pinitVec;
}

inline rds::Handle Class::checkedPropTypeRedefinesHandle() const {
  assertx(m_maybeRedefsPropTy);
  m_extra->m_checkedPropTypeRedefs.bind(rds::Mode::Normal);
  return m_extra->m_checkedPropTypeRedefs.handle();
}

inline rds::Handle Class::checkedPropInitialValuesHandle() const {
  assertx(m_needsPropInitialCheck);
  m_extra->m_checkedPropInitialValues.bind(rds::Mode::Normal);
  return m_extra->m_checkedPropInitialValues.handle();
}

///////////////////////////////////////////////////////////////////////////////
// Property storage.

inline void Class::initPropHandle() const {
  m_propDataCache.bind(rds::Mode::Normal);
}

inline rds::Handle Class::propHandle() const {
  return m_propDataCache.handle();
}

inline rds::Handle Class::sPropInitHandle() const {
  return m_sPropCacheInit.handle();
}

inline rds::Handle Class::sPropHandle(Slot index) const {
  return sPropLink(index).handle();
}

inline rds::Link<StaticPropData, rds::Mode::NonNormal>
Class::sPropLink(Slot index) const {
  assertx(m_sPropCacheInit.bound());
  assertx(numStaticProperties() > index);
  return m_sPropCache[index];
}

inline rds::Link<bool, rds::Mode::NonLocal> Class::sPropInitLink() const {
  return m_sPropCacheInit;
}

///////////////////////////////////////////////////////////////////////////////
// Constants.

inline size_t Class::numConstants() const {
  return m_constants.size();
}

inline const Class::Const* Class::constants() const {
  return m_constants.accessList();
}

inline bool Class::hasConstant(const StringData* clsCnsName) const {
  // m_constants.contains(clsCnsName) returns abstract constants
  auto clsCnsInd = m_constants.findIndex(clsCnsName);
  return (clsCnsInd != kInvalidSlot) &&
    !m_constants[clsCnsInd].isAbstract() &&
    !m_constants[clsCnsInd].isType();
}

inline bool Class::hasTypeConstant(const StringData* typeConstName,
                                   bool includeAbs) const {
  auto typeConstInd = m_constants.findIndex(typeConstName);
  return (typeConstInd != kInvalidSlot) &&
    (!m_constants[typeConstInd].isAbstract() || includeAbs) &&
    m_constants[typeConstInd].isType();
}

///////////////////////////////////////////////////////////////////////////////
// Interfaces and traits.

inline folly::Range<const ClassPtr*> Class::declInterfaces() const {
  return folly::range(m_declInterfaces.begin(),
                      m_declInterfaces.end());
}

inline const Class::InterfaceMap& Class::allInterfaces() const {
  return m_interfaces;
}

inline Slot Class::traitsBeginIdx() const {
  return m_extra->m_traitsBeginIdx;
}

inline Slot Class::traitsEndIdx() const   {
  return m_extra->m_traitsEndIdx;
}

inline const VMCompactVector<ClassPtr>& Class::usedTraitClasses() const {
  return m_extra->m_usedTraits;
}

inline const Class::TraitAliasVec& Class::traitAliases() const {
  return m_extra->m_traitAliases;
}

inline void Class::addTraitAlias(const PreClass::TraitAliasRule& rule) const {
  allocExtraData();
  m_extra.raw()->m_traitAliases.push_back(rule.asNamePair());
}

inline const Class::RequirementMap& Class::allRequirements() const {
  return m_requirements;
}

///////////////////////////////////////////////////////////////////////////////
// Instance bits.

inline bool Class::checkInstanceBit(unsigned int bit) const {
  assertx(bit > 0);
  return m_instanceBits[bit];
}

///////////////////////////////////////////////////////////////////////////////
// Throwable initialization.

inline bool Class::needsInitThrowable() const {
  return m_needsInitThrowable;
}

///////////////////////////////////////////////////////////////////////////////
// JIT data.

inline rds::Handle Class::classHandle() const {
  return m_cachedClass.handle();
}

inline void Class::setClassHandle(rds::Link<LowPtr<Class>,
                                            rds::Mode::NonLocal> link) const {
  assertx(!m_cachedClass.bound());
  m_cachedClass = link;
}

inline Class* Class::getCached() const {
  return m_cachedClass.isInit() ? *m_cachedClass : nullptr;
}

inline void Class::setCached() {
  m_cachedClass.initWith(this);
}

///////////////////////////////////////////////////////////////////////////////
// Native data.

inline const Native::NativeDataInfo* Class::getNativeDataInfo() const {
  return m_extra->m_nativeDataInfo;
}

///////////////////////////////////////////////////////////////////////////////
// Closure subclasses.

inline bool Class::isScopedClosure() const {
  return m_scoped;
}

inline const Class::ScopedClonesMap& Class::scopedClones() const {
  return m_extra->m_scopedClones;
}

/////////////////////////////////////////////////////////////////////////////
// Memoization

inline size_t Class::numMemoSlots() const {
  return m_extra->m_nextMemoSlot;
}

inline bool Class::hasMemoSlots() const {
  return numMemoSlots() > 0;
}

inline std::pair<Slot, bool> Class::memoSlotForFunc(FuncId func) const {
  assertx(hasMemoSlots());
  auto const it = m_extra->m_memoMappings.find(func);
  if (it != m_extra->m_memoMappings.end()) return it->second;
  // Each mapping is only stored in the class which defines it, so recurse up to
  // the parent. We should only be calling this with functions which have a memo
  // slot, so assert if we reach the end without finding a slot.
  if (m_parent) return m_parent->memoSlotForFunc(func);
  always_assert(false);
}

///////////////////////////////////////////////////////////////////////////////
// Other methods.

inline MaybeDataType Class::enumBaseTy() const {
  return m_enumBaseTy;
}

inline EnumValues* Class::getEnumValues() const {
  return m_extra->m_enumValues.load(std::memory_order_relaxed);
}

///////////////////////////////////////////////////////////////////////////////
// ExtraData.

inline void Class::allocExtraData() const {
  if (!m_extra) {
    m_extra = new ExtraData();
  }
}

///////////////////////////////////////////////////////////////////////////////
// Non-member functions.

inline Attr classKindAsAttr(ClassKind kind) {
  return static_cast<Attr>(kind);
}

inline bool isTrait(const Class* cls) {
  return cls->attrs() & AttrTrait;
}

inline bool isEnum(const Class* cls) {
  return cls->attrs() & AttrEnum;
}

inline bool isInterface(const Class* cls) {
  return cls->attrs() & AttrInterface;
}

inline bool isNormalClass(const Class* cls) {
  return !(cls->attrs() & (AttrTrait | AttrInterface | AttrEnum));
}

inline bool isAbstract(const Class* cls) {
  return cls->attrs() & AttrAbstract;
}

inline bool classHasPersistentRDS(const Class* cls) {
  return cls != nullptr &&
    rds::isPersistentHandle(cls->classHandle());
}

inline bool classMayHaveMagicPropMethods(const Class* cls) {
  auto constexpr no_overrides =
    AttrNoOverrideMagicGet |
    AttrNoOverrideMagicSet |
    AttrNoOverrideMagicIsset |
    AttrNoOverrideMagicUnset;

  return (cls->attrs() & no_overrides) != no_overrides;
}

inline const StringData* classToStringHelper(const Class* cls) {
 if (RuntimeOption::EvalRaiseClassConversionWarning) {
   raise_warning(Strings::CLASS_TO_STRING);
 }
 return cls->name();
}
///////////////////////////////////////////////////////////////////////////////
}
