#ifndef ZAIMONI_STL_CACHE_HPP
#define ZAIMONI_STL_CACHE_HPP 1

#include <cstdint>
#include <map>

namespace zaimoni {

	class I_erase {
	public:
		virtual ~I_erase() = default;
		virtual void erase(const void* src) = 0;
	};

	template<class T>
	class cache : public I_erase {
	private:
		std::map<uintptr_t, T> _cache;
	public:
		cache() = default;
		cache(const cache& src) = delete;
		cache(cache&& src) = delete;
		virtual ~cache() = default;
		cache& operator=(const cache& src) = delete;
		cache& operator=(cache&& src) = delete;

		const T* lookup(const void* src) const {
			decltype(auto) stage = _cache.find(reinterpret_cast<uintptr_t>(src));
			if (stage == _cache.end()) return nullptr;
			return &stage->second;
		}

		void update(const void* src, T val) { _cache.insert_or_assign(reinterpret_cast<uintptr_t>(src), std::move(val)); }
		void erase(const void* src) override { _cache.erase(reinterpret_cast<uintptr_t>(src)); }
	};

}

#endif
