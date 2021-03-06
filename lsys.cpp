#include "lsys.hpp"
#include "adaptsdl.hpp"
#include "spinner/emplace.hpp"

namespace rs {
	using spn::SHandle;
	using spn::LHandle;
	// ------------------- LSysFunc::Block -------------------
	LSysFunc::Block::Block(): current(0), result(new LCTable()) {}
	LSysFunc::Block::Block(Block&& blk):
		uriVec(std::move(blk.uriVec)),
		current(blk.current),
		result(std::move(blk.result)) {}
	// ------------------- LSysFunc -------------------
	LSysFunc::LSysFunc(): _serialCur(0), _procCur(0), _progress(0), _state(State::Idle), _bLoop(true), _objIdCursor(0) {
		_thread.start(*this);
	}
	LSysFunc::~LSysFunc() {
		// スレッドの終了を待つ
		_mutex.lock();
			_bLoop = false;
			_cond.signal_all();
		_mutex.unlock();
		_thread.join();
	}
	LSysFunc::LoadThread::LoadThread(): Thread("LoadThread") {}
	void LSysFunc::LoadThread::run(LSysFunc& self) {
		UniLock lk(self._mutex);
		while(self._bLoop) {
			// unordered_mapなのでコンテナのメモリ位置は変わらない)
			auto itr = self._async.find(self._procCur);
			if(itr != self._async.end()) {
				Block& blk = itr->second;
				self._state = State::Processing;
				for(;;) {
					// リソースを1つ読み込む度に進捗を書き込む
					auto& cur = blk.uriVec[blk.current];
					// 並列処理のため、一旦ロックを解除
					lk.unlock();
					spn::TryEmplace(*blk.result, cur.first, spn::ResMgrBase::LoadResource(cur.second));
					lk.lock();
					self._progress = ++blk.current / static_cast<float>(blk.uriVec.size());
					if(blk.current == static_cast<int>(blk.uriVec.size()))
						break;
				}
				self._progress = 0;
				++self._procCur;
				self._state = State::Idle;
			}
			else {
				// 処理するタスクが無くなったらスリープ
				self._cond.wait(lk);
			}
		}
	}

	// 呼び出しスレッドで読み込む
	LHandle LSysFunc::loadResource(const std::string& urisrc) {
		spn::URI uri(urisrc);
		return spn::ResMgrBase::LoadResource(uri);
	}
	// 呼び出しスレッドで読み込む
	LCTable LSysFunc::loadResources(LValueG tbl) {
		// tblには{ resourceName = "uri://path/to/resource", ... } というフォーマットでURIが記載されている
		LCTable ret;
		tbl.iterateTable([&ret](LuaState& lsc) {
			LCValue ent = lsc.toLCValue(-2);
			spn::URI uri(lsc.toString(-1));
			auto h = spn::ResMgrBase::LoadResource(uri);
			LCValue lcv(h);
			spn::TryEmplace(ret, std::move(ent), h);
		});
		return ret;
	}
	// 非同期ロードスレッドで読み込む
	lua_Integer LSysFunc::loadResourcesASync(LValueG tbl) {
		UniLock lk(_mutex);
		auto id = _serialCur++;
		Block block;
		tbl.iterateTable([&block](LuaState& lsc) {
			block.uriVec.emplace_back(lsc.toString(-2), spn::URI(lsc.toString(-1)));
		});
		spn::TryEmplace(_async, id, std::move(block));
		_cond.signal_all();
		return id;
	}
	LCValue LSysFunc::getResult(lua_Integer id) {
		UniLock lk(_mutex);
		if(id >= _procCur + static_cast<int>(_state)) {
			// まだ処理が完了していないタスク番号
			return LuaNil();
		}
		auto itr = _async.find(id);
		if(itr == _async.end()) {
			// 該当タスク番号の結果は既に取得され、破棄済み
			return LuaNil();
		}
		LCValue ret(std::move(itr->second.result));
		// エントリを削除
		_async.erase(itr);
		return ret;
	}
	lua_Number LSysFunc::queryProgress(lua_Integer id) {
		UniLock lk(_mutex);
		if(id > _procCur) {
			// まだ処理されていない
			return 0.f;
		}
		if(id < _procCur) {
			// 処理済み
			return 1.f;
		}
		// 現在の進捗を返す
		return _progress;
	}
	int LSysFunc::getNTask() {
		UniLock lk(_mutex);
		return static_cast<int>(_serialCur - _procCur);
	}
	void LSysFunc::sleep(lua_Integer ms) const {
		SDL_Delay(ms);
	}
	ObjTypeId LSysFunc::addObjectId(const std::string& name) {
		Assert(Trap, _nameMap.count(name)==0, "registered same object(%1%) twice", name)
		return _nameMap[name] = ++_objIdCursor;
	}
	ObjTypeId_OP LSysFunc::getObjectId(const std::string& name) const {
		auto itr = _nameMap.find(name);
		if(itr != _nameMap.end())
			return itr->second;
		return spn::none;
	}
}
#include "apppath.hpp"
namespace rs {
	void LSysFunc::loadClass(const std::string& name, const SPLua& ls) {
		// オブジェクトIdの生成
		if(_nameMap.count(name) == 0)
			_nameMap[name] = ++_objIdCursor;

		std::string fileName(name);
		fileName.append(".");
		fileName.append(luaNS::ScriptExtension);
		HLRW hlRW = mgr_path.getRW(luaNS::ScriptResourceEntry, fileName, RWops::Read, nullptr);
		Assert(Trap, hlRW, "script file \"%1%\" not found.", fileName)
		LuaImport::LoadClass(*ls, name, hlRW);
	}
}
