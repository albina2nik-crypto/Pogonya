import unreal as u
if not u.PogonyaGraphBuilder.build_character_graphs():
    raise RuntimeError('Blueprint graph creation failed; see BlueprintGraphAudit.txt and Unreal log.')
