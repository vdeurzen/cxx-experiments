#include <boost/variant.hpp>
#include <iostream>

namespace variant_tooling {
    namespace detail {
        template <typename T> struct functor_traits_helper;

        /*
         * Specialize for pointer to const member functions.
         */
        template <typename T, typename Result, typename... Args>
        struct functor_traits_helper<Result (T::*)(Args...) const> {
            using result_type = Result;
        };

        /*
         * Specialize for pointer to non-const member functions.
         */
        template <typename T, typename Result, typename... Args>
        struct functor_traits_helper<Result (T::*)(Args...)> {
            using result_type = Result;
        };

        /*
         * Instantiate helper with pointer to member function.
         *
         * Based on: https://stackoverflow.com/a/43561563
         *
         */
        template <typename Func>
        struct functor_traits
            : functor_traits_helper<decltype(&Func::operator())> {
        };

        /*
         * Build an overload set for a variadic set of functors.
         *
         * Based on Vitorio Romeo's implementation for C++14 as presented at
         * C++Now 2017:
         *
         * https://youtu.be/3KyW5Ve3LtI
         *
         */
        template <typename TF, typename... TFs>
        struct overload_set : TF, overload_set<TFs...> {
            using TF::operator();
            using overload_set<TFs...>::operator();
            using result_type = typename overload_set<TFs...>::result_type;

            template <typename TFFwd, typename... TRest>
            overload_set(TFFwd&& f, TRest&&... rest)
                : TF{ std::forward<TFFwd>(f) }
                , overload_set<TFs...>{ std::forward<TRest>(rest)... }
            {
            }
        };

        /*
         * Base overload.
         */
        template <typename TF> struct overload_set<TF> : TF {
            using result_type = typename functor_traits<TF>::result_type;
            using TF::operator();

            template <typename TFFwd>
            overload_set(TFFwd&& f)
                : TF{ std::forward<TFFwd>(f) }
            {
            }
        };

        /*
         * Helper function to deduce the functor types for building an
         * overload set.
         */
        template <typename... TFs>
        overload_set<typename std::decay<TFs>::type...> overload(TFs&&... fs)
        {
            return overload_set<typename std::decay<TFs>::type...>(
                std::forward<TFs>(fs)...);
        }

        /*
         * Specific implementation of visitation for Boost's variant.
         */
        template <typename T> class boost_visitor_facilitator {
            const T& visitable_;

        public:
            boost_visitor_facilitator(const T& visitable)
                : visitable_{ visitable }
            {
            }

            template <typename... TVariantVisitors>
            auto operator()(TVariantVisitors&&... visitors) ->
                typename decltype(overload(std::forward<decltype(visitors)>(
                    visitors)...))::result_type const
            {
                auto visitor
                    = overload(std::forward<decltype(visitors)>(visitors)...);
                return boost::apply_visitor(visitor, visitable_);
            }
        };
    } // detail

    template <typename TVariant>
    detail::boost_visitor_facilitator<TVariant> match(TVariant&& v)
    {
        return { v };
    }
} // variant_tooling

using variant = boost::variant<int32_t, int64_t, float, double, std::string>;

struct float_overload {
    // Required for older GCC versions :/
    template <typename... Ts> float_overload(Ts...) {}

    int operator()(float& f) const
    {
        std::cout << "float: " << f++ << std::endl;
        return 0;
    }
};

int main(int argc, char** argv)
{
    variant someValue{ 3.141f };
    if (argc > 1) {
        someValue = std::string(argv[1]);
    }

    auto v = variant_tooling::match(someValue)(
        // We can mix and match custom functors and lambda's. Pay attention
        // to the constness though. If you need mutability for some reason,
        // mark the lambda's as mutable and remove the const qualifier for any
        // of your custom functors' call operators.
        float_overload{},
        [](int32_t i32) {
            std::cout << "i32: " << i32 << std::endl;
            return 1;
        },
        [](int64_t i64) {
            std::cout << "i64: " << i64 << std::endl;
            return 2;
        },
        [](double d) {
            std::cout << "double: " << d << std::endl;
            return 3;
        },
        [](std::string s) {
            std::cout << "string: " << s << std::endl;
            return 4;
        });

    std::cout << "v: " << v << std::endl;

    return 0;
}
