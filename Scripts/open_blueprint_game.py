"""Open the actual editable gameplay graphs in the Unreal editor."""
import unreal as u
u.EditorPythonScripting.set_keep_python_script_alive(True)
if not u.PogonyaGraphBuilder.open_blueprint_game_graphs():
    raise RuntimeError('Could not open gameplay Blueprint graphs')
u.EditorAssetLibrary.sync_browser_to_objects([
    '/Game/Pogonya/Blueprints/BP_PogonyaCharacter',
    '/Game/Pogonya/Blueprints/BP_PogonyaHUD',
    '/Game/Pogonya/Blueprints/BP_PogonyaGameMode',
])
u.log('POGONYA_GRAPHS_OPEN: character lives, HUD and round rules')
