/*
 * ControlPolicy.h
 *
 * Copyright (C) 2017 OpenCog Foundation
 *
 * Authors: Nil Geisweiller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef OPENCOG_CONTROLPOLICY_H_
#define OPENCOG_CONTROLPOLICY_H_

#include <opencog/atomspace/AtomSpace.h>

#include "BIT.h"
#include "../Rule.h"

class ControlPolicyUTest;

namespace opencog
{

//! a map from handles to truth values
typedef std::map<Handle, TruthValuePtr> HandleTVMap;

// Hold RuleTypedSubstitutionPair and double, the probability of
// selecting the rule, which must be passed to the BIT to calculate
// the and-BIT complexity.
typedef std::pair<RuleTypedSubstitutionPair, double> RuleSelection;

class ControlPolicy
{
    friend class ::ControlPolicyUTest;
public:
	ControlPolicy(const RuleSet& rules, const BIT& bit,
	              AtomSpace* control_as=nullptr);
	~ControlPolicy();

	const std::string preproof_predicate_name = "URE:BC:preproof-of";

	// Inference rule set for expanding and-BITs.
	RuleSet rules;

	/**
	 * Select a valid inference rule given a target. The selected is a
	 * new object because a new rule is created, its variables are
	 * uniquely renamed, possibly some partial substitutions are
	 * applied.
	 *
	 * Unless a control_as is provided at construction time, the
	 * Selection is random amongst the valid rules and weighted
	 * according to their weights.
	 *
	 * TODO: add comments about inference control policy, see
	 * <OPENCOG_ROOT>/examples/pln/inference-control-learning/README.md
	 *
	 * The andbit and bitleaf are not const because if the rules are
	 * exhausted it will set its exhausted flag to false.
	 */
	RuleSelection select_rule(AndBIT& andbit, BITNode& bitleaf);

private:
	// Reference to the BackwardChainer BIT
	const BIT& _bit;

	// Map alias rule to their default TV. This is used whenever no
	// control rule can be used to predict inference expansion.
	HandleTVMap _default_tvs;

	// AtomSpace holding the inference control rules (or simply
	// control rules for short).
	//
	// Inference control rules are classified according to the
	// decision of the inference control they affect. For now the
	// following are supported:
	//
	// 1. Expansion Control Rules: for choosing the inference rule to
	//    expand an and-BIT.
	AtomSpace* _control_as;

	// AtomSpace holding the pattern matcher queries to fetch the
	// various control rule
	AtomSpace* _query_as;

	// Map each action (inference rule expansion) to the set of
	// control rules involving it.
	std::map<Handle, HandleSet> _expansion_control_rules;

	/**
	 * Return all valid rules, in the sense that they may possibly be
	 * used to infer the target.
	 */
	RuleTypedSubstitutionMap get_valid_rules(const AndBIT& andbit,
	                                         const BITNode& bitleaf);

	/**
	 * Select a rule for expansion amongst a set of valid ones.
	 */
	RuleSelection select_rule(const AndBIT& andbit,
	                          const BITNode& bitleaf,
	                          const RuleTypedSubstitutionMap& rules);

	/**
	 * Return the conditional TVs that a given rule expands a supposed
	 * preproof into another preproof.
	 */
	HandleTVMap expansion_success_tvs(const AndBIT& andbit,
	                                  const BITNode& bitleaf,
	                                  const RuleTypedSubstitutionMap& rules);

	/**
	 * Calculate the rule weights, according to the control rules
	 * present is _control_as, or otherwise default rule TVs, to do
	 * weighted random selection.
	 */
	std::vector<double> rule_weights(const HandleTVMap& success_tvs,
	                                 const RuleTypedSubstitutionMap& rules);

	/**
	 * Given the weights (action probability) of each inference rule
	 * alias, return the weights over rule instantiations (unified to
	 * the target). Unifying a rule to a target can lead to multiple
	 * rules, each one will have a equal fraction of weight so that
	 * the sum of weights of all unified rules equals the rule alias
	 * weight.
	 *
	 * Later on this might be replaced by performing action selection
	 * on the rules themselves rather than their aliases.
	 */
	std::vector<double> rule_weights(
		const HandleCounter& alias_weights,
		const RuleTypedSubstitutionMap& inf_rules) const;

	/**
	 * Return the set of rule aliases, as aliases of inference rules
	 * are used in control rules.
	 */
	HandleSet rule_aliases(const RuleSet& rules) const;
	HandleSet rule_aliases(const RuleTypedSubstitutionMap& rules) const;

	/**
	 * Return the map from rule aliases to their default weights.
	 */
	HandleCounter default_alias_weights(const RuleTypedSubstitutionMap& rules) const;

	/**
	 * Get all active expansion control rules concerning the given
	 * inference rule.
	 */
	HandleSet active_expansion_control_rules(const Handle& inf_rule_alias);

	/**
	 * Return true iff the given control is current active, that is
	 * the case of an expansion control rule whether the pattern is
	 * true.
	 *
	 * Ultimately this should be replace by a TV because most patterns
	 * will have a certain probability of being true, or some degree
	 * of truth. To do well it should rely on a conditional
	 * instantiation PLN rule.
	 */
	bool control_rule_active(const Handle& ctrl_rule) const;

	/**
	 * Return the pattern in a given expansion control rule, if it has
	 * any.
	 */
	Handle get_expansion_control_rule_pattern(const Handle& ctrl_rule) const;

	/**
	 * Given an inference rule, fetch both pattern and pattern free
	 * expansion control rules. See comments of
	 * fetch_pattern_free_expansion_control_rules and
	 * fetch_pattern_expansion_control_rules
	 */
	HandleSet fetch_expansion_control_rules(const Handle& inf_rule);

	/**
	 * Fetch control rules from _control_as involved in BIT
	 * expansion. Informally that if and-BIT, A, expands into B from L
	 * with the given rule, and follow some pattern, then B has a
	 * probability TV of being a preproof of T. Formally
	 *
	 * ImplicationScope <TV>
	 *  VariableList
	 *    Variable "$T"  ;; Theorem/target to prove
	 *    TypedVariable  ;; and-BIT to expand
	 *      Variable "$A"
	 *      Type "BindLink"
	 *    Variable "$L"  ;; Leaf from A to expand
	 *    TypedVariable  ;; Resulting and-BIT from the expansion of L from A with rule R
	 *      Variable "$B"
	 *      Type "BindLink"
	 *  And
	 *    Execution
	 *      Schema "expand-and-BIT"
	 *      List
	 *        BontExec Variable "$A"
	 *        Variable "$L"
	 *        DontExec <inf_rule>
	 *      DontExec Variable "$B"
	 *    <pattern-1>
	 *    ...
	 *    <pattern-n>
	 *  Evaluation
	 *    Predicate "preproof"
	 *    List
	 *      DontExec Variable "$B"
	 *      Variable "$T"
	 *
	 * n is the number of patterns in addition to the expansion itself
	 * (the Execution link).
	 */
	HandleSet fetch_expansion_control_rules(const Handle& inf_rule, int n);

	/**
	 * Helpers to build various hypergraphs used to build various queries
	 */
	Handle mk_vardecl_vardecl(const Handle& vardecl_var);
	Handle mk_list_of_args_vardecl(const Handle& args_var);
	Handle mk_expand_exec(const Handle& input_andbit_var,
	                      const Handle& input_leaf_var,
	                      const Handle& inf_rule,
	                      const Handle& output_andbit_var);
	Handle mk_preproof_eval(const Handle& preproof_args_var);
	Handle mk_expansion_control_rules_query(const Handle& inf_rule, int n);
	HandleSeq mk_pattern_vars(int n);
	Handle mk_pattern_var(int i);

	/**
	 * Calculate the actual mean of a TV. which is to be constracted
	 * by the mean in the TruthValue class which doesn't correspond to
	 * the actual mean of the second order distribution.
	 *
	 * TODO: replace this by the mean method of the TruthValue once
	 * this class is properly re-implemented.
	 */
	double get_actual_mean(TruthValuePtr tv) const;
};


} // namespace opencog

#endif /* OPENCOG_CONTROLPOLICY_H_ */
