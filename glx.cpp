#include "glx.hpp"
#include "spinner/common.hpp"
#include "spinner/matrix.hpp"
#include <boost/format.hpp>
#include <fstream>
#include "spinner/unituple/operator.hpp"
#include "util/screen.hpp"
#include "systeminfo.hpp"
#include "adaptsdl.hpp"
#include "drawtask.hpp"

namespace rs {
	IEffect::GlxId IEffect::s_myId;
	const int DefaultUnifPoolSize = 0x100;
	draw::TokenBuffer* MakeUniformTokenBuffer(UniMap& um, UnifPool& pool, GLint id) {
		auto itr = um.find(id);
		if(itr == um.end()) {
			return um[id] = pool.construct();
		}
		return itr->second;
	}

	const GLType_ GLType;
	const GLInout_ GLInout;
	const GLSem_ GLSem;
	const GLPrecision_ GLPrecision;
	const GLBoolsetting_ GLBoolsetting;
	const GLSetting_ GLSetting;
	const GLStencilop_ GLStencilop;
	const GLFunc_ GLFunc;
	const GLEq_ GLEq;
	const GLBlend_ GLBlend;
	const GLFace_ GLFace;
	const GLFacedir_ GLFacedir;
	const GLColormask_ GLColormask;
	const GLShadertype_ GLShadertype;
	const GLBlocktype_ GLBlocktype;

	const char* GLType_::cs_typeStr[] = {
		BOOST_PP_SEQ_FOR_EACH(PPFUNC_STR, EMPTY, SEQ_GLTYPE)
	};
	const char* GLSem_::cs_typeStr[] = {
		BOOST_PP_SEQ_FOR_EACH(PPFUNC_STR, EMPTY, SEQ_VSEM)
	};
	const VSFunc ValueSettingR::cs_func[] = {
		BOOST_PP_SEQ_FOR_EACH(PPFUNC_GLSET_FUNC, EMPTY, SEQ_GLSETTING)
	};
	const int ValueSettingR::cs_funcNArg[] = {
		BOOST_PP_SEQ_FOR_EACH(PPFUNC_GLSET_NARG, EMPTY, SEQ_GLSETTING)
	};
	const char* ValueSettingR::cs_funcName[] = {
		BOOST_PP_SEQ_FOR_EACH(PPFUNC_GLSET_NAME, EMPTY, SEQ_GLSETTING)
	};
	const char* GLPrecision_::cs_typeStr[] = {
		BOOST_PP_SEQ_FOR_EACH(PPFUNC_STR, EMPTY, SEQ_PRECISION)
	};
	const char* GLSetting_::cs_typeStr[] = {
		BOOST_PP_SEQ_FOR_EACH(PPFUNC_STR, EMPTY, SEQ_GLSETTING)
	};
	const char* GLBlocktype_::cs_typeStr[] = {
		BOOST_PP_SEQ_FOR_EACH(PPFUNC_STR, EMPTY, SEQ_BLOCK)
	};

	// -------------- ValueSettingR --------------
	ValueSettingR::ValueSettingR(const ValueSetting& s) {
		func = cs_func[s.type];
		int nV = std::min(s.value.size(), countof(value));
		// 引数の数が合わなかったらエラー
		const int ArgLength = cs_funcNArg[s.type];
		Assert(Throw, ArgLength==nV, "amount of argument(s) is not valid(func=%1%, required=%2%, actual=%3%)",
				cs_funcName[s.type], ArgLength, nV)
		for(int i=0 ; i<nV ; i++)
			value[i] = s.value[i];
		for(int i=nV ; i<static_cast<int>(countof(value)) ; i++)
			value[i] = boost::blank();
	}
	void ValueSettingR::action() const { func(*this); }
	bool ValueSettingR::operator == (const ValueSettingR& s) const {
		for(int i=0 ; i<static_cast<int>(countof(value)) ; i++)
			if(!(value[i] == s.value[i]))
				return false;
		return func == s.func;
	}

	// -------------- BoolSettingR --------------
	const VBFunc BoolSettingR::cs_func[] = {
		&IGL::glEnable, &IGL::glDisable
	};
	BoolSettingR::BoolSettingR(const BoolSetting& s) {
		func = cs_func[(s.value) ? 0 : 1];
		flag = s.type;
	}
	void BoolSettingR::action() const {
		(GL.*func)(flag); }
	bool BoolSettingR::operator == (const BoolSettingR& s) const {
		return flag==s.flag && func==s.func;
	}

	GLEffect::EC_FileNotFound::EC_FileNotFound(const std::string& fPath):
		EC_Base((boost::format("file path: \"%1%\" was not found.") % fPath).str()) {}

	// ----------------- ArgChecker -----------------
	ArgChecker::ArgChecker(std::ostream& ost, const std::string& shName, const std::vector<ArgItem>& args):_shName(shName), _ost(ost) {
		AssertTArg(Throw, args.size()<countof(_arg), (GLE_InvalidArgument)(const std::string&)(const char*), _shName, "too many arguments")
		int nA = args.size();
		for(int i=0 ; i<nA ; i++) {
			_target[i] = Detect(args[i].type);
			_arg[i] = &args[i];
		}
		for(int i=nA ; i<N_TARGET ; i++) {
			_target[i] = NONE;
			_arg[i] = nullptr;
		}
	}
	ArgChecker::TARGET ArgChecker::Detect(int type) {
		if(type <= GLType_::TYPE::boolT)
			return BOOLEAN;
		if(type <= GLType_::TYPE::floatT)
			return SCALAR;
		if(type <= GLType_::TYPE::ivec4T)
			return VECTOR;
		return NONE;
	}
	void ArgChecker::_checkAndSet(TARGET tgt) {
		auto t = _target[_cursor];
		auto* arg = _arg[_cursor];
		AssertTArg(Trap, t!=NONE, (GLE_InvalidArgument)(const std::string&)(const char*), _shName, "(none)")
		AssertTArg(Trap, t==tgt, (GLE_InvalidArgument)(const std::string&)(const std::string&), _shName, arg->name)
		_ost << GLType_::cs_typeStr[arg->type] << ' ' << arg->name;
		++_cursor;
	}
	void ArgChecker::operator()(const std::vector<float>& v) {
		int typ = _arg[_cursor]->type;
		_checkAndSet(VECTOR);
		_ost << '=' << GLType_::cs_typeStr[typ] << '(';
		int nV = v.size();
		for(int i=0 ; i<nV-1 ; i++)
			_ost << v[i] << ',';
		_ost << v.back() << ");" << std::endl;
	}
	void ArgChecker::operator()(float v) {
		_checkAndSet(SCALAR);
		_ost << '=' << v << ';' << std::endl;
	}
	void ArgChecker::operator()(bool b) {
		_checkAndSet(BOOLEAN);
		_ost << '=' << b << ';' << std::endl;
	}
	void ArgChecker::finalizeCheck() {
		if(_cursor >=static_cast<int>(countof(_target)))
			return;
		if(_target[_cursor] != NONE)
			throw GLE_InvalidArgument(_shName, "(missing arguments)");
	}

	namespace {
		const std::string cs_rtname[] = {
			"effect"
		};
		BlockSet LoadGLXStructSet(const std::string& name) {
			BlockSet bs;
			std::unordered_set<std::string> loaded,
											inclset{name};
			while(!inclset.empty()) {
				auto itr = inclset.begin();
				// インポートブロックの読み込み
				auto hdl = mgr_block.loadResourceApp(*itr, GLEffect::LoadGLXStruct, [](auto&&){});
				loaded.emplace(*itr);
				inclset.erase(itr);
				for(auto& inc : hdl->incl) {
					if(loaded.count(inc) == 0)
						inclset.emplace(inc);
				}
				bs.emplace(std::move(hdl));
			}
			return bs;
		}
	}
	// ----------------- FxBlock -----------------
	FxBlock::FxBlock(): ResMgrApp(cs_rtname) {}
	// ----------------- BlockSet -----------------
	template <class RET, class GETM>
	auto FindBlock(const BlockSet& bs, GETM getM, const std::string& s) -> spn::Optional<const RET&> {
		for(auto& b : bs) {
			const auto& m = getM(b.cref()).get();
			auto itr = m.find(s);
			if(itr != m.cend())
				return itr->second;
		}
		return spn::none;
	}
	spn::Optional<const AttrStruct&> BlockSet::findAttribute(const std::string& s) const {
		return FindBlock<AttrStruct>(*this, [](auto&& str) { return std::cref(str.atM); }, s);
	}
	spn::Optional<const ConstStruct&> BlockSet::findConst(const std::string& s) const {
		return FindBlock<ConstStruct>(*this, [](auto&& str) { return std::cref(str.csM); }, s);
	}
	spn::Optional<const UnifStruct&> BlockSet::findUniform(const std::string& s) const {
		return FindBlock<UnifStruct>(*this, [](auto&& str) { return std::cref(str.uniM); }, s);
	}
	spn::Optional<const VaryStruct&> BlockSet::findVarying(const std::string& s) const {
		return FindBlock<VaryStruct>(*this, [](auto&& str) { return std::cref(str.varM); }, s);
	}
	spn::Optional<const ShStruct&> BlockSet::findShader(const std::string& s) const {
		return FindBlock<ShStruct>(*this, [](auto&& str) { return std::cref(str.shM); }, s);
	}
	spn::Optional<const TPStruct&> BlockSet::findTechPass(const std::string& s) const {
		for(auto& b : *this) {
			auto& tpL = b->tpL;
			auto itr = std::find_if(tpL.cbegin(), tpL.cend(), [&](auto& tp){ return tp.name == s; });
			if(itr != tpL.cend())
				return *itr;
		}
		return spn::none;
	}
	spn::Optional<const CodeStruct&> BlockSet::findCode(const std::string& s) const {
		return FindBlock<CodeStruct>(*this, [](auto&& str) { return std::cref(str.codeM); }, s);
	}

	// ----------------- GLEffect -----------------
	GLXStruct GLEffect::LoadGLXStruct(const spn::URI& uri) {
		AdaptSDL s(mgr_rw.fromURI(uri, RWops::Read));

		// 一括でメモリに読み込む
		std::string str;
		s.seekg(0, s.end);
		auto len = s.tellg();
		s.seekg(0, s.beg);
		str.resize(len);
		s.read(&str[0], len);

		return ParseGlx(std::move(str));
	}
	GLEffect::GLEffect(const std::string& name) {
		_blockSet = LoadGLXStructSet(name);
		std::vector<TPStruct*> tpV;
		for(auto& blk : _blockSet) {
			auto& b = blk.get().ref();
			for(auto& tp : b.tpL)
				tpV.emplace_back(&tp);
		}
		auto op = _blockSet.findAttribute("CubeAttr");
		try {
			// Tech/Passを順に実行形式へ変換
			// (一緒にTech/Pass名リストを構築)
			int nI = tpV.size();
			_techName.resize(nI);
			for(int techID=0 ; techID<nI ; techID++) {
				auto& nmm = _techName[techID];
				auto& tpTech = *tpV.at(techID);
				// Pass毎に処理
				int nJ = tpTech.tpL.size();
				nmm.resize(nJ+1);
				nmm[0] = tpTech.name;
				for(int passID=0 ; passID<nJ ; passID++) {
					nmm[passID+1] = tpTech.tpL.at(passID).get().name;
					GL16Id tpid{{uint8_t(techID), uint8_t(passID)}};
					auto res = _techMap.insert(std::make_pair(tpid, TPStructR(_blockSet, tpTech, tpTech.tpL.at(passID).get())));
					// テクスチャインデックスリスト作成
					TPStructR& tpr = res.first->second;
					GLuint pid = tpr.getProgram().cref()->getProgramID();
					GLint nUnif;
					GL.glGetProgramiv(pid, GL_ACTIVE_UNIFORMS, &nUnif);

					// Sampler2D変数が見つかった順にテクスチャIdを割り振る
					GLint curI = 0;
					TexIndex& texIndex = _texMap[tpid];
					for(GLint i=0 ; i<nUnif ; i++) {
						GLsizei len;
						int size;
						GLenum typ;
						GLchar cbuff[0x100];	// GLSL変数名の最大がよくわからない (ので、数は適当)

						GLEC_D(Trap, glGetActiveUniform, pid, i, sizeof(cbuff), &len, &size, &typ, cbuff);
						auto opInfo = GLFormat::QueryGLSLInfo(typ);
						if(opInfo->type == GLSLType::TextureT) {
							// GetActiveUniformでのインデックスとGetUniformLocationIDは異なる場合があるので・・
							GLint id = GLEC_D(Trap, glGetUniformLocation, pid, cbuff);
							Assert(Trap, id>=0)
							texIndex.insert(std::make_pair(id, curI++));
						}
					}
				}
			}
		} catch(const std::exception& e) {
			LogOutput("GLEffect exception: %1%", e.what());
			throw;
		}
		GLEC_Chk_D(Trap)

		_setConstantUniformList(&GlxId::GetUnifList());
		_setConstantTechPassList(&GlxId::GetTechList());
	}

	// -------------- GLEffect::Current::Vertex --------------
	GLEffect::Current::Vertex::Vertex() {}
	void GLEffect::Current::Vertex::reset() {
		_spVDecl.reset();
		for(auto& v : _vbuff)
			v.release();
	}
	void GLEffect::Current::Vertex::setVDecl(const SPVDecl& v) {
		_spVDecl = v;
	}
	void GLEffect::Current::Vertex::setVBuffer(HVb hVb, int n) {
		_vbuff[n] = hVb;
	}
	void GLEffect::Current::Vertex::extractData(draw::VStream& dst,
												TPStructR::VAttrID vAttrId) const
	{
		Assert(Trap, _spVDecl, "VDecl is not set")
		dst.spVDecl = _spVDecl;
		for(int i=0 ; i<static_cast<int>(countof(_vbuff)) ; i++) {
			if(_vbuff[i])
				dst.vbuff[i] = _vbuff[i]->get()->getDrawToken();
		}
		dst.vAttrId = vAttrId;
	}
	bool GLEffect::Current::Vertex::operator != (const Vertex& v) const {
		if(_spVDecl != v._spVDecl)
			return true;
		for(int i=0 ; i<static_cast<int>(countof(_vbuff)) ; i++) {
			if(_vbuff[i] != v._vbuff[i])
				return true;
		}
		return false;
	}

	// -------------- GLEffect::Current::Index --------------
	GLEffect::Current::Index::Index() {}
	void GLEffect::Current::Index::reset() {
		_ibuff.release();
	}
	void GLEffect::Current::Index::setIBuffer(HIb hIb) {
		_ibuff = hIb;
	}
	HIb GLEffect::Current::Index::getIBuffer() const {
		return _ibuff;
	}
	void GLEffect::Current::Index::extractData(draw::VStream& dst) const {
		if(_ibuff)
			dst.ibuff = spn::construct(_ibuff->get()->getDrawToken());
	}
	bool GLEffect::Current::Index::operator != (const Index& idx) const {
		return _ibuff != idx._ibuff;
	}

	// -------------- GLEffect::Current --------------
	UnifPool GLEffect::Current::s_unifPool(DefaultUnifPoolSize);
	diff::Buffer GLEffect::Current::getDifference() {
		diff::Buffer diff = {};
		if(vertex != vertex_prev)
			++diff.vertex;
		if(index != index_prev)
			++diff.index;

		vertex_prev = vertex;
		index_prev = index;
		return diff;
	}
	void GLEffect::Current::reset() {
		vertex.reset();
		vertex_prev.reset();
		index.reset();
		index_prev.reset();

		bDefaultParam = false;
		tech = spn::none;
		_clean_drawvalue();
		hlFb = HFb();
		viewport = spn::none;
	}
	void GLEffect::Current::_clean_drawvalue() {
		pass = spn::none;
		tps = spn::none;
		pTexIndex = nullptr;
		// セットされているUniform変数を未セット状態にする
		for(auto& u : uniMap)
			s_unifPool.destroy(u.second);
		uniMap.clear();
	}
	void GLEffect::Current::setTech(GLint idTech, bool bDefault) {
		if(!tech || *tech != idTech) {
			// TechIDをセットしたらPassIDは無効になる
			tech = idTech;
			bDefaultParam = bDefault;
			_clean_drawvalue();
		}
	}
	void GLEffect::Current::setPass(GLint idPass, TechMap& tmap, TexMap& texMap) {
		// TechIdをセットせずにPassIdをセットするのは禁止
		AssertT(Trap, tech, GLE_Error, "tech is not selected")
		if(!pass || *pass != idPass) {
			_clean_drawvalue();
			pass = idPass;

			// TPStructRの参照をTech,Pass Idから検索してセット
			GL16Id id{{uint8_t(*tech), uint8_t(*pass)}};
			Assert(Trap, tmap.count(id)==1)
			tps = tmap.at(id);
			pTexIndex = &texMap.at(id);

			// デフォルト値読み込み
			if(bDefaultParam) {
				auto& def = tps->getUniformDefault();
				for(auto& d : def) {
					auto* buff = MakeUniformTokenBuffer(uniMap, s_unifPool, d.first);
					d.second->clone(*buff);
				}
			}
			// 各種セッティングをするTokenをリストに追加
			tps->getProgram()->get()->getDrawToken(tokenML);
			tokenML.allocate<draw::UserFunc>([&tp_tmp = *tps](){
				tp_tmp.applySetting();
			});
		}
	}
	void GLEffect::Current::outputFramebuffer() {
		if(hlFb) {
			auto& fb = *hlFb;
			if(fb)
				fb->get()->getDrawToken(tokenML);
			else
				GLFBufferTmp(0, mgr_info.getScreenSize()).getDrawToken(tokenML);
			hlFb = spn::none;

			// ビューポートはデフォルトでフルスクリーンに初期化
			if(!viewport)
				viewport = spn::construct(false, spn::RectF{0,1,0,1});
		}
		if(viewport) {
			using T = draw::Viewport;
			new(tokenML.allocate_memory(sizeof(T), draw::CalcTokenOffset<T>())) T(*viewport);
			viewport = spn::none;
		}
	}
	void GLEffect::Current::_outputDrawCall(draw::VStream& vs) {
		outputFramebuffer();
		// set uniform value
		if(!uniMap.empty()) {
			// 中身shared_ptrなのでコピーする
			for(auto& u : uniMap) {
				u.second->takeout(tokenML);
				s_unifPool.destroy(u.second);
			}
			uniMap.clear();
		}
		// set VBuffer(VDecl)
		vertex.extractData(vs, tps->getVAttrID());
		// set IBuffer
		index.extractData(vs);
	}
	void GLEffect::Current::outputDrawCallIndexed(GLenum mode, GLsizei count, GLenum sizeF, GLuint offset) {
		draw::VStream vs;
		_outputDrawCall(vs);

		tokenML.allocate<draw::DrawIndexed>(std::move(vs), mode, count, sizeF, offset);
	}
	void GLEffect::Current::outputDrawCall(GLenum mode, GLint first, GLsizei count) {
		draw::VStream vs;
		_outputDrawCall(vs);

		tokenML.allocate<draw::Draw>(std::move(vs), mode, first, count);
	}
	void GLEffect::onDeviceLost() {
		if(_bInit) {
			_bInit = false;
			for(auto& p : _techMap)
				p.second.ts_onDeviceLost();

			_current.reset();
		}
	}
	void GLEffect::onDeviceReset() {
		if(!_bInit) {
			_bInit = true;
			for(auto& p : _techMap)
				p.second.ts_onDeviceReset(*this);
		}
	}
	void GLEffect::setVDecl(const SPVDecl& decl) {
		_current.vertex.setVDecl(decl);
	}
	void GLEffect::setVStream(HVb vb, int n) {
		_current.vertex.setVBuffer(vb, n);
	}
	void GLEffect::setIStream(HIb ib) {
		_current.index.setIBuffer(ib);
	}
	void GLEffect::setTechnique(int techId, bool bDefault) {
		_current.setTech(techId, bDefault);
		_unifId.resultCur = nullptr;
	}
	void GLEffect::setPass(int passId) {
		_current.setPass(passId, _techMap, _texMap);
		if(_unifId.src)
			_unifId.resultCur = &_unifId.result.at(GL16Id{{uint8_t(*_current.tech), uint8_t(passId)}});
	}
	OPGLint GLEffect::_getPassId(int techId, const std::string& pass) const {
		auto& tech = _techName[techId];
		int nP = tech.size();
		for(int i=1 ; i<nP ; i++) {
			if(tech[i] == pass)
				return i-1;
		}
		return spn::none;
	}
	OPGLint GLEffect::getPassId(const std::string& tech, const std::string& pass) const {
		if(auto idTech = getTechId(tech))
			return _getPassId(*idTech, pass);
		return spn::none;
	}
	OPGLint GLEffect::getTechId(const std::string& tech) const {
		int nT = _techName.size();
		for(int i=0 ; i<nT ; i++) {
			if(_techName[i][0] == tech)
				return i;
		}
		return spn::none;
	}
	OPGLint GLEffect::getPassId(const std::string& pass) const {
		AssertT(Trap, _current.tech, GLE_Error, "tech is not selected")
		return _getPassId(*_current.tech, pass);
	}
	OPGLint GLEffect::getCurPassId() const {
		return _current.pass;
	}
	OPGLint GLEffect::getCurTechId() const {
		return _current.tech;
	}
	HLProg GLEffect::getProgram(int techId, int passId) const {
		if(techId < 0) {
			if(!_current.tech)
				return HLProg();
			techId = *_current.tech;
		}
		if(passId < 0) {
			if(!_current.pass)
				return HLProg();
			passId = *_current.pass;
		}
		auto itr = _techMap.find(GL16Id{{uint8_t(techId), uint8_t(passId)}});
		if(itr != _techMap.end())
			return itr->second.getProgram();
		return HLProg();
	}
	draw::TokenBuffer& GLEffect::_makeUniformTokenBuffer(GLint id) {
		return *MakeUniformTokenBuffer(_current.uniMap, _current.s_unifPool, id);
	}

	namespace draw {
		// -------------- VStream --------------
		RUser<VStream> VStream::use() {
			return RUser<VStream>(*this);
		}
		void VStream::use_begin() const {
			if(spVDecl)
				spVDecl->apply(VData{vbuff, *vAttrId});
			if(ibuff)
				ibuff->use_begin();
		}
		void VStream::use_end() const {
			if(spVDecl) {
				GL.glBindBuffer(GL_ARRAY_BUFFER, 0);
				GL.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			}
			if(ibuff)
				ibuff->use_end();
		}

		// -------------- UserFunc --------------
		UserFunc::UserFunc(const Func& f):
			_func(f)
		{}
		void UserFunc::exec() {
			_func();
		}
		// -------------- Tag_Draw --------------
		Draw::Draw(VStream&& vs, GLenum mode, GLint first, GLsizei count):
			DrawBase(std::move(vs)),
			_mode(mode),
			_first(first),
			_count(count)
		{}
		void Draw::exec() {
			auto u = use();
			GL.glDrawArrays(_mode, _first, _count);
			GLEC_Chk_D(Trap);
		}
		// -------------- Tag_DrawI --------------
		DrawIndexed::DrawIndexed(VStream&& vs, GLenum mode, GLsizei count, GLenum sizeF, GLuint offset):
			DrawBase(std::move(vs)),
			_mode(mode),
			_count(count),
			_sizeF(sizeF),
			_offset(offset)
		{}
		void DrawIndexed::exec() {
			auto u = use();
			GL.glDrawElements(_mode, _count, _sizeF, reinterpret_cast<const GLvoid*>(_offset));
			GLEC_Chk_D(Trap);
		}

		// -------------- Uniforms --------------
		namespace {
			using IGLF_V = void (*)(GLint, const void*, int);
			const IGLF_V c_iglfV[] = {
				[](GLint id, const void* ptr, int n) {
					GL.glUniform1fv(id, n, reinterpret_cast<const GLfloat*>(ptr)); },
				[](GLint id, const void* ptr, int n) {
					GL.glUniform2fv(id, n, reinterpret_cast<const GLfloat*>(ptr)); },
				[](GLint id, const void* ptr, int n) {
					GL.glUniform3fv(id, n, reinterpret_cast<const GLfloat*>(ptr)); },
				[](GLint id, const void* ptr, int n) {
					GL.glUniform4fv(id, n, reinterpret_cast<const GLfloat*>(ptr)); },
				[](GLint id, const void* ptr, int n) {
					GL.glUniform1iv(id, n, reinterpret_cast<const GLint*>(ptr)); },
				[](GLint id, const void* ptr, int n) {
					GL.glUniform2iv(id, n, reinterpret_cast<const GLint*>(ptr)); },
				[](GLint id, const void* ptr, int n) {
					GL.glUniform3iv(id, n, reinterpret_cast<const GLint*>(ptr)); },
				[](GLint id, const void* ptr, int n) {
					GL.glUniform4iv(id, n, reinterpret_cast<const GLint*>(ptr)); }
			};
			using IGLF_M = void(*)(GLint, const void*, int, GLboolean);
			const IGLF_M c_iglfM[] = {
				[](GLint id, const void* ptr, int n, GLboolean bT) {
					GL.glUniformMatrix2fv(id, n, bT, reinterpret_cast<const GLfloat*>(ptr)); },
				[](GLint id, const void* ptr, int n, GLboolean bT) {
					GL.glUniformMatrix3fv(id, n, bT, reinterpret_cast<const GLfloat*>(ptr)); },
				[](GLint id, const void* ptr, int n, GLboolean bT) {
					GL.glUniformMatrix4fv(id, n, bT, reinterpret_cast<const GLfloat*>(ptr)); }
			};
		}
		void Unif_Vec_Exec(int idx, GLint id, const void* ptr, int n) {
			c_iglfV[idx](id, ptr, n);
		}
		void Unif_Mat_Exec(int idx, GLint id, const void* ptr, int n, bool bT) {
			c_iglfM[idx](id, ptr, n, bT ? GL_TRUE : GL_FALSE);
		}
	}
	void GLEffect::clearFramebuffer(const draw::ClearParam& param) {
		_current.outputFramebuffer();
		_current.tokenML.allocate<draw::Clear>(param);
	}
	void GLEffect::draw(GLenum mode, GLint first, GLsizei count) {
		_prepareUniforms();
		_current.outputDrawCall(mode, first, count);
		mgr_drawtask.refWriteEnt().append(std::move(_current.tokenML));

		_diffCount.buffer += _current.getDifference();
		++_diffCount.drawNoIndexed;
	}
	void GLEffect::drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem) {
		_prepareUniforms();
		HIb hIb = _current.index.getIBuffer();
		auto str = hIb->get()->getStride();
		auto szF = GLIBuffer::GetSizeFlag(str);
		_current.outputDrawCallIndexed(mode, count, szF, offsetElem*str);
		mgr_drawtask.refWriteEnt().append(std::move(_current.tokenML));

		_diffCount.buffer += _current.getDifference();
		++_diffCount.drawIndexed;
	}
	// Uniform設定は一旦_unifMapに蓄積した後、出力
	void GLEffect::_makeUniformToken(draw::TokenDst& dst, GLint id, const bool* b, int n, bool bT) const {
		int tmp[n];
		for(int i=0 ; i<n ; i++)
			tmp[i] = static_cast<int>(b[i]);
		_makeUniformToken(dst, id, static_cast<const int*>(tmp), 1, bT);
	}
	void GLEffect::_makeUniformToken(draw::TokenDst& dst, GLint id, const int* iv, int n, bool /*bT*/) const {
		MakeUniformToken<draw::Unif_Vec<int, 1>>(dst, id, id, iv, 1, n);
	}
	void GLEffect::_makeUniformToken(draw::TokenDst& dst, GLint id, const float* fv, int n, bool /*bT*/) const {
		MakeUniformToken<draw::Unif_Vec<float, 1>>(dst, id, id, fv, 1, n);
	}
	void GLEffect::_makeUniformToken(draw::TokenDst& dst, GLint id, const double* dv, int n, bool bT) const {
		float tmp[n];
		for(int i=0 ; i<n ; i++)
			tmp[i] = static_cast<float>(dv[i]);
		_makeUniformToken(dst, id, static_cast<const float*>(tmp), n, bT);
	}
	void GLEffect::_makeUniformToken(draw::TokenDst& dst, GLint id, const HTex* hTex, int n, bool /*bT*/) const {
		// テクスチャユニット番号を検索
		auto itr = _current.pTexIndex->find(id);
		Assert(Warn, itr != _current.pTexIndex->end(), "texture index not found")
		if(itr != _current.pTexIndex->end()) {
			auto aID = itr->second;
			if(n > 1) {
				std::vector<const IGLTexture*> pTexA(n);
				for(int i=0 ; i<n ; i++)
					pTexA[i] = (hTex[i].cref()).get();
				MakeUniformToken<draw::TextureA>(dst, id,
								id, reinterpret_cast<const HRes*>(hTex),
								pTexA.data(), aID, n);
				return;
			}
			AssertP(Trap, n==1)
			HTex hTex2(*hTex);
			hTex2.ref()->getDrawToken(dst, id, 0, aID);
		}
	}
	void GLEffect::_makeUniformToken(draw::TokenDst& dst, GLint id, const HLTex* hlTex, int n, bool bT) const {
		if(n > 1) {
			std::vector<HLTex> hTexA(n);
			for(int i=0 ; i<n ; i++)
				hTexA[i] = hlTex[i].get();
			_makeUniformToken(dst, id, hTexA.data(), n, bT);
			return;
		}
		AssertP(Trap, n==1)
		HTex hTex = hlTex->get();
		_makeUniformToken(dst, id, &hTex, 1, bT);
	}
	OPGLint GLEffect::getUniformID(const std::string& name) const {
		AssertP(Trap, _current.tps, "Tech/Pass is not set")
		auto& tps = *_current.tps;
		HProg hProg = tps.getProgram();
		AssertP(Trap, hProg.valid(), "shader program handle is invalid")
		return hProg->get()->getUniformID(name);
	}
	void GLEffect::beginTask() {
		mgr_drawtask.beginTask(HFx::FromHandle(handleFromThis()));
		_current.reset();
		spn::TupleZeroFill(_diffCount);
	}
	void GLEffect::endTask() {
		mgr_drawtask.endTask();
	}
	void GLEffect::clearTask() {
		mgr_drawtask.clear();
	}
	void GLEffect::execTask() {
		mgr_drawtask.execTask();
	}
	void GLEffect::_setConstantUniformList(const StrV* src) {
		_unifId.src = src;
		auto& r = _unifId.result;
		r.clear();

		_unifId.resultCur = nullptr;
		// 全てのTech&Passの組み合わせについてそれぞれUniform名を検索し、番号(GLint)を登録
		int nTech = _techName.size();
		for(int i=0 ; i<nTech ; i++) {
			GL16Id tpId{{uint8_t(i),0}};
			for(;;) {
				auto itr = _techMap.find(tpId);
				if(itr == _techMap.end())
					break;
				auto& r2 = r[tpId];
				auto& tps = itr->second;
				const GLProgram* p = tps.getProgram()->get();

				for(auto& srcstr : *src) {
					if(auto id = p->getUniformID(srcstr)) {
						r2.push_back(*id);
					} else
						r2.push_back(-1);
				}

				++tpId[1];
			}
		}
	}
	void GLEffect::_setConstantTechPassList(const StrPairV* src) {
		_techId.src = src;
		_techId.result.clear();

		// TechとPass名のペアについてそれぞれTechId, PassIdを格納
		auto& r = _techId.result;
		int n = src->size();
		r.resize(n);
		for(int i=0 ; i<n ; i++) {
			auto& ip = r[i];
			ip.first = *getTechId((*src)[i].first);
			ip.second = *_getPassId(ip.first, (*src)[i].second);
		}
	}
	OPGLint GLEffect::getUnifId(IdValue id) const {
		// 定数値に対応するUniform変数が見つからない時は警告を出す
		if(static_cast<int>(_unifId.resultCur->size()) <= id.value)
			return spn::none;
		auto ret = (*_unifId.resultCur)[id.value];
		if(ret < 0)
			return spn::none;
		return ret;
	}
	GLEffect::IdPair GLEffect::_getTechPassId(IdValue id) const {
		Assert(Trap, (static_cast<int>(_techId.result.size()) > id.value), "TechPass-ConstantId: Invalid Id (%1%)", id.value)
		return _techId.result[id.value];
	}
	void GLEffect::setTechPassId(IdValue id) {
		auto ip = _getTechPassId(id);
		setTechnique(ip.first, true);
		setPass(ip.second);
	}
	void GLEffect::setFramebuffer(HFb fb) {
		_current.hlFb = fb;
		_current.viewport = spn::none;
	}
	void GLEffect::resetFramebuffer() {
		setFramebuffer(HLFb());
	}
	void GLEffect::setViewport(bool bPixel, const spn::RectF& r) {
		_current.viewport = spn::construct(bPixel, r);
	}
	diff::Effect GLEffect::getDifference() const {
		return _diffCount;
	}
	void GLEffect::_prepareUniforms() {}

	// ------------- ShStruct -------------
	const std::string& ShStruct::getShaderString() const {
		if(info_str.empty()) {
			info_str = '{' + info + '}';
		}
		return info_str;
	}
}
