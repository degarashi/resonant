#pragma once

#define DEF_SETVAL(fn_name, name) const auto fn_name = [](auto& ctx){ _val(ctx).name = _attr(ctx); };
#define DEF_PUSHVAL(fn_name, name) const auto fn_name = [](auto& ctx){ _val(ctx).name.push_back(_attr(ctx)); };
#define DEF_INSVAL(fn_name, name, name_entry) const auto fn_name = [](auto& ctx){ _val(ctx).name.emplace(_attr(ctx).name_entry, _attr(ctx)); };
