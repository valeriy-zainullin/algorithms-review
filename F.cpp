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
	// Обозначим за pi(n) -- количество простых от 1 до n,
	//   phi(n, b) -- количество чисел от 1 до n, у которых
	//   в разложении на простые все простые больше b-го
	//   простое (индексация с единицы), единица тоже
	//   считается (там простых в разложении нет, потому условие верно).
	// Выберем y в [cbrt(n); sqrt(n)] (окажется, оптимально взять
	//   y = cbrt(n) (log(n)) ^ (2/3), подробнее тут:
	//   https://codeforces.com/blog/entry/91632).
	// pi(n) = phi(n, pi(y)) + pi(y) - F - 1.
	//   Почему? phi(n, pi(y)) считает количество простых от
	//   pi(y)-го простого до n и составные, у которых каждое
	//   простое в разложении > pi(y)-го простого.
	//   Кроме этого от 1 до n есть составные, у которых все
	//   простые больше pi(y)-го простого.
	//   Как элегантно исключить эти числа? y в [cbrt(n); sqrt(n)],
	//   потому у любого такого составного числа сумма степеней
	//   простых в разложении не более двух, иначе как минимум три,
	//   но любое простое больше primes[pi(y) - 1] ~=
	//   pi(y) ln(pi(y)) >= y / ln(y) ln(pi(y)) ~= y / ln(y)
	//   (ln(y) - ln(ln(y)) >= cbrt(n) (теорема о распределении
	//   простых чисел: примерное значение n-го простого,
	//   эквивалентная величина для pi(k) (эквивалентные величины
	//   оцениваюся через друг друга с некоторой константой, потому
	//   всё ок, надо наш алгоритм использовать для больших n, а для
	//   маленьких использовать наивный), тогда само число
	//   k > y^3 >= cbrt(n)^3 = n, противоречие, т.к. рассмотрели
	//   число от 1 до n. Значит, оба сомножителя не более sqrt(n),
	//   они у нас будут, т.к. мы простые считаем от 1 до n / y >=
	//   sqrt(n). Тогда посчитаем простые числа от 1 до n / y обычным
	//   просеиванием за O(n / y), двумя указателями посчитаем F за
	//   O(n / y). Все нужные числа имеют два простых сомножителя,
	//   потому их не пропустим, с другой стороны все числа, которые
	//   вычислили входят в нужные. Ровно их и посчитаем, получается.
	//   Вычтем единицу, т.к. считали единицу в phi. Простые
	//   не больше y мы тоже посчитали, pi(y).
	// Как считать phi(n, pi(y))? pi(y) посчитать можем, обычным
	//   просеиванием. Осталось только посчитать phi.
	//   Функцию phi(m, b) при m <= max { y, n / y } (= n / y, т.к.
	//   n / y > sqrt(n), y <= sqrt(n)) мы можем посчитать с помощью
	//   обычного просеивания. У нас получится массив наименьших простых
	//   делителей для каждого числа, нужно посчитать числа i <= m, у
	//   которых lowest_prime_div[i] > primes[b - 1] (b -- номер
	//   простого в 1-индексации).
	// Однако, m у нас большое изначально. Для phi(m, b) есть
	//   рекуррентное соотношение:
	//   phi(m, b) = phi(m, b - 1) - phi(m / primes[b - 1], b - 1).
	//   Числа не более m, у которых все простые больше b-го,
	//   вложены в числа, у которых все простые больше (b-1)-го.
	//   Что в разности? Те числа, у которых не все простые больше
	//   b-го, но все простые больше (b-1)-го. Это те числа,
	//   у которых есть b-ое простое в разложении. Среди чисел от
	//   1 до m, тем, у которых есть b-ое простое, соответствуют
	//   однозначно числа 1 до m / primes[b - 1] (делим на это
	//   простое, получаем соответствующее). Поскольку это целые
	//   числа, это же множестко -- это целые от 1 до
	//   floor(m / primes[b - 1]).
	// Сколько будет рекурсивных вызовов до достижения подходящей границы?
	//   floor(floor(m / p[1]) / p[2]) = floor(m / (p[1] p[2])),
	//   т.к. для любых чисел a, b, c floor(floor(a / b) / c) = floor(a / (bc)).
	//   Доказательство.
	//   k_3 = floor(a / (bc)) <=> k_3 bc <= a < (k_3 + 1) bc
	//   k_1 = floor(a / b) <=> k_1 b <= a < (k_1 + 1) b.
	//   k_2 = floor(floor(a / b) / c) <=> k_2 c <= floor(a / b) = k_1 < (k_2 + 1) c
	//     <=> k_2 c <= floor(a / b) = k_1 <= (k_2 + 1) c - 1
	//     => k_2 bc <= a < ((k_2 + 1) c - 1) + 1) b
	//     <=> k_2 bc <= a < k_2 bc.
	//     => k_2 = k_3.
	//  Т.е. первый аргумент для phi при рекурсивном вычислении выглядит так:
	//    n, floor(n / p[y - 1]), floor(n / p[y - 2]), ...
	//    Мы останавливаемся при floor(n / floor(y)). Тогда всего значений floor(n / t)
	//    для 1 <= t < y не более y. А второй аргумент меняется от pi(y) до 1.
	//    При вычислении phi(y, pi(y)) всего пар значений y * pi(y).
	// Надо сохранить не только пару значений, но ещё знак, который они вкладывают в
	//   значение phi(n, pi(y)).
	// Каждое слагаемое после раскрытия уже вычислимо через таблицу от обычного
	//   просеивания. Изобразим на плоскости пары точек (lowest_prime_div[t], t).
	//   phi(m, b) -- это количество точек в прямоугольнике
	//   prime[b - 1] + 1 <= lowest_prime_div <= n / y,
	//   1            <= t                <= m.
	// Посчитать ответы на эти запросы можем деревом Фенвика, например, с помощью
	//   сканирующей прямой, поддерживая префиксную сумму для каждой ординаты.
	//   В начале отсортируем запросы по t, затем при начале обработки, не включая
	//   точки текущей абсциссы в количество, посчитаем сумму слева от левой границы
	//   прямоугольника, а в конце посчитаем для запросов, которые заканчиваются,
	//   сумму справа от правой границы прямогольника (она уже будет включать точки
	//   внутри).
	// Этот этап работает за O((n / y) log(n / y) + q), просеивание работает за O(n / y),
	//   pi(y) считаем за O(log(n / y)), F считаем за O(n / y) двумя
	//   указателями (когда начинаем, первый указатель на простые на первом простом,
	//   второй в конце, смещаем второй, пока не будет в произведении не меньше y + 1,
	//   добавляем разность второго указателя и конца + 1). Второй указатель не левее
	//   первого, т.к. иначе каждое число посчитаем дважды (кроме тех случаев, когда
	//   это квадрат простого).
	// pi(y) ~= y / ln(y) (распределение простых), потому q ~= y * y / ln (y).
	// В общем, получается O(n^(2 / 3) (log(n))^(1 / 3)).
	
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