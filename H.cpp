/*
Быстрое преобразование Фурье.

Мы хотим научиться считать A(x) * B(x), где A(x) и B(x) -- многочлены. У нас есть две формы
  записи многочлена степени n:
  в виде (n+1)-го коэффициента и в виде (n+1)-ой точки.
  
В первом виде мы можем получитб коэффициенты за O(n), вычислить в точке за O(n), посчитать
  произведение двух многочленов степени n за O(n^2).
    
Во втором виде мы можем получить коэффициенты за O(?) (сколько-то, есть разные интерполяции),
  вычислить в точке за O(?) (нужна интерполяция), посчитать произведение двух многочленов
  степени n за O(n) (если для многочленов возьмём 2n+1 точку, т.к. у произведения степень не более 2n).
  
Хотелось бы вычислить в точках, перемножить поточечно, а затем получить коэффициенты обратно.
  
Точки можем выбирать произвольно, просто нужно k разных. k > n. Для произведения многочленов
  степени n надо k взять равным 2n+1 (многочлен однозначно задаётся степень+1 точками).
  
Будем брать корни k-ой степени из комплексной единицы. Польза в том, что тогда одни корни
  получаются из других. И без ограничения общности степень многочлена будем считать
  степенью двойки - 1: если не так, просто дополним массив нулями до размера, равного степени
  двойки - 1, как мы увидим, асимптотика от этого не пострадает.
  Это нужно для того, чтобы коэффициентов всегда было чётно при делении на две половины, или
  один.

Разделим многочлен на четные и нечетные коэффициенты.
  A(x)   = a_0 + a_1 x + a_2 x^2 + ... + a_{2t} x^{2t},
  A_0(x) = a_0 + a_2 x + a_4 x^2 + ... + a_{2t} x^t,
  A_1(x) = a_1 + a_3 x + a_5 x^2 + ... + a_{2t - 1} x^t.
  
Заметим, что
  A(x) = A_0(x^2) + x A_1(x^2).

Вычислим A_0 и A_1 в точках. Можем рекурсивно, у них степень в два раза меньше.
  Как теперь получить A(x)? Если мы объединим A_0 и A_1 в A за O(n), то
  получим рекуррентное соотношение T(n) = 2 T(n / 2) + O(n), его решением
  является O(n log(n)), а ещё просто можно представить дерево, высота log(n),
  т.к. n убывает в два раза при спуске в ребенка, на каждом уровне делается
  O(n) операций.
  
Пусть w_n -- первообразный комплексный корень n-ой степени из единицы.
  
...
*/

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

// std::complex => >1.587sec - TL
// Свой complex =>  0.987sec - OK
// std::complex без дополнительных флагов при компиляции
//   будет проверять double на nan, из-за этого замедление.
struct MyComplex {
	MyComplex()
	  : re_(0), im_(0) {}
	  
	MyComplex(const MyComplex& other) = default;

	explicit MyComplex(double value)
	  : re_(value), im_(0) {}
	  
	explicit MyComplex(double re, double im)
	  : re_(re), im_(im) {}

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
	
	static const int kStraight = 0;
	static const int kReverse  = 1;
	
	// Рекурсивная реализация. Работает, но не проходит по времени.
	// Попробуем сделать без выделения новых массивов и без рекурсии.
	void operator()(std::vector<DataType>& items, int direction = kStraight) {
		BitReversePermute(items);
		int log2_length = __builtin_ctzll(static_cast<uint64_t>(items.size()));
		
		static const double kPi = std::atan(1.0) * 4;
		
		for (int iteration = 0; iteration < log2_length; ++iteration) {
			int len = 1 << iteration;
			DataType primitive_root;
			if (direction == kStraight) {
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
		if (direction == kReverse) {
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
	
	fft(lhs_complex, FFT::kReverse);
	
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

	std::reverse(polynome_first.begin(), polynome_first.end());
	std::reverse(polynome_second.begin(), polynome_second.end());
	std::vector<int32_t> product = MultiplyPolynomes(polynome_first, polynome_second);
	std::reverse(product.begin(), product.end());
	
	printf("%zu ", product.size() - 1);
	for (int32_t coefficient: product) {
		printf("%" PRId32 " ", coefficient);
	}
	printf("\n");

	return 0;
}