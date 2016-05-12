/**
 * This file describes the Encoder class. This class
 * is used to create encodings of a PDDL domain and 
 * problem pair.
 */
#include <sstream>
#include <string>
#include <cstdio>
#include <iostream>
#include <vector>

#include "z3++.h"

#include "ptree.h"
#include "instantiation.h"
#include "VisitController.h"
#include "FastEnvironment.h"
#include "TIM.h"

#include "SMTPlan/PlannerOptions.h"
#include "SMTPlan/ProblemInfo.h"
#include "SMTPlan/Algebraist.h"

#ifndef KCL_encoder
#define KCL_encoder

namespace SMTPlan
{
	enum EncState
	{
		ENC_NONE,
		ENC_INIT,
		ENC_GOAL,
		ENC_LITERAL,
		ENC_ACTION_CONDITION,
		ENC_ACTION_DURATION,
		ENC_ACTION_EFFECT
	};

	class Encoder : public VAL::VisitController
	{
	private:

		/* encoding info */
		int upper_bound;
		int next_layer;
		std::vector<z3::expr> goal_expression;

		/* problem info */
		PlannerOptions * opt;
		ProblemInfo * problem_info;
		VAL::FastEnvironment * fe;
		VAL::analysis * val_analysis;
		Algebraist * algebraist;

		/* encoding state */
		EncState enc_state;
		int enc_expression_h;
		std::vector<z3::expr> enc_expression_stack;
		std::string enc_function_symbol;

		/* encoding information */
		std::vector<std::vector<int> > simpleStartAddEffects;
		std::vector<std::vector<int> > simpleStartDelEffects;
		std::vector<std::vector<int> > simpleEndAddEffects;
		std::vector<std::vector<int> > simpleEndDelEffects;
		std::map<int, std::vector<std::pair<int, z3::expr> > > simpleStartAssignEffects;
		std::map<int, std::vector<std::pair<int, z3::expr> > > simpleEndAssignEffects;
		std::vector<bool> initialState;

		/* SMT variables */
		std::vector<z3::expr> time_vars;
		std::vector<z3::expr> duration_vars;
		std::vector<std::vector<z3::expr> > pre_function_vars;
		std::vector<std::vector<z3::expr> > pos_function_vars;
		std::vector<std::vector<z3::expr> > pre_literal_vars;
		std::vector<std::vector<z3::expr> > pos_literal_vars;
		std::vector<std::vector<z3::expr> > sta_action_vars;
		std::vector<std::vector<z3::expr> > end_action_vars;
		std::vector<std::vector<z3::expr> > dur_action_vars;
		std::vector<std::vector<z3::expr> > run_action_vars;

		/* encoding methods */
		void encodeHeader(int H);
		void encodeTimings(int H);
		void encodeLiteralVariableSupport(int H);
		void encodeFunctionVariableSupport(int H);
		void encodeFunctionFlows(int H);
		void encodeGoalState(int H);
		void encodeInitialState();

		void parseExpression(VAL::expression * e);

		/* internal encoding methods */
		z3::expr mk_or(z3::expr_vector args) {
			std::vector<Z3_ast> array;
			for (unsigned i = 0; i < args.size(); i++) array.push_back(args[i]);
			return z3::to_expr(args.ctx(), Z3_mk_or(args.ctx(), array.size(), &(array[0])));
		}

		z3::expr mk_and(z3::expr_vector args) {
			std::vector<Z3_ast> array;
			for (unsigned i = 0; i < args.size(); i++) array.push_back(args[i]);
			return z3::to_expr(args.ctx(), Z3_mk_and(args.ctx(), array.size(), &(array[0])));
		}

	public:

		Encoder(Algebraist * alg, VAL::analysis* analysis, PlannerOptions &options, ProblemInfo &pi)
		{
			next_layer = 0;

			problem_info = &pi;
			val_analysis = analysis;
			algebraist = alg;
			const int pneCount = Inst::instantiatedOp::howManyPNEs();
			const int litCount = Inst::instantiatedOp::howManyLiterals();
			const int actCount = Inst::instantiatedOp::howMany();

			simpleStartAddEffects = std::vector<std::vector<int> >(litCount);
			simpleStartDelEffects = std::vector<std::vector<int> >(litCount);
			simpleEndAddEffects = std::vector<std::vector<int> >(litCount);
			simpleEndDelEffects = std::vector<std::vector<int> >(litCount);

			initialState = std::vector<bool>(litCount);

			pre_function_vars = std::vector<std::vector<z3::expr> >(pneCount);
			pos_function_vars = std::vector<std::vector<z3::expr> >(pneCount);
			pre_literal_vars = std::vector<std::vector<z3::expr> >(litCount);
			pos_literal_vars = std::vector<std::vector<z3::expr> >(litCount);
			sta_action_vars = std::vector<std::vector<z3::expr> >(actCount);
			end_action_vars = std::vector<std::vector<z3::expr> >(actCount);
			run_action_vars = std::vector<std::vector<z3::expr> >(actCount);
			dur_action_vars = std::vector<std::vector<z3::expr> >(actCount);

			z3::config cfg;
			cfg.set("auto_config", true);
			z3_context = new z3::context(cfg);
			z3_tactic = new z3::tactic(*z3_context, "qfnra-nlsat");
			z3_solver = new z3::solver(*z3_context, z3_tactic->mk_solver());
		}

		/* encoding methods */
		bool encode(int H);

		/* visitor methods */
		virtual void visit_action(VAL::action * o);
		virtual void visit_durative_action(VAL::durative_action * da);

		virtual void visit_simple_goal(VAL::simple_goal *);
		virtual void visit_qfied_goal(VAL::qfied_goal *);
		virtual void visit_conj_goal(VAL::conj_goal *);
		virtual void visit_disj_goal(VAL::disj_goal *);
		virtual void visit_timed_goal(VAL::timed_goal *);
		virtual void visit_imply_goal(VAL::imply_goal *);
		virtual void visit_neg_goal(VAL::neg_goal *);

		virtual void visit_assignment(VAL::assignment * e);
		virtual void visit_simple_effect(VAL::simple_effect * e);
		virtual void visit_forall_effect(VAL::forall_effect * e);
		virtual void visit_cond_effect(VAL::cond_effect * e);
		virtual void visit_timed_effect(VAL::timed_effect * e);
		virtual void visit_timed_initial_literal(VAL::timed_initial_literal * til);
		virtual void visit_effect_lists(VAL::effect_lists * e);

		virtual void visit_comparison(VAL::comparison *);
		virtual void visit_plus_expression(VAL::plus_expression * s);
		virtual void visit_minus_expression(VAL::minus_expression * s);
		virtual void visit_mul_expression(VAL::mul_expression * s);
		virtual void visit_div_expression(VAL::div_expression * s);
		virtual void visit_uminus_expression(VAL::uminus_expression * s);
		virtual void visit_int_expression(VAL::int_expression * s);
		virtual void visit_float_expression(VAL::float_expression * s);
		virtual void visit_special_val_expr(VAL::special_val_expr * s);
		virtual void visit_func_term(VAL::func_term * s);

		virtual void visit_preference(VAL::preference *);
		virtual void visit_event(VAL::event * e);
		virtual void visit_process(VAL::process * p);
		virtual void visit_derivation_rule(VAL::derivation_rule * o);

		/* solving */
		z3::context * z3_context;
		z3::tactic * z3_tactic;
		z3::solver * z3_solver;
		z3::check_result solve();
		void printModel();
	};

} // close namespace

#endif

