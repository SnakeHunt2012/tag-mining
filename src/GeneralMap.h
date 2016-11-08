// GeneralMap 1.0.4 2014-08-11

#ifndef __GENERAL_MAP_H__
#define __GENERAL_MAP_H__

#include <iterator>
#include <algorithm>

namespace ft2 {

template <class ValueT, class SizeT>                                      class Vector;
template <class KeyT, class ValueT, int option, class CharT, class SizeT> class Trie;
template <class KeyT, class ValueT, class HashT, class SizeT>             class HashMap;
template <class ValueT1, class ValueT2>                                   class Pair;

} // namespace ft2

namespace generalmap {

template <class MapT>
class General // use vector<V> in the same way as map<size_t, V>
{
	template <int n> struct If {};

	template <bool cond, class T, class F> struct Conditional             { typedef F type; };
	template            <class T, class F> struct Conditional<true, T, F> { typedef T type; };

	template <class T1, class T2> struct IsSame       { enum { value = false }; };
	template <class T>            struct IsSame<T, T> { enum { value = true  }; };

	template <class T> struct RemoveConst          { typedef T type; };
	template <class T> struct RemoveConst<const T> { typedef T type; };

	template <class T> struct IsConst           { enum { value = false }; };
	template <class T> struct IsConst<const T>  { enum { value = true  }; };

	template <class T> struct IsFt              { enum { value = false }; };
	template <class T> struct IsFt<const T>     { enum { value = IsFt<T>::value }; };

	template <class V, class S>   struct IsFt<ft2::Vector<V, S> > { enum { value = true }; };
	template <class K, class V, int o, class C, class S>
	struct IsFt<ft2::Trie<K, V, o, C, S> >                        { enum { value = true }; };
	template <class K, class V, class H, class S>
	struct IsFt<ft2::HashMap<K, V, H, S> >                        { enum { value = true }; };
	template <class T1, class T2> struct IsFt<ft2::Pair<T1, T2> > { enum { value = true }; };

	template <class T> struct IsFtPair          { enum { value = false }; };
	template <class T> struct IsFtPair<const T> { enum { value = IsFtPair<T>::value }; };

	template <class T1, class T2> struct IsFtPair<ft2::Pair<T1, T2> > { enum { value = true }; };

	template <class T> struct IsPair
	{
		template <class C> static long checkSecond(If<sizeof(&C::second)> *);
		template <class C> static char checkSecond(...);

		enum { value = sizeof(checkSecond<T>(0)) == sizeof(long) };
	};
	template <class T> struct IsPair<const T>   { enum { value = IsPair<T>::value }; };

	template <class T1, class T2> struct IsPair<ft2::Pair<T1, T2> > { enum { value = true }; };

	template <class T> struct IsVector          { enum { value = false }; };
	template <class T> struct IsVector<const T> { enum { value = IsVector<T>::value }; };

	template <class V, class S> struct IsVector<ft2::Vector<V, S> > { enum { value = true }; };
	template <class T, class A> struct IsVector<std::vector<T, A> > { enum { value = true }; };

	template <class M, bool hasSecond> struct KeyType
	{
		typedef size_t type;
	};
	template <class M> struct KeyType<M, true>
	{
		typedef typename RemoveConst<typename M::value_type::first_type>::type type;
	};
	template <class M, bool hasSecond> struct ValueType
	{
		typedef typename M::value_type type;
	};
	template <class M> struct ValueType<M, true>
	{
		typedef typename M::value_type::second_type type;
	};
	template <class M, bool hasSecond> struct ConstMappedReference
	{
		typedef typename std::iterator_traits<typename M::const_iterator>::reference type;
	};
	template <class M> struct ConstMappedReference<M, true>
	{
		typedef typename Conditional<IsFtPair<typename M::value_type>::value
				&& IsFt<typename M::value_type::second_type>::value,
								typename M::value_type::second_type,
					const typename M::value_type::second_type &>::type type;
	};
	template <class M, bool hasSecond> struct MappedReference
	{
		typedef typename std::iterator_traits<typename M::iterator>::reference type;
	};
	template <class M> struct MappedReference<M, true>
	{
		typedef typename Conditional<IsFtPair<typename M::value_type>::value
				&& IsFt<typename M::value_type::second_type>::value,
								typename M::value_type::second_type,
								typename M::value_type::second_type &>::type type;
	};

public:
	enum { hasSecond = IsPair<typename MapT::value_type>::value };
	enum { hasFind = hasSecond && !IsVector<MapT>::value };
	enum { isConst = IsConst<MapT>::value ||
			IsSame<typename MapT::const_iterator, typename MapT::iterator>::value };

	typedef typename KeyType             <MapT, hasSecond>::type key_type;
	typedef typename ValueType           <MapT, hasSecond>::type value_type;
	typedef typename Conditional<IsFt<MapT>::value || IsConst<MapT>::value,
					typename ConstMappedReference<MapT, hasSecond>::type,
					typename      MappedReference<MapT, hasSecond>::type>::type mapped_reference;
	typedef typename ConstMappedReference<MapT, hasSecond>::type  const_mapped_reference;
	typedef typename Conditional<IsFt<MapT>::value || IsConst<MapT>::value,
					typename MapT::const_iterator, typename MapT::iterator>::type iterator;
	typedef typename MapT::const_iterator                           const_iterator;

	typedef typename Conditional<IsFt<MapT>::value, MapT,       MapT &>::type      Reference;
	typedef typename Conditional<IsFt<MapT>::value, MapT, const MapT &>::type ConstReference;

	General(Reference m) : m_map(m), m_constmap(m) {}

	template <class KeyT>
	const_iterator find(const KeyT &key) const
	{
		return find_0(key, (If<hasSecond> *)(0), (If<hasFind> *)(0));
	}
	template <class KeyT>
	iterator find(const KeyT &key)
	{
		return find_0(key, (If<hasSecond> *)(0), (If<hasFind> *)(0));
	}
	template <class KeyT>
	size_t distance(const KeyT &key) const
	{
		return distance_0(key, (If<hasSecond> *)(0));
	}
	key_type key(const_iterator it) const
	{
		return key_0(it, (If<hasSecond> *)(0));
	}
	const_mapped_reference value(const_iterator it) const
	{
		return value_0(it, (If<hasSecond> *)(0));
	}
	mapped_reference value(iterator it)
	{
		return value_0(it, (If<hasSecond> *)(0));
	}
	const_mapped_reference operator [](const_iterator it) const
	{
		return value_0(it, (If<hasSecond> *)(0));
	}
	mapped_reference operator [](iterator it)
	{
		return value_0(it, (If<hasSecond> *)(0));
	}
	template <class KeyT, class ValueT>
	iterator insert(const KeyT &key, const ValueT &value)
	{
		return insert_0(key, value, (If<hasSecond> *)(0), (If<hasFind> *)(0));
	}

private:
	template <class KeyT> struct EqualTo1st
	{
		KeyT m_key;

		EqualTo1st(const KeyT &key) : m_key(key) {}

		template <class T>
		bool operator ()(const T &x) const { return x.first == m_key; }
	};

	template <class KeyT>
	const_iterator find_0(const KeyT &key, If<0> *, If<0> *) const
	{
		return key >= m_constmap.size() ? m_constmap.end() : m_constmap.begin() + key;
	}
	template <class KeyT>
	const_iterator find_0(const KeyT &key, If<0> *, If<1> *) const
	{
		return m_constmap.find(key);
	}
	template <class KeyT>
	const_iterator find_0(const KeyT &key, If<1> *, If<0> *) const
	{
		return std::find_if(m_constmap.begin(), m_constmap.end(), EqualTo1st<KeyT>(key));
	}
	template <class KeyT>
	const_iterator find_0(const KeyT &key, If<1> *, If<1> *) const
	{
		return m_constmap.find(key);
	}
	template <class KeyT>
	iterator find_0(const KeyT &key, If<0> *, If<0> *)
	{
		return key >= m_map.size() ? m_map.end() : m_map.begin() + key;
	}
	template <class KeyT>
	iterator find_0(const KeyT &key, If<0> *, If<1> *)
	{
		return m_map.find(key);
	}
	template <class KeyT>
	iterator find_0(const KeyT &key, If<1> *, If<0> *)
	{
		return std::find_if(m_map.begin(), m_map.end(), EqualTo1st<KeyT>(key));
	}
	template <class KeyT>
	iterator find_0(const KeyT &key, If<1> *, If<1> *)
	{
		return m_map.find(key);
	}
	template <class KeyT>
	size_t distance_0(const KeyT &key, If<0> *) const
	{
		return key;
	}
	template <class KeyT>
	size_t distance_0(const KeyT &key, If<1> *) const
	{
		return std::distance(m_constmap.begin(), find(key));
	}
	key_type key_0(const_iterator it, If<0> *) const
	{
		return std::distance(m_constmap.begin(), it);
	}
	key_type key_0(const_iterator it, If<1> *) const
	{
		return it->first;
	}
	const_mapped_reference value_0(const_iterator it, If<0> *) const
	{
		return *it;
	}
	const_mapped_reference value_0(const_iterator it, If<1> *) const
	{
		return it->second;
	}
	mapped_reference value_0(iterator it, If<0> *)
	{
		return *it;
	}
	mapped_reference value_0(iterator it, If<1> *)
	{
		return it->second;
	}
	template <class KeyT, class ValueT>
	iterator insert_0(const KeyT &key, const ValueT &value, If<0> *, If<0> *)
	{
		if (m_map.size() <= key) m_map.resize(key + 1);
		m_map[key] = value; return m_map.begin() + key;
	}
	template <class KeyT, class ValueT>
	iterator insert_0(const KeyT &key, const ValueT &value, If<0> *, If<1> *)
	{
		return m_map.insert(std::make_pair(key, value)).first;
	}
	template <class KeyT, class ValueT>
	iterator insert_0(const KeyT &key, const ValueT &value, If<1> *, If<0> *)
	{
		m_map.push_back(std::make_pair(key, value)); return m_map.end() - 1;
	}
	template <class KeyT, class ValueT>
	iterator insert_0(const KeyT &key, const ValueT &value, If<1> *, If<1> *)
	{
		return m_map.insert(std::make_pair(key, value)).first;
	}

private:
	Reference      m_map;
	ConstReference m_constmap;
};

template <class MapT>
inline General<const MapT> general(const MapT &m)
{
	return General<const MapT>(m);
}
template <class MapT>
inline General<MapT> general(MapT &m)
{
	return General<MapT>(m);
}

} // namespace generalmap

#endif // __GENERAL_MAP_H__
