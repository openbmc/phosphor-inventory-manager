#pragma once

namespace phosphor
{
namespace inventory
{
namespace manager
{
/** @struct MakeVariantVisitor
 *  @brief Return a variant if the visited type is a possible variant type.
 *
 *  @tparam V - The desired variant type.
 */
template <typename V>
struct MakeVariantVisitor
{
    /** @struct Make
     *  @brief Return variant visitor.
     *
     *  @tparam T - The variant type to return.
     *  @tparam Arg - The type being visited in the source variant.
     *  @tparam Enable - Overload resolution removal.
     */
    template <typename T, typename Arg, typename Enable = void>
    struct Make
    {
        static auto make(Arg&& arg)
        {
            throw sdbusplus::message::variant_ns::bad_variant_access(
                "in MakeVariantVisitor");
            return T();
        }
    };

    /** @struct Make
     *  @brief Return variant visitor.
     *
     *  struct Make specialization if Arg is in T (int -> variant<int, char>).
     */
    template <typename T, typename Arg>
    struct Make<T, Arg,
               typename std::enable_if<std::is_convertible<Arg, T>::value>::type>
    {
        static auto make(Arg&& arg)
        {
            return T(std::forward<Arg>(arg));
        }
    };

    /** @brief Make variant visitor.  */
    template <typename Arg>
    auto operator()(Arg&& arg) const
    {
        return Make<V, Arg>::make(arg);
    }
};

/** @brief Convert variants with different contained types.
 *
 *  @tparam V - The desired variant type.
 *  @tparam Arg - The source variant type.
 *
 *  @param[in] v - The source variant.
 *  @returns - The converted variant.
 */
template <typename V, typename Arg>
auto convertVariant(Arg&& v)
{
    return sdbusplus::message::variant_ns::apply_visitor(
               MakeVariantVisitor<V>(), v);
}
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
