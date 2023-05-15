/*
Z-функция + динамика. Можно ли разбить префиксами суффикс второй строки.
*/

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#if defined(NDEBUG)
#define assert_with_node(expr, msg)
#else
#define assert_with_note(expr, msg) if (!(expr)) { puts(msg); assert(false); }
#endif

std::vector<size_t> CalculateZFunc(const std::string& string) {
	std::vector<size_t> z_func(string.size(), 0);
	
	z_func[0] = string.size();
	
	size_t rightmost_z_block_start = 0;
	size_t rightmost_z_block_end = 0;
	for (size_t i = 1; i < string.size(); ++i) {
		if (rightmost_z_block_end < i) {
			assert_with_note(rightmost_z_block_end == i - 1, "Rightmost_z_block can only fall behind by one position.");
			
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

std::vector<size_t> CalculatePartition(const std::string& source_word, const std::string& target_word) {
	assert_with_note(!source_word.empty(), "Source word mustn't be empty.");
	assert_with_note(!target_word.empty(), "Target word mustn't be empty.");

	std::string joined = source_word + "#" + target_word;
	std::vector<size_t> z_func = CalculateZFunc(joined);
	
	std::vector<bool> dp_can_partition_suffix_at(joined.size(), false);
	std::vector<size_t> dp_next_suffix_start(joined.size(), 0);
	size_t last_partitionable_suffix_start = 0;
	bool have_partitionable_suffixes = false;
	for (size_t pos_1indexed = joined.size(); pos_1indexed > source_word.size() + 1; --pos_1indexed) {
		size_t pos = pos_1indexed - 1;
		// If max prefix we can take from this pos spans to the end of the target_word.
		//   This pos doesn't need to continue partition from another one.
		if (z_func[pos] == joined.size() - pos) {
			last_partitionable_suffix_start = pos;
			dp_can_partition_suffix_at[pos] = true;
			dp_next_suffix_start[pos] = pos;
			have_partitionable_suffixes = true;
		} else {
			// If z_func[pos] = 0, can't start a prefix here.
			// If z_func[pos] > 0, let it be 2, can use prefix of length 1 or 2.
			//   next suffix is either pos+1 or pos+2
			if (
				have_partitionable_suffixes &&
				last_partitionable_suffix_start <= pos + std::min(z_func[pos], source_word.size())
			) {
				dp_can_partition_suffix_at[pos] = true;
				dp_next_suffix_start[pos] = last_partitionable_suffix_start;
				last_partitionable_suffix_start = pos;
			}
		}
		// printf("target_word_pos = %zu, z_func[pos] = %zu, dp_can_partition_suffix_at[pos] = %u, dp_next_suffix_start[pos] = %zu, last_partitionable_suffix_start = %zu.\n", pos - source_word.size(), z_func[pos], static_cast<unsigned>(static_cast<bool>(dp_can_partition_suffix_at[pos])), dp_next_suffix_start[pos], last_partitionable_suffix_start);
	}

	if (!dp_can_partition_suffix_at[source_word.size() + 1]) {
		return {};
	}
	
	std::vector<size_t> partition_prefix_starts;
	size_t pos = source_word.size() + 1;
	while (true) {
		partition_prefix_starts.push_back(pos - source_word.size() - 1);

		// Covers the entire string left.
		if (pos == dp_next_suffix_start[pos]) {
			break;
		}
		pos = dp_next_suffix_start[pos];	
	}
	
	return partition_prefix_starts;
}

int main() {
	constexpr size_t kMaxStrLen = 75 * 1000 + 1;
	constexpr const char* const kStrFormat = "%75000s";
	static char k_buffer[kMaxStrLen + 1] = {0};
	
	scanf(kStrFormat, k_buffer);
	std::string source_word(k_buffer);

	scanf(kStrFormat, k_buffer);
	std::string target_word(k_buffer);
	
	std::vector<size_t> result = CalculatePartition(source_word, target_word);
	if (result.empty()) {
		printf("Yes\n");
		return 0;
	}
	
	printf("No\n");
	result.push_back(target_word.size());
	for (size_t i = 0; i < result.size() - 1; ++i) {
		for (size_t pos = result[i]; pos < result[i + 1]; ++pos) {
			printf("%c", target_word[pos]);
		}
		printf(" ");
	}
	printf("\n");

	return 0;
}