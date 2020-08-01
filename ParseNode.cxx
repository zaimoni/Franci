#include "ParseNode.hxx"

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
