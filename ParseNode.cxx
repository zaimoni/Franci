#include "ParseNode.hxx"

ParseNode::ParseNode(kuroda::parser<MetaConcept>::sequence& dest, size_t lb, size_t ub, slice mode)
: MetaConcept(ParseNode_MC)
{
	assert(lb < ub);
	assert(dest.size() > ub);
	switch(mode) {
	case CLOSED: {
		const auto audit = dest.size();	// remove before commit
		const size_t delta = ub - lb;
		if (2 <= delta) {
			decltype(_infix) staging(delta - 1);
			memmove(staging.c_array(), dest.data()+lb+1, sizeof(MetaConcept*)*(delta-1));
			std::fill_n(dest.c_array() + lb + 1, delta - 1, nullptr);
			staging.swap(_infix);
		}
		_anchor.reset(dest[lb]);
		_post_anchor.reset(dest[ub]);
		dest[lb] = this;
		dest.DeleteNSlotsAt(delta,lb+1);
		SUCCEED_OR_DIE(audit == dest.size() + delta);
	}
		break;
	default: SUCCEED_OR_DIE(0 && "unhandled mode");
	}
}

size_t ParseNode::size() const {
	size_t ret = _prefix.size();
	if (!_anchor.empty()) ret += 1;
	ret += _infix.size();
	if (!_post_anchor.empty()) ret += 1;
	ret += _postfix.size();
	return ret;
}

const MetaConcept* ParseNode::ArgN(size_t n) const {
	if (!_prefix.empty()) {
		if (_prefix.size() > n) return _prefix[n];
		n -= _prefix.size();
	}
	if (!_anchor.empty()) {
		if (0 == n) return _anchor;
		--n;
	}
	if (!_infix.empty()) {
		if (_infix.size() > n) return _infix[n];
		n -= _infix.size();
	}
	if (!_post_anchor.empty()) {
		if (0 == n) return _post_anchor;
		--n;
	}
	if (!_postfix.empty()) {
		if (_postfix.size() > n) return _postfix[n];
		n -= _postfix.size();
	}
	return 0;
}

MetaConcept* ParseNode::ArgN(size_t n) {
	if (!_prefix.empty()) {
		if (_prefix.size() > n) return _prefix[n];
		n -= _prefix.size();
	}
	if (!_anchor.empty()) {
		if (0 == n) return _anchor;
		--n;
	}
	if (!_infix.empty()) {
		if (_infix.size() > n) return _infix[n];
		n -= _infix.size();
	}
	if (!_post_anchor.empty()) {
		if (0 == n) return _post_anchor;
		--n;
	}
	if (!_postfix.empty()) {
		if (_postfix.size() > n) return _postfix[n];
		n -= _postfix.size();
	}
	return 0;
}

size_t ParseNode::apply_all_infix(std::function<bool(MetaConcept*&)> xform)
{
	size_t ret = 0;
	for (decltype(auto) x : _infix) ret += xform(x);
	return ret;
}

std::string ParseNode::to_s_aux() const
{
	std::string ret("#");
	bool first = true;
	if (!_prefix.empty()) for (decltype(auto) x : _prefix) {
		if (!first) ret += ' ';
		if (x) ret += x->to_s();
		else ret += "\"\"";
		first = false;
	}
	if (!_anchor.empty()) {
		if (!first) ret += ' ';
		ret += _anchor->to_s();
		first = false;
	}
	if (!_infix.empty())  for (decltype(auto) x : _infix) {
		if (!first) ret += ' ';
		if (x) ret += x->to_s();
		else ret += "\"\"";
		first = false;
	}
	if (!_post_anchor.empty()) {
		if (!first) ret += ' ';
		ret += _post_anchor->to_s();
		first = false;
	}
	if (!_postfix.empty()) for (decltype(auto) x : _postfix) {
		if (!first) ret += ' ';
		if (x) ret += x->to_s();
		else ret += "\"\"";
		first = false;
	}
	ret += '#';
	return ret;
}