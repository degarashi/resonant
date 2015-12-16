#include "test.hpp"
#include "engine.hpp"
#include "primitive.hpp"
#include "geometry.hpp"

// ---------------------- Primitive ----------------------
rs::WVb Primitive::s_wVb[Type::Num][2];
rs::WIb Primitive::s_wIb[Type::Num][2];
rs::WVb Primitive::s_wVbLine[Type::Num][2];
const rs::IdValue Primitive::T_Prim = GlxId::GenTechId("Primitive", "Default"),
				Primitive::T_PrimDepth = GlxId::GenTechId("Primitive", "Depth"),
				Primitive::T_PrimCube = GlxId::GenTechId("Primitive", "CubeDefault"),
				Primitive::T_PrimCubeDepth = GlxId::GenTechId("Primitive", "CubeDepth"),
				Primitive::T_PrimLine = GlxId::GenTechId("Primitive", "Line");
void Primitive::_initVb(Type::E typ, bool bFlat, bool bFlip) {
	int indexI = bFlip ? 1 : 0;

	if(!(_hlVb = s_wVb[typ][indexI].lock())) {
		boom::Vec3V tmpPos;
		boom::IndexV tmpIndex;
		boom::Vec2V tmpUv;
		spn::Pose3D pose;
		pose.setRot(spn::Quat::RotationZ(spn::DegF(1.f)));
		if(typ != Type::Cube) {
			pose.setScale({1,1,2});
			pose.setOffset({0, 0, -1});
		}
		switch(typ) {
			case Type::Cone:
				boom::geo3d::Geometry::MakeCone(tmpPos, tmpIndex, 16);
				break;
			case Type::Cube:
				boom::geo3d::Geometry::MakeCube(tmpPos, tmpIndex);
				break;
			case Type::Sphere:
				boom::geo3d::Geometry::MakeSphere(tmpPos, tmpIndex, 32, 16);
				break;
			case Type::Torus:
				boom::geo3d::Geometry::MakeTorus(tmpPos, tmpIndex, 0.5f, 64, 32);
				break;
			case Type::Capsule:
				boom::geo3d::Geometry::MakeCapsule(tmpPos, tmpIndex, 1, 8);
				break;
			default:
				AssertF(Trap, "invalid type")
		};
		if(bFlip)
			boom::FlipFace(tmpIndex.begin(), tmpIndex.end(), tmpIndex.begin(), 0);

		if(typ == Type::Torus)
			boom::geo3d::Geometry::UVUnwrapCylinder(tmpUv, pose, tmpPos);
		else
			boom::geo3d::Geometry::UVUnwrapSphere(tmpUv, 2,2, pose, tmpPos);

		boom::Vec3V posv, normalv;
		boom::Vec2V uvv;
		boom::IndexV indexv;
		if(bFlat) {
			boom::geo3d::Geometry::MakeVertexNormalFlat(posv, indexv, normalv, uvv,
														tmpPos, tmpIndex, tmpUv);
		} else {
			boom::geo3d::Geometry::MakeVertexNormal(normalv, tmpPos, tmpIndex);
			posv = tmpPos;
			uvv = tmpUv;
			indexv = tmpIndex;
		}
		if(typ == Type::Cube)
			boom::geo3d::Geometry::UVUnwrapCube(uvv, pose, posv, indexv);

		boom::Vec4V tanv;
		boom::geo3d::Geometry::CalcTangent(tanv, posv, indexv, normalv, uvv);

		// 大きさ1の立方体を定義しておいて後で必要に応じてスケーリングする
		const int nV = posv.size();
		std::vector<vertex::prim_tan> tmpV(nV);
		for(int i=0 ; i<nV ; i++) {
			auto& v = tmpV[i];
			v.pos = posv[i];
			v.tex = uvv[i];
			v.normal = normalv[i];
			v.tangent_c = tanv[i];
		}
		_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
		_hlVb.ref()->initData(std::move(tmpV));
		s_wVb[typ][indexI] = _hlVb.weak();
		_hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
		_hlIb.ref()->initData(std::move(indexv));
		_hlVbLine = _MakeVbLine(posv, normalv, tanv);
		s_wIb[typ][indexI] = _hlIb.weak();
		s_wVbLine[typ][indexI] = _hlVbLine.weak();
	} else {
		_hlIb = s_wIb[typ][indexI].lock();
		_hlVbLine = s_wVbLine[typ][indexI].lock();
	}
}
rs::HLVb Primitive::_MakeVbLine(const Vec3V& srcPos, const Vec3V& srcNormal, const Vec4V& srcTangentC)
{
	const spn::Vec4 c_color[3] = {
		{1,0,0,1},
		{0,1,0,1},
		{0,0,1,1}
	};
	auto fnAddLine = [](auto* dst, const auto& origin, const auto& dir, const auto& c){
		dst[0].pos = dst[1].pos = origin;
		dst[0].dir = spn::Vec3(0);
		dst[0].color = dst[1].color = c;
		dst[1].dir = dir;
	};
	const int nV = srcPos.size();
	std::vector<vertex::line> vtx(nV*6);
	for(int i=0 ; i<nV ; i++) {
		auto* dst = vtx.data() + i*6;
		spn::Vec3 dir[3];
		auto &x = dir[0],
			&y = dir[1],
			&z = dir[2];
		z = srcNormal[i];
		x = srcTangentC[i].asVec3();
		x -= z*x.dot(z);
		x.normalize();
		y = z.cross(x) * srcTangentC[i].w;
		y -= z*x.dot(z) + y*x.dot(y);
		y.normalize();

		// center, axis-x, axis-y, axis-z
		for(int j=0 ; j<3 ; j++)
			fnAddLine(dst+j*2, srcPos[i], dir[j], c_color[j]);
	}
	rs::HLVb hl = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	hl.ref()->initData(std::move(vtx));
	return hl;
}
Primitive::Primitive(float s, rs::HTex hTex, rs::HTex hTexNormal, Type::E typ, bool bFlat, bool bFlip):
	_hlTex(hTex),
	_hlTexNormal(hTexNormal),
	_bShowNormal(false)
{
	setScale({s,s,s});
	_initVb(typ, bFlat, bFlip);
}
void Primitive::advance(float t) {
	const spn::AQuat& q = this->getRot();
	auto& self = const_cast<Primitive&>(*this);
	self.setRot(q >> spn::AQuat::Rotation(spn::AVec3(1,1,0).normalization(), spn::RadF(t*0.01f)));
}
void Primitive::draw(Engine& e) const {
	auto typ = e.getDrawType();
	switch(typ) {
		case Engine::DrawType::Normal:
			e.setTechPassId(T_Prim);
			e.setUniform(rs::unif3d::texture::Diffuse, _hlTex);
			if(_hlTexNormal)
				e.setUniform(rs::unif3d::texture::Normal, _hlTexNormal);
			break;
		case Engine::DrawType::CubeNormal:
			e.setTechPassId(T_PrimCube);
			e.setUniform(rs::unif3d::texture::Diffuse, _hlTex);
			if(_hlTexNormal)
				e.setUniform(rs::unif3d::texture::Normal, _hlTexNormal);
			break;
		case Engine::DrawType::CubeDepth:
			e.setTechPassId(T_PrimCubeDepth);
			break;
		case Engine::DrawType::Depth:
			e.setTechPassId(T_PrimDepth);
			break;
		default:
			AssertF(Trap, "")
	}
	e.setVDecl(rs::DrawDecl<vdecl::prim_tan>::GetVDecl());
	e.ref<rs::SystemUniform3D>().setWorld(getToWorld().convertA44());
	e.setVStream(_hlVb, 0);
	e.setIStream(_hlIb);
	e.drawIndexed(GL_TRIANGLES, _hlIb->get()->getNElem());

	// 法線を表示
	if(_bShowNormal && typ == Engine::DrawType::Normal) {
		e.setVDecl(rs::DrawDecl<vdecl::line>::GetVDecl());
		e.setTechPassId(T_PrimLine);
		e.setVStream(_hlVbLine, 0);
		e.draw(GL_LINES, 0, _hlVbLine->get()->getNElem());
	}
}
void Primitive::exportDrawTag(rs::DrawTag& d) const {
	d.idTex[0] = _hlTex.get();
}
void Primitive::showNormals(bool b) {
	_bShowNormal = b;
}

// ---------------------- Primitive頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<vdecl::prim>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},
		{0,20, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::NORMAL}
	});
	return vd;
}
const rs::SPVDecl& rs::DrawDecl<vdecl::prim_tan>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		// from vdecl::prim
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},
		{0,20, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::NORMAL},

		{0,32, GL_FLOAT, GL_FALSE, 4, (GLuint)rs::VSem::TANGENT}
	});
	return vd;
}
const rs::SPVDecl& rs::DrawDecl<vdecl::line>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0, 12, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::TEXCOORD0},
		{0,24, GL_FLOAT, GL_FALSE, 4, (GLuint)rs::VSem::COLOR}
	});
	return vd;
}

// ---------------------- PrimitiveObj ----------------------
struct PrimitiveObj::St_Default : StateT<St_Default> {
	void onDraw(const PrimitiveObj& self, rs::IEffect& e) const override {
		self.draw(static_cast<Engine&>(e));
	}
};
PrimitiveObj::PrimitiveObj(float size, rs::HTex hTex, rs::HTex hTexNormal, Type::E typ, bool bFlat, bool bFlip):
	Primitive(size, hTex, hTexNormal, typ, bFlat, bFlip)
{
	setStateNew<St_Default>();
}
#include "../luaimport.hpp"
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, PrimitiveObj, PrimitiveObj, "DrawableObj", NOTHING,
		(showNormals)
		(advance)
		(setOffset),
		(float)(rs::HTex)(rs::HTex)(Primitive::Type::E)(bool)(bool))
