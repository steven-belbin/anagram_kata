#include <spdlog/spdlog.h>

#include <algorithm>
#include <format>
#include <iterator>
#include <optional>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <string_view>

namespace
{

template<typename char_type = char>
class ci_char_traits : public std::char_traits<char_type>
{
    public:
        ci_char_traits() = default;
        virtual ~ci_char_traits() = default;

        static bool eq(const char_type lhs, const char_type rhs)
        {
            return toupper(lhs) == toupper(rhs);
        }

        static bool ne(const char_type lhs, const char_type rhs)
        {
            return !eq(lhs, rhs);
        }

        static bool lt(const char_type lhs, const char_type rhs)
        {
            return toupper(lhs) < toupper(rhs);
        }

        static bool gt(const char_type lhs, const char_type rhs)
        {
            return lt(rhs, lhs); // Use lt but swap rhs & lhs.
        }

        static int compare(const char_type* lhs,
                           const char_type* rhs,
                           const size_t count)
        {
            for (size_t index = 0; index < count; ++index, ++lhs, ++rhs)
            {
                if (lt(*lhs, *rhs)) return -1;
                if (gt(*lhs, *rhs)) return 1;
            }

            return 0;
        }

        static const char_type* find(const char_type* text,
                                     const size_t count,
                                     const char letter)
        {
            for (auto index = 0; index < count && ne(*text, letter); ++index, ++text)
                continue;

            return text;
        }
};

using anagram_key = std::basic_string<char>;

using text = std::basic_string<char>;

using text_set = std::set<text>;

using text_view = std::basic_string_view<char>;

using anagram_dictionary = std::map<anagram_key, text_set>;

struct formatter
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return std::end(ctx);
    }

    template<typename format_context>
    constexpr auto format(const anagram_dictionary& dictionary, format_context& ctx)
    {
        auto&& out = ctx.out();

        format_to(out, "[");

        auto iter = std::begin(dictionary);

        if (iter != std::end(dictionary))
            format_to(out, {}, *iter);

        for (; iter != std::end(dictionary); ++iter)
            format_to(out, ", {}", *iter);

        return format_to(out, "]");
    }
};

/// <summary>
/// Compute the anagram key which is in fact a string itself as to support any character set.
/// The anagram key is preserved in lower case thus 'DOG' and 'dog' are consider as anagrams.
/// The anagram key has it characters sorted. Thus, 'God' and 'dog' both produce the same key of 'dgo'.
/// </summary>
auto compute_anagram(const text_view& text)
{
    anagram_key key;

    std::ranges::copy_if(text,
                         std::back_inserter(key),
                         [](const auto& character)
                         {
                           return std::isalnum(character);
                         });

    std::transform(std::begin(key),
                   std::end(key),
                   std::begin(key),
                   [](const auto& character) { return std::tolower(character); });

    std::ranges::sort(key);

    spdlog::debug("For the text '{}' produced an anagram key of '{}'", text, key);

    return key;
}

auto insert_into_anagram_dictionary(anagram_dictionary& dictionary,
                                    const text& text)
{
    const auto [_, inserted] = dictionary[compute_anagram(text)].insert(text);

    if (inserted)
        spdlog::debug("Inserted '{}' into the anagram dictionary.", text);
    else
        spdlog::debug("The '{}' already exists within the anagrm dictionary.", text);

    return inserted;
}

auto fetch_matching_anagrams(const anagram_dictionary& dictionary,
                             const text_view& text) -> std::optional<text_set>
{
    const auto key_match = dictionary.find(compute_anagram(text));

    return (key_match != std::end(dictionary) && !key_match->second.empty())
         ? std::optional{ key_match->second }
         : std::nullopt;
}

auto to_format(const text_set& matches)
{
    std::string output = "[";

    auto it = std::begin(matches);
    auto end = std::end(matches);

    if (it != end)
        output += std::format("{}", *it);

    for(; it != end; ++it)
        output += std::format(", {}", *it);

    output += "]";

    return output;
}

auto report_matching_anagrams(const anagram_dictionary& dictionary,
                              const text_view& text)
{
    const auto matches = fetch_matching_anagrams(dictionary, text);

    if (matches.has_value())
        spdlog::info("Here is the list of matching anagrams for '{}' are {}.\r\n", text, to_format(matches.value()));
    else
        spdlog::info("No matching anagrams were found for '{}'.\r\n", text);

    return matches.has_value();
}

void load_dictionary(anagram_dictionary& dictionary)
{
    std::stringstream yyz;

    yyz << "bob" << " " << "god" << " " << "dog" << std::flush;

    yyz.clear();
    yyz.seekg(0, std::ios::beg);

    while (!yyz.eof() && yyz.good())
    {
        std::string text;

        yyz >> text;

        insert_into_anagram_dictionary(dictionary, text);
    }
}

}

int main()
{
    spdlog::default_logger_raw()->set_level(spdlog::level::debug);

    anagram_dictionary dictionary;

    load_dictionary(dictionary);

    insert_into_anagram_dictionary(dictionary, "Kayak");
    insert_into_anagram_dictionary(dictionary, "kayak");
    insert_into_anagram_dictionary(dictionary, "C\tA\tT\t");
    insert_into_anagram_dictionary(dictionary, "ooggle!");

    report_matching_anagrams(dictionary, "KAYAK");

    report_matching_anagrams(dictionary, "cat");
    report_matching_anagrams(dictionary, "act");

    report_matching_anagrams(dictionary, "GOD");

    return 0;
}