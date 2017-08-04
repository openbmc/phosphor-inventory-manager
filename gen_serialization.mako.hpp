## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// This file was auto generated.  Do not edit.

#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
% for iface in interfaces:
#include <${iface.header()}>
% endfor

namespace cereal
{

% for iface in interfaces:
<% properties = interface_composite.names(str(iface)) %>\
template<class Archive>
void save(Archive& a,
          const ${iface.namespace()}& object)
{
<%
    props = ["object." + p[:1].lower() + p[1:] + "()" for p in properties]
    props = ', '.join(props)
%>\
    a(${props});
}


template<class Archive>
void load(Archive& a,
          ${iface.namespace()}& object)
{
% for p in properties:
<% t = "object." + p[:1].lower() + p[1:] + "()" %>\
    decltype(${t}) ${p}{};
% endfor
<%
    props = ', '.join(properties)
%>\
    a(${props});
% for p in properties:
<% t = "object." + p[:1].lower() + p[1:] + "(" + p + ")" %>\
    ${t};
% endfor
}

% endfor
} // namespace cereal
