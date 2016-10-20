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
namespace details
{
namespace holder
{

struct Base
{
    Base() = default;
    virtual ~Base() = default;
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
    Base(Base&&) = delete;
    Base& operator=(Base&&) = delete;

    virtual void operator()(Manager &mgr) const = 0;
    virtual void operator()(Manager &mgr)
    {
        const_cast<const Base &>(*this)(mgr);
    }
};

template <typename T>
struct Holder final : public Base
{
    Holder() = delete;
    ~Holder() = default;
    Holder(const Holder&) = delete;
    Holder & operator=(const Holder&) = delete;
    Holder(Holder&&) = delete;
    Holder& operator=(Holder&&) = delete;
    explicit Holder(T &&func) : _func(std::forward<T>(func)) {}

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

} // namespace holder

struct Wrapper
{
    template <typename T>
    Wrapper(T &&func) :
        _ptr(std::shared_ptr<holder::Base>(
                    new holder::Holder<T>(std::forward<T>(func)))) { }

    ~Wrapper() = default;
    Wrapper(const Wrapper&) = default;
    Wrapper& operator=(const Wrapper&) = delete;
    Wrapper(Wrapper&&) = delete;
    Wrapper& operator=(Wrapper&&) = delete;

    void operator()(Manager &mgr)
    {
        (*_ptr)(mgr);
    }
    void operator()(Manager &mgr) const
    {
        (*_ptr)(mgr);
    }

    private:
    std::shared_ptr<holder::Base> _ptr;
};

} // namespace details

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

} // namespace actions
} // namespace manager
} // namespace inventory
} // namespace phosphor

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
