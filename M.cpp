#include <cassert>
#include <string>
#include <memory>
#include <vector>

#if defined(NDEBUG)
#define assert_with_node(expr, msg)
#else
#define assert_with_note(expr, msg) if (!(expr)) { puts(msg); assert(false); }
#endif

#define DEBUG 0

namespace SuffixArray {
	constexpr char kTextEndChr = '\10';
	constexpr char kSrcStrAlphabetMaxChr = 127; //'z';

	static void Calculate(std::string_view text, std::vector<size_t>& suffix_array, std::vector<size_t>& inv_suffix_array) {
		assert_with_note(!text.empty(), "Test mustn't be empty.");
		for (size_t i = 0; i < text.size(); ++i) {
			assert(text[i] > kTextEndChr);
		}
	
		// Этап 1: дополнение строки.
		std::string text_mod(text);
		text_mod.push_back(kTextEndChr);

		// Общие массивы для этапов, остаются с предыдущей итерации
		//   для новой.
		std::vector<size_t> sorted_items;
		std::vector<size_t> component_by_item(text_mod.size(), 0);

		// Этап 2.
		// Детали в M_notes.txt, прочитайте, с радостью отвечу на ваши вопросы.
		// Сортируем строки длины 1.
		{
			std::vector<size_t> num_occurs(static_cast<size_t>(kSrcStrAlphabetMaxChr) + 1, 0);
			for (size_t i = 0; i < text_mod.size(); ++i) {
				const size_t digit = text_mod[i];
				num_occurs[digit] += 1;
			}
			// Замечание 1 из M_notes.txt. Прочитайте, с радостью отвечу на ваши вопросы.
			std::vector<size_t>& num_items_before_digit = num_occurs;
			size_t cur_digit_num_items_before = 0;
			for (size_t i = 0; i <= kSrcStrAlphabetMaxChr; ++i) {
				size_t num_occurences = num_occurs[i];
				num_items_before_digit[i] = cur_digit_num_items_before;
				// For the next iteration this pos is included.
				cur_digit_num_items_before += num_occurences;
			}
			sorted_items.assign(text_mod.size(), 0);
			for (size_t i = 0; i < text_mod.size(); ++i) {
				const size_t digit = text_mod[i];
				sorted_items[num_items_before_digit[digit]] = i;
				num_items_before_digit[digit] += 1;
			}
			// Считаем номера компонент эквивалентности.
			component_by_item[sorted_items[0]] = 0;
			for (size_t i = 1; i < sorted_items.size(); ++i) {
				// Замечание 2 из M_notes.txt. Прочитайте, с радостью отвечу на ваши вопросы.
				if (text[sorted_items[i]] != text[sorted_items[i - 1]]) {
					component_by_item[sorted_items[i]] = component_by_item[sorted_items[i - 1]] + 1;
				} else {
					component_by_item[sorted_items[i]] = component_by_item[sorted_items[i - 1]];
				}
			}
		}

		#if DEBUG
			printf("text = \"");
			for (size_t i = 0; i < text.size(); ++i) {
				if (i != 0 && i % 5 == 0) {
					printf(" ");
				}
				printf("\\%02x", (unsigned int) text[i]);
			}
				printf("\".\n");

			printf("component_by_item = [");
			for (size_t i = 0; i < component_by_item.size(); ++i) {
				printf("%zu,", component_by_item[i]);
			}
			printf("].\n");
	
			printf("sorted_items = [");
			for (size_t i = 0; i < sorted_items.size(); ++i) {
				printf("%zu,", sorted_items[i]);
			}
			printf("].\n");
		#endif

		// Переход к следующей итерации из M_notes.txt. Прочитайте, с радостью отвечу на ваши вопросы.
		// TODO: write explaination as a step. Read the original article, explain better.
		std::vector<size_t> new_sorted_items(text_mod.size());
		std::vector<size_t> new_component_by_item(text_mod.size());
		// ull to avoid comparision between signed and unsigned, it's
		//   a warning. Don't think about it when you write it first
		//   time, you'll fix that.
		//   TODO: сделать секцию: особенности реализации на C++, там это указать.

		// While previous iteration was not enough.
		// (1ull << (len_log - 1)) < text_mod.size() is wrong. What'll happen?
		//   We need to sort by more than, length characters, This will stop
		//   the first time it's greater than length, without sorting!
		//   It may become greater, just only once! If it wasn't greater or
		//   equal before.
		for (size_t len_log = 1; (1ull << (len_log - 1)) < text_mod.size(); ++len_log) {
			// Сортировка по второй половине из M_notes.txt. Прочитайте, с радостью отвечу на ваши вопросы.		
			const size_t len = static_cast<size_t>(1) << len_log;
			const size_t half_len = len / 2;
			for (size_t i = 0; i < text_mod.size(); ++i) {
				new_sorted_items[i] = (sorted_items[i] + text_mod.size() - half_len) % text_mod.size();
			}
			// Сортировка по второй половине закончена.

			// Сортировка по первой половине из M_notes.txt. Прочитайте, с радостью отвечу на ваши вопросы.		
			{
				std::vector<size_t> num_occurs(text_mod.size(), 0);
				for (size_t i = 0; i < text_mod.size(); ++i) {
					const size_t digit = component_by_item[i];
					num_occurs[digit] += 1;
				}
				// Исключающие префиксные суммы, как говорят публикации
				//   алгоритма Каркайнена-Сандерса.
				std::vector<size_t>& num_items_before_digit = num_occurs;
				size_t cur_digit_num_items_before = 0;
				for (size_t i = 0; i < num_occurs.size(); ++i) {
					size_t num_occurences = num_occurs[i];
					num_items_before_digit[i] = cur_digit_num_items_before;
					// For the next iteration this pos is included.
					cur_digit_num_items_before += num_occurences;
				}
				// Перебираем пары в порядке сортировки по второй половине.
				for (size_t i: new_sorted_items) {
					// Берем компоненту первой половины.
					const size_t digit = component_by_item[i];
					sorted_items[num_items_before_digit[digit]] = i;
					num_items_before_digit[digit] += 1;
				}
				// Сортировка по первой половине закончена.
				
				// Считаем номера компонент эквивалентности.
				new_component_by_item[sorted_items[0]] = 0;
				for (size_t i = 1; i < sorted_items.size(); ++i) {
					size_t prev_item = sorted_items[i - 1];
					size_t cur_item = sorted_items[i];
	
					size_t prev_item_first_half_comp  = component_by_item[prev_item];
					size_t prev_item_second_half_comp = component_by_item[(prev_item + half_len) % text_mod.size()];
	
					size_t cur_item_first_half_comp  = component_by_item[cur_item];
					size_t cur_item_second_half_comp = component_by_item[(cur_item + half_len) % text_mod.size()];

					// We already know (p_f, p_s) <= (c_f, c_s),
					//   it's p_f < c_f || (p_f == c_f && p_s <= c_s)
					// We just they are not equal we just need to
					//   check p_f < c_f || p_s < c_s, because if
					//   p_f < c_f is not true, p_f >= c_f,
					//   then p_f == c_f.
					if (prev_item_first_half_comp < cur_item_first_half_comp || prev_item_second_half_comp < cur_item_second_half_comp) {
						new_component_by_item[cur_item] = new_component_by_item[prev_item] + 1;
					} else {
						new_component_by_item[cur_item] = new_component_by_item[prev_item];
					}
				}
				// Массив новых компонент переходит в следующую итерацию,
				//   а предыдущий используем как временную память.
				//   Мы её перезапишем.
				// А массив отсортированного порядка мы поменяли ещё
				//   при сортировке по первой половине.
				std::swap(new_component_by_item, component_by_item);
			}
		
			#if DEBUG
				printf("After len_log = %zu.\n", len_log);
				printf("component_by_item = [");
				for (size_t i = 0; i < component_by_item.size(); ++i) {
					printf("%zu,", component_by_item[i]);
				}
				printf("].\n");
	
				printf("sorted_items = [");
				for (size_t i = 0; i < sorted_items.size(); ++i) {
					printf("%zu,", sorted_items[i]);
				}
				printf("].\n");
			
				printf("\n");
			#endif
		}

		// После всех итераций отсортировали по большому количеству символов (>= n),
		//   по факту получили сортировку суффиксов.
		// Только удалим символ ноль, он лежит первым, т.к. это самый маленький
		//   суффикс.
		//   TODO: добавить в шаги, указать здесь шаг, как выше.
		sorted_items.erase(sorted_items.begin(), sorted_items.begin() + 1);
		suffix_array = std::move(sorted_items);
		
		// После всех итераций размер каждой компоненты равен одному,
		//   т.к. все элементы различны. И это просто индекс суффикса
		//   в суффиксном массиве (обратный суффиксный массив).
		//   TODO: добавить в шаги, указать здесь шаг, как выше.
		//   TODO: указать, зачем обратный суффиксный массив и в шаге, и тут: для lcp.
		// Только нужно удалить компоненту по item-у text.size(), т.к это нулевой символ.
		//   И все компоненты сдвинуть на один, т.к. первая компонента -- компонента
		//   нулевого символа.
		component_by_item.erase(component_by_item.end() - 1, component_by_item.end());
		for (size_t& component: component_by_item) {
			component -= 1;
		}
		inv_suffix_array = std::move(component_by_item);
	}
}
	
int main() {
	std::string text;
	
	constexpr size_t kBufferSize = 4096;
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(kBufferSize);
	while (true) {
		size_t length = fread(buffer.get(), sizeof(char), kBufferSize, stdin);
		// printf("length = %zu.\n", length);
		text.insert(text.end(), buffer.get(), buffer.get() + length);
		
		if (length == 0) {
			// Some error or eof.
			break;
		}
	}
	
	// В задаче считается, что '\n' -- конец строки, а не разделитель.
	//   Как и везде в unix. Удаляем его. Я тестировал без него.
	if (text.back() == '\n') {
		text.pop_back();
	}
	
	// printf("text = %s.\n", text.c_str());
	
	std::vector<size_t> suffix_array;
	std::vector<size_t> inv_suffix_array;
	SuffixArray::Calculate(text, suffix_array, inv_suffix_array);
	for (size_t i = 0; i < suffix_array.size(); ++i) {
		// В ответе просят в 1-индексации.
		printf("%zu ", suffix_array[i] + 1);
	}
	printf("\n");
	
	return 0;
}
