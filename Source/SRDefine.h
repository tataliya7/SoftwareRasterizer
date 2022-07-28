#ifndef SR_SOLUTION_CONFIGURATIONS_DEBUG
    #if defined(_DEBUG) || defined(DEBUG)
        #define SR_SOLUTION_CONFIGURATIONS_DEBUG
    #else
        #ifndef SR_SOLUTION_CONFIGURATIONS_RELEASE
            #define SR_SOLUTION_CONFIGURATIONS_RELEASE
        #endif
    #endif
#endif

#if defined(SR_SOLUTION_CONFIGURATIONS_DEBUG)
	#define ASSERT(expression) assert(expression)
#else
	#define ASSERT(expression)
#endif

#if defined(_MSC_VER)
    #define SR_DISABLE_WARNINGS __pragma(warning(push, 0))
    #define SR_ENABLE_WARNINGS __pragma(warning(pop))
#endif

#if defined(_MSC_VER)
    #define FORCEINLINE __forceinline
#endif

#define ENUM_CLASS_OPERATORS(EnumClass) inline           EnumClass& operator|=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))rhs | (__underlying_type(EnumClass))rhs); } \
										inline           EnumClass& operator&=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))rhs & (__underlying_type(EnumClass))rhs); } \
										inline           EnumClass& operator^=(EnumClass& lhs, EnumClass rhs)   { return lhs = (EnumClass)((__underlying_type(EnumClass))rhs ^ (__underlying_type(EnumClass))rhs); } \
										inline constexpr EnumClass  operator| (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs | (__underlying_type(EnumClass))rhs); } \
										inline constexpr EnumClass  operator& (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs & (__underlying_type(EnumClass))rhs); } \
										inline constexpr EnumClass  operator^ (EnumClass  lhs, EnumClass rhs)   { return (EnumClass)((__underlying_type(EnumClass))lhs ^ (__underlying_type(EnumClass))rhs); } \
										inline constexpr bool       operator! (EnumClass  e)                    { return !(__underlying_type(EnumClass))e; } \
										inline constexpr EnumClass  operator~ (EnumClass  e)                    { return (EnumClass)~(__underlying_type(EnumClass))e; } \
										inline           bool       HAS_ANY_FLAGS(EnumClass e, EnumClass flags) { return (e & flags) != (EnumClass)0; }
