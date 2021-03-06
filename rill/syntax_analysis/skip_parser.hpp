//
// Copyright yutopp 2013 - .
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef RILL_SYNTAX_ANALYSIS_SKIP_PARSER_HPP
#define RILL_SYNTAX_ANALYSIS_SKIP_PARSER_HPP

#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#include <boost/spirit/include/qi.hpp>


namespace rill
{
    namespace syntax_analysis
    {
        namespace qi = boost::spirit::qi;
        namespace ascii = boost::spirit::ascii;

        template<typename Iterator>
        class skip_grammer
            : public qi::grammar<Iterator>
        {
        public:
            skip_grammer()
                : skip_grammer::base_type( comment_, "rill_comment" )
            {
                comment_
                    = ascii::space
                    | ( "//" >> *( ascii::char_ - '\n' ) >> ( '\n' | qi::eoi ) )
                    | ( "/*" >> *( ascii::char_ - "*/" ) >> "*/" );
            }

        private:
            qi::rule<Iterator> comment_;
        };

    } // namespace syntax_analysis
} // namespace rill

#endif /*RILL_SYNTAX_ANALYSIS_SKIP_PARSER_HPP*/
