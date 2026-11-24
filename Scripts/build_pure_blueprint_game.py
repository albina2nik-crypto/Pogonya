import unreal as u

if not u.PogonyaGraphBuilder.build_blueprint_game():
    raise RuntimeError('Pure Blueprint build failed. See Saved/PureBlueprintAudit.txt and Saved/BuildPureBlueprints.log.')
u.log('PURE_BLUEPRINT_BUILD_OK')
