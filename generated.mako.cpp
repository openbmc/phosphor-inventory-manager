## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// This file was auto generated.  Do not edit.

#include "manager.hpp"
% for i in interfaces:
#include <${'/'.join(i.split('.') + ['server.hpp'])}>
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
        "${i}",
        details::interface::holder::Holder<
            sdbusplus::server::${'::'.join(i.split('.'))}>::make,
    },
% endfor
};

const Manager::Events Manager::_events{
% for e in events:
    {
    % if e.get('description'):
        // ${e['description']}
    % endif
        "${e['name']}",
        std::make_tuple(
        % for i, s in enumerate(e['signature'].items()):
            % if i + 1 == len(e['signature']):
            ${'"{0}=\'{1}\'"'.format(*s)},
            % else:
            ${'"{0}=\'{1}\',"'.format(*s)}
            % endif
        % endfor
            % if e['filter'].get('args'):
            filters::${e['filter']['type']}(
                % for i, a in enumerate(e['filter']['args']):
                    % if i + 1 == len(e['filter']['args']):
                "${a['value']}"),
                    % else:
                "${a['value']}",
                    % endif
                % endfor
            % else:
            filters::${e['filter']['type']},
            % endif
            % if e['action'].get('args'):
            actions::${e['action']['type']}(
                % for i, a in enumerate(e['action']['args']):
                    % if i + 1 == len(e['action']['args']):
                "${a['value']}")
                    % else:
                "${a['value']}",
                    % endif
                % endfor
            % else:
            actions::${e['action']['type']}
            % endif
        ),
    },
% endfor
};

} // namespace manager
} // namespace inventory
} // namespace phosphor
