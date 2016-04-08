#pragma once
#include "spinner/vector.hpp"
#include "spinner/size.hpp"

struct IHash {
	virtual int getHash(int x, int y) const = 0;
	virtual spn::Vec2 getVector(int x, int y) const = 0;
};
using Hash_SP = std::shared_ptr<IHash>;
class Hash2D : public IHash {
	private:
		using Table = std::vector<int>;
		Table	_table;
		using Vectors = std::vector<spn::Vec2>;
		Vectors	_vectors;

		void _initVectors(spn::PowInt size);
	public:
		Hash2D(spn::MTRandom& mt, spn::PowInt size);
		static Hash_SP Create(uint32_t seed, int size);

		int getHash(int x, int y) const override;
		spn::Vec2 getVector(int x, int y) const override;
};

struct IHashVec {
	virtual float getElev(float x, float y) const = 0;
	virtual spn::RangeF getRange() const = 0;
};
using HashVec_SP = std::shared_ptr<IHashVec>;

class ClipHash : public IHashVec {
	private:
		Hash_SP				_hash;
		const spn::PowInt	_scale;
	public:
		ClipHash(const Hash_SP& h, spn::PowInt scale);
		static HashVec_SP Create(const Hash_SP& h, int scale);

		float getElev(float x, float y) const override;
		spn::RangeF getRange() const override;
};
class ClipHashMod : public IHashVec {
	private:
		HashVec_SP			_hash;
		float				_ox, _oy,
							_rx, _ry,
							_er;
	public:
		ClipHashMod(const HashVec_SP& h);
		static HashVec_SP Create(const HashVec_SP& h,
								float ox, float oy,
								float rx, float ry,
								float e);

		void setOffset(float x, float y);
		void setRatio(float x, float y);
		void setElevRatio(float e);
		float getElev(float x, float y) const override;
		spn::RangeF getRange() const override;
};
class ClipHashV : public IHashVec {
	private:
		using HashV = std::vector<HashVec_SP>;
		HashV	_hashV;
	public:
		static HashVec_SP Create(const HashV& v);
		void addHash(const HashVec_SP& h);
		float getElev(float x, float y) const override;
		spn::RangeF getRange() const override;
};
