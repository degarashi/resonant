#define BOOST_PP_VARIADICS 1
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
	void ValueSettingR::StencilFuncFront(int func, int ref, int mask) {
		glStencilFuncSeparate(GL_FRONT, func, ref, mask);
	}
	void ValueSettingR::StencilFuncBack(int func, int ref, int mask) {
		glStencilFuncSeparate(GL_BACK, func, ref, mask);
	}
	void ValueSettingR::StencilOpFront(int sfail, int dpfail, int dppass) {
		glStencilOpSeparate(GL_FRONT, sfail, dpfail, dppass);
	}
	void ValueSettingR::StencilOpBack(int sfail, int dpfail, int dppass) {
		glStencilOpSeparate(GL_BACK, sfail, dpfail, dppass);
	}
	void ValueSettingR::StencilMaskFront(int mask) {
		glStencilMaskSeparate(GL_FRONT, mask);
	}
	void ValueSettingR::StencilMaskBack(int mask) {
		glStencilMaskSeparate(GL_BACK, mask);
	}
	ValueSettingR::ValueSettingR(const ValueSetting& s) {
		func = cs_func[s.type];
		int nV = std::min(static_cast<int>(s.value.size()), countof(value));
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
		glEnable, glDisable
	};
	BoolSettingR::BoolSettingR(const BoolSetting& s) {
		func = cs_func[(s.value) ? 0 : 1];
		flag = s.type;
	}
	void BoolSettingR::action() const { func(flag); }
	bool BoolSettingR::operator == (const BoolSettingR& s) const {
		return flag==s.flag && func==s.func;
	}

	GLEffect::EC_FileNotFound::EC_FileNotFound(const std::string& fPath):
		EC_Base((boost::format("file path: \"%1%\" was not found.") % fPath).str()) {}
	// ----------------- VDecl -----------------
	VDecl::VDecl() {}
	VDecl::VDecl(std::initializer_list<VDInfo> il) {
		// StreamID毎に集計
		std::vector<VDInfo> tmp[VData::MAX_STREAM];
		for(auto& v : il)
			tmp[v.streamID].push_back(v);

		// 頂点定義のダブり確認
		for(auto& t : tmp) {
			// オフセットでソート
			std::sort(t.begin(), t.end(), [](const VDInfo& v0, const VDInfo& v1) { return v0.offset < v1.offset; });

			unsigned int ofs = 0;
			for(auto& t2 : t) {
				AssertT(Trap, ofs<=t2.offset, (GLE_Error)(const char*), "invalid vertex offset")
				ofs += GLFormat::QuerySize(t2.elemFlag) * t2.elemSize;
			}
		}

		_func.resize(il.size());
		int cur = 0;
		for(int i=0 ; i<countof(tmp) ; i++) {
			_nEnt[i] = cur;
			for(auto& t2 : tmp[i]) {
				_func[cur] = [t2](GLuint stride, const VData::AttrA& attr) {
					auto attrID = attr[t2.semID];
					if(attrID < 0)
						return;
					glEnableVertexAttribArray(attrID);
					// AssertMsg(Trap, "%1%, %2%", t2.semID, attr[t2.semID])
					GLEC_ChkP(Trap)
					glVertexAttribPointer(attrID, t2.elemSize, t2.elemFlag, t2.bNormalize, stride, (const GLvoid*)t2.offset);
					GLEC_ChkP(Trap)
				};
				++cur;
			}
		}
		_nEnt[VData::MAX_STREAM] = _nEnt[VData::MAX_STREAM-1];
	}
	void VDecl::apply(const VData& vdata) const {
		for(int i=0 ; i<VData::MAX_STREAM ; i++) {
			auto& hl = vdata.hlBuff[i];
			// VStreamが設定されていればBindする
			if(hl.valid()) {
				auto& sp = hl.cref();
				sp->use(GLVBuffer::TagUse);
				GLuint stride = sp->getStride();
				for(int j=_nEnt[i] ; j<_nEnt[i+1] ; j++)
					_func[j](stride, vdata.attrID);
			}
		}
	}

	// ----------------- TPStructR -----------------
	TPStructR::TPStructR() {}
	TPStructR::TPStructR(TPStructR&& tp) {
		swap(tp);
	}
	#define TPR_SWAP(z,data,elem) boost::swap(elem, data.elem);
	#define SEQ_TPR_SWAP (_prog)(_vAttrID)(_setting)(_noDefValue)(_defValue)(_bInit)(_attrL)(_varyL)(_constL)(_unifL)
	void TPStructR::swap(TPStructR& t) noexcept {
		BOOST_PP_SEQ_FOR_EACH(TPR_SWAP, t, SEQ_TPR_SWAP)
	}
	bool TPStructR::findSetting(const Setting& s) const {
		auto itr = std::find(_setting.begin(), _setting.end(), s);
		return itr!=_setting.end();
	}
	SettingList TPStructR::CalcDiff(const TPStructR& from, const TPStructR& to) {
		// toと同じ設定がfrom側にあればスキップ
		// fromに無かったり、異なっていればエントリに加える
		SettingList ret;
		for(auto& s : to._setting) {
			if(!from.findSetting(s)) {
				ret.push_back(s);
			}
		}
		return std::move(ret);
	}

	ShStruct& ShStruct::operator = (const ShStruct& a) {
		this->~ShStruct();
		new(this) ShStruct(a);
		return *this;
	}
	ShStruct::ShStruct(ShStruct&& a): ShStruct() { swap(a); }
	void ShStruct::swap(ShStruct& a) noexcept {
		std::swap(type, a.type);
		std::swap(name, a.name);
		std::swap(args, a.args);
		std::swap(info, a.info);
	}
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
		boost::regex re_comment(R"(//[^\s$]+)"),		//!< 一行コメント
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
		// テスト表示
		_result.output(std::cout);
		if(bS)
			std::cout << "------- analysis succeeded! -------" << std::endl;
		else
			std::cout << "------- analysis failed! -------" << std::endl;
		if(itr != str.cend()) {
			std::cout << "<but not reached to end>" << std::endl
				<< "remains: " << std::endl << std::string(itr, str.cend()) << std::endl;
		}
	#endif
		if(!bS || itr!=str.cend())
			throw EC_GLXGrammar("invalid GLEffect format");

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
				_techMap.insert(std::make_pair(GL16ID(techID, passID), TPStructR(_result, techID, passID)));
			}
		}
		GLEC_ChkP(Trap)
	}
	void GLEffect::onDeviceLost() {
		if(_bInit) {
			_bInit = false;
			for(auto& p : _techMap)
				p.second.ts_onDeviceLost();

			if(!_idTech)
				_idTech = _idTechCur;
			if(!_idPass)
				_idPass = _idPassCur;
			_idTechCur = _idPassCur = boost::none;
			_bDefaultParam = true;
			_tps = boost::none;
			_texIndex.clear();
			_rflg = REFL_ALL;

			// セットされているUniform変数を未セット状態にする
			for(auto& p : _uniMapIDTmp)
				_uniMapID.insert(p);
			_uniMapIDTmp.clear();
		}
	}
	void GLEffect::onDeviceReset() {
		if(!_bInit) {
			_bInit = true;
			for(auto& p : _techMap)
				p.second.ts_onDeviceReset();
		}
	}
	void GLEffect::setVDecl(const SPVDecl &decl) {
		if(_spVDecl.get() != decl.get()) {
			_spVDecl = decl;
			_rflg |= REFL_VSTREAM;
		}
	}
	void GLEffect::setVStream(HVb vb, int n) {
		_vBuffer[n] = vb;
		_rflg |= REFL_VSTREAM;
	}
	void GLEffect::setIStream(HIb ib) {
		_iBuffer = ib;
		_rflg |= REFL_ISTREAM;
	}
	void GLEffect::setTechnique(int techID, bool bDefault) {
		if(_idTech != techID) {
			_idTech = techID;
			_bDefaultParam = bDefault;
			_rflg |= REFL_PROGRAM;
		}
	}
	void GLEffect::setPass(int passID) {
		if(_idPass != passID) {
			_idPass = passID;
			_rflg |= REFL_PROGRAM;
		}
	}
	int GLEffect::getTechID(const std::string& tech) const {
		int nT = _techName.size();
		for(int i=0 ; i<nT ; i++) {
			if(_techName[i][0] == tech)
				return i;
		}
		return -1;
	}
	int GLEffect::getPassID(const std::string& pass) const {
		AssertT(Trap, _idTech, (GLE_Error)(const char*), "tech is not selected")
		auto& tech = _techName[*_idTech];
		int nP = tech.size();
		for(int i=1 ; i<nP ; i++) {
			if(tech[i] == pass)
				return i-1;
		}
		return -1;
	}
	int GLEffect::getCurPassID() const {
		AssertT(Trap, _idPass, (GLE_Error)(const char*), "pass is not selected")
		return *_idPass;
	}
	int GLEffect::getCurTechID() const {
		AssertT(Trap, _idTech, (GLE_Error)(const char*), "tech is not selected")
		return *_idTech;
	}

	// Uniformは一旦キャッシュにとっておいて後でセット
	// Uniformを毎回名前で検索するのがアレなのでIDを取得
	// 現在選択しているShaderをUseし、頂点ポインタの設定
	void GLEffect::applySetting() {
		if(_rflg != 0) {
			_refreshProgram();
			_refreshUniform();
			_refreshVStream();
			_refreshIStream();
		}
	}
	void GLEffect::_refreshProgram() {
		if(Bit::ChClear(_rflg, REFL_PROGRAM)) {
			AssertT(Trap, (_idTech && _idPass), (GLE_Error)(const char*), "tech or pass is not selected")

			GL16ID id(*_idTech, *_idPass);
			auto& tps = _techMap.at(id);
			// [_idTechCur|_idPassCur] から [_idTech|_idPass] への遷移
			// とりあえず全設定
			tps.getProgram().cref()->use();
			tps.applySetting();

			_uniMapIDTmp.clear();
			if(_bDefaultParam) {
				// デフォルト値読み込み
				const auto& def = tps.getUniformDefault();
				for(auto& ent : def)
					_uniMapID.insert(ent);
			} else
				_uniMapID.clear();

			_tps = tps;
			_idTechCur = _idTech;
			_idPassCur = _idPass;

			// テクスチャインデックスリスト作成
			_texIndex.clear();
			GLuint pid = tps.getProgram().cref()->getProgramID();
			GLint nUnif;
			glGetProgramiv(pid, GL_ACTIVE_UNIFORMS, &nUnif);

			GLsizei len;
			int size;
			GLenum typ;
			GLchar cbuff[0];
			// Sampler2D変数が見つかった順にテクスチャIDを割り振る
			GLint cur = 0;
			for(GLint i=0 ; i<nUnif ; i++) {
				GLEC_P(Trap, glGetActiveUniform, pid, i, sizeof(cbuff), &len, &size, &typ, cbuff);
				if(typ == GL_SAMPLER_2D)
					_texIndex.insert(std::make_pair(i, cur++));
			}
		}
	}
	namespace {
		struct UniVisitor : boost::static_visitor<> {
			GLint						_id;
			const GLEffect::TexIndex&	_tIdx;

			UniVisitor(const GLEffect::TexIndex& ti): _tIdx(ti) {}
			void setID(GLint id) { _id = id; }
			void operator()(bool b) const { glUniform1i(_id, b ? 0 : 1); }
			void operator()(int v) const { glUniform1i(_id, v); }
			void operator()(float v) const { glUniform1f(_id, v); }
			void operator()(const spn::Vec3& v) const { glUniform3fv(_id, 1, v.m); }
			void operator()(const spn::Vec4& v) const { glUniform4fv(_id, 1, v.m);}
			void operator()(const spn::AMat32& m) const {
				this->operator()(m.convert33()); }
			void operator()(const spn::Mat32& m) const {
				this->operator()(m.convert33());
			}
			void operator()(const spn::AMat33& m) const {
				this->operator()(spn::Mat33(m)); }
			void operator()(const spn::Mat33& m) const {
				glUniformMatrix3fv(_id, 1, true, reinterpret_cast<const GLfloat*>(m.ma)); }
			void operator()(const spn::AMat43& m) const {
				this->operator()(m.convert44()); }
			void operator()(const spn::Mat43& m) const {
				this->operator()(m.convert44()); }
			void operator()(const spn::AMat44& m) const {
				this->operator()(spn::Mat44(m)); }
			void operator()(const spn::Mat44& m) const {
				glUniformMatrix4fv(_id, 1, true, reinterpret_cast<const GLfloat*>(m.ma));
			}
			void operator()(const HLTex& tex) const {
				auto itr = _tIdx.find(_id);
				if(itr != _tIdx.end()) {
					auto& cr = tex.cref();
					cr->setActiveID(itr->second);
					cr->use(IGLTexture::TagUse);
					glUniform1i(_id, itr->second);
				} else
					Assert(Warn, false, "uniform id=%1% is not sampler", _id)
			}
		};
	}
	void GLEffect::_refreshUniform() {
		// 現状なら_uniMapID.empty()だけで判定できるが便宜上
		if(Bit::ChClear(_rflg, REFL_UNIFORM)) {
			_refreshProgram();
			if(!_uniMapID.empty()) {
				// UnifParamの変数をシェーダーに設定
				UniVisitor visitor(_texIndex);
				for(auto itr=_uniMapID.begin() ; itr!=_uniMapID.end() ; itr++) {
					visitor.setID(itr->first);
					boost::apply_visitor(visitor, itr->second);
					GLEC_ChkP(Trap);
					// 設定し終わったらエントリを削除 (設定済みの方へ移す)
					_uniMapIDTmp.insert(*itr);
				}
				_uniMapID.clear();
			}
		}
	}
	void GLEffect::_refreshVStream() {
		if(Bit::ChClear(_rflg, REFL_VSTREAM)) {
			_refreshProgram();
			// VertexAttribをシェーダーに設定
			_tps->setVertex(_spVDecl, _vBuffer);
		}
	}
	void GLEffect::_refreshIStream() {
		if(Bit::ChClear(_rflg, REFL_ISTREAM)) {
			_refreshProgram();
			// ElementArrayをシェーダーに設定
			_iBuffer.cref()->use(GLIBuffer::TagUse);
			GLEC_ChkP(Trap)
		}
	}

	GLint GLEffect::getUniformID(const std::string& name) {
		_refreshProgram();
		GLint loc = glGetUniformLocation(_tps->getProgram().cref()->getProgramID(), name.c_str());
		GLEC_ChkP(Trap)
		Assert(Warn, loc>=0, "Uniform argument \"%1%\" not found", name)
		return loc;
	}
	void GLEffect::draw(GLenum mode, GLint first, GLsizei count) {
		applySetting();
		glDrawArrays(mode, first, count);
		GLEC_ChkP(Trap);
	}
	void GLEffect::drawIndexed(GLenum mode, GLsizei count, GLuint offset) {
		applySetting();
		GLenum sz = _iBuffer.cref()->getStride() == sizeof(GLshort) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;
		glDrawElements(mode, count, sz, reinterpret_cast<const GLvoid*>(offset));
		GLEC_ChkP(Trap);
	}
	const UniMapID& GLEffect::getUniformMap() {
		_refreshUniform();
		return _uniMapIDTmp;
	}
	namespace {
		class TPSDupl {
			using TPList = std::vector<const TPStruct*>;
			const GLXStruct &_glx;
			const TPStruct &_tTech, &_tPass;
			//! [Pass][Tech][Tech(Base0)][Tech(Base1)]...
			TPList _tpList;

			void _getTPStruct(TPList& dst, const TPStruct* tp) const {
				dst.push_back(tp);
				auto& der = tp->derive;
				for(auto& name : der) {
					auto itr = std::find_if(_glx.tpL.begin(), _glx.tpL.end(), [&name](const boost::recursive_wrapper<TPStruct>& t){return t.get().name == name;});
					_getTPStruct(dst, &(*itr));
				}
			}
			public:
				TPSDupl(const GLXStruct& gs, int tech, int pass): _glx(gs), _tTech(gs.tpL.at(tech)), _tPass(_tTech.tpL.at(pass).get()) {
					_tpList.push_back(&_tPass);
					// 継承関係をリストアップ
					_getTPStruct(_tpList, &_tTech);
				}

				template <class ST>
				void _extractBlocks(std::vector<const ST*>& dst, const ST* attr, const NameMap<ST> (GLXStruct::*mfunc)) const {
					for(auto itr=attr->derive.rbegin() ; itr!=attr->derive.rend() ; itr++) {
						auto* der = &(_glx.*mfunc).at(*itr);
						_extractBlocks(dst, der, mfunc);
					}
					if(std::find(dst.begin(), dst.end(), attr) == dst.end())
						dst.push_back(attr);
				}

				template <class ST, class ENT>
				std::vector<const ENT*> exportEntries(uint32_t blockID, const std::map<std::string,ST> (GLXStruct::*mfunc)) const {
					// 使用されるAttributeブロックを収集
					std::vector<const ST*> tmp, tmp2;
					// 配列末尾から処理をする
					for(auto itr=_tpList.rbegin() ; itr!=_tpList.rend() ; itr++) {
						const TPStruct* tp = (*itr);
						// ブロックは順方向で操作 = 後に書いたほうが優先
						for(auto& blk : tp->blkL) {
							if(blk.type == blockID) {
								if(!blk.bAdd)
									tmp.clear();
								for(auto& name : blk.name)
									tmp.push_back(&(_glx.*mfunc).at(name));
							}
						}
					}
					// ブロック継承展開
					for(auto& p : tmp) {
						// 既に同じブロックが登録されていたら何もしない(エントリの重複を省く)
						_extractBlocks(tmp2, p, mfunc);
					}
					// エントリ抽出: 同じ名前のエントリがあればエラー = 異なるエントリに同じ変数が存在している
					std::vector<const ENT*> ret;
					for(auto& p : tmp2) {
						for(auto& e : p->entry) {
							if(std::find_if(ret.begin(), ret.end(), [&e](const ENT* tmp){return e.name==tmp->name;}) != ret.end())
								throw GLE_LogicalError((boost::format("duplication of entry \"%1%\"") % e.name).str());
							ret.push_back(&e);
						}
					}
					return ret;
				}
				using MacroMap = TPStructR::MacroMap;
				using MacroPair = MacroMap::value_type;
				MacroMap exportMacro() const {
					MacroMap mm;
					for(auto itr=_tpList.rbegin() ; itr!=_tpList.rend() ; itr++) {
						for(auto& mc : (*itr)->mcL) {
							MacroPair mp(mc.fromStr, mc.toStr ? (*mc.toStr) : std::string());
							mm.insert(std::move(mp));
						}
					}
					return std::move(mm);
				}
				SettingList exportSetting() const {
					std::vector<ValueSettingR> vsL;
					std::vector<BoolSettingR> bsL;
					for(auto itr=_tpList.rbegin() ; itr!=_tpList.rend() ; itr++) {
						const TPStruct* tp = (*itr);
						// フラグ設定エントリ
						for(auto& bs : tp->bsL) {
							// 実行時形式に変換してからリストに追加
							BoolSettingR bsr(bs);
							auto itr=std::find(bsL.begin(), bsL.end(), bsr);
							if(itr == bsL.end()) {
								// 新規に追加
								bsL.push_back(bsr);
							} else {
								// 既存の項目を上書き
								*itr = bsr;
							}
						}

						// 値設定エントリ
						for(auto& vs : tp->vsL) {
							ValueSettingR vsr(vs);
							auto itr=std::find(vsL.begin(), vsL.end(), vsr);
							if(itr == vsL.end())
								vsL.push_back(vsr);
							else
								*itr = vsr;
						}
					}
					SettingList ret;
					for(auto& b : bsL)
						ret.push_back(b);
					for(auto& v : vsL)
						ret.push_back(v);
					return std::move(ret);
				}
		};
		template <class DST, class SRC>
		void OutputS(DST& dst, const SRC& src) {
			for(auto& p : src) {
				p->output(dst);
				dst << std::endl;
			}
		}
	}

	namespace {
		struct Visitor : boost::static_visitor<> {
			GLuint		pgID;
			GLint		uniID;
			UniMapID	result;

			bool setKey(const std::string& key) {
				uniID = glGetUniformLocation(pgID, key.c_str());
				GLEC_ChkP(Trap)
				Assert(Warn, uniID>=0, "Uniform argument \"%1%\" not found", key)
				return uniID >= 0;
			}
			template <class T>
			void _addResult(T&& t) {
				if(uniID >= 0)
					result.insert(std::make_pair(uniID, std::forward<T>(t)));
			}

			void operator()(const std::vector<float>& v) {
				if(v.size() == 3)
					_addResult(spn::Vec3{v[0],v[1],v[2]});
				else
					_addResult(spn::Vec4{v[0],v[1],v[2],v[3]});
			}
			template <class T>
			void operator()(const T& v) {
				_addResult(v);
			}
		};
	}
	TPStructR::TPStructR(const GLXStruct& gs, int tech, int pass) {
		auto& tp = gs.tpL.at(tech);
		auto& tps = tp.tpL[pass].get();
		const ShSetting* selectSh[ShType::NUM_SHTYPE] = {};
		// PassかTechからシェーダー名を取ってくる
		for(auto& a : tp.shL)
			selectSh[a.type] = &a;
		for(auto& a : tps.shL)
			selectSh[a.type] = &a;

		// VertexとPixelシェーダは必須、Geometryは任意
		AssertT(Trap, (selectSh[ShType::VERTEX] && selectSh[ShType::PIXEL]), (GLE_LogicalError)(const char*), "no vertex or pixel shader found")

		std::stringstream ss;
		TPSDupl dupl(gs, tech, pass);
		HLSh shP[ShType::NUM_SHTYPE];
		for(int i=0 ; i<countof(selectSh) ; i++) {
			auto* shp = selectSh[i];
			if(!shp)
				continue;

			{
				// マクロを定義をGLSLソースに出力
				auto mc = dupl.exportMacro();
				for(auto& p : mc)
					ss << "#define " << p.first << ' ' << p.second << std::endl;
			}
			if(i==ShType::VERTEX) {
				// Attribute定義は頂点シェーダの時だけ出力
				_attrL = dupl.exportEntries<AttrStruct,AttrEntry>(GLBlocktype_::attributeT, &GLXStruct::atM);
				OutputS(ss, _attrL);
			}
			// それぞれ変数ブロックをGLSLソースに出力
			// :Varying
			_varyL = dupl.exportEntries<VaryStruct,VaryEntry>(GLBlocktype_::varyingT, &GLXStruct::varM);
			OutputS(ss, _varyL);
			// :Const
			_constL = dupl.exportEntries<ConstStruct,ConstEntry>(GLBlocktype_::constT, &GLXStruct::csM);
			OutputS(ss, _constL);
			// :Uniform
			_unifL = dupl.exportEntries<UnifStruct,UnifEntry>(GLBlocktype_::uniformT, &GLXStruct::uniM);
			OutputS(ss, _unifL);

			const ShStruct& s = gs.shM.at(shp->shName);
			// シェーダー引数の型チェック
			// ユーザー引数はグローバル変数として用意
			ArgChecker acheck(ss, shp->shName, s.args);
			for(auto& a : shp->args)
				boost::apply_visitor(acheck, a);
			acheck.finalizeCheck();

			// 関数名はmain()に書き換え
			ss << "void main() {" << s.info << '}' << std::endl;
	#ifdef DEBUG
			std::cout << ss.str();
			std::cout.flush();
	#endif
			shP[i] = mgr_gl.makeShader(c_glShFlag[i], ss.str());

			ss.str("");
			ss.clear();
		}
		// シェーダーのリンク処理
		_prog = mgr_gl.makeProgram(shP[0].get(), shP[1].get(), shP[2].get());
		// OpenGLステート設定リストを形成
		SettingList sl = dupl.exportSetting();
		_setting.swap(sl);
	}
	void TPStructR::ts_onDeviceLost() {
		AssertP(Trap, _bInit)
		_bInit = false;
		// OpenGLのリソースが絡んでる変数を消去
		std::memset(_vAttrID, 0xff, sizeof(_vAttrID));
		_noDefValue.clear();
		_defValue.clear();
	}
	void TPStructR::ts_onDeviceReset() {
		AssertP(Trap, !_bInit)
		_bInit = true;
		auto& prog = *_prog.cref();
		prog.onDeviceReset();

		// 頂点AttribIDを無効な値で初期化
		for(auto& v : _vAttrID)
			v = -2;	// 初期値=-2, クエリの無効値=-1
		for(auto& p : _attrL) {
			// 頂点セマンティクス対応リストを生成
			// セマンティクスの重複はエラー
			auto& atID = _vAttrID[p->sem];
			AssertT(Trap, atID==-2, (GLE_LogicalError)(const std::string&), (boost::format("duplication of vertex semantics \"%1% : %2%\"") % p->name % GLSem_::cs_typeStr[p->sem]).str())
			atID = glGetAttribLocation(prog.getProgramID(), p->name.c_str());
			GLEC_ChkP(Trap)
			// -1の場合は警告を出す(もしかしたらシェーダー内で使ってないだけかもしれない)
		}

		// Uniform変数にデフォルト値がセットしてある物をリストアップ
		Visitor visitor;
		visitor.pgID = _prog.cref()->getProgramID();
		for(const auto* p : _unifL) {
			if(p->defStr) {
				// 変数名をIDに変換
				if(visitor.setKey(p->name))
					boost::apply_visitor(visitor, *p->defStr);
			} else
				_noDefValue.insert(p->name);
		}
		_defValue.swap(visitor.result);
	}

	void TPStructR::applySetting() const {
		struct Visitor : boost::static_visitor<> {
			void operator()(const BoolSettingR& bs) const {
				bs.action();
			}
			void operator()(const ValueSettingR& vs) const {
				vs.action();
			}
		};
		for(auto& st : _setting)
			boost::apply_visitor(Visitor(), st);
	}
	const UniMapID& TPStructR::getUniformDefault() const { return _defValue; }
	const UniEntryMap& TPStructR::getUniformEntries() const { return _noDefValue; }

	void TPStructR::setVertex(const SPVDecl &vdecl, const HLVb (&stream)[VData::MAX_STREAM]) const {
		vdecl->apply(VData(stream, _vAttrID));
	}
	const HLProg& TPStructR::getProgram() const { return _prog; }
}
