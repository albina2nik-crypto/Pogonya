"""Create an inspectable Blueprint child without replacing gameplay or the map."""
import unreal as u
path='/Game/Pogonya/Characters/BP_ChaseCharacter'
library=u.EditorAssetLibrary
if library.does_asset_exist(path):
    bp=library.load_asset(path)
else:
    library.make_directory('/Game/Pogonya/Characters')
    factory=u.BlueprintFactory()
    factory.set_editor_property('parent_class',u.load_class(None,'/Script/Pogonya.ChaseCharacter'))
    bp=u.AssetToolsHelpers.get_asset_tools().create_asset('BP_ChaseCharacter','/Game/Pogonya/Characters',u.Blueprint,factory)
    if not bp:raise RuntimeError('Could not create character Blueprint')
u.BlueprintEditorLibrary.compile_blueprint(bp)
library.save_loaded_asset(bp)
u.log('Character Blueprint saved: '+path+'; gameplay is inherited from native ChaseCharacter.')
