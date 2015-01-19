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
	draw::SPToken GLEffect::Current::Vertex::makeToken(IUserTaskReceiver& r, TPStructR::VAttrID vAttrId) {
		if(_bChanged) {
			_bChanged = false;
			Assert(Trap, _spVDecl, "VDecl is not set")
			return std::make_unique<draw::VStream>(r, _vbuff, _spVDecl, vAttrId);
		}
		return nullptr;
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
	draw::SPToken GLEffect::Current::Index::makeToken(IUserTaskReceiver& r) {
		if(_bChanged) {
			_bChanged = false;
			if(_ibuff)
				return std::make_shared<draw::Buffer>(_ibuff->get()->getDrawToken(r));
		}
		return nullptr;
	}

	// -------------- GLEffect::Current --------------
	GLEffect::Current::Current() {
		tagProc = std::make_unique<draw::Tag_Proc>();
	}
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

			tagProc->addTask([&tp_tmp = *tps](){
				tp_tmp.applySetting();
			});
//		}
	}
	draw::UPTagDraw GLEffect::Current::outputDrawTag() {
		auto ret = std::make_unique<draw::Tag_Draw>();

		// set Program
		ret->addToken(tps->getProgram()->get()->getDrawToken());
		// set VBuffer(VDecl)
		ret->addToken(vertex.makeToken(*tagProc, tps->getVAttrID()));
		// set IBuffer
		if(auto t = index.makeToken(*tagProc))
			ret->addToken(t);

		GLEC_Chk_D(Trap)
		// set uniform value
		if(!uniMap.empty()) {
			// 中身shared_ptrなのでコピーする
			for(auto& sp : uniMap)
				ret->addToken(sp.second);
			uniMap.clear();
		}
		return std::move(ret);
	}
	draw::UPTagProc GLEffect::Current::outputProcTag() {
		if(!tagProc->isEmpty()) {
			auto ret = std::move(tagProc);
			tagProc.reset(new draw::Tag_Proc);
			return std::move(ret);
		}
		return nullptr;
	}
	draw::UPTagDraw GLEffect::_exportTags() {
		if(auto t = _current.outputProcTag())
			_task.pushTag(std::move(t));
		return _current.outputDrawTag();
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
	}
	void GLEffect::setPass(int passId) {
		_current.setPass(passId, _techMap);
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
		auto& tech = _techName[*_current.tech];
		int nP = tech.size();
		for(int i=1 ; i<nP ; i++) {
			if(tech[i] == pass)
				return i-1;
		}
		return spn::none;
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
		VStream::VStream(IUserTaskReceiver& r,
						const HLVb (&vb)[VData::MAX_STREAM],
						const SPVDecl& vdecl,
						TPStructR::VAttrID vAttrId):
			Token(HRes()),
			_spVDecl(vdecl),
			_vAttrId(vAttrId)
		{
			for(int i=0 ; i<countof(_vbuff) ; i++) {
				if(vb[i]) {
					_vbuff[i] = vb[i]->get()->getDrawToken(r);
				}
			}
		}
		void VStream::exec() {
			_spVDecl->apply(VData{_vbuff, _vAttrId});
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
			while(_curWrite >= _curRead) {
				if(_curWrite == _curRead) {
					++_curWrite;
					execTask(false);
					break;
				}
				execTask(false);
				++_curRead;
			}
			GL.glFinish();
			for(auto& e : _entry)
				e.clear();
			_curWrite = _curRead = 0;
		}
		void Task::execTask(bool bSkip) {
			spn::Optional<UniLock> lk(_mutex);
			auto diff = _curWrite - _curRead;
			Assert(Trap, diff >= 0)
			if(diff > 0) {
				lk = spn::none;
				// MThとアクセスするエントリが違うから同期をとらなくて良い
				for(auto& ent : refReadEnt())
					ent->exec(bSkip);
				GL.glFlush();
				lk = spn::construct(std::ref(_mutex));
				++_curRead;
				_cond.signal();
			}
		}

		// -------------- Tag_Proc --------------
		void Tag_Proc::exec(bool bSkip) {
			if(!_task.empty()) {
				for(auto& f : _task)
					f();
				// ここではまだ開放しない
			}
		}
		void Tag_Proc::addTask(UserTask t) {
			_task.push_back(std::move(t));
		}
		bool Tag_Proc::isEmpty() const {
			return _task.empty();
		}

		// -------------- Tag_Draw --------------
		void Tag_Draw::exec(bool bSkip) {
			if(!bSkip) {
				for(auto& t : _token)
					t->exec();
			}
		}
		void Tag_Draw::addToken(const SPToken& token) {
			_token.emplace_back(token);
		}

		// -------------- DrawCall --------------
		DrawCall::DrawCall(GLenum mode, GLint first, GLsizei count): Token(HRes()), _mode(mode), _first(first), _count(count) {}
		void DrawCall::exec() {
			GL.glDrawArrays(_mode, _first, _count);
			GLEC_Chk_D(Trap);
		}

		// -------------- DrawCallI --------------
		DrawCallI::DrawCallI(GLenum mode, GLsizei count, GLenum sizeF, GLuint offset): Token(HRes()), _mode(mode), _count(count), _sizeF(sizeF), _offset(offset) {}
		void DrawCallI::exec() {
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
		auto t = _exportTags();
		// DrawTagにDrawCallTokenを加えた後に出力
		t->addToken(std::make_shared<draw::DrawCall>(mode, first, count));
		_task.pushTag(std::move(t));
	}
	void GLEffect::drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem) {
		auto t = _exportTags();
		HIb hIb = _current.index.getIBuffer();
		auto str = hIb->get()->getStride();
		auto szF = GLIBuffer::GetSizeFlag(str);
		t->addToken(std::make_shared<draw::DrawCallI>(mode, count, szF, offsetElem * str));
		_task.pushTag(std::move(t));
	}
	// Uniform設定は一旦_unifMapに蓄積した後、出力
	void GLEffect::_makeUniformToken(UniMap& dstToken, IUserTaskReceiver& r, GLint id, const bool* b, int n, bool bT) const {
		int tmp[n];
		for(int i=0 ; i<n ; i++)
			tmp[i] = static_cast<int>(b[i]);
		_makeUniformToken(dstToken, r, id, static_cast<const int*>(tmp), 1, bT);
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IUserTaskReceiver& /*r*/, GLint id, const int* iv, int n, bool /*bT*/) const {
		dstToken.emplace(id, std::make_shared<draw::Unif_Vec<int, 1>>(id, iv, 1, n));
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IUserTaskReceiver& /*r*/, GLint id, const float* fv, int n, bool /*bT*/) const {
		dstToken.emplace(id, std::make_shared<draw::Unif_Vec<float, 1>>(id, fv, 1, n));
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IUserTaskReceiver& r, GLint id, const double* dv, int n, bool bT) const {
		float tmp[n];
		for(int i=0 ; i<n ; i++)
			tmp[i] = static_cast<float>(dv[i]);
		_makeUniformToken(dstToken, r, id, static_cast<const float*>(tmp), n, bT);
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IUserTaskReceiver& r, GLint id, const HTex* hTex, int n, bool /*bT*/) const {
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
			dstToken.emplace(id, hTex2.ref()->getDrawToken(r, id, 0, aID));
		}
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IUserTaskReceiver& r, GLint id, const HLTex* hlTex, int n, bool bT) const {
		if(n > 1) {
			std::vector<HLTex> hTexA(n);
			for(int i=0 ; i<n ; i++)
				hTexA[i] = hlTex[i].get();
			_makeUniformToken(dstToken, r, id, hTexA.data(), n, bT);
			return;
		}
		AssertP(Trap, n==1)
		HTex hTex = hlTex->get();
		_makeUniformToken(dstToken, r, id, &hTex, 1, bT);
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
	void GLEffect::execTask(bool bSkip) {
		_task.execTask(bSkip);
	}

	// ------------- ShStruct -------------
	void ShStruct::swap(ShStruct& a) noexcept {
		std::swap(type, a.type);
		std::swap(version_str, a.version_str);
		std::swap(name, a.name);
		std::swap(args, a.args);
		std::swap(info, a.info);
	}
}
