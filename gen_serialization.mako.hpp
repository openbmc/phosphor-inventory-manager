## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// This file was auto generated.  Do not edit.

#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include "config.h"
% for iface in interfaces:
#include <${iface.header()}>
% endfor

% for iface in interfaces:
<% properties = interface_composite.names(str(iface)) %>\
CEREAL_CLASS_VERSION(${iface.namespace()}, CLASS_VERSION);
% endfor

// Emitting signals prior to claiming a well known DBus service name causes
// un-necessary DBus traffic and wakeups.  De-serialization only happens prior
// to claiming a well known name, so don't emit signals.
static constexpr auto skipSignals = true;
namespace cereal
{

% for iface in interfaces:
<% properties = interface_composite.names(str(iface)) %>\
template<class Archive>
void save(Archive& a,
          const ${iface.namespace()}& object,
          const std::uint32_t version)
{
<%
    props = ["object." + p.camelCase + "()" for p in properties]
    props = ', '.join(props)
%>\
    a(${props});
}


template<class Archive>
void load(Archive& a,
          ${iface.namespace()}& object,
          const std::uint32_t version)
{
% for p in properties:
<% t = "object." + p.camelCase + "()" %>\
    decltype(${t}) ${p.CamelCase}{};
% endfor
<%
    props = ', '.join([p.CamelCase for p in properties])
%>\
    a(${props});
% for p in properties:
<% t = "object." + p.camelCase + "(" + p.CamelCase + ", skipSignals)" %>\
    ${t};
% endfor
}

% endfor
} // namespace cereal
