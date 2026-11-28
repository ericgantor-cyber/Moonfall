"""Install regional fishing banks in DeepWatersPreview only.
Select authored, axis-aligned water PhysicsVolumes already tagged
MoonfallFishingProvince_0 through MoonfallFishingProvince_8 (one per province).
Does not save maps or modify existing actors. Inspect geometry before saving.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from deep_waters_layout import regional_water_plan, regional_environment_plan, REGIONAL_HABITATS

# Opt in only after inspecting the swimming space; no forced combat.
ENABLE_WILDLIFE = False

def main():
    import unreal
    import math
    actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world=unreal.EditorLevelLibrary.get_editor_world()
    if world is None or not world.get_path_name().startswith("/Game/DeepWaters/Maps/DeepWatersPreview."):
        raise RuntimeError("Open DeepWatersPreview")
    mode=getattr(unreal,"MoonfallNorthPreviewGameMode",None)
    treasure=getattr(unreal,"MoonfallGrottoTreasure",None)
    enemy=getattr(unreal,"MoonfallEnemy",None)
    if ENABLE_WILDLIFE and enemy is None:
        raise RuntimeError("Compile aquatic enemy class first")
    if mode is None or treasure is None or world.get_world_settings().get_editor_property("default_game_mode")!=mode:
        raise RuntimeError("Compile and select isolated preview game mode")
    selected=actors.get_selected_level_actors()
    if not selected:
        raise RuntimeError("Select tagged water volumes")
    plans=[]
    used=set()
    all_actors=actors.get_all_level_actors()
    for volume in selected:
        if not isinstance(volume,unreal.PhysicsVolume) or not volume.get_editor_property("water_volume"):
            raise RuntimeError("Selection must contain only water PhysicsVolumes")
        tags=[str(t) for t in volume.get_editor_property("tags")]
        matched=[i for i in range(9) if f"MoonfallFishingProvince_{i}" in tags]
        if len(matched)!=1 or matched[0] in used:
            raise RuntimeError("Exactly one distinct province tag per selected volume")
        province=matched[0]
        tag=f"MoonfallRegionalWaterInstalled_{province}"
        if any(tag in [str(t) for t in a.get_editor_property("tags")] for a in all_actors):
            raise RuntimeError("Region already installed")
        for other in all_actors:
            if other!=volume and isinstance(other,unreal.PhysicsVolume) and f"MoonfallFishingProvince_{province}" in [str(t) for t in other.get_editor_property("tags")]:
                raise RuntimeError("Duplicate province volume")
        rot=volume.get_actor_rotation()
        if any(abs(v)>.01 for v in (rot.pitch,rot.yaw,rot.roll)):
            raise RuntimeError("Axis-aligned water only")
        origin,extent=volume.get_actor_bounds(False)
        if not all(math.isfinite(v) for v in (origin.x,origin.y,origin.z,extent.x,extent.y,extent.z)):
            raise RuntimeError("Invalid bounds")
        shore,nodes=regional_water_plan(province,extent.x,extent.y,extent.z*2)
        used.add(province)
        environment=regional_environment_plan(province,extent.x,extent.y,extent.z*2)
        plans.append((province,tag,origin,extent,shore,nodes,environment))
    meshes={name:unreal.EditorAssetLibrary.load_asset(f"/Engine/BasicShapes/{name}.{name}") for name in ("Cube","Plane","Cone","Sphere","Cylinder")}
    material=unreal.EditorAssetLibrary.load_asset("/Game/DeepWaters/Materials/M_DeepWatersSurface")
    if any(v is None for v in meshes.values()) or material is None:
        raise RuntimeError("Required meshes or water surface material missing")
    created=[]
    try:
        for province,tag,origin,extent,shore,nodes,environment in plans:
            surface=origin.z+extent.z
            def place(cls,label,pos):
                x,y,z=pos
                actor=actors.spawn_actor_from_class(cls,unreal.Vector(origin.x+x,origin.y+y,surface+z),unreal.Rotator(0,0,0))
                if actor is None:
                    raise RuntimeError("Spawn failed: "+label)
                created.append(actor)
                actor.set_actor_label(REGIONAL_HABITATS[province]+" "+label)
                actor.set_editor_property("tags",[unreal.Name(tag)])
                return actor
            water=place(unreal.StaticMeshActor,"water surface",(0,0,0))
            mesh=water.get_editor_property("static_mesh_component")
            mesh.set_static_mesh(meshes["Plane"])
            mesh.set_material(0,material)
            mesh.set_collision_profile_name("NoCollision")
            water.set_actor_scale3d(unreal.Vector(extent.x/50,extent.y/50,1))
            for label,pos,scale in shore:
                bank=place(unreal.StaticMeshActor,label,pos)
                mesh=bank.get_editor_property("static_mesh_component")
                mesh.set_static_mesh(meshes["Cube"])
                mesh.set_collision_profile_name("BlockAll")
                bank.set_actor_scale3d(unreal.Vector(*scale))
            for index,label,shape,pos,scale in nodes:
                node=place(treasure,label,pos)
                node.set_editor_property("resource_index",index)
                mesh=node.get_editor_property("chest_mesh")
                mesh.set_static_mesh(meshes[shape])
                mesh.set_relative_scale3d(unreal.Vector(*scale))
            ornaments,light_pos,color,wildlife=environment
            for label,shape,pos,scale in ornaments:
                ornament=place(unreal.StaticMeshActor,label,pos)
                mesh=ornament.get_editor_property("static_mesh_component")
                mesh.set_static_mesh(meshes[shape])
                mesh.set_collision_profile_name("NoCollision")
                mesh.set_editor_property("cast_shadow",False)
                ornament.set_actor_scale3d(unreal.Vector(*scale))
            beacon=place(unreal.PointLight,"habitat light",light_pos)
            light=beacon.get_component_by_class(unreal.PointLightComponent)
            if light is None:
                raise RuntimeError("Missing habitat light component")
            light.set_light_color(unreal.LinearColor(*color,1.0))
            light.set_intensity(1200.0)
            light.set_attenuation_radius(400.0)
            light.set_editor_property("cast_shadows",False)
            if ENABLE_WILDLIFE and wildlife is not None:
                profile,pos=wildlife
                creature=place(enemy,"aquatic guardian",pos)
                creature.set_editor_property("tags",[unreal.Name(tag),unreal.Name("MoonfallAquaticPreview")])
                creature.set_editor_property("aquatic_profile_id",profile)
    except Exception:
        for actor in reversed(created):
            actors.destroy_actor(actor)
        raise
    unreal.log("Regional banks installed. Inspect dry approach, floor and swim exits before manually saving.")

if __name__=="__main__":
    main()
