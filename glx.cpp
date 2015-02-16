#include "glx.hpp"
#include "spinner/common.hpp"
#include "spinner/matrix.hpp"
#include <boost/format.hpp>
#include <fstream>

namespace rs {
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
		for(int i=0 ; i<nV ; i++)
			value[i] = s.value[i];
		for(int i=nV ; i<countof(value) ; i++)
			value[i] = boost::blank();
	}
	void ValueSettingR::action() const { func(*this); }
	bool ValueSettingR::operator == (const ValueSettingR& s) const {
		for(int i=0 ; i<countof(value) ; i++)
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
		AssertT(Trap, t!=NONE, (GLE_InvalidArgument)(const std::string&)(const char*), _shName, "(none)")
		AssertT(Trap, t==tgt, (GLE_InvalidArgument)(const std::string&)(const std::string&), _shName, arg->name)
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
		_ost << v << ';' << std::endl;
	}
	void ArgChecker::operator()(bool b) {
		_checkAndSet(BOOLEAN);
		_ost << b << ';' << std::endl;
	}
	void ArgChecker::finalizeCheck() {
		if(_cursor >=countof(_target))
			return;
		if(_target[_cursor] != NONE)
			throw GLE_InvalidArgument(_shName, "(missing arguments)");
	}

	namespace {
		boost::regex re_comment(R"(//[^\n$]+)"),		//!< 一行コメント
					re_comment2(R"(/\*[^\*]*\*/)");		//!< 範囲コメント
	}
	// ----------------- GLEffect -----------------
	GLEffect::GLEffect(spn::AdaptStream& s) {
		// 一括でメモリに読み込む
		std::string str;
		s.seekg(0, s.end);
		auto len = s.tellg();
		s.seekg(0, s.beg);
		str.resize(len);
		s.read(&str[0], len);

		// コメント部分を除去 -> スペースに置き換える
		str = boost::regex_replace(str, re_comment, " ");
		str = boost::regex_replace(str, re_comment2, " ");

		GR_Glx glx;
		auto itr = str.cbegin();
		bool bS = boost::spirit::qi::phrase_parse(itr, str.cend(), glx, standard::space, _result);
	#ifdef DEBUG
		LogOutput((bS) ? "------- analysis succeeded! -------"
					: "------- analysis failed! -------");
		if(itr != str.cend()) {
			LogOutput("<but not reached to end>\nremains: %1%", std::string(itr, str.cend()));
		} else {
			// 解析結果の表示
			std::stringstream ss;
			ss << _result;
			LogOutput(ss.str());
		}
	#endif
		if(!bS || itr!=str.cend()) {
			std::stringstream ss;
			ss << "GLEffect parse error:";
			if(itr != str.cend())
				ss << "remains:\n" << std::string(itr, str.cend());
			throw EC_GLXGrammar(ss.str());
		}

		try {
			// Tech/Passを順に実行形式へ変換
			// (一緒にTech/Pass名リストを構築)
			int nI = _result.tpL.size();
			_techName.resize(nI);
			for(int techID=0 ; techID<nI ; techID++) {
				auto& nmm = _techName[techID];
				auto& tpTech = _result.tpL.at(techID);
				// Pass毎に処理
				int nJ = _result.tpL[techID].tpL.size();
				nmm.resize(nJ+1);
				nmm[0] = tpTech.name;
				for(int passID=0 ; passID<nJ ; passID++) {
					nmm[passID+1] = tpTech.tpL.at(passID).get().name;
					_techMap.insert(std::make_pair(GL16Id{{uint8_t(techID), uint8_t(passID)}}, TPStructR(_result, techID, passID)));
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
	GLEffect::Current::Vertex::Vertex(): _bChanged(true) {}
	void GLEffect::Current::Vertex::reset() {
		_spVDecl.reset();
		for(auto& v : _vbuff)
			v.setNull();
		_bChanged = true;
	}
	void GLEffect::Current::Vertex::setVDecl(const SPVDecl& v) {
		if(_spVDecl != v) {
			_bChanged = true;
			_spVDecl = v;
		}
	}
	void GLEffect::Current::Vertex::setVBuffer(HVb hVb, int n) {
		if(_vbuff[n].get() != hVb) {
			_bChanged = true;
			_vbuff[n] = hVb;
		}
	}
	void GLEffect::Current::Vertex::extractData(draw::VStream& dst,
												TPStructR::VAttrID vAttrId) const
	{
		if(_bChanged) {
			_bChanged = false;
			Assert(Trap, _spVDecl, "VDecl is not set")
			dst.spVDecl = _spVDecl;
			for(int i=0 ; i<countof(_vbuff) ; i++) {
				if(_vbuff[i])
					dst.vbuff[i] = _vbuff[i]->get()->getDrawToken();
			}
			dst.vAttrId = vAttrId;
		}
	}

	// -------------- GLEffect::Current::Index --------------
	GLEffect::Current::Index::Index(): _bChanged(true) {}
	void GLEffect::Current::Index::reset() {
		_ibuff.setNull();
		_bChanged = true;
	}
	void GLEffect::Current::Index::setIBuffer(HIb hIb) {
		if(_ibuff.get() != hIb) {
			_bChanged = true;
			_ibuff = hIb;
		}
	}
	HIb GLEffect::Current::Index::getIBuffer() const {
		return _ibuff;
	}
	void GLEffect::Current::Index::extractData(draw::VStream& dst) const {
		if(_bChanged) {
			_bChanged = false;
			if(_ibuff)
				dst.ibuff = spn::construct(_ibuff->get()->getDrawToken());
		}
	}

	// -------------- GLEffect::Current --------------
	void GLEffect::Current::reset() {
		vertex.reset();
		index.reset();
		bDefaultParam = false;
		tech = spn::none;
		_clean_drawvalue();
	}
	void GLEffect::Current::_clean_drawvalue() {
		pass = spn::none;
		tps = spn::none;
		tokenV.clear();
		// セットされているUniform変数を未セット状態にする
		uniMap.clear();
		texIndex.clear();
	}
	void GLEffect::Current::setTech(GLint idTech, bool bDefault) {
		if(!tech || *tech != idTech) {
			// TechIDをセットしたらPassIDは無効になる
			tech = idTech;
			bDefaultParam = bDefault;
			_clean_drawvalue();
		}
	}
	void GLEffect::Current::setPass(GLint idPass, TechMap& tmap) {
		// TechIdをセットせずにPassIdをセットするのは禁止
		AssertT(Trap, tech, (GLE_Error)(const char*), "tech is not selected")
//		if(!cur.pass || *cur.pass != passId) {
			_clean_drawvalue();
			pass = idPass;

			// TPStructRの参照をTech,Pass Idから検索してセット
			GL16Id id{{uint8_t(*tech), uint8_t(*pass)}};
			Assert(Trap, tmap.count(id)==1)
			tps = tmap.at(id);

			// デフォルト値読み込み
			if(bDefaultParam) {
				auto& def = tps->getUniformDefault();
				uniMap = def;
			}
			// テクスチャインデックスリスト作成
			GLuint pid = tps->getProgram().cref()->getProgramID();
			GLint nUnif;
			GL.glGetProgramiv(pid, GL_ACTIVE_UNIFORMS, &nUnif);

			GLsizei len;
			int size;
			GLenum typ;
			GLchar cbuff[0x100];	// GLSL変数名の最大がよくわからない (ので、数は適当)
			// Sampler2D変数が見つかった順にテクスチャIdを割り振る
			GLint curI = 0;
			texIndex.clear();
			for(GLint i=0 ; i<nUnif ; i++) {
				GLEC_D(Trap, glGetActiveUniform, pid, i, sizeof(cbuff), &len, &size, &typ, cbuff);
				auto opInfo = GLFormat::QueryGLSLInfo(typ);
				if(opInfo->type == GLSLType::TextureT) {
					// GetActiveUniformでのインデックスとGetUniformLocationIDは異なる場合があるので・・
					GLint id = GL.glGetUniformLocation(pid, cbuff);
					Assert(Trap, id>=0)
					texIndex.insert(std::make_pair(id, curI++));
				}
			}

			tokenV.emplace_back(tps->getProgram()->get()->getDrawToken());
			tokenV.emplace_back(new draw::UserFunc([&tp_tmp = *tps](){
				tp_tmp.applySetting();
			}));
//		}
	}
	void GLEffect::Current::_outputDrawCall(draw::VStream& vs) {
		// set uniform value
		if(!uniMap.empty()) {
			// 中身shared_ptrなのでコピーする
			for(auto& sp : uniMap)
				tokenV.emplace_back(sp.second);
			uniMap.clear();
		}
		// set VBuffer(VDecl)
		vertex.extractData(vs, tps->getVAttrID());
		// set IBuffer
		index.extractData(vs);
	}
	draw::UPTag GLEffect::Current::outputDrawCallIndexed(GLenum mode, GLsizei count, GLenum sizeF, GLuint offset) {
		draw::VStream vs;
		_outputDrawCall(vs);
		return std::make_unique<draw::Tag_DrawI>(std::move(tokenV), std::move(vs),
										mode, count, sizeF, offset);
	}
	draw::UPTag GLEffect::Current::outputDrawCall(GLenum mode, GLint first, GLsizei count) {
		draw::VStream vs;
		_outputDrawCall(vs);
		return std::make_unique<draw::Tag_Draw>(std::move(tokenV), std::move(vs),
										mode, first, count);
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
		_current.setPass(passId, _techMap);
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
		AssertT(Trap, _current.tech, (GLE_Error)(const char*), "tech is not selected")
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

		// -------------- Task --------------
		Task::Task(): _curWrite(0), _curRead(0) {}
		Task::UPTagV& Task::refWriteEnt() {
			UniLock lk(_mutex);
			return _entry[_curWrite % NUM_ENTRY];
		}
		Task::UPTagV& Task::refReadEnt() {
			UniLock lk(_mutex);
			return _entry[_curRead % NUM_ENTRY];
		}
		void Task::pushTag(UPTag tag) {
			Assert(Trap, tag, "null pointer detected")
			// DThとアクセスするエントリが違うからemplace_back中の同期をとらなくて良い
			refWriteEnt().emplace_back(std::move(tag));
		}
		void Task::beginTask() {
			UniLock lk(_mutex);
			// 読み込みカーソルが追いついてない時は追いつくまで待つ
			auto diff = _curWrite - _curRead;
			Assert(Trap, diff >= 0)
			while(diff >= NUM_ENTRY) {
				_cond.wait(lk);
				diff = _curWrite - _curRead;
				Assert(Trap, diff >= 0)
			}
			auto& we = refWriteEnt();
			lk.unlock();
			we.clear();
		}
		void Task::endTask() {
			GL.glFinish();
			UniLock lk(_mutex);
			++_curWrite;
		}
		void Task::clear() {
			UniLock lk(_mutex);
			_curWrite = _curRead+1;
			GL.glFinish();
			for(auto& e : _entry)
				e.clear();
			_curWrite = _curRead = 0;
		}
		void Task::execTask() {
			spn::Optional<UniLock> lk(_mutex);
			auto diff = _curWrite - _curRead;
			Assert(Trap, diff >= 0)
			if(diff > 0) {
				auto& readent = refReadEnt();
				lk = spn::none;
				// MThとアクセスするエントリが違うから同期をとらなくて良い
				for(auto& ent : readent)
					ent->exec();
				GL.glFlush();
				lk = spn::construct(std::ref(_mutex));
				++_curRead;
				_cond.signal();
			}
		}

		// -------------- UserFunc --------------
		UserFunc::UserFunc(const Func& f):
			Token(HRes()),
			_func(f)
		{}
		void UserFunc::exec() {
			_func();
		}

		// -------------- Tag_DrawBase --------------
		Tag_DrawBase::Tag_DrawBase(TokenV&& t, VStream&& vs):
			_token(std::move(t)),
			_vstream(std::move(vs))
		{}
		RUser<VStream> Tag_DrawBase::use() {
			for(auto& t : _token)
				t->exec();
			return _vstream.use();
		}
		// -------------- Tag_Draw --------------
		Tag_Draw::Tag_Draw(TokenV&& t, VStream&& vs, GLenum mode, GLint first, GLsizei count):
			Tag_DrawBase(std::move(t), std::move(vs)),
			_mode(mode),
			_first(first),
			_count(count)
		{}
		void Tag_Draw::exec() {
			auto u = use();
			GL.glDrawArrays(_mode, _first, _count);
			GLEC_Chk_D(Trap);
		}
		// -------------- Tag_DrawI --------------
		Tag_DrawI::Tag_DrawI(TokenV&& t, VStream&& vs, GLenum mode, GLsizei count, GLenum sizeF, GLuint offset):
			Tag_DrawBase(std::move(t), std::move(vs)),
			_mode(mode),
			_count(count),
			_sizeF(sizeF),
			_offset(offset)
		{}
		void Tag_DrawI::exec() {
			auto u = use();
			GL.glDrawElements(_mode, _count, _sizeF, reinterpret_cast<const GLvoid*>(_offset));
			GLEC_Chk_D(Trap);
		}

		// -------------- Uniforms --------------
		Uniform::Uniform(HRes hRes, GLint id): Token(hRes), idUnif(id) {}

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
	namespace {
		struct UniVisitor : boost::static_visitor<> {
			GLint						_id;
			const GLEffect::TexIndex&	_tIdx;

			UniVisitor(const GLEffect::TexIndex& ti): _tIdx(ti) {}
			void operator()(const HLTex& tex) const {
				auto itr = _tIdx.find(_id);
				if(itr != _tIdx.end()) {
					auto& cr = tex.cref();
					cr->setActiveID(itr->second);
					auto u = cr->use();
					GL.glUniform1i(_id, itr->second);
				} else
					Assert(Warn, false, "uniform id=%1% is not sampler", _id)
			}
		};
	}
	void GLEffect::draw(GLenum mode, GLint first, GLsizei count) {
		auto t = _current.outputDrawCall(mode, first, count);
		_task.pushTag(std::move(t));
	}
	void GLEffect::drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem) {
		HIb hIb = _current.index.getIBuffer();
		auto str = hIb->get()->getStride();
		auto szF = GLIBuffer::GetSizeFlag(str);
		auto t = _current.outputDrawCallIndexed(mode, count, szF, offsetElem*str);
		_task.pushTag(std::move(t));
	}
	// Uniform設定は一旦_unifMapに蓄積した後、出力
	void GLEffect::_makeUniformToken(UniMap& dstToken, GLint id, const bool* b, int n, bool bT) const {
		int tmp[n];
		for(int i=0 ; i<n ; i++)
			tmp[i] = static_cast<int>(b[i]);
		_makeUniformToken(dstToken, id, static_cast<const int*>(tmp), 1, bT);
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, GLint id, const int* iv, int n, bool /*bT*/) const {
		dstToken.emplace(id, std::make_shared<draw::Unif_Vec<int, 1>>(id, iv, 1, n));
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, GLint id, const float* fv, int n, bool /*bT*/) const {
		dstToken.emplace(id, std::make_shared<draw::Unif_Vec<float, 1>>(id, fv, 1, n));
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, GLint id, const double* dv, int n, bool bT) const {
		float tmp[n];
		for(int i=0 ; i<n ; i++)
			tmp[i] = static_cast<float>(dv[i]);
		_makeUniformToken(dstToken, id, static_cast<const float*>(tmp), n, bT);
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, GLint id, const HTex* hTex, int n, bool /*bT*/) const {
		// テクスチャユニット番号を検索
		auto itr = _current.texIndex.find(id);
		Assert(Warn, itr != _current.texIndex.end(), "texture index not found")
		if(itr != _current.texIndex.end()) {
			auto aID = itr->second;
			if(n > 1) {
				std::vector<const IGLTexture*> pTexA(n);
				for(int i=0 ; i<n ; i++)
					pTexA[i] = (hTex[i].cref()).get();
				dstToken.emplace(id, std::make_shared<draw::TextureA>(id,
											reinterpret_cast<const HRes*>(hTex),
											pTexA.data(), aID, n));
				return;
			}
			AssertP(Trap, n==1)
			HTex hTex2(*hTex);
			dstToken.emplace(id, hTex2.ref()->getDrawToken(id, 0, aID));
		}
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, GLint id, const HLTex* hlTex, int n, bool bT) const {
		if(n > 1) {
			std::vector<HLTex> hTexA(n);
			for(int i=0 ; i<n ; i++)
				hTexA[i] = hlTex[i].get();
			_makeUniformToken(dstToken, id, hTexA.data(), n, bT);
			return;
		}
		AssertP(Trap, n==1)
		HTex hTex = hlTex->get();
		_makeUniformToken(dstToken, id, &hTex, 1, bT);
	}
	OPGLint GLEffect::getUniformID(const std::string& name) const {
		AssertP(Trap, _current.tps, "Tech/Pass is not set")
		auto& tps = *_current.tps;
		HProg hProg = tps.getProgram();
		AssertP(Trap, hProg.valid(), "shader program handle is invalid")
		return hProg->get()->getUniformID(name);
	}
	void GLEffect::beginTask() {
		_task.beginTask();
		_current.reset();
	}
	void GLEffect::endTask() {
		_task.endTask();
	}
	void GLEffect::clearTask() {
		_task.clear();
	}
	void GLEffect::execTask() {
		_task.execTask();
	}
	void GLEffect::_setConstantUniformList(const StrV* src) {
		_unifId.src = src;
		auto& r = _unifId.result;
		r.clear();

		_unifId.resultCur = nullptr;
		// 全てのTech&Passの組み合わせについてそれぞれUniform名を検索し、番号(GLint)を登録
		int nTech = _result.tpL.size();
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
		if(_unifId.resultCur->size() <= id.value)
			return spn::none;
		return (*_unifId.resultCur)[id.value];
	}
	GLEffect::IdPair GLEffect::_getTechPassId(IdValue id) const {
		Assert(Trap, (_techId.result.size() > id.value), "TechPass-ConstantId: Invalid Id (%1%)", id.value)
		return _techId.result[id.value];
	}
	void GLEffect::setTechPassId(IdValue id) {
		auto ip = _getTechPassId(id);
		setTechnique(ip.first, true);
		setPass(ip.second);
	}
	GLEffect::GlxId GLEffect::s_myId;

	// ------------- ShStruct -------------
	void ShStruct::swap(ShStruct& a) noexcept {
		std::swap(type, a.type);
		std::swap(version_str, a.version_str);
		std::swap(name, a.name);
		std::swap(args, a.args);
		std::swap(info, a.info);
	}
}
