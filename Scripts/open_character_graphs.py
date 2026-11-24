import unreal as u
asset=u.EditorAssetLibrary.load_asset('/Game/Pogonya/Characters/BP_ChaseCharacter_Graphs')
if asset:
    if not u.get_editor_subsystem(u.AssetEditorSubsystem).open_editor_for_assets([asset]):
        raise RuntimeError('Could not open Blueprint Editor')
    u.log('Opened BP_ChaseCharacter_Graphs. Select 01_Movement through 06_Jump in My Blueprint / Graphs.')
