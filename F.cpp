#include <algorithm>
#include <cmath>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>

#define DEBUG 0

// Алгоритм, чем-то похожий на алгоритм Месселя-Леймера.
uint64_t CountPrimesBelowEq(uint64_t boundary) {
	// Подробнее в F_notes.txt. Техника оказалась интересная.
	
	const auto precalc_sieve = [](const uint64_t max_prime, std::vector<uint64_t>& primes, std::vector<uint64_t>& lowest_prime_div) {
		lowest_prime_div.assign(max_prime + 1, 0);
		primes.clear();
		
		std::vector<bool> is_prime(max_prime + 1, true);
		
		for (uint64_t i = 2; i <= max_prime; ++i) {
			if (is_prime[i]) {
				lowest_prime_div[i] = primes.size();
				primes.push_back(i);
			}
			
			for (size_t j = 0; primes[j] * i <= max_prime; ++j) {
				uint64_t value = primes[j] * i;
				is_prime[value] = false;
				lowest_prime_div[value] = j;
				if (j == lowest_prime_div[i]) {
					break;
				}
			}
		}
		
		primes.shrink_to_fit();
		lowest_prime_div.shrink_to_fit();
	};
	
	// Если n = 1, то y = 0 после округления. При n <= 9 воспользуемся обычным просеиванием.
	// Ограничим на самомо деле 1000 (при проверке не давало правильный ответ для n = 10,
	//   т.к используем теоремы о средней величине k-го простого, а они начинают работать
	//   для больших номеров простых).
	
	static constexpr uint64_t kTrivialBoundary = 9;//1000;
	
	if (boundary <= kTrivialBoundary) {
		std::vector<uint64_t> primes;
		std::vector<uint64_t> lowest_prime_div;
		precalc_sieve(boundary, primes, lowest_prime_div);
		return primes.size();
	}
	
	// Получаем не y, а n / y. Это оптимальная величина для n = 10^11 и n = 10^12.
	auto n_div_y = static_cast<uint64_t>(powl(static_cast<long double>(boundary), 0.64));
	uint64_t y = (boundary + n_div_y - 1) / n_div_y;
	
	std::vector<uint64_t> primes;
	std::vector<uint64_t> lowest_prime_div;
	precalc_sieve(n_div_y, primes, lowest_prime_div);
	
	// pi(n) = phi(n, pi(y)) + pi(y) - F - 1.
	uint64_t pi_value = 0;
		
	// Считаем pi(y). y <= n / y, т.к. y <= sqrt(n).
	auto pi_y = static_cast<uint64_t>(std::upper_bound(primes.begin(), primes.end(), y) - primes.begin());
	
	// Учитываем.
	pi_value += pi_y;
	
	// Вычитаем F. Если будет переполнение вниз, не страшно, т.к. реальное значение помещается
	//   в uint64_t и мы просто считаем его по модулю.
	//uint64_t F_value = 0;
	for (size_t i = pi_y, j = primes.size() - 1; i < primes.size(); ++i) {
		while (j - 1 >= i && primes[i] * primes[j] > boundary) {
			j -= 1;
		}
		
		// printf("i = %zu, j = %zu, primes[i] = %" PRIu64 ", primes[j] = %" PRIu64 ".\n", i, j, primes[i], primes[j]);
		
		// Перепроверяем условие, т.к. primes[i] -- меньший сомножитель,
		//   для него больший мог не найтись.
		if (i <= j && primes[i] * primes[j] <= boundary) {
			//for (size_t k = i; k <= j; ++k) {
				// printf("F includes %" PRIu64 ".\n", primes[i] * primes[k]);
			//}
			
			//F_value  += static_cast<uint64_t>(j - i + 1);
			pi_value -= static_cast<uint64_t>(j - i + 1);
		}
	}
	
	// printf("#2. F_value = %" PRIu64 ".\n", F_value);
	#if DEBUG
		if (boundary <= 100'000) {
			std::vector<uint64_t> primes1;
			std::vector<uint64_t> lowest_prime_div1;
			precalc_sieve(100'000, primes1, lowest_prime_div1);
			uint64_t F_real = 0;
			for (uint64_t i = 2; i <= boundary; ++i) {
				if (primes1[lowest_prime_div1[i]] != i && lowest_prime_div1[i] + 1 > pi_y) {
					F_real += 1;
					// printf("F includes %" PRIu64 ", min_div = %" PRIu64 ".\n", i, primes1[lowest_prime_div1[i]]);
				}
			}
			printf("#2. F_real = %" PRIu64 ".\n", F_real);
		}
	#endif
	
	// Считаем phi(n, pi(y)).
	
	struct Query {
		uint64_t phi_boundary;
		uint64_t prime_index;
		int sign;
	};
	std::vector<Query> queries;

	// Без комментария в начале функции сложно понять, посмотрите туда.
	std::function<void(uint64_t, uint64_t, int)> phi = [&](uint64_t phi_boundary, uint64_t prime_index, int sign) {
		if (phi_boundary == 0) {
			return;
		}
	
		if (phi_boundary <= n_div_y) {
			queries.push_back(Query{phi_boundary, prime_index, sign});
			return;
		}
		
		/*
			phi(phi_boundary, prime_index - 1, sign);
			phi(phi_boundary / primes[prime_index - 1], prime_index - 1, -sign);
		*/
		// Чтобы съэкономить стек, один вызов будем оставлять на этом уровне.
		//   Тогда при рекурсивном вызове значение phi_boundary уменьшается
		//   как минимум вдвое. Что дает логирифмическую глубину.
		// Альтернативно можно использовать свой стек, а не аппаратный,
		//   вычислать с помощью while на этом стеке, сохраняя состояния
		//   (условный rip) и значения аргументов.
		
		while (prime_index > 1) {
			phi(phi_boundary / primes[prime_index - 1], prime_index - 1, -sign);
			prime_index -= 1;
		}
		
		// printf("phi_boundary = %" PRIu64 ".\n", phi_boundary);
		
		// prime_index = 1, это просто количество нечетных чисел от 1 до phi_boundary.
		// Эти значения будут не запросами, а сразу раскроем и добавим к результату.
		if (sign > 0) {
			pi_value += (phi_boundary + 1) / 2;
		} else {
			pi_value -= (phi_boundary + 1) / 2;
		}
	};
	phi(boundary, pi_y, +1);
	queries.shrink_to_fit();

	std::sort(queries.begin(), queries.end(), [&](const Query& lhs, const Query& rhs) {		
		if (lhs.phi_boundary != rhs.phi_boundary) {
			return lhs.phi_boundary < rhs.phi_boundary;
		}

		if (lhs.prime_index != rhs.prime_index) {
			return lhs.prime_index < rhs.prime_index;
		}

		return lhs.sign < rhs.sign;
	});
	
	std::vector<uint64_t> fwt(primes.size(), 0);
	auto fwt_add = [&fwt](size_t pos, uint64_t value) {
		for (; pos < fwt.size(); pos |= pos + 1) {
			fwt[pos] += value;
		}
	};
	auto fwt_sum = [&fwt](size_t prefix_len) -> uint64_t {
		uint64_t result = 0;
		for (; prefix_len > 0; prefix_len = (prefix_len - 1) & prefix_len) {
			result += fwt[prefix_len - 1];
		}
		return result;
	};
		
	size_t last_added_int = 1;
	for (const Query& query: queries) {
		while (last_added_int < query.phi_boundary) {
			last_added_int += 1;
			fwt_add(lowest_prime_div[last_added_int], +1);
		}
		
		uint64_t query_result = 0;
		if (query.prime_index - 1 >= primes.size()) {
			query_result = 0;
		} else {
			// Количество чисел <= phi_boundary - количество тех, у кого
			//   есть простое в разложении не больше (prime_index-1)-го простого.
			query_result = query.phi_boundary - fwt_sum(query.prime_index);
		}
		
		if (query.sign > 0) {
			pi_value += query_result;
		} else {
			pi_value -= query_result;
		}
	}
	
	// Вычитаем единицу.
	pi_value -= 1;

	return pi_value;
}

int main() {
	uint64_t boundary = 0;
	scanf("%" SCNu64, &boundary);
	
	printf("%" PRIu64 "\n", CountPrimesBelowEq(boundary));

	return 0;
}