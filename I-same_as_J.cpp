#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

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

static std::vector<size_t> FindTails(std::string_view text) {
	std::string concat;
	concat.insert(concat.end(), text.crbegin(), text.crend());
	concat += "#";
	concat += text;
	
	std::vector<size_t> z_func = CalculateZFunc(concat);
	std::vector<size_t> result;
	
	for (size_t i = text.size() + 1; i < concat.size(); ++i) {
		size_t z_value = z_func[i];
		if (z_value % 2 == 1 && 2 * z_value + 1 >= concat.size() - i) {
			result.emplace_back(i - text.size() - 1);
		}
		if (z_value % 2 == 0 && 2 * z_value >= concat.size() - i) {
			result.emplace_back(i - text.size() - 1);
		}
	}
	
	return result;
}

int main() {
	size_t num_words = 0;
	scanf("%zu", &num_words);
	
	struct Node {
		Node()
		  : node_by_chr('z' - 'a' + 1, 0) {}
		 
		std::vector<size_t> node_by_chr;

		std::vector<size_t> strs_end_here;	
		std::vector<size_t> strs_of_prefix_with_palindromic_tails;
	};
	
	std::vector<Node> nodes(1);

	std::vector<std::pair<std::string, size_t>> words(num_words);
	{
		constexpr const size_t       kStrMaxLen = 1000 * 1000;
		constexpr const char * const kStrFmt    = "%1000000s";
		auto buffer = std::make_unique<char[]>(kStrMaxLen + 1);
		for (size_t i = 0; i < num_words; ++i) {
			scanf(kStrFmt, buffer.get());
			words[i] = {buffer.get(), i + 1};
		}
	}

	// Сортируем строки. При |S_i| != |S_j| строки
	//   мы перебираем строки, большая уже
	//   добавлена, потому меньшая обязательно
	//   сможет спуститься, если получается палиндром.
	// Если длины равны, просто придем в терминальную
	//   вершину, там количество палиндромных
	//   дописываний до слов -- количество слов,
	//   которые в этой вершине заканчиваются.
	// Каждую пару можно считать два раза!
	//   В разном порядке то есть. Единственное,
	//   не будем считать строку с ней же самой. И ок!
	// Влом доказывать, почему работает. Так-то это
	//   хорошее упражнение, реально разобрать, всё
	//   объяснить тут. Видимо, у меня нет опыта,
	//   потому легко в голове не складывается. Но
	//   задачу решить можно!
	/*
	std::sort(words.begin(), words.end(), [](const std::pair<std::string, size_t>& lhs, const std::pair<std::string, size_t>& rhs) {
		return lhs.first.size() > rhs.first.size();
	});*/

	std::vector<std::pair<size_t, size_t>> result;
	for (size_t i = 0; i < words.size(); ++i) {
		std::string& word = words[i].first;
		
		// Добавить строку в бор, в вершинах
		//   учесть количество палиндромных
		//   хвостов.
		size_t cur_node = 0;
		std::vector<size_t> palindromic_tails = FindTails(word);
		// printf("palindromic_tails.size() = %zu.\n", palindromic_tails.size());
		size_t last_palindromic_tail = 0;
		// Спустились по первым j символам.
		for (size_t j = 1; j <= word.size(); ++j) {
			// printf("i = %zu, j = %zu, word[j] = %c.\n", i, j, word[j]);
			size_t next_node = nodes[cur_node].node_by_chr[word[j - 1] - 'a'];
			if (next_node == 0) {
				// Создаём вершину, переходим.
				next_node = nodes.size();
				nodes.emplace_back();
				nodes[cur_node].node_by_chr[word[j - 1] - 'a'] = next_node;
			}
			cur_node = next_node;
			// После перехода учитываем количество палиндромных хвостов.
			//   Получается, в корне не учтены все хвосты, но строк
			//   длины ноль и нет.
			// Если после спуска по первым j символам j-ый
			//   символ -- начало палиндромного хвоста, то
			//   хвостом можно дополнить (как раз j символов
			//   до j-го прошли, удобно).
			while (last_palindromic_tail + 1 < palindromic_tails.size() && palindromic_tails[last_palindromic_tail] < j) {
				++last_palindromic_tail;
			}
			if (palindromic_tails[last_palindromic_tail] == j) {
				nodes[cur_node].strs_of_prefix_with_palindromic_tails.push_back(words[i].second);
			} else if (j == word.size()) {
				nodes[cur_node].strs_of_prefix_with_palindromic_tails.push_back(words[i].second);
			}
		}
		nodes[cur_node].strs_end_here.push_back(words[i].second);
	}
	for (size_t i = 0; i < num_words; ++i) {
		std::string& word = words[i].first;
	
		// Спускаемся по бору, 
		// Наложили n-символов из начала и конца. Теперь первая строка
		//   закончилась или строк с таким концом нет. Когда строки
		//   заканчиваются, можем взять, если оставшееся начало
		//   нашей -- палиндром. Если дальше символа нет, то дальше
		//   мы бы сравнивали нас с собой. Прежде чем спускаться, мы
		//   уже учли строки, которые закончились в этой вершине. Потому всё ок.
		// Спустились не полностью. Можем взять вершины с палиндромными хвостами
		//   если сейчас сами в палиндромном начале.
		// Строки могут заканчиваться, тогда они были меньше.
		//   Можем в конце концов не уткнуться в стену в конце,
		//   тогда посчитаем строки в первой половине, больше.
		// Равные просто никогда не добавим.
		std::reverse(word.begin(), word.end());
		std::vector<size_t> palindromic_prefixes = FindTails(word);
		for (size_t& start: palindromic_prefixes) {
			start = word.size() - start;
		}
		/*
		for (size_t k = 0; k < palindromic_prefixes.size(); ++k) {
			printf("i = %zu, k = %zu, palindromic_prefixes[k] = %zu.\n", i, k, palindromic_prefixes[k]);
		}
		*/
		size_t last_palindromic_prefix = 0;

		size_t cur_node = 0;
		bool found = true;
		size_t num_descends = 0;
		for (size_t j = 0; j < word.size(); ++j) {
			size_t next_node = nodes[cur_node].node_by_chr[word[j] - 'a'];
			// printf("i = %zu, cur_node = %zu, j = %zu, word[j] = %c.\n", i, cur_node, j, word[j]);
			if (next_node == 0) {
				found = false;
				break;
			}
			cur_node = next_node;
			num_descends += 1;

			// printf("i = %zu, num_descends = %zu, last_palindromic_prefix = %zu, word.size() - num_descends = %zu.\n", i, num_descends, last_palindromic_prefix, word.size() - num_descends);
			while (last_palindromic_prefix + 1 < palindromic_prefixes.size() && palindromic_prefixes[last_palindromic_prefix + 1] >= word.size() - num_descends) {
				last_palindromic_prefix += 1;
			}
			// printf("i = %zu, num_descends = %zu, last_palindromic_prefix = %zu.\n", i, num_descends, last_palindromic_prefix);
			if (last_palindromic_prefix < palindromic_prefixes.size() && palindromic_prefixes[last_palindromic_prefix] == word.size() - num_descends) {
				// printf("i = %zu, num_descends = %zu, is palindromic_prefix.\n", i, num_descends);
				for (size_t str_index: nodes[cur_node].strs_end_here) {
					if (str_index != words[i].second) {
						result.emplace_back(str_index, words[i].second);
					}
				}
			}
		}
		// printf("i = %zu, found = %d, num_descends = %zu.\n", i, (int) found, num_descends);
		// Спустились полностью, первая половина такой же длины или больше.
		if (found) {
			for (size_t str_index: nodes[cur_node].strs_of_prefix_with_palindromic_tails) {
				if (str_index != words[i].second) {
					result.emplace_back(str_index, words[i].second);
				}
			}
		}
	}
	
	/*
	for (size_t i = 0; i < nodes.size(); ++i) {
		printf("nodes[%zu] = [[", i);
		for (size_t j = 0; j < 'z' - 'a' + 1; ++j) {
			if (nodes[i].node_by_chr[j] != 0) {
				printf("'%c' -> %zu,", (char) ('a' + j), nodes[i].node_by_chr[j]);
			}
		}
		printf("],[");
		for (size_t j = 0; j < nodes[i].strs_of_prefix_with_palindromic_tails.size(); ++j) {
			printf("%zu,", nodes[i].strs_of_prefix_with_palindromic_tails[j]);
		}
		printf("]]\n");
	}
	*/
	
	printf("%zu\n", result.size());
	for (auto& item: result) {
		printf("%zu %zu\n", item.first, item.second);
	}
	
	return 0;
}