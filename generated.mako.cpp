## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// This file was auto generated.  Do not edit.
#include "manager.hpp"
#include "utils.hpp"
#include "functor.hpp"
% for i in interfaces:
#include <${i.header()}>
% endfor
#include "gen_serialization.hpp"

namespace phosphor
{
namespace inventory
{
namespace manager
{

using namespace std::literals::string_literals;

const Manager::Makers Manager::_makers{
% for i in interfaces:
    {
        "${str(i)}",
        std::make_tuple(
            MakeInterface<
                ServerObject<
                    ${i.namespace()}>>::make,
            MakeInterface<
                ServerObject<
                    ${i.namespace()}>>::assign,
            MakeInterface<
                ServerObject<
                    ${i.namespace()}>>::serialize,
            MakeInterface<
                ServerObject<
                    ${i.namespace()}>>::deserialize
        )
    },
% endfor
};

const Manager::Events Manager::_events{
% for e in events:
    {
    % if e.description:
        // ${e.description.strip()}
    % endif
        ${e.call(loader, indent=indent +2)},
    },
%endfor
};

} // namespace manager
} // namespace inventory
} // namespace phosphor
