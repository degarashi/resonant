#pragma once
#include "test/tweak/common.hpp"
#include "spinner/structure/treenode.hpp"
#include "../../luaw.hpp"
#include <string>

namespace rs {
	struct CCoreID;
}
namespace tweak {
	using Name = std::string;
	//! Tweakノード
	//! 複数の子ノードを持てる
	class INode : public spn::TreeNode<INode>, public IBase {
		protected:
			Name			_name;
			rs::HLText		_text;
		public:
			INode(rs::CCoreID cid, const Name& name);
			virtual bool expand(bool b) = 0;
			virtual bool isExpanded() const = 0;
			virtual bool isNode() const = 0;
			virtual void setPointer(Value_UP v) = 0;
			virtual std::ostream& write(std::ostream& s) const = 0;
			virtual void drawInfo(const Vec2& offset, const Vec2& unit, Drawer& d) const = 0;
			void sortAlphabetically();
			const Name& getName() const;
	};
	class Node : public INode {
		private:
			bool		_expanded;
			rs::HLText	_mark[2];
		public:
			Node(rs::CCoreID cid, const Name& name);
			bool expand(bool b) override;
			bool isExpanded() const override;
			bool isNode() const override;
			void setPointer(Value_UP v) override;
			std::ostream& write(std::ostream& s) const override;
			void drawInfo(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
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
			Value_UP			_value;
			Define_SP			_def;
			void _applyValue();
		public:
			Entry(rs::CCoreID cid, const Name& name, spn::WHandle target, const Define_SP& def);
			bool expand(bool b) override;
			bool isExpanded() const override;
			bool isNode() const override;
			void increment(float inc, int index) override;
			int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			void set(const LValueS& v, bool bStep) override;
			void setPointer(Value_UP v) override;
			std::ostream& write(std::ostream& s) const override;
			void drawInfo(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			rs::LCValue get() const override;
	};
	using Entry_SP = std::shared_ptr<Entry>;
}
