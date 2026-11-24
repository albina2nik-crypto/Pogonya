import unreal as u
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
level.load_level('/Game/Pogonya/Maps/L_Sector04')
editor=u.get_editor_subsystem(u.UnrealEditorSubsystem)
world=editor.get_editor_world()
settings=world.get_world_settings()
tags=list(settings.get_editor_property('tags'))
marker=u.Name('Pogonya_RotationsV2')
if marker not in tags:
    for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        if isinstance(actor,u.StaticMeshActor) and not actor.get_actor_label().startswith('Overhead pipe'):
            r=actor.get_actor_rotation()
            actor.set_actor_rotation(u.Rotator(pitch=r.roll,yaw=r.pitch,roll=r.yaw),False)
        if isinstance(actor,u.PlayerStart):
            actor.set_actor_rotation(u.Rotator(pitch=0,yaw=90,roll=0),False)
    tags.append(marker)
    settings.set_editor_property('tags',tags)
editor.set_level_viewport_camera_info(u.Vector(140,-2450,300),u.Rotator(pitch=-6,yaw=87,roll=0))
level.save_current_level()
u.log('POGONYA: rotation repair saved')
