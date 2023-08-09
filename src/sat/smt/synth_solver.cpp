/*++
Copyright (c) 2020 Microsoft Corporation

Module Name:

    synth_solver.h

Author:

    Petra Hozzova 2023-08-08
    Nikolaj Bjorner (nbjorner) 2020-08-17

--*/

#include "ast/synth_decl_plugin.h"
#include "sat/smt/synth_solver.h"
#include "sat/smt/euf_solver.h"

namespace synth {

    solver::solver(euf::solver& ctx):
        th_euf_solver(ctx, symbol("synth"), ctx.get_manager().mk_family_id("synth")) {}
        
    solver::~solver() {}


    // recognize synthesis objective as part of search objective.
    // register it for calls to check.
    void solver::asserted(sat::literal lit) {

    }

    bool solver::contains_uncomputable(expr* e) {
	return false;
    }

    bool solver::synthesize(app* e) {

        if (e->get_num_args() == 0)
            return false;
        
        auto * n = expr2enode(e->get_arg(0));
        expr_ref_vector repr(m);
	euf::enode_vector todo;
        auto has_rep = [&](euf::enode* n) { return !!repr.get(n->get_root_id(), nullptr); };
        auto get_rep = [&](euf::enode* n) { return repr.get(n->get_root_id(), nullptr); };
        for (unsigned i = 1; i < e->get_num_args(); ++i) {
	    expr * arg = e->get_arg(i);
	    auto * narg = expr2enode(arg);
            todo.push_back(narg);
	    repr.setx(narg->get_root_id(), arg);
        }
	for (unsigned i = 0; i < todo.size() && !has_rep(n); ++i) {
	    auto * nn = todo[i];
            for (auto * p : euf::enode_parents(nn)) {
	        if (has_rep(p))
                    continue;
                if (all_of(euf::enode_args(p), [&](auto * ch) { return has_rep(ch); })) {
                    ptr_buffer<expr> args;
	            for (auto * ch : euf::enode_args(p))
                        args.push_back(get_rep(ch));
                    app * papp = m.mk_app(p->get_decl(), args);
	            repr.setx(p->get_root_id(), papp);
		    todo.push_back(p);
		}
	    }
        }
	expr * sol = get_rep(n);
        if (!sol)
            return false;

        sat::literal lit = eq_internalize(n->get_expr(), sol);
        add_unit(~lit);
        IF_VERBOSE(0, verbose_stream() << mk_pp(sol, m) << "\n");
        return true;
    }

    // block current model using realizer by E-graph (and arithmetic)
    // 
    sat::check_result solver::check() {
    for (app* e : m_synth)
        if (synthesize(e))
               sat::check_result::CR_CONTINUE; 
        return sat::check_result::CR_DONE;
    }

    // nothing particular to do
    void solver::push_core() {

    }

    // nothing particular to do
    void solver::pop_core(unsigned n) {
    }

    // nothing particular to do
    bool solver::unit_propagate() {
        return false;
    }

    // retrieve explanation for assertions made by synth solver. It only asserts unit literals so nothing to retrieve
    void solver::get_antecedents(sat::literal l, sat::ext_justification_idx idx, sat::literal_vector & r, bool probing) {
    }

    // where we collect statistics (number of invocations?)
    void solver::collect_statistics(statistics& st) const {}

    void solver::add_uncomputable(app* e) {
        for (unsigned i = 0; i < e->get_num_args(); ++i) {
	    expr* a = e->get_arg(i);
	    if (is_app(a)) {
		func_decl* f = to_app(a)->get_decl();
		m_uncomputable.insert(f);
		ctx.push(insert_obj_trail(m_uncomputable, f));
            }
	}
    }

    // recognize synthesis objectives here.
    sat::literal solver::internalize(expr* e, bool sign, bool root) {
        internalize(e);
	sat::literal lit = ctx.expr2literal(e);
	if (sign)
	    lit.neg();
        return lit;
    }

    // recognize synthesis objectives here and above
    void solver::internalize(expr* e) {
        SASSERT(is_app(e));
	sat::bool_var bv = ctx.get_si().add_bool_var(e);
        sat::literal lit(bv, false);
	ctx.attach_lit(lit, e);
        synth::util util(m);
        if (util.is_synthesiz3(e))
            ctx.push_vec(m_synth, to_app(e));
        if (util.is_grammar(e))
	    add_uncomputable(to_app(e));
    }

    // display current state (eg. current set of realizers)
    std::ostream& solver::display(std::ostream& out) const {
        return out;
    }

    // justified by "synth".
    std::ostream& solver::display_justification(std::ostream& out, sat::ext_justification_idx idx) const {
        return out << "synth";
    }
    
    std::ostream& solver::display_constraint(std::ostream& out, sat::ext_constraint_idx idx) const {
        return out << "synth";
    }

    // create a clone of the solver.
    euf::th_solver* solver::clone(euf::solver& ctx) {
        NOT_IMPLEMENTED_YET();
        return nullptr;
    }

}
