extends RefCounted
## Shared version gate. Legacy feature tests must not fail because a newer
## gameplay script reports the current Moonfall release rather than v31 or v32.
static func current_version() -> String:
 var project_name:String = str(ProjectSettings.get_setting("application/config/name", ""))
 var index:int = project_name.rfind(" v")
 if index < 0:
  return ""
 return project_name.substr(index + 2).strip_edges()

static func matches_project(game:Node) -> bool:
 if game == null or not game.has_method("_release_label"):
  return false
 var version:String = current_version()
 if version.is_empty():
  return false
 var prefix:String = "v" + version
 var label:String = str(game.call("_release_label"))
 return label == prefix or label.begins_with(prefix + " ")

static func matches_report(report:Dictionary) -> bool:
 var version:String = current_version()
 return not version.is_empty() and str(report.get("version", "")) == "v" + version
