//--------------------------------------------------------------------------------
// (C) Copyright 2014-2015 Stephane Molina, All rights reserved.
// See https://github.com/Dllieu for updates, documentation, and revision history.
//--------------------------------------------------------------------------------
#include <boost/test/unit_test.hpp>
#include <boost/type_index.hpp>
#include <type_traits>

#include <iostream>
#include "generic/TypeTraits.h"

BOOST_AUTO_TEST_SUITE( TypeTraits )

namespace
{
    // T&& is a universal , it can either mean lvalue (&) or rvalue (&&)
    template <typename T>
    T&& forward( T&& param )
    {
        if ( std::is_lvalue_reference< T >::value )
            return param; // if lvalue, return lvalue ( T& or const T& )
        return std::move( param );
    }
}

BOOST_AUTO_TEST_CASE( DecayTest )
{
    BOOST_CHECK( ( std::is_same<int, std::decay_t<int&> >::value ) );
    BOOST_CHECK( ( std::is_same<int, std::decay_t<int&&> >::value ) );
    BOOST_CHECK( ( std::is_same<int, std::decay_t<const int&> >::value ) );
    BOOST_CHECK( ( std::is_same<int*, std::decay_t<int[2]> >::value ) );
    BOOST_CHECK( ( std::is_same<int(*)(int), std::decay_t<int (int)>>::value ) );

    int i = 5;
    BOOST_CHECK( ( std::is_same< decltype( forward(i) ), int& >::value ) );
    BOOST_CHECK( ( std::is_same< decltype( forward(5) ), int&& >::value ) );
}

namespace
{
    class A {};
    class B { virtual ~B() {} };
}

BOOST_AUTO_TEST_CASE( HasVirtualDestructorTest )
{
    BOOST_CHECK( ! std::has_virtual_destructor< A >::value );
    BOOST_CHECK( std::has_virtual_destructor< B >::value );
}

namespace
{
    class RealClass
    {
    public:
        int     realA() { return 0; }
        double  realB() { return 1; }
    };

    class ForwardClass
    {
    public:
        int         forwardA();
        double      forwardB();

    private:
        RealClass   realClass_;
    };
}

#define IMPLEMFORWARDCLASS( methodName, methodFromRealClassToBeForwarded ) \
    std::decay_t< std::result_of_t< decltype ( &RealClass::##methodFromRealClassToBeForwarded )( RealClass ) > > ForwardClass::##methodName() \
    { \
        return realClass_.##methodFromRealClassToBeForwarded(); \
    }

IMPLEMFORWARDCLASS( forwardA, realA );
IMPLEMFORWARDCLASS( forwardB, realB );

#undef IMPLEMFORWARDCLASS

BOOST_AUTO_TEST_CASE( TraitsTest )
{
    ForwardClass forwardClass;
    RealClass realClass;

    BOOST_CHECK( forwardClass.forwardA() == realClass.realA() );
    BOOST_CHECK( forwardClass.forwardB() == realClass.realB() );
}

BOOST_AUTO_TEST_CASE( VariableTypeTest )
{
    ForwardClass forwardClass;
    (void)forwardClass; // unreferenced local variable
    // class ::anonymous::ForwardClass
    BOOST_CHECK( ! boost::typeindex::type_id_with_cvr< decltype( forwardClass ) >().pretty_name().empty() );
}

namespace
{
    template < typename T >
    std::enable_if_t< std::is_pointer< T >::value >     callWithPointer( T, bool& isPointer ) { isPointer = true; }

    template < typename T, std::size_t sz >
    void callWithPointer( T(&)[sz], bool& isPointer ) { isPointer = false; }
}

BOOST_AUTO_TEST_CASE( ArrayParameterTest )
{
    bool isPointer = false;
    int i[5];

    callWithPointer( &i, isPointer );
    BOOST_CHECK( isPointer );

    callWithPointer( i, isPointer );
    BOOST_CHECK( !isPointer );
}

namespace
{
    template <typename T>
    auto has_to_string_helper( ... ) //... to disambiguate call
        -> std::false_type;

    //true case, to_string valid for T
    template <typename T>
    auto has_to_string_helper( int ) //int to disambiguate call
        -> decltype( std::to_string( std::declval<T>() ), std::true_type{} );

    //alias to make it nice to use
    template <typename T>
    using has_to_string = decltype( has_to_string_helper<T>( 0 ) );
}

BOOST_AUTO_TEST_CASE( HasToStringTest )
{
    BOOST_CHECK( has_to_string< int >::value );
    BOOST_CHECK( ! has_to_string< std::vector< int > >::value );
}

namespace
{
    // don't work with vc120
    /*template <typename...>
    using void_t = void;

    template <typename T, typename = void>
    struct stream_insertion_exists : std::false_type {};

    template <typename T>
    struct stream_insertion_exists<T, void_t<
        decltype( std::declval<std::ostream&>() << std::declval<T>() )
    > > : std::true_type{};*/

    template <typename T>
    auto has_stream_insertion_helper( ... ) // ... to disambiguate call
        -> std::false_type;

    template <typename T>
    auto has_stream_insertion_helper( int ) // int to disambiguate call
        -> decltype( std::declval<std::ostream&>() << std::declval<T>(), std::true_type{} );

    template <typename T>
    using has_stream_insertion = decltype( has_stream_insertion_helper<T>( 0 ) );

    class ClassNoStream {};
    class ClassHasStream {};
    std::ostream&   operator<<( std::ostream&, const ClassHasStream& );
}

BOOST_AUTO_TEST_CASE( HasStreamInsertionTest )
{
    BOOST_CHECK( has_stream_insertion< int >::value );
    BOOST_CHECK( ! has_stream_insertion< ClassNoStream >::value );
    BOOST_CHECK( has_stream_insertion< ClassHasStream >::value );
}

BOOST_AUTO_TEST_SUITE_END() // TypeTraits
