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

#ifndef incl_HPHP_ARRAY_PROVENANCE_H
#define incl_HPHP_ARRAY_PROVENANCE_H

#include "hphp/runtime/base/runtime-option.h"
#include "hphp/runtime/base/typed-value.h"
#include "hphp/runtime/base/types.h"

#include "hphp/util/low-ptr.h"
#include "hphp/util/rds-local.h"

#include <folly/Format.h>
#include <folly/Optional.h>

namespace HPHP {

struct StringData;

namespace arrprov {

///////////////////////////////////////////////////////////////////////////////

/*
 * A provenance annotation
 *
 * We need to store the filename and line since when assembling units, we
 * don't necessarily have the final Unit allocated yet. It may be faster to
 * make this a tagged union or store a different Tag type for static arrays
 */
struct Tag {
  Tag() = default;
  Tag(const StringData* filename, int line)
    : m_filename(filename)
    , m_line(line) {
    assertx(m_filename);
  }

  const StringData* filename() const { return m_filename; }
  int line() const { return m_line; }

  bool operator==(const Tag& other) const {
    return m_filename == other.m_filename &&
           m_line == other.m_line;
  }
  bool operator!=(const Tag& other) const { return !(*this == other); }

  std::string toString() const;

private:
  const StringData* m_filename{nullptr};
  int m_line{0};
};

/*
 * This is a separate struct so it can live in RDS and not be GC scanned--the
 * actual RDS-local handle is kept in the implementation
 */
struct ArrayProvenanceTable {
  /* The table itself -- allocated in general heap */
  folly::F14FastMap<const ArrayData*, Tag> tags;

  /*
   * We never dereference ArrayData*s from this table--so it's safe for the GC
   * to ignore them in this table
   */
  TYPE_SCAN_IGNORE_FIELD(tags);
};

///////////////////////////////////////////////////////////////////////////////

/*
 * Create a tag based on the current PC and unit.
 *
 * Requires VM regs to be synced or for a sync point to be available.
 */
folly::Optional<Tag> tagFromProgramCounter();

/*
 * Whether `ad` or `tv` admits a provenance tag---i.e., whether it's either a
 * vec or a dict.
 */
bool arrayWantsTag(const ArrayData* ad);
bool tvWantsTag(TypedValue tv);

/*
 * Get the provenance tag for `ad`.
 */
folly::Optional<Tag> getTag(const ArrayData* ad);

/*
 * Set the provnenance tag for `ad` to `tag`.
 */
void setTag(ArrayData* ad, const Tag& tag);

/*
 * Copy the provenance tag from `src` to `dest`.
 */
void copyTag(const ArrayData* src, ArrayData* dest);

/*
 * Like copyTag(), but for static arrays.
 *
 * Unlike all the other functions here, copyTagStatic() bails if array
 * provenance tracking is turned off.  (Everything else assumes it as a
 * prerequisite.)
 */
void copyTagStatic(const ArrayData* src, ArrayData* dest);

/*
 * Tag an array and all of its subarrays.
 *
 * If an empty or static array is encountered, it will not be tagged.
 */
void setTagRecursive(ArrayData* ad, const Tag& tag);

/*
 * Clear a tag for a released array---only call this if the array is henceforth
 * unreachable.
 */
void clearTag(const ArrayData* ad);

/*
 * Tag `tv` with provenance for the current PC and unit (if it admits a tag and
 * doesn't already have one).
 */
TypedValue tagTV(TypedValue tv);

/*
 * Produce a static empty array (with the same kind and contents as `base`)
 * with the given provenance tag.
 *
 * You should be calling this with staticEmptyVecArray() or
 * staticEmptyDictArray()
 */
const ArrayData* makeEmptyArray(const ArrayData* base, const Tag& t);

///////////////////////////////////////////////////////////////////////////////

}}

#define incl_HPHP_ARRAY_PROVENANCE_INL_H_
#include "hphp/runtime/base/array-provenance-inl.h"
#undef incl_HPHP_ARRAY_PROVENANCE_INL_H_

#endif
