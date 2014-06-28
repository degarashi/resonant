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
					GL.glEnableVertexAttribArray(attrID);
					#ifndef USE_OPENGLES2
						auto typ = GLFormat::QueryGLSLInfo(t2.elemFlag)->type;
						if(typ == GLSLType::BoolT || typ == GLSLType::IntT) {
							// PCにおけるAMDドライバの場合、Int値はIPointerでセットしないと値が化けてしまう為
							GL.glVertexAttribIPointer(attrID, t2.elemSize, t2.elemFlag, stride, reinterpret_cast<const GLvoid*>(t2.offset));
						} else
					#endif
							GL.glVertexAttribPointer(attrID, t2.elemSize, t2.elemFlag, t2.bNormalize, stride, reinterpret_cast<const GLvoid*>(t2.offset));
				};
				++cur;
			}
		}
		_nEnt[VData::MAX_STREAM] = _nEnt[VData::MAX_STREAM-1];
	}
	void VDecl::apply(const VData& vdata) const {
		for(int i=0 ; i<VData::MAX_STREAM ; i++) {
			auto& ovb = vdata.buff[i];
			// VStreamが設定されていればBindする
			if(ovb) {
				auto& vb = *ovb;
				vb.use_begin();
				GLuint stride = vb.getStride();
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
	#define SEQ_TPR_SWAP (_prog)(_vAttrID)(_setting)(_noDefValue)(_defaultValue)(_bInit)(_attrL)(_varyL)(_constL)(_unifL)
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

	void ShStruct::swap(ShStruct& a) noexcept {
		std::swap(type, a.type);
		std::swap(version_str, a.version_str);
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
		boost::regex re_comment(R"(//[^\n$]+)"),		//!< 一行コメント
					re_comment2(R"(/\*[^\*]*\*/)");		//!< 範囲コメント

		class TPSDupl {
			private:
				using TPList = std::vector<const TPStruct*>;
				const GLXStruct &_glx;
				const TPStruct &_tTech, &_tPass;
				//! 優先度順に並べたTPStructのポインタリスト [Pass][Tech][Tech(Base0)][Tech(Base1)]...
				TPList _tpList;

				void _listupBlocks(TPList& dst, const TPStruct* tp) const {
					dst.push_back(tp);
					auto& tpL = _glx.tpL;
					for(auto& name : tp->derive) {
						auto itr = std::find_if(tpL.begin(),
												tpL.end(),
												[&name](const boost::recursive_wrapper<TPStruct>& t){
													return t.get().name == name;
												});
						// 継承Techは必ずGLEffectが持っている筈
						Assert(Trap, itr!=tpL.end());
						_listupBlocks(dst, &(*itr));
					}
				}
				template <class ST>
				void _extractBlocks(std::vector<const ST*>& dst, const ST* attr, const NameMap<ST> (GLXStruct::*mfunc)) const {
					for(auto itr=attr->derive.rbegin() ; itr!=attr->derive.rend() ; itr++) {
						Assert(Trap, (_glx.*mfunc).count(*itr)==1)
						auto* der = &(_glx.*mfunc).at(*itr);
						_extractBlocks(dst, der, mfunc);
					}
					if(std::find(dst.begin(), dst.end(), attr) == dst.end())
						dst.push_back(attr);
				}
			public:
				TPSDupl(const GLXStruct& gs, int tech, int pass):
					_glx(gs),
					_tTech(gs.tpL.at(tech)),
					_tPass(_tTech.tpL.at(pass).get())
				{
					// 継承関係をリストアップ
					// Pass, Tech, Tech(Base0) ... Tech(BaseN) の順番
					_tpList.push_back(&_tPass);
					_listupBlocks(_tpList, &_tTech);
				}

				template <class ST, class ENT>
				std::vector<const ENT*> exportEntries(uint32_t blockID, const std::map<std::string,ST> (GLXStruct::*mfunc)) const {
					// 使用されるAttributeブロックを収集
					std::vector<const ST*> tmp, tmp2;
					// 配列末尾から処理をする = Pass, Tech, TechBase... の順
					for(auto itr=_tpList.rbegin() ; itr!=_tpList.rend() ; itr++) {
						const TPStruct* tp = (*itr);
						// ブロックは順方向で操作 ( Block = A,B,C ならAが優先 )
						for(auto& blk : tp->blkL) {
							if(blk.type == blockID) {
								// += 演算子でないならエントリを初期化
								if(!blk.bAdd)
									tmp.clear();
								for(auto& name : blk.name) {
									Assert(Trap, (_glx.*mfunc).count(name)==1)
									tmp.push_back(&(_glx.*mfunc).at(name));
								}
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
				dst << *p << std::endl;
			}
		}
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
					_techMap.insert(std::make_pair(GL16ID(techID, passID), TPStructR(_result, techID, passID)));
				}
			}
		} catch(const std::exception& e) {
			LogOutput("GLEffect exception: %1%", e.what());
			throw;
		}
		GLEC_ChkP(Trap)
	}
	void GLEffect::onDeviceLost() {
		if(_bInit) {
			_bInit = false;
			for(auto& p : _techMap)
				p.second.ts_onDeviceLost();

			auto& cur = _current;
			cur.tech = spn::none;
			cur.pass = spn::none;
			_bDefaultParam = true;
			cur.tps = spn::none;
			cur.texIndex.clear();
			// セットされているUniform変数を未セット状態にする
			cur.uniMap.clear();
		}
	}
	void GLEffect::onDeviceReset() {
		if(!_bInit) {
			_bInit = true;
			for(auto& p : _techMap)
				p.second.ts_onDeviceReset(*this);
		}
	}
	// Tech, Pass, 他InitTagの内容を変更したらフラグを立て、
	void GLEffect::setVDecl(const SPVDecl& decl) {
		auto& it = _current.init;
		if(!it._spVDecl || it._spVDecl != decl) {
			_current.bInit = true;
			it._spVDecl = decl;
		}
	}
	void GLEffect::setVStream(HVb vb, int n) {
		auto& it = _current.init;
		if(!it._opVb[n] || it._opVb[n]->getBuffID() != vb.ref()->getBuffID()) {
			_current.bInit = true;
			HRes hRes = vb;
			it._opVb[n] = vb.ref()->getDrawToken(_current.init, hRes);
		}
	}
	void GLEffect::setIStream(HIb ib) {
		auto& it = _current.init;
		if(!it._opIb || it._opIb->getBuffID() != ib.ref()->getBuffID()) {
			_current.bInit = true;
			HRes hRes = ib;
			it._opIb = ib.ref()->getDrawToken(_current.init, hRes);
		}
	}
	void GLEffect::setTechnique(int techID, bool bDefault) {
		// TechIDをセットしたらPassIDは無効になる
		auto& cur = _current;
		if(!cur.tech || *cur.tech != techID) {
			cur.tech = techID;
			cur.pass = spn::none;
			_bDefaultParam = bDefault;
			cur.bInit = true;
		}
	}
	void GLEffect::setPass(int passID) {
		// TechIDをセットせずにPassIDをセットするのは禁止
		AssertT(Trap, _current.tech, (GLE_Error)(const char*), "tech is not selected")
		auto& cur = _current;
//		if(!cur.pass || *cur.pass != passID) {
			cur.pass = passID;
			GL16ID id(*cur.tech, *cur.pass);
			Assert(Trap, _techMap.count(id)==1)
			cur.tps = _techMap.at(id);
			cur.bInit = true;
			auto& tps = *cur.tps;
			cur.init._opProgram = spn::construct(tps.getProgram().get());
			cur.init._opVAttrID = tps.getVAttrID();
			// UnifMapをクリア
			cur.uniMap.clear();
			// デフォルト値読み込み
			if(_bDefaultParam) {
				auto& def = tps.getUniformDefault();
				cur.uniMap = def;
			}
			// テクスチャインデックスリスト作成
			GLuint pid = tps.getProgram().cref()->getProgramID();
			GLint nUnif;
			GL.glGetProgramiv(pid, GL_ACTIVE_UNIFORMS, &nUnif);

			GLsizei len;
			int size;
			GLenum typ;
			GLchar cbuff[0x100];	// GLSL変数名の最大がよくわからない
			// Sampler2D変数が見つかった順にテクスチャIDを割り振る
			GLint curI = 0;
			cur.texIndex.clear();
			for(GLint i=0 ; i<nUnif ; i++) {
				GLEC_P(Trap, glGetActiveUniform, pid, i, sizeof(cbuff), &len, &size, &typ, cbuff);
				auto opInfo = GLFormat::QueryGLSLInfo(typ);
				if(opInfo->type == GLSLType::TextureT) {
					// GetActiveUniformでのインデックスとGetUniformLocationIDは異なる場合があるので・・
					GLint id = GL.glGetUniformLocation(pid, cbuff);
					Assert(Trap, id>=0)
					cur.texIndex.insert(std::make_pair(id, curI++));
				}
			}

			cur.init.addPreFunc([&tps](){
				tps.applySetting();
			});
//		}
	}
	OPGLint GLEffect::getTechID(const std::string& tech) const {
		int nT = _techName.size();
		for(int i=0 ; i<nT ; i++) {
			if(_techName[i][0] == tech)
				return i;
		}
		return spn::none;
	}
	OPGLint GLEffect::getPassID(const std::string& pass) const {
		AssertT(Trap, _current.tech, (GLE_Error)(const char*), "tech is not selected")
		auto& tech = _techName[*_current.tech];
		int nP = tech.size();
		for(int i=1 ; i<nP ; i++) {
			if(tech[i] == pass)
				return i-1;
		}
		return spn::none;
	}
	OPGLint GLEffect::getCurPassID() const {
		return _current.pass;
	}
	OPGLint GLEffect::getCurTechID() const {
		return _current.tech;
	}
	HLProg GLEffect::getProgram(int techID, int passID) const {
		if(techID < 0) {
			if(!_current.tech)
				return HLProg();
			techID = *_current.tech;
		}
		if(passID < 0) {
			if(!_current.pass)
				return HLProg();
			passID = *_current.pass;
		}
		auto itr = _techMap.find(GL16ID(techID, passID));
		if(itr != _techMap.end())
			return itr->second.getProgram();
		return HLProg();
	}

	void GLEffect::_exportInitTag() {
		// もしInitTagが有効ならそれを出力
		if(_current.bInit) {
			_current.bInit = false;
			// 後続の描画コールのためにコピーを手元に残す
			_task.pushTag(new draw::InitTag(_current.init));
			_current.init.clearTags();
		}
	}

	namespace draw {
		void Tag::clearTags() {
			_funcL.clear();
		}
		// -------------- Task --------------
		Task::Task(): _curWrite(0), _curRead(0) {}
		Task::UPTagL& Task::refWriteEnt() {
			UniLock lk(_mutex);
			return _entry[_curWrite % NUM_ENTRY];
		}
		Task::UPTagL& Task::refReadEnt() {
			UniLock lk(_mutex);
			return _entry[_curRead % NUM_ENTRY];
		}
		void Task::pushTag(Tag* tag) {
			// DThとアクセスするエントリが違うからemplace_back中の同期をとらなくて良い
			refWriteEnt().emplace_back(tag);
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
			refWriteEnt().clear();
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
				void (Tag::*func)() = (bSkip) ? &Tag::cancel : &Tag::exec;
				// MThとアクセスするエントリが違うから同期をとらなくて良い
				for(auto& ent : refReadEnt())
					((*ent).*func)();
				GL.glFlush();
				lk = spn::construct(std::ref(_mutex));
				++_curRead;
				_cond.signal();
			}
		}

		// -------------- Tag --------------
		void Tag::exec() {
			cancel();
		}
		void Tag::cancel() {
			if(!_funcL.empty()) {
				for(auto& f : _funcL)
					f();
				// ここではまだ開放しない
			}
		}
		void Tag::addPreFunc(PreFunc pf) {
			_funcL.push_back(std::move(pf));
		}

		// -------------- NormalTag --------------
		void NormalTag::exec() {
			Tag::exec();
			for(auto& f : _tokenL)
				f->exec();
		}

		// -------------- InitTag --------------
		void InitTag::exec() {
			_opProgram->exec();
			// VertexAttribをシェーダーに設定
			_spVDecl->apply(VData(_opVb, *_opVAttrID));
			// ElementArrayをシェーダーに設定
			if(_opIb)
				_opIb->use_begin();
			GLEC_ChkP(Trap)

			// Programをセットした後にPreFuncを実行
			Tag::exec();
		}

		// -------------- DrawCall --------------
		DrawCall::DrawCall(GLenum mode, GLint first, GLsizei count): Token(HRes()), _mode(mode), _first(first), _count(count) {}
		void DrawCall::exec() {
			GL.glDrawArrays(_mode, _first, _count);
			GLEC_ChkP(Trap);
		}

		// -------------- DrawCallI --------------
		DrawCallI::DrawCallI(GLenum mode, GLsizei count, GLenum sizeF, GLuint offset): Token(HRes()), _mode(mode), _count(count), _sizeF(sizeF), _offset(offset) {}
		void DrawCallI::exec() {
			GL.glDrawElements(_mode, _count, _sizeF, reinterpret_cast<const GLvoid*>(_offset));
			GLEC_ChkP(Trap);
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
	bool GLEffect::_exportUniform() {
		// 何かUniform変数がセットされていたらNormalTagとして出力
		auto& cur = _current;
		if(!cur.uniMap.empty()) {
			// 中身shared_ptrなのでコピーする
			for(auto& sp : cur.uniMap)
				cur.normal._tokenL.push_back(sp.second);
			cur.uniMap.clear();
			return true;
		}
		return false;
	}
	void GLEffect::draw(GLenum mode, GLint first, GLsizei count) {
		_exportInitTag();
		_exportUniform();
		// NormalTagにDrawCallTokenを加えた後に出力
		_current.normal._tokenL.emplace_back(new draw::DrawCall(mode, first, count));
		_task.pushTag(new draw::NormalTag(std::move(_current.normal)));
		_current.bNormal = false;
	}
	void GLEffect::drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem) {
		_exportInitTag();
		_exportUniform();
		draw::Buffer& ib = *_current.init._opIb;
		auto str = ib.getStride();
		auto szF = GLIBuffer::GetSizeFlag(str);
		_current.normal._tokenL.emplace_back(new draw::DrawCallI(mode, count, szF, offsetElem * str));
		_task.pushTag(new draw::NormalTag(std::move(_current.normal)));
		_current.bNormal = false;
	}
	void GLEffect::setUserPriority(Priority p) {
		_current.prio.userP = p;
	}
	// Uniform設定は一旦_unifMapに蓄積した後、出力
	void GLEffect::_makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const bool* b, int n, bool bT) const {
		int tmp[n];
		for(int i=0 ; i<n ; i++)
			tmp[i] = static_cast<int>(b[i]);
		_makeUniformToken(dstToken, pf, id, static_cast<const int*>(tmp), 1, bT);
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IPreFunc& /*pf*/, GLint id, const int* iv, int n, bool /*bT*/) const {
		dstToken.emplace(id, std::make_shared<draw::Unif_Vec<int, 1>>(id, iv, 1, n));
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IPreFunc& /*pf*/, GLint id, const float* fv, int n, bool /*bT*/) const {
		dstToken.emplace(id, std::make_shared<draw::Unif_Vec<float, 1>>(id, fv, 1, n));
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const double* dv, int n, bool bT) const {
		float tmp[n];
		for(int i=0 ; i<n ; i++)
			tmp[i] = static_cast<float>(dv[i]);
		_makeUniformToken(dstToken, pf, id, static_cast<const float*>(tmp), n, bT);
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const HTex* hTex, int n, bool /*bT*/) const {
		// テクスチャユニット番号を検索
		auto itr = _current.texIndex.find(id);
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
			dstToken.emplace(id, hTex2.ref()->getDrawToken(pf, id, 0, aID, *hTex));
		}
	}
	void GLEffect::_makeUniformToken(UniMap& dstToken, IPreFunc& pf, GLint id, const HLTex* hlTex, int n, bool bT) const {
		if(n > 1) {
			std::vector<HLTex> hTexA(n);
			for(int i=0 ; i<n ; i++)
				hTexA[i] = hlTex[i].get();
			_makeUniformToken(dstToken, pf, id, hTexA.data(), n, bT);
			return;
		}
		AssertP(Trap, n==1)
		HTex hTex = hlTex->get();
		_makeUniformToken(dstToken, pf, id, &hTex, 1, bT);
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
		_current.tech = spn::none;
		_current.pass = spn::none;
		_current.tps = spn::none;
		_current.bInit = false;
		_current.bNormal = false;
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

	namespace {
		struct Visitor : boost::static_visitor<>, IPreFunc {
			GLuint		pgID;
			GLint		uniID;
			UniMap		result;
			PreFuncL	funcL;
			const GLEffect&	glx;
			Visitor(const GLEffect& g): glx(g) {}

			void addPreFunc(PreFunc pf) override {
				funcL.push_back(pf);
			}
			bool setKey(const std::string& key) {
				uniID = GL.glGetUniformLocation(pgID, key.c_str());
				// ここでキーが見つからない = uniformブロックで宣言されているがGLSLコードで使われない場合なのでエラーではない
				// Assert(Warn, uniID>=0, "Uniform argument \"%1%\" not found", key)
				return uniID >= 0;
			}
			template <class T>
			void _addResult(const T& t) {
				if(uniID >= 0)
					glx._makeUniformToken(result, *this, uniID, &t, 1, false);
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
		AssertT(Throw, (selectSh[ShType::VERTEX] && selectSh[ShType::FRAGMENT]), (GLE_LogicalError)(const char*), "no vertex or fragment shader found")

		std::stringstream ss;
		TPSDupl dupl(gs, tech, pass);
		HLSh shP[ShType::NUM_SHTYPE];
		for(int i=0 ; i<countof(selectSh) ; i++) {
			auto* shp = selectSh[i];
			if(!shp)
				continue;
			Assert(Trap, gs.shM.count(shp->shName)==1)
			const ShStruct& s = gs.shM.at(shp->shName);
			// シェーダーバージョンを出力
			ss << "#version " << s.version_str << std::endl;
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
			shP[i] = mgr_gl.makeShader(static_cast<ShType>(i), ss.str());

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
		_defaultValue.clear();
	}
	void TPStructR::ts_onDeviceReset(const GLEffect& glx) {
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
			AssertT(Throw, atID==-2, (GLE_LogicalError)(const std::string&), (boost::format("duplication of vertex semantics \"%1% : %2%\"") % p->name % GLSem_::cs_typeStr[p->sem]).str())
			auto at = prog.getAttribID(p->name.c_str());
			atID = (at) ? *at : -1;
			// -1の場合は警告を出す(もしかしたらシェーダー内で使ってないだけかもしれない)
		}

		// Uniform変数にデフォルト値がセットしてある物をリストアップ
		Visitor visitor(glx);
		visitor.pgID = _prog.cref()->getProgramID();
		for(const auto* p : _unifL) {
			if(visitor.setKey(p->name)) {
				if(p->defStr) {
					// 変数名をIDに変換
					boost::apply_visitor(visitor, *p->defStr);
				} else
					_noDefValue.insert(visitor.uniID);
			}
		}
		_defaultValue.swap(visitor.result);
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
	const UniMap& TPStructR::getUniformDefault() const { return _defaultValue; }
	const UniIDSet& TPStructR::getUniformEntries() const { return _noDefValue; }
	const HLProg& TPStructR::getProgram() const { return _prog; }
	TPStructR::VAttrID TPStructR::getVAttrID() const { return _vAttrID; }
	const PreFuncL& TPStructR::getPreFunc() const { return _preFuncL; }
}
