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
        std::make_tuple(
            std::vector<details::EventBasePtr>(
                {
    % if e.cls == 'match':
                    std::make_shared<details::DbusSignal>(
        % for i, s in enumerate(e.signatures[0].sig.items()):
            % if i == 0:
                        ${'"{0}=\'{1}\',"'.format(*s)}
            % elif i + 1 == len(e.signatures[0].sig):
                            ${'"{0}=\'{1}\'"'.format(*s)},
            % else:
                            ${'"{0}=\'{1}\',"'.format(*s)}
            % endif
        % endfor
        % if e.filters[0].pointer:
                        details::make_filter(${e.filters[0].bare_method()})),
        % else:
            % if e.filters[0].args:
                        details::make_filter(
                            ${e.filters[0].bare_method()}(
                % for i, arg in enumerate(e.filters[0].args):
                    % if i + 1 != len(e.filters[0].args):
                                ${arg.cppArg()},
                    % else:
                                ${arg.cppArg()}))),
                    % endif
                % endfor
            % else:
                        details::make_filter(
                            ${e.filters[0].bare_method()}()),
            % endif
        % endif
    % endif
                }
            ),
            std::vector<details::ActionBasePtr>(
                {
    % for action in e.actions:
        % if action.pointer:
                    details::make_action(${action.bare_method()}),
        % else:
            % if action.args:
                    details::make_action(
                        ${action.bare_method()}(
                % for i, arg in enumerate(action.args):
                    % if i + 1 != len(action.args):
                            ${arg.cppArg()},
                    % else:
                            ${arg.cppArg()})),
                    % endif
                % endfor
            % else:
                        details::make_action(
                            ${action.bare_method()}()),
            % endif
        % endif
    % endfor
                }
            )
        )
    },
%endfor
};

} // namespace manager
} // namespace inventory
} // namespace phosphor
