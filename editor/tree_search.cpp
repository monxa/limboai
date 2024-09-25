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

#ifdef TOOLS_ENABLED

#include "tree_search.h"

#include "../bt/behavior_tree.h"
#include "../util/limbo_compat.h" // for edscale
#include "../util/limbo_string_names.h"
#include "../util/limbo_utility.h"

#ifdef LIMBOAI_MODULE
#include "core/math/math_funcs.h"
#include "editor/editor_interface.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/separator.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"
#include "scene/resources/style_box_flat.h"
#endif // LIMBOAI_MODULE

#ifdef LIMBOAI_GDEXTENSION
#include <godot_cpp/classes/editor_interface.hpp> // for edge scale
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/math.hpp>
#endif // LIMBOAI_GDEXTENSION

#define UPPER_BOUND (1 << 15) // for substring search.

/* ------- TreeSearchPanel ------- */

void TreeSearchPanel::_initialize_controls() {
	line_edit_search = memnew(LineEdit);
	check_button_filter_highlight = memnew(CheckBox);
	close_button = memnew(Button);
	label_filter = memnew(Label);

	line_edit_search->set_placeholder(TTR("Search tree"));

	label_filter->set_text(TTR("Filter"));

	BUTTON_SET_ICON(close_button, get_theme_icon(LW_NAME(Close), LW_NAME(EditorIcons)));
	close_button->set_theme_type_variation("FlatButton");

	// positioning and sizing
	this->set_anchors_and_offsets_preset(LayoutPreset::PRESET_BOTTOM_WIDE);
	this->set_v_size_flags(SIZE_SHRINK_CENTER); // Do not expand vertically

	// hack add separator to the left so line edit doesn't clip.
	line_edit_search->set_h_size_flags(SIZE_EXPAND_FILL);

	_add_spacer(0.25); // otherwise the lineedits expand margin touches the left border.
	add_child(line_edit_search);
	add_child(check_button_filter_highlight);
	add_child(label_filter);
	_add_spacer();
	add_child(close_button);
	_add_spacer(0.25);
}

void TreeSearchPanel::_initialize_close_callbacks() {
	Callable calleable_set_invisible = Callable(this, "set_visible").bind(false); // don't need a custom bind.
	close_button->connect("pressed", calleable_set_invisible);
	close_button->set_shortcut(LW_GET_SHORTCUT("limbo_ai/hide_tree_search"));
}

void TreeSearchPanel::_add_spacer(float p_width_multiplier) {
	Control *spacer = memnew(Control);
	spacer->set_custom_minimum_size(Vector2(8.0 * EDSCALE * p_width_multiplier, 0.0));
	add_child(spacer);
}

void TreeSearchPanel::_emit_text_changed(const String &p_text) {
	this->emit_signal("text_changed", p_text);
}

void TreeSearchPanel::_emit_text_submitted(const String &p_text) {
	this->emit_signal("text_submitted");
}

void TreeSearchPanel::_emit_filter_toggled() {
	this->emit_signal("filter_toggled");
}

void TreeSearchPanel::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_initialize_controls();
			line_edit_search->connect("text_changed", callable_mp(this, &TreeSearchPanel::_emit_text_changed));
			_initialize_close_callbacks();
			line_edit_search->connect("text_submitted", callable_mp(this, &TreeSearchPanel::_emit_text_submitted));
			check_button_filter_highlight->connect("pressed", callable_mp(this, &TreeSearchPanel::_emit_filter_toggled));
			break;
		}
	}
}

void TreeSearchPanel::_bind_methods() {
	ADD_SIGNAL(MethodInfo("text_changed"));
	ADD_SIGNAL(MethodInfo("text_submitted"));
	ADD_SIGNAL(MethodInfo("filter_toggled"));
}

TreeSearchPanel::TreeSearchPanel() {
	this->set_visible(false);
}

bool TreeSearchPanel::has_focus() {
	return false;
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

void TreeSearchPanel::show_and_focus() {
	this->set_visible(true);
	line_edit_search->grab_focus();
}

/* !TreeSearchPanel */

/* ------- TreeSearch ------- */

void TreeSearch::_filter_tree(const String &p_search_mask) {
	if (matching_entries.size() == 0) {
		return;
	}

	for (int i = 0; i < ordered_tree_items.size(); i++) {
		TreeItem *cur_item = ordered_tree_items[i];

		if (number_matches.has(cur_item)) {
			continue;
		}

		TreeItem *first_counting_ancestor = cur_item;
		while (first_counting_ancestor && !number_matches.has(first_counting_ancestor)) {
			first_counting_ancestor = first_counting_ancestor->get_parent();
		}
		if (!first_counting_ancestor || first_counting_ancestor == tree_reference->get_root() || !_vector_has_bsearch(matching_entries, first_counting_ancestor)) {
			cur_item->set_visible(false);
		}
	}
}

void TreeSearch::_highlight_tree(const String &p_search_mask) {
	callable_cache.clear();
	for (int i = 0; i < ordered_tree_items.size(); i++) {
		TreeItem *entry = ordered_tree_items[i];

		int num_m = number_matches.has(entry) ? number_matches.get(entry) : 0;
		if (num_m == 0) {
			continue;
			;
		}

		// make sure to also call any draw method already defined.
		Callable parent_draw_method;
		if (entry->get_cell_mode(0) == TreeItem::CELL_MODE_CUSTOM) {
			parent_draw_method = entry->get_custom_draw_callback(0);
		}

		Callable draw_callback = callable_mp(this, &TreeSearch::_draw_highlight_item).bind(p_search_mask, parent_draw_method);

		// -- this is necessary because of the modularity of this implementation
		// cache render properties of entry
		String cached_text = entry->get_text(0);
		Ref<Texture2D> cached_icon = entry->get_icon(0);
		int cached_max_width = entry->get_icon_max_width(0);
		callable_cache[entry] = draw_callback;

		// this removes render properties in entry
		entry->set_custom_draw_callback(0, draw_callback);
		entry->set_cell_mode(0, TreeItem::CELL_MODE_CUSTOM);

		// restore render properties
		entry->set_text(0, cached_text);
		entry->set_icon(0, cached_icon);
		entry->set_icon_max_width(0, cached_max_width);
	}
}

// custom draw callback for highlighting (bind 2)
void TreeSearch::_draw_highlight_item(TreeItem *p_tree_item, Rect2 p_rect, String p_search_mask, Callable p_parent_draw_method) {
	if (!p_tree_item)
		return;
	// call any parent draw methods such as for probability FIRST.
	p_parent_draw_method.call(p_tree_item, p_rect);

	// first part: outline
	if (matching_entries.has(p_tree_item)) {
		// font info
		Ref<Font> font = p_tree_item->get_custom_font(0);
		if (font.is_null()) {
			font = p_tree_item->get_tree()->get_theme_font(LW_NAME(font));
		}
		ERR_FAIL_NULL(font);
		double font_size = p_tree_item->get_custom_font_size(0);
		if (font_size == -1) {
			font_size = p_tree_item->get_tree()->get_theme_font_size(LW_NAME(font));
		}

		// substring size
		String string_full = p_tree_item->get_text(0);
		StringSearchIndices substring_idx = _substring_bounds(string_full, p_search_mask);

		String substring_match = string_full.substr(substring_idx.lower, substring_idx.upper - substring_idx.lower);
		Vector2 substring_match_size = font->get_string_size(substring_match, HORIZONTAL_ALIGNMENT_LEFT, -1.f, font_size);

		String substring_before = string_full.substr(0, substring_idx.lower);
		Vector2 substring_before_size = font->get_string_size(substring_before, HORIZONTAL_ALIGNMENT_LEFT, -1.f, font_size);

		// stylebox
		Ref<StyleBox> stylebox = p_tree_item->get_tree()->get_theme_stylebox("Focus");
		ERR_FAIL_NULL(stylebox);

		// extract separation
		float h_sep = p_tree_item->get_tree()->get_theme_constant("h_separation");

		// compose draw rect
		const Vector2 PADDING = Vector2(4., 2.);
		Rect2 draw_rect = p_rect;

		Vector2 rect_offset = Vector2(substring_before_size.x, 0);
		rect_offset.x += p_tree_item->get_icon_max_width(0) * EDSCALE;
		rect_offset.x += (h_sep + 4.) * EDSCALE; // TODO: Find better way to determine texts x-offset
		rect_offset.y = (p_rect.size.y - substring_match_size.y) / 2; // center box vertically

		draw_rect.position += rect_offset - PADDING / 2;
		draw_rect.size = substring_match_size + PADDING;

		// draw
		stylebox->draw(p_tree_item->get_tree()->get_canvas_item(), draw_rect);
	}

	// second part: draw number (TODO: maybe use columns)
	int num_mat = number_matches.has(p_tree_item) ? number_matches.get(p_tree_item) : 0;
	if (num_mat > 0) {
		float h_sep = p_tree_item->get_tree()->get_theme_constant("h_separation");
		Ref<Font> font = tree_reference->get_theme_font("font");
		float font_size = tree_reference->get_theme_font_size("font") * 0.75;

		String num_string = String::num_int64(num_mat);
		Vector2 string_size = font->get_string_size(num_string, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
		Vector2 text_pos = p_rect.position;

		text_pos.x += p_rect.size.x - string_size.x - h_sep;
		text_pos.y += font->get_descent(font_size) + p_rect.size.y / 2.; // center vertically

		font->draw_string(tree_reference->get_canvas_item(), text_pos, num_string, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size);
	}
}

void TreeSearch::_update_matching_entries(const String &search_mask) {
	Vector<TreeItem *> accum;
	matching_entries = _find_matching_entries(tree_reference->get_root(), search_mask, accum);
}

/* this linearizes the tree into [ordered_tree_items] like so:
 - i1
	- i2
	- i3
 - i4 ---> [i1,i2,i3,i4]
*/
void TreeSearch::_update_ordered_tree_items(TreeItem *p_tree_item) {
	if (!p_tree_item)
		return;
	if (p_tree_item == p_tree_item->get_tree()->get_root()) {
		ordered_tree_items.clear();
	}
	// Add the current item to the list
	ordered_tree_items.push_back(p_tree_item);

	// Recursively collect items from the first child
	TreeItem *child = p_tree_item->get_first_child();
	while (child) {
		_update_ordered_tree_items(child);
		child = child->get_next();
	}
}

void TreeSearch::_update_number_matches() {
	number_matches.clear();
	for (int i = 0; i < matching_entries.size(); i++) {
		TreeItem *item = matching_entries[i];
		while (item) {
			int old_num_value = number_matches.has(item) ? number_matches.get(item) : 0;
			number_matches[item] = old_num_value + 1;
			item = item->get_parent();
		}
	}
}

Vector<TreeItem *> TreeSearch::_find_matching_entries(TreeItem *p_tree_item, const String &p_search_mask, Vector<TreeItem *> &p_accum) {
	if (!p_tree_item)
		return p_accum;
	StringSearchIndices item_search_indices = _substring_bounds(p_tree_item->get_text(0), p_search_mask);
	if (item_search_indices.hit())
		p_accum.insert(p_accum.bsearch(p_tree_item, true), p_tree_item);

	for (int i = 0; i < p_tree_item->get_child_count(); i++) {
		TreeItem *child = p_tree_item->get_child(i);
		_find_matching_entries(child, p_search_mask, p_accum);
	}
	return p_accum;
}

// Returns the lower and upper bounds of a substring. Does fuzzy search: Simply looks if words exist in right ordering.
// Also ignores case if p_search_mask is lowercase. Example:
// p_searcheable = "TimeLimit 2 sec", p_search_mask = limit 2 sec -> [4,14]. With p_search_mask = "LimiT 2 SEC" or "Limit sec 2" -> [-1,-1]
TreeSearch::StringSearchIndices TreeSearch::_substring_bounds(const String &p_searchable, const String &p_search_mask) const {
	StringSearchIndices result;
	result.lower = UPPER_BOUND;
	result.upper = 0;

	if (p_search_mask.is_empty()) {
		return result; // Early return if search_mask is empty.
	}

	// Determine if the search should be case-insensitive.
	bool is_case_insensitive = (p_search_mask == p_search_mask.to_lower());
	String searchable_processed = is_case_insensitive ? p_searchable.to_lower() : p_searchable;
	PackedStringArray words = p_search_mask.split(" ");
	int word_position = 0;
	for (const String &word : words) {
		if (word.is_empty()) {
			continue; // Skip empty words.
		}

		String word_processed = is_case_insensitive ? word.to_lower() : word;

		// Find the position of the next word in the searchable string.
		word_position = searchable_processed.find(word_processed, word_position);

		if (word_position < 0) {
			// If any word is not found, return an empty StringSearchIndices.
			return StringSearchIndices();
		}

		// Update lower and upper bounds.
		result.lower = MIN(result.lower, word_position);
		result.upper = MAX(result.upper, static_cast<int>(word_position + word.length()));
	}

	return result;
}

void TreeSearch::_select_item(TreeItem *item) {
	if (!item)
		return;
	tree_reference->set_selected(item, 0);
	tree_reference->scroll_to_item(item);
	item = item->get_parent();
	while (item) {
		item->set_collapsed(false);
		item = item->get_parent();
	}
}

void TreeSearch::_select_first_match() {
	if (matching_entries.size() == 0) {
		return;
	}
	for (int i = 0; i < ordered_tree_items.size(); i++) {
		TreeItem *item = ordered_tree_items[i];
		if (!_vector_has_bsearch(matching_entries, item)) {
			continue;
		}
		String debug_string = "[";
		_select_item(item);
		return;
	}
}

void TreeSearch::_select_next_match() {
	if (matching_entries.size() == 0) {
		return;
	}
	TreeItem *selected = tree_reference->get_selected(); // we care about a single item here.
	if (!selected) {
		_select_first_match();
		return;
	}

	int selected_idx = -1;
	for (int i = 0; i < ordered_tree_items.size(); i++) {
		if (ordered_tree_items[i] == selected) {
			selected_idx = i;
			break;
		}
	}

	// find the best fitting entry.
	for (int i = 0; i < ordered_tree_items.size(); i++) {
		TreeItem *item = ordered_tree_items[i];
		if (!_vector_has_bsearch(matching_entries, item)) {
			continue;
		}

		_select_item(item);
		return;
	}
	_select_first_match(); // wrap around.
}

template <typename T>
inline bool TreeSearch::_vector_has_bsearch(Vector<T *> p_vec, T *element) {
	int idx = p_vec.bsearch(element, true);
	bool in_array = idx >= 0 && idx < p_vec.size();

	return in_array && p_vec[idx] == element;
}

void TreeSearch::on_item_edited(TreeItem *item) {
	if (item->get_cell_mode(0) != TreeItem::CELL_MODE_CUSTOM) {
		return;
	}

	if (!callable_cache.has(item) || item->get_custom_draw_callback(0) == callable_cache.get(item)) {
		return;
	}

	item->set_custom_draw_callback(0, callable_cache.get(item));
}

// Call this as a post-processing step for the already constructed tree.
void TreeSearch::update_search(Tree *p_tree) {
	ERR_FAIL_COND(!search_panel || !p_tree);

	// ignore if panel not visible or no search string is given.
	if (!search_panel->is_visible() || search_panel->get_text().length() == 0) {
		return;
	}

	tree_reference = p_tree;

	String search_mask = search_panel->get_text();
	TreeItem *tree_root = p_tree->get_root();
	TreeSearchMode search_mode = search_panel->get_search_mode();

	_update_ordered_tree_items(p_tree->get_root());
	_update_matching_entries(search_mask);
	_update_number_matches();

	_highlight_tree(search_mask);
	if (!search_panel->is_connected("text_submitted", callable_mp(this, &TreeSearch::_select_next_match))) {
		search_panel->connect("text_submitted", callable_mp(this, &TreeSearch::_select_next_match));
	}

	if (search_mode == TreeSearchMode::FILTER) {
		_filter_tree(search_mask);
	}
}

TreeSearch::TreeSearch() {
	search_panel = memnew(TreeSearchPanel);
}

TreeSearch::~TreeSearch() {
}

/* !TreeSearch */

#endif // TOOLS_ENABLED