#include "drawtask.hpp"
#include "glhead.hpp"
#include "glx.hpp"

namespace rs {
	namespace draw {
		// -------------- Task --------------
		Task::Task(): _curWrite(0), _curRead(0) {}
		TokenML& Task::refWriteEnt() {
			UniLock lk(_mutex);
			return _entry[_curWrite % NUM_ENTRY];
		}
		TokenML& Task::refReadEnt() {
			UniLock lk(_mutex);
			return _entry[_curRead % NUM_ENTRY];
		}
		void Task::beginTask(HFx hFx) {
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
			_hlFx[_curWrite % NUM_ENTRY] = hFx;
			we.clear();
		}
		void Task::endTask() {
			GL.glFlush();
			UniLock lk(_mutex);
			++_curWrite;
		}
		void Task::clear() {
			UniLock lk(_mutex);
			_curWrite = _curRead+1;
			GL.glFinish();
			for(auto& e : _entry)
				e.clear();
			for(auto& h : _hlFx)
				h.release();
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
				readent.exec();
				GL.glFlush();
				lk = spn::construct(std::ref(_mutex));
				++_curRead;
				_cond.signal();
			}
		}
	}
}
