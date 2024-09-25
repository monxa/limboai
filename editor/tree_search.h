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

#include "../bt/tasks/bt_task.h" // for tree item parsing

#ifdef LIMBOAI_GDEXTENSION
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/h_flow_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>

#endif // LIMBOAI_GDEXTENSION

#ifdef LIMBOAI_MODULE
#include "scene/gui/check_button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/tree.h"
#endif // LIMBOAI_MODULE

using namespace godot;

enum TreeSearchMode {
	HIGHLIGHT = 0,
	FILTER = 1
};

class TreeSearchPanel : public HFlowContainer {
	GDCLASS(TreeSearchPanel, HFlowContainer)
private:
	Button *toggle_button_filter_highlight;
	Button *close_button;
	Label *label_highlight;
	Label *label_filter;
	LineEdit *line_edit_search;
	CheckButton *check_button_filter_highlight;
	void _initialize_controls();
	void _initialize_close_callbacks();
	void _add_spacer(float width_multiplier = 1.f);

	void _on_draw_highlight(TreeItem *item, Rect2 rect);
	void _emit_text_changed(const String &text);
	void _emit_text_submitted(const String &p_text);
	void _notification(int p_what);

protected:
	static void _bind_methods();
	void _process_shortcut_input(const Ref<InputEvent> &p_event);

public:
	TreeSearchMode get_search_mode();
	String get_text();
	void show_and_focus();
	TreeSearchPanel();
	bool has_focus();
};

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

	Tree * tree_reference;

	Vector<TreeItem *> ordered_tree_items;
	Vector<TreeItem *> matching_entries;
	HashMap<TreeItem *, int> number_matches;
	HashMap<TreeItem *, Callable> callable_cache;
	
	void _filter_tree(TreeItem *tree_item, const String &search_mask);
	void _highlight_tree(const String &p_search_mask);
	void _draw_highlight_item(TreeItem *tree_item, Rect2 rect, String search_mask, Callable parent_draw_method);
	void _update_matching_entries(const String &search_mask);
	void _update_ordered_tree_items(TreeItem *tree_item);
	void _update_number_matches();
	Vector<TreeItem *> _find_matching_entries(TreeItem *tree_item, const String &search_mask, Vector<TreeItem *> &buffer);
	StringSearchIndices _substring_bounds(const String &searchable, const String &search_mask) const;

	void _select_item(TreeItem * item);
	void _select_first_match();
	void _select_next_match();


protected:
	static void _bind_methods(){}

public:
	// we will add everything from TaskTree.h
	void update_search(Tree *tree);
	void on_item_edited(TreeItem *item);
	TreeSearchPanel *search_panel;

	TreeSearch();
	~TreeSearch();
};

#endif // TREE_SEARCH_H
#endif // ! TOOLS_ENABLED