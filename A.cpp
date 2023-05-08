#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

// Let pi be a prefix function of a string.
//   Let's analyse pi's properties.
// [------------------]
// [     ]     |
//      [     ]i
// pi[i] can't be greater than pi[i-1]+1,
//   otherwise pi[i-1] >= pi[i]-1.
//                                  i
//               [----------------------]
//   p[i-1]       111111111111      |
//                      111111111111|
//   p[i]              22222222222222|
//                22222222222222     | Suppose pi[i] > pi[pi[i - 1]-1], then
//   pnew[i-1]    3333333333333     |
//                     3333333333333|
// It's so iff string[i] == string[pi[i-1]].
//   If it's so, pi[i] is atleast pi[i-1]+1, but can't be greater.
//   If pi[i-1]+1=pi[i], then naturally
//   string[pi[i-1]] (prefix of len i+1) == string[i+1]
// Try to make it pi[i - 1]+1, otherwise
//   select the next one that is possible.
//   It can be proven that lengths between
//   pi[i-1] and pi[pi[i-1]-1]+2 can't be
//   the value of prefix_function in this
//   position. If pi[i] > pi[pi[i-1]-1],
//   then pi[i - 1]
//                                  i
//               [----------------------]
//   p[i-1]       111111111111      |
//                      111111111111|
//   p[p[i-1]-1]  222      222      |
//                      222         | Suppose pi[i] > pi[pi[i - 1]-1], then
//                33333         33333|
//                      33333        
//                        3333t
//   pi[pi[i-1]-1] is at least 4, when we
//   assumed 3.
//   Just now analyse some examples with such a picture.
static std::vector<size_t> CalculatePrefixFunc(std::string_view string) {
	std::vector<size_t> prefix_func(string.size(), 0);
	for (size_t i = 1; i < string.size(); ++i) {
		size_t candidate = prefix_func[i - 1] + 1;
		while (candidate > 1 && string[candidate - 1] != string[i]) {
			candidate = prefix_func[(candidate - 1) - 1] + 1;
		}

		// Candidate > 0, it's invariant when we initialize
		//   the variable and when we change it in the loop.
		if (string[candidate - 1] == string[i]) {
			prefix_func[i] = candidate;
		} else {
			// Left the loop because of candidate <= 1 and already
			//   checked, that string[candidate - 1] != string[i]
			prefix_func[i] = 0;
		}
	}
	
	return prefix_func;
}

static std::vector<size_t> FindSubstr(std::string_view pattern, std::string_view text) {
	std::string joined(pattern);
	assert(pattern.find('#') == std::string_view::npos); // std::string_view::contains and std::string::contains from c++23 onwards, wanted to use pattern.contains().
	joined.push_back('#');
	joined += text;
	
	std::vector<size_t> result;
	
	std::vector<size_t> prefix_func = CalculatePrefixFunc(joined);
	for (size_t i = pattern.size() + 1; i < joined.size(); ++i) {
		if (prefix_func[i] == pattern.size()) {
			result.push_back((i - pattern.size() - 1) - pattern.size() + 1);
		}
	}
	
	return result;
}

int main() {
	static const size_t kMaxStrLen = 50 * 1000;
	static const char * const kFormatStr = "%50000s";
	
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(kMaxStrLen + 1);
	
	scanf(kFormatStr, buffer.get());
	std::string text(buffer.get());

	scanf(kFormatStr, buffer.get());
	std::string pattern(buffer.get());

	std::vector<size_t> result = FindSubstr(pattern, text);
	for (size_t item: result) {
		printf("%zu\n", item);
	}
	
	return 0;
}