/**
 * limbo_utility.cpp
 * =============================================================================
 * Copyright 2021-2023 Serhii Snitsaruk
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#include "limbo_utility.h"

#include "modules/limboai/bt/tasks/bt_task.h"

#include "core/error/error_macros.h"
#include "core/object/script_language.h"
#include "core/variant/variant.h"
#include "scene/resources/texture.h"

#ifdef TOOLS_ENABLED
#include "editor/editor_node.h"
#endif // TOOLS_ENABLED

LimboUtility *LimboUtility::singleton = nullptr;

LimboUtility *LimboUtility::get_singleton() {
	return singleton;
}

String LimboUtility::decorate_var(String p_variable) const {
	String var = p_variable.trim_prefix("$").trim_prefix("\"").trim_suffix("\"");
	if (var.find(" ") == -1 && !var.is_empty()) {
		return vformat("$%s", var);
	} else {
		return vformat("$\"%s\"", var);
	}
}

String LimboUtility::get_status_name(int p_status) const {
	switch (p_status) {
		case BTTask::FRESH:
			return "FRESH";
		case BTTask::RUNNING:
			return "RUNNING";
		case BTTask::FAILURE:
			return "FAILURE";
		case BTTask::SUCCESS:
			return "SUCCESS";
		default:
			return "";
	}
}

Ref<Texture2D> LimboUtility::get_task_icon(String p_class_or_script_path) const {
#ifdef TOOLS_ENABLED
	ERR_FAIL_COND_V_MSG(p_class_or_script_path.is_empty(), Variant(), "BTTask: script path or class cannot be empty.");

	Ref<Theme> theme = EditorNode::get_singleton()->get_editor_theme();
	ERR_FAIL_COND_V(theme.is_null(), nullptr);

	if (p_class_or_script_path.begins_with("res:")) {
		Ref<Script> s = ResourceLoader::load(p_class_or_script_path, "Script");
		if (s.is_null()) {
			return theme->get_icon(SNAME("FileBroken"), SNAME("EditorIcons"));
		}

		EditorData &ed = EditorNode::get_editor_data();
		Ref<Texture2D> script_icon = ed.get_script_icon(s);
		if (script_icon.is_valid()) {
			return script_icon;
		}

		StringName base_type = s->get_instance_base_type();
		if (theme->has_icon(base_type, SNAME("EditorIcons"))) {
			return theme->get_icon(base_type, SNAME("EditorIcons"));
		}
	}

	if (theme->has_icon(p_class_or_script_path, SNAME("EditorIcons"))) {
		return theme->get_icon(p_class_or_script_path, SNAME("EditorIcons"));
	}

	// Use an icon of one of the base classes: look up max 3 parents.
	StringName class_name = p_class_or_script_path;
	for (int i = 0; i < 3; i++) {
		class_name = ClassDB::get_parent_class(class_name);
		if (theme->has_icon(class_name, SNAME("EditorIcons"))) {
			return theme->get_icon(class_name, SNAME("EditorIcons"));
		}
	}
	// Return generic resource icon as a fallback.
	return theme->get_icon(SNAME("Resource"), SNAME("EditorIcons"));
#endif // TOOLS_ENABLED

	// * Class icons are not available at runtime as they are part of the editor theme.
	return nullptr;
}

String LimboUtility::get_check_operator_string(CheckType p_check_type) {
	switch (p_check_type) {
		case LimboUtility::CheckType::CHECK_EQUAL: {
			return "==";
		} break;
		case LimboUtility::CheckType::CHECK_LESS_THAN: {
			return "<";
		} break;
		case LimboUtility::CheckType::CHECK_LESS_THAN_OR_EQUAL: {
			return "<=";
		} break;
		case LimboUtility::CheckType::CHECK_GREATER_THAN: {
			return ">";
		} break;
		case LimboUtility::CheckType::CHECK_GREATER_THAN_OR_EQUAL: {
			return ">=";
		} break;
		case LimboUtility::CheckType::CHECK_NOT_EQUAL: {
			return "!=";
		} break;
		default: {
			return "?";
		} break;
	}
}

bool LimboUtility::perform_check(CheckType p_check_type, const Variant &left_value, const Variant &right_value) {
	switch (p_check_type) {
		case LimboUtility::CheckType::CHECK_EQUAL: {
			return Variant::evaluate(Variant::OP_EQUAL, left_value, right_value);
		} break;
		case LimboUtility::CheckType::CHECK_LESS_THAN: {
			return Variant::evaluate(Variant::OP_LESS, left_value, right_value);
		} break;
		case LimboUtility::CheckType::CHECK_LESS_THAN_OR_EQUAL: {
			return Variant::evaluate(Variant::OP_LESS_EQUAL, left_value, right_value);
		} break;
		case LimboUtility::CheckType::CHECK_GREATER_THAN: {
			return Variant::evaluate(Variant::OP_GREATER, left_value, right_value);
		} break;
		case LimboUtility::CheckType::CHECK_GREATER_THAN_OR_EQUAL: {
			return Variant::evaluate(Variant::OP_GREATER_EQUAL, left_value, right_value);
		} break;
		case LimboUtility::CheckType::CHECK_NOT_EQUAL: {
			return Variant::evaluate(Variant::OP_NOT_EQUAL, left_value, right_value);
		} break;
		default: {
			return false;
		} break;
	}
}

void LimboUtility::_bind_methods() {
	ClassDB::bind_method(D_METHOD("decorate_var", "p_variable"), &LimboUtility::decorate_var);
	ClassDB::bind_method(D_METHOD("get_status_name", "p_status"), &LimboUtility::get_status_name);
	ClassDB::bind_method(D_METHOD("get_task_icon", "p_class_or_script_path"), &LimboUtility::get_task_icon);

	BIND_ENUM_CONSTANT(CHECK_EQUAL);
	BIND_ENUM_CONSTANT(CHECK_LESS_THAN);
	BIND_ENUM_CONSTANT(CHECK_LESS_THAN_OR_EQUAL);
	BIND_ENUM_CONSTANT(CHECK_GREATER_THAN);
	BIND_ENUM_CONSTANT(CHECK_GREATER_THAN_OR_EQUAL);
	BIND_ENUM_CONSTANT(CHECK_NOT_EQUAL);
}

LimboUtility::LimboUtility() {
	singleton = this;
}

LimboUtility::~LimboUtility() {
	singleton = nullptr;
}
