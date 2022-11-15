#pragma once
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <iostream>
#include <complex>
#include <cmath>

namespace pbd {
	// Bounding box type ripped from ez::geo.
	// For once I'm stealing code from myself.
	template<glm::length_t L, typename scalar_t>
	struct BBox {
		static_assert(L > 1, "pbd::BBox requires a number of dimensions greater than one!");
		static_assert(L <= 4, "pbd::BBox is not defined for dimensions greater than four!");
		static_assert(std::is_floating_point_v<scalar_t>, "pbd::BBox requires an arithmetic value type!");

		using T = scalar_t;
		using vec_t = glm::vec<L, scalar_t>;
		static constexpr int Components = L;
		using self_t = BBox<L, scalar_t>;
	private:
		static constexpr scalar_t Eps() {
			if constexpr (std::is_same_v<scalar_t, float>) {
				return 1e-5f;
			}
			else {
				return 1e-10;
			}
		}
		static constexpr vec_t EpsV() {
			return vec_t(Eps());
		}
	public:
		static self_t Between(const vec_t& p0, const vec_t& p1) noexcept {
			return self_t(glm::min(p0, p1), glm::max(p0, p1));
		};
		static self_t Merge(const self_t& a, const self_t& b) noexcept {
			self_t tmp = a;
			return tmp.merge(b);
		};
		static bool Overlaps(const self_t& a, const self_t& b) noexcept {
			return a.overlaps(b);
		}

		BBox() noexcept
			: min{ T(0) }
			, max{ T(0) }
		{};
		BBox(const vec_t& _min, const vec_t& _max) noexcept
			: min(_min)
			, max(_max)
		{};

		template<typename U>
		BBox(const BBox<L, U>& other) noexcept
			: min{ other.min }
			, max{ other.max }
		{};

		~BBox() noexcept = default;
		BBox(const BBox&) noexcept = default;
		BBox(BBox&&) noexcept = default;
		BBox& operator=(const BBox&) noexcept = default;
		BBox& operator=(BBox&&) noexcept = default;

		T width() const noexcept {
			return max[0] - min[0];
		}
		T height() const noexcept {
			return max[1] - min[1];
		}
		T area() const noexcept {
			return width() * height();
		}

		template<int K = L, typename = std::enable_if_t<(K > 2)>>
		T depth() const noexcept {
			return max[2] - min[2];
		}
		template<int K = L, typename = std::enable_if_t<(K == 3)>>
		T volume() const noexcept {
			return width() * height() * depth();
		}

		vec_t size() const noexcept {
			return max - min;
		}

		vec_t center() const noexcept {
			// Divide by two instead of multiply by 0.5, since the T variable could be an integer type.
			return (min + max) / T(2);
		};

		self_t& translate(const vec_t& offset) noexcept {
			min += offset;
			max += offset;

			return *this;
		};
		self_t& centerTo(const vec_t& point) noexcept {
			return translate(point - center());
		};

		self_t& expand(const T& amount) noexcept {
			min -= amount;
			max += amount;

			return *this;
		};
		self_t& expand(const vec_t& amount) noexcept {
			min -= amount;
			max += amount;

			return *this;
		};
		self_t& expand(const vec_t& minv, const vec_t& maxv) noexcept {
			min -= minv;
			max += maxv;

			return *this;
		};
		self_t expanded(const T& amount) const noexcept {
			self_t copy = *this;
			return copy.expand(amount);
		};
		self_t expanded(const vec_t& amount) const noexcept {
			self_t copy = *this;
			return copy.expand(amount);
		};
		self_t expanded(const vec_t& minv, const vec_t& maxv) const noexcept {
			self_t copy = *this;
			return copy.expand(minv, maxv);
		};


		self_t& contract(const vec_t& minv, const vec_t& maxv) noexcept {
			min += minv;
			max -= maxv;
			vec_t c = center();

			min = glm::min(c, min);
			max = glm::max(c, max);

			return *this;
		};
		self_t& contract(const vec_t& amount) noexcept {
			return contract(amount, amount);
		};
		self_t& contract(const T& amount) noexcept {
			return contract(vec_t(amount), vec_t(amount));
		};
	
		self_t contracted(const T& amount) const noexcept {
			self_t copy = *this;
			return copy.contract(amount);
		};
		self_t contracted(const vec_t& amount) const noexcept {
			self_t copy = *this;
			return copy.contract(amount);
		};
		self_t contracted(const vec_t& minv, const vec_t& maxv) const noexcept {
			self_t copy = *this;
			return copy.contract(minv, maxv);
		};
		
		template<typename F>
		self_t& scale(const glm::vec<L, F>& factor) noexcept {
			min *= factor;
			max *= factor;
			return *this;
		};
		template<typename F>
		self_t& scale(const F& factor) noexcept {
			return scale(glm::vec<L, F>(factor));
		};
		template<typename F>
		self_t& scale(const vec_t& c, const glm::vec<L, F>& factor) noexcept {
			min = (min - c) * factor + c;
			max = (max - c) * factor + c;

			return *this;
		};
		template<typename F>
		self_t& scale(const vec_t& c, const F& factor) noexcept {
			return scale(c, glm::vec<L, F>(factor));
		};
		
		template<typename F>
		self_t scaled(const F& factor) const noexcept {
			self_t copy = *this;
			return copy.scale(factor);
		};
		template<typename F>
		self_t scaled(const glm::vec<L, F>& factor) const noexcept {
			self_t copy = *this;
			return copy.scale(factor);
		};
		template<typename F>
		self_t scaled(const vec_t& c, const F& factor) const noexcept {
			self_t copy = *this;
			return copy.scale(c, factor);
		};
		template<typename F>
		self_t scaled(const vec_t& c, const glm::vec<L, F>& factor) const noexcept {
			self_t copy = *this;
			return copy.scale(c, factor);
		};


		self_t& merge(const self_t& other) noexcept {
			max = glm::max(max, other.max);
			min = glm::min(min, other.min);

			return *this;
		};
		self_t& merge(const vec_t& point) noexcept {
			max = glm::max(max, point);
			min = glm::min(min, point);

			return *this;
		};

		self_t merged(const self_t& other) const noexcept {
			self_t copy = *this;
			copy.merge(other);
			return copy;
		};
		self_t merged(const vec_t& point) const noexcept {
			self_t copy = *this;
			copy.merge(point);
			return copy;
		};

		// This function always succeeds.
		vec_t toWorld(const vec_t& point) const noexcept {
			return point * (max - min) + min;
		};
		// Check isValid prior to calling this function if theres a chance the rect could be invalid.
		vec_t toLocal(const vec_t& point) const noexcept {
			return (point - min) / (max - min);
		};

		bool isValid() const noexcept {
			return glm::all(glm::greaterThan(max - min, EpsV()));
		};

		bool contains(const vec_t& point) const noexcept {
			return 
				glm::all(glm::lessThan(min, point)) && 
				glm::all(glm::lessThan(point, max));
		};

		bool contains(const self_t& other) const noexcept {
			return (
				glm::all(glm::lessThanEqual(other.max, max)) &&
				glm::all(glm::greaterThanEqual(other.min, min))
			);
		};

		bool overlaps(const self_t & other) const noexcept {
			return !(
				glm::any(glm::greaterThan(other.min, max)) ||
				glm::any(glm::lessThan(other.max, min))
			);
		}

		bool operator==(const self_t& other) const noexcept {
			return
				glm::all(glm::lessThan(glm::abs(min - other.min), EpsV())) &&
				glm::all(glm::lessThan(glm::abs(max - other.max), EpsV()));
		}
		bool operator!=(const self_t& other) const noexcept {
			return
				glm::any(glm::greaterThan(glm::abs(min - other.min), EpsV())) ||
				glm::any(glm::greaterThan(glm::abs(max - other.max), EpsV()));
		}

		template<int K = L, typename = std::enable_if_t<(K == 2)>>
		self_t rotated(const T& angle) const noexcept {
			return rotated(std::complex<T>(std::cos(angle), std::sin(angle)));
		}

		template<int K = L, typename = std::enable_if_t<(K == 2)>>
		self_t rotated(const std::complex<T>& rot) const noexcept {
			auto rotate = [&](const glm::tvec2<T> & pos) {
				std::complex<T> tmp{pos.x, pos.y};
				tmp = tmp * rot;
				return glm::tvec2<T>(tmp.real(), tmp.imag());
			};

			glm::tvec2<T> tmp = rotate(min);
			self_t result(tmp, tmp);
			result.merge(rotate(glm::tvec2<T>(min.x, max.y)));
			result.merge(rotate(glm::tvec2<T>(max.x, min.y)));
			result.merge(rotate(max));

			return result;
		}

		template<int K = L, typename = std::enable_if_t<(K == 3)>>
		self_t rotated(const T& angle, const glm::tvec3<T>& axis) const noexcept {
			return rotated(glm::angleAxis(angle, axis));
		}

		template<int K = L, typename = std::enable_if_t<(K == 3)>>
		self_t rotated(const glm::tquat<T> & rot) const noexcept {
			glm::tquat<T> crot = glm::conjugate(rot);

			auto rotate = [&](const glm::tvec3<T> & pos) {
				glm::tquat<T> tmp;
				tmp.x = pos.x;
				tmp.y = pos.y;
				tmp.z = pos.z;
				tmp.w = T(0);

				tmp = rot * tmp * crot;
				return glm::tvec3<T>(tmp.x, tmp.y, tmp.z);
			};

			glm::tvec3<T> tmp = rotate(min);
			self_t result(tmp, tmp);
			result.merge(rotate(glm::tvec3<T>(min.x, min.y, max.z)));
			result.merge(rotate(glm::tvec3<T>(min.x, max.y, min.z)));
			result.merge(rotate(glm::tvec3<T>(min.x, max.y, max.z)));
			
			result.merge(rotate(glm::tvec3<T>(max.x, min.y, min.z)));
			result.merge(rotate(glm::tvec3<T>(max.x, min.y, max.z)));
			result.merge(rotate(glm::tvec3<T>(max.x, max.y, min.z)));
			result.merge(rotate(glm::tvec3<T>(max.x, max.y, max.z)));

			return result;
		}


		vec_t min, max;
	};
}

template<glm::length_t L, typename scalar_t>
std::ostream& operator<<(std::ostream& os, const pbd::BBox<L, scalar_t>& bbox) {
	os << "BBox{ min[" << bbox.min.x << ", " << bbox.min.y << "], max[";
	os << bbox.max.x << ", " << bbox.max.y << "] }";
	return os;
};