/*
  Copyright 2007-2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lilv_internal.h"

LilvUI*
lilv_ui_new(LilvWorld* world,
            LilvNode* uri,
            LilvNode* type_uri,
            LilvNode* binary_uri)
{
	assert(uri);
	assert(type_uri);
	assert(binary_uri);

	LilvUI* ui = (LilvUI*)malloc(sizeof(LilvUI));
	ui->world      = world;
	ui->uri        = uri;
	ui->binary_uri = binary_uri;

	// FIXME: kludge
	char* bundle     = lilv_strdup(lilv_node_as_string(ui->binary_uri));
	char* last_slash = strrchr(bundle, '/') + 1;
	*last_slash = '\0';
	ui->bundle_uri = lilv_new_uri(world, bundle);
	free(bundle);

	ui->classes = lilv_nodes_new();
	zix_tree_insert((ZixTree*)ui->classes, type_uri, NULL);

	return ui;
}

void
lilv_ui_free(LilvUI* ui)
{
	lilv_node_free(ui->uri);
	ui->uri = NULL;

	lilv_node_free(ui->bundle_uri);
	ui->bundle_uri = NULL;

	lilv_node_free(ui->binary_uri);
	ui->binary_uri = NULL;

	lilv_nodes_free(ui->classes);

	free(ui);
}

LILV_API
const LilvNode*
lilv_ui_get_uri(const LilvUI* ui)
{
	assert(ui);
	assert(ui->uri);
	return ui->uri;
}

LILV_API
unsigned
lilv_ui_is_supported(const LilvUI*       ui,
                     LilvUISupportedFunc supported_func,
                     const LilvNode*     container_type,
                     const LilvNode**    ui_type)
{
	const LilvNodes* classes = lilv_ui_get_classes(ui);
	LILV_FOREACH(nodes, c, classes) {
		const LilvNode* type = lilv_nodes_get(classes, c);
		const unsigned  q    = supported_func(lilv_node_as_uri(container_type),
		                                       lilv_node_as_uri(type));
		if (q) {
			if (ui_type) {
				*ui_type = type;
			}
			return q;
		}
	}

	return 0;
}

LILV_API
const LilvNodes*
lilv_ui_get_classes(const LilvUI* ui)
{
	return ui->classes;
}

LILV_API
bool
lilv_ui_is_a(const LilvUI* ui, const LilvNode* ui_class_uri)
{
	return lilv_nodes_contains(ui->classes, ui_class_uri);
}

LILV_API
const LilvNode*
lilv_ui_get_bundle_uri(const LilvUI* ui)
{
	assert(ui);
	assert(ui->bundle_uri);
	return ui->bundle_uri;
}

LILV_API
const LilvNode*
lilv_ui_get_binary_uri(const LilvUI* ui)
{
	assert(ui);
	assert(ui->binary_uri);
	return ui->binary_uri;
}

static LilvNodes*
lilv_ui_get_value_internal(const LilvUI* ui,
                           const SordNode*   predicate)
{
    return lilv_world_query_values_internal(
        ui->world, ui->uri->node, predicate, NULL);
}

LILV_API
LilvNodes*
lilv_ui_get_supported_features(const LilvUI* ui)
{
    LilvNodes* optional = lilv_ui_get_optional_features(ui);
    LilvNodes* required = lilv_ui_get_required_features(ui);
    LilvNodes* result   = lilv_nodes_new();

    LILV_FOREACH(nodes, i, optional)
        zix_tree_insert((ZixTree*)result,
                        lilv_node_duplicate(lilv_nodes_get(optional, i)),
                        NULL);
    LILV_FOREACH(nodes, i, required)
        zix_tree_insert((ZixTree*)result,
                        lilv_node_duplicate(lilv_nodes_get(required, i)),
                        NULL);

    lilv_nodes_free(optional);
    lilv_nodes_free(required);

    return result;
}

LILV_API
LilvNodes*
lilv_ui_get_required_features(const LilvUI* ui)
{
    return lilv_ui_get_value_internal(
        ui, ui->world->uris.lv2_requiredFeature);
}

LILV_API
LilvNodes*
lilv_ui_get_optional_features(const LilvUI* ui)
{
    return lilv_ui_get_value_internal(
        ui, ui->world->uris.lv2_optionalFeature);
}

LILV_API
LilvNodes*
lilv_ui_get_extension_data(const LilvUI* ui)
{
    return lilv_ui_get_value_internal(ui, ui->world->uris.lv2_extensionData);
}
