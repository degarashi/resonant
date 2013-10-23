#pragma once

#include <string>
#include <chrono>
#include <functional>
#include "spinner/abstbuff.hpp"

#include <deque>
class PathBlock {
	protected:
		using Path = std::deque<char32_t>;
		using Segment = std::deque<int>;
		//! 絶対パスにおける先頭スラッシュを除いたパス
		Path		_path;
		//! 区切り文字の前からのオフセット
		Segment		_segment;
		const static char32_t SC,		//!< セパレート文字
		DOT,		//!< 拡張子の前の記号
		EOS;		//!< 終端記号
		bool		_bAbsolute;

		static bool _IsSC(char32_t c);
		//! パスを分解しながらdstに書き込み
		template <class ItrO, class ItrI, class CB>
		static void _ReWriteSC(ItrO dst, ItrI from, ItrI to, char32_t sc, CB cb);

		static int _ExtGetNum(const std::string& ext);
		static int _ExtIncNum(std::string& ext, int n=1);
		template <class CB>
		void _iterateSegment(const char32_t* c, int len, char32_t sc, CB cb) {
			char32_t tc[128];
			int wcur = 0;
			while(*c != U'\0') {
				if(_IsSC(*c)) {
					tc[wcur] = U'\0';
			cb(tc, wcur);
			wcur = 0;
				} else
					tc[wcur++] = *c;
				++c;
			}
		}

	public:
		PathBlock();
		PathBlock(const PathBlock&) = default;
		PathBlock(PathBlock&& p);
		PathBlock(spn::To32Str p);

		PathBlock& operator <<= (spn::To32Str elem);

		void pushBack(spn::To32Str elem);
		void popBack();
		void pushFront(spn::To32Str elem);
		void popFront();

		std::string plain_utf8(bool bAbs=true) const;
		std::string getFirst_utf8(bool bAbs=true) const;
		std::string getLast_utf8() const;
		std::u32string plain_utf32(bool bAbs=true) const;
		std::u32string getFirst_utf32(bool bAbs=true) const;
		std::u32string getLast_utf32() const;

		int size() const;
		int segments() const;
		void setPath(spn::To32Str p);
		bool isAbsolute() const;
		bool hasExtention() const;
		void setExtension(spn::To32Str ext);
		std::string getExtension(bool bRaw=false) const;
		int getExtNum() const;
		int addExtNum(int n=1);
		void clear();
};