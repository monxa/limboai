/**
 * tree_search.h
 * =============================================================================
 * Copyright 2021-2024 Serhii Snitsaruk
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#ifdef TOOLS_ENABLED

#ifndef TREE_SEARCH_H
#define TREE_SEARCH_H

#ifdef LIMBOAI_MODULE
#include "core/templates/hash_map.h"
#include "scene/gui/check_box.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/tree.h"
#endif // LIMBOAI_MODULE

#ifdef LIMBOAI_GDEXTENSION
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#endif // LIMBOAI_GDEXTENSION

using namespace godot;

class TreeSearchPanel;

class TreeSearch : public RefCounted {
	GDCLASS(TreeSearch, RefCounted)
private:
	struct StringSearchIndices {
		// initialize to opposite bounds.
		int lower = -1;
		int upper = -1;

		bool hit() {
			return 0 <= lower && lower < upper;
		}
	};

	TreeSearchPanel *search_panel;

	// For TaskTree: These are updated when the tree is updated through TaskTree::_create_tree.
	Tree *tree_reference;

	Vector<TreeItem *> ordered_tree_items;
	Vector<TreeItem *> matching_entries;
	HashMap<TreeItem *, int> number_matches;
	HashMap<TreeItem *, Callable> callable_cache;

	bool was_searched_recently = false; // Performance
	bool was_filtered_recently = false; // Performance

	void _clean_callable_cache();

	// Update_search() calls these
	void _filter_tree(const String &p_search_mask);
	void _clear_filter();

	void _highlight_tree();
	void _highlight_tree_item(TreeItem *p_tree_item);

	// Custom draw-Callback (bind inherited Callable).
	void _draw_highlight_item(TreeItem *p_tree_item, Rect2 p_rect, Callable p_parent_draw_method);

	void _update_matching_entries(const String &p_search_mask);
	void _update_ordered_tree_items(TreeItem *p_tree_item);
	void _update_number_matches();

	void _find_matching_entries(TreeItem *p_tree_item, const String &p_search_mask, Vector<TreeItem *> &p_accum);
	String _get_search_mask();
	StringSearchIndices _substring_bounds(const String &p_searchable, const String &p_search_mask) const;

	void _select_item(TreeItem *p_item);
	void _select_first_match();
	void _select_next_match();

	template <typename T>
	bool _vector_has_bsearch(Vector<T *> p_vec, T *element);

protected:
	static void _bind_methods() {}

public:
	enum TreeSearchMode {
		HIGHLIGHT = 0,
		FILTER = 1
	};

	void update_search(Tree *p_tree);
	void notify_item_edited(TreeItem *p_item);

	TreeSearch() { ERR_FAIL_MSG("TreeSearch needs a TreeSearchPanel to work properly."); }
	TreeSearch(TreeSearchPanel *p_search_panel);
};

// --------------------------------------------

class TreeSearchPanel : public HFlowContainer {
	GDCLASS(TreeSearchPanel, HFlowContainer)

private:
	Button *toggle_button_filter_highlight;
	Button *close_button;
	Label *label_filter;
	LineEdit *line_edit_search;
	CheckBox *check_button_filter_highlight;
	void _initialize_controls();
	void _add_spacer(float width_multiplier = 1.f);

	void _on_draw_highlight(TreeItem *p_item, Rect2 p_rect);
	void _notification(int p_what);

protected:
	static void _bind_methods();

public:
	TreeSearch::TreeSearchMode get_search_mode();
	String get_text();
	void show_and_focus();
	TreeSearchPanel();
};

#endif // TREE_SEARCH_H
#endif // ! TOOLS_ENABLED
