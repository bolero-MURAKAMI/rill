//
// Copyright yutopp 2013 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <rill/code_generator/llvm_ir_generator.hpp>
#include <rill/utility/embedded_function_holder.hpp>
#include <rill/environment.hpp>

#include <boost/scope_exit.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include <llvm/Analysis/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <rill/ast/root.hpp>
#include <rill/ast/statement.hpp>
#include <rill/ast/expression.hpp>
#include <rill/ast/value.hpp>


namespace rill
{
    namespace code_generator
    {
        template<typename EnvIdT, typename IdTablePtr, typename IRBuilderPtr>
        static auto inline ref_value_with( EnvIdT const& env_id, IdTablePtr const& table, IRBuilderPtr builder )
            -> llvm::Value*
        {
            auto const ref_value = table->ref_value( env_id );
            if ( table->is_alloca_inst( env_id ) ) {
                return builder->CreateLoad( ref_value );
            }
            return ref_value;
        }



        // = definition ===
        // class llvm_ir_generator
        // ================

        llvm_ir_generator::llvm_ir_generator( const_environment_ptr const& root_env, embedded_function_holder_ptr const& action_holder )
            : context_( llvm::getGlobalContext() )
            , module_( std::make_shared<llvm::Module>( "rill", context_ ) )
            , builder_( std::make_shared<llvm::IRBuilder<>>( context_ ) )
            , llvm_table_( std::make_shared<env_id_llvm_table>() )
            , root_env_( root_env )
            , action_holder_( action_holder )
        {}


        // Root Scope
        RILL_TV_OP_CONST( llvm_ir_generator, ast::root, r, root_env )
        {
            // maybe rooted once

            //
            // bind primitive types
            //

            // bind [ int -> i32 ]
            llvm_table_->bind_type( root_env->lookup( ast::intrinsic::make_single_identifier( "int" ) )->get_id(), llvm::Type::getInt32Ty( llvm::getGlobalContext() ) );

            // bind [ string -> i8* ]
            llvm_table_->bind_type( root_env->lookup( ast::intrinsic::make_single_identifier( "string" ) )->get_id(), llvm::Type::getInt8Ty( llvm::getGlobalContext() )->getPointerTo() );

            // bind [ void -> void ]
            llvm_table_->bind_type( root_env->lookup( ast::intrinsic::make_single_identifier( "void" ) )->get_id(), llvm::Type::getVoidTy( llvm::getGlobalContext() ) );

            //
            for( auto const& node : r->statements_ )
                dispatch( node );
        }

        //
        RILL_TV_OP_CONST( llvm_ir_generator, ast::expression_statement, s, env )
        {
            dispatch( s->expression_, root_env_->get_related_env_by_ast_ptr( s->expression_ ) );
        }


        RILL_TV_OP_CONST( llvm_ir_generator, ast::return_statement, s, env )
        {
            builder_->CreateRet( dispatch( s->expression_, env ) );
        }


        RILL_TV_OP_CONST( llvm_ir_generator, ast::function_definition_statement, s, self_env )
        {
            //
            std::cout << "!!!!!!ast::function_definition_statement" << self_env << " / " << root_env_->get_id() << std::endl;

            //
            auto const& f_env = std::static_pointer_cast<function_symbol_environment const>( ( self_env != nullptr ) ? self_env : root_env_->get_related_env_by_ast_ptr( s ) );
            assert( f_env != nullptr );
            if ( llvm_table_->is_defined( f_env->get_id() ) )
                return;

            auto const& parameter_variable_decl_env_ids = f_env->get_parameter_decl_ids();
            auto const& parameter_variable_type_env_ids = f_env->get_parameter_type_ids();
            std::cout << "()()=> :" << f_env->mangled_name() << std::endl;

            //
            auto const current_insert_point = builder_->saveIP();
            BOOST_SCOPE_EXIT((&builder_)(&current_insert_point)) {
                // restore insert point
                builder_->restoreIP( current_insert_point );
            } BOOST_SCOPE_EXIT_END

            //
            // TODO: use current_insert_point.isSet(), if it is false, this function needs external linkage( currently, all functions are exported as external linkage )
            auto const linkage = llvm::Function::ExternalLinkage;
            if ( current_insert_point.isSet() ) {
                //
                std::cout << "not external." << std::endl;
            }

            // signature
            std::vector<llvm::Type*> parameter_types;
            for( auto const& type_env_id : parameter_variable_type_env_ids ) {
                std::cout << type_env_id << std::endl;
                parameter_types.push_back( llvm_table_->ref_type( type_env_id ) );
            }
            llvm::Type* const return_type = llvm_table_->ref_type( f_env->get_return_type_environment()->get_id() );
            llvm::FunctionType* const func_type = llvm::FunctionType::get( return_type, parameter_types, false/*not variable*/ );
            
            // function body
            llvm::Function* const func = llvm::Function::Create( func_type, linkage, f_env->mangled_name(), module_.get() );
            for( llvm::Function::arg_iterator ait = func->arg_begin(); ait != func->arg_end(); ++ait ) {
                std::cout << "Argument No: " << ait->getArgNo() << std::endl;
                auto const& var = std::static_pointer_cast<variable_symbol_environment const>( f_env->get_env_at( parameter_variable_decl_env_ids[ait->getArgNo()] ).lock() );
                ait->setName( var->mangled_name() );

                llvm_table_->bind_value( var->get_id(), ait );
            }

            //
            llvm_table_->bind_function_type( f_env->get_id(), func_type );

            // create a new basic block to start insertion into.
            llvm::BasicBlock* const basic_brock = llvm::BasicBlock::Create( llvm::getGlobalContext(), "entry", func );
            builder_->SetInsertPoint( basic_brock );

            //
            for( auto const& node : s->statements_ )
                dispatch( node, root_env_->get_related_env_by_ast_ptr( node ) );

            std::cout << "AAA: " << s->statements_.size() << std::endl;
            builder_->CreateRetVoid();

            //
            llvm::verifyFunction( *func, llvm::PrintMessageAction );
        }



        RILL_TV_OP_CONST( llvm_ir_generator, ast::embedded_function_definition_statement, s, env )
        {
            // cast to function symbol env
            auto const& f_env = std::static_pointer_cast<function_symbol_environment const>( ( env != nullptr ) ? env : root_env_->get_related_env_by_ast_ptr( s ) );
            assert( f_env != nullptr );
            if ( llvm_table_->is_defined( f_env->get_id() ) )
                return;

            auto const& parameter_variable_decl_env_ids = f_env->get_parameter_decl_ids();
            auto const& parameter_variable_type_env_ids = f_env->get_parameter_type_ids();


            //
            auto const current_insert_point = builder_->saveIP();
            BOOST_SCOPE_EXIT((&builder_)(&current_insert_point)) {
                // restore insert point
                builder_->restoreIP( current_insert_point );
            } BOOST_SCOPE_EXIT_END

            //
            // TODO: use current_insert_point.isSet(), if it is false, this function needs external linkage( currently, all functions are exported as external linkage )
            auto const linkage = llvm::Function::ExternalLinkage;
            if ( current_insert_point.isSet() ) {
                //
                std::cout << "not external." << std::endl;
            }

            // define paramter and return types
            std::vector<llvm::Type*> parmeter_types;
            for( auto const& type_env_id : parameter_variable_type_env_ids ) {
                parmeter_types.push_back( llvm_table_->ref_type( type_env_id ) );
            }
            auto const& return_type = llvm_table_->ref_type( f_env->get_return_type_environment()->get_id() );

            // get function type
            llvm::FunctionType* const func_type = llvm::FunctionType::get( return_type, parmeter_types, false/*is not variadic*/ );

            //
            llvm_table_->bind_function_type( f_env->get_id(), func_type );

            // construct function and paramter variables
            llvm::Function* const func = llvm::Function::Create( func_type, llvm::Function::ExternalLinkage, f_env->mangled_name(), module_.get() );
            for( llvm::Function::arg_iterator ait=func->arg_begin(); ait!=func->arg_end(); ++ait ) {
                auto const& var = std::static_pointer_cast<variable_symbol_environment const>( f_env->get_env_at( parameter_variable_decl_env_ids[ait->getArgNo()] ).lock() );
                ait->setName( var->mangled_name() );

                llvm_table_->bind_value( var->get_id(), ait );
            }

            // set initial insert point to entry
            llvm::BasicBlock* const function_entry_block = llvm::BasicBlock::Create( llvm::getGlobalContext(), "entry", func );
            builder_->SetInsertPoint( function_entry_block );

            // build inner statements(that contain embedded_function_call)
            for( auto const& node : s->statements_ )
                dispatch( node, root_env_->get_related_env_by_ast_ptr( node ) );

            // TODO: fix
            builder_->CreateRetVoid();

            //
            llvm::verifyFunction( *func, llvm::PrintMessageAction );
        }




        RILL_TV_OP_CONST( llvm_ir_generator, ast::variable_declaration_statement, s, env )
        {
            // TODO: all of variablea(mutable) should be allocated at head of function...

            // cast to variable symbol env
            auto const& v_env = std::static_pointer_cast<variable_symbol_environment const>( ( env != nullptr ) ? env : root_env_->get_related_env_by_ast_ptr( s ) );
            assert( v_env != nullptr );

            //
            auto const& variable_type = llvm_table_->ref_type( v_env->get_type_environment()->get_id() );

            // TODO: decide initial value
            // calucurate initial value
            // auto const& initial_value = dispatch( s, _ );

            llvm::AllocaInst* const allca_inst = builder_->CreateAlloca( variable_type, 0 );

            llvm_table_->bind_value( v_env->get_id(), allca_inst );
        }


        RILL_TV_OP_CONST( llvm_ir_generator, ast::extern_function_declaration_statement, s, env )
        {
            // cast to function symbol env
            auto const& f_env = std::static_pointer_cast<function_symbol_environment const>( ( env != nullptr ) ? env : root_env_->get_related_env_by_ast_ptr( s ) );
            assert( f_env != nullptr );
            if ( llvm_table_->is_defined( f_env->get_id() ) )
                return;

            auto const& parameter_variable_decl_env_ids = f_env->get_parameter_decl_ids();
            auto const& parameter_variable_type_env_ids = f_env->get_parameter_type_ids();


            //
            auto const current_insert_point = builder_->saveIP();
            BOOST_SCOPE_EXIT((&builder_)(&current_insert_point)) {
                // restore insert point
                builder_->restoreIP( current_insert_point );
            } BOOST_SCOPE_EXIT_END



            // define paramter and return types
            std::vector<llvm::Type*> parmeter_types;
            for( auto const& type_env_id : parameter_variable_type_env_ids ) {
                parmeter_types.push_back( llvm_table_->ref_type( type_env_id ) );
            }
            auto const& return_type = llvm_table_->ref_type( f_env->get_return_type_environment()->get_id() );

            // get function type
            llvm::FunctionType* const func_type = llvm::FunctionType::get( return_type, parmeter_types, false/*is not variadic*/ );

            //
            llvm_table_->bind_function_type( f_env->get_id(), func_type );

        }




        RILL_TV_OP_CONST( llvm_ir_generator, ast::binary_operator_expression, e, _ )
        {
            // Look up Function
            auto const f_env = std::static_pointer_cast<function_symbol_environment const>( root_env_->get_related_env_by_ast_ptr( e ) );
            assert( f_env != nullptr );
            auto const& target_name = f_env->mangled_name();
            auto const& callee_function = [&]() -> llvm::Function* const {
                if ( auto const f = module_->getFunction( target_name ) )
                    return f;

                // generate
                dispatch( f_env->get_related_ast(), f_env );
                if ( auto const f = module_->getFunction( target_name ) )
                    return f;

                return nullptr;
            }();
            if ( !callee_function ) {
                // unexpected error...
                assert( false );
            }


            // evaluate values(and push to stack) and returned value type
            auto const& rhs_value = dispatch( e->rhs_, _ );
            auto const& lhs_value = dispatch( e->lhs_, _ );
            assert( rhs_value != nullptr && lhs_value != nullptr );
            std::vector<llvm::Value*> const args = { rhs_value, lhs_value };


            std::cout << "CALL!!!!!" << std::endl;

            // invocation
            return builder_->CreateCall( callee_function, args, "calltmp" );
        }



        RILL_TV_OP_CONST( llvm_ir_generator, ast::call_expression, e, _ )
        {
            // Look up Function
            auto const f_env = std::static_pointer_cast<function_symbol_environment const>( root_env_->get_related_env_by_ast_ptr( e ) );
            assert( f_env != nullptr );


            if ( !llvm_table_->is_defined( f_env->get_id() ) ) {
                // 
                std::cout << "!llvm_table_->is_defined( f_env->get_id() ): " << f_env->mangled_name() << std::endl;
                dispatch( f_env->get_related_ast(), f_env );
            }

            std::cout << "current : " << f_env->mangled_name() << std::endl;
            auto const& callee_function
                = ( f_env->has_attribute( function_symbol_environment::attr::e_extern ) )
                ? [&, this]() -> llvm::Constant* const {
                    llvm::FunctionType* const func_type = llvm_table_->ref_function_type( f_env->get_id() );
                    auto const& s = f_env->get_related_ast();
                    assert( s != nullptr );
                    return module_->getOrInsertFunction( std::static_pointer_cast<ast::extern_function_declaration_statement const>( s )->get_extern_symbol_name(), func_type );
                }()
                : [&, this]() -> llvm::Constant* const {
                    auto const& target_name = f_env->mangled_name();
                    std::cout << "SS: " << target_name << std::endl;
                    if ( auto const f = module_->getFunction( target_name ) )
                        return f;

                    return nullptr;
                }()
                ;
            if ( !callee_function ) {
                // unexpected error...
                assert( false && "unexpected... callee_function was not found" );
            }


            // call function that defined in rill modules
            // evaluate argument from last to front(but ordering of vector is from front to last)
            std::vector<llvm::Value*> args;
            for( auto const& val : e->arguments_ | boost::adaptors::reversed ) {
                args.insert( args.begin(), dispatch( val, _ ) );
                assert( args.back() != nullptr );
            }

            // invocation
            return builder_->CreateCall( callee_function, args/*, "calltmp"*/ );
        }


        // TODO: change name to native code injection expression
        RILL_TV_OP_CONST( llvm_ir_generator, ast::embedded_function_call_expression, e, _ )
        {
            // look up the function
            auto const f_env = std::static_pointer_cast<function_symbol_environment const>( root_env_->get_related_env_by_ast_ptr( e ) );
            assert( f_env != nullptr );

            // look up the action
            auto const& action = action_holder_->at( e->action_id_ );
            assert( action != nullptr );

            // generate codes into this context
            auto const value = action->invoke( processing_context::llvm_ir_generator_k, module_, builder_, llvm_table_, f_env->get_parameter_decl_ids() );
            assert( value != nullptr );

            return value;
        }


        RILL_TV_OP_CONST( llvm_ir_generator, ast::term_expression, e, _ )
        {
            return dispatch( e->value_, _ );
        }

        RILL_TV_OP_CONST( llvm_ir_generator, ast::intrinsic_value, v, _ )
        {
            // TODO: check primitive type
            if ( v->literal_type_name_->get_inner_symbol()->to_native_string() == "int" ) {
                // Currently, return int type( 32bit, integer )
                return llvm::ConstantInt::get( llvm::getGlobalContext(), llvm::APInt( 32, std::static_pointer_cast<ast::intrinsic::int32_value const>( v->value_ )->get_value() ) );

            } else if ( v->literal_type_name_->get_inner_symbol()->to_native_string() == "string" ) {
                // char pointer...(string?)
                return builder_->CreateGlobalStringPtr( std::static_pointer_cast<ast::intrinsic::string_value const>( v->value_ )->value_.c_str() );

            } else {
                assert( false );
                return nullptr;
            }
        }

        RILL_TV_OP_CONST( llvm_ir_generator, ast::variable_value, v, _ )
        {
            auto const v_env = std::static_pointer_cast<variable_symbol_environment const>( root_env_->get_related_env_by_ast_ptr( v ) );
            assert( v_env != nullptr );

            return ref_value_with( v_env->get_id(), llvm_table_, builder_ );
        }
    } // namespace code_generator
} // namespace rill