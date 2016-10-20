#pragma once

#include <utility>
#include <memory>

namespace phosphor
{
namespace inventory
{
namespace manager
{
class Manager;
namespace actions
{

inline void noop(Manager &mgr) noexcept { }

struct DestroyObject
{
    DestroyObject() = delete;
    ~DestroyObject() = default;
    DestroyObject(const DestroyObject&) = delete;
    DestroyObject& operator=(const DestroyObject&) = delete;
    DestroyObject(DestroyObject&&) = default;
    DestroyObject& operator=(DestroyObject&&) = delete;
    explicit DestroyObject(const char *path) : _path(path) {}
    void operator()(Manager &) const;

    private:
    const char *_path;
};

namespace details
{

struct WrapperBase
{
    WrapperBase() = default;
    virtual ~WrapperBase() = default;
    WrapperBase(const WrapperBase&) = delete;
    WrapperBase& operator=(const WrapperBase&) = delete;
    WrapperBase(WrapperBase&&) = delete;
    WrapperBase& operator=(WrapperBase&&) = delete;

    virtual void operator()(Manager &mgr) const = 0;
    virtual void operator()(Manager &mgr)
    {
        const_cast<const WrapperBase &>(*this)(mgr);
    }
};

template <typename T>
struct Wrapper final : public WrapperBase
{
    Wrapper() = delete;
    ~Wrapper() = default;
    Wrapper(const Wrapper&) = delete;
    Wrapper & operator=(const Wrapper&) = delete;
    Wrapper(Wrapper&&) = delete;
    Wrapper& operator=(Wrapper&&) = delete;
    explicit Wrapper(T &&func) : _func(std::forward<T>(func)) {}

    virtual void operator()(Manager &mgr) const override
    {
        _func(mgr);
    }

    virtual void operator()(Manager &mgr) override
    {
        _func(mgr);
    }

    private:
    T _func;
};

struct Holder
{
    template <typename T>
    Holder(T &&func) :
        _ptr(std::shared_ptr<WrapperBase>(
                    new Wrapper<T>(std::forward<T>(func)))) { }

    ~Holder() = default;
    Holder(const Holder&) = default;
    Holder& operator=(const Holder&) = delete;
    Holder(Holder&&) = delete;
    Holder& operator=(Holder&&) = delete;

    void operator()(Manager &mgr)
    {
        (*_ptr)(mgr);
    }
    void operator()(Manager &mgr) const
    {
        (*_ptr)(mgr);
    }

    private:
    std::shared_ptr<WrapperBase> _ptr;
};

} // namespace details
} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
