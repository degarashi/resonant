#pragma once
#include "glresource.hpp"
#include "glx_id.hpp"
#include "differential.hpp"

namespace rs {
	using OPGLint = spn::Optional<GLint>;
	class SystemUniform2D;
	class SystemUniform3D;
	struct IEffect : IGLResource {
		struct tagConstant {};
		using GlxId = rs::IdMgr_Glx<tagConstant>;
		//! Uniform & TechPass 定数にIdを割り当てるクラス
		static GlxId	s_myId;

		virtual OPGLint getTechId(const std::string& tech) const = 0;
		virtual OPGLint getPassId(const std::string& pass) const = 0;
		virtual OPGLint getPassId(const std::string& tech, const std::string& pass) const = 0;
		virtual OPGLint getCurTechId() const = 0;
		virtual OPGLint getCurPassId() const = 0;
		virtual void setTechPassId(IdValue id) = 0;
		virtual HLProg getProgram(int techId=-1, int passId=-1) const = 0;
		virtual void setTechnique(int id, bool bReset) = 0;
		virtual void setPass(int id) = 0;
		virtual void setFramebuffer(HFb fb) = 0;
		virtual HFb getFramebuffer() const = 0;
		virtual void setViewport(bool bPixel, const spn::RectF& r) = 0;
		virtual void resetFramebuffer() = 0;
		virtual void setVDecl(const SPVDecl& decl) = 0;
		virtual void setVStream(HVb vb, int n) = 0;
		virtual void setIStream(HIb ib) = 0;
		virtual OPGLint getUniformID(const std::string& name) const = 0;
		virtual OPGLint getUnifId(IdValue id) const = 0;
		virtual void clearFramebuffer(const draw::ClearParam& param) = 0;
		virtual void drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem=0) = 0;
		virtual void draw(GLenum mode, GLint first, GLsizei count) = 0;
		virtual void clearTask() = 0;
		virtual void beginTask() = 0;
		virtual void endTask() = 0;
		virtual void execTask() = 0;
		virtual diff::Effect getDifference() const = 0;

		virtual draw::TokenBuffer& _makeUniformTokenBuffer(GLint id) = 0;
		template <class UT, class... Ts>
		static void MakeUniformToken(draw::TokenDst& dst, GLint /*id*/, Ts&&... ts) {
			new(dst.allocate_memory(sizeof(UT), draw::CalcTokenOffset<UT>())) UT(std::forward<Ts>(ts)...);
		}
		//! 定数値を使ったUniform変数設定。Uniform値が存在しなくてもエラーにならない
		template <bool Check, class... Ts>
		void _setUniformById(IdValue id, Ts&&... ts) {
			if(auto idv = getUnifId(id))
				setUniform(*idv, std::forward<Ts>(ts)...);
			else {
				// 定数値に対応するUniform変数が見つからない時は警告を出す
				Assert(Warn, !Check, "Uniform-ConstantId: %1% not found", id.value)
			}
		}

		//! 定数値を使ったUniform変数設定
		/*! clang補完の為の引数明示 -> _setUniform(...) */
		template <bool Check=true, class T>
		void setUniform(IdValue id, T&& t, bool bT=false) {
			_setUniformById<Check>(id, std::forward<T>(t), bT); }
		//! 単体Uniform変数セット
		template <class T, class = typename std::enable_if< !std::is_pointer<T>::value >::type>
		void setUniform(GLint id, const T& t, bool bT=false) {
			setUniform(id, &t, 1, bT); }
		//! 配列Uniform変数セット
		template <class T>
		void setUniform(GLint id, const T* t, int n, bool bT=false) {
			_makeUniformToken(_makeUniformTokenBuffer(id), id, t, n, bT); }
		// clang補完の為の引数明示 -> _setUniform(...)
		template <bool Check=true, class T>
		void setUniform(IdValue id, const T* t, int n, bool bT=false) {
			_setUniformById<Check>(id, t, n, bT); }
		//! ベクトルUniform変数
		template <int DN, bool A>
		void _makeUniformToken(draw::TokenDst& dst, GLint id, const spn::VecT<DN,A>* v, int n, bool) const {
			MakeUniformToken<draw::Unif_Vec<float, DN>>(dst, id, id, v, n); }
		//! 行列Uniform変数(非正方形)
		template <int DM, int DN, bool A>
		void _makeUniformToken(draw::TokenDst& dst, GLint id, const spn::MatT<DM,DN,A>* m, int n, bool bT) const {
			constexpr int DIM = spn::TValue<DM,DN>::great;
			std::vector<spn::MatT<DIM,DIM,false>> tm(n);
			for(int i=0 ; i<n ; i++)
				m[i].convert(tm[i]);
			_makeUniformToken(dst, id, tm.data(), n, bT);
		}
		//! 行列Uniform変数(正方形)
		template <int DN, bool A>
		void _makeUniformToken(draw::TokenDst& dst, GLint id, const spn::MatT<DN,DN,A>* m, int n, bool bT) const {
			MakeUniformToken<draw::Unif_Mat<float, DN>>(dst, id, id, m, n, bT); }

		virtual void _makeUniformToken(draw::TokenDst& dst, GLint id, const bool* b, int n, bool) const = 0;
		virtual void _makeUniformToken(draw::TokenDst& dst, GLint id, const float* fv, int n, bool) const = 0;
		virtual void _makeUniformToken(draw::TokenDst& dst, GLint id, const double* fv, int n, bool) const = 0;
		virtual void _makeUniformToken(draw::TokenDst& dst, GLint id, const int* iv, int n, bool) const = 0;
		virtual void _makeUniformToken(draw::TokenDst& dst, GLint id, const HTex* hTex, int n, bool) const = 0;
		virtual void _makeUniformToken(draw::TokenDst& dst, GLint id, const HLTex* hlTex, int n, bool) const = 0;

		virtual SystemUniform2D& ref2D() { AssertF(Trap, "this class has no SystemUniform2D interface") }
		virtual SystemUniform3D& ref3D() { AssertF(Trap, "this class has no SystemUniform3D interface") }
	};
}
