#pragma once
#include "test/tweak/common.hpp"
#include "spinner/structure/treenode.hpp"
#include "../../luaw.hpp"
#include <string>

namespace rs {
	struct CCoreID;
}
namespace tweak {
	struct STextPack;
	using Name = std::string;
	//! Tweakノード
	//! 複数の子ノードを持てる
	class INode : public spn::TreeNode<INode>, public IBase {
		public:
			friend class spn::TreeNode<INode>;
			using node_t = spn::TreeNode<INode>;
			using Size = int;
		private:
			static void OnChildRemove(const node_t* self, const node_t::SP& node);
			static void OnChildAdded(const node_t* self, const node_t::SP& node);
		protected:
			Name			_name;
			rs::HLText		_text;
			virtual void _setRefreshSize() const = 0;
			virtual Size _getThisSize() const = 0;
			virtual node_t::SP _rewindRange(Size& size) const = 0;
		public:
			virtual Size _getCachedSize() const = 0;
			INode(rs::CCoreID cid, const Name& name);
			virtual bool expand(bool b) = 0;
			virtual bool isExpanded() const = 0;
			virtual bool isNode() const = 0;
			virtual void setPointer(Value_UP v) = 0;
			virtual spn::SizeF drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const = 0;
			virtual std::ostream& write(std::ostream& s) const = 0;
			void sortAlphabetically();
			const Name& getName() const;
			/*!
				\param[in]	size	残り幅
				\return 該当するノードが見つかればそのポインタ、なければnull
			*/
			node_t::SP rewindRange(Size& size, bool bChk=false) const;
	};
	class Node : public INode {
		private:
			bool			_expanded;
			rs::HLText		_mark[2];
			mutable Size	_cached_size;
			mutable bool	_bRefl;
		protected:
			void _setRefreshSize() const override;
			Size _getThisSize() const override;
			node_t::SP _rewindRange(Size& size) const override;
		public:
			Size _getCachedSize() const override;
			Node(rs::CCoreID cid, const Name& name);
			bool expand(bool b) override;
			bool isExpanded() const override;
			bool isNode() const override;
			void setPointer(Value_UP v) override;
			std::ostream& write(std::ostream& s) const override;
			int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			spn::SizeF drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const override;
	};

	// 定数パラメータごとに1つ定義
	struct Define {
		Name		name;		// 変数名
		Value_UP	defvalue;	// デフォルト値(値操作の方式)
		LValueG		apply;		// 値をどのように適用するか
	};
	using Define_SP = std::shared_ptr<Define>;
	class Entry : public INode {
		private:
			spn::WHandle		_target;
			Value_UP			_value,
								_initialValue;
			Define_SP			_def;
			void _applyValue();
		protected:
			void _setRefreshSize() const override;
			Size _getThisSize() const override;
			node_t::SP _rewindRange(Size& size) const override;
		public:
			Size _getCachedSize() const override;
			Entry(rs::CCoreID cid, const Name& name, spn::WHandle target, const Define_SP& def);
			bool expand(bool b) override;
			bool isExpanded() const override;
			bool isNode() const override;
			void increment(float inc, int index) override;
			int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			spn::SizeF drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const override;
			void set(const LValueS& v, bool bStep) override;
			void setInitial(const LValueS& v) override;
			void setPointer(Value_UP v) override;
			std::ostream& write(std::ostream& s) const override;
			rs::LCValue get() const override;
	};
	using Entry_SP = std::shared_ptr<Entry>;
}
