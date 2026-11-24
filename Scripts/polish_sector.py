"""Refine only materials and lighting on the generated level; preserve geometry."""
import unreal as u
import os, math, random, struct, zlib
R='/Game/Pogonya'
assets=u.EditorAssetLibrary
edit=u.MaterialEditingLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
level.load_level(R+'/Maps/L_Sector04')
for a in actors.get_all_level_actors():
    if isinstance(a,u.DirectionalLight):
        a.set_actor_rotation(u.Rotator(pitch=-38,yaw=-25,roll=0),False)
        a.get_component_by_class(u.DirectionalLightComponent).set_editor_property('intensity',1.4)
    if a.get_actor_label().startswith('Overhead pipe'):
        a.set_actor_rotation(u.Rotator(pitch=90,yaw=0,roll=0),False)
    if isinstance(a,u.TextRenderActor):
        a.set_actor_rotation(u.Rotator(pitch=0,yaw=-90,roll=0),False)
    if isinstance(a,u.PointLight):
        c=a.get_component_by_class(u.PointLightComponent)
        c.set_editor_property('intensity_units',u.LightUnits.LUMENS)
    if isinstance(a,u.PostProcessVolume):
        s=a.get_editor_property('settings')
        s.set_editor_property('auto_exposure_min_brightness',2.5)
        s.set_editor_property('auto_exposure_max_brightness',2.5)
        a.set_editor_property('settings',s)

existing={str(a.get_actor_label()) for a in actors.get_all_level_actors()}
for i,y in enumerate([-1500,900]):
    label='Moon bounce '+str(i)
    if label in existing:continue
    a=actors.spawn_actor_from_class(u.PointLight,u.Vector(0,y,600))
    a.set_actor_label(label)
    a.set_folder_path('Lighting')
    c=a.get_component_by_class(u.PointLightComponent)
    c.set_editor_property('intensity_units',u.LightUnits.LUMENS)
    c.set_editor_property('intensity',8000)
    c.set_editor_property('attenuation_radius',2700)
    c.set_editor_property('cast_shadows',False)
    c.set_light_color(u.LinearColor(.18,.35,.55,1))

# Reduce contrast of the generated procedural weathering.
colors={'Asphalt':(.075,.085,.09),'Brick':(.24,.16,.12),'Concrete':(.29,.30,.28),'Steel':(.09,.12,.12),'Container':(.075,.18,.17),'Rust':(.28,.12,.055),'Wood':(.22,.13,.065),'Hunter':(.42,.075,.035),'Runner':(.045,.21,.25),'Paint':(.79,.61,.24)}
# Locate expression objects via the material's editor-only expression collection.
for name,col in colors.items():
    m=assets.load_asset(R+'/Materials/M_'+name)
    expressions=[]
    queue=[edit.get_material_property_input_node(m,u.MaterialProperty.MP_BASE_COLOR)]
    while queue:
        ex=queue.pop()
        if ex is None or ex in expressions:continue
        expressions.append(ex)
        queue.extend(edit.get_inputs_for_material_expression(m,ex))
    for ex in expressions:
        if isinstance(ex,u.MaterialExpressionNoise):
            ex.set_editor_property('scale',.20)
            ex.set_editor_property('output_min',0.0)
            ex.set_editor_property('output_max',1.0)
        if isinstance(ex,u.MaterialExpressionConstant3Vector):
            val=ex.get_editor_property('constant')
            if max(val.r,val.g,val.b)<max(col)*.6:
                ex.set_editor_property('constant',u.LinearColor(*(x*.74 for x in col),1))
    edit.recompile_material(m)
    assets.save_loaded_asset(m)

# Tileable brick albedo made from code, imported as a normal Unreal texture.
random.seed(41)
N=512
raw=bytearray()
for y in range(N):
    raw.append(0)
    row=y//32
    for x in range(N):
        xx=(x+(64 if row%2 else 0))%128
        mortar=y%32<2 or xx<3
        grain=random.uniform(.78,1.2)
        brick=(math.sin((x//128+row*5)*17.3)+1)*.09+.85
        base=(60,57,49) if mortar else (91,57,39)
        raw.extend(max(0,min(255,int(c*grain*brick))) for c in base)
def chunk(k,data):return struct.pack('>I',len(data))+k+data+struct.pack('>I',zlib.crc32(k+data)&0xffffffff)
png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',N,N,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(bytes(raw)))+chunk(b'IEND',b'')
source=os.path.join(u.Paths.project_dir(),'Scripts','Generated_Brick.png')
with open(source,'wb') as f:f.write(png)
task=u.AssetImportTask()
task.set_editor_property('filename',source)
task.set_editor_property('destination_path',R+'/Textures')
task.set_editor_property('destination_name','T_Brick')
task.set_editor_property('automated',True)
task.set_editor_property('replace_existing',True)
task.set_editor_property('save',True)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
tex=assets.load_asset(R+'/Textures/T_Brick')
m=assets.load_asset(R+'/Materials/M_Brick')
pos=edit.create_material_expression(m,u.MaterialExpressionWorldPosition,-900,500)
mask=edit.create_material_expression(m,u.MaterialExpressionComponentMask,-700,500)
mask.set_editor_property('r',False);mask.set_editor_property('g',True);mask.set_editor_property('b',True);mask.set_editor_property('a',False)
edit.connect_material_expressions(pos,'',mask,'Input')
scale=edit.create_material_expression(m,u.MaterialExpressionMultiply,-500,500)
scale.set_editor_property('const_b',.0025)
edit.connect_material_expressions(mask,'',scale,'A')
sample=edit.create_material_expression(m,u.MaterialExpressionTextureSample,-250,500)
sample.set_editor_property('texture',tex)
edit.connect_material_expressions(scale,'',sample,'UVs')
edit.connect_material_property(sample,'RGB',u.MaterialProperty.MP_BASE_COLOR)
edit.recompile_material(m)
assets.save_loaded_asset(m)
level.save_current_level()
u.log('POGONYA: lighting and surface polish saved')
