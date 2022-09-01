#ifndef HTML_TAG_HPP
#define HTML_TAG_HPP 1

#include "Zaimoni.STL/LexParse/LexNode.hpp"

class HTMLtag : public formal::parsed {
	using kv_pairs_t = std::vector < std::pair<std::string, std::string> >;
	std::string _tag_name;
	std::shared_ptr<const kv_pairs_t> kv_pairs; // could use std::map instead
	formal::src_location _origin;
	unsigned long long _bitmap;

	static constexpr const decltype(_bitmap) Start = (1ULL << 0);
	static constexpr const decltype(_bitmap) End = (1ULL << 1);

public:
	enum class mode : decltype(_bitmap) {
		opening = Start,
			closing = End,
			self_closing = Start | End
	};

	// if we are using HTML tags, we should also care about HTML entities
	static constexpr const unsigned long long Entity = (1ULL << 1); // reserve this flag for both word and lex_node

	static_assert(!(formal::Comment & Entity));
	static_assert(!(formal::Error & Entity));
	static_assert(!(formal::Inert_Token & Entity));
	static_assert(!(formal::Tokenized & Entity));

	HTMLtag() = delete;
	HTMLtag(const HTMLtag& src) = default;
	HTMLtag(HTMLtag&& src) = default;
	HTMLtag& operator=(const HTMLtag& src) = default;
	HTMLtag& operator=(HTMLtag&& src) = default;
	~HTMLtag() = default;

	HTMLtag(const std::string& tag, mode code, formal::src_location origin) : _tag_name(tag), _origin(origin), _bitmap(decltype(_bitmap)(code)) {}
	HTMLtag(std::string&& tag, mode code, formal::src_location origin) noexcept : _tag_name(std::move(tag)), _origin(origin), _bitmap(decltype(_bitmap)(code)) {}
	HTMLtag(const std::string& tag, mode code, formal::src_location origin, decltype(kv_pairs) data) : _tag_name(tag), kv_pairs(data), _origin(origin), _bitmap(decltype(_bitmap)(code)) {}
	HTMLtag(std::string&& tag, mode code, formal::src_location origin, decltype(kv_pairs) data) noexcept : _tag_name(std::move(tag)), kv_pairs(data), _origin(origin), _bitmap(decltype(_bitmap)(code)) {}

	std::unique_ptr<parsed> clone() const override { return std::unique_ptr<parsed>(new HTMLtag(*this)); }
	void CopyInto(parsed*& dest) const override { zaimoni::CopyInto(*this, dest); }	// polymorphic assignment
	void MoveInto(parsed*& dest) override { zaimoni::MoveIntoV2(std::move(*this), dest); }	// polymorphic move

	auto tag_type() const { return (mode)(_bitmap & (Start | End)); }
	const std::string& tag_name() const { return _tag_name; }

	formal::src_location origin() const override { return _origin; }

	static const std::string* is_balanced_pair(const formal::lex_node& src);
	static bool is_balanced_pair(const formal::lex_node& src, const std::string& target);
	std::string to_s() const override;
	static std::unique_ptr<HTMLtag> parse(kuroda::parser<formal::lex_node>::sequence& src, size_t viewpoint);
};

#endif
