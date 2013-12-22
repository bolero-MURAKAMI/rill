//
// Copyright yutopp 2013 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <vector>
#include <string>

#include <boost/optional.hpp>

#include "../environment/environment_fwd.hpp"

#include "detail/tree_visitor_base.hpp"
#include "detail/dispatch_assets.hpp"

#include "../attribute/attribute.hpp"

#include "statement_fwd.hpp"

#include "ast_base.hpp"
#include "value.hpp"
#include "expression.hpp"


namespace rill
{
    namespace ast
    {
        // ----------------------------------------------------------------------
        // ----------------------------------------------------------------------
        //
        // statements
        //
        // ----------------------------------------------------------------------
        // ----------------------------------------------------------------------

        // 
        struct statement
            : public ast_base
        {
        public:
            RILL_AST_ADAPT_VISITOR_VIRTUAL( statement )

        public:
            virtual ~statement()
            {};
        };





        struct block_statement RILL_CXX11_FINAL
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( block_statement )
            
        public:
            block_statement( std::vector<statement_ptr> const& s )
                : statements_( s )
            {}

            block_statement( statement_ptr const& s )
                : statements_( 1, s ) // initialize with one element
            {}
/*
            block_statement( statement_list&& s )
                : s_( s )
            {}
*/
        public:
            statement_list const statements_;
        };



        //
        struct value_initializer_unit
        {
            expression_ptr initializer;
            type_expression_ptr type;
        };


        struct variable_declaration_unit
        {
            identifier_value_base_ptr name;
            value_initializer_unit init_unit;
        };


        typedef std::vector<variable_declaration_unit> variable_declaration_unit_list;


        struct variable_declaration
        {
            attribute::quality_kind quality;        // Ex. val | ref | ...
            variable_declaration_unit decl_unit;
        };


        typedef std::vector<variable_declaration> parameter_list;


        //
        template<typename Target>
        struct template_statement
            : statement
        {
        public:
//            RILL_AST_ADAPT_VISITOR( template_statement )
        };




        struct expression_statement
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( expression_statement )

        public:
            expression_statement( expression_ptr const& expr )
                : expression_( expr )
            {}

        public:
            expression_ptr const expression_;
        };


        struct empty_statement
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( empty_statement )

        public:
            empty_statement()
            {}
        };



        struct extern_statement_base
            : public statement
        {
        public:
            virtual ~extern_statement_base()
            {}
        };


        struct extern_function_declaration_statement
            : public extern_statement_base
        {
        public:
            RILL_AST_ADAPT_VISITOR( extern_function_declaration_statement )

        public:
            extern_function_declaration_statement(
                identifier_value_ptr const& symbol_name,
                parameter_list const& parameter_list,
                type_expression_ptr const& return_type,
                native_string_t const& extern_symbol_name
                )
                : identifier_( symbol_name )
                , parameter_list_( parameter_list )
                , return_type_( return_type )
                , extern_symbol_name_( extern_symbol_name )
            {}

        public:
            auto get_identifier() const
                -> identifier_value_ptr
            {
                return identifier_;
            }

            auto get_parameter_list() const
                -> parameter_list
            {
                return parameter_list_;
            }

            auto get_extern_symbol_name() const
                -> native_string_t
            {
                return extern_symbol_name_;
            }

        public:
            identifier_value_ptr identifier_;
            parameter_list parameter_list_;
            type_expression_ptr return_type_;

            native_string_t extern_symbol_name_;
        };






        struct function_definition_statement_base
            : public statement
        {
//            ADAPT_STATEMENT_VISITOR( function_definition_statement_base )

        public:
            function_definition_statement_base( block_statement_ptr const& block )
                : block_( block )
            {}

            virtual ~function_definition_statement_base()
            {}

        public:


        public:
            block_statement_ptr const block_;
        };



        struct function_definition_statement
            : public function_definition_statement_base
        {
        public:
            RILL_AST_ADAPT_VISITOR( function_definition_statement )

        public:
            function_definition_statement(
                identifier_value_ptr const& symbol_name,
                parameter_list const& parameter_list,
                boost::optional<type_expression_ptr> const& return_type,
                block_statement_ptr const& block
                )
                : function_definition_statement_base( block )
                , identifier_( symbol_name )
                , parameter_list_( parameter_list )
                , return_type_( return_type )
            {}

        public:
            auto get_identifier() const
                -> identifier_value_ptr
            {
                return identifier_;
            }

            auto get_parameter_list() const
                -> parameter_list
            {
                return parameter_list_;
            }

        public:
            identifier_value_ptr identifier_;
            parameter_list parameter_list_;
            boost::optional<type_expression_ptr> return_type_;
        };


        struct intrinsic_function_definition_statement
            : public function_definition_statement_base
        {
        public:
            RILL_AST_ADAPT_VISITOR( intrinsic_function_definition_statement )

        public:
            intrinsic_function_definition_statement( block_statement_ptr const& block )
                : function_definition_statement_base( block )
            {}
        };




        struct class_function_definition_statement
            : public function_definition_statement_base
        {
        public:
            RILL_AST_ADAPT_VISITOR( class_function_definition_statement )

        public:
            class_function_definition_statement(
                identifier_value_base_ptr const& function_name,
                parameter_list const& parameter_list,
                boost::optional<type_expression_ptr> const& return_type,
                block_statement_ptr const& block
                )
                : function_definition_statement_base( block )
                , identifier_( function_name )
                , parameter_list_( parameter_list )
                , return_type_( return_type )
            {}

        public:
            auto get_identifier() const
                -> identifier_value_base_ptr
            {
                return identifier_;
            }

            auto get_parameter_list() const
                -> parameter_list
            {
                return parameter_list_;
            }

        public:
            identifier_value_base_ptr identifier_;
            parameter_list parameter_list_;
            boost::optional<type_expression_ptr> return_type_;
        };





        struct class_definition_statement
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( class_definition_statement )

        public:
            class_definition_statement(
                identifier_value_ptr const& identifier
                )
                : identifier_( identifier )
            {}

            class_definition_statement(
                identifier_value_ptr const& identifier,
                boost::optional<parameter_list> const& constructor_parameter_list,
                block_statement_ptr const& block
                )
                : identifier_( identifier )
                , constructor_parameter_list_( constructor_parameter_list ? std::move( *constructor_parameter_list ) : parameter_list() )
                , block_( block )
            {}

        public:
            auto get_identifier() const
                -> identifier_value_ptr
            {
                return identifier_;
            }

            auto get_constructor_parameter_list() const
                -> parameter_list
            {
                return constructor_parameter_list_;
            }

        public:
            identifier_value_ptr const identifier_;
            parameter_list const constructor_parameter_list_;
            block_statement_ptr const block_;
        };




        struct test_while_statement
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( test_while_statement )

        public:
            test_while_statement(
                expression_ptr const& cond,
                statement_ptr const& body_statement
                )
                : conditional_( cond )
                , body_statement_( body_statement )
            {}

        public:
            expression_ptr const conditional_;
            statement_ptr const body_statement_;
        };



        struct test_if_statement
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( test_if_statement )

        public:
            test_if_statement(
                expression_ptr const& cond,
                statement_ptr const& then_statement,
                boost::optional<statement_ptr> const& else_statement
                )
                : conditional_( cond )
                , then_statement_( then_statement )
                , else_statement_( else_statement )
            {}

        public:
            expression_ptr const conditional_;
            statement_ptr const then_statement_;
            boost::optional<statement_ptr> else_statement_;
        };

 





        struct variable_declaration_statement
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( variable_declaration_statement )

        public:
            variable_declaration_statement( variable_declaration const& decl )
                : declaration_( decl )
            {}

        public:
            auto get_identifier() const
                -> identifier_value_base_ptr
            {
                return declaration_.decl_unit.name;
            }

        public:
            variable_declaration const declaration_;
        };



        struct class_variable_declaration_statement
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( class_variable_declaration_statement )

        public:
            class_variable_declaration_statement( variable_declaration const& decl )
                : declaration_( decl )
            {}

        public:
            auto get_identifier() const
                -> identifier_value_base_ptr
            {
                return declaration_.decl_unit.name;
            }

        public:
            variable_declaration const declaration_;
        };





        struct return_statement
            : public statement
        {
        public:
            RILL_AST_ADAPT_VISITOR( return_statement )

        public:
            return_statement( expression_ptr const& expr )
                : expression_( expr )
            {}

        public:
            expression_ptr const expression_;
        };







        // make native
        inline auto make_native_class( identifier_value_ptr const& class_identifier )
            -> class_definition_statement_ptr
        {
            return std::make_shared<class_definition_statement>( class_identifier );
        }







    } // namespace ast
} // namespace rill


BOOST_FUSION_ADAPT_STRUCT(
    rill::ast::value_initializer_unit,
    (rill::ast::expression_ptr,             initializer)
    (rill::ast::type_expression_ptr,        type)
    )

BOOST_FUSION_ADAPT_STRUCT(
    rill::ast::variable_declaration_unit,
    (rill::ast::identifier_value_base_ptr,  name)
    (rill::ast::value_initializer_unit,     init_unit)
    )

BOOST_FUSION_ADAPT_STRUCT(
    rill::ast::variable_declaration,
    (rill::attribute::quality_kind,         quality)
    (rill::ast::variable_declaration_unit,  decl_unit)
    )

