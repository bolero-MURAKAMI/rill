//
// Copyright yutopp 2013 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <rill/semantic_analysis/identifier_collector.hpp>
#include <rill/environment.hpp>

#include <rill/ast/root.hpp>
#include <rill/ast/statement.hpp>
#include <rill/ast/expression.hpp>
#include <rill/ast/value.hpp>


namespace rill
{
    namespace semantic_analysis
    {
        // Root Scope
        RILL_TV_OP( identifier_collector, ast::root_ptr, r, env )
        {
            // build environment
            for( auto const& node : r->statements_ )
                dispatch_as_env( node, *this, env );
        }

        //
        RILL_TV_OP( identifier_collector, ast::expression_statement_ptr, s, env )
        {
            // DO NOT COLLECT IDENTIFIERS
        }

        RILL_TV_OP( identifier_collector, ast::function_definition_statement_ptr, s, env )
        {
            // TODO: remove this case in syntax analysis phase
            if ( s->get_identifier()->nest_size() != 1 )
                std::cout << "function_definition_statement error!!!!!!! can not specified nested definition here." << std::endl;//error()

            // TODO: add steady step to check
            //     : OR CHANGE THE PARSER
            assert( s->get_identifier()->nest_size() == 1 ); // can not use nested type here

            std::cout << "collected : " << s->get_identifier()->get_last_identifier()->get_base_symbol()->get_native_string() << std::endl
                      << "param_num : " << s->get_parameter_list().size() << std::endl;

            // add function symbol to current environment
            env->mark_as( kind::function_k, s->get_identifier()->get_last_identifier(), s );
        }

        RILL_TV_OP( identifier_collector, ast::class_definition_statement_ptr, s, env )
        {
            // TODO: implement it
            assert( false );
        }
    } // namespace semantic_analysis
} // namespace rill