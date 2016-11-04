// FastTrie 2.4.10 2014-08-11

#ifndef __FAST_TRIE_2_H__
#define __FAST_TRIE_2_H__

#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
#include <numeric>

#include "MMap.h"

#ifndef FT2_NAMESPACE
namespace ft2 {
#else
namespace FT2_NAMESPACE {
#endif

template <class ValueT> class Container;
template <class ValueT> class Iterator;
template <class ValueT> class ReverseIterator;

template <class ValueT, class SizeT>                                      class Vector;
template <class KeyT, class ValueT, int option, class CharT, class SizeT> class Trie;
template <class KeyT, class ValueT, class HashT, class SizeT>             class HashMap;
template <class ValueT1, class ValueT2>                                   class Pair;

enum
{
	FT_NOTAIL     = 1, ///< do not save solo trie path in separated tail block
	FT_PATH       = 2, ///< save position of trie path terminator for extracting key
	FT_QUICKBUILD = 4, ///< reduce node allocating searching when build
};

enum { FT_ALIGN = 16 };

template <class T>
struct Range /// range of objects
{
	const T *begin; ///< begin of the range
	const T *end;   ///< end of the range

	Range()                       : begin(0), end(0) {}
	Range(const T *b, const T *e) : begin(b), end(e) {}
	template <class OtherT>
	Range(const Range<OtherT> &r) : begin((T *)r.begin), end((T *)r.end) {}
	template <class ValueT>
	inline Range(const ValueT &x);

	/// whether the size is 0
	bool  empty() const { return size() == 0; }
	/// number of objects in the range
	size_t size() const { return end - begin; }
};

/* defines range of various types, facilitates MulAddHash, Trie, etc. */

template <class T>
inline Range<T> range(const T &key)
{
	return Range<T>(&key, &key + 1);
}
template <class _CharT, class _Traits, class _Alloc>
inline Range<_CharT> range(const std::basic_string<_CharT, _Traits, _Alloc> &key)
{
	return Range<_CharT>(&*key.begin(), &*key.end());
}
template <class _Tp, class _Alloc>
inline Range<_Tp> range(const std::vector<_Tp, _Alloc> &key)
{
	return Range<_Tp>(&*key.begin(), &*key.end());
}
template <class ValueT, class SizeT>
inline Range<ValueT> range(const Vector<ValueT, SizeT> &key)
{
	return Range<ValueT>(&*key.begin(), &*key.end());
}
inline Range<char> range(const char *key)
{
	return Range<char>(key, key ? key + strlen(key) : 0);
}
inline Range<char> range(char *key)
{
	return Range<char>(key, key + strlen(key));
}
template <class T>
inline Range<T> range(T *key)
{
	T *end = key;
	while (*end) end = std::find(end, end + 1048576, T());
	return Range<T>(key, end);
}

template <class T>
template <class ValueT>
Range<T>::Range(const ValueT &x) { *this = range(x); }

template <class T>
struct HasVariableRange /// variable range types: string, vector, T *, ...
{ enum { value = false }; typedef T type; };

template <class _CharT, class _Traits, class _Alloc>
struct HasVariableRange<std::basic_string<_CharT, _Traits, _Alloc> >
                                                   { enum { value = true }; typedef _CharT type; };
template <class _Tp, class _Alloc>
struct HasVariableRange<std::vector<_Tp, _Alloc> > { enum { value = true }; typedef _Tp    type; };
template <class ValueT, class SizeT>
struct HasVariableRange<Vector<ValueT, SizeT> >    { enum { value = true }; typedef ValueT type; };
template <class T> struct HasVariableRange<T *>    { enum { value = true }; typedef T      type; };
template <class T, size_t N>
struct HasVariableRange<T[N]>                      { enum { value = true }; typedef T      type; };

class MulAddHash /// hash various types into uint32_t
{
public:
	template <class KeyT>
	uint32_t operator ()(const KeyT &key) const
	{
		static const uint32_t primes[16] =
		{
			0x01EE5DB9, 0x491408C3, 0x0465FB69, 0x421F0141,
			0x2E7D036B, 0x2D41C7B9, 0x58C0EF0D, 0x7B15A53B,
			0x7C9D3761, 0x5ABB9B0B, 0x24109367, 0x5A5B741F,
			0x6B9F12E9, 0x71BA7809, 0x081F69CD, 0x4D9B740B,
		};

		Range<uint8_t> r = range(key);

		uint32_t sum = 0, i = 0;

		if      (sizeof(typename HasVariableRange<KeyT>::type) % 4 == 0)
			for (const uint32_t *p = (uint32_t *)r.begin; p != (uint32_t *)r.end; ++ p)
				sum += primes[i ++ & 15] * (uint32_t)*p;
		else if (sizeof(typename HasVariableRange<KeyT>::type) % 4 == 2)
			for (const uint16_t *p = (uint16_t *)r.begin; p != (uint16_t *)r.end; ++ p)
				sum += primes[i ++ & 15] * (uint32_t)*p;
		else
			for (const uint8_t  *p = (uint8_t  *)r.begin; p != (uint8_t  *)r.end; ++ p)
				sum += primes[i ++ & 15] * (uint32_t)*p;

		return sum;
	}
};

template <typename _Tp>
class TmpAlloc /// memory allocator using tmpdir on disk
{
	const char *m_tmpdir;

public:
	typedef       _Tp      value_type;
	typedef       _Tp *          pointer;
	typedef const _Tp *    const_pointer;
	typedef       _Tp &          reference;
	typedef const _Tp &    const_reference;
	typedef size_t         size_type;
	typedef std::ptrdiff_t difference_type;

	template <typename _Tp1>
	struct rebind { typedef TmpAlloc<_Tp1> other; };

	TmpAlloc(const char *tmpdir = 0)  throw() : m_tmpdir(    tmpdir) {}
	TmpAlloc(const TmpAlloc &a)       throw() : m_tmpdir(a.m_tmpdir) {}
	template <typename _Tp1>
	TmpAlloc(const TmpAlloc<_Tp1> &a) throw() : m_tmpdir(a.m_tmpdir) {}

	pointer       address(reference x)       const { return &x; }
	const_pointer address(const_reference x) const { return &x; }

	pointer allocate(size_type n, const void *p = 0)
	{
		if (m_tmpdir == 0) return std::allocator<_Tp>().allocate(n, p);

		if (n > this->max_size()) throw std::bad_alloc();

		std::pair<size_t, int> *info = 0;
		size_t size = FT_ALIGN + sizeof(_Tp) * n;
		int fd = -1;

		if (n <= 4096)
			info = (std::pair<size_t, int> *)(::operator new(size));
		else
		{
			std::string tmpdir = std::string(m_tmpdir) + "/ft2.XXXXXX";

			fd = mkstemp(&tmpdir[0]);
			if (fd < 0 || unlink(&tmpdir[0]) || ftruncate(fd, size)) goto error;
			info = (std::pair<size_t, int> *)mmap(
					0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if ((void *)info == MAP_FAILED) goto error;
		}

		info->first  = size;
		info->second = fd;

		return (_Tp *)((char *)info + FT_ALIGN);

error:
		if (fd >= 0) close(fd);
		throw std::bad_alloc();

		return 0;
	}

	void deallocate(pointer p, size_type n)
	{
		if (m_tmpdir == 0) { std::allocator<_Tp>().deallocate(p, n); return; }

		std::pair<size_t, int> *info =
				(std::pair<size_t, int> *)((char *)p - FT_ALIGN);
		int fd = info->second;

		if (fd < 0)
      ::operator delete((void *)info);
		else
		{
			munmap((void *)info, info->first);
			close(fd);
		}
	}

	size_type max_size() const throw() { return std::allocator<_Tp>().max_size(); }
	void construct(pointer p, const _Tp &val) { std::allocator<_Tp>().construct(p, val); }
	void destroy(pointer p)                   { std::allocator<_Tp>().destroy(p); }

	bool operator ==(const TmpAlloc<_Tp> &a) { return m_tmpdir == a.m_tmpdir; }
	bool operator !=(const TmpAlloc<_Tp> &a) { return m_tmpdir != a.m_tmpdir; }

	template <class _Tp1> friend class TmpAlloc;
};

/* from C++11 type_traits */

template <bool cond, class T, class F> struct Conditional             { typedef F type; };
template            <class T, class F> struct Conditional<true, T, F> { typedef T type; };

template <bool, class T = void> struct EnableIf {};
template <class T>              struct EnableIf<true, T> { typedef T type; };

template <class T1, class T2> struct IsSame       { enum { value = false }; };
template <class T>            struct IsSame<T, T> { enum { value = true  }; };

template <class T> struct RemoveConst          { typedef T type; };
template <class T> struct RemoveConst<const T> { typedef T type; };

/** @brief container which actually hold data
 *
 * Use this class for loading or building FastTrie.
 *
 * @tparam ValueT value type of the container, may be any of following:
 *                -# C primitive types except pointers, e.g.: char, double
 *                -# composition of type 1, e.g.: std::pair<int, float>
 *                -# Vector<ValueT> for a variable-length value, e.g.:
 *                   Vector<char> for a string
 *                -# Trie<KeyT, ValueT> or HashMap<KeyT, ValueT>
 *                   for an associative array, e.g.:
 *                   HashMap<int, Vector<char> > is an int to string mapping
 *                -# Pair<ValueT1, ValueT2> for any composition, e.g.:
 *                   Pair<Trie<int, int>, Vector<char> > is a trie and a string
 */
template <class ValueT>
class Container : public MMap<Container<ValueT> >
{
public:
	/**
	 * std_value_type maps a Container's ValueT to an appropriate STL type.
	 * Basically, it maps Vector to vector, Trie or HashMap to map, Pair to pair,
	 * and others to themselves. Just a template programming facility.
	 */
	typedef ValueT std_value_type;

	typedef       ValueT   value_type;
	typedef const ValueT &       reference;
	typedef const ValueT & const_reference;
	typedef const ValueT *       iterator;
	typedef const ValueT * const_iterator;
	typedef std::reverse_iterator<const ValueT *>       reverse_iterator;
	typedef std::reverse_iterator<const ValueT *> const_reverse_iterator;

	/**
	 * is_primitive tells whether ValueT is C primitive type or composition.
	 */
	enum { is_primitive = true };

	/** @brief init Container with no elements */
	Container() : m_numValues(0), m_values(0) {}
	/** @brief load Container from filename
	 *
	 * @param[in]  filename filename to be loaded \n
	 *                      User must keep the file available during the lifetime.
	 * @param[in]  prot     see mmap(2), e.g.: use PROT_WRITE for a force writing.
	 * @param[in]  flags    see mmap(2), e.g.: use MAP_SHARED | MAP_POPULATE for
	 *                      a reading-ahead on the file.
	 * @throw      int      failed (file not exists, unreadable, etc.)
	 */
	Container(const char *filename, int prot = PROT_READ, int flags = MAP_SHARED);
	/** @brief load Container from memory address
	 *
	 * @param[in]  begin    memory address to be loaded \n
	 *                      User must keep the memory valid during the lifetime.
	 * @param[in]  end      memory ending address to be checked \n
	 *                      If end is 0, won't check.
	 * @throw      int      check failed
	 */
	Container(const void *begin, const void *end = 0);

	/** @brief return the begin iterator of the container */
	const_iterator begin() const { return m_values; }
	/** @brief return the end iterator of the container */
	const_iterator end()   const { return m_values + m_numValues; }
	/** @brief return the reverse begin iterator of the container */
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	/** @brief return the reverse end iterator of the container */
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	/** @brief return whether the size is 0 */
	bool  empty() const { return size() == 0; }
	/** @brief return the size of the container */
	size_t size() const { return m_numValues; }
	/** @brief return the i'th element in the container */
	const_reference operator [](size_t i) const { return m_values[i]; }

	/** @brief build a container
	 *
	 * @tparam OutIteratorT output iterator type, which should be char type
	 * @tparam IteratorT    input iterator type, which should:
	 *                      - for C primitive types or their compositions as
	 *                        ValueT, point to ValueT
	 *                      - for Vector as ValueT, point to std::vector
	 *                      - for Trie or HashMap as ValueT, point to std::map or
	 *                        sorted uniqued std::list<std::pair<key, value> >
	 *                      - for Pair as ValueT, point to std::pair
	 *
	 * @param[out] out      output iterator where data will be written to \n
	 *                      Use ostreambuf_iterator<char>(cout) to write stdout,
	 *                      or something alike to write a file or a string.
	 * @param[in]  begin    begin iterator of values \n
	 *                      NOTICE: For building a Trie (std::map x), begin and
	 *                      end should point to &x and &x + 1, not x.begin() and
	 *                      x.end(). Same is building a Vector from std::vector.
	 * @param[in]  end      end iterator of values
	 * @param[in]  tmpdir   if not null, swap intermediate data on disk in tmpdir
	 */
	template <class OutIteratorT, class IteratorT>
	static OutIteratorT build(OutIteratorT out, IteratorT begin, IteratorT end,
			const char *tmpdir = 0);

	template <class OutIteratorT>
	static OutIteratorT build(OutIteratorT out, const ValueT *begin, const ValueT *end,
			const char *tmpdir = 0);
	template <class OutIteratorT>
	static OutIteratorT build(OutIteratorT out, ValueT *begin, ValueT *end,
			const char *tmpdir = 0)
	{
		return build(out, (const ValueT *)begin, (const ValueT *)end, tmpdir);
	}

private:
	const uint8_t * initPointers(const uint8_t *begin, const uint8_t *end = 0);

	size_t m_numValues;
	const ValueT *m_values;

	template <class V>                                   friend class Container;
	template <class V, class S>                          friend class Vector;
	template <class K, class V, int o, class C, class S> friend class Trie;
	template <class K, class V, class H, class S>        friend class HashMap;
	template <class V1, class V2>                        friend class Pair;
};

template <class ValueT, class SizeT>
class Container<Vector<ValueT, SizeT> >
		: public MMap<Container<Vector<ValueT, SizeT> > >
{
public:
	typedef std::vector<typename Container<ValueT>::std_value_type> std_value_type;

	typedef                 Vector<ValueT, SizeT>   value_type;
	typedef                 Vector<ValueT, SizeT>         reference;
	typedef                 Vector<ValueT, SizeT>   const_reference;
	typedef        Iterator<Vector<ValueT, SizeT> >       iterator;
	typedef        Iterator<Vector<ValueT, SizeT> > const_iterator;
	typedef ReverseIterator<Vector<ValueT, SizeT> >       reverse_iterator;
	typedef ReverseIterator<Vector<ValueT, SizeT> > const_reverse_iterator;

	enum { is_primitive = false };

	Container();
	Container(const char *filename, int prot = PROT_READ, int flags = MAP_SHARED);
	Container(const void *begin, const void *end = 0);

	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end()   const { return const_iterator(this, size()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	bool  empty() const { return size() == 0; }
	size_t size() const { return m_entries.size() - 1; }
	const_reference operator [](size_t i) const { return const_reference(this, i); }

	template <class OutIteratorT, class IteratorT>
	static OutIteratorT build(OutIteratorT out, IteratorT begin, IteratorT end,
			const char *tmpdir = 0);

private:
	const uint8_t * initPointers(const uint8_t *begin, const uint8_t *end = 0);

	Container<SizeT>  m_entries;
	Container<ValueT> m_values;

	static const SizeT m_defaultEntries[1];

	template <class V>                                   friend class Container;
	template <class V, class S>                          friend class Vector;
	template <class K, class V, int o, class C, class S> friend class Trie;
	template <class K, class V, class H, class S>        friend class HashMap;
	template <class V1, class V2>                        friend class Pair;
};

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
class Container<Trie<KeyT, ValueT, option, CharT, SizeT> >
		: public MMap<Container<Trie<KeyT, ValueT, option, CharT, SizeT> > >
{
public:
	typedef std::vector<std::pair<
			typename Container<KeyT  >::std_value_type,
			typename Container<ValueT>::std_value_type> > std_value_type;

	typedef                 Trie<KeyT, ValueT, option, CharT, SizeT>   value_type;
	typedef                 Trie<KeyT, ValueT, option, CharT, SizeT>         reference;
	typedef                 Trie<KeyT, ValueT, option, CharT, SizeT>   const_reference;
	typedef        Iterator<Trie<KeyT, ValueT, option, CharT, SizeT> >       iterator;
	typedef        Iterator<Trie<KeyT, ValueT, option, CharT, SizeT> > const_iterator;
	typedef ReverseIterator<Trie<KeyT, ValueT, option, CharT, SizeT> >       reverse_iterator;
	typedef ReverseIterator<Trie<KeyT, ValueT, option, CharT, SizeT> > const_reverse_iterator;

	enum { is_primitive = false };

	Container();
	Container(const char *filename, int prot = PROT_READ, int flags = MAP_SHARED);
	Container(const void *begin, const void *end = 0);

	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end()   const { return const_iterator(this, size()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	bool  empty() const { return size() == 0; }
	size_t size() const { return m_nodes[0].children; }
	const_reference operator [](size_t i) const { return const_reference(this, i); }

	template <class OutIteratorT, class IteratorT>
	static OutIteratorT build(OutIteratorT out, IteratorT begin, IteratorT end,
			const char *tmpdir = 0);

private:
	struct Less1st
	{
		template <class T>
		bool operator ()(const T &a, const T &b) const
		{
			return std::lexicographical_compare(
					Range<CharT>(a->first).begin, Range<CharT>(a->first).end,
					Range<CharT>(b->first).begin, Range<CharT>(b->first).end);
		}
		template <class ValueT1, class ValueT2>
		bool operator ()(
				const Iterator<Pair<ValueT1, ValueT2> > &a,
				const Iterator<Pair<ValueT1, ValueT2> > &b) const
		{
			ValueT1 a1 = a->first, b1 = b->first;
			return std::lexicographical_compare(
					Range<CharT>(a1).begin, Range<CharT>(a1).end,
					Range<CharT>(b1).begin, Range<CharT>(b1).end);
		}
	};

	struct EqualTo1st
	{
		template <class T>
		bool operator ()(const T &a, const T &b) const { return a->first == b->first; }
	};

	struct Node
	{
		Node()                 : parent(0), children(0) {}
		Node(SizeT p, SizeT c) : parent(p), children(c) {}

		SizeT parent;
		SizeT children;
	};

	template <class IteratorT>
	struct OpenNode
	{
		IteratorT begin;
		IteratorT end;
		uint32_t  level;
		uint32_t  count;
		int32_t   character;
		SizeT     entry;
	};

	enum { FT_MARGIN = (1 << sizeof(CharT) * 8) + 1 };
	enum { FT_MASK = (SizeT)(1) << (sizeof(SizeT) * 8 - 1) };
	enum { CHAR_TERMINATOR = -1 };

	const uint8_t * initPointers(const uint8_t *begin, const uint8_t *end = 0);

	Container<Node>                  m_nodes;
	Container<SizeT>                 m_paths;
	Container<Vector<CharT, SizeT> > m_tails;
	Container<ValueT>                m_values;

	static const Node m_defaultNodes[1];

	/* If FT_PATH specified in option on querying but not on building, try to
	 * build the path container and save it in m_pathString. */
	std::string m_pathString;

	template <class V>                                   friend class Container;
	template <class V, class S>                          friend class Vector;
	template <class K, class V, int o, class C, class S> friend class Trie;
	template <class K, class V, class H, class S>        friend class HashMap;
	template <class V1, class V2>                        friend class Pair;
};

template <class KeyT, class ValueT, class HashT, class SizeT>
class Container<HashMap<KeyT, ValueT, HashT, SizeT> >
		: public MMap<Container<HashMap<KeyT, ValueT, HashT, SizeT> > >
{
public:
	typedef std::vector<std::pair<
			typename Container<KeyT  >::std_value_type,
			typename Container<ValueT>::std_value_type> > std_value_type;

	typedef                 HashMap<KeyT, ValueT, HashT, SizeT>   value_type;
	typedef                 HashMap<KeyT, ValueT, HashT, SizeT>         reference;
	typedef                 HashMap<KeyT, ValueT, HashT, SizeT>   const_reference;
	typedef        Iterator<HashMap<KeyT, ValueT, HashT, SizeT> >       iterator;
	typedef        Iterator<HashMap<KeyT, ValueT, HashT, SizeT> > const_iterator;
	typedef ReverseIterator<HashMap<KeyT, ValueT, HashT, SizeT> >       reverse_iterator;
	typedef ReverseIterator<HashMap<KeyT, ValueT, HashT, SizeT> > const_reverse_iterator;

	enum { is_primitive = false };

	Container() {}
	Container(const char *filename, int prot = PROT_READ, int flags = MAP_SHARED);
	Container(const void *begin, const void *end = 0);

	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end()   const { return const_iterator(this, size()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	bool  empty() const { return size() == 0; }
	size_t size() const { return m_maps.size(); }
	const_reference operator [](size_t i) const { return const_reference(this, i); }

	template <class OutIteratorT, class IteratorT>
	static OutIteratorT build(OutIteratorT out, IteratorT begin, IteratorT end,
			const char *tmpdir = 0);

private:
	struct Less1st
	{
		template <class T>
		bool operator ()(const T &a, const T &b) const { return a->first <  b->first; }
	};

	struct EqualTo1st
	{
		template <class T>
		bool operator ()(const T &a, const T &b) const { return a->first == b->first; }
	};

	struct Less1stObj
	{
		template <class T>
		bool operator ()(const T &a, const T &b) const { return a.first <  b.first; }
	};

	struct EqualTo1stObj
	{
		template <class T>
		bool operator ()(const T &a, const T &b) const { return a.first == b.first; }
	};

	template <class OutIteratorT, class IteratorT>
	static OutIteratorT build(OutIteratorT out, IteratorT begin, IteratorT end,
			const char *tmpdir, char /* !is_primitive */);
	template <class OutIteratorT, class IteratorT>
	static OutIteratorT build(OutIteratorT out, IteratorT begin, IteratorT end,
			const char *tmpdir, long /*  is_primitive */);

	const uint8_t * initPointers(const uint8_t *begin, const uint8_t *end = 0);

	Container<Vector<Vector<Pair<KeyT, ValueT>, SizeT>, SizeT> > m_maps;

	template <class V>                                   friend class Container;
	template <class V, class S>                          friend class Vector;
	template <class K, class V, int o, class C, class S> friend class Trie;
	template <class K, class V, class H, class S>        friend class HashMap;
	template <class V1, class V2>                        friend class Pair;
};

template <class ValueT1, class ValueT2>
class Container<Pair<ValueT1, ValueT2> >
		: public MMap<Container<Pair<ValueT1, ValueT2> > >
{
public:
	typedef std::pair<
			typename Container<ValueT1>::std_value_type,
			typename Container<ValueT2>::std_value_type> std_value_type;

	typedef                 Pair<ValueT1, ValueT2>   value_type;
	typedef                 Pair<ValueT1, ValueT2>         reference;
	typedef                 Pair<ValueT1, ValueT2>   const_reference;
	typedef        Iterator<Pair<ValueT1, ValueT2> >       iterator;
	typedef        Iterator<Pair<ValueT1, ValueT2> > const_iterator;
	typedef ReverseIterator<Pair<ValueT1, ValueT2> >       reverse_iterator;
	typedef ReverseIterator<Pair<ValueT1, ValueT2> > const_reverse_iterator;

	enum { is_primitive = false };

	Container() {}
	Container(const char *filename, int prot = PROT_READ, int flags = MAP_SHARED);
	Container(const void *begin, const void *end = 0);

	const_iterator begin() const { return const_iterator(this, 0); }
	const_iterator end()   const { return const_iterator(this, size()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	bool  empty() const { return size() == 0; }
	size_t size() const { return m_values1.size(); }
	const_reference operator [](size_t i) const { return const_reference(this, i); }

	template <class OutIteratorT, class IteratorT>
	static OutIteratorT build(OutIteratorT out, IteratorT begin, IteratorT end,
			const char *tmpdir = 0);

private:
	const uint8_t * initPointers(const uint8_t *begin, const uint8_t *end = 0);

	Container<ValueT1> m_values1;
	Container<ValueT2> m_values2;

	template <class V>                                   friend class Container;
	template <class V, class S>                          friend class Vector;
	template <class K, class V, int o, class C, class S> friend class Trie;
	template <class K, class V, class H, class S>        friend class HashMap;
	template <class V1, class V2>                        friend class Pair;
};

/** @brief Vector interface to Container
 *
 * @tparam ValueT value type, may be any of following:
 *                -# C primitive types except pointers, e.g.: char, double
 *                -# composition of type 1, e.g.: std::pair<int, float>
 *                -# Vector<ValueT> for a variable-length value, e.g.:
 *                   Vector<char> for a string
 *                -# Trie<KeyT, ValueT> or HashMap<KeyT, ValueT>
 *                   for an associative array, e.g.:
 *                   HashMap<int, Vector<char> > is an int to string mapping
 *                -# Pair<ValueT1, ValueT2> for any composition, e.g.:
 *                   Pair<Trie<int, int>, Vector<char> > is a trie and a string
 * @tparam SizeT  size type, may be uint32_t or uint64_t, corresponding to
 *                32-bits or 64-bits addressing
 */
template <class ValueT, class SizeT = uint32_t>
class Vector
{
public:
	typedef typename Container<ValueT>::value_type             value_type;
	typedef typename Container<ValueT>::      reference              reference;
	typedef typename Container<ValueT>::const_reference        const_reference;
	typedef typename Container<ValueT>::      iterator               iterator;
	typedef typename Container<ValueT>::const_iterator         const_iterator;
	typedef typename Container<ValueT>::      reverse_iterator       reverse_iterator;
	typedef typename Container<ValueT>::const_reverse_iterator const_reverse_iterator;

	Vector() : m_container(&m_defaultContainer), m_i(0) {}

	/** @brief return the begin iterator of the vector */
	const_iterator begin() const
	{
		return m_container->m_values.begin() + m_container->m_entries[m_i];
	}
	/** @brief return the end iterator of the vector */
	const_iterator end() const
	{
		return m_container->m_values.begin() + m_container->m_entries[m_i + 1];
	}
	/** @brief return the reverse begin iterator of the vector */
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	/** @brief return the reverse end iterator of the vector */
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	/** @brief return whether the size is 0 */
	bool  empty() const { return size() == 0; }
	/** @brief return the size of the vector */
	size_t size() const
	{
		return m_container->m_entries[m_i + 1] - m_container->m_entries[m_i];
	}
	/** @brief return the i'th element in the vector */
	const_reference operator [](size_t i) const
	{
		return m_container->m_values[m_container->m_entries[m_i] + i];
	}

	const Vector * operator ->() const { return this; }

	/** @brief to std::basic_string */
	template <class _CharT, class _Traits, class _Alloc>
	operator std::basic_string<_CharT, _Traits, _Alloc>() const
	{
		return std::basic_string<_CharT, _Traits, _Alloc>(begin(), end());
	}

	/** @brief to std::vector */
	template <class _Tp, class _Alloc>
	operator std::vector<_Tp, _Alloc>() const
	{
		return std::vector<_Tp, _Alloc>(begin(), end());
	}

	friend bool operator ==(const Vector &a, const Vector &b)
	{ return a.size() == b.size() &&  std::equal(a.begin(), a.end(), b.begin()); }
	friend bool operator !=(const Vector &a, const Vector &b)
	{ return a.size() != b.size() || !std::equal(a.begin(), a.end(), b.begin()); }
	friend bool operator < (const Vector &a, const Vector &b)
	{ return  std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
	friend bool operator > (const Vector &a, const Vector &b)
	{ return  std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	friend bool operator <=(const Vector &a, const Vector &b)
	{ return !std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	friend bool operator >=(const Vector &a, const Vector &b)
	{ return !std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }

	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator ==(const Vector &a, const std::basic_string<_CharT, _Traits, _Alloc> &b)
	{ return a.size() == b.size() &&  std::equal(a.begin(), a.end(), b.begin()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator !=(const Vector &a, const std::basic_string<_CharT, _Traits, _Alloc> &b)
	{ return a.size() != b.size() || !std::equal(a.begin(), a.end(), b.begin()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator < (const Vector &a, const std::basic_string<_CharT, _Traits, _Alloc> &b)
	{ return  std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator > (const Vector &a, const std::basic_string<_CharT, _Traits, _Alloc> &b)
	{ return  std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator <=(const Vector &a, const std::basic_string<_CharT, _Traits, _Alloc> &b)
	{ return !std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator >=(const Vector &a, const std::basic_string<_CharT, _Traits, _Alloc> &b)
	{ return !std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }

	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator ==(const std::basic_string<_CharT, _Traits, _Alloc> &a, const Vector &b)
	{ return a.size() == b.size() &&  std::equal(a.begin(), a.end(), b.begin()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator !=(const std::basic_string<_CharT, _Traits, _Alloc> &a, const Vector &b)
	{ return a.size() != b.size() || !std::equal(a.begin(), a.end(), b.begin()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator < (const std::basic_string<_CharT, _Traits, _Alloc> &a, const Vector &b)
	{ return  std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator > (const std::basic_string<_CharT, _Traits, _Alloc> &a, const Vector &b)
	{ return  std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator <=(const std::basic_string<_CharT, _Traits, _Alloc> &a, const Vector &b)
	{ return !std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	template <class _CharT, class _Traits, class _Alloc>
	friend bool operator >=(const std::basic_string<_CharT, _Traits, _Alloc> &a, const Vector &b)
	{ return !std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }

	template <class _Tp, class _Alloc>
	friend bool operator ==(const Vector &a, const std::vector<_Tp, _Alloc> &b)
	{ return a.size() == b.size() &&  std::equal(a.begin(), a.end(), b.begin()); }
	template <class _Tp, class _Alloc>
	friend bool operator !=(const Vector &a, const std::vector<_Tp, _Alloc> &b)
	{ return a.size() != b.size() || !std::equal(a.begin(), a.end(), b.begin()); }
	template <class _Tp, class _Alloc>
	friend bool operator < (const Vector &a, const std::vector<_Tp, _Alloc> &b)
	{ return  std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
	template <class _Tp, class _Alloc>
	friend bool operator > (const Vector &a, const std::vector<_Tp, _Alloc> &b)
	{ return  std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	template <class _Tp, class _Alloc>
	friend bool operator <=(const Vector &a, const std::vector<_Tp, _Alloc> &b)
	{ return !std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	template <class _Tp, class _Alloc>
	friend bool operator >=(const Vector &a, const std::vector<_Tp, _Alloc> &b)
	{ return !std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }

	template <class _Tp, class _Alloc>
	friend bool operator ==(const std::vector<_Tp, _Alloc> &a, const Vector &b)
	{ return a.size() == b.size() &&  std::equal(a.begin(), a.end(), b.begin()); }
	template <class _Tp, class _Alloc>
	friend bool operator !=(const std::vector<_Tp, _Alloc> &a, const Vector &b)
	{ return a.size() != b.size() || !std::equal(a.begin(), a.end(), b.begin()); }
	template <class _Tp, class _Alloc>
	friend bool operator < (const std::vector<_Tp, _Alloc> &a, const Vector &b)
	{ return  std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }
	template <class _Tp, class _Alloc>
	friend bool operator > (const std::vector<_Tp, _Alloc> &a, const Vector &b)
	{ return  std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	template <class _Tp, class _Alloc>
	friend bool operator <=(const std::vector<_Tp, _Alloc> &a, const Vector &b)
	{ return !std::lexicographical_compare(b.begin(), b.end(), a.begin(), a.end()); }
	template <class _Tp, class _Alloc>
	friend bool operator >=(const std::vector<_Tp, _Alloc> &a, const Vector &b)
	{ return !std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }

	friend bool operator ==(const Vector &a, const char *b)
	{ return  std::equal(a.begin(), a.end(), b) && !b[a.size()]; }
	friend bool operator !=(const Vector &a, const char *b)
	{ return !std::equal(a.begin(), a.end(), b) ||  b[a.size()]; }
	friend bool operator < (const Vector &a, const char *b)
	{ return  std::lexicographical_compare(a.begin(), a.end(), b, b + strlen(b)); }
	friend bool operator > (const Vector &a, const char *b)
	{ return  std::lexicographical_compare(b, b + strlen(b), a.begin(), a.end()); }
	friend bool operator <=(const Vector &a, const char *b)
	{ return !std::lexicographical_compare(b, b + strlen(b), a.begin(), a.end()); }
	friend bool operator >=(const Vector &a, const char *b)
	{ return !std::lexicographical_compare(a.begin(), a.end(), b, b + strlen(b)); }

	friend bool operator ==(const char *a, const Vector &b)
	{ return  std::equal(b.begin(), b.end(), a) && !a[b.size()]; }
	friend bool operator !=(const char *a, const Vector &b)
	{ return !std::equal(b.begin(), b.end(), a) ||  a[b.size()]; }
	friend bool operator < (const char *a, const Vector &b)
	{ return  std::lexicographical_compare(a, a + strlen(a), b.begin(), b.end()); }
	friend bool operator > (const char *a, const Vector &b)
	{ return  std::lexicographical_compare(b.begin(), b.end(), a, a + strlen(a)); }
	friend bool operator <=(const char *a, const Vector &b)
	{ return !std::lexicographical_compare(b.begin(), b.end(), a, a + strlen(a)); }
	friend bool operator >=(const char *a, const Vector &b)
	{ return !std::lexicographical_compare(a, a + strlen(a), b.begin(), b.end()); }

private:
	Vector(const Container<Vector<ValueT, SizeT> > *container, size_t i)
			: m_container(container), m_i(i) {}

	static const Container<Vector<ValueT, SizeT> > defaultContainer();

	/* Vector is actually a pointer to the m_i'th element in m_container. */
	const Container<Vector<ValueT, SizeT> > *m_container;
	size_t m_i;

	/* The default value Vector() points to m_defaultContainer. */
	static const Container<Vector<ValueT, SizeT> > m_defaultContainer;

	friend class Container      <Vector<ValueT, SizeT> >;
	friend class Iterator       <Vector<ValueT, SizeT> >;
	friend class ReverseIterator<Vector<ValueT, SizeT> >;
};

typedef Vector<char>               String;
typedef Vector<char, uint64_t> LongString;

/** @brief Trie interface to Container
 *
 * @tparam KeyT   key type, may be any of following:
 *                -# C primitive types except pointers, e.g.: char, double
 *                -# composition of type 1, e.g.: std::pair<int, float>
 *                -# Vector<ValueT> for a variable-length key, e.g.:
 *                   Vector<char> for a string
 * @tparam ValueT value type, may be any of following:
 *                -# C primitive types except pointers, e.g.: char, double
 *                -# composition of type 1, e.g.: std::pair<int, float>
 *                -# Vector<ValueT> for a variable-length value, e.g.:
 *                   Vector<char> for a string
 *                -# Trie<KeyT, ValueT> or HashMap<KeyT, ValueT>
 *                   for an associative array, e.g.:
 *                   HashMap<int, Vector<char> > is an int to string mapping
 *                -# Pair<ValueT1, ValueT2> for any composition, e.g.:
 *                   Pair<Trie<int, int>, Vector<char> > is a trie and a string
 * @tparam option structure option, may be bitwise-or'd of following:
 *                - FT_NOTAIL: do not saves trie tails isolatedly, degrades
 *                  performance in most cases
 *                - FT_PATH: saves or loads trie paths, for extracting the key
 *                  from an iterator, cost more time on loading or more space
 *                - FT_QUICKBUILD: quick build at the cost of a larger output
 * @tparam CharT  character type, may be uint8_t or uint16_t, corresponding to
 *                256-branches or 65536-branches trie
 * @tparam SizeT  size type, may be uint32_t or uint64_t, corresponding to
 *                32-bits or 64-bits addressing
 */
template <class KeyT, class ValueT,
		int option = 0, class CharT = uint8_t, class SizeT = uint32_t>
class Trie
{
public:
	typedef typename Container<ValueT>::value_type             value_type;
	typedef typename Container<ValueT>::      reference              reference;
	typedef typename Container<ValueT>::const_reference        const_reference;
	typedef typename Container<ValueT>::      iterator               iterator;
	typedef typename Container<ValueT>::const_iterator         const_iterator;
	typedef typename Container<ValueT>::      reverse_iterator       reverse_iterator;
	typedef typename Container<ValueT>::const_reverse_iterator const_reverse_iterator;

	typedef KeyT key_type;

	Trie() : m_container(&m_defaultContainer), m_i(0) {}

	/** @brief return the begin iterator of the trie */
	const_iterator begin() const
	{
		return m_container->m_values.begin()
				+ (m_container->m_nodes[m_i + 1].parent & ~FT_MASK);
	}
	/** @brief return the end iterator of the trie */
	const_iterator end() const
	{
		return m_container->m_values.begin()
				+ (m_container->m_nodes[m_i + 2].parent & ~FT_MASK);
	}
	/** @brief return the reverse begin iterator of the trie */
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	/** @brief return the reverse end iterator of the trie */
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	/** @brief return whether the size is 0 */
	bool  empty() const { return size() == 0; }
	/** @brief return the size of the trie */
	size_t size() const
	{
		return m_container->m_nodes[m_i + 2].parent
				-  m_container->m_nodes[m_i + 1].parent;
	}
	/** @brief get the value of a key in trie, same as operator () */
	template <class AnyKeyT>
	const_reference operator [](const AnyKeyT &key) const
	{
		return operator ()(key);
	}

	const Trie * operator ->() const { return this; }

	/** @brief match a key in trie
	 *
	 * @param[in]  keyBegin begin of the key
	 * @param[in]  keyEnd   end of the key
	 * @param[out] value    value of the key \n
	 *                      If value is 0 or no match, no value will be written.
	 * @return              number of matches
	 */
	uint32_t match(const CharT *keyBegin, const CharT *keyEnd,
			ValueT *value = 0) const;
	/** @brief match all keys in trie, fixed beginning, variable ending
	 *
	 * @param[in]  keyBegin begin of possible keys
	 * @param[in]  keyEnd   maximum end of possible keys
	 * @param[in]  maxMatches maximum number of matches returned
	 * @param[out] ranges   ranges of matched keys \n
	 *                      If ranges is 0 or no match, no range will be written.
	 * @param[out] values   values of matched keys \n
	 *                      If values is 0 or no match, no value will be written.
	 * @return              number of matches
	 */
	uint32_t matchBeginning(const CharT *keyBegin, const CharT *keyEnd,
			uint32_t maxMatches = (uint32_t)(-1),
			Range<CharT> *ranges = 0, value_type *values = 0) const;
	/** @brief match all keys in trie, variable beginning, variable ending
	 *
	 * @param[in]  keyBegin minimum begin of possible keys
	 * @param[in]  keyEnd   maximum end of possible keys
	 * @param[in]  maxMatches maximum number of matches returned
	 * @param[out] ranges   ranges of matched keys \n
	 *                      If ranges is 0 or no match, no range will be written.
	 * @param[out] values   values of matched keys \n
	 *                      If values is 0 or no match, no value will be written.
	 * @param[in]  step     iterating step of the beginning of possible keys \n
	 *                      E.g. key = "abcdefg" and step = 2, possible keys are
	 *                      "a...", "c...", "e..." and "g...".
	 * @return              number of matches
	 */
	uint32_t matchAll(const CharT *keyBegin, const CharT *keyEnd,
			uint32_t maxMatches = (uint32_t)(-1),
			Range<CharT> *ranges = 0, value_type *values = 0, uint32_t step = 1) const;

	/** @brief locate the value of a key in trie
	 *
	 * @param[in]  keyBegin begin of the key
	 * @param[in]  keyEnd   end of the key
	 * @return              iterator to the value of the key \n
	 *                      If no match, end() will be returned.
	 *
	 * Similar with match(keyBegin, keyEnd, value) const, but return the iterator
	 * instead of number of matches.
	 */
	const_iterator find(const CharT *keyBegin, const CharT *keyEnd) const;

	/** @brief locate the value of a key in trie
	 *
	 * @param[in]  key      the key
	 * @return              iterator to the value of the key \n
	 *                      If no match, end() will be returned.
	 */
	const_iterator find(const KeyT &key) const
	{
		return find((CharT *)range(key).begin, (CharT *)range(key).end);
	}

	/* for Vector as KeyT: string, vector or * are acceptable */
	template <class AnyKeyT>
	typename EnableIf<
			HasVariableRange<KeyT>::value && HasVariableRange<AnyKeyT>::value,
			const_iterator>::type
	find(const AnyKeyT &key) const
	{
		return find((CharT *)range(key).begin, (CharT *)range(key).end);
	}

	/** @brief get the value of a key in trie
	 *
	 * @param[in]  keyBegin begin of the key
	 * @param[in]  keyEnd   end of the key
	 * @return              value of the key \n
	 *                      If no match, ValueT() will be returned.
	 *
	 * Similar with match(keyBegin, keyEnd, value) const, but return the value
	 * instead of number of matches.
	 */
	const_reference operator ()(const CharT *keyBegin, const CharT *keyEnd) const;

	/** @brief get the value of a key in trie
	 *
	 * @param[in]  key      the key
	 * @return              value of the key \n
	 *                      If no match, ValueT() will be returned.
	 */
	const_reference operator ()(const KeyT &key) const
	{
		return operator ()((CharT *)range(key).begin, (CharT *)range(key).end);
	}

	/* for Vector as KeyT: string, vector or * are acceptable */
	template <class AnyKeyT>
	typename EnableIf<
			HasVariableRange<KeyT>::value && HasVariableRange<AnyKeyT>::value,
			const_reference>::type
	operator ()(const AnyKeyT &key) const
	{
		return operator ()((CharT *)range(key).begin, (CharT *)range(key).end);
	}

	/** @brief get the key of an iterator
	 *
	 * @tparam KeyT   key type, same as in operator ()(key) const
	 *
	 * @param[in]  it       the iterator
	 * @return              key of the iterator \n
	 *                      If FT_PATH not specified in option, neither during
	 *                      building or querying, key will be unextractable.
	 *                      Under this circumstance, KeyT() will be returned.
	 */
	template <class StdKeyT>
	const StdKeyT key(const_iterator it) const;

	/** @brief to std::map */
	template <class _Key, class _Tp, class _Compare, class _Alloc>
	operator std::map<_Key, _Tp, _Compare, _Alloc>() const
	{
		typedef typename std::map<_Key, _Tp, _Compare, _Alloc>::key_type key_type;

		std::map<_Key, _Tp, _Compare, _Alloc> result;
		for (const_iterator it = begin(); it != end(); ++ it)
			result.insert(std::make_pair(key<key_type>(it), *it));
		return result;
	}

	/** @brief to std::set */
	template <class _Key, class _Compare, class _Alloc>
	operator std::set<_Key, _Compare, _Alloc>() const
	{
		typedef typename std::set<_Key, _Compare, _Alloc>::key_type key_type;

		std::set<_Key, _Compare, _Alloc> result;
		for (const_iterator it = begin(); it != end(); ++ it)
			result.insert(key<key_type>(it));
		return result;
	}

private:
	/* an partial specialized template for fetching variable key type */
	template <class T>
	struct Memory
	{
		static bool resize(T &x, size_t size) { return sizeof(T) == size; }
		static void *begin(T &x) { return &x; }
		static void *end  (T &x) { return &x + 1; }
	};

	template <class _CharT, class _Traits, class _Alloc>
	struct Memory<std::basic_string<_CharT, _Traits, _Alloc> >
	{
		typedef std::basic_string<_CharT, _Traits, _Alloc> T;

		static bool resize(T &x, size_t size)
		{
			if (size % sizeof(_CharT)) return false;
			x.resize(size / sizeof(_CharT)); return true;
		}
		static void *begin(T &x) { return &*x.begin(); }
		static void *end  (T &x) { return &*x.end(); }
	};

	template <class _Tp, class _Alloc>
	struct Memory<std::vector<_Tp, _Alloc> >
	{
		typedef std::vector<_Tp, _Alloc> T;

		static bool resize(T &x, size_t size)
		{
			if (size % sizeof(_Tp)) return false;
			x.resize(size / sizeof(_Tp)); return true;
		}
		static void *begin(T &x) { return &*x.begin(); }
		static void *end  (T &x) { return &*x.end(); }
	};

	typedef typename Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::Node Node;

	enum { FT_MASK = Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::FT_MASK };
	enum { CHAR_TERMINATOR = -1 };

	Trie(const Container<Trie<KeyT, ValueT, option, CharT, SizeT> > *container, size_t i)
			: m_container(container), m_i(i) {}

	static const Container<Trie<KeyT, ValueT, option, CharT, SizeT> > defaultContainer();

	/* Trie is actually a pointer to the m_i'th element in m_container. */
	const Container<Trie<KeyT, ValueT, option, CharT, SizeT> > *m_container;
	size_t m_i;

	/* The default value Trie() points to m_defaultContainer. */
	static const Container<Trie<KeyT, ValueT, option, CharT, SizeT> > m_defaultContainer;

	/* If no match, m_zero = ValueT() will be return. */
	static const ValueT m_zero;

	friend class Container      <Trie<KeyT, ValueT, option, CharT, SizeT> >;
	friend class Iterator       <Trie<KeyT, ValueT, option, CharT, SizeT> >;
	friend class ReverseIterator<Trie<KeyT, ValueT, option, CharT, SizeT> >;
};

/** @brief HashMap interface to Container
 *
 * @tparam KeyT   key type, may be any of following:
 *                -# C primitive types except pointers, e.g.: char, double
 *                -# composition of type 1, e.g.: std::pair<int, float>
 *                -# Vector<ValueT> for a variable-length key, e.g.:
 *                   Vector<char> for a string
 * @tparam ValueT value type, may be any of following:
 *                -# C primitive types except pointers, e.g.: char, double
 *                -# composition of type 1, e.g.: std::pair<int, float>
 *                -# Vector<ValueT> for a variable-length value, e.g.:
 *                   Vector<char> for a string
 *                -# Trie<KeyT, ValueT> or HashMap<KeyT, ValueT>
 *                   for an associative array, e.g.:
 *                   HashMap<int, Vector<char> > is an int to string mapping
 *                -# Pair<ValueT1, ValueT2> for any composition, e.g.:
 *                   Pair<Trie<int, int>, Vector<char> > is a trie and a string
 * @tparam HashT  hash function type
 * @tparam SizeT  size type, may be uint32_t or uint64_t, corresponding to
 *                32-bits or 64-bits addressing
 */
template <class KeyT, class ValueT,
		class HashT = MulAddHash, class SizeT = uint32_t>
class HashMap
{
public:
	typedef typename Container<Pair<KeyT, ValueT> >::value_type             value_type;
	typedef typename Container<Pair<KeyT, ValueT> >::      reference              reference;
	typedef typename Container<Pair<KeyT, ValueT> >::const_reference        const_reference;
	typedef typename Container<Pair<KeyT, ValueT> >::      iterator               iterator;
	typedef typename Container<Pair<KeyT, ValueT> >::const_iterator         const_iterator;
	typedef typename Container<Pair<KeyT, ValueT> >::      reverse_iterator       reverse_iterator;
	typedef typename Container<Pair<KeyT, ValueT> >::const_reverse_iterator const_reverse_iterator;

	typedef KeyT key_type;

	HashMap() : m_container(&m_defaultContainer), m_i(0) {}

	/** @brief return the begin iterator of the hash map */
	const_iterator begin() const
	{
		return m_container->m_maps[m_i].begin()->begin();
	}
	/** @brief return the end iterator of the hash map */
	const_iterator end() const
	{
		return m_container->m_maps[m_i].end()->begin();
	}
	/** @brief return the reverse begin iterator of the hash map */
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	/** @brief return the reverse end iterator of the hash map */
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	/** @brief return whether the size is 0 */
	bool  empty() const { return size() == 0; }
	/** @brief return the size of the hash map */
	size_t size() const
	{
		return end() - begin();
	}
	/** @brief get the value of a key in hash map, same as operator () */
	template <class AnyKeyT>
	typename Container<ValueT>::const_reference operator [](const AnyKeyT &key) const
	{
		return operator ()(key);
	}

	const HashMap * operator ->() const { return this; }

	/** @brief locate a key in hash map
	 *
	 * @param[in]  key      the key
	 * @return              iterator to the key-value Pair \n
	 *                      If no match, end() will be returned.
	 */
	inline const_iterator find(const KeyT &key) const;

	/* for Vector as KeyT: string, vector or * are acceptable */
	template <class AnyKeyT>
	typename EnableIf<
			HasVariableRange<KeyT>::value && HasVariableRange<AnyKeyT>::value,
			const_iterator>::type
	find(const AnyKeyT &key) const
	{
		return find(key, range(key));
	}

	/** @brief get the value of a key in hash map
	 *
	 * @param[in]  key      the key
	 * @return              value of the key \n
	 *                      If no match, ValueT() will be returned.
	 */
	inline typename Container<ValueT>::const_reference
	operator ()(const KeyT &key) const;

	/* for Vector as KeyT: string, vector or * are acceptable */
	template <class AnyKeyT>
	typename EnableIf<
			HasVariableRange<KeyT>::value && HasVariableRange<AnyKeyT>::value,
			typename Container<ValueT>::const_reference>::type
	operator ()(const AnyKeyT &key) const
	{
		return operator ()(key, range(key));
	}

	/** @brief to std::map */
	template <class _Key, class _Tp, class _Compare, class _Alloc>
	operator std::map<_Key, _Tp, _Compare, _Alloc>() const
	{
		return std::map<_Key, _Tp, _Compare, _Alloc>(begin(), end());
	}

	/** @brief to std::set */
	template <class _Key, class _Compare, class _Alloc>
	operator std::set<_Key, _Compare, _Alloc>() const
	{
		std::set<_Key, _Compare, _Alloc> result;
		for (const_iterator it = begin(); it != end(); ++ it)
			result.insert(it->first);
		return result;
	}

private:
	HashMap(const Container<HashMap<KeyT, ValueT, HashT, SizeT> > *container, size_t i)
			: m_container(container), m_i(i) {}

	template <class StdKeyT, class T>
	inline const_iterator find(const StdKeyT &key, Range<T> r) const;

	template <class StdKeyT, class T>
	inline typename Container<ValueT>::const_reference
	operator ()(const StdKeyT &key, Range<T> r) const;

	static const Container<HashMap<KeyT, ValueT, HashT, SizeT> > defaultContainer();

	/* HashMap is actually a pointer to the m_i'th element in m_container. */
	const Container<HashMap<KeyT, ValueT, HashT, SizeT> > *m_container;
	size_t m_i;

	/* The default value HashMap() points to m_defaultContainer. */
	static const Container<HashMap<KeyT, ValueT, HashT, SizeT> > m_defaultContainer;

	/* If no match, m_zero = ValueT() will be return. */
	static const ValueT m_zero;

	friend class Container      <HashMap<KeyT, ValueT, HashT, SizeT> >;
	friend class Iterator       <HashMap<KeyT, ValueT, HashT, SizeT> >;
	friend class ReverseIterator<HashMap<KeyT, ValueT, HashT, SizeT> >;
};

/** @brief Pair interface to Container
 *
 * @tparam ValueT1 first value type, may be any of following:
 *                -# C primitive types except pointers, e.g.: char, double
 *                -# composition of type 1, e.g.: std::pair<int, float>
 *                -# Vector<ValueT> for a variable-length value, e.g.:
 *                   Vector<char> for a string
 *                -# Trie<KeyT, ValueT> or HashMap<KeyT, ValueT>
 *                   for an associative array, e.g.:
 *                   HashMap<int, Vector<char> > is an int to string mapping
 *                -# Pair<ValueT1, ValueT2> for any composition, e.g.:
 *                   Pair<Trie<int, int>, Vector<char> > is a trie and a string
 * @tparam ValueT2 second value type
 */
template <class ValueT1, class ValueT2>
class Pair
{
public:
	typedef ValueT1 first_type;
	typedef ValueT2 second_type;

	typename Container<ValueT1>::const_reference first;
	typename Container<ValueT2>::const_reference second;

	Pair(
			typename Container<ValueT1>::const_reference f = m_defaultFirst,
			typename Container<ValueT2>::const_reference s = m_defaultSecond)
			: first(f), second(s) {}

	const Pair * operator ->() const { return this; }

	const Pair & operator =(const Pair &x)
	{
		if (&x != this) { this->Pair::~Pair(); new (this) Pair(x); } return *this;
	}

	/** @brief to std::pair */
	template <class _T1, class _T2>
	operator std::pair<_T1, _T2>() const
	{
		return std::pair<_T1, _T2>(first, second);
	}

	friend bool operator ==(const Pair &a, const Pair &b)
	{ return a.first == b.first && a.second == b.second; }
	friend bool operator !=(const Pair &a, const Pair &b)
	{ return a.first != b.first || a.second != b.second; }
	friend bool operator < (const Pair &a, const Pair &b)
	{ return a.first <  b.first || (a.first == b.first && a.second <  b.second); }
	friend bool operator > (const Pair &a, const Pair &b)
	{ return a.first >  b.first || (a.first == b.first && a.second >  b.second); }
	friend bool operator <=(const Pair &a, const Pair &b)
	{ return a.first <= b.first || (a.first == b.first && a.second <= b.second); }
	friend bool operator >=(const Pair &a, const Pair &b)
	{ return a.first >= b.first || (a.first == b.first && a.second >= b.second); }

  template<class _T1, class _T2>
	friend bool operator ==(const Pair &a, const std::pair<_T1, _T2> &b)
	{ return a.first == b.first && a.second == b.second; }
  template<class _T1, class _T2>
	friend bool operator !=(const Pair &a, const std::pair<_T1, _T2> &b)
	{ return a.first != b.first || a.second != b.second; }
  template<class _T1, class _T2>
	friend bool operator < (const Pair &a, const std::pair<_T1, _T2> &b)
	{ return a.first <  b.first || (a.first == b.first && a.second <  b.second); }
  template<class _T1, class _T2>
	friend bool operator > (const Pair &a, const std::pair<_T1, _T2> &b)
	{ return a.first >  b.first || (a.first == b.first && a.second >  b.second); }
  template<class _T1, class _T2>
	friend bool operator <=(const Pair &a, const std::pair<_T1, _T2> &b)
	{ return a.first <= b.first || (a.first == b.first && a.second <= b.second); }
  template<class _T1, class _T2>
	friend bool operator >=(const Pair &a, const std::pair<_T1, _T2> &b)
	{ return a.first >= b.first || (a.first == b.first && a.second >= b.second); }

  template<class _T1, class _T2>
	friend bool operator ==(const std::pair<_T1, _T2> &a, const Pair &b)
	{ return a.first == b.first && a.second == b.second; }
  template<class _T1, class _T2>
	friend bool operator !=(const std::pair<_T1, _T2> &a, const Pair &b)
	{ return a.first != b.first || a.second != b.second; }
  template<class _T1, class _T2>
	friend bool operator < (const std::pair<_T1, _T2> &a, const Pair &b)
	{ return a.first <  b.first || (a.first == b.first && a.second <  b.second); }
  template<class _T1, class _T2>
	friend bool operator > (const std::pair<_T1, _T2> &a, const Pair &b)
	{ return a.first >  b.first || (a.first == b.first && a.second >  b.second); }
  template<class _T1, class _T2>
	friend bool operator <=(const std::pair<_T1, _T2> &a, const Pair &b)
	{ return a.first <= b.first || (a.first == b.first && a.second <= b.second); }
  template<class _T1, class _T2>
	friend bool operator >=(const std::pair<_T1, _T2> &a, const Pair &b)
	{ return a.first >= b.first || (a.first == b.first && a.second >= b.second); }

private:
	Pair(const Container<Pair<ValueT1, ValueT2> > *container, size_t i)
			: first(container->m_values1[i]), second(container->m_values2[i]) {}

	static const ValueT1 m_defaultFirst;
	static const ValueT2 m_defaultSecond;

	friend class Container      <Pair<ValueT1, ValueT2> >;
	friend class Iterator       <Pair<ValueT1, ValueT2> >;
	friend class ReverseIterator<Pair<ValueT1, ValueT2> >;
};

template <class ValueT>
class Iterator
{
public:
	typedef std::random_access_iterator_tag iterator_category;
	typedef ValueT value_type;
	typedef size_t difference_type;
	typedef ValueT pointer;
	typedef ValueT reference;

	Iterator() : m_container(0), m_i(0) {}

	explicit Iterator(const ReverseIterator<ValueT> &r)
			: m_container(r.m_container), m_i(r.m_i) {}

	const Iterator & base() const { return *this; }

	reference operator * ()         const { return ValueT(m_container, m_i); }
	pointer   operator ->()         const { return ValueT(m_container, m_i); }
	reference operator [](size_t i) const { return ValueT(m_container, m_i + i); }

	Iterator & operator ++()    { ++ m_i; return *this; }
	Iterator   operator ++(int) { return Iterator(m_container, m_i ++); }
	Iterator & operator --()    { -- m_i; return *this; }
	Iterator   operator --(int) { return Iterator(m_container, m_i --); }
	Iterator & operator +=(size_t i)       { m_i += i; return *this; }
	Iterator   operator + (size_t i) const { return Iterator(m_container, m_i + i); }
	Iterator & operator -=(size_t i)       { m_i -= i; return *this; }
	Iterator   operator - (size_t i) const { return Iterator(m_container, m_i - i); }

	friend bool   operator ==(const Iterator &a, const Iterator &b)
	{ return a.m_i == b.m_i && a.m_container == b.m_container; }
	friend bool   operator !=(const Iterator &a, const Iterator &b)
	{ return a.m_i != b.m_i || a.m_container != b.m_container; }
	friend bool   operator < (const Iterator &a, const Iterator &b) { return a.m_i <  b.m_i; }
	friend bool   operator > (const Iterator &a, const Iterator &b) { return a.m_i >  b.m_i; }
	friend bool   operator <=(const Iterator &a, const Iterator &b) { return a.m_i <= b.m_i; }
	friend bool   operator >=(const Iterator &a, const Iterator &b) { return a.m_i >= b.m_i; }
	friend Iterator        operator + (size_t i, const Iterator &b) { return b + i; }
	friend difference_type operator - (const Iterator &a, const Iterator &b) { return a.m_i -  b.m_i; }

private:
	Iterator(const Container<ValueT> *container, size_t i)
			: m_container(container), m_i(i) {}

	/* Iterator is a pointer to the m_i'th element in m_container. */
	const Container<ValueT> *m_container;
	size_t m_i;

	friend class Container      <ValueT>;
	friend class ReverseIterator<ValueT>;

	/* HashMap need return conference to value */
	template <class K, class V, class H, class S> friend class HashMap;
};

template <class ValueT>
class ReverseIterator
{
public:
	typedef std::random_access_iterator_tag iterator_category;
	typedef ValueT value_type;
	typedef size_t difference_type;
	typedef ValueT pointer;
	typedef ValueT reference;

	ReverseIterator() : m_container(0), m_i(0) {}

	explicit ReverseIterator(const Iterator<ValueT> &i)
			: m_container(i.m_container), m_i(i.m_i) {}

	Iterator<ValueT> base() const { return Iterator<ValueT>(*this); }

	reference operator * ()         const { return ValueT(m_container, m_i - 1); }
	pointer   operator ->()         const { return ValueT(m_container, m_i - 1); }
	reference operator [](size_t i) const { return ValueT(m_container, m_i - 1 - i); }

	ReverseIterator & operator ++()    { -- m_i; return *this; }
	ReverseIterator   operator ++(int) { return ReverseIterator(m_container, m_i --); }
	ReverseIterator & operator --()    { ++ m_i; return *this; }
	ReverseIterator   operator --(int) { return ReverseIterator(m_container, m_i ++); }
	ReverseIterator & operator +=(size_t i)       { m_i -= i; return *this; }
	ReverseIterator   operator + (size_t i) const { return ReverseIterator(m_container, m_i - i); }
	ReverseIterator & operator -=(size_t i)       { m_i += i; return *this; }
	ReverseIterator   operator - (size_t i) const { return ReverseIterator(m_container, m_i + i); }

	friend bool   operator ==(const ReverseIterator &a, const ReverseIterator &b)
	{ return a.m_i == b.m_i && a.m_container == b.m_container; }
	friend bool   operator !=(const ReverseIterator &a, const ReverseIterator &b)
	{ return a.m_i != b.m_i || a.m_container != b.m_container; }
	friend bool   operator < (const ReverseIterator &a, const ReverseIterator &b) { return b.m_i <  a.m_i; }
	friend bool   operator > (const ReverseIterator &a, const ReverseIterator &b) { return b.m_i >  a.m_i; }
	friend bool   operator <=(const ReverseIterator &a, const ReverseIterator &b) { return b.m_i <= a.m_i; }
	friend bool   operator >=(const ReverseIterator &a, const ReverseIterator &b) { return b.m_i >= a.m_i; }
	friend ReverseIterator operator + (size_t i, const ReverseIterator &b) { return b + i; }
	friend difference_type operator - (const ReverseIterator &a, const ReverseIterator &b) { return b.m_i -  a.m_i; }

private:
	ReverseIterator(const Container<ValueT> *container, size_t i)
			: m_container(container), m_i(i) {}

	/* ReverseIterator is a pointer to the m_i'th element in m_container. */
	const Container<ValueT> *m_container;
	size_t m_i;

	friend class Container<ValueT>;
	friend class Iterator <ValueT>;
};

template <class ValueT>
Container<ValueT>::Container(const char *filename, int prot, int flags)
		: MMap<Container<ValueT> >(filename, prot, flags)
{
	if (filename[0] == 0) throw int(-1);

	const uint8_t *begin = (uint8_t *)this->mmap().first;
	const uint8_t *end = begin + this->mmap().second;

	if (initPointers(begin, end) != end) throw int(-1);
}

template <class ValueT>
Container<ValueT>::Container(const void *begin, const void *end)
{
	if (initPointers((uint8_t *)begin, (uint8_t *)end)
			!= (uint8_t *)end && end) throw int(-1);
}

template <class ValueT>
const uint8_t * Container<ValueT>::initPointers(
		const uint8_t *begin, const uint8_t *end)
{
	if (end && end < begin + FT_ALIGN) return 0;

	m_numValues = ((size_t *)begin)[0];
	m_values = (ValueT *)(begin + FT_ALIGN);

	size_t bytes = sizeof(ValueT) * m_numValues;
	bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN;

	if (end && end < begin + FT_ALIGN + bytes) return 0;

	return begin + FT_ALIGN + bytes;
}

template <class ValueT, class SizeT>
Container<Vector<ValueT, SizeT> >::Container()
{
	m_entries.m_numValues = 1;
	m_entries.m_values = m_defaultEntries;
}

template <class ValueT, class SizeT>
Container<Vector<ValueT, SizeT> >::
Container(const char *filename, int prot, int flags)
		: MMap<Container<Vector<ValueT, SizeT> > >(filename, prot, flags)
{
	if (filename[0] == 0) throw int(-1);

	const uint8_t *begin = (uint8_t *)this->mmap().first;
	const uint8_t *end = begin + this->mmap().second;

	if (initPointers(begin, end) != end) throw int(-1);
}

template <class ValueT, class SizeT>
Container<Vector<ValueT, SizeT> >::Container(
		const void *begin, const void *end)
{
	if (initPointers((uint8_t *)begin, (uint8_t *)end)
			!= (uint8_t *)end && end) throw int(-1);
}

template <class ValueT, class SizeT>
const uint8_t * Container<Vector<ValueT, SizeT> >::initPointers(
		const uint8_t *begin, const uint8_t *end)
{
	begin = m_entries.initPointers(begin, end); if (!begin) return 0;
	begin = m_values .initPointers(begin, end); if (!begin) return 0;

	return begin;
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::Container()
{
	m_nodes.m_numValues = 1;
	m_nodes.m_values = m_defaultNodes;
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::
Container(const char *filename, int prot, int flags)
		: MMap<Container<Trie<KeyT, ValueT, option, CharT, SizeT> > >(filename, prot, flags)
{
	if (filename[0] == 0) throw int(-1);

	const uint8_t *begin = (uint8_t *)this->mmap().first;
	const uint8_t *end = begin + this->mmap().second;

	if (initPointers(begin, end) != end) throw int(-1);
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::Container(
		const void *begin, const void *end)
{
	if (initPointers((uint8_t *)begin, (uint8_t *)end)
			!= (uint8_t *)end && end) throw int(-1);
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
const uint8_t * Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::initPointers(
		const uint8_t *begin, const uint8_t *end)
{
	begin = m_nodes .initPointers(begin, end); if (!begin) return 0;
	begin = m_paths .initPointers(begin, end); if (!begin) return 0;
	begin = m_tails .initPointers(begin, end); if (!begin) return 0;
	begin = m_values.initPointers(begin, end); if (!begin) return 0;

	if ((option & FT_PATH) && m_paths.size() < m_values.size())
	{
		m_pathString.clear();

		std::vector<SizeT> paths(m_values.size());

		for (size_t i = size() + 2; i != m_nodes.size(); i ++)
		{
			if (m_nodes[i].parent
					&& m_nodes[i].parent < m_nodes.size()
					&& m_nodes[i].children < paths.size()
					&& m_nodes[m_nodes[i].parent].children + CHAR_TERMINATOR == i)
				paths[m_nodes[i].children] = m_nodes[i].parent;
			if ((m_nodes[i].children & FT_MASK)
					&& (m_nodes[i].children & ~FT_MASK) < paths.size())
				paths[m_nodes[i].children & ~FT_MASK] = i;
		}

		Container<SizeT>::build(
				std::back_inserter(m_pathString), &*paths.begin(), &*paths.end());
	}

	return begin;
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
uint32_t Trie<KeyT, ValueT, option, CharT, SizeT>::match(
		const CharT *keyBegin, const CharT *keyEnd, ValueT *value) const
{
	const Node *nodes = m_container->m_nodes.m_values;

	SizeT node = 1 + m_i;
	SizeT children = nodes[node].children;

	for (const CharT *key = keyBegin; ; key ++)
	{
		if (!(option & FT_NOTAIL) && (children & FT_MASK))
		{
			children &= ~FT_MASK;

			if ((size_t)(keyEnd - key)     != m_container->m_tails[children].size()
					|| !std::equal(key, keyEnd, &*m_container->m_tails[children].begin()))
				return 0;

			if (value) *value = m_container->m_values[children];

			return 1;
		}

		if (key == keyEnd) break;

		if (nodes[children + *key].parent != node) return 0;
		node = children + *key;
		children = nodes[children + *key].children;
	}

	SizeT term = children + CHAR_TERMINATOR;

	if (nodes[term].parent != node) return 0;
	if (value) *value = m_container->m_values[nodes[term].children];

	return 1;
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
uint32_t Trie<KeyT, ValueT, option, CharT, SizeT>::matchBeginning(
		const CharT *keyBegin, const CharT *keyEnd,
		uint32_t maxMatches, Range<CharT> *ranges,
		Trie<KeyT, ValueT, option, CharT, SizeT>::value_type *values) const
{
	uint32_t numMatches = 0;

	if (!maxMatches) return numMatches;

	const Node *nodes = m_container->m_nodes.m_values;

	SizeT node = 1 + m_i;
	SizeT children = nodes[node].children;

	for (const CharT *key = keyBegin; ; key ++)
	{
		if (!(option & FT_NOTAIL) && (children & FT_MASK))
		{
			children &= ~FT_MASK;

			size_t size = m_container->m_tails[children].size();

			if ((size_t)(keyEnd - key) < size
					|| !std::equal(key, key + size, &*m_container->m_tails[children].begin()))
				return numMatches;

			if (ranges)
			{
				ranges[numMatches].begin = keyBegin;
				ranges[numMatches].end = key + size;
			}
			if (values) values[numMatches] = m_container->m_values[children];

			return ++ numMatches;
		}

		SizeT term = children + CHAR_TERMINATOR;

		if (nodes[term].parent == node)
		{
			if (ranges)
			{
				ranges[numMatches].begin = keyBegin;
				ranges[numMatches].end = key;
			}
			if (values) values[numMatches] = m_container->m_values[nodes[term].children];

			if (++ numMatches >= maxMatches) return numMatches;
		}

		if (key == keyEnd) break;

		if (nodes[children + *key].parent != node) return numMatches;
		node = children + *key;
		children = nodes[children + *key].children;
	}

	return numMatches;
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
uint32_t Trie<KeyT, ValueT, option, CharT, SizeT>::matchAll(
		const CharT *keyBegin, const CharT *keyEnd,
		uint32_t maxMatches, Range<CharT> *ranges,
		Trie<KeyT, ValueT, option, CharT, SizeT>::value_type *values, uint32_t step) const
{
	uint32_t numMatches = 0;

	for (const CharT *key = keyBegin; ; key += step)
	{
		numMatches += matchBeginning(key, keyEnd, maxMatches - numMatches,
				(ranges ? ranges + numMatches : 0),
				(values ? values + numMatches : 0));
		if ((size_t)(keyEnd - key) < step || numMatches == maxMatches) break;
	}

	return numMatches;
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
typename Trie<KeyT, ValueT, option, CharT, SizeT>::const_iterator
Trie<KeyT, ValueT, option, CharT, SizeT>::find(
		const CharT *keyBegin, const CharT *keyEnd) const
{
	const Node *nodes = m_container->m_nodes.m_values;

	SizeT node = 1 + m_i;
	SizeT children = nodes[node].children;

	for (const CharT *key = keyBegin; ; key ++)
	{
		if (!(option & FT_NOTAIL) && (children & FT_MASK))
		{
			children &= ~FT_MASK;

			if ((size_t)(keyEnd - key)     != m_container->m_tails[children].size()
					|| !std::equal(key, keyEnd, &*m_container->m_tails[children].begin()))
				return end();

			return m_container->m_values.begin() + children;
		}

		if (key == keyEnd) break;

		if (nodes[children + *key].parent != node) return end();
		node = children + *key;
		children = nodes[children + *key].children;
	}

	SizeT term = children + CHAR_TERMINATOR;

	if (nodes[term].parent != node) return end();
	return m_container->m_values.begin() + nodes[term].children;
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
typename Trie<KeyT, ValueT, option, CharT, SizeT>::const_reference
Trie<KeyT, ValueT, option, CharT, SizeT>::operator ()(
		const CharT *keyBegin, const CharT *keyEnd) const
{
	const Node *nodes = m_container->m_nodes.m_values;

	SizeT node = 1 + m_i;
	SizeT children = nodes[node].children;

	for (const CharT *key = keyBegin; ; key ++)
	{
		if (!(option & FT_NOTAIL) && (children & FT_MASK))
		{
			children &= ~FT_MASK;

			if ((size_t)(keyEnd - key)     != m_container->m_tails[children].size()
					|| !std::equal(key, keyEnd, &*m_container->m_tails[children].begin()))
				return m_zero;

			return m_container->m_values[children];
		}

		if (key == keyEnd) break;

		if (nodes[children + *key].parent != node) return m_zero;
		node = children + *key;
		children = nodes[children + *key].children;
	}

	SizeT term = children + CHAR_TERMINATOR;

	if (nodes[term].parent != node) return m_zero;
	return m_container->m_values[nodes[term].children];
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
template <class StdKeyT>
const StdKeyT Trie<KeyT, ValueT, option, CharT, SizeT>::key(const_iterator it) const
{
	const Node  *nodes = m_container->m_nodes.m_values;
	const SizeT *paths = m_container->m_paths.m_values;

	if ((option & FT_PATH)
			&& m_container->m_paths.size() < m_container->m_values.size())
		paths = Container<SizeT>((void *)m_container->m_pathString.c_str()).m_values;
	else if (!(option & FT_PATH))
		return StdKeyT();

	size_t i = it - m_container->m_values.begin();

	size_t numChars = 0;
	if (!(option & FT_NOTAIL)) numChars += m_container->m_tails[i].size();
	for (SizeT node = paths[i]; !(nodes[node].parent & FT_MASK);
			node = nodes[node].parent) numChars ++;

	StdKeyT result;

	if (!Memory<StdKeyT>::resize(result, numChars * sizeof(CharT))) return result;

	CharT *p = (CharT *)Memory<StdKeyT>::end(result);
	if (!(option & FT_NOTAIL))
	{
		p -= m_container->m_tails[i].size();
		std::copy(m_container->m_tails[i].begin(), m_container->m_tails[i].end(), p);
	}
	for (SizeT node = paths[i]; !(nodes[node].parent & FT_MASK);
			node = nodes[node].parent)
		*(-- p) = (CharT)(node - nodes[nodes[node].parent].children);

	return result;
}

template <class KeyT, class ValueT, class HashT, class SizeT>
Container<HashMap<KeyT, ValueT, HashT, SizeT> >::
Container(const char *filename, int prot, int flags)
		: MMap<Container<HashMap<KeyT, ValueT, HashT, SizeT> > >(filename, prot, flags)
{
	if (filename[0] == 0) throw int(-1);

	const uint8_t *begin = (uint8_t *)this->mmap().first;
	const uint8_t *end = begin + this->mmap().second;

	if (initPointers(begin, end) != end) throw int(-1);
}

template <class KeyT, class ValueT, class HashT, class SizeT>
Container<HashMap<KeyT, ValueT, HashT, SizeT> >::Container(
		const void *begin, const void *end)
{
	if (initPointers((uint8_t *)begin, (uint8_t *)end)
			!= (uint8_t *)end && end) throw int(-1);
}

template <class KeyT, class ValueT, class HashT, class SizeT>
const uint8_t * Container<HashMap<KeyT, ValueT, HashT, SizeT> >::initPointers(
		const uint8_t *begin, const uint8_t *end)
{
	begin = m_maps.initPointers(begin, end); if (!begin) return 0;

	return begin;
}

template <class KeyT, class ValueT, class HashT, class SizeT>
typename HashMap<KeyT, ValueT, HashT, SizeT>::const_iterator
HashMap<KeyT, ValueT, HashT, SizeT>::find(const KeyT &key) const
{
	if (m_container->m_maps[m_i].size() == 0) return end();

	Vector<Pair<KeyT, ValueT>, SizeT> items =
			m_container->m_maps[m_i][HashT()(key) % m_container->m_maps[m_i].size()];

	for (typename Vector<Pair<KeyT, ValueT>, SizeT>::const_iterator
			it = items.begin(); it != items.end(); ++ it)
		if (key == it->first) return it;

	return end();
}

template <class KeyT, class ValueT, class HashT, class SizeT>
typename Container<ValueT>::const_reference
HashMap<KeyT, ValueT, HashT, SizeT>::operator ()(const KeyT &key) const
{
	if (m_container->m_maps[m_i].size() == 0) return m_zero;

	Vector<Pair<KeyT, ValueT>, SizeT> items =
			m_container->m_maps[m_i][HashT()(key) % m_container->m_maps[m_i].size()];

	for (typename Vector<Pair<KeyT, ValueT>, SizeT>::const_iterator
			it = items.begin(); it != items.end(); ++ it)
		if (key == it->first) return it->second;

	return m_zero;
}

template <class KeyT, class ValueT, class HashT, class SizeT>
template <class StdKeyT, class T>
typename HashMap<KeyT, ValueT, HashT, SizeT>::const_iterator
HashMap<KeyT, ValueT, HashT, SizeT>::find(const StdKeyT &key, Range<T> r) const
{
	if (m_container->m_maps[m_i].size() == 0) return end();

	Vector<Pair<KeyT, ValueT>, SizeT> items =
			m_container->m_maps[m_i][HashT()(key) % m_container->m_maps[m_i].size()];

	for (typename Vector<Pair<KeyT, ValueT>, SizeT>::const_iterator
			it = items.begin(); it != items.end(); ++ it)
		if ((size_t)(r.end - r.begin)  == it->first.size()
				&& std::equal(r.begin, r.end, it->first.begin()))
			return it;

	return end();
}

template <class KeyT, class ValueT, class HashT, class SizeT>
template <class StdKeyT, class T>
typename Container<ValueT>::const_reference
HashMap<KeyT, ValueT, HashT, SizeT>::operator ()(const StdKeyT &key, Range<T> r) const
{
	if (m_container->m_maps[m_i].size() == 0) return m_zero;

	Vector<Pair<KeyT, ValueT>, SizeT> items =
			m_container->m_maps[m_i][HashT()(key) % m_container->m_maps[m_i].size()];

	for (typename Vector<Pair<KeyT, ValueT>, SizeT>::const_iterator
			it = items.begin(); it != items.end(); ++ it)
		if ((size_t)(r.end - r.begin)  == it->first.size()
				&& std::equal(r.begin, r.end, it->first.begin()))
			return it->second;

	return m_zero;
}

template <class ValueT1, class ValueT2>
Container<Pair<ValueT1, ValueT2> >::
Container(const char *filename, int prot, int flags)
		: MMap<Container<Pair<ValueT1, ValueT2> > >(filename, prot, flags)
{
	if (filename[0] == 0) throw int(-1);

	const uint8_t *begin = (uint8_t *)this->mmap().first;
	const uint8_t *end = begin + this->mmap().second;

	if (initPointers(begin, end) != end) throw int(-1);
}

template <class ValueT1, class ValueT2>
Container<Pair<ValueT1, ValueT2> >::Container(
		const void *begin, const void *end)
{
	if (initPointers((uint8_t *)begin, (uint8_t *)end)
			!= (uint8_t *)end && end) throw int(-1);
}

template <class ValueT1, class ValueT2>
const uint8_t * Container<Pair<ValueT1, ValueT2> >::initPointers(
		const uint8_t *begin, const uint8_t *end)
{
	begin = m_values1.initPointers(begin, end); if (!begin) return 0;
	begin = m_values2.initPointers(begin, end); if (!begin) return 0;

	return m_values1.size() == m_values2.size() ? begin : 0;
}

template <class ValueT>
template <class OutIteratorT, class IteratorT>
OutIteratorT Container<ValueT>::build(
		OutIteratorT out, IteratorT begin, IteratorT end, const char *tmpdir)
{
	size_t numValues = std::distance(begin, end);
	size_t bytes = sizeof(ValueT) * numValues;
	bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

	char zeros[FT_ALIGN] = { 0 };

	out = std::copy((char *)&numValues, (char *)(&numValues + 1),     out);
	out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numValues),      out);

	std::vector<
			typename Conditional<IsSame<ValueT, bool>::value, // use char as bool
			char, ValueT>::type> values;
	for (IteratorT it = begin; it != end; values.push_back(*(it ++)))
		if (values.size() == 1048576)
		{
			out = std::copy((char *)&*values.begin(), (char *)&*values.end(), out);
			values.resize(0);
		}

	out = std::copy((char *)&*values.begin(), (char *)&*values.end(), out);
	out = std::copy(zeros, zeros + bytes,                             out);

	return out;
}

template <class ValueT>
template <class OutIteratorT>
OutIteratorT Container<ValueT>::build(
		OutIteratorT out, const ValueT *begin, const ValueT *end, const char *tmpdir)
{
	size_t numValues = std::distance(begin, end);
	size_t bytes = sizeof(ValueT) * numValues;
	bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

	char zeros[FT_ALIGN] = { 0 };

	out = std::copy((char *)&numValues, (char *)(&numValues + 1),     out);
	out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numValues),      out);
	out = std::copy((char *)begin, (char *)end,                       out);
	out = std::copy(zeros, zeros + bytes,                             out);

	return out;
}

template <class ValueT, class SizeT>
template <class OutIteratorT, class IteratorT>
OutIteratorT Container<Vector<ValueT, SizeT> >::build(
		OutIteratorT out, IteratorT begin, IteratorT end, const char *tmpdir)
{
	typedef typename std::iterator_traits<IteratorT>
			::value_type::const_iterator SubIterator;
	typedef typename std::iterator_traits<SubIterator>
			::value_type ValueType;

	// generate entries

	size_t numEntries = std::distance(begin, end) + 1;
	size_t bytes = sizeof(SizeT) * numEntries;
	bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

	char zeros[FT_ALIGN] = { 0 };

	out = std::copy((char *)&numEntries, (char *)(&numEntries + 1),   out);
	out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numEntries),     out);

	SizeT numValues = 0;
	for (IteratorT it = begin; it != end; numValues += (it ++)->size())
		out = std::copy((char *)&numValues, (char *)(&numValues + 1), out);

	out = std::copy((char *)&numValues, (char *)(&numValues + 1),     out);
	out = std::copy(zeros, zeros + bytes,                             out);

	// generate values

	if (Container<ValueT>::is_primitive)
	{
		bytes = sizeof(ValueT) * numValues;
		bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

		out = std::copy((char *)&numValues, (char *)(&numValues + 1),   out);
		out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numValues),    out);

		for (IteratorT it = begin; it != end; ++ it)
		{
			typedef typename std::iterator_traits<IteratorT>
					::value_type::const_reference SubReference;

			SubReference vBegin = *it->begin();
			SubReference vEnd   = *it->end();

			out = std::copy((char *)&vBegin, (char *)&vEnd, out);
		}

		out = std::copy(zeros, zeros + bytes,                           out);
	}
	else
	{
		std::vector<ValueType, TmpAlloc<ValueType> > values(tmpdir);
		values.reserve(numValues);
		for (IteratorT it = begin; it != end; ++ it)
			values.insert(values.end(), it->begin(), it->end());

		Container<ValueT>::build(out, values.begin(), values.end(), tmpdir);
	}

	return out;
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
template <class OutIteratorT, class IteratorT>
OutIteratorT Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::build(
		OutIteratorT out, IteratorT begin, IteratorT end, const char *tmpdir)
{
	typedef typename std::iterator_traits<IteratorT>
			::value_type::const_iterator SubIterator;
	typedef typename RemoveConst<typename std::iterator_traits<SubIterator>
			::value_type::first_type>::type KeyType;
	typedef typename std::iterator_traits<SubIterator>
			::value_type::second_type ValueType;

	// sort and unique input key-value pairs

	SizeT numInputPairs = 0;
	for (IteratorT it = begin; it != end; numInputPairs += (it ++)->size()) ;

	std::vector<SizeT,       TmpAlloc<SizeT> >       inputEntries(1, 0, tmpdir);
	std::vector<SubIterator, TmpAlloc<SubIterator> > inputPairs(tmpdir);
	inputEntries.reserve(std::distance(begin, end) + 1);
	inputPairs  .reserve(numInputPairs);
	for (IteratorT it = begin; it != end; ++ it)
	{
		for (SubIterator itSub = it->begin(); itSub != it->end(); ++ itSub)
			inputPairs.push_back(itSub);
		std::stable_sort(
				inputPairs.begin() + inputEntries.back(), inputPairs.end(), Less1st());
		inputPairs.erase(std::unique(
				inputPairs.begin() + inputEntries.back(), inputPairs.end(), EqualTo1st()),
				inputPairs.end());
		inputEntries.push_back(inputPairs.size());
	}

	// generate trie nodes

	size_t numTries  = inputEntries.size() - 1;
	size_t numValues = inputEntries.back();

	const size_t BITS = 8 * sizeof(size_t);

	std::vector<Node,   TmpAlloc<Node> >   nodes(tmpdir);
	std::vector<size_t, TmpAlloc<size_t> > masks(tmpdir);
	std::vector<SizeT,  TmpAlloc<SizeT> >  paths(tmpdir);
	std::vector<SizeT,  TmpAlloc<SizeT> >  tailEntries(1, 0, tmpdir);
	std::vector<CharT,  TmpAlloc<CharT> >  tailChars(tmpdir);

	nodes.reserve( numTries + 2 + FT_MARGIN);
	masks.reserve((numTries + 2 + FT_MARGIN) / BITS + 2);
	if (  option & FT_PATH   ) paths      .reserve(numValues);
	if (!(option & FT_NOTAIL)) tailEntries.reserve(numValues + 1);

	nodes.resize( numTries + 2,             Node( 0, 1));
	nodes.resize( numTries + 2 + FT_MARGIN, Node(-1, 1));
	masks.resize((numTries + 2 + FT_MARGIN) / BITS + 2);
	nodes[0].children          =   numTries + 2;
	nodes[numTries + 2].parent = - numTries - 2;
	for (size_t i = 0; i != numTries + 2; i ++)
		masks[i / BITS] |= (size_t)(1) << i % BITS;

	for (size_t i = 0; i != inputEntries.size() - 1; i ++)
	{
		nodes[i + 1].parent = inputEntries[i] | FT_MASK;

		typedef typename std::vector<SubIterator, TmpAlloc<SubIterator> >
				::const_iterator InputIterator;

		OpenNode<InputIterator> open;

		open.begin     = inputPairs.begin() + inputEntries[i];
		open.end       = inputPairs.begin() + inputEntries[i + 1];
		open.level     = 0;
		open.count     = 0;
		open.character = 0;
		open.entry     = i + 1;

		std::list<OpenNode<InputIterator> > openNodes;

		if (inputEntries[i] < inputEntries[i + 1]) openNodes.push_back(open);

		while (!openNodes.empty())
		{
			OpenNode<InputIterator> head = openNodes.back();
			openNodes.pop_back();

			if (!(option & FT_NOTAIL))
			{
				if (head.count == 1 &&
						head.level < Range<CharT>((*head.begin)->first).size())
				{
					nodes[head.entry].children = head.begin - inputPairs.begin() | FT_MASK;
					if (  option & FT_PATH   ) paths.push_back(head.entry);
					if (!(option & FT_NOTAIL))
					{
						tailChars.insert(tailChars.end(), 
								Range<CharT>((*head.begin)->first).begin + head.level,
								Range<CharT>((*head.begin)->first).end);
						tailEntries.push_back(tailChars.size());
					}

					continue;
				}
			}

			int32_t minCharacter = (1U << 31) - 1; // max int32_t

			std::list<OpenNode<InputIterator> > subOpenNodes;
			std::list<std::pair<int32_t, size_t> > bits;

			for (InputIterator itSub = head.begin; itSub != head.end; ++ itSub)
			{
				if (subOpenNodes.empty()
						|| Range<CharT>((*itSub                     )->first).size() <= head.level
						|| Range<CharT>((*subOpenNodes.front().begin)->first).size() <= head.level
						|| Range<CharT>((*itSub                     )->first).begin[head.level]
						!= Range<CharT>((*subOpenNodes.front().begin)->first).begin[head.level])
				{
					if (!subOpenNodes.empty())
						subOpenNodes.front().end = itSub;

					open.begin     = itSub;
					open.end       = itSub;
					open.level     = head.level + 1;
					open.count     = 0;
					open.character = Range<CharT>((*itSub)->first).size() <= head.level ?
							CHAR_TERMINATOR : (int32_t)Range<CharT>((*itSub)->first).begin[head.level];
					open.entry     = 0;

					subOpenNodes.push_front(open);

					if (bits.empty() ||
							bits.back().first != (open.character + BITS) / BITS - 1)
						bits.push_back(std::make_pair((open.character + BITS) / BITS - 1,
								(size_t)(1) << (open.character + BITS) % BITS));
					else
						bits.back().second |=
								(size_t)(1) << (open.character + BITS) % BITS;     

					if (open.character < minCharacter)
						minCharacter = open.character;
				}

				subOpenNodes.front().count ++;
			}
			if (!subOpenNodes.empty())
				subOpenNodes.front().end = head.end;

			SizeT children = nodes[0].children;
			while (1 + minCharacter > (int32_t)children)
				children += nodes[children].children;
			children -= minCharacter;

			typename std::list<OpenNode<InputIterator> >::iterator itEnt;

			for (std::list<std::pair<int32_t, size_t> >::const_iterator
					itBit = bits.begin(); itBit != bits.end(); )
			{
				if ((masks[children / BITS + itBit->first    ] &
						(itBit->second <<         children % BITS )) || (children % BITS &&
						(masks[children / BITS + itBit->first + 1] &
						(itBit->second >> (BITS - children % BITS)))))
				{
					children += nodes[children + minCharacter].children;
					itBit = bits.begin();
				}
				else ++ itBit;
			}

			if (nodes.size() < children + FT_MARGIN * 2)
			{
				nodes.resize( children + FT_MARGIN * 2, Node(-1, 1));
				masks.resize((children + FT_MARGIN * 2) / BITS + 2);
			}

			nodes[head.entry].children = children;

			for (itEnt = subOpenNodes.begin(); itEnt != subOpenNodes.end(); )
			{
				itEnt->entry = children + itEnt->character;

				nodes[itEnt->entry + nodes[itEnt->entry].parent].children =
						nodes[itEnt->entry].children - nodes[itEnt->entry].parent;
				nodes[itEnt->entry + nodes[itEnt->entry].children].parent =
						nodes[itEnt->entry].parent - nodes[itEnt->entry].children;
				nodes[itEnt->entry].parent = head.entry;
				masks[itEnt->entry / BITS] |= (size_t)(1) << itEnt->entry % BITS;

				if (itEnt->character == CHAR_TERMINATOR)
				{
					nodes[itEnt->entry].children = itEnt->begin - inputPairs.begin();
					if (  option & FT_PATH   ) paths.push_back(head.entry);
					if (!(option & FT_NOTAIL)) tailEntries.push_back(tailChars.size());

					itEnt = subOpenNodes.erase(itEnt);
				}
				else ++ itEnt;
			}

			if (option & FT_QUICKBUILD)
			{
				while (nodes[0].children + FT_MARGIN * 8 < nodes.size())
					nodes[0].children += nodes[nodes[0].children].children;
				nodes[nodes[0].children].parent = - nodes[0].children;
			}

			openNodes.splice(openNodes.end(), subOpenNodes);
		}
	}

	while (nodes[nodes.size() - FT_MARGIN].parent & FT_MASK)
		nodes.resize(nodes.size() - 1);

	nodes[0           ].children = numTries;
	nodes[1 + numTries].parent   = numValues | FT_MASK;

	for (size_t i = numTries + 2; i != nodes.size(); i ++)
		if (nodes[i].parent & FT_MASK)
			nodes[i].parent = nodes[i].children = 0;

	std::vector<size_t, TmpAlloc<size_t> >(tmpdir).swap(masks);
	Container<Node>::build(out, &*nodes.begin(), &*nodes.end(), tmpdir);
	std::vector<Node, TmpAlloc<Node> >(tmpdir).swap(nodes);
	Container<SizeT>::build(out, &*paths.begin(), &*paths.end(), tmpdir);
	std::vector<SizeT, TmpAlloc<SizeT> >(tmpdir).swap(paths);
	Container<SizeT>::build(out, tailEntries.begin(), tailEntries.end(), tmpdir);
	std::vector<SizeT, TmpAlloc<SizeT> >(tmpdir).swap(tailEntries);
	Container<CharT>::build(out, tailChars.begin(), tailChars.end(), tmpdir);
	std::vector<CharT, TmpAlloc<CharT> >(tmpdir).swap(tailChars);

	// generate values

	if (Container<ValueT>::is_primitive)
	{
		size_t bytes = sizeof(ValueT) * numValues;
		bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

		char zeros[FT_ALIGN] = { 0 };

		out = std::copy((char *)&numValues, (char *)(&numValues + 1),     out);
		out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numValues),      out);

		std::vector<
				typename Conditional<Container<ValueT>::is_primitive, // avoid warning
				typename Conditional<IsSame<ValueT, bool>::value, // use char as bool
				char, ValueT>::type, ValueType>::type> values;
		for (size_t i = 0; i != numValues; values.push_back(inputPairs[i ++]->second))
			if (values.size() == 1048576)
			{
				out = std::copy((char *)&*values.begin(), (char *)&*values.end(), out);
				values.resize(0);
			}

		out = std::copy((char *)&*values.begin(), (char *)&*values.end(), out);
		out = std::copy(zeros, zeros + bytes,                             out);
	}
	else
	{
		std::vector<ValueType, TmpAlloc<ValueType> > values(tmpdir);
		values.reserve(numValues);
		for (size_t i = 0; i != numValues; i ++)
			values.push_back(inputPairs[i]->second);

		std::vector<SizeT,       TmpAlloc<SizeT> >      (tmpdir).swap(inputEntries);
		std::vector<SubIterator, TmpAlloc<SubIterator> >(tmpdir).swap(inputPairs);

		Container<ValueT>::build(out, values.begin(), values.end(), tmpdir);
	}

	return out;
}

template <class KeyT, class ValueT, class HashT, class SizeT>
template <class OutIteratorT, class IteratorT>
OutIteratorT Container<HashMap<KeyT, ValueT, HashT, SizeT> >::build(
		OutIteratorT out, IteratorT begin, IteratorT end, const char *tmpdir)
{
	typedef typename std::iterator_traits<IteratorT>
			::value_type::const_iterator SubIterator;
	typedef typename RemoveConst<typename std::iterator_traits<SubIterator>
			::value_type::first_type>::type KeyType;
	typedef typename std::iterator_traits<SubIterator>
			::value_type::second_type ValueType;

	return build(out, begin, end, tmpdir, typename Conditional<
			Container<KeyType  >::is_primitive &&
			Container<ValueType>::is_primitive, long, char>::type());
}

template <class KeyT, class ValueT, class HashT, class SizeT>
template <class OutIteratorT, class IteratorT>
OutIteratorT Container<HashMap<KeyT, ValueT, HashT, SizeT> >::build(
		OutIteratorT out, IteratorT begin, IteratorT end, const char *tmpdir, char)
{
	typedef typename std::iterator_traits<IteratorT>
			::value_type::const_iterator SubIterator;
	typedef typename RemoveConst<typename std::iterator_traits<SubIterator>
			::value_type::first_type>::type KeyType;
	typedef typename std::iterator_traits<SubIterator>
			::value_type::second_type ValueType;

	// sort and unique input key-value pairs

	SizeT numInputPairs = 0;
	for (IteratorT it = begin; it != end; numInputPairs += (it ++)->size()) ;

	std::vector<SizeT,       TmpAlloc<SizeT> >       inputEntries(1, 0, tmpdir);
	std::vector<SubIterator, TmpAlloc<SubIterator> > inputPairs(tmpdir);
	inputEntries.reserve(std::distance(begin, end) + 1);
	inputPairs  .reserve(numInputPairs);
	for (IteratorT it = begin; it != end; ++ it)
	{
		for (SubIterator itSub = it->begin(); itSub != it->end(); ++ itSub)
			inputPairs.push_back(itSub);
		std::stable_sort(
				inputPairs.begin() + inputEntries.back(), inputPairs.end(), Less1st());
		inputPairs.erase(std::unique(
				inputPairs.begin() + inputEntries.back(), inputPairs.end(), EqualTo1st()),
				inputPairs.end());
		inputEntries.push_back(inputPairs.size());
	}

	// generate entries

	size_t numEntries = inputEntries.size();
	size_t bytes = sizeof(SizeT) * numEntries;
	bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

	char zeros[FT_ALIGN] = { 0 };

	out = std::copy((char *)&numEntries, (char *)(&numEntries + 1),   out);
	out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numEntries),     out);

	SizeT numBuckets = 0;
	for (size_t i = 0; i != inputEntries.size() - 1; i ++)
	{
		out = std::copy((char *)&numBuckets, (char *)(&numBuckets + 1), out);
		size_t bucketSize = inputEntries[i + 1] - inputEntries[i];
		numBuckets += bucketSize;
	}

	out = std::copy((char *)&numBuckets, (char *)(&numBuckets + 1),   out);
	out = std::copy(zeros, zeros + bytes,                             out);

	// generate bucket entries

	size_t numBucketEntries = numBuckets + 1;
	bytes = sizeof(SizeT) * numBucketEntries;
	bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

	out = std::copy((char *)&numBucketEntries, (char *)(&numBucketEntries + 1), out);
	out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numBucketEntries),         out);

	std::vector<SizeT, TmpAlloc<SizeT> > pairNums(tmpdir);
	std::vector<SizeT, TmpAlloc<SizeT> > pairEntries(1, 0, tmpdir);
	pairNums   .reserve(numBuckets);
	pairEntries.reserve(numBuckets + 1);
	for (size_t i = 0; i != inputEntries.size() - 1; i ++)
	{
		size_t bucketSize = inputEntries[i + 1] - inputEntries[i];
		pairNums.insert(pairNums.end(), bucketSize, 0);
		for (size_t j = inputEntries[i]; j != inputEntries[i + 1]; j ++)
		{
			size_t h = pairNums.size() - bucketSize +
					HashT()(inputPairs[j]->first) % bucketSize;
			pairNums[h] ++;
		}
	}
	std::partial_sum(pairNums.begin(), pairNums.end(),
			std::back_inserter(pairEntries));

	out = std::copy((char *)&*pairEntries.begin(), (char *)&*pairEntries.end(), out);
	out = std::copy(zeros, zeros + bytes,                                       out);

	// generate keys & values

	pairNums.resize(0);

	std::vector<KeyType, TmpAlloc<KeyType> > keys(
			pairEntries.back(), KeyType(), tmpdir);
	std::vector<ValueType, TmpAlloc<ValueType> > values(
			pairEntries.back(), ValueType(), tmpdir);
	for (size_t i = 0; i != inputEntries.size() - 1; i ++)
	{
		size_t bucketSize = inputEntries[i + 1] - inputEntries[i];
		pairNums.insert(pairNums.end(), bucketSize, 0);
		for (size_t j = inputEntries[i]; j != inputEntries[i + 1]; j ++)
		{
			size_t h = pairNums.size() - bucketSize +
					HashT()(inputPairs[j]->first) % bucketSize;
			keys  [pairEntries[h] + pairNums[h]] = inputPairs[j]->first;
			values[pairEntries[h] + pairNums[h]] = inputPairs[j]->second;
			pairNums[h] ++;
		}
	}

	std::vector<SizeT, TmpAlloc<SizeT> >(tmpdir).swap(pairNums);
	std::vector<SizeT, TmpAlloc<SizeT> >(tmpdir).swap(pairEntries);

	std::vector<SizeT,       TmpAlloc<SizeT> >      (tmpdir).swap(inputEntries);
	std::vector<SubIterator, TmpAlloc<SubIterator> >(tmpdir).swap(inputPairs);

	Container<KeyT>::build(out, keys.begin(), keys.end(), tmpdir);
	std::vector<KeyType, TmpAlloc<KeyType> >(tmpdir).swap(keys);
	Container<ValueT>::build(out, values.begin(), values.end(), tmpdir);
	std::vector<ValueType, TmpAlloc<ValueType> >(tmpdir).swap(values);

	return out;
}

template <class KeyT, class ValueT, class HashT, class SizeT>
template <class OutIteratorT, class IteratorT>
OutIteratorT Container<HashMap<KeyT, ValueT, HashT, SizeT> >::build(
		OutIteratorT out, IteratorT begin, IteratorT end, const char *tmpdir, long)
{
	typedef typename std::iterator_traits<IteratorT>
			::value_type::const_iterator SubIterator;
	typedef typename RemoveConst<typename std::iterator_traits<SubIterator>
			::value_type::first_type>::type KeyType;
	typedef typename std::iterator_traits<SubIterator>
			::value_type::second_type ValueType;
	typedef typename std::pair<KeyType, ValueType> PairType;

	// sort and unique input key-value pairs

	SizeT numInputPairs = 0;
	for (IteratorT it = begin; it != end; numInputPairs += (it ++)->size()) ;

	std::vector<SizeT,    TmpAlloc<SizeT> >    inputEntries(1, 0, tmpdir);
	std::vector<PairType, TmpAlloc<PairType> > inputPairs(tmpdir);
	inputEntries.reserve(std::distance(begin, end) + 1);
	inputPairs  .reserve(numInputPairs);
	for (IteratorT it = begin; it != end; ++ it)
	{
		for (SubIterator itSub = it->begin(); itSub != it->end(); ++ itSub)
			inputPairs.push_back(std::make_pair(itSub->first, itSub->second));
		std::stable_sort(
				inputPairs.begin() + inputEntries.back(), inputPairs.end(), Less1stObj());
		inputPairs.erase(std::unique(
				inputPairs.begin() + inputEntries.back(), inputPairs.end(), EqualTo1stObj()),
				inputPairs.end());
		inputEntries.push_back(inputPairs.size());
	}

	// generate entries

	size_t numEntries = inputEntries.size();
	size_t bytes = sizeof(SizeT) * numEntries;
	bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

	char zeros[FT_ALIGN] = { 0 };

	out = std::copy((char *)&numEntries, (char *)(&numEntries + 1),   out);
	out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numEntries),     out);

	SizeT numBuckets = 0;
	for (size_t i = 0; i != inputEntries.size() - 1; i ++)
	{
		out = std::copy((char *)&numBuckets, (char *)(&numBuckets + 1), out);
		size_t bucketSize = inputEntries[i + 1] - inputEntries[i];
		numBuckets += bucketSize;
	}

	out = std::copy((char *)&numBuckets, (char *)(&numBuckets + 1),   out);
	out = std::copy(zeros, zeros + bytes,                             out);

	// generate bucket entries

	size_t numBucketEntries = numBuckets + 1;
	bytes = sizeof(SizeT) * numBucketEntries;
	bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

	out = std::copy((char *)&numBucketEntries, (char *)(&numBucketEntries + 1), out);
	out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numBucketEntries),         out);

	std::vector<SizeT, TmpAlloc<SizeT> > pairNums(tmpdir);
	std::vector<SizeT, TmpAlloc<SizeT> > pairEntries(1, 0, tmpdir);
	pairNums   .reserve(numBuckets);
	pairEntries.reserve(numBuckets + 1);
	for (size_t i = 0; i != inputEntries.size() - 1; i ++)
	{
		size_t bucketSize = inputEntries[i + 1] - inputEntries[i];
		pairNums.insert(pairNums.end(), bucketSize, 0);
		for (size_t j = inputEntries[i]; j != inputEntries[i + 1]; j ++)
		{
			size_t h = pairNums.size() - bucketSize +
					HashT()(inputPairs[j].first) % bucketSize;
			pairNums[h] ++;
		}
	}
	std::partial_sum(pairNums.begin(), pairNums.end(),
			std::back_inserter(pairEntries));

	out = std::copy((char *)&*pairEntries.begin(), (char *)&*pairEntries.end(), out);
	out = std::copy(zeros, zeros + bytes,                                       out);

	// generate keys & values

	pairNums.resize(0);

	std::vector<KeyType, TmpAlloc<KeyType> > keys(
			pairEntries.back(), KeyType(), tmpdir);
	std::vector<ValueType, TmpAlloc<ValueType> > values(
			pairEntries.back(), ValueType(), tmpdir);
	for (size_t i = 0; i != inputEntries.size() - 1; i ++)
	{
		size_t bucketSize = inputEntries[i + 1] - inputEntries[i];
		pairNums.insert(pairNums.end(), bucketSize, 0);
		for (size_t j = inputEntries[i]; j != inputEntries[i + 1]; j ++)
		{
			size_t h = pairNums.size() - bucketSize +
					HashT()(inputPairs[j].first) % bucketSize;
			keys  [pairEntries[h] + pairNums[h]] = inputPairs[j].first;
			values[pairEntries[h] + pairNums[h]] = inputPairs[j].second;
			pairNums[h] ++;
		}
	}

	std::vector<SizeT, TmpAlloc<SizeT> >(tmpdir).swap(pairNums);
	std::vector<SizeT, TmpAlloc<SizeT> >(tmpdir).swap(pairEntries);

	std::vector<SizeT,    TmpAlloc<SizeT> >   (tmpdir).swap(inputEntries);
	std::vector<PairType, TmpAlloc<PairType> >(tmpdir).swap(inputPairs);

	Container<KeyT>::build(out, keys.begin(), keys.end(), tmpdir);
	std::vector<KeyType, TmpAlloc<KeyType> >(tmpdir).swap(keys);
	Container<ValueT>::build(out, values.begin(), values.end(), tmpdir);
	std::vector<ValueType, TmpAlloc<ValueType> >(tmpdir).swap(values);

	return out;
}

template <class ValueT1, class ValueT2>
template <class OutIteratorT, class IteratorT>
OutIteratorT Container<Pair<ValueT1, ValueT2> >::build(
		OutIteratorT out, IteratorT begin, IteratorT end, const char *tmpdir)
{
	typedef typename std::iterator_traits<IteratorT>
			::value_type::first_type  ValueType1;
	typedef typename std::iterator_traits<IteratorT>
			::value_type::second_type ValueType2;

	size_t numValues = std::distance(begin, end);

	// generate first values

	if (Container<ValueT1>::is_primitive)
	{
		size_t bytes = sizeof(ValueT1) * numValues;
		bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

		char zeros[FT_ALIGN] = { 0 };

		out = std::copy((char *)&numValues, (char *)(&numValues + 1),     out);
		out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numValues),      out);

		std::vector<
				typename Conditional<Container<ValueT1>::is_primitive, // avoid warning
				typename Conditional<IsSame<ValueT1, bool>::value, // use char as bool
				char, ValueT1>::type, ValueType1>::type> values;
		for (IteratorT it = begin; it != end; values.push_back((it ++)->first))
			if (values.size() == 1048576)
			{
				out = std::copy((char *)&*values.begin(), (char *)&*values.end(), out);
				values.resize(0);
			}

		out = std::copy((char *)&*values.begin(), (char *)&*values.end(), out);
		out = std::copy(zeros, zeros + bytes,                             out);
	}
	else
	{
		std::vector<ValueType1, TmpAlloc<ValueType1> > values(tmpdir);
		values.reserve(numValues);
		for (IteratorT it = begin; it != end; ++ it)
			values.push_back(it->first);

		Container<ValueT1>::build(out, values.begin(), values.end(), tmpdir);
	}

	// generate second values

	if (Container<ValueT2>::is_primitive)
	{
		size_t bytes = sizeof(ValueT2) * numValues;
		bytes = (bytes + FT_ALIGN - 1) / FT_ALIGN * FT_ALIGN - bytes;

		char zeros[FT_ALIGN] = { 0 };

		out = std::copy((char *)&numValues, (char *)(&numValues + 1),     out);
		out = std::copy(zeros, zeros + FT_ALIGN - sizeof(numValues),      out);

		std::vector<
				typename Conditional<Container<ValueT2>::is_primitive, // avoid warning
				typename Conditional<IsSame<ValueT2, bool>::value, // use char as bool
				char, ValueT2>::type, ValueType2>::type> values;
		for (IteratorT it = begin; it != end; values.push_back((it ++)->second))
			if (values.size() == 1048576)
			{
				out = std::copy((char *)&*values.begin(), (char *)&*values.end(), out);
				values.resize(0);
			}

		out = std::copy((char *)&*values.begin(), (char *)&*values.end(), out);
		out = std::copy(zeros, zeros + bytes,                             out);
	}
	else
	{
		std::vector<ValueType2, TmpAlloc<ValueType2> > values(tmpdir);
		values.reserve(numValues);
		for (IteratorT it = begin; it != end; ++ it)
			values.push_back(it->second);

		Container<ValueT2>::build(out, values.begin(), values.end(), tmpdir);
	}

	return out;
}

template <class ValueT, class SizeT>
const Container<Vector<ValueT, SizeT> >
Vector<ValueT, SizeT>::defaultContainer()
{
	static std::string data;

	if (data.empty())
	{
		typename Container<Vector<ValueT, SizeT> >::std_value_type null;
		Container<Vector<ValueT, SizeT> >::build(
				std::back_inserter(data), &null, &null + 1);
	}

	return Container<Vector<ValueT, SizeT> >((void *)data.c_str());
}

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
const Container<Trie<KeyT, ValueT, option, CharT, SizeT> >
Trie<KeyT, ValueT, option, CharT, SizeT>::defaultContainer()
{
	static std::string data;

	if (data.empty())
	{
		typename Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::std_value_type null;
		Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::build(
				std::back_inserter(data), &null, &null + 1);
	}

	return Container<Trie<KeyT, ValueT, option, CharT, SizeT> >((void *)data.c_str());
}

template <class KeyT, class ValueT, class HashT, class SizeT>
const Container<HashMap<KeyT, ValueT, HashT, SizeT> >
HashMap<KeyT, ValueT, HashT, SizeT>::defaultContainer()
{
	static std::string data;

	if (data.empty())
	{
		typename Container<HashMap<KeyT, ValueT, HashT, SizeT> >::std_value_type null;
		Container<HashMap<KeyT, ValueT, HashT, SizeT> >::build(
				std::back_inserter(data), &null, &null + 1);
	}

	return Container<HashMap<KeyT, ValueT, HashT, SizeT> >((void *)data.c_str());
}

template <class ValueT, class SizeT>
const SizeT Container<Vector<ValueT, SizeT> >::m_defaultEntries[1] = { 0 };

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
const typename Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::Node
		Container<Trie<KeyT, ValueT, option, CharT, SizeT> >::m_defaultNodes[1];

template <class ValueT, class SizeT>
const Container<Vector<ValueT, SizeT> >
Vector<ValueT, SizeT>::m_defaultContainer =
		Vector<ValueT, SizeT>::defaultContainer();

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
const Container<Trie<KeyT, ValueT, option, CharT, SizeT> >
Trie<KeyT, ValueT, option, CharT, SizeT>::m_defaultContainer =
		Trie<KeyT, ValueT, option, CharT, SizeT>::defaultContainer();

template <class KeyT, class ValueT, class HashT, class SizeT>
const Container<HashMap<KeyT, ValueT, HashT, SizeT> >
HashMap<KeyT, ValueT, HashT, SizeT>::m_defaultContainer =
		HashMap<KeyT, ValueT, HashT, SizeT>::defaultContainer();

template <class ValueT1, class ValueT2>
const ValueT1 Pair<ValueT1, ValueT2>::m_defaultFirst  = ValueT1();
template <class ValueT1, class ValueT2>
const ValueT2 Pair<ValueT1, ValueT2>::m_defaultSecond = ValueT2();

template <class KeyT, class ValueT, int option, class CharT, class SizeT>
const ValueT Trie<KeyT, ValueT, option, CharT, SizeT>::m_zero = ValueT();

template <class KeyT, class ValueT, class HashT, class SizeT>
const ValueT HashMap<KeyT, ValueT, HashT, SizeT>::m_zero = ValueT();

#ifndef FT2_NAMESPACE
} // namespace ft2
#else
} // namespace FT2_NAMESPACE
#endif

#endif // __FAST_TRIE_2_H__
