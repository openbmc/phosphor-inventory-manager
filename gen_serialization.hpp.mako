## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// This file was auto generated.  Do not edit.

#include "config.h"

#include <cereal/types/map.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>
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
// The version we started using cereal NVP from
static constexpr size_t CLASS_VERSION_WITH_NVP = 2;

% for iface in interfaces:
<% properties = interface_composite.names(str(iface)) %>\
template<class Archive>
void save([[maybe_unused]] Archive& a,
          [[maybe_unused]] const ${iface.namespace()}& object,
          const std::uint32_t /* version */)
{
% for p in properties:
<% t = "cereal::make_nvp(\"" + p.CamelCase + "\", object." + p.camelCase + "())"
%>\
        a(${t});
% endfor
}

template<class Archive>
void load(Archive& a,
          [[maybe_unused]] ${iface.namespace()}& object,
          const std::uint32_t version)
{
% for p in properties:
<% t = "object." + p.camelCase + "()" %>\
    decltype(${t}) ${p.CamelCase}{};
% endfor
    if (version < CLASS_VERSION_WITH_NVP)
    {
<%
    props = ', '.join([p.CamelCase for p in properties])
%>\
        a(${props});
    }
    else
    {
% for p in properties:
<% t = "cereal::make_nvp(\"" + p.CamelCase + "\", " + p.CamelCase + ")" %>\
        try
        {
            a(${t});
        }
        catch (const Exception &e)
        {
            // Ignore any exceptions, property value stays default
        }
% endfor
    }
% for p in properties:
<% t = "object." + p.camelCase + "(" + p.CamelCase + ", skipSignals)" %>\
    ${t};
% endfor
}

% endfor
} // namespace cereal
