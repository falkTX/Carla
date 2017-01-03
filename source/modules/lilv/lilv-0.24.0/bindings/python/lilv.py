"""Lilv Python interface"""

__author__     = "David Robillard"
__copyright__  = "Copyright 2016 David Robillard"
__license__    = "ISC"
__version__    = "0.22.1"
__maintainer__ = "David Robillard"
__email__      = "d@drobilla.net"
__status__     = "Production"

import ctypes
import os
import sys

from ctypes import Structure, CDLL, POINTER, CFUNCTYPE
from ctypes import c_bool, c_double, c_float, c_int, c_size_t, c_uint, c_uint32
from ctypes import c_char, c_char_p, c_void_p
from ctypes import byref

# Load lilv library

_lib = CDLL("liblilv-0.so")

# Set namespaced aliases for all lilv functions

class String(str):
    # Wrapper for string parameters to pass as raw C UTF-8 strings
    def from_param(cls, obj):
        return obj.encode('utf-8')

    from_param = classmethod(from_param)

def _as_uri(obj):
    if type(obj) in [Plugin, PluginClass, UI]:
        return obj.get_uri()
    else:
        return obj

free                             = _lib.lilv_free
# uri_to_path                      = _lib.lilv_uri_to_path
file_uri_parse                   = _lib.lilv_file_uri_parse
new_uri                          = _lib.lilv_new_uri
new_file_uri                     = _lib.lilv_new_file_uri
new_string                       = _lib.lilv_new_string
new_int                          = _lib.lilv_new_int
new_float                        = _lib.lilv_new_float
new_bool                         = _lib.lilv_new_bool
node_free                        = _lib.lilv_node_free
node_duplicate                   = _lib.lilv_node_duplicate
node_equals                      = _lib.lilv_node_equals
node_get_turtle_token            = _lib.lilv_node_get_turtle_token
node_is_uri                      = _lib.lilv_node_is_uri
node_as_uri                      = _lib.lilv_node_as_uri
node_is_blank                    = _lib.lilv_node_is_blank
node_as_blank                    = _lib.lilv_node_as_blank
node_is_literal                  = _lib.lilv_node_is_literal
node_is_string                   = _lib.lilv_node_is_string
node_as_string                   = _lib.lilv_node_as_string
node_get_path                    = _lib.lilv_node_get_path
node_is_float                    = _lib.lilv_node_is_float
node_as_float                    = _lib.lilv_node_as_float
node_is_int                      = _lib.lilv_node_is_int
node_as_int                      = _lib.lilv_node_as_int
node_is_bool                     = _lib.lilv_node_is_bool
node_as_bool                     = _lib.lilv_node_as_bool
plugin_classes_free              = _lib.lilv_plugin_classes_free
plugin_classes_size              = _lib.lilv_plugin_classes_size
plugin_classes_begin             = _lib.lilv_plugin_classes_begin
plugin_classes_get               = _lib.lilv_plugin_classes_get
plugin_classes_next              = _lib.lilv_plugin_classes_next
plugin_classes_is_end            = _lib.lilv_plugin_classes_is_end
plugin_classes_get_by_uri        = _lib.lilv_plugin_classes_get_by_uri
scale_points_free                = _lib.lilv_scale_points_free
scale_points_size                = _lib.lilv_scale_points_size
scale_points_begin               = _lib.lilv_scale_points_begin
scale_points_get                 = _lib.lilv_scale_points_get
scale_points_next                = _lib.lilv_scale_points_next
scale_points_is_end              = _lib.lilv_scale_points_is_end
uis_free                         = _lib.lilv_uis_free
uis_size                         = _lib.lilv_uis_size
uis_begin                        = _lib.lilv_uis_begin
uis_get                          = _lib.lilv_uis_get
uis_next                         = _lib.lilv_uis_next
uis_is_end                       = _lib.lilv_uis_is_end
uis_get_by_uri                   = _lib.lilv_uis_get_by_uri
nodes_free                       = _lib.lilv_nodes_free
nodes_size                       = _lib.lilv_nodes_size
nodes_begin                      = _lib.lilv_nodes_begin
nodes_get                        = _lib.lilv_nodes_get
nodes_next                       = _lib.lilv_nodes_next
nodes_is_end                     = _lib.lilv_nodes_is_end
nodes_get_first                  = _lib.lilv_nodes_get_first
nodes_contains                   = _lib.lilv_nodes_contains
nodes_merge                      = _lib.lilv_nodes_merge
plugins_size                     = _lib.lilv_plugins_size
plugins_begin                    = _lib.lilv_plugins_begin
plugins_get                      = _lib.lilv_plugins_get
plugins_next                     = _lib.lilv_plugins_next
plugins_is_end                   = _lib.lilv_plugins_is_end
plugins_get_by_uri               = _lib.lilv_plugins_get_by_uri
world_new                        = _lib.lilv_world_new
world_set_option                 = _lib.lilv_world_set_option
world_free                       = _lib.lilv_world_free
world_load_all                   = _lib.lilv_world_load_all
world_load_bundle                = _lib.lilv_world_load_bundle
world_load_specifications        = _lib.lilv_world_load_specifications
world_load_plugin_classes        = _lib.lilv_world_load_plugin_classes
world_unload_bundle              = _lib.lilv_world_unload_bundle
world_load_resource              = _lib.lilv_world_load_resource
world_unload_resource            = _lib.lilv_world_unload_resource
world_get_plugin_class           = _lib.lilv_world_get_plugin_class
world_get_plugin_classes         = _lib.lilv_world_get_plugin_classes
world_get_all_plugins            = _lib.lilv_world_get_all_plugins
world_find_nodes                 = _lib.lilv_world_find_nodes
world_get                        = _lib.lilv_world_get
world_ask                        = _lib.lilv_world_ask
plugin_verify                    = _lib.lilv_plugin_verify
plugin_get_uri                   = _lib.lilv_plugin_get_uri
plugin_get_bundle_uri            = _lib.lilv_plugin_get_bundle_uri
plugin_get_data_uris             = _lib.lilv_plugin_get_data_uris
plugin_get_library_uri           = _lib.lilv_plugin_get_library_uri
plugin_get_name                  = _lib.lilv_plugin_get_name
plugin_get_class                 = _lib.lilv_plugin_get_class
plugin_get_value                 = _lib.lilv_plugin_get_value
plugin_has_feature               = _lib.lilv_plugin_has_feature
plugin_get_supported_features    = _lib.lilv_plugin_get_supported_features
plugin_get_required_features     = _lib.lilv_plugin_get_required_features
plugin_get_optional_features     = _lib.lilv_plugin_get_optional_features
plugin_has_extension_data        = _lib.lilv_plugin_has_extension_data
plugin_get_extension_data        = _lib.lilv_plugin_get_extension_data
plugin_get_num_ports             = _lib.lilv_plugin_get_num_ports
plugin_get_port_ranges_float     = _lib.lilv_plugin_get_port_ranges_float
plugin_has_latency               = _lib.lilv_plugin_has_latency
plugin_get_latency_port_index    = _lib.lilv_plugin_get_latency_port_index
plugin_get_port_by_index         = _lib.lilv_plugin_get_port_by_index
plugin_get_port_by_symbol        = _lib.lilv_plugin_get_port_by_symbol
plugin_get_port_by_designation   = _lib.lilv_plugin_get_port_by_designation
plugin_get_project               = _lib.lilv_plugin_get_project
plugin_get_author_name           = _lib.lilv_plugin_get_author_name
plugin_get_author_email          = _lib.lilv_plugin_get_author_email
plugin_get_author_homepage       = _lib.lilv_plugin_get_author_homepage
plugin_is_replaced               = _lib.lilv_plugin_is_replaced
plugin_get_related               = _lib.lilv_plugin_get_related
port_get_node                    = _lib.lilv_port_get_node
port_get_value                   = _lib.lilv_port_get_value
port_get                         = _lib.lilv_port_get
port_get_properties              = _lib.lilv_port_get_properties
port_has_property                = _lib.lilv_port_has_property
port_supports_event              = _lib.lilv_port_supports_event
port_get_index                   = _lib.lilv_port_get_index
port_get_symbol                  = _lib.lilv_port_get_symbol
port_get_name                    = _lib.lilv_port_get_name
port_get_classes                 = _lib.lilv_port_get_classes
port_is_a                        = _lib.lilv_port_is_a
port_get_range                   = _lib.lilv_port_get_range
port_get_scale_points            = _lib.lilv_port_get_scale_points
state_new_from_world             = _lib.lilv_state_new_from_world
state_new_from_file              = _lib.lilv_state_new_from_file
state_new_from_string            = _lib.lilv_state_new_from_string
state_new_from_instance          = _lib.lilv_state_new_from_instance
state_free                       = _lib.lilv_state_free
state_equals                     = _lib.lilv_state_equals
state_get_num_properties         = _lib.lilv_state_get_num_properties
state_get_plugin_uri             = _lib.lilv_state_get_plugin_uri
state_get_uri                    = _lib.lilv_state_get_uri
state_get_label                  = _lib.lilv_state_get_label
state_set_label                  = _lib.lilv_state_set_label
state_set_metadata               = _lib.lilv_state_set_metadata
state_emit_port_values           = _lib.lilv_state_emit_port_values
state_restore                    = _lib.lilv_state_restore
state_save                       = _lib.lilv_state_save
state_to_string                  = _lib.lilv_state_to_string
state_delete                     = _lib.lilv_state_delete
scale_point_get_label            = _lib.lilv_scale_point_get_label
scale_point_get_value            = _lib.lilv_scale_point_get_value
plugin_class_get_parent_uri      = _lib.lilv_plugin_class_get_parent_uri
plugin_class_get_uri             = _lib.lilv_plugin_class_get_uri
plugin_class_get_label           = _lib.lilv_plugin_class_get_label
plugin_class_get_children        = _lib.lilv_plugin_class_get_children
plugin_instantiate               = _lib.lilv_plugin_instantiate
instance_free                    = _lib.lilv_instance_free
plugin_get_uis                   = _lib.lilv_plugin_get_uis
ui_get_uri                       = _lib.lilv_ui_get_uri
ui_get_classes                   = _lib.lilv_ui_get_classes
ui_is_a                          = _lib.lilv_ui_is_a
ui_is_supported                  = _lib.lilv_ui_is_supported
ui_get_bundle_uri                = _lib.lilv_ui_get_bundle_uri
ui_get_binary_uri                = _lib.lilv_ui_get_binary_uri

## LV2 types

LV2_Handle            = POINTER(None)
LV2_URID_Map_Handle   = POINTER(None)
LV2_URID_Unmap_Handle = POINTER(None)
LV2_URID              = c_uint32

class LV2_Feature(Structure):
    __slots__ = [ 'URI', 'data' ]
    _fields_  = [('URI', c_char_p),
                 ('data', POINTER(None))]

class LV2_Descriptor(Structure):
    __slots__ = [ 'URI',
                  'instantiate',
                  'connect_port',
                  'activate',
                  'run',
                  'deactivate',
                  'cleanup',
                  'extension_data' ]

LV2_Descriptor._fields_ = [
    ('URI', c_char_p),
    ('instantiate', CFUNCTYPE(LV2_Handle, POINTER(LV2_Descriptor),
                              c_double, c_char_p, POINTER(POINTER(LV2_Feature)))),
    ('connect_port', CFUNCTYPE(None, LV2_Handle, c_uint32, POINTER(None))),
    ('activate', CFUNCTYPE(None, LV2_Handle)),
    ('run', CFUNCTYPE(None, LV2_Handle, c_uint32)),
    ('deactivate', CFUNCTYPE(None, LV2_Handle)),
    ('cleanup', CFUNCTYPE(None, LV2_Handle)),
    ('extension_data', CFUNCTYPE(c_void_p, c_char_p)),
]

class LV2_URID_Map(Structure):
    __slots__ = [ 'handle', 'map' ]
    _fields_  = [
        ('handle', LV2_URID_Map_Handle),
        ('map', CFUNCTYPE(LV2_URID, LV2_URID_Map_Handle, c_char_p)),
    ]

class LV2_URID_Unmap(Structure):
    __slots__ = [ 'handle', 'unmap' ]
    _fields_  = [
        ('handle', LV2_URID_Unmap_Handle),
        ('unmap', CFUNCTYPE(c_char_p, LV2_URID_Unmap_Handle, LV2_URID)),
    ]

# Lilv types

class Plugin(Structure):
    """LV2 Plugin."""
    def __init__(self, world, plugin):
        self.world  = world
        self.plugin = plugin

    def __eq__(self, other):
        return self.get_uri() == other.get_uri()

    def verify(self):
        """Check if `plugin` is valid.

        This is not a rigorous validator, but can be used to reject some malformed
        plugins that could cause bugs (e.g. plugins with missing required fields).

        Note that normal hosts do NOT need to use this - lilv does not
        load invalid plugins into plugin lists.  This is included for plugin
        testing utilities, etc.
        """
        return plugin_verify(self.plugin)

    def get_uri(self):
        """Get the URI of `plugin`.

        Any serialization that refers to plugins should refer to them by this.
        Hosts SHOULD NOT save any filesystem paths, plugin indexes, etc. in saved
        files pass save only the URI.

        The URI is a globally unique identifier for one specific plugin.  Two
        plugins with the same URI are compatible in port signature, and should
        be guaranteed to work in a compatible and consistent way.  If a plugin
        is upgraded in an incompatible way (eg if it has different ports), it
        MUST have a different URI than it's predecessor.
        """
        return Node.wrap(node_duplicate(plugin_get_uri(self.plugin)))

    def get_bundle_uri(self):
        """Get the (resolvable) URI of the plugin's "main" bundle.

        This returns the URI of the bundle where the plugin itself was found.  Note
        that the data for a plugin may be spread over many bundles, that is,
        get_data_uris() may return URIs which are not within this bundle.

        Typical hosts should not need to use this function.
        Note this always returns a fully qualified URI.  If you want a local
        filesystem path, use lilv.file_uri_parse().
        """
        return Node.wrap(node_duplicate(plugin_get_bundle_uri(self.plugin)))

    def get_data_uris(self):
        """Get the (resolvable) URIs of the RDF data files that define a plugin.

        Typical hosts should not need to use this function.
        Note this always returns fully qualified URIs.  If you want local
        filesystem paths, use lilv.file_uri_parse().
        """
        return Nodes(plugin_get_data_uris(self.plugin))

    def get_library_uri(self):
        """Get the (resolvable) URI of the shared library for `plugin`.

        Note this always returns a fully qualified URI.  If you want a local
        filesystem path, use lilv.file_uri_parse().
        """
        return Node.wrap(node_duplicate(plugin_get_library_uri(self.plugin)))

    def get_name(self):
        """Get the name of `plugin`.

        This returns the name (doap:name) of the plugin.  The name may be
        translated according to the current locale, this value MUST NOT be used
        as a plugin identifier (use the URI for that).
        """
        return Node.wrap(plugin_get_name(self.plugin))

    def get_class(self):
        """Get the class this plugin belongs to (e.g. Filters)."""
        return PluginClass(plugin_get_class(self.plugin))

    def get_value(self, predicate):
        """Get a value associated with the plugin in a plugin's data files.

        `predicate` must be either a URI or a QName.

        Returns the ?object of all triples found of the form:

        plugin-uri predicate ?object

        May return None if the property was not found, or if object(s) is not
        sensibly represented as a LilvNodes (e.g. blank nodes).
        """
        return Nodes(plugin_get_value(self.plugin, predicate.node))

    def has_feature(self, feature_uri):
        """Return whether a feature is supported by a plugin.

        This will return true if the feature is an optional or required feature
        of the plugin.
        """
        return plugin_has_feature(self.plugin, feature_uri.node)

    def get_supported_features(self):
        """Get the LV2 Features supported (required or optionally) by a plugin.

        A feature is "supported" by a plugin if it is required OR optional.

        Since required features have special rules the host must obey, this function
        probably shouldn't be used by normal hosts.  Using get_optional_features()
        and get_required_features() separately is best in most cases.
        """
        return Nodes(plugin_get_supported_features(self.plugin))

    def get_required_features(self):
        """Get the LV2 Features required by a plugin.

        If a feature is required by a plugin, hosts MUST NOT use the plugin if they do not
        understand (or are unable to support) that feature.

        All values returned here MUST be return plugin_(self.plugin)ed to the plugin's instantiate method
        (along with data, if necessary, as defined by the feature specification)
        or plugin instantiation will fail.
        """
        return Nodes(plugin_get_required_features(self.plugin))

    def get_optional_features(self):
        """Get the LV2 Features optionally supported by a plugin.

        Hosts MAY ignore optional plugin features for whatever reasons.  Plugins
        MUST operate (at least somewhat) if they are instantiated without being
        passed optional features.
        """
        return Nodes(plugin_get_optional_features(self.plugin))

    def has_extension_data(self, uri):
        """Return whether or not a plugin provides a specific extension data."""
        return plugin_has_extension_data(self.plugin, uri.node)

    def get_extension_data(self):
        """Get a sequence of all extension data provided by a plugin.

        This can be used to find which URIs get_extension_data()
        will return a value for without instantiating the plugin.
        """
        return Nodes(plugin_get_extension_data(self.plugin))

    def get_num_ports(self):
        """Get the number of ports on this plugin."""
        return plugin_get_num_ports(self.plugin)

    # def get_port_ranges_float(self, min_values, max_values, def_values):
    #     """Get the port ranges (minimum, maximum and default values) for all ports.

    #     `min_values`, `max_values` and `def_values` must either point to an array
    #     of N floats, where N is the value returned by get_num_ports()
    #     for this plugin, or None.  The elements of the array will be set to the
    #     the minimum, maximum and default values of the ports on this plugin,
    #     with array index corresponding to port index.  If a port doesn't have a
    #     minimum, maximum or default value, or the port's type is not float, the
    #     corresponding array element will be set to NAN.

    #     This is a convenience method for the common case of getting the range of
    #     all float ports on a plugin, and may be significantly faster than
    #     repeated calls to Port.get_range().
    #     """
    #     plugin_get_port_ranges_float(self.plugin, min_values, max_values, def_values)

    def get_num_ports_of_class(self, *args):
        """Get the number of ports on this plugin that are members of some class(es)."""
        args = list(map(lambda x: x.node, args))
        args += (None,)
        return plugin_get_num_ports_of_class(self.plugin, *args)

    def has_latency(self):
        """Return whether or not the plugin introduces (and reports) latency.

        The index of the latency port can be found with
        get_latency_port() ONLY if this function returns true.
        """
        return plugin_has_latency(self.plugin)

    def get_latency_port_index(self):
        """Return the index of the plugin's latency port.

        Returns None if the plugin has no latency port.

        Any plugin that introduces unwanted latency that should be compensated for
        (by hosts with the ability/need) MUST provide this port, which is a control
        rate output port that reports the latency for each cycle in frames.
        """
        return plugin_get_latency_port_index(self.plugin) if self.has_latency() else None

    def get_port(self, key):
        """Get a port on `plugin` by index or symbol."""
        if type(key) == int:
            return self.get_port_by_index(key)
        else:
            return self.get_port_by_symbol(key)

    def get_port_by_index(self, index):
        """Get a port on `plugin` by `index`."""
        return Port.wrap(self, plugin_get_port_by_index(self.plugin, index))

    def get_port_by_symbol(self, symbol):
        """Get a port on `plugin` by `symbol`.

        Note this function is slower than get_port_by_index(),
        especially on plugins with a very large number of ports.
        """
        if type(symbol) == str:
            symbol = self.world.new_string(symbol)
        return Port.wrap(self, plugin_get_port_by_symbol(self.plugin, symbol.node))

    def get_port_by_designation(self, port_class, designation):
        """Get a port on `plugin` by its lv2:designation.

        The designation of a port describes the meaning, assignment, allocation or
        role of the port, e.g. "left channel" or "gain".  If found, the port with
        matching `port_class` and `designation` is be returned, otherwise None is
        returned.  The `port_class` can be used to distinguish the input and output
        ports for a particular designation.  If `port_class` is None, any port with
        the given designation will be returned.
        """
        return Port.wrap(self,
                         plugin_get_port_by_designation(self.plugin,
                                                        port_class.node,
                                                        designation.node))

    def get_project(self):
        """Get the project the plugin is a part of.

        More information about the project can be read via find_nodes(),
        typically using properties from DOAP (e.g. doap:name).
        """
        return Node.wrap(plugin_get_project(self.plugin))

    def get_author_name(self):
        """Get the full name of the plugin's author.

        Returns None if author name is not present.
        """
        return Node.wrap(plugin_get_author_name(self.plugin))

    def get_author_email(self):
        """Get the email address of the plugin's author.

        Returns None if author email address is not present.
        """
        return Node.wrap(plugin_get_author_email(self.plugin))

    def get_author_homepage(self):
        """Get the address of the plugin author's home page.

        Returns None if author homepage is not present.
        """
        return Node.wrap(plugin_get_author_homepage(self.plugin))

    def is_replaced(self):
        """Return true iff `plugin` has been replaced by another plugin.

        The plugin will still be usable, but hosts should hide them from their
        user interfaces to prevent users from using deprecated plugins.
        """
        return plugin_is_replaced(self.plugin)

    def get_related(self, resource_type):
        """Get the resources related to `plugin` with lv2:appliesTo.

        Some plugin-related resources are not linked directly to the plugin with
        rdfs:seeAlso and thus will not be automatically loaded along with the plugin
        data (usually for performance reasons).  All such resources of the given @c
        type related to `plugin` can be accessed with this function.

        If `resource_type` is None, all such resources will be returned, regardless of type.

        To actually load the data for each returned resource, use world.load_resource().
        """
        return Nodes(plugin_get_related(self.plugin, resource_type))

    def get_uis(self):
        """Get all UIs for `plugin`."""
        return UIs(plugin_get_uis(self.plugin))

class PluginClass(Structure):
    """Plugin Class (type/category)."""
    def __init__(self, plugin_class):
        self.plugin_class = plugin_class

    def __str__(self):
        return self.get_uri().__str__()

    def get_parent_uri(self):
        """Get the URI of this class' superclass.

           May return None if class has no parent.
        """
        return Node.wrap(node_duplicate(plugin_class_get_parent_uri(self.plugin_class)))

    def get_uri(self):
        """Get the URI of this plugin class."""
        return Node.wrap(node_duplicate(plugin_class_get_uri(self.plugin_class)))

    def get_label(self):
        """Get the label of this plugin class, ie "Oscillators"."""
        return Node.wrap(node_duplicate(plugin_class_get_label(self.plugin_class)))

    def get_children(self):
        """Get the subclasses of this plugin class."""
        return PluginClasses(plugin_class_get_children(self.plugin_class))

class Port(Structure):
    """Port on a Plugin."""
    @classmethod
    def wrap(cls, plugin, port):
        return Port(plugin, port) if plugin and port else None

    def __init__(self, plugin, port):
        self.plugin = plugin
        self.port   = port

    def get_node(self):
        """Get the RDF node of `port`.

        Ports nodes may be may be URIs or blank nodes.
        """
        return Node.wrap(node_duplicate(port_get_node(self.plugin, self.port)))

    def get_value(self, predicate):
        """Port analog of Plugin.get_value()."""
        return Nodes(port_get_value(self.plugin.plugin, self.port, predicate.node))

    def get(self, predicate):
        """Get a single property value of a port.

        This is equivalent to lilv_nodes_get_first(lilv_port_get_value(...)) but is
        simpler to use in the common case of only caring about one value.  The
        caller is responsible for freeing the returned node.
        """
        return Node.wrap(port_get(self.plugin.plugin, self.port, predicate.node))

    def get_properties(self):
        """Return the LV2 port properties of a port."""
        return Nodes(port_get_properties(self.plugin.plugin, self.port))

    def has_property(self, property_uri):
        """Return whether a port has a certain property."""
        return port_has_property(self.plugin.plugin, self.port, property_uri.node)

    def supports_event(self, event_type):
        """Return whether a port supports a certain event type.

        More precisely, this returns true iff the port has an atom:supports or an
        ev:supportsEvent property with `event_type` as the value.
        """
        return port_supports_event(self.plugin.plugin, self.port, event_type.node)

    def get_index(self):
        """Get the index of a port.

        The index is only valid for the life of the plugin and may change between
        versions.  For a stable identifier, use the symbol.
        """
        return port_get_index(self.plugin.plugin, self.port)

    def get_symbol(self):
        """Get the symbol of a port.

        The 'symbol' is a short string, a valid C identifier.
        """
        return Node.wrap(node_duplicate(port_get_symbol(self.plugin.plugin, self.port)))

    def get_name(self):
        """Get the name of a port.

        This is guaranteed to return the untranslated name (the doap:name in the
        data file without a language tag).
        """
        return Node.wrap(port_get_name(self.plugin.plugin, self.port))

    def get_classes(self):
        """Get all the classes of a port.

        This can be used to determine if a port is an input, output, audio,
        control, midi, etc, etc, though it's simpler to use is_a().
        The returned list does not include lv2:Port, which is implied.
        Returned value is shared and must not be destroyed by caller.
        """
        return Nodes(port_get_classes(self.plugin.plugin, self.port))

    def is_a(self, port_class):
        """Determine if a port is of a given class (input, output, audio, etc).

        For convenience/performance/extensibility reasons, hosts are expected to
        create a LilvNode for each port class they "care about".  Well-known type
        URI strings are defined (e.g. LILV_URI_INPUT_PORT) for convenience, but
        this function is designed so that Lilv is usable with any port types
        without requiring explicit support in Lilv.
        """
        return port_is_a(self.plugin.plugin, self.port, port_class.node)

    def get_range(self):
        """Return the default, minimum, and maximum values of a port as a tuple."""
        pdef = POINTER(Node)()
        pmin = POINTER(Node)()
        pmax = POINTER(Node)()
        port_get_range(self.plugin.plugin, self.port, byref(pdef), byref(pmin), byref(pmax))
        return (Node(pdef.contents) if pdef else None,
                Node(pmin.contents) if pmin else None,
                Node(pmax.contents) if pmax else None)

    def get_scale_points(self):
        """Get the scale points (enumeration values) of a port.

        This returns a collection of 'interesting' named values of a port
        (e.g. appropriate entries for a UI selector associated with this port).
        Returned value may be None if `port` has no scale points.
        """
        return ScalePoints(port_get_scale_points(self.plugin.plugin, self.port))

class ScalePoint(Structure):
    """Scale point (detent)."""
    def __init__(self, point):
        self.point = point

    def get_label(self):
        """Get the label of this scale point (enumeration value)."""
        return Node.wrap(scale_point_get_label(self.point))

    def get_value(self):
        """Get the value of this scale point (enumeration value)."""
        return Node.wrap(scale_point_get_value(self.point))

class UI(Structure):
    """Plugin UI."""
    def __init__(self, ui):
        self.ui = ui

    def __str__(self):
        return str(self.get_uri())

    def __eq__(self, other):
        return self.get_uri() == _as_uri(other)

    def get_uri(self):
        """Get the URI of a Plugin UI."""
        return Node.wrap(node_duplicate(ui_get_uri(self.ui)))

    def get_classes(self):
        """Get the types (URIs of RDF classes) of a Plugin UI.

        Note that in most cases is_supported() should be used, which avoids
           the need to use this function (and type specific logic).
        """
        return Nodes(ui_get_classes(self.ui))

    def is_a(self, class_uri):
        """Check whether a plugin UI has a given type."""
        return ui_is_a(self.ui, class_uri.node)

    def get_bundle_uri(self):
        """Get the URI of the UI's bundle."""
        return Node.wrap(node_duplicate(ui_get_bundle_uri(self.ui)))

    def get_binary_uri(self):
        """Get the URI for the UI's shared library."""
        return Node.wrap(node_duplicate(ui_get_binary_uri(self.ui)))

class Node(Structure):
    """Data node (URI, string, integer, etc.).

    A Node can be converted to the corresponding Python datatype, and all nodes
    can be converted to strings, for example::

       >>> world = lilv.World()
       >>> i = world.new_int(42)
       >>> print(i)
       42
       >>> int(i) * 2
       84
    """
    @classmethod
    def wrap(cls, node):
        return Node(node) if node else None

    def __init__(self, node):
        self.node = node

    def __del__(self):
        if hasattr(self, 'node'):
            node_free(self.node)

    def __eq__(self, other):
        otype = type(other)
        if otype in [str, int, float]:
            return otype(self) == other
        return node_equals(self.node, other.node)

    def __ne__(self, other):
        return not node_equals(self.node, other.node)

    def __str__(self):
        return node_as_string(self.node).decode('utf-8')

    def __int__(self):
        if not self.is_int():
            raise ValueError('node %s is not an integer' % str(self))
        return node_as_int(self.node)

    def __float__(self):
        if not self.is_float():
            raise ValueError('node %s is not a float' % str(self))
        return node_as_float(self.node)

    def __bool__(self):
        if not self.is_bool():
            raise ValueError('node %s is not a bool' % str(self))
        return node_as_bool(self.node)
    __nonzero__ = __bool__

    def get_turtle_token(self):
        """Return this value as a Turtle/SPARQL token."""
        return node_get_turtle_token(self.node).decode('utf-8')

    def is_uri(self):
        """Return whether the value is a URI (resource)."""
        return node_is_uri(self.node)

    def is_blank(self):
        """Return whether the value is a blank node (resource with no URI)."""
        return node_is_blank(self.node)

    def is_literal(self):
        """Return whether this value is a literal (i.e. not a URI)."""
        return node_is_literal(self.node)

    def is_string(self):
        """Return whether this value is a string literal.

        Returns true if value is a string value (and not numeric).
        """
        return node_is_string(self.node)

    def get_path(self, hostname=None):
        """Return the path of a file URI node.

        Returns None if value is not a file URI."""
        return node_get_path(self.node, hostname).decode('utf-8')

    def is_float(self):
        """Return whether this value is a decimal literal."""
        return node_is_float(self.node)

    def is_int(self):
        """Return whether this value is an integer literal."""
        return node_is_int(self.node)

    def is_bool(self):
        """Return whether this value is a boolean."""
        return node_is_bool(self.node)

class Iter(Structure):
    """Collection iterator."""
    def __init__(self, collection, iterator, constructor, iter_get, iter_next, iter_is_end):
        self.collection = collection
        self.iterator = iterator
        self.constructor = constructor
        self.iter_get = iter_get
        self.iter_next = iter_next
        self.iter_is_end = iter_is_end

    def get(self):
        """Get the current item."""
        return self.constructor(self.iter_get(self.collection, self.iterator))

    def next(self):
        """Move to and return the next item."""
        if self.is_end():
            raise StopIteration
        elem = self.get()
        self.iterator = self.iter_next(self.collection, self.iterator)
        return elem

    def is_end(self):
        """Return true if the end of the collection has been reached."""
        return self.iter_is_end(self.collection, self.iterator)

    __next__ = next

class Collection(Structure):
    # Base class for all lilv collection wrappers.
    def __init__(self, collection, iter_begin, constructor, iter_get, iter_next, is_end):
        self.collection = collection
        self.constructor = constructor
        self.iter_begin = iter_begin
        self.iter_get = iter_get
        self.iter_next = iter_next
        self.is_end = is_end

    def __iter__(self):
        return Iter(self.collection, self.iter_begin(self.collection), self.constructor,
                    self.iter_get, self.iter_next, self.is_end)

    def __getitem__(self, index):
        if index >= len(self):
            raise IndexError
        pos = 0
        for i in self:
            if pos == index:
                return i
            pos += 1

    def begin(self):
        return self.__iter__()

    def get(self, iterator):
        return iterator.get()

class Plugins(Collection):
    """Collection of plugins."""
    def __init__(self, world, collection):
        def constructor(plugin):
            return Plugin(world, plugin)

        super(Plugins, self).__init__(collection, plugins_begin, constructor, plugins_get, plugins_next, plugins_is_end)
        self.world = world

    def __contains__(self, key):
        return bool(self.get_by_uri(_as_uri(key)))

    def __len__(self):
        return plugins_size(self.collection)

    def __getitem__(self, key):
        if type(key) == int:
            return super(Plugins, self).__getitem__(key)
        return self.get_by_uri(key)

    def get_by_uri(self, uri):
        plugin = plugins_get_by_uri(self.collection, uri.node)
        return Plugin(self.world, plugin) if plugin else None

class PluginClasses(Collection):
    """Collection of plugin classes."""
    def __init__(self, collection):
        super(PluginClasses, self).__init__(
            collection, plugin_classes_begin, PluginClass,
            plugin_classes_get, plugin_classes_next, plugin_classes_is_end)

    def __contains__(self, key):
        return bool(self.get_by_uri(_as_uri(key)))

    def __len__(self):
        return plugin_classes_size(self.collection)

    def __getitem__(self, key):
        if type(key) == int:
            return super(PluginClasses, self).__getitem__(key)
        return self.get_by_uri(key)

    def get_by_uri(self, uri):
        plugin_class = plugin_classes_get_by_uri(self.collection, uri.node)
        return PluginClass(plugin_class) if plugin_class else None

class ScalePoints(Collection):
    """Collection of scale points."""
    def __init__(self, collection):
        super(ScalePoints, self).__init__(
            collection, scale_points_begin, ScalePoint,
            scale_points_get, scale_points_next, scale_points_is_end)

    def __len__(self):
        return scale_points_size(self.collection)

class UIs(Collection):
    """Collection of plugin UIs."""
    def __init__(self, collection):
        super(UIs, self).__init__(collection, uis_begin, UI,
                                  uis_get, uis_next, uis_is_end)

    def __contains__(self, uri):
        return bool(self.get_by_uri(_as_uri(uri)))

    def __len__(self):
        return uis_size(self.collection)

    def __getitem__(self, key):
        if type(key) == int:
            return super(UIs, self).__getitem__(key)
        return self.get_by_uri(key)

    def get_by_uri(self, uri):
        ui = uis_get_by_uri(self.collection, uri.node)
        return UI(ui) if ui else None

class Nodes(Collection):
    """Collection of data nodes."""
    @classmethod
    def constructor(ignore, node):
        return Node(node_duplicate(node))

    def __init__(self, collection):
        super(Nodes, self).__init__(collection, nodes_begin, Nodes.constructor,
                                    nodes_get, nodes_next, nodes_is_end)

    def __contains__(self, value):
        return nodes_contains(self.collection, value.node)

    def __len__(self):
        return nodes_size(self.collection)

    def merge(self, b):
        return Nodes(nodes_merge(self.collection, b.collection))

class Namespace():
    """Namespace prefix.

    Use attribute syntax to easily create URIs within this namespace, for
    example::

       >>> world = lilv.World()
       >>> ns = Namespace(world, "http://example.org/")
       >>> print(ns.foo)
       http://example.org/foo
    """
    def __init__(self, world, prefix):
        self.world  = world
        self.prefix = prefix

    def __eq__(self, other):
        return str(self) == str(other)

    def __str__(self):
        return self.prefix

    def __getattr__(self, suffix):
        return self.world.new_uri(self.prefix + suffix)

class World(Structure):
    """Library context.

    Includes a set of namespaces as the instance variable `ns`, so URIs can be constructed like::

        uri = world.ns.lv2.Plugin

    :ivar ns: Common LV2 namespace prefixes: atom, doap, foaf, lilv, lv2, midi, owl, rdf, rdfs, ui, xsd.
    """
    def __init__(self):
        world = self

        # Define Namespaces class locally so available prefixes are documented
        class Namespaces():
            """Set of namespaces.

            Use to easily construct uris, like: ns.lv2.InputPort"""

            atom = Namespace(world, 'http://lv2plug.in/ns/ext/atom#')
            doap = Namespace(world, 'http://usefulinc.com/ns/doap#')
            foaf = Namespace(world, 'http://xmlns.com/foaf/0.1/')
            lilv = Namespace(world, 'http://drobilla.net/ns/lilv#')
            lv2  = Namespace(world, 'http://lv2plug.in/ns/lv2core#')
            midi = Namespace(world, 'http://lv2plug.in/ns/ext/midi#')
            owl  = Namespace(world, 'http://www.w3.org/2002/07/owl#')
            rdf  = Namespace(world, 'http://www.w3.org/1999/02/22-rdf-syntax-ns#')
            rdfs = Namespace(world, 'http://www.w3.org/2000/01/rdf-schema#')
            ui   = Namespace(world, 'http://lv2plug.in/ns/extensions/ui#')
            xsd  = Namespace(world, 'http://www.w3.org/2001/XMLSchema#')

        self.world = _lib.lilv_world_new()
        self.ns    = Namespaces()

    def __del__(self):
        world_free(self.world)

    def set_option(self, uri, value):
        """Set a world option.

        Currently recognized options:
        lilv.OPTION_FILTER_LANG
        lilv.OPTION_DYN_MANIFEST
        """
        return world_set_option(self, uri, value.node)

    def load_all(self):
        """Load all installed LV2 bundles on the system.

        This is the recommended way for hosts to load LV2 data.  It implements the
        established/standard best practice for discovering all LV2 data on the
        system.  The environment variable LV2_PATH may be used to control where
        this function will look for bundles.

        Hosts should use this function rather than explicitly load bundles, except
        in special circumstances (e.g. development utilities, or hosts that ship
        with special plugin bundles which are installed to a known location).
        """
        world_load_all(self.world)

    def load_bundle(self, bundle_uri):
        """Load a specific bundle.

        `bundle_uri` must be a fully qualified URI to the bundle directory,
        with the trailing slash, eg. file:///usr/lib/lv2/foo.lv2/

        Normal hosts should not need this function (use load_all()).

        Hosts MUST NOT attach any long-term significance to bundle paths
        (e.g. in save files), since there are no guarantees they will remain
        unchanged between (or even during) program invocations. Plugins (among
        other things) MUST be identified by URIs (not paths) in save files.
        """
        world_load_bundle(self.world, bundle_uri.node)

    def load_specifications(self):
        """Load all specifications from currently loaded bundles.

        This is for hosts that explicitly load specific bundles, its use is not
        necessary when using load_all().  This function parses the
        specifications and adds them to the model.
        """
        world_load_specifications(self.world)

    def load_plugin_classes(self):
        """Load all plugin classes from currently loaded specifications.

        Must be called after load_specifications().  This is for hosts
        that explicitly load specific bundles, its use is not necessary when using
        load_all().
        """
        world_load_plugin_classes(self.world)

    def unload_bundle(self, bundle_uri):
        """Unload a specific bundle.

        This unloads statements loaded by load_bundle().  Note that this
        is not necessarily all information loaded from the bundle.  If any resources
        have been separately loaded with load_resource(), they must be
        separately unloaded with unload_resource().
        """
        return world_unload_bundle(self.world, bundle_uri.node)

    def load_resource(self, resource):
        """Load all the data associated with the given `resource`.

        The resource must be a subject (i.e. a URI or a blank node).
        Returns the number of files parsed, or -1 on error.

        All accessible data files linked to `resource` with rdfs:seeAlso will be
        loaded into the world model.
        """
        return world_load_resource(self.world, _as_uri(resource).node)

    def unload_resource(self, resource):
        """Unload all the data associated with the given `resource`.

        The resource must be a subject (i.e. a URI or a blank node).

        This unloads all data loaded by a previous call to
        load_resource() with the given `resource`.
        """
        return world_unload_resource(self.world, _as_uri(resource).node)

    def get_plugin_class(self):
        """Get the parent of all other plugin classes, lv2:Plugin."""
        return PluginClass(world_get_plugin_class(self.world))

    def get_plugin_classes(self):
        """Return a list of all found plugin classes."""
        return PluginClasses(world_get_plugin_classes(self.world))

    def get_all_plugins(self):
        """Return a list of all found plugins.

        The returned list contains just enough references to query
        or instantiate plugins.  The data for a particular plugin will not be
        loaded into memory until a call to an lilv_plugin_* function results in
        a query (at which time the data is cached with the LilvPlugin so future
        queries are very fast).

        The returned list and the plugins it contains are owned by `world`
        and must not be freed by caller.
        """
        return Plugins(self, _lib.lilv_world_get_all_plugins(self.world))

    def find_nodes(self, subject, predicate, obj):
        """Find nodes matching a triple pattern.

        Either `subject` or `object` may be None (i.e. a wildcard), but not both.
        Returns all matches for the wildcard field, or None.
        """
        return Nodes(world_find_nodes(self.world,
                                      subject.node if subject is not None else None,
                                      predicate.node if predicate is not None else None,
                                      obj.node if obj is not None else None))

    def get(self, subject, predicate, obj):
        """Find a single node that matches a pattern.

        Exactly one of `subject`, `predicate`, `object` must be None.

        Returns the first matching node, or None if no matches are found.
        """
        return Node.wrap(world_get(self.world,
                                   subject.node if subject is not None else None,
                                   predicate.node if predicate is not None else None,
                                   obj.node if obj is not None else None))

    def ask(self, subject, predicate, obj):
        """Return true iff a statement matching a certain pattern exists.

        This is useful for checking if particular statement exists without having to
        bother with collections and memory management.
        """
        return world_ask(self.world,
                         subject.node if subject is not None else None,
                         predicate.node if predicate is not None else None,
                         obj.node if obj is not None else None)

    def new_uri(self, uri):
        """Create a new URI node."""
        return Node.wrap(_lib.lilv_new_uri(self.world, uri))

    def new_file_uri(self, host, path):
        """Create a new file URI node.  The host may be None."""
        return Node.wrap(_lib.lilv_new_file_uri(self.world, host, path))

    def new_string(self, string):
        """Create a new string node."""
        return Node.wrap(_lib.lilv_new_string(self.world, string))

    def new_int(self, val):
        """Create a new int node."""
        return Node.wrap(_lib.lilv_new_int(self.world, val))

    def new_float(self, val):
        """Create a new float node."""
        return Node.wrap(_lib.lilv_new_float(self.world, val))

    def new_bool(self, val):
        """Create a new bool node."""
        return Node.wrap(_lib.lilv_new_bool(self.world, val))

class Instance(Structure):
    """Plugin instance."""
    __slots__ = [ 'lv2_descriptor', 'lv2_handle', 'pimpl', 'plugin', 'rate', 'instance' ]
    _fields_  = [
        ('lv2_descriptor', POINTER(LV2_Descriptor)),
        ('lv2_handle', LV2_Handle),
        ('pimpl', POINTER(None)),
    ]

    def __init__(self, plugin, rate, features=None):
        self.plugin   = plugin
        self.rate     = rate
        self.instance = plugin_instantiate(plugin.plugin, rate, features)

    def get_uri(self):
        """Get the URI of the plugin which `instance` is an instance of.

           Returned string is shared and must not be modified or deleted.
        """
        return self.get_descriptor().URI

    def connect_port(self, port_index, data):
        """Connect a port to a data location.

           This may be called regardless of whether the plugin is activated,
           activation and deactivation does not destroy port connections.
        """
        import numpy
        if data is None:
            self.get_descriptor().connect_port(
                self.get_handle(),
                port_index,
                data)
        elif type(data) == numpy.ndarray:
            self.get_descriptor().connect_port(
                self.get_handle(),
                port_index,
                data.ctypes.data_as(POINTER(c_float)))
        else:
            raise Exception("Unsupported data type")

    def activate(self):
        """Activate a plugin instance.

           This resets all state information in the plugin, except for port data
           locations (as set by connect_port()).  This MUST be called
           before calling run().
        """
        if self.get_descriptor().activate:
            self.get_descriptor().activate(self.get_handle())

    def run(self, sample_count):
        """Run `instance` for `sample_count` frames.

           If the hint lv2:hardRTCapable is set for this plugin, this function is
           guaranteed not to block.
        """
        self.get_descriptor().run(self.get_handle(), sample_count)

    def deactivate(self):
        """Deactivate a plugin instance.

           Note that to run the plugin after this you must activate it, which will
           reset all state information (except port connections).
        """
        if self.get_descriptor().deactivate:
            self.get_descriptor().deactivate(self.get_handle())

    def get_extension_data(self, uri):
        """Get extension data from the plugin instance.

           The type and semantics of the data returned is specific to the particular
           extension, though in all cases it is shared and must not be deleted.
        """
        if self.get_descriptor().extension_data:
            return self.get_descriptor().extension_data(str(uri))

    def get_descriptor(self):
        """Get the LV2_Descriptor of the plugin instance.

           Normally hosts should not need to access the LV2_Descriptor directly,
           use the lilv_instance_* functions.
        """
        return self.instance[0].lv2_descriptor[0]

    def get_handle(self):
        """Get the LV2_Handle of the plugin instance.

           Normally hosts should not need to access the LV2_Handle directly,
           use the lilv_instance_* functions.
        """
        return self.instance[0].lv2_handle

class State(Structure):
    """Plugin state (TODO)."""
    pass

class VariadicFunction(object):
    # Wrapper for calling C variadic functions
    def __init__(self, function, restype, argtypes):
        self.function         = function
        self.function.restype = restype
        self.argtypes         = argtypes

    def __call__(self, *args):
        fixed_args = []
        i          = 0
        for argtype in self.argtypes:
            fixed_args.append(argtype.from_param(args[i]))
            i += 1
        return self.function(*fixed_args + list(args[i:]))

# Set return and argument types for lilv C functions

free.argtypes = [POINTER(None)]
free.restype  = None

# uri_to_path.argtypes = [String]
# uri_to_path.restype  = c_char_p

file_uri_parse.argtypes = [String, POINTER(POINTER(c_char))]
file_uri_parse.restype  = c_char_p

new_uri.argtypes = [POINTER(World), String]
new_uri.restype  = POINTER(Node)

new_file_uri.argtypes = [POINTER(World), c_char_p, String]
new_file_uri.restype  = POINTER(Node)

new_string.argtypes = [POINTER(World), String]
new_string.restype  = POINTER(Node)

new_int.argtypes = [POINTER(World), c_int]
new_int.restype  = POINTER(Node)

new_float.argtypes = [POINTER(World), c_float]
new_float.restype  = POINTER(Node)

new_bool.argtypes = [POINTER(World), c_bool]
new_bool.restype  = POINTER(Node)

node_free.argtypes = [POINTER(Node)]
node_free.restype  = None

node_duplicate.argtypes = [POINTER(Node)]
node_duplicate.restype  = POINTER(Node)

node_equals.argtypes = [POINTER(Node), POINTER(Node)]
node_equals.restype  = c_bool

node_get_turtle_token.argtypes = [POINTER(Node)]
node_get_turtle_token.restype  = c_char_p

node_is_uri.argtypes = [POINTER(Node)]
node_is_uri.restype  = c_bool

node_as_uri.argtypes = [POINTER(Node)]
node_as_uri.restype  = c_char_p

node_is_blank.argtypes = [POINTER(Node)]
node_is_blank.restype  = c_bool

node_as_blank.argtypes = [POINTER(Node)]
node_as_blank.restype  = c_char_p

node_is_literal.argtypes = [POINTER(Node)]
node_is_literal.restype  = c_bool

node_is_string.argtypes = [POINTER(Node)]
node_is_string.restype  = c_bool

node_as_string.argtypes = [POINTER(Node)]
node_as_string.restype  = c_char_p

node_get_path.argtypes = [POINTER(Node), POINTER(POINTER(c_char))]
node_get_path.restype  = c_char_p

node_is_float.argtypes = [POINTER(Node)]
node_is_float.restype  = c_bool

node_as_float.argtypes = [POINTER(Node)]
node_as_float.restype  = c_float

node_is_int.argtypes = [POINTER(Node)]
node_is_int.restype  = c_bool

node_as_int.argtypes = [POINTER(Node)]
node_as_int.restype  = c_int

node_is_bool.argtypes = [POINTER(Node)]
node_is_bool.restype  = c_bool

node_as_bool.argtypes = [POINTER(Node)]
node_as_bool.restype  = c_bool

plugin_classes_free.argtypes = [POINTER(PluginClasses)]
plugin_classes_free.restype  = None

plugin_classes_size.argtypes = [POINTER(PluginClasses)]
plugin_classes_size.restype  = c_uint

plugin_classes_begin.argtypes = [POINTER(PluginClasses)]
plugin_classes_begin.restype  = POINTER(Iter)

plugin_classes_get.argtypes = [POINTER(PluginClasses), POINTER(Iter)]
plugin_classes_get.restype  = POINTER(PluginClass)

plugin_classes_next.argtypes = [POINTER(PluginClasses), POINTER(Iter)]
plugin_classes_next.restype  = POINTER(Iter)

plugin_classes_is_end.argtypes = [POINTER(PluginClasses), POINTER(Iter)]
plugin_classes_is_end.restype  = c_bool

plugin_classes_get_by_uri.argtypes = [POINTER(PluginClasses), POINTER(Node)]
plugin_classes_get_by_uri.restype  = POINTER(PluginClass)

scale_points_free.argtypes = [POINTER(ScalePoints)]
scale_points_free.restype  = None

scale_points_size.argtypes = [POINTER(ScalePoints)]
scale_points_size.restype  = c_uint

scale_points_begin.argtypes = [POINTER(ScalePoints)]
scale_points_begin.restype  = POINTER(Iter)

scale_points_get.argtypes = [POINTER(ScalePoints), POINTER(Iter)]
scale_points_get.restype  = POINTER(ScalePoint)

scale_points_next.argtypes = [POINTER(ScalePoints), POINTER(Iter)]
scale_points_next.restype  = POINTER(Iter)

scale_points_is_end.argtypes = [POINTER(ScalePoints), POINTER(Iter)]
scale_points_is_end.restype  = c_bool

uis_free.argtypes = [POINTER(UIs)]
uis_free.restype  = None

uis_size.argtypes = [POINTER(UIs)]
uis_size.restype  = c_uint

uis_begin.argtypes = [POINTER(UIs)]
uis_begin.restype  = POINTER(Iter)

uis_get.argtypes = [POINTER(UIs), POINTER(Iter)]
uis_get.restype  = POINTER(UI)

uis_next.argtypes = [POINTER(UIs), POINTER(Iter)]
uis_next.restype  = POINTER(Iter)

uis_is_end.argtypes = [POINTER(UIs), POINTER(Iter)]
uis_is_end.restype  = c_bool

uis_get_by_uri.argtypes = [POINTER(UIs), POINTER(Node)]
uis_get_by_uri.restype  = POINTER(UI)

nodes_free.argtypes = [POINTER(Nodes)]
nodes_free.restype  = None

nodes_size.argtypes = [POINTER(Nodes)]
nodes_size.restype  = c_uint

nodes_begin.argtypes = [POINTER(Nodes)]
nodes_begin.restype  = POINTER(Iter)

nodes_get.argtypes = [POINTER(Nodes), POINTER(Iter)]
nodes_get.restype  = POINTER(Node)

nodes_next.argtypes = [POINTER(Nodes), POINTER(Iter)]
nodes_next.restype  = POINTER(Iter)

nodes_is_end.argtypes = [POINTER(Nodes), POINTER(Iter)]
nodes_is_end.restype  = c_bool

nodes_get_first.argtypes = [POINTER(Nodes)]
nodes_get_first.restype  = POINTER(Node)

nodes_contains.argtypes = [POINTER(Nodes), POINTER(Node)]
nodes_contains.restype  = c_bool

nodes_merge.argtypes = [POINTER(Nodes), POINTER(Nodes)]
nodes_merge.restype  = POINTER(Nodes)

plugins_size.argtypes = [POINTER(Plugins)]
plugins_size.restype  = c_uint

plugins_begin.argtypes = [POINTER(Plugins)]
plugins_begin.restype  = POINTER(Iter)

plugins_get.argtypes = [POINTER(Plugins), POINTER(Iter)]
plugins_get.restype  = POINTER(Plugin)

plugins_next.argtypes = [POINTER(Plugins), POINTER(Iter)]
plugins_next.restype  = POINTER(Iter)

plugins_is_end.argtypes = [POINTER(Plugins), POINTER(Iter)]
plugins_is_end.restype  = c_bool

plugins_get_by_uri.argtypes = [POINTER(Plugins), POINTER(Node)]
plugins_get_by_uri.restype  = POINTER(Plugin)

world_new.argtypes = []
world_new.restype  = POINTER(World)

world_set_option.argtypes = [POINTER(World), String, POINTER(Node)]
world_set_option.restype  = None

world_free.argtypes = [POINTER(World)]
world_free.restype  = None

world_load_all.argtypes = [POINTER(World)]
world_load_all.restype  = None

world_load_bundle.argtypes = [POINTER(World), POINTER(Node)]
world_load_bundle.restype  = None

world_load_specifications.argtypes = [POINTER(World)]
world_load_specifications.restype  = None

world_load_plugin_classes.argtypes = [POINTER(World)]
world_load_plugin_classes.restype  = None

world_unload_bundle.argtypes = [POINTER(World), POINTER(Node)]
world_unload_bundle.restype  = c_int

world_load_resource.argtypes = [POINTER(World), POINTER(Node)]
world_load_resource.restype  = c_int

world_unload_resource.argtypes = [POINTER(World), POINTER(Node)]
world_unload_resource.restype  = c_int

world_get_plugin_class.argtypes = [POINTER(World)]
world_get_plugin_class.restype  = POINTER(PluginClass)

world_get_plugin_classes.argtypes = [POINTER(World)]
world_get_plugin_classes.restype  = POINTER(PluginClasses)

world_get_all_plugins.argtypes = [POINTER(World)]
world_get_all_plugins.restype  = POINTER(Plugins)

world_find_nodes.argtypes = [POINTER(World), POINTER(Node), POINTER(Node), POINTER(Node)]
world_find_nodes.restype  = POINTER(Nodes)

world_get.argtypes = [POINTER(World), POINTER(Node), POINTER(Node), POINTER(Node)]
world_get.restype  = POINTER(Node)

world_ask.argtypes = [POINTER(World), POINTER(Node), POINTER(Node), POINTER(Node)]
world_ask.restype  = c_bool

plugin_verify.argtypes = [POINTER(Plugin)]
plugin_verify.restype  = c_bool

plugin_get_uri.argtypes = [POINTER(Plugin)]
plugin_get_uri.restype  = POINTER(Node)

plugin_get_bundle_uri.argtypes = [POINTER(Plugin)]
plugin_get_bundle_uri.restype  = POINTER(Node)

plugin_get_data_uris.argtypes = [POINTER(Plugin)]
plugin_get_data_uris.restype  = POINTER(Nodes)

plugin_get_library_uri.argtypes = [POINTER(Plugin)]
plugin_get_library_uri.restype  = POINTER(Node)

plugin_get_name.argtypes = [POINTER(Plugin)]
plugin_get_name.restype  = POINTER(Node)

plugin_get_class.argtypes = [POINTER(Plugin)]
plugin_get_class.restype  = POINTER(PluginClass)

plugin_get_value.argtypes = [POINTER(Plugin), POINTER(Node)]
plugin_get_value.restype  = POINTER(Nodes)

plugin_has_feature.argtypes = [POINTER(Plugin), POINTER(Node)]
plugin_has_feature.restype  = c_bool

plugin_get_supported_features.argtypes = [POINTER(Plugin)]
plugin_get_supported_features.restype  = POINTER(Nodes)

plugin_get_required_features.argtypes = [POINTER(Plugin)]
plugin_get_required_features.restype  = POINTER(Nodes)

plugin_get_optional_features.argtypes = [POINTER(Plugin)]
plugin_get_optional_features.restype  = POINTER(Nodes)

plugin_has_extension_data.argtypes = [POINTER(Plugin), POINTER(Node)]
plugin_has_extension_data.restype  = c_bool

plugin_get_extension_data.argtypes = [POINTER(Plugin)]
plugin_get_extension_data.restype  = POINTER(Nodes)

plugin_get_num_ports.argtypes = [POINTER(Plugin)]
plugin_get_num_ports.restype  = c_uint32

plugin_get_port_ranges_float.argtypes = [POINTER(Plugin), POINTER(c_float), POINTER(c_float), POINTER(c_float)]
plugin_get_port_ranges_float.restype  = None

plugin_get_num_ports_of_class = VariadicFunction(_lib.lilv_plugin_get_num_ports_of_class,
                                                 c_uint32,
                                                 [POINTER(Plugin), POINTER(Node)])

plugin_has_latency.argtypes = [POINTER(Plugin)]
plugin_has_latency.restype  = c_bool

plugin_get_latency_port_index.argtypes = [POINTER(Plugin)]
plugin_get_latency_port_index.restype  = c_uint32

plugin_get_port_by_index.argtypes = [POINTER(Plugin), c_uint32]
plugin_get_port_by_index.restype  = POINTER(Port)

plugin_get_port_by_symbol.argtypes = [POINTER(Plugin), POINTER(Node)]
plugin_get_port_by_symbol.restype  = POINTER(Port)

plugin_get_port_by_designation.argtypes = [POINTER(Plugin), POINTER(Node), POINTER(Node)]
plugin_get_port_by_designation.restype  = POINTER(Port)

plugin_get_project.argtypes = [POINTER(Plugin)]
plugin_get_project.restype  = POINTER(Node)

plugin_get_author_name.argtypes = [POINTER(Plugin)]
plugin_get_author_name.restype  = POINTER(Node)

plugin_get_author_email.argtypes = [POINTER(Plugin)]
plugin_get_author_email.restype  = POINTER(Node)

plugin_get_author_homepage.argtypes = [POINTER(Plugin)]
plugin_get_author_homepage.restype  = POINTER(Node)

plugin_is_replaced.argtypes = [POINTER(Plugin)]
plugin_is_replaced.restype  = c_bool

plugin_get_related.argtypes = [POINTER(Plugin), POINTER(Node)]
plugin_get_related.restype  = POINTER(Nodes)

port_get_node.argtypes = [POINTER(Plugin), POINTER(Port)]
port_get_node.restype  = POINTER(Node)

port_get_value.argtypes = [POINTER(Plugin), POINTER(Port), POINTER(Node)]
port_get_value.restype  = POINTER(Nodes)

port_get.argtypes = [POINTER(Plugin), POINTER(Port), POINTER(Node)]
port_get.restype  = POINTER(Node)

port_get_properties.argtypes = [POINTER(Plugin), POINTER(Port)]
port_get_properties.restype  = POINTER(Nodes)

port_has_property.argtypes = [POINTER(Plugin), POINTER(Port), POINTER(Node)]
port_has_property.restype  = c_bool

port_supports_event.argtypes = [POINTER(Plugin), POINTER(Port), POINTER(Node)]
port_supports_event.restype  = c_bool

port_get_index.argtypes = [POINTER(Plugin), POINTER(Port)]
port_get_index.restype  = c_uint32

port_get_symbol.argtypes = [POINTER(Plugin), POINTER(Port)]
port_get_symbol.restype  = POINTER(Node)

port_get_name.argtypes = [POINTER(Plugin), POINTER(Port)]
port_get_name.restype  = POINTER(Node)

port_get_classes.argtypes = [POINTER(Plugin), POINTER(Port)]
port_get_classes.restype  = POINTER(Nodes)

port_is_a.argtypes = [POINTER(Plugin), POINTER(Port), POINTER(Node)]
port_is_a.restype  = c_bool

port_get_range.argtypes = [POINTER(Plugin), POINTER(Port), POINTER(POINTER(Node)), POINTER(POINTER(Node)), POINTER(POINTER(Node))]
port_get_range.restype  = None

port_get_scale_points.argtypes = [POINTER(Plugin), POINTER(Port)]
port_get_scale_points.restype  = POINTER(ScalePoints)

state_new_from_world.argtypes = [POINTER(World), POINTER(LV2_URID_Map), POINTER(Node)]
state_new_from_world.restype  = POINTER(State)

state_new_from_file.argtypes = [POINTER(World), POINTER(LV2_URID_Map), POINTER(Node), String]
state_new_from_file.restype  = POINTER(State)

state_new_from_string.argtypes = [POINTER(World), POINTER(LV2_URID_Map), String]
state_new_from_string.restype  = POINTER(State)

LilvGetPortValueFunc = CFUNCTYPE(c_void_p, c_char_p, POINTER(None), POINTER(c_uint32), POINTER(c_uint32))

state_new_from_instance.argtypes = [POINTER(Plugin), POINTER(Instance), POINTER(LV2_URID_Map), c_char_p, c_char_p, c_char_p, String, LilvGetPortValueFunc, POINTER(None), c_uint32, POINTER(POINTER(LV2_Feature))]
state_new_from_instance.restype  = POINTER(State)

state_free.argtypes = [POINTER(State)]
state_free.restype  = None

state_equals.argtypes = [POINTER(State), POINTER(State)]
state_equals.restype  = c_bool

state_get_num_properties.argtypes = [POINTER(State)]
state_get_num_properties.restype  = c_uint

state_get_plugin_uri.argtypes = [POINTER(State)]
state_get_plugin_uri.restype  = POINTER(Node)

state_get_uri.argtypes = [POINTER(State)]
state_get_uri.restype  = POINTER(Node)

state_get_label.argtypes = [POINTER(State)]
state_get_label.restype  = c_char_p

state_set_label.argtypes = [POINTER(State), String]
state_set_label.restype  = None

state_set_metadata.argtypes = [POINTER(State), c_uint32, POINTER(None), c_size_t, c_uint32, c_uint32]
state_set_metadata.restype  = c_int

LilvSetPortValueFunc            = CFUNCTYPE(None, c_char_p, POINTER(None), POINTER(None), c_uint32, c_uint32)
state_emit_port_values.argtypes = [POINTER(State), LilvSetPortValueFunc, POINTER(None)]
state_emit_port_values.restype  = None

state_restore.argtypes = [POINTER(State), POINTER(Instance), LilvSetPortValueFunc, POINTER(None), c_uint32, POINTER(POINTER(LV2_Feature))]
state_restore.restype  = None

state_save.argtypes = [POINTER(World), POINTER(LV2_URID_Map), POINTER(LV2_URID_Unmap), POINTER(State), c_char_p, c_char_p, String]
state_save.restype  = c_int

state_to_string.argtypes = [POINTER(World), POINTER(LV2_URID_Map), POINTER(LV2_URID_Unmap), POINTER(State), c_char_p, String]
state_to_string.restype  = c_char_p

state_delete.argtypes = [POINTER(World), POINTER(State)]
state_delete.restype  = c_int

scale_point_get_label.argtypes = [POINTER(ScalePoint)]
scale_point_get_label.restype  = POINTER(Node)

scale_point_get_value.argtypes = [POINTER(ScalePoint)]
scale_point_get_value.restype  = POINTER(Node)

plugin_class_get_parent_uri.argtypes = [POINTER(PluginClass)]
plugin_class_get_parent_uri.restype  = POINTER(Node)

plugin_class_get_uri.argtypes = [POINTER(PluginClass)]
plugin_class_get_uri.restype  = POINTER(Node)

plugin_class_get_label.argtypes = [POINTER(PluginClass)]
plugin_class_get_label.restype  = POINTER(Node)

plugin_class_get_children.argtypes = [POINTER(PluginClass)]
plugin_class_get_children.restype  = POINTER(PluginClasses)

plugin_instantiate.argtypes = [POINTER(Plugin), c_double, POINTER(POINTER(LV2_Feature))]
plugin_instantiate.restype = POINTER(Instance)

instance_free.argtypes = [POINTER(Instance)]
instance_free.restype = None

plugin_get_uis.argtypes = [POINTER(Plugin)]
plugin_get_uis.restype = POINTER(UIs)

ui_get_uri.argtypes = [POINTER(UI)]
ui_get_uri.restype = POINTER(Node)

ui_get_classes.argtypes = [POINTER(UI)]
ui_get_classes.restype = POINTER(Nodes)

ui_is_a.argtypes = [POINTER(UI), POINTER(Node)]
ui_is_a.restype = c_bool

LilvUISupportedFunc = CFUNCTYPE(c_uint, c_char_p, c_char_p)

ui_is_supported.argtypes = [POINTER(UI), LilvUISupportedFunc, POINTER(Node), POINTER(POINTER(Node))]
ui_is_supported.restype = c_uint

ui_get_bundle_uri.argtypes = [POINTER(UI)]
ui_get_bundle_uri.restype = POINTER(Node)

ui_get_binary_uri.argtypes = [POINTER(UI)]
ui_get_binary_uri.restype = POINTER(Node)

OPTION_FILTER_LANG  = 'http://drobilla.net/ns/lilv#filter-lang'
OPTION_DYN_MANIFEST = 'http://drobilla.net/ns/lilv#dyn-manifest'

# Define URI constants for compatibility with old Python bindings

LILV_NS_DOAP             = 'http://usefulinc.com/ns/doap#'
LILV_NS_FOAF             = 'http://xmlns.com/foaf/0.1/'
LILV_NS_LILV             = 'http://drobilla.net/ns/lilv#'
LILV_NS_LV2              = 'http://lv2plug.in/ns/lv2core#'
LILV_NS_OWL              = 'http://www.w3.org/2002/07/owl#'
LILV_NS_RDF              = 'http://www.w3.org/1999/02/22-rdf-syntax-ns#'
LILV_NS_RDFS             = 'http://www.w3.org/2000/01/rdf-schema#'
LILV_NS_XSD              = 'http://www.w3.org/2001/XMLSchema#'
LILV_URI_ATOM_PORT       = 'http://lv2plug.in/ns/ext/atom#AtomPort'
LILV_URI_AUDIO_PORT      = 'http://lv2plug.in/ns/lv2core#AudioPort'
LILV_URI_CONTROL_PORT    = 'http://lv2plug.in/ns/lv2core#ControlPort'
LILV_URI_CV_PORT         = 'http://lv2plug.in/ns/lv2core#CVPort'
LILV_URI_EVENT_PORT      = 'http://lv2plug.in/ns/ext/event#EventPort'
LILV_URI_INPUT_PORT      = 'http://lv2plug.in/ns/lv2core#InputPort'
LILV_URI_MIDI_EVENT      = 'http://lv2plug.in/ns/ext/midi#MidiEvent'
LILV_URI_OUTPUT_PORT     = 'http://lv2plug.in/ns/lv2core#OutputPort'
LILV_URI_PORT            = 'http://lv2plug.in/ns/lv2core#Port'
LILV_OPTION_FILTER_LANG  = 'http://drobilla.net/ns/lilv#filter-lang'
LILV_OPTION_DYN_MANIFEST = 'http://drobilla.net/ns/lilv#dyn-manifest'
