// Чтобы узнать больше, смотрите H_notes.txt

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>

#define DEBUG 0

/**
 * \brief   Complex number class.
 *
 * \details std::complex => >1.587sec - TL,
 *          Custom complex (without any tricks) =>  0.987sec - OK,
 *          std::complex without special flags for compilation
 *          will check double for being nan, thus slow computations.
 */
struct MyComplex {
	/**
	 * \brief   Creates complex number with zero real and imaginary parts.
	 */
	MyComplex()
	  : re_(0), im_(0) {}
	  
	/**
	 * \brief   Creates complex number with specified real and imaginary parts.
	 *
	 * \param[in]     re   Real part, double.
	 * \param[in]     im   Imaginary part, double.
	 */
	explicit MyComplex(double re, double im)
	  : re_(re), im_(im) {}

	/**
	 * \brief   Creates complex number with specified real part and zero imaginary part.
	 *
	 * \note    Suitable as a conversion function for doubles, but is explicit to avoid
	 *          bugs.
	 * 
	 * \param[in]     re   Real part, double.
	 */
	explicit MyComplex(double value)
	  : re_(value), im_(0) {}

	MyComplex(const MyComplex& other) = default;

	MyComplex& operator*=(MyComplex& rhs) {
		double re1 = re_ * rhs.re_ - im_ * rhs.im_;
		double im1 = re_ * rhs.im_ + im_ * rhs.re_;
		
		re_ = re1;
		im_ = im1;
		
		return *this;
	}
	
	MyComplex& operator/=(double value) {
		re_ /= value;
		im_ /= value;
		
		return *this;
	}

	MyComplex operator+(MyComplex& rhs) {
		double re = re_ + rhs.re_;
		double im = im_ + rhs.im_;
		
		return MyComplex(re, im);
	}
	
	MyComplex operator-(MyComplex& rhs) {
		double re = re_ - rhs.re_;
		double im = im_ - rhs.im_;
		
		return MyComplex(re, im);
	}

	MyComplex& operator=(const MyComplex& other) = default;

	double Real() {
		return re_;
	}

	double re_;
	double im_;
};

struct FFT {
	using DataType = MyComplex; //std::complex<double>;
	
	enum class Direction {
		kStraight,
		kReverse
	};
	
	// Рекурсивная реализация. Работает, но не проходит по времени.
	// Попробуем сделать без выделения новых массивов и без рекурсии.
	void operator()(std::vector<DataType>& items, Direction direction = Direction::kStraight) {
		BitReversePermute(items);
		int log2_length = __builtin_ctzll(static_cast<uint64_t>(items.size()));
		
		static const double kPi = std::atan(1.0) * 4;
		
		for (int iteration = 0; iteration < log2_length; ++iteration) {
			int len = 1 << iteration;
			DataType primitive_root;
			if (direction == Direction::kStraight) {
				primitive_root = DataType(std::cos(kPi / len), std::sin(kPi / len));
			} else {
				primitive_root = DataType(std::cos(kPi / len), -std::sin(kPi / len));
			}
			for (size_t i = 0; i < items.size(); i += 2 * len) {
				DataType current_root(1, 0);
				for (int j = 0; j < len; ++j) {
					DataType offset = items[i + len + j];
					offset *= current_root;
					items[i + len + j] = items[i + j] - offset;
					items[i + j] = items[i + j] + offset;
					current_root *= primitive_root;
				}
			}
		}
		if (direction == Direction::kReverse) {
			for (DataType& item: items) {
				item /= static_cast<double>(items.size());
			}
		}

	}
	
	template<typename T>
	static void BitReversePermute(std::vector<T>& items) {
		int log2_of_length = __builtin_ctzll(static_cast<uint64_t>(items.size()));
		std::vector<size_t> reversed_binary_repr(items.size());
		reversed_binary_repr[0] = 0;
		for (size_t i = 1; i < items.size(); ++i) {
			reversed_binary_repr[i] = (
				((i & 1) << (log2_of_length - 1)) | (reversed_binary_repr[i >> 1] >> 1)
			);
		}
		for (size_t j = 0; j < items.size(); ++j) {
			if (j < static_cast<size_t>(reversed_binary_repr[j])) {
				std::swap(items[j], items[reversed_binary_repr[j]]);
			}
		}
	}
};

std::vector<int32_t> MultiplyPolynomes(const std::vector<int32_t>& lhs, const std::vector<int32_t>& rhs) {
	std::vector<MyComplex> lhs_complex(lhs.size());
	for (size_t i = 0; i < lhs.size(); ++i) {
		lhs_complex[i] = MyComplex(static_cast<double>(lhs[i]));
	}
	std::vector<MyComplex> rhs_complex(rhs.size());
	for (size_t i = 0; i < rhs.size(); ++i) {
		rhs_complex[i] = MyComplex(static_cast<double>(rhs[i]));
	}
	
	// lhs -- многочлен степени a-1, lhs_size = a.
	// rhs -- многочлен степени b-1, rhs_size = b.
	// Нужно вычислить оба хотя бы в a-1+b-1+1 = a + b - 1
	//   точке, потому расширяем массивы.
	// И ещё нужно, чтобы размер был степенью двойки.
	size_t new_size = 1;
	while (new_size < lhs_complex.size() + rhs_complex.size()) {
		new_size *= 2;
	}
	
	// Расширяем массивы.
	lhs_complex.reserve(new_size);
	while (lhs_complex.size() < new_size) {
		lhs_complex.emplace_back(0);
	}
	rhs_complex.reserve(new_size);
	while (rhs_complex.size() < new_size) {
		rhs_complex.emplace_back(0);
	}
	
	FFT fft;
	
	fft(lhs_complex);
	
	#if DEBUG
		std::cout << "lhs_complex = [";
		for (size_t i = 0; i < lhs_complex.size(); ++i) {
			std::cout << lhs_complex[i] << ", ";
		}
		std::cout << "].\n";
	#endif
	
	fft(rhs_complex);
	
	#if DEBUG
		std::cout << "rhs_complex = [";
		for (size_t i = 0; i < rhs_complex.size(); ++i) {
			std::cout << rhs_complex[i] << ", ";
		}
		std::cout << "].\n";
	#endif

	// Перемножаем значения в точках, сохраним в lhs_complex,
	//   в какой-то массив надо сохранить, чтобы восстановить
	//   коэффициенты.
	for (size_t i = 0; i < new_size; ++i) {
		lhs_complex[i] *= rhs_complex[i];
	}
	
	fft(lhs_complex, FFT::Direction::kReverse);
	
	#if DEBUG
		std::cout << "lhs_complex = [";
		for (size_t i = 0; i < lhs_complex.size(); ++i) {
			std::cout << lhs_complex[i] << ", ";
		}
		std::cout << "].\n";
	#endif

	std::vector<int32_t> result(new_size, 0);
	assert(lhs_complex.size() == new_size); // Реализация FFT не меняет размер.
	for (size_t i = 0; i < lhs_complex.size(); ++i) {
		result[i] = static_cast<int32_t>(std::round(lhs_complex[i].Real()));
	}
	
	#if DEBUG
		std::cerr << "result = [";
		for (size_t i = 0; i < lhs_complex.size(); ++i) {
			std::cerr << result[i] << ", ";
		}
		std::cerr << "].\n";
	#endif

	// Удаляем ведущие нули. Если получился просто ноль, остановимся,
	//   последний одночлен не проверяем.
	while (result.size() >= 2 && result.back() == 0) {
		result.pop_back();
	}
	
	return result;
}

// Перемножает многочлены, у которых старший коэффициент в начале массива, а не младший.
std::vector<int32_t> MultiplyBigEndianPolynomes(std::vector<int32_t>& polynome_first, std::vector<int32_t>& polynome_second) {
	std::reverse(polynome_first.begin(), polynome_first.end());
	std::reverse(polynome_second.begin(), polynome_second.end());
	std::vector<int32_t> product = MultiplyPolynomes(polynome_first, polynome_second);
	std::reverse(product.begin(), product.end());
	
	return product;
}

int main() {
	size_t degree_first = 0;
	scanf("%zu", &degree_first);
	
	std::vector<int32_t> polynome_first(degree_first + 1, 0);
	for (size_t i = 0; i <= degree_first; ++i) {
		scanf("%" SCNd32, &polynome_first[i]);
	}
	
	size_t degree_second = 0;
	scanf("%zu", &degree_second);
	
	std::vector<int32_t> polynome_second(degree_second + 1, 0);
	for (size_t i = 0; i <= degree_second; ++i) {
		scanf("%" SCNd32, &polynome_second[i]);
	}
	
	std::vector<int32_t> product = MultiplyBigEndianPolynomes(polynome_first, polynome_second);
	
	printf("%zu ", product.size() - 1);
	for (int32_t coefficient: product) {
		printf("%" PRId32 " ", coefficient);
	}
	printf("\n");

	return 0;
}