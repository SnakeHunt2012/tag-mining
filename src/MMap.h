// FastTrie 2.4.10 2014-08-11

#ifndef __MMAP_2_H__
#define __MMAP_2_H__

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>

#include <string>
#include <map>

#ifndef FT2_NAMESPACE
namespace ft2 {
#else
namespace FT2_NAMESPACE {
#endif

template <class T>
class MMap
{
public:
	MMap(const char *filename = "", int prot = PROT_READ, int flags = MAP_SHARED);
	MMap(const MMap &from);
	MMap & operator =(const MMap &from);
	~MMap();

	const std::string & filename() const { return m_filename; }

	std::pair<const void *, size_t> mmap() const;

private:
	struct Usage
	{
		Usage() : fd(-1), address(0), length(0), used(0) {}

		int fd;
		const void *address;
		size_t length;
		size_t used;
	};

	typedef std::map<std::pair<size_t, size_t>, Usage> UsageMap;

	std::string               m_filename;
	std::pair<size_t, size_t> m_devinode;

	static pthread_mutex_t m_mutex;
	static UsageMap       *m_mmaps;
};

template <class T>
MMap<T>::MMap(const char *filename, int prot, int flags)
		: m_filename(filename), m_devinode(0, 0)
{
	if (m_filename.empty()) return;

	Usage usage;
	struct stat st;
	typename UsageMap::iterator it;

	pthread_mutex_lock(&m_mutex);
	if (m_mmaps == 0) m_mmaps = new UsageMap();
	usage.fd = open(m_filename.c_str(),
			(prot & PROT_WRITE && flags & MAP_SHARED) ? O_RDWR : O_RDONLY);
	if (usage.fd < 0) goto error;
	if (fstat(usage.fd, &st) < 0 || st.st_size == 0) goto error;
	m_devinode = std::make_pair(st.st_dev, st.st_ino);
	it = m_mmaps->find(m_devinode);
	if (it == m_mmaps->end())
	{
		usage.address = ::mmap(0, st.st_size, prot, flags, usage.fd, 0);
		if (usage.address == MAP_FAILED) goto error;
		usage.length = st.st_size;
#ifdef MAP_POPULATE
		if (flags & MAP_POPULATE)
			madvise((void *)usage.address, usage.length, MADV_WILLNEED);
#endif
		it = m_mmaps->insert(std::make_pair(m_devinode, usage)).first;
	}
	else close(usage.fd);
	it->second.used ++;
	pthread_mutex_unlock(&m_mutex);
	return;

error:
	if (usage.fd >= 0) close(usage.fd);
	if (m_mmaps && m_mmaps->empty()) { delete m_mmaps; m_mmaps = 0; }
	pthread_mutex_unlock(&m_mutex);
	throw int(-1);
}

template <class T>
MMap<T>::MMap(const MMap<T> &from)
		: m_filename(from.m_filename), m_devinode(from.m_devinode)
{
	if (m_filename.empty()) return;

	pthread_mutex_lock(&m_mutex);
	typename UsageMap::iterator it = m_mmaps->find(m_devinode);
	it->second.used ++;
	pthread_mutex_unlock(&m_mutex);
}

template <class T>
MMap<T> & MMap<T>::operator =(const MMap<T> &from)
{
	if (&from == this) return *this;

	if (!m_filename.empty())
	{
		pthread_mutex_lock(&m_mutex);
		typename UsageMap::iterator it = m_mmaps->find(m_devinode);
		it->second.used --;
		if (it->second.used == 0)
		{
			munmap((void *)it->second.address, it->second.length);
			close(it->second.fd);
			m_mmaps->erase(it);
		}
		if (m_mmaps && m_mmaps->empty()) { delete m_mmaps; m_mmaps = 0; }
		pthread_mutex_unlock(&m_mutex);
	}

	m_filename = from.m_filename;
	m_devinode = from.m_devinode;

	if (!m_filename.empty())
	{
		pthread_mutex_lock(&m_mutex);
		typename UsageMap::iterator it = m_mmaps->find(m_devinode);
		it->second.used ++;
		pthread_mutex_unlock(&m_mutex);
	}

	return *this;
}

template <class T>
MMap<T>::~MMap()
{
	if (m_filename.empty()) return;

	pthread_mutex_lock(&m_mutex);
	typename UsageMap::iterator it = m_mmaps->find(m_devinode);
	it->second.used --;
	if (it->second.used == 0)
	{
		munmap((void *)it->second.address, it->second.length);
		close(it->second.fd);
		m_mmaps->erase(it);
	}
	if (m_mmaps && m_mmaps->empty()) { delete m_mmaps; m_mmaps = 0; }
	pthread_mutex_unlock(&m_mutex);
}

template <class T>
std::pair<const void *, size_t> MMap<T>::mmap() const
{
	if (m_filename.empty()) return std::pair<const void *, size_t>();

	pthread_mutex_lock(&m_mutex);
	typename UsageMap::iterator it = m_mmaps->find(m_devinode);
	std::pair<const void *, size_t> result(it->second.address, it->second.length);
	pthread_mutex_unlock(&m_mutex);

	return result;
}

template <class T>
pthread_mutex_t MMap<T>::m_mutex = PTHREAD_MUTEX_INITIALIZER;
template <class T>
typename MMap<T>::UsageMap *MMap<T>::m_mmaps = 0;

#ifndef FT2_NAMESPACE
} // namespace ft2
#else
} // namespace FT2_NAMESPACE
#endif

#endif // __MMAP_2_H__
