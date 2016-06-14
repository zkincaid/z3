/*++
Module Name:

    str_decl_plugin.h

Abstract:

    <abstract>

Author:

    Murphy Berzish (mtrberzi) 2015-09-02.

Revision History:

--*/
#include<sstream>
#include"str_decl_plugin.h"
#include"string_buffer.h"
#include"warning.h"
#include"ast_pp.h"
#include"ast_smt2_pp.h"

str_decl_plugin::str_decl_plugin():
    m_strv_sym("String"),
    m_str_decl(0),
    m_concat_decl(0),
    m_length_decl(0),
    m_charat_decl(0),
    m_startswith_decl(0),
    m_arith_plugin(0),
    m_arith_fid(0),
    m_int_sort(0){
}

str_decl_plugin::~str_decl_plugin(){
}

void str_decl_plugin::finalize(void) {
    #define DEC_REF(decl) if (decl) { m_manager->dec_ref(decl); } ((void) 0)
    DEC_REF(m_str_decl);
    DEC_REF(m_concat_decl);
    DEC_REF(m_length_decl);
    DEC_REF(m_charat_decl);
    DEC_REF(m_startswith_decl);
    DEC_REF(m_int_sort);
}

void str_decl_plugin::set_manager(ast_manager * m, family_id id) {
    decl_plugin::set_manager(m, id);
    m_str_decl = m->mk_sort(symbol("String"), sort_info(id, STRING_SORT));
    m->inc_ref(m_str_decl);
    sort * s = m_str_decl;

    SASSERT(m_manager->has_plugin(symbol("arith")));
    m_arith_fid = m_manager->mk_family_id("arith");
    m_arith_plugin = static_cast<arith_decl_plugin*>(m_manager->get_plugin(m_arith_fid));
    SASSERT(m_arith_plugin);

    m_int_sort = m_manager->mk_sort(m_arith_fid, INT_SORT);
    SASSERT(m_int_sort != 0); // arith_decl_plugin must be installed before str_decl_plugin.
    m_manager->inc_ref(m_int_sort);
    sort * i = m_int_sort;

    sort* boolT = m_manager->mk_bool_sort();

#define MK_OP(FIELD, NAME, KIND, SORT)                                                  \
    FIELD = m->mk_func_decl(symbol(NAME), SORT, SORT, SORT, func_decl_info(id, KIND));  \
    m->inc_ref(FIELD)

    MK_OP(m_concat_decl, "Concat", OP_STRCAT, s);

    m_length_decl = m->mk_func_decl(symbol("Length"), s, i, func_decl_info(id, OP_STRLEN));
    m_manager->inc_ref(m_length_decl);

    m_charat_decl = m->mk_func_decl(symbol("CharAt"), s, i, s, func_decl_info(id, OP_STR_CHARAT));
    m_manager->inc_ref(m_charat_decl);

    m_startswith_decl = m->mk_func_decl(symbol("StartsWith"), s, s, boolT, func_decl_info(id, OP_STR_STARTSWITH));
    m_manager->inc_ref(m_startswith_decl);
}

decl_plugin * str_decl_plugin::mk_fresh() {
    return alloc(str_decl_plugin);
}

sort * str_decl_plugin::mk_sort(decl_kind k, unsigned num_parameters, parameter const * parameters) {
    switch (k) {
    case STRING_SORT: return m_str_decl;
    default: return 0;
    }
}

func_decl * str_decl_plugin::mk_func_decl(decl_kind k) {
    switch(k) {
    case OP_STRCAT: return m_concat_decl;
    case OP_STRLEN: return m_length_decl;
    case OP_STR_CHARAT: return m_charat_decl;
    case OP_STR_STARTSWITH: return m_startswith_decl;
    default: return 0;
    }
}

func_decl * str_decl_plugin::mk_func_decl(decl_kind k, unsigned num_parameters, parameter const * parameters,
                                         unsigned arity, sort * const * domain, sort * range) {
    if (k == OP_STR) {
        m_manager->raise_exception("OP_STR not yet implemented in mk_func_decl!");
        return 0;
    }
    if (arity == 0) {
        m_manager->raise_exception("no arguments supplied to string operator");
        return 0;
    }
    return mk_func_decl(k);
}

app * str_decl_plugin::mk_string(std::string & val) {
	std::map<std::string, app*>::iterator it = string_cache.find(val);
	//if (it == string_cache.end()) {
	if (true) {
		char * new_buffer = alloc_svect(char, (val.length() + 1));
		strcpy(new_buffer, val.c_str());
		parameter p[1] = {parameter(new_buffer)};
		func_decl * d;
		d = m_manager->mk_const_decl(m_strv_sym, m_str_decl, func_decl_info(m_family_id, OP_STR, 1, p));
		app * str = m_manager->mk_const(d);
		string_cache[val] = str;
		return str;
	} else {
		return it->second;
	}
}

app * str_decl_plugin::mk_string(const char * val) {
	std::string key(val);
	return mk_string(key);
}

app * str_decl_plugin::mk_fresh_string() {
    // cheating.
    // take the longest string in the cache, append the letter "A", and call it fresh.
    std::string longestString = "";
    std::map<std::string, app*>::iterator it = string_cache.begin();
    for (; it != string_cache.end(); ++it) {
        if (it->first.length() > longestString.length()) {
            longestString = it->first;
        }
    }
    longestString += "A";
    return mk_string(longestString);
}

void str_decl_plugin::get_op_names(svector<builtin_name> & op_names, symbol const & logic) {
    op_names.push_back(builtin_name("Concat", OP_STRCAT));
    op_names.push_back(builtin_name("Length", OP_STRLEN));
    op_names.push_back(builtin_name("CharAt", OP_STR_CHARAT));
    op_names.push_back(builtin_name("StartsWith", OP_STR_STARTSWITH));
}

void str_decl_plugin::get_sort_names(svector<builtin_name> & sort_names, symbol const & logic) {
    sort_names.push_back(builtin_name("String", STRING_SORT));
}

bool str_decl_plugin::is_value(app * e) const {
    if (e->get_family_id() != m_family_id) {
        return false;
    }
    switch (e->get_decl_kind()) {
    case OP_STR:
        return true;
    default:
        return false;
    }
}

bool str_recognizers::is_string(expr const * n, const char ** val) const {
    if (!is_app_of(n, m_afid, OP_STR))
            return false;
    func_decl * decl = to_app(n)->get_decl();
    *val    = decl->get_parameter(0).get_string();
    return true;
}

bool str_recognizers::is_string(expr const * n) const {
    const char * tmp = 0;
    return is_string(n, & tmp);
}

std::string str_recognizers::get_string_constant_value(expr const *n) const {
    const char * cstr = 0;
    bool isString = is_string(n, & cstr);
    SASSERT(isString);
    std::string strval(cstr);
    return strval;
}

str_util::str_util(ast_manager &m) :
    str_recognizers(m.mk_family_id(symbol("str"))),
    m_manager(m) {
    SASSERT(m.has_plugin(symbol("str")));
    m_plugin = static_cast<str_decl_plugin*>(m.get_plugin(m.mk_family_id(symbol("str"))));
}
