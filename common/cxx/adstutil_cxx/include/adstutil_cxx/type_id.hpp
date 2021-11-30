#pragma once

namespace adst::common {

// using type_id_t to mimic std built in class naming
class type_id_t
{
public:
    template <typename T>
    friend constexpr type_id_t type_id(); // the only function which able to create the type_id_t class

    type_id_t() = delete;

    bool operator==(type_id_t rhs) const
    {
        return id_ == rhs.id_;
    }

    friend bool operator<(const type_id_t& lhs, const type_id_t& rhs)
    {
        return lhs.id_ < rhs.id_;
    }

    bool operator!=(type_id_t rhs) const
    {
        return id_ != rhs.id_;
    }

private:
    using singature = type_id_t();

    constexpr explicit type_id_t(singature* id)
        : id_{id}
    {
    }

    singature* id_;
};

// here is the trick of using the compiler type system to generate
// unique function pointer for type_id<T>() function.
// then encapsulate this function pointer in type_id_t and use it as id.
// this works across different compile contexts.
// not sure about dynamic Library boundaries ... there linkage is different.
// so I would not use it in such a context.
template <typename T>
constexpr type_id_t type_id()
{
    return type_id_t(&type_id<T>);
}

} // namespace adst::common
