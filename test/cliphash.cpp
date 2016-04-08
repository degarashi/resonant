#include "cliphash.hpp"
#include "spinner/random.hpp"
#include "spinner/matrix.hpp"

// -------- Hash2D --------
Hash2D::Hash2D(spn::MTRandom& mt, const spn::PowInt size):
	_table(size*2)
{
	_initVectors(size);
	const auto itr = _table.begin(),
				itrE = itr+size;
	std::iota(itr, itrE, 0);
	std::shuffle(itr, itrE, mt.refMt());
	// 同じのを2回繰り返す
	std::copy(itr, itrE, itrE);
}
Hash_SP Hash2D::Create(uint32_t seed, const int size) {
	constexpr uint32_t Id = 0x10002000;
	mgr_random.initEngine(Id);
	auto rd = mgr_random.get(Id);
	rd.refMt().seed(seed);
	return std::make_shared<Hash2D>(rd, size);
}
void Hash2D::_initVectors(const spn::PowInt size) {
	const int isize = static_cast<int>(size);
	_vectors.resize(isize);
	float cur = 0;
	const float diff = spn::RadF::OneRotationAng / isize;
	for(int i=0 ; i<isize ; i++, cur+=diff) {
		_vectors[i] = spn::Vec2{0,1} * spn::Mat22::Rotation(spn::RadF(cur));
	}
}
int Hash2D::getHash(const int x, const int y) const {
	const int mask = _table.size()/2 -1;
	return _table[(x&mask) + _table[y&mask]];
}
spn::Vec2 Hash2D::getVector(const int x, const int y) const {
	return _vectors[getHash(x,y)];
}

#include "spinner/rectdiff.hpp"
// -------- ClipHash --------
ClipHash::ClipHash(const Hash_SP& h, const spn::PowInt scale):
	_hash(h),
	_scale(scale)
{}
HashVec_SP ClipHash::Create(const Hash_SP& h, const int scale) {
	return std::make_shared<ClipHash>(h, scale);
}

namespace {
	float F(const float t) {
		return t*t*t * (t * (6*t - 15) + 10);
	}
}
float ClipHash::getElev(const float x, const float y) const {
	using spn::Vec2;
	using spn::rect::LoopValue;
	using spn::rect::LoopValueD;
	const int	iy0 = LoopValueD(float(y) / _scale, 1.f),
				iy1 = iy0 + 1;
	const float ty = float(y) / _scale;
	const float uy0 = ty - iy0;

	const float tx = float(x) / _scale;
	const int	ix0 = LoopValueD(float(x) / _scale, 1.f);
	auto fnGrad_dx = [hash = _hash.get(), tx,ty](const int ix, const int iy){
		const auto g0 = hash->getVector(ix, iy),
					g1 = hash->getVector(ix+1, iy);
		const float n0 = g0.dot(Vec2{tx-ix, ty-iy}),
					n1 = g1.dot(Vec2{tx-(ix+1), ty-iy});
		const float ux0 = tx - ix;
		return n0*(1 - F(ux0)) + n1*F(ux0);
	};
	const float nx0 = fnGrad_dx(ix0, iy0),
				nx1 = fnGrad_dx(ix0, iy1);
	const float n0 = nx0*(1 - F(uy0)) + nx1*F(uy0);
	return n0;
}
spn::RangeF ClipHash::getRange() const {
	return {-1.f, 1.f};
}

// -------- ClipHashMod --------
ClipHashMod::ClipHashMod(const HashVec_SP& h):
	_hash(h),
	_ox(0), _oy(0),
	_rx(1), _ry(1),
	_er(1)
{}
HashVec_SP ClipHashMod::Create(const HashVec_SP& h,
								const float ox, const float oy,
								const float rx, const float ry,
								const float e)
{
	auto sp = std::make_shared<ClipHashMod>(h);
	sp->setOffset(ox, oy);
	sp->setRatio(rx, ry);
	sp->setElevRatio(e);
	return sp;
}
void ClipHashMod::setOffset(const float x, const float y) {
	_ox = x;
	_oy = y;
}
void ClipHashMod::setRatio(const float x, const float y) {
	_rx = x;
	_ry = y;
}
void ClipHashMod::setElevRatio(const float e) {
	_er = e;
}
float ClipHashMod::getElev(const float x, const float y) const {
	return _hash->getElev( x*_rx + _ox, y*_ry + _oy) * _er;
}
spn::RangeF ClipHashMod::getRange() const {
	return _hash->getRange() * _er;
}

// -------- ClipHashV --------
HashVec_SP ClipHashV::Create(const HashV& v) {
	auto sp = std::make_shared<ClipHashV>();
	for(auto& vt : v)
		sp->addHash(vt);
	return sp;
}
void ClipHashV::addHash(const HashVec_SP& h) {
	_hashV.emplace_back(h);
}
float ClipHashV::getElev(const float x, const float y) const {
	float res = 0;
	for(auto& h : _hashV)
		res += h->getElev(x,y);
	return res;
}
spn::RangeF ClipHashV::getRange() const {
	spn::RangeF r{0,0};
	for(auto& h : _hashV) {
		const auto r2 = h->getRange();
		r.from += std::min(0.f, r2.from);
		r.to += std::max(0.f, r2.to);
	}
	return r;
}
