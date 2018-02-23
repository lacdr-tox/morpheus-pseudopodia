//
// Created by gerhard on 23-2-18.
//

#ifndef MORPHEUS_CPP_FUTURE_H
#define MORPHEUS_CPP_FUTURE_H

#if __cplusplus >= 201703L
#warning C++17 already defines these functions
#endif

namespace cpp17 {

	// clamp: http://en.cppreference.com/w/cpp/algorithm/clamp
	template<class T, class Compare>
	constexpr const T &clamp(const T &v, const T &lo, const T &hi, Compare comp) {
		return assert(!comp(hi, lo)),
				comp(v, lo) ? lo : comp(hi, v) ? hi : v;
	}

	template<class T>
	constexpr const T &clamp(const T &v, const T &lo, const T &hi) {
		return clamp(v, lo, hi, std::less<T>());
	}
}

#endif //MORPHEUS_CPP_FUTURE_H
