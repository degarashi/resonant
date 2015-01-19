#include "glx.hpp"

namespace rs {
	namespace {
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
	namespace {
		struct Visitor : boost::static_visitor<>, IUserTaskReceiver {
			GLuint		pgID;
			GLint		uniID;
			UniMap		result;
			UserTaskV	funcV;
			const GLEffect&	glx;
			Visitor(const GLEffect& g): glx(g) {}

			void addTask(UserTask t) override {
				funcV.push_back(t);
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
	const UserTaskV& TPStructR::getUserTask() const { return _userTaskV; }
}

