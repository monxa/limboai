/**
 * tree_search.cpp
 * =============================================================================
 * Copyright 2021-2024 Serhii Snitsaruk
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#include "tree_search.h"

#include "../util/limbo_compat.h" // for edge scale
#include "../util/limbo_string_names.h"
#include "../util/limbo_utility.h"
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/editor_interface.hpp> // for edge scale
#include <godot_cpp/templates/vector.hpp>

#ifdef TOOLS_ENABLED

/* ------- TreeSearchPanel ------- */

void TreeSearchPanel::_initialize_controls() {
	line_edit_search = memnew(LineEdit);
	check_button_filter_highlight = memnew(CheckButton);
	close_button = memnew(Button);
	label_highlight = memnew(Label);
	label_filter = memnew(Label);

	line_edit_search->set_placeholder(TTR("Search tree"));

	label_highlight->set_text(TTR("Highlight"));
	label_filter->set_text(TTR("Filter"));

	close_button->set_button_icon(get_theme_icon(LW_NAME(Close), LW_NAME(EditorIcons)));
	// positioning and sizing
	this->set_anchors_and_offsets_preset(LayoutPreset::PRESET_BOTTOM_WIDE);
	this->set_v_size_flags(SIZE_SHRINK_CENTER); // Do not expand vertically

	// hack add separator to the left so line edit doesn't clip.
	line_edit_search->set_h_size_flags(SIZE_EXPAND_FILL);

	add_spacer(0.25); // otherwise the lineedits expand margin touches the left border.
	add_child(line_edit_search);
	add_spacer();
	add_child(label_highlight);
	add_child(check_button_filter_highlight);
	add_child(label_filter);
	add_child(close_button);
	add_spacer(0.25);
}

void TreeSearchPanel::add_spacer(float width_multiplier) {
	Control *spacer = memnew(Control);
	spacer->set_custom_minimum_size(Vector2(8.0 * EDSCALE * width_multiplier, 0.0));
	add_child(spacer);
}

/* !TreeSearchPanel */

TreeSearchPanel::TreeSearchPanel() {
	_initialize_controls();
}

TreeSearchMode TreeSearchPanel::get_search_mode() {
	if (!check_button_filter_highlight || !check_button_filter_highlight->is_pressed()) {
		return TreeSearchMode::HIGHLIGHT;
	}
	return TreeSearchMode::FILTER;
}

String TreeSearchPanel::get_text() {
	if (!line_edit_search)
		return String();
	return line_edit_search->get_text();
}

/* ------- TreeSearch ------- */

void TreeSearch::filter_tree(TreeItem *tree_item, String search_mask) {
	PRINT_LINE("filter tree not yet implemented!", search_mask);
}

void TreeSearch::highlight_tree(TreeItem *tree_item, String search_mask) {
	PRINT_LINE("highlight tree not yet implemented! ", search_mask);
	// queue/iterative instead of recursive approach. dsf
	Vector<TreeItem> queue;
	Vector<TreeItem> hits;
}

// Call this as a post-processing step for the already constructed tree.
void TreeSearch::apply_search(Tree *tree) {
	if (!search_panel || !search_panel->is_visible()){
		return;
	}

	TreeItem * tree_root = tree->get_root();
	String search_mask = search_panel->get_text();
	TreeSearchMode search_mode = search_panel->get_search_mode();
	if (search_mode == TreeSearchMode::HIGHLIGHT){
		highlight_tree(tree_root, search_mask);
	}
	if (search_mode == TreeSearchMode::FILTER){
		filter_tree(tree_root, search_mask);
	}
}

TreeSearch::TreeSearch() {
	search_panel = memnew(TreeSearchPanel);
}

TreeSearch::~TreeSearch() {
}

/* !TreeSearch */

#endif // ! TOOLS_ENABLED