extends RefCounted
## v26 persistence: unchanged schema-2 checkpoint layout, isolated I/O.
## All methods are stateless so tests and other systems can call them directly.
const MAX_SAVE_BYTES := 250000

static func decode(path: String) -> Dictionary:
 if not FileAccess.file_exists(path):return {}
 var file:=FileAccess.open(path,FileAccess.READ)
 if file==null:return {}
 if file.get_length()>MAX_SAVE_BYTES:
  file.close()
  return {}
 var raw:=file.get_as_text()
 file.close()
 var parsed:Variant=JSON.parse_string(raw)
 if typeof(parsed)!=TYPE_DICTIONARY:return {}
 var data:Dictionary=parsed
 if int(data.get("schema",-1))!=2:return {}
 if not (str(data.get("hero","")) in ["z","nyra"]):return {}
 var coords:Variant=data.get("at",[])
 if not (coords is Array) or coords.size()!=2:return {}
 if not (coords[0] is float or coords[0] is int) or not (coords[1] is float or coords[1] is int):return {}
 if not is_finite(float(coords[0])) or not is_finite(float(coords[1])):return {}
 # Missing modern fields are allowed for historic schema-2 checkpoints.
 for field in ["hp","gold","kills","level","xp","world_clock","world_day","weather_remaining","hero_points"]:
  if not data.has(field):continue
  var value:Variant=data[field]
  if not (value is float or value is int) or not is_finite(float(value)):return {}
 return data

static func commit_checkpoint(data:Dictionary,staging:String,primary:String,backup:String,expected_kills:int,expected_hero:String) -> String:
 var file:=FileAccess.open(staging,FileAccess.WRITE)
 if file==null:return "WRITE FAILED"
 file.store_string(JSON.stringify(data))
 file.flush()
 file.close()
 var staged:Dictionary=decode(staging)
 if staged.is_empty() or int(staged.get("kills",-1))!=expected_kills or str(staged.get("hero",""))!=expected_hero:
  return "INVALID STAGING"
 var primary_abs:=ProjectSettings.globalize_path(primary)
 var staging_abs:=ProjectSettings.globalize_path(staging)
 var backup_abs:=ProjectSettings.globalize_path(backup)
 if FileAccess.file_exists(primary) and not decode(primary).is_empty():
  if DirAccess.copy_absolute(primary_abs,backup_abs)!=OK:return "BACKUP FAILED"
 var result:=DirAccess.rename_absolute(staging_abs,primary_abs)
 if result!=OK:
  # Windows can disallow overwrite-rename: only remove the original if
  # there is a verified backup. Restore it if promotion then fails.
  if FileAccess.file_exists(primary) and not decode(backup).is_empty():
   if DirAccess.remove_absolute(primary_abs)==OK:
    result=DirAccess.rename_absolute(staging_abs,primary_abs)
    if result!=OK:DirAccess.copy_absolute(backup_abs,primary_abs)
 if result!=OK or decode(primary).is_empty():return "COMMIT FAILED"
 return "OK"
