"""Activate the editable Blueprint game in the existing level, preserving its backup."""
import pathlib
import unreal as u

root = pathlib.Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
backup = root / 'Saved/BlueprintMigrationBackup/before-pure-blueprints/Content/Pogonya/Maps/L_Sector04.umap'
if not backup.is_file():
    raise RuntimeError('Original map backup is required before migration')
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
level.load_level('/Game/Pogonya/Maps/L_Sector04')
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
mode = u.load_class(None, '/Game/Pogonya/Blueprints/BP_PogonyaGameMode.BP_PogonyaGameMode_C')
gate_class = u.load_class(None, '/Game/Pogonya/Blueprints/BP_PogonyaGate.BP_PogonyaGate_C')
if not mode or not gate_class:
    raise RuntimeError('Compile and save Blueprint assets before activating')
world.get_world_settings().set_editor_property('default_game_mode', mode)
prefixes = ('Exit blast door', 'Gate leaf', 'Gate brace', 'Gate diagonal', 'Gate pedestrian door', 'Gate office hatch')
removed = []
existing_gate = None
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if actor.get_class() == gate_class:
        existing_gate = actor
    if isinstance(actor, u.StaticMeshActor) and label.startswith(prefixes):
        removed.append(label)
        actors.destroy_actor(actor)
    elif isinstance(actor, u.TextRenderActor):
        text = str(actor.get_component_by_class(u.TextRenderComponent).get_editor_property('text'))
        if 'EXTRACTION' in text:
            actor.set_actor_location(u.Vector(0, 2735, 600), False, False)
        elif 'AUTHORIZED PERSONNEL' in text:
            actor.set_actor_location(u.Vector(0, 2735, 550), False, False)
if not existing_gate:
    existing_gate = actors.spawn_actor_from_class(gate_class, u.Vector(0, 2828, 0))
    existing_gate.set_actor_label('Blueprint Exit Gate')
    existing_gate.set_folder_path('Exit')
if not level.save_current_level():
    raise RuntimeError('Could not save migrated map')
(root / 'Saved/BlueprintMapMigration.txt').write_text(
    'GameMode: ' + mode.get_path_name() + '\nGate: ' + existing_gate.get_path_name()
    + '\nReplaced static actors:\n' + '\n'.join(removed), encoding='utf-8')
u.log('BLUEPRINT_MAP_ACTIVE')
