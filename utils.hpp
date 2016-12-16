#pragma once

namespace phosphor
{
namespace inventory
{
namespace manager
{
namespace details
{
namespace holder
{

/** @struct Base
 *  @brief Adapt from any type base class.
 *
 *  Provides an un-templated base class for use with an adapt to any type
 *  adapter to enable containers of mixed types.
 */
struct Base
{
    Base() = default;
    virtual ~Base() = default;
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
    Base(Base&&) = default;
    Base& operator=(Base&&) = default;
};

/** @struct Holder
 *  @brief Adapt from any type.
 *
 *  Adapts any type to enable containers of mixed types.
 *
 *  @tparam T - The adapted type.
 */
template <typename T>
struct Holder : public Base
{
    Holder() = delete;
    virtual ~Holder() = default;
    Holder(const Holder&) = delete;
    Holder & operator=(const Holder&) = delete;
    Holder(Holder&&) = default;
    Holder& operator=(Holder&&) = default;
    explicit Holder(T&& held) : _held(std::forward<T>(held)) {}

    /** @brief Construct an adapter.
     *
     *  @param[in] held - The object to be adapted.
     *
     *  @returns - std::unique pointer to the adapted object.
     *
     *  @tparam Ret - The type of the pointer to be returned.
     *  @tparam Held - The type of the object to be adapted.
     */
    template <typename Ret, typename Held>
    static auto make_unique(Held&& held)
    {
        return std::make_unique<Ret>(
                std::forward<Held>(held));
    }

    /** @brief Construct an adapter.
     *
     *  @param[in] held - The object to be adapted.
     *
     *  @returns - std::shared pointer to the adapted object.
     *
     *  @tparam Ret - The type of the pointer to be returned.
     *  @tparam Held - The type of the object to be adapted.
     */
    template <typename Ret, typename Held>
    static auto make_shared(Held&& held)
    {
        return std::make_shared<Ret>(
                std::forward<Held>(held));
    }

    /** @brief Provides a weak reference to the held interface. */
    T& get()
    {
        return _held;
    }

    /** @brief Provides a weak reference to the held interface. */
    const T& get() const
    {
        return _held;
    }

    protected:
        T _held;
};

/** @struct CallableBase
 *  @brief Adapt any callable function object base class.
 *
 *  Provides an un-templated base class for use with an adapt to any
 *  callable function object type.
 *
 *  @tparam Ret - The return type of the callable.
 *  @tparam Args - The argument types of the callable.
 */
template <typename Ret, typename ...Args>
struct CallableBase
{
    CallableBase() = default;
    virtual ~CallableBase() = default;
    CallableBase(const CallableBase&) = delete;
    CallableBase& operator=(const CallableBase&) = delete;
    CallableBase(CallableBase&&) = default;
    CallableBase& operator=(CallableBase&&) = default;

    virtual Ret operator()(Args&&...args) const = 0;
    virtual Ret operator()(Args&&...args)
    {
        return const_cast<const CallableBase&>(*this)(
                std::forward<Args>(args)...);
    }
};

/** @struct CallableHolder
 *  @brief Adapt from any callable type.
 *
 *  Adapts any callable type.
 *
 *  @tparam T - The type of the callable.
 *  @tparam Ret - The return type of the callable.
 *  @tparam Args - The argument types of the callable.
 */
template <typename T, typename Ret, typename ...Args>
struct CallableHolder final :
    public CallableBase<Ret, Args...>,
    public Holder<T>
{
    CallableHolder() = delete;
    ~CallableHolder() = default;
    CallableHolder(const CallableHolder&) = delete;
    CallableHolder & operator=(const CallableHolder&) = delete;
    CallableHolder(CallableHolder&&) = default;
    CallableHolder& operator=(CallableHolder&&) = default;
    explicit CallableHolder(T&& func) : Holder<T>(std::forward<T>(func)) {}

    virtual Ret operator()(Args&&...args) const override
    {
        return this->_held(std::forward<Args>(args)...);
    }

    virtual Ret operator()(Args&&...args) override
    {
        return this->_held(std::forward<Args>(args)...);
    }
};

} // namespace holder
} // namespace details
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
