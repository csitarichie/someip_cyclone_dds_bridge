#pragma once

/** \file compiler_diagnostics.hpp Defines macros to help with compiler warnings/errors. */

/**
 * \brief Convert \a s to a string literal using the `#` preprocessor
 *        operator.
 *
 * \param s String to stringize.
 */
#define ADST_STRINGIZE(s) #s

/**
 * \def ADST_DISABLE_WARNING(warning_name)
 *
 * \brief Disables a clang/gcc compiler warning until the next
 *        #ADST_RESTORE_CLANG_WARNING() statement.
 *
 * Note: Using this macro is only possible if the warning has the same name
 * for gcc and clang.
 *
 * \param warning_name The name of the warning to be disabled
 *        (warning_name must be given as quoted string).
 */

/**
 * \def ADST_RESTORE_WARNING()
 *
 * \brief Re-enables a clang/gcc compiler warning from a previous
 *        #ADST_DISABLE_WARNING(warning_name) statement.
 */

/**
 * \def ADST_DISABLE_CLANG_WARNING(warning_name)
 *
 * \brief Disables a clang compiler warning until the next
 *        #ADST_RESTORE_CLANG_WARNING() statement.
 *
 * \param warning_name The name of the warning to be disabled
 *        (warning_name must be given as quoted string).
 */

/**
 * \def ADST_RESTORE_CLANG_WARNING()
 *
 * \brief Re-enables a clang compiler warning from a previous
 *        #ADST_DISABLE_CLANG_WARNING(warning_name) statement.
 */

/**
 * \def ADST_DISABLE_GCC_WARNING(warning_name)
 *
 * \brief Disables a gcc compiler warning until the next
 *        #ADST_RESTORE_GCC_WARNING() statement.
 *
 * \param warning_name The name of the warning to be disabled
 *        (warning_name must be given as quoted string).
 */

/**
 * \def ADST_RESTORE_GCC_WARNING()
 *
 * \brief Re-enables a gcc compiler warning from a previous
 *        #ADST_DISABLE_GCC_WARNING(warning_name) statement.
 */

/**
 * \dev UNUSED_PARAM
 *
 * \brief Avoid warnings about unused parameters.
 *
 * \param P Parameter name.
 */
#define UNUSED_PARAM(P) (void)(P)

#if defined(__GNUC__)
// Multiple compilers define __GNUC__: gcc, clang, intel.

/**
 * \def PRAGMA_STATEMENT
 *
 * \brief Allows to use \#pragma statements as part of a macro.
 *
 * \param x Argument(s) of \#pragma statement to be generated.
 */
#    define PRAGMA_STATEMENT(x) _Pragma(#    x)

/**
 * \def PRAGMA_IGNORE_WARNING(warning_flag)
 *
 * \brief Helper macro which creates the compiler specific pragram incantation
 *        which disables a compiler warning.
 *
 * \param warning_flag The warning flag to be disabled
 *        (warning_flag must be given as quoted string).
 */

#    if defined(__clang__)
#        define PRAGMA_IGNORE_WARNING(warning_flag) PRAGMA_STATEMENT(clang diagnostic ignored warning_flag)
#        define ADST_DISABLE_CLANG_WARNING(warning_name) \
            PRAGMA_STATEMENT(clang diagnostic push)      \
            PRAGMA_IGNORE_WARNING("-W" warning_name)
#        define ADST_RESTORE_CLANG_WARNING() PRAGMA_STATEMENT(clang diagnostic pop)

#        define ADST_DISABLE_WARNING(warning_name) ADST_DISABLE_CLANG_WARNING(warning_name)
#        define ADST_RESTORE_WARNING() ADST_RESTORE_CLANG_WARNING()

#        define ADST_DISABLE_GCC_WARNING(warning_name)
#        define ADST_RESTORE_GCC_WARNING()
#    else
// GCC
#        define PRAGMA_IGNORE_WARNING(warning_flag) PRAGMA_STATEMENT(GCC diagnostic ignored warning_flag)
#        define ADST_DISABLE_GCC_WARNING(warning_name) \
            PRAGMA_STATEMENT(GCC diagnostic push)      \
            PRAGMA_IGNORE_WARNING("-W" warning_name)
#        define ADST_RESTORE_GCC_WARNING() PRAGMA_STATEMENT(GCC diagnostic pop)

#        define ADST_DISABLE_WARNING(warning_name) ADST_DISABLE_GCC_WARNING(warning_name)
#        define ADST_RESTORE_WARNING() ADST_RESTORE_GCC_WARNING()

#        define ADST_DISABLE_CLANG_WARNING(warning_name)
#        define ADST_RESTORE_CLANG_WARNING()
#    endif
#else
#    define ADST_DISABLE_GCC_WARNING(warning_name)
#    define ADST_RESTORE_GCC_WARNING()

#    define ADST_DISABLE_CLANG_WARNING(warning_name)
#    define ADST_RESTORE_CLANG_WARNING()
#
#    define ADST_DISABLE_WARNING(warning_name)
#    define ADST_RESTORE_WARNING()
#endif
