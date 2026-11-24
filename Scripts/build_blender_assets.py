"""Build editable native Blender asset libraries for Pogonya.
Run with Blender 4.4+: --background --factory-startup --python this_file.
This authors companion source assets; it does not change Unreal assets.
"""
from pathlib import Path
import math, json
import bpy
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Blender'
QA=ROOT/'Saved'/'BlenderQA'
OUT.mkdir(exist_ok=True); QA.mkdir(parents=True,exist_ok=True)
PALETTE={
 'Asphalt':((.075,.085,.09),.25,.12), 'Brick':((.24,.16,.12),.88,0),
 'Concrete':((.29,.30,.28),.88,0), 'Steel':((.09,.12,.12),.4,.78),
 'Container':((.075,.18,.17),.5,.65), 'Rust':((.28,.12,.055),.65,.6),
 'Wood':((.22,.13,.065),.83,0), 'Rubber':((.018,.025,.029),.8,0),
 'Hunter':((.42,.075,.035),.75,0), 'Runner':((.045,.21,.25),.76,0),
 'Mask':((.7,.69,.58),.62,.1), 'Water':((.045,.07,.075),.065,.8),
 'Lamp':((1,.49,.12),.25,.1), 'Red':((.9,.025,.008),.25,.1),
 'Key':((.95,.58,.1),.3,.65), 'Paint':((.79,.61,.24),.7,0),
 'Window':((.12,.3,.32),.22,.45), 'Studio':((.14,.17,.20),.72,0),
}

def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc=bpy.context.scene
    sc.unit_settings.system='METRIC'; sc.unit_settings.scale_length=1
    sc.render.engine='CYCLES'; sc.cycles.samples=32; sc.cycles.use_denoising=True
    sc.cycles.device='CPU'; sc.render.threads_mode='FIXED'; sc.render.threads=6
    sc.render.resolution_x=1280; sc.render.resolution_y=900; sc.render.resolution_percentage=100
    sc.render.image_settings.file_format='PNG'
    sc.world=bpy.data.worlds.new('Studio_World'); sc.world.use_nodes=True
    sc.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.3,.36,.45,1)
    sc.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.4
    sc.view_settings.view_transform='AgX'
    sc.render.film_transparent=False
    mats={}
    for name,(col,rough,metal) in PALETTE.items():
        m=bpy.data.materials.new('M_'+name); m.use_nodes=True; m.diffuse_color=(*col,1); m.use_fake_user=True
        bs=m.node_tree.nodes.get('Principled BSDF')
        bs.inputs['Base Color'].default_value=(*col,1)
        bs.inputs['Roughness'].default_value=rough; bs.inputs['Metallic'].default_value=metal
        if name in ['Lamp','Red']:
            bs.inputs['Emission Color'].default_value=(*col,1); bs.inputs['Emission Strength'].default_value=2
        if name in ['Wood','Concrete','Asphalt','Rust','Container','Hunter','Runner']:
            ns=m.node_tree.nodes; links=m.node_tree.links
            tex=ns.new('ShaderNodeTexNoise'); tex.inputs['Scale'].default_value=24; tex.inputs['Detail'].default_value=2
            tex.location=(-550,0)
            ramp=ns.new('ShaderNodeValToRGB'); ramp.location=(-300,60)
            ramp.color_ramp.elements[0].color=(*(c*.76 for c in col),1)
            ramp.color_ramp.elements[1].color=(*col,1)
            links.new(tex.outputs['Fac'],ramp.inputs['Fac']); links.new(ramp.outputs['Color'],bs.inputs['Base Color'])
            bump=ns.new('ShaderNodeBump'); bump.inputs['Strength'].default_value=.13; bump.inputs['Distance'].default_value=.012
            bump.location=(-260,-170); links.new(tex.outputs['Fac'],bump.inputs['Height']); links.new(bump.outputs['Normal'],bs.inputs['Normal'])
        if name=='Brick':
            image=bpy.data.images.load(str(ROOT/'Scripts'/'Generated_Brick.png'),check_existing=True); image.pack()
            tex=m.node_tree.nodes.new('ShaderNodeTexImage'); tex.image=image; tex.projection='BOX'; tex.projection_blend=.05; tex.location=(-260,20)
            coord=m.node_tree.nodes.new('ShaderNodeTexCoord'); coord.location=(-650,0)
            mapping=m.node_tree.nodes.new('ShaderNodeMapping'); mapping.location=(-460,0); mapping.inputs['Scale'].default_value=(2,2,2)
            m.node_tree.links.new(coord.outputs['Generated'],mapping.inputs['Vector']); m.node_tree.links.new(mapping.outputs['Vector'],tex.inputs['Vector']); m.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color'])
        m.asset_mark(); mats[name]=m
    return mats

def collection(name):
    col=bpy.data.collections.new(name); bpy.context.scene.collection.children.link(col); return col

def place(o,name,col,mat=None,parent=None):
    o.name=name
    for c in list(o.users_collection): c.objects.unlink(o)
    col.objects.link(o)
    if mat: o.data.materials.append(mat)
    if parent: o.parent=parent
    return o

def root(name,col,pos=(0,0,0)):
    o=bpy.data.objects.new(name,None); col.objects.link(o); o.location=pos; o.empty_display_type='PLAIN_AXES'; o.empty_display_size=.25
    o['asset_group']=name; return o

def cube(name,loc,dim,col,mat,parent=None,bevel=.008,rot=(0,0,0)):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc)
    o=bpy.context.object; o.dimensions=dim
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.rotation_euler=rot
    place(o,name,col,mat,parent)
    if bevel:
        mod=o.modifiers.new('Small_edge_bevel','BEVEL'); mod.width=min(bevel,min(dim)*.15); mod.segments=2
    return o

def sphere(name,loc,dim,col,mat,parent=None):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=12,radius=.5,location=loc)
    o=bpy.context.object; o.dimensions=dim; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    place(o,name,col,mat,parent)
    for f in o.data.polygons: f.use_smooth=True
    return o

def cyl(name,loc,radius,depth,col,mat,parent=None,rot=(0,0,0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=radius,depth=depth,location=loc,rotation=rot)
    o=bpy.context.object; place(o,name,col,mat,parent)
    mod=o.modifiers.new('Rim_bevel','BEVEL'); mod.width=min(radius*.07,.008); mod.segments=2
    return o

def torus(name,loc,major,minor,col,mat,parent=None,rot=(0,0,0)):
    bpy.ops.mesh.primitive_torus_add(major_segments=32,minor_segments=12,major_radius=major,minor_radius=minor,location=loc,rotation=rot)
    return place(bpy.context.object,name,col,mat,parent)

def text_obj(text,loc,size,col,mat):
    data=bpy.data.curves.new('Label_'+text,'FONT'); data.body=text; data.size=size; data.align_x='CENTER'; data.extrude=.001
    ob=bpy.data.objects.new('Label_'+text,data); col.objects.link(ob); ob.location=loc; ob.rotation_euler=(math.pi/2,0,0); data.materials.append(mat)
    return ob

def stage(mats,cam_pos,target,ortho,floor_size):
    col=collection('90_Presentation_camera_lights_floor')
    cube('Presentation_floor',(0,0,-.09),(floor_size,floor_size,.14),col,mats['Studio'],bevel=.02)
    camdata=bpy.data.cameras.new('Showcase_Camera'); cam=bpy.data.objects.new('Showcase_Camera',camdata); col.objects.link(cam)
    cam.location=cam_pos; cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    camdata.type='ORTHO'; camdata.ortho_scale=ortho; camdata.lens=50; bpy.context.scene.camera=cam
    for name,pos,power,size,color in [
        ('Key',(-4,-5,8),1700,6,(1,.86,.7)),('Fill',(5,-1,6),1300,5,(.65,.82,1)),('Rim',(0,5,7),1900,4,(.8,.88,1))]:
        data=bpy.data.lights.new(name,'AREA'); data.energy=power; data.shape='DISK'; data.size=size; data.color=color
        ob=bpy.data.objects.new(name,data); col.objects.link(ob); ob.location=pos; ob.rotation_euler=(Vector(target)-ob.location).to_track_quat('-Z','Y').to_euler()
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                space=area.spaces.active; space.shading.type='SOLID'; space.shading.color_type='MATERIAL'; space.overlay.show_extras=False
                space.region_3d.view_perspective='CAMERA'
                space.clip_end=500
    return col

def finish(name,preview,notes):
    sc=bpy.context.scene
    bpy.ops.object.select_all(action='DESELECT')
    meshes=[o for o in sc.objects if o.type=='MESH' and not any(c.name.startswith('90_') for c in o.users_collection)]
    assert meshes
    # Primitive UVs are retained; create a basic unwrap only if a mesh lacks UVs.
    for ob in meshes:
        if not ob.data.uv_layers:
            ob.select_set(True); bpy.context.view_layer.objects.active=ob; bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.uv.smart_project(island_margin=.03); bpy.ops.object.mode_set(mode='OBJECT'); ob.select_set(False)
    first=meshes[0]; first.select_set(True); bpy.context.view_layer.objects.active=first
    info=bpy.data.texts.new('READ_ME_RU'); info.write(notes)
    sc['project']='Pogonya Sector 04'; sc['units']='meters'; sc['companion_assets']=True
    sc['note']='Editable companion Blender models. Existing Unreal prototype meshes are not replaced automatically.'
    for img in bpy.data.images:
        if img.source=='FILE' and not img.packed_file: img.pack()
    filepath=OUT/name
    bpy.ops.wm.save_as_mainfile(filepath=str(filepath),compress=True)
    manifest={'file':name,'blender':bpy.app.version_string,'mesh_objects':len(meshes),'vertices':sum(len(o.data.vertices) for o in meshes),'polygons':sum(len(o.data.polygons) for o in meshes),'materials':[m.name for m in bpy.data.materials],'collections':[c.name for c in bpy.data.collections],'meshes_without_uv':[o.name for o in meshes if not o.data.uv_layers],'packed_images':[i.name for i in bpy.data.images if i.packed_file]}
    (QA/(filepath.stem+'_manifest.json')).write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
    sc.render.filepath=str(QA/preview); bpy.ops.render.render(write_still=True)
    print('POGONYA_SAVED',json.dumps(manifest,ensure_ascii=False),flush=True)

def characters():
    M=reset()
    for role,x in [('Hunter',-1.15),('Runner',1.15)]:
        col=collection('01_'+role if role=='Hunter' else '02_'+role)
        r=root(role+'_ROOT',col,(x,0,0))
        cube(role+'_Coat',(0,0,1.13),(.55,.38,.66),col,M[role],r)
        sphere(role+'_Hood',(0,0,1.59),(.42,.40,.46),col,M['Rubber'],r)
        sphere(role+'_Mask',(0,-.17,1.60),(.29,.10,.27),col,M['Mask'],r)
        cube(role+'_Visor',(0,-.225,1.65),(.26,.025,.055),col,M['Rubber'],r,bevel=.003)
        for side,sign in [('Left',-1),('Right',1)]:
            cube(role+'_'+side+'Leg',(sign*.15,0,.42),(.22,.20,.80),col,M['Rubber'],r)
            cube(role+'_'+side+'Arm',(sign*.36,0,1.11),(.18,.20,.65),col,M[role],r)
            cube(role+'_'+side+'Boot',(sign*.15,-.055,.065),(.23,.32,.13),col,M['Rubber'],r)
        cube(role+'_Backpack',(0,.26,1.16),(.34,.18,.43),col,M['Rubber'],r)
        if role=='Hunter':
            cube('Hunter_Baton',(.40,-.30,.85),(.07,.07,1.05),col,M['Wood'],r,rot=(math.radians(-25),0,0))
        col.asset_mark()
    pickups=collection('03_Pickups_stick_and_key')
    br=root('Stick_ROOT',pickups,(-2.45,.25,0))
    cube('Stick_Wood',(0,0,.525),(.08,.08,1.05),pickups,M['Wood'],br)
    kr=root('Key_ROOT',pickups,(2.5,-.05,.65))
    torus('Key_Ring',(0,0,.18),.10,.025,pickups,M['Key'],kr,rot=(math.pi/2,0,0))
    cube('Key_Shaft',(0,0,-.015),(.035,.035,.28),pickups,M['Key'],kr,bevel=.004)
    cube('Key_Tooth_01',(.045,0,-.10),(.10,.035,.04),pickups,M['Key'],kr,bevel=.004)
    cube('Key_Tooth_02',(.03,0,-.16),(.07,.035,.04),pickups,M['Key'],kr,bevel=.004)
    pickups.asset_mark()
    pres=stage(M,(3.7,-7.5,3.2),(0,0,.95),6.6,200)
    for text,x in [('HUNTER',-1.15),('RUNNER',1.15)]: text_obj(text,(x,-.45,.035),.16,pres,M['Mask'])
    text_obj('STICK',(-2.45,-.12,.035),.11,pres,M['Mask']); text_obj('KEY',(2.5,-.35,.035),.11,pres,M['Mask'])
    finish('Pogonya_Characters.blend','Characters_preview.png',
        'ПОГОНЯ — ПЕРСОНАЖИ И ПРЕДМЕТЫ\n\nДва сегментных персонажа: Hunter и Runner. Палка и ключ находятся в коллекции 03_Pickups_stick_and_key.\nКаждая деталь является отдельным редактируемым Mesh-объектом. Масштаб — метры.\nМатериалы находятся в Material Properties и Shader Editor. Текстура кирпича упакована внутрь файла.\nДля осмотра модели выберите коллекцию персонажа или объект, нажмите точку на цифровой клавиатуре. Tab — редактирование сетки.\n90_Presentation — только камера, свет, подписи и фон для просмотра.\nСкелет и анимационные клипы не входят в этот файл. В UE используется процедурная анимация частей модели.\nЭто отдельные редактируемые Blender-ассеты по дизайну прототипа; автоматической замены мешей в UE не выполнялось.\n')

def environment():
    M=reset()
    c=collection('01_Cargo_Container'); r=root('Container_ROOT',c,(-4,1.7,0))
    cube('Container_Shell',(0,0,1.45),(2.6,6.2,2.9),c,M['Container'],r,bevel=.016)
    for side in [-1,1]:
        for k in range(25): cube('Container_Rib',(side*1.32,-2.94+k*.24,1.45),(.06,.08,2.7),c,M['Container'],r,bevel=.003)
        for z in [.08,2.82]: cube('Container_Edge',(side*1.3,0,z),(.12,6.36,.12),c,M['Rust'],r)
    for x in [-.62,.62]:
        cube('Container_Door',(x,-3.13,1.45),(1.21,.08,2.7),c,M['Container'],r)
        cyl('Door_Locking_Bar',(x,-3.2,1.45),.025,2.45,c,M['Steel'],r)
        cube('Door_Handle',(x+.16,-3.26,1.33),(.32,.08,.06),c,M['Rust'],r)
    c.asset_mark()
    c=collection('02_Wooden_Pallet'); r=root('Pallet_ROOT',c,(-1.5,-2.1,0))
    for k in range(5): cube('Pallet_Slat',(-.48+k*.24,0,.15),(.17,1.10,.09),c,M['Wood'],r)
    for off in [-.42,.42]: cube('Pallet_Foot',(0,off,.07),(1.10,.13,.14),c,M['Wood'],r)
    c.asset_mark()
    c=collection('03_Supply_Crate'); r=root('Crate_ROOT',c,(.15,-2.1,0))
    cube('Crate_Box',(0,0,.45),(.90,.85,.90),c,M['Wood'],r)
    for side in [-1,1]:
        for z in [.09,.81]: cube('Crate_Frame',(0,side*.44,z),(.95,.065,.12),c,M['Wood'],r)
        for x in [-.37,.37]: cube('Crate_Corner',(x,side*.44,.45),(.11,.065,.80),c,M['Wood'],r)
    c.asset_mark()
    c=collection('04_Oil_Drum'); r=root('Drum_ROOT',c,(1.65,-2.1,0))
    cyl('Drum_Body',(0,0,.46),.29,.92,c,M['Rust'],r)
    for z in [.04,.12,.78,.90]: cyl('Drum_Ring',(0,0,z),.305,.035,c,M['Steel'],r)
    cyl('Drum_Cap',(.12,.05,.93),.042,.018,c,M['Steel'],r); c.asset_mark()
    c=collection('05_Rubber_Tire'); r=root('Tire_ROOT',c,(3,-2.1,0))
    torus('Tire',(0,0,.13),.31,.13,c,M['Rubber'],r); c.asset_mark()
    c=collection('06_Brick_Wall_and_Window'); r=root('Wall_ROOT',c,(.4,3.3,0))
    cube('Brick_Wall',(0,0,1.7),(4.2,.32,3.4),c,M['Brick'],r)
    cube('Concrete_Plinth',(0,-.22,.23),(4.25,.2,.46),c,M['Concrete'],r)
    for x in [-2.02,2.02]: cube('Concrete_Pier',(x,-.15,1.7),(.24,.22,3.4),c,M['Concrete'],r)
    cube('Window_Recess',(0,-.185,2.1),(2.2,.05,1.35),c,M['Rubber'],r)
    cube('Window_Glass',(0,-.22,2.1),(2.02,.025,1.20),c,M['Window'],r,bevel=0)
    for z in [1.5,2.1,2.7]: cube('Window_Horizontal_Frame',(0,-.25,z),(2.12,.06,.06),c,M['Steel'],r)
    for x in [-1.04,0,1.04]: cube('Window_Vertical_Frame',(x,-.25,2.1),(.06,.06,1.28),c,M['Steel'],r)
    c.asset_mark()
    c=collection('07_Exit_Gate'); r=root('Gate_ROOT',c,(5,3,0))
    for x in [-1.75,1.75]: cube('Gate_Post',(x,0,1.65),(.24,.42,3.3),c,M['Concrete'],r)
    cube('Gate_Lintel',(0,0,3.35),(3.8,.42,.24),c,M['Concrete'],r)
    for x in [-.83,.83]:
        cube('Gate_Leaf',(x,0,1.55),(1.62,.12,3.05),c,M['Rust'],r)
        for z in [.3,2.8]: cube('Gate_Brace',(x,-.10,z),(1.58,.07,.09),c,M['Steel'],r)
        cube('Gate_Diagonal',(x,-.12,1.55),(2.6,.06,.08),c,M['Steel'],r,rot=(0,math.radians(-53),0))
    for x in [-1.25,1.25]: cube('Gate_Lamp',(x,-.3,3.48),(.5,.25,.14),c,M['Lamp'],r)
    c.asset_mark()
    c=collection('08_Pipe_Section'); r=root('Pipe_ROOT',c,(4.7,.2,0))
    for x in [-1.25,1.25]: cyl('Pipe_Support',(x,0,1.35),.065,2.7,c,M['Steel'],r)
    for y in [-.12,.12]: cyl('Overhead_Pipe',(0,y,2.75),.07,2.8,c,M['Rust'],r,rot=(0,math.pi/2,0))
    c.asset_mark()
    c=collection('09_Material_Samples')
    for i,key in enumerate(['Asphalt','Brick','Concrete','Steel','Container','Rust','Wood','Rubber','Water','Paint']):
        sphere('Sample_'+key,(-4.5+i*1.0,-4.7,.22),(.42,.42,.42),c,M[key])
    pres=stage(M,(12,-19,13),(0,.25,1.0),18.5,200)
    for name,pos in [('PALLET',(-1.5,-2.9,.03)),('CRATE',(.15,-2.9,.03)),('DRUM',(1.65,-2.9,.03)),('TIRE',(3,-2.9,.03))]: text_obj(name,pos,.13,pres,M['Mask'])
    finish('Pogonya_Environment.blend','Environment_preview.png',
        'ПОГОНЯ — ОКРУЖЕНИЕ И МАТЕРИАЛЫ\n\nКоллекции 01–08: контейнер, поддон, ящик, бочка, шина, кирпичная стена с окном, ворота и секция труб.\nКаждая модель состоит из редактируемых Mesh-объектов. Родительский объект ROOT перемещает модель целиком. Масштаб — метры.\n09_Material_Samples — образцы материалов. M_Brick содержит упакованную текстуру; остальные поверхности используют цвет и процедурные узлы.\nВыберите объект: Tab — редактирование сетки, Material Properties — материал, Shader Editor — узлы материала.\n90_Presentation — фон, камера, свет и подписи для просмотра, не часть игровых моделей.\nКоллекции и материалы отмечены как Blender Assets. Файл открывается в Blender 4.4 или новее.\nЭто отдельный набор ассетов, а не экспорт полной карты Unreal. Он не изменяет файлы UE.\n')

characters()
environment()
