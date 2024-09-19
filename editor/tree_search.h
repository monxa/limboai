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
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/variant/signal.hpp>
#include <godot_cpp/classes/label.hpp>
#endif // LIMBOAI_GDEXTENSION

#ifdef LIMBOAI_MODULE
// TODO: Add includes for godot module variant.
#endif // LIMBOAI_MODULE

using namespace godot;

enum TreeSearchMode {
		HIGHLIGHT = 0,
		FILTER = 1
};

class TreeSearchPanel : public HBoxContainer {
GDCLASS(TreeSearchPanel, HBoxContainer)
private:
	Button *toggle_button_filter_highlight;
	Button *close_button;
	Label * label_highlight;
	Label * label_filter;
	LineEdit *line_edit_search;
	CheckButton *check_button_filter_highlight;
	
	void _initialize_controls();
	void add_spacer(float width_multiplier = 1.f);
	


protected:
	static void _bind_methods(){}; // we don't need anything exposed.

public:
	TreeSearchMode get_search_mode();
	String get_text();
	
	TreeSearchPanel();
};

class TreeSearch {
private:
	void filter_tree(TreeItem *tree_item, String search_mask);
	void highlight_tree(TreeItem *tree_item, String search_mask);

	void highlight_item(TreeItem * tree_item, String search_mask);
	Vector<TreeItem *> find_matching_entries(TreeItem * tree_item, Vector<TreeItem * > buffer = Vector<TreeItem *>());

public:
	// we will add everything from TaskTree.h
	void apply_search(Tree *tree);
	void set_search_evaluation_method(Callable method);
	TreeSearchPanel *search_panel;

	TreeSearch();
	~TreeSearch();
};

#endif // TREE_SEARCH_H
#endif // ! TOOLS_ENABLED
