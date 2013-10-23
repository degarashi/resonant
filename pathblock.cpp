#include "spinner/misc.hpp"
#include "path.hpp"

// -------------------------- PathBlock --------------------------
PathBlock::PathBlock() {}
PathBlock::PathBlock(PathBlock&& p): _path(std::move(p._path)), _segment(std::move(p._segment)), _bAbsolute(p._bAbsolute) {}
PathBlock::PathBlock(spn::To32Str p): PathBlock() {
	setPath(p);
}
template <class ItrO, class ItrI, class CB>
void PathBlock::_ReWriteSC(ItrO dst, ItrI from, ItrI to, char32_t sc, CB cb) {
	int count = 0;
	while(from != to) {
		if(_IsSC(*from)) {
			cb(count);
			count = 0;
			*dst++ = sc;
		} else {
			++count;
			*dst++ = *from;
		}
		++from;
	}
	if(count > 0)
		cb(count);
}
const char32_t PathBlock::SC(U'/'),
				PathBlock::DOT(U'.'),
				PathBlock::EOS(U'\0');
bool PathBlock::_IsSC(char32_t c) {
	return c==U'\\' || c==SC;
}
void PathBlock::setPath(spn::To32Str elem) {
	int len = elem.getLength();
	const auto *ptr = elem.getPtr(),
			   *ptrE = ptr + len;
			   // 分割文字を探し、それに統一しながらsegment数を数える
			   // 絶対パスの先頭SCは除く
			   if((_bAbsolute = _IsSC(*ptr))) {
				   ++ptr;
				   --len;
			   }
			   if(_IsSC(*(ptrE-1))) {
				   --ptrE;
				   --len;
			   }
			   _path.resize(len);
			   _segment.clear();
			   _ReWriteSC(_path.begin(), ptr, ptrE, SC, [this](int n){ _segment.push_back(n); });
}
bool PathBlock::isAbsolute() const {
	return _bAbsolute;
}
PathBlock& PathBlock::operator <<= (spn::To32Str elem) {
	pushBack(elem);
	return *this;
}
void PathBlock::pushBack(spn::To32Str elem) {
	auto *src = elem.getPtr(),
			   *srcE = src + elem.getLength();
			   if(src == srcE)
				   return;
			   if(_IsSC(*src))
				   ++src;
			   if(_IsSC(*(srcE-1)))
				   --srcE;

			   if(!_path.empty())
				   _path.push_back(SC);
			   size_t pathLen = _path.size();
			   _path.resize(pathLen+(srcE-src));
			   _ReWriteSC(_path.begin()+pathLen, src, srcE, SC, [this](int n){ _segment.push_back(n); });
}
void PathBlock::popBack() {
	int sg = segments();
	if(sg > 0) {
		if(sg > 1) {
			_path.erase(_path.end()-_segment.back()-1, _path.end());
			_segment.pop_back();
		} else
			clear();
	}
}
void PathBlock::pushFront(spn::To32Str elem) {
	auto *src = elem.getPtr(),
			   *srcE = src + elem.getLength();
			   if(src == srcE)
				   return;
			   if((_bAbsolute = _IsSC(*src)))
				   ++src;
			   if(_IsSC(*(srcE-1)))
				   --srcE;

			   _path.push_front(SC);
			   _path.insert(_path.begin(), EOS);
			   int ofs = 0;
			   _ReWriteSC(_path.begin(), src, srcE, SC, [&ofs, this](int n){ _segment.insert(_segment.begin()+(ofs++), n); });
}
void PathBlock::popFront() {
	int sg = segments();
	if(sg > 0) {
		if(sg > 1) {
			_path.erase(_path.begin(), _path.begin()+_segment.front()+1);
			_segment.pop_front();
			_bAbsolute = false;
		} else
			clear();
	}
}
std::string PathBlock::plain_utf8(bool bAbs) const {
	return spn::Text::UTFConvertTo8(plain_utf32(bAbs));
}
std::u32string PathBlock::plain_utf32(bool bAbs) const {
	std::u32string s;
	if(bAbs && _bAbsolute)
		s.assign(1,SC);
	if(!_path.empty())
		s.append(_path.begin(), _path.end());
	return std::move(s);
}
std::string PathBlock::getFirst_utf8(bool bAbs) const {
	return spn::Text::UTFConvertTo8(getFirst_utf32(bAbs));
}
std::u32string PathBlock::getFirst_utf32(bool bAbs) const {
	std::u32string s;
	if(bAbs && _bAbsolute)
		s.assign(1,SC);
	if(!_path.empty()) {
		s.append(_path.begin(),
				 _path.begin() + _segment.front());
	}
	return std::move(s);
}
std::string PathBlock::getLast_utf8() const {
	return spn::Text::UTFConvertTo8(getLast_utf32());
}
std::u32string PathBlock::getLast_utf32() const {
	std::u32string s;
	if(!_path.empty()) {
		s.append(_path.end()-_segment.back(),
				 _path.end());
	}
	return std::move(s);
}
int PathBlock::size() const {
	return _path.size() + (isAbsolute() ? 1 : 0);
}
int PathBlock::segments() const {
	return _segment.size();
}
void PathBlock::clear() {
	_path.clear();
	_segment.clear();
	_bAbsolute = false;
}
std::string PathBlock::getExtension(bool bRaw) const {
	std::string rt;
	if(segments() > 0) {
		auto ts = getLast_utf32();
		const auto* str = ts.c_str();
		char32_t tc[64];
		int wcur = 0;
		bool bFound = false;
		for(;;) {
			auto c = *str++;
			if(c == EOS) {
				if(bFound) {
					tc[wcur] = EOS;
					if(bRaw) {
						// 末尾の数字を取り去る
						--wcur;
						while(wcur>=0 && std::isdigit(static_cast<char>(tc[wcur])))
							tc[wcur--] = EOS;
					}
					rt = spn::Text::UTFConvertTo8(tc);
				}
				break;
			} else if(c == DOT)
				bFound = true;
			else if(bFound)
				tc[wcur++] = c;
		}
	}
	return std::move(rt);
}
int PathBlock::getExtNum() const {
	if(segments() > 0) {
		auto ts = getLast_utf8();
		return _ExtGetNum(ts);
	}
	return -1;
}
int PathBlock::_ExtGetNum(const std::string& ext) {
	int wcur = 0;
	const auto* str = ext.c_str();
	for(;;) {
		auto c = *str++;
		if(std::isdigit(c)) {
			// 数字部分を取り出す
			return std::atol(str-1);
		} else if(c == '\0')
			break;
	}
	return 0;
}
int PathBlock::addExtNum(int n) {
	if(segments() > 0) {
		auto ts = getExtension(false);
		n = _ExtIncNum(ts, n);
		setExtension(std::move(ts));
		return n;
	}
	return -1;
}
void PathBlock::setExtension(spn::To32Str ext) {
	if(segments() > 0) {
		auto ts = getLast_utf32();
		const auto* str = ts.c_str();
		int rcur = 0;
		while(str[rcur]!=EOS && str[rcur++]!=DOT);
		if(str[rcur] == EOS) {
			// 拡張子を持っていない
			ts += DOT;
		} else {
			// 拡張子を持っている
			ts.resize(rcur);
		}
		ts.append(ext.getPtr(), ext.getPtr()+ext.getSize());
		popBack();
		pushBack(std::move(ts));
	}
}
int PathBlock::_ExtIncNum(std::string& ext, int n) {
	int wcur = 0,
	rcur = 0;
	char tc[32] = {};
	const auto* str = ext.c_str();
	for(;;) {
		auto c = str[rcur];
		if(std::isdigit(c)) {
			// 数字部分を取り出す
			n += std::atol(str+rcur);
			ext.resize(rcur);
			std::sprintf(tc, "%d", n);
			ext += tc;
			return n;
		} else if(c == '\0')
			break;
		++rcur;
	}
	std::sprintf(tc, "%d", n);
	ext += tc;
	return n;
}
