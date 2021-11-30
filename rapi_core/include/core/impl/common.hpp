#pragma once

#include <istream>
#include <type_traits>

#include "adstutil_cxx/type_id.hpp"

namespace adst::ep::test_engine::core::impl {

/**
 * Returns a human readable string of the given message type (which, for now, is only intended for
 * tracing).
 *
 * The message name is provided by the protobuf reflection interface (otherwise messages would
 * require a static NAME member).
 *
 * @tparam EventT The event (or message) type to use.
 * @return String containing the types name.
 */
template <typename EventT>
const std::string getTypeName()
{
   return EventT::NAME.c_str();
}

/**
 * Internal alias for type_id type to be able to easily change to a different implementation.
 */
using type_id_t = adst::common::type_id_t;

/**
 * Helper for getting "type id".
 * @tparam T Input type for id creation.
 * @return A unique scalar id for input type T.
 */
template <typename T>
type_id_t type_id()
{
    return adst::common::type_id<T>();
}

/**
 * Verify that the type used as Event fulfills the necessary requirements to be used as Event.
 */
template <class Event>
constexpr bool validateEvent()
{
    static_assert(std::is_const<Event>::value == false, "Struct must be without const");
    static_assert(std::is_volatile<Event>::value == false, "Struct must be without volatile");
    static_assert(std::is_reference<Event>::value == false, "Struct must be without reference");
    static_assert(std::is_pointer<Event>::value == false, "Struct must be without pointer");
    return true;
}

} // namespace adst::ep::test_engine::core::impl
