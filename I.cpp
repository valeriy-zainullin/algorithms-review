#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#define ENABLE_64BIT_HASH 0

// Good articles about polynomial hashing:
//   (1) https://codeforces.com/blog/entry/60445?locale=en
//   (2) https://codeforces.com/blog/entry/17507

struct StrHash {
	uint32_t hash1;
	uint32_t hash2;

	#if ENABLE_64BIT_HASH
		uint64_t hash3; // TL, need to reduce number of hashes.
	#endif

public:
	uint64_t ToU64() const {
		return (static_cast<uint64_t>(hash1) << 32) + hash2;
	}
	
private:
	struct Modules {
		static constexpr const uint32_t kMod1 = 1000 * 1000 * 1000 + 7;
		static constexpr const uint32_t kMod2 = 1073676287;
		// kMod3 = 2^64
	};

	// Я думаю так, мб есть какие-то обсуждения в сообществе
	//   на эту тему..
	// Модуль 2^64 хорош тем, что он даёт очень большое
	//   пространство. Если мы будем явно брать какой-то
	//   модуль, нам придется брать не больше корня квадратного
	//   из максимума, который представим, т.к. мы хотим
	//   два такие числа уметь перемножать, а если произведение
	//   этих чисел переполнит uint64_t, от значения будет
	//   взять модуль 2^64, при взятии модуля правильное
	//   значение уже не получим. А по модулю 2^64 пусть
	//   берет, всё правильно.
	// Наибольшое пространство за одну пару (основание, модуль).
	//   Выгодно.
	
	static const size_t kAlphabetSize = 'z' + 1;
	
	static uint32_t GetRandomBase() {
		std::random_device rd;
		
		// Like this, because of MinGW giving
		//   the same value at random_device for all calls.
		//   More at "Among the disadvantages of polynomial hashing"
		//   and at "`UPD`: Solved b)" in (1).
		// Maybe I should've put Enlgi
		uint64_t seed = rd();
		seed ^= static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		seed ^= reinterpret_cast<uint64_t>(std::make_unique<char>().get());
		
		static std::mt19937 generator(static_cast<uint32_t>(seed));
		
		uint32_t base = generator();
		while (base % 2 == 0 || base <= kAlphabetSize) {
			base = generator();
		}
		
		return base;
	}
	
	static uint32_t base_1;
	static uint32_t base_2;
	static uint32_t base_3;

	friend class StrHasher;
};
std::ostream& operator<<(std::ostream& stream, const StrHash& hash) {
	#if ENABLE_64BIT_HASH
		stream << '(' << hash.hash1 << ',' << hash.hash2 << ',' << hash.hash3 << ')';
	#else
		stream << '(' << hash.hash1 << ',' << hash.hash2 << /*',' << hash.hash3 <<*/ ')';
	#endif
	
	return stream;
}

uint32_t StrHash::base_1 = StrHash::GetRandomBase();
uint32_t StrHash::base_2 = StrHash::GetRandomBase();

#if ENABLE_64BIT_HASH
	uint32_t StrHash::base_3 = StrHash::GetRandomBase();
#endif

bool operator==(const StrHash& lhs, const StrHash& rhs) {
	#if ENABLE_64BIT_HASH
		return lhs.hash1 == rhs.hash1 && lhs.hash2 == rhs.hash2 && lhs.hash3 == rhs.hash3;
	#else
		return lhs.hash1 == rhs.hash1 && lhs.hash2 == rhs.hash2;
	#endif
}

bool operator<(const StrHash& lhs, const StrHash& rhs) {
	if (lhs.hash1 != rhs.hash1) {
		return lhs.hash1 < rhs.hash1;
	}
	
	if (lhs.hash2 != rhs.hash2) {
		return lhs.hash2 < rhs.hash2;
	}
	
	#if ENABLE_64BIT_HASH
		if (lhs.hash3 != rhs.hash3) {
			return lhs.hash3 < rhs.hash3;
		}
	#endif

	// Equal.
	return false;
}

class StrHasher {
public:
	explicit StrHasher(const std::string& string)
	  : StrHasher(string.begin(), string.end()) {}
	
	explicit StrHasher(const std::string_view view)
	  : StrHasher(view.begin(), view.end()) {}
	
	template <typename IteratorType>
	StrHasher(IteratorType begin, IteratorType end) {
		auto length = static_cast<size_t>(end - begin);

		InitBasePowArray<StrHash::kMod1>(base1_pow_, StrHash::base_1, length);
		InitBasePowArray<StrHash::kMod2>(base2_pow_, StrHash::base_2, length);
		#if ENABLE_64BIT_HASH
			InitBasePowArrayMod64(base3_pow_, StrHash::base_3, length);
		#endif
		
		// Хешируем по каждой паре (основание, модуль) по отдельности.
		InitHashArray<StrHash::kMod1>(hash1_, StrHash::base_1, begin, length);
		InitHashArray<StrHash::kMod2>(hash2_, StrHash::base_2, begin, length);
		// InitHashArrayMod64(hash3_, StrHash::base_3, begin, length);
	}
	
	StrHasher(const StrHasher&) = delete; // Quite expensive. Please recreate, if needed, or use method copy (which will be just by-field copy).
	StrHasher(StrHasher&&) = default;

	// Hash substr [start; end). 0-indexed.
	StrHash HashSubstr(size_t start, size_t end) const {
		assert(start <= end);
		
		size_t str_length = hash1_.size() - 1;
		assert(end <= str_length);
		(void) str_length;
	
		uint32_t result_hash1 = HashSubstrSingleBaseMod<StrHash::kMod1>(hash1_, base1_pow_, start, end);
		uint32_t result_hash2 = HashSubstrSingleBaseMod<StrHash::kMod2>(hash2_, base2_pow_, start, end);
		#if ENABLE_64BIT_HASH
			uint64_t result_hash3 = HashSubstrSingleBaseMod64(hash3_, base3_pow_, start, end);
		#endif  
		
		#if ENABLE_64BIT_HASH
			return StrHash{result_hash1, result_hash2, result_hash3};
		#else
			return StrHash{result_hash1, result_hash2};
		#endif
	}
private:
	// Making mod a template parameter,
	//   so that compiler knows it at the
	//   moment of compilation. This should
	//   speed functions a lot, as the
	//   compiler may optimize modulo
	//   with bitwise shifts and ands.

	template <uint32_t Mod>
	void InitBasePowArray(std::vector<uint32_t>& base_pow, uint32_t base, size_t str_length) {
		// To avoid overflow
		//   and not use long text
		//   static_cast.
		uint64_t base_u64 = base;
		
		base_pow.resize(str_length + 1, 0);
		base_pow[0] = 1;
		for (size_t i = 1; i <= str_length; ++i) {
			base_pow[i] = static_cast<uint32_t>(base_pow[i - 1] * base_u64 % Mod);
		}
	}
	
	template <uint32_t Mod, typename IteratorType>
	void InitHashArray(std::vector<uint32_t>& hash, uint32_t base, IteratorType begin, size_t str_length) {
		IteratorType it = begin;

		// To avoid overflow
		//   and not use long text
		//   static_cast.
		uint64_t base_u64 = base;
		
		hash.resize(str_length + 1, 0);
		hash[0] = 0;
		for (size_t i = 1; i <= str_length; ++i) {
			hash[i] = static_cast<uint32_t>((hash[i - 1] * base_u64 + *it) % Mod);
			it += 1;
		}
	}
	
	template <uint32_t Mod>
	uint32_t HashSubstrSingleBaseMod(const std::vector<uint32_t>& hash, const std::vector<uint32_t>& base_pow, size_t start, size_t end) const {
		// 0-indexed index is number of items before the item.
		//   So end index is last item of substr + 1 =
		//   = number of items before last item exclusive + 1
		//   = number of items before last item inclusive.
		size_t substr_len = end - start;
		return static_cast<uint32_t>((Mod + hash[end] - static_cast<uint64_t>(hash[start]) * base_pow[substr_len] % Mod) % Mod);
	}

	#if 
	void InitBasePowArrayMod64(std::vector<uint64_t>& base_pow, uint32_t base, size_t str_length) {
		base_pow.resize(str_length + 1, 0);
		base_pow[0] = 1;
		for (size_t i = 1; i <= str_length; ++i) {
			base_pow[i] = base_pow[i - 1] * base;
		}
	}

	template <typename IteratorType>
	void InitHashArrayMod64(std::vector<uint64_t>& hash, uint32_t base, IteratorType begin, size_t str_length) {
		IteratorType it = begin;
		
		hash.resize(str_length + 1, 0);
		hash[0] = 0;
		for (size_t i = 1; i <= str_length; ++i) {
			hash[i] = hash[i - 1] * base + *it;
			it += 1;
		}
	}

	uint64_t HashSubstrSingleBaseMod64(const std::vector<uint64_t>& hash, const std::vector<uint64_t>& base_pow, size_t start, size_t end) const {
		// 0-indexed index is number of items before the item.
		//   So end index is last item of substr + 1 =
		//   = number of items before last item exclusive + 1
		//   = number of items before last item inclusive.
		size_t substr_len = end - start;
		return hash[end] - hash[start] * base_pow[substr_len];
	}
	*/

private:
	// Степени оснований, взятые по соответствующим модулям.
	std::vector<uint32_t> base1_pow_;
	std::vector<uint32_t> base2_pow_;
	// std::vector<uint64_t> base3_pow_;

	// Массивы хешей, в каждом массиве
	//   k-ый элемент -- хеш первых k символов
	//   строки по паре (основание, модуль) с
	//   номером, соответствюущим номеру массива.
	std::vector<uint32_t> hash1_;
	std::vector<uint32_t> hash2_;
	// std::vector<uint64_t> hash3_;
};

std::vector<size_t> CalculateZFunc(const std::string& string) {
	std::vector<size_t> z_func(string.size(), 0);
	
	z_func[0] = string.size();
	
	size_t rightmost_z_block_start = 0;
	size_t rightmost_z_block_end = 0;
	for (size_t i = 1; i < string.size(); ++i) {
		if (rightmost_z_block_end < i) {
			assert(rightmost_z_block_end == i - 1);
			
			if (string[0] != string[i]) {
				z_func[i] = 0;
				rightmost_z_block_end = i;
				continue;
			}
			
			size_t new_z_block_end = i;
			while (
				new_z_block_end < string.size() &&
				string[new_z_block_end + 1] == string[new_z_block_end + 1 - i]
			) {
				new_z_block_end += 1;
			}
			
			z_func[i] = new_z_block_end - i + 1;
			rightmost_z_block_start = i;
			rightmost_z_block_end = new_z_block_end;
		} else {
			size_t guaranteed_value = z_func[i - rightmost_z_block_start];
			if (guaranteed_value < rightmost_z_block_end - i + 1) {
				// z_func[i] can't be greater, otherwise
				//   z_func[i - rightmost_block_start] would be greater.
				z_func[i] = guaranteed_value;
			} else {
				guaranteed_value = rightmost_z_block_end - i + 1; // Limit the value.
				rightmost_z_block_start = i;
				
				while (
					rightmost_z_block_end + 1 < string.size() &&
					string[rightmost_z_block_end + 1] == string[rightmost_z_block_end + 1 - i]
				) {
					rightmost_z_block_end += 1;
				}
				
				z_func[i] = rightmost_z_block_end - rightmost_z_block_start + 1;
			}
		}
	}
	
	return z_func;
}

static std::vector<size_t> FindPalindromicSuffixes(std::string_view text) {
	std::string concat;
	concat.insert(concat.end(), text.crbegin(), text.crend());
	concat += "#";
	concat += text;
	
	std::vector<size_t> z_func = CalculateZFunc(concat);
	std::vector<size_t> result;
	
	for (size_t i = text.size() + 1; i < concat.size(); ++i) {
		size_t z_value = z_func[i];
		// Считаем для суффикса какой его префикс максимальной
		//   длины при развороте совпадает с суффиксом. Отложим
		//   эти отрезки от начала и конца суффикса. Надо
		//   уменьшать на 2, пока не будет такого, что получим
		//   непересекающиеся отрезки. Если изначально эти отрезки
		//   пересекались или полностью покрывали без пересечения,
		//   всё получится. Во втором случае просто получили
		//   палиндром, при этом поняли, что суффикс чётной длины.
		//   А в первом случае будем на один уменьшать длины,
		//   получим либо покрытие без пересечение, либо покрытие,
		//   но один символ между отрезками будет, тогда тоже
		//   палиндром. Так что просто необходимо проверить, что
		//   отрезки соседние или пересекаются.
		if (2 * z_value >= concat.size() - i) {
			// Записываем начало суффикса.
			result.emplace_back(i - text.size() - 1);
		}
	}
	
	return result;
}

std::vector<std::tuple<std::size_t, std::size_t>> FindPalindromicConcats(std::vector<std::string>& words) {
	struct Entry {
		std::string text;
		size_t position;
	};

	std::vector<Entry> entries; 

	for (size_t i = 0; i < words.size(); ++i) {
		// From c++20 onwards: words[i] = {.text = std::move(word), .position = i + 1};
		// The contest had c++17 only.
		entry[i] = {std::move(word), i + 1};		
	}
	
	std::sort(entries.begin(), entries.end(), [](const Entry& lhs, const Entry& rhs) {
		return lhs.text.size() < rhs.text.size();
	});

	// Считаем хеши строк в прямом порядке и обратном (развёрнутых).
	std::vector<StrHasher> hashers;
	std::vector<StrHasher> hashers_reversed;
	hashers.reserve(words.size());
	hashers_reversed.reserve(words.size());
	for (const Word& word: words) {
		hashers.emplace_back(word.text);
		hashers_reversed.emplace_back(word.text.crbegin(), word.text.crend());
	}
	
	// Пусть A -- множество пар (i, j) таких, что
	//   s_i + s_j -- палиндром.
	//  Мы можем разложить это множество на
	//    непересекающиеся так: A_1 -- те пары,
	//    где длины слов совпадают. A_2 -- те пары,
	//    где длины слов различны. Затем A_2
	//    разложим по B_k x {k}, B_k -- те i,
	//    при которых s_i + s_k палиндром (с
	//    фиксированным словом) и |s_i| < |s_k|.
	//    И разложим по {k} x C_k, C_k -- те j,
	//    при которых s_k + s_j палиндром.
	// Т.е. в каждой паре A_2 длины слов различны,
	//    посчитаем пару для слова с наибольшей
      	//    длиной в ней.
	// Почему так надо? Потому что, перебирая слово,
	//   надо знать, оно самое длинное или самое
	//   маленькое по длине, т.к. от этого зависит то,
	//   какие хеши искать. Код в комменте ниже не
	//   работает.
	
	std::unordered_map<uint64_t, std::vector<size_t>> hash_to_positions;

	// Выделяем отрезки слов одной длины.
	//   Для каждого из них сначала считаем
	//   палиндромы, где они самые длинные,
	//   т.е. со словами меньшей длины.
	//   Только такие были добавлены.
	// Затем считаем палиндромы между собой,
	//   просто храня полностью хеши
	//   развёрнутой строки. На забываем
	//   учесть i != j.
	// Затем добавляем строки текущей длины
	//   в словарь.
	for (size_t begin = 0; begin < words.size();) {
		// [begin; end)
		size_t end = begin + 1;
		while (end < words.size() && words[end].text.size() == words[begin].text.size()) {
			// Оказалось, сдвинуть пока можно и отрезок слов такой
			//   длины включает end, сдвигаем.
			// Это называется методом двух указателей.
			end += 1;
		}
		
		// Считаем пары, где текущее слово самое длинное.
		for (size_t i = begin; i < end; ++i) {
			size_t word_len      = words[i].text.size();
			size_t word_position = words[i].position;
			
			// Если наша строка годится в качестве первой.
			//   Перебираем, какую длину имела вторая часть.
			//   Её длина должна быть меньше (текущая строка
			//   самая длинная в паре).
			// При этом в нашей строке остаётся хвостик,
			//   это будет суффикс. Надо проверить, что он
			//   сам -- палиндром. Мы будем проверять только
			//   те длины, при которых хвостик -- палиндром.
			std::vector<size_t> palindromic_suffixes = FindPalindromicSuffixes(words[i].text);
			// palindromic_suffixes -- массив начал суффиксов, т.е.
			//   длины префиксов, которые можно проверять.
			for (size_t second_half_len: palindromic_suffixes) {
				if (second_half_len == 0) {
					// Возможно, что оказалось, сама строка
					//   палиндром. Ладно, пропускаем.
					// Не может быть длины ноль, поскольку
					//   пустого слова во вводе нет.
					continue;
				}

				// В развёрнутой строке префикс становится суффиксом и разворачивается.
				//   Нам как раз нужен развёрнутый префикс, потому это суффикс. И затем
				//   получить его хеш.
				auto it = hash_to_positions.find(hashers_reversed[i].HashSubstr(word_len - second_half_len, word_len).ToU64());
				
				if (it != hash_to_positions.end()) {
					for (size_t position: it->second) {
						result.emplace_back(word_position, position);
					}
				}
			}

			// Если наша строка годится в качестве второй.
			//   Перебираем, какую длину имела первая часть.
			//   Её длина должна быть меньше (текущая строка
			//   самая длинная в паре).
			// При этом в нашей строке остаётся хвостик,
			//   это будет префикс. Надо проверить, что он
			//   сам -- палиндром. Мы будем проверять только
			//   те длины, при которых хвостик -- палиндром.
			std::reverse(words[i].text.begin(), words[i].text.end());
			std::vector<size_t> palindromic_prefixes = FindPalindromicSuffixes(words[i].text);
			// Получили начала суффиксов-палиндромов в развёрнутой строке.
			//   Это длины префиксов, которые надо удалить, чтобы получить
			//   такие суффиксы (индекс элемента в массиве с 0-индексацией
			//   -- количество элементов до него, а у нас номер первого
			//   символа суффикса).
			// При развороте строки префиксы переходят в суффиксы (и
			//   наоборот) и каждый префикс при этом разворачивается.
			// Потому мы получили длины суффиксов, при которых префиксы
			//   -- палиндромы.
			std::reverse(words[i].text.begin(), words[i].text.end());
			// palindromic_prefixes -- массив длин суффиксов,
			//   при удалении которых префиксы -- палиндромы.
			//   То, что надо.
			for (size_t first_half_len: palindromic_prefixes) {
				if (first_half_len == 0) {
					// Возможно, что оказалось, сама строка
					//   палиндром. Ладно, пропускаем.
					// Не может быть длины ноль, поскольку
					//   пустого слова во вводе нет.
					continue;
				}
				// В развёрнутой строке суффикс становится префиксом и разворачивается.
				//   Нам как раз нужен развёрнутый суффикс, потому это префикс. И затем
				//   получить его хеш.
				auto it = hash_to_positions.find(hashers_reversed[i].HashSubstr(0, first_half_len).ToU64());
				
				if (it != hash_to_positions.end()) {
					for (size_t position: it->second) {
						result.emplace_back(position, word_position);
					}
				}
			}
		}
		
		// Считаем палиндромы, где два оба слова текущей длины.
		std::unordered_map<uint64_t, std::vector<size_t>> curlen_hash_to_positions;
		for (size_t i = begin; i < end; ++i) {
			size_t word_len      = words[i].text.size();
			size_t word_position = words[i].position;
			StrHash hash = hashers[i].HashSubstr(0, word_len);

			curlen_hash_to_positions[hash.ToU64()].push_back(word_position);
		}
		// Перебираем второй элемент пары, для него легко
		//   найдем все первые.
		for (size_t i = begin; i < end; ++i) {
			size_t word_len      = words[i].text.size();
			size_t word_position = words[i].position;

			auto it = curlen_hash_to_positions.find(hashers_reversed[i].HashSubstr(0, word_len).ToU64());
			
			if (it != curlen_hash_to_positions.end()) {
				for (size_t position: it->second) {
					// If i != j в терминах задачи.
					if (position != word_position) {
						result.emplace_back(position, word_position);
					}
				}
			}
		}
	
		// Добавляем строки текущей длины в словарь.
		for (size_t i = begin; i < end; ++i) {
			size_t word_len      = words[i].text.size();
			size_t word_position = words[i].position;
			StrHash hash = hashers[i].HashSubstr(0, word_len);

			// Если такого элемента не было, он создается,
			//   при этом вызывается конструктор по умолчанию.
			//   Для std::vector -- создать пустой вектор.
			hash_to_positions[hash.ToU64()].push_back(word_position);			
		}
		
		begin = end;
	}
	
	// Больше не нужно, каждую пару считаем точно один раз,
	//   т.к. разложили на непересекающиеся множества.
	// std::sort(result.begin(), result.end());
	// result.erase(std::unique(result.begin(), result.end()), result.end());
	
	printf("%zu\n", result.size());
	for (const auto& [first, second]: result) {
		printf("%zu %zu\n", first, second);
	}
}

int main() {
	freopen("input.txt", "r", stdin);

	size_t num_words = 0;
	std::cin >> num_words;
		
	std::vector<std::string> words(num_words);

	for (auto& word: words) {
		std::string word;
		std::cin >> word;
	}
			
	std::vector<std::pair<size_t, size_t>> result;	
		
	return 0;
}
