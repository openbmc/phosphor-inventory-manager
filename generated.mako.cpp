## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// This file was auto generated.  Do not edit.
#include "manager.hpp"
#include "utils.hpp"
% for i in interfaces:
#include <${i.header()}>
% endfor

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
        details::MakeInterface<
            details::ServerObject<
                ${i.namespace()}>>::make,
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
