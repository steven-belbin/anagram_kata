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
using anagram_key = std::basic_string<char>;

using text = std::basic_string<char>;

using text_set = std::set<text>;

using text_view = std::basic_string_view<char>;

using anagram_dictionary = std::map<anagram_key, text_set>;

/// <summary>
/// Formatter of the set of text entries that are stored in a anagram dictionary.
/// </summary>
template<class char_type>
struct std::formatter<text_set, char_type>
{
    constexpr auto parse(auto& ctx)
    {
        return std::end(ctx);
    }

    constexpr auto format(const text_set& collection, auto& ctx)
    {
        format_to(ctx.out(), "[");
        format_entries(collection, ctx);
        return format_to(ctx.out(), "]");
    }

private:

    constexpr auto format_entries(const text_set& collection, auto& ctx)
    {
        auto it = std::begin(collection);
        const auto& end = std::end(collection);

        if (it != end)
        {
            format_to(ctx.out(), "{}", *it);
            ++it;
        }

        for (; it != end; ++it)
            format_to(ctx.out(), ", {}", *it);

        return ctx;
    }
};

/// <summary>
/// Computes the anagram key which is in fact a string itself as to support any character set.
/// The anagram key 
///     1. consists of only alpha-numeric characters. Thus symbols are filtered out.
///     2. is preserved in lower case thus 'DOG' and 'dog' are consider as anagrams.
///     3. characters within the anagram key string are sorts. Thus, 'God' and 'dog' are produce 'dgo' as their key.
///     4. an empty string indicates that the text cannot be used in an anagram.
/// </summary>
auto compute_anagram(const text_view& text) -> std::optional<anagram_key>
{
    anagram_key key;

    std::ranges::copy_if(text,
                         std::back_inserter(key),
                         [](const auto& character)
                         {
                           return std::isalnum(character);
                         });

    if (key.empty())
    {
        spdlog::error("Failed to compute a valid anagram key for the text '{}'.", text);
        return std::nullopt;
    }

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
    const auto key = compute_anagram(text);

    if (!key)
        return false;

    const auto [_, inserted] = dictionary[key.value()].insert(text);

    if (inserted)
        spdlog::debug("Inserted '{}' into the anagram dictionary.", text);
    else
        spdlog::debug("The '{}' already exists within the anagrm dictionary.", text);

    return inserted;
}

auto fetch_matching_anagrams(const anagram_dictionary& dictionary,
                             const text_view& text) -> std::optional<text_set>
{
    const auto key = compute_anagram(text);

    if (!key)
        return std::nullopt;

    const auto key_match = dictionary.find(key.value());

    return (key_match != std::end(dictionary) && !key_match->second.empty())
         ? std::optional{ key_match->second }
         : std::nullopt;
}

auto report_matching_anagrams(const anagram_dictionary& dictionary,
                              const text_view& text)
{
    const auto matches = fetch_matching_anagrams(dictionary, text);

    if (matches)
        spdlog::info("Here is the list of matching anagrams for '{}' are {}.\r\n", text, std::format("{}", matches.value()));
    else
        spdlog::info("No matching anagrams were found for '{}'.\r\n", text);

    return matches.has_value();
}

void load_dictionary(anagram_dictionary& dictionary,
                     std::istream& input_stream)
{
    while (!input_stream.eof() && input_stream.good())
    {
        std::string text;
        input_stream >> text;

        if (input_stream.good())
            insert_into_anagram_dictionary(dictionary, text);
    }
}

void load_dictionary(anagram_dictionary& dictionary)
{
    std::stringstream text_stream;

    text_stream << "bob" << '\n'
                << "god" << '\n'
                << "act" << '\n'
                << "dog" << std::flush;

    text_stream.clear();
    text_stream.seekg(0, std::ios::beg);

    load_dictionary(dictionary, text_stream);
}
}

int main()
{
    spdlog::default_logger_raw()->set_level(spdlog::level::info);

    anagram_dictionary dictionary;

    load_dictionary(dictionary);

    insert_into_anagram_dictionary(dictionary, "Kayak");
    insert_into_anagram_dictionary(dictionary, "kayak");
    insert_into_anagram_dictionary(dictionary, "C\tA\tT\t");
    insert_into_anagram_dictionary(dictionary, "***Cat***");
    insert_into_anagram_dictionary(dictionary, "dog");
    insert_into_anagram_dictionary(dictionary, "###");

    report_matching_anagrams(dictionary, "KAYAK");
    report_matching_anagrams(dictionary, "cat");
    report_matching_anagrams(dictionary, "act");
    report_matching_anagrams(dictionary, "GOD");
    report_matching_anagrams(dictionary, "unknown");
    report_matching_anagrams(dictionary, "###");

    return 0;
}