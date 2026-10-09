"""Prancha elétrica v3: símbolos/netlist do projeto, renderização pelo KiCad.

Reproduzir:
  uv run --no-project --with cairosvg --with pypdf python \
      experiments/figures/scripts/fig8_circuito.py

KiCad desenha os componentes e os rótulos. A composição SVG apenas acrescenta
moldura/carimbo e recupera o texto editável que o exportador fornece oculto
junto dos contornos de glifos. Não há um segundo desenho do circuito.
"""
import copy
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

import cairosvg
from pypdf import PdfReader, PdfWriter

RAIZ = pathlib.Path(__file__).resolve().parents[3]
os.environ['KAELIX_PCB'] = 'v3'
sys.path.insert(0, str(RAIZ / 'hardware'))
import gen_schematic as SCH
from sexpr import sym, st, dump, head, find1

OUT = RAIZ / 'experiments/figures/output'
EDITAVEL = RAIZ / 'hardware/pcb/v3/prancha'
CLI = shutil.which('kicad-cli') or '/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli'
NS = 'http://www.w3.org/2000/svg'
ET.register_namespace('', NS)
COR = '#272727'
FONTE = 'Arial, Helvetica, DejaVu Sans, sans-serif'

# Cada referência ocorre exatamente uma vez. Posições em mm, só apresentação.
FOLHAS = [
    ('A alimentação integra proteção, regulação e carga por USB-C', {
        'J1': (65, 60), 'Q3': (150, 60), 'U4': (245, 60),
        'C9': (320, 60), 'C10': (375, 60),
        'J3': (80, 140), 'U5': (225, 140), 'C13': (320, 130), 'C14': (375, 130),
        'R11': (60, 210), 'R12': (115, 210), 'R13': (170, 210),
        'RT2': (215, 210), 'C15': (280, 210),
        'R9': (320, 210), 'R10': (375, 210), 'C11': (150, 160),
    }),
    ('O ESP32-S3 conecta aquisição, telemetria LoRa e programação', {
        'U1': (135, 115), 'U3': (300, 108), 'J2': (365, 190),
        'R6': (65, 225), 'R7': (115, 225), 'C1': (165, 225),
        'C2': (210, 225), 'C3': (255, 225), 'C7': (300, 225), 'C8': (375, 145),
    }),
    ('O corte pelo lado positivo desliga sensores e pull-ups juntos', {
        'Q1': (80, 65), 'Q2': (175, 65), 'R4': (245, 65),
        'R5': (310, 65), 'R8': (375, 65),
        'U2': (125, 145), 'R2': (245, 135), 'R3': (310, 135), 'C4': (375, 135),
        'R1': (65, 215), 'RT1': (135, 215), 'C12': (205, 215),
        'C5': (280, 215), 'C6': (350, 215),
    }),
]

DEFS = {r: (lb, sn, fp, val, mapa) for r, lb, sn, fp, val, x, y, mapa
        in SCH.COMPONENTES + SCH.EXTRAS}
DEFS.update({r: ('Device', sn, fp, val, {'1': n1, '2': n2})
             for r, sn, fp, val, x, y, n1, n2 in SCH.PASSIVOS})
refs = [r for _, layout in FOLHAS for r in layout]
if len(refs) != len(set(refs)) or set(refs) != set(DEFS):
    raise RuntimeError('As folhas devem representar todas as referências uma única vez')

original_resolve = SCH.resolve

def resolve_apresentacao(lb, sn):
    definition, pins = original_resolve(lb, sn)
    definition, pins = copy.deepcopy(definition), copy.deepcopy(pins)
    factor = 2.5 if sn in ('ESP32-S3-WROOM-1', 'MPU-6050', 'Ai-Thinker-Ra-02', 'Conn_01x06', 'BQ21040DBV', 'Conn_01x02', 'HT7333') else 1.5
    # Ampliação uniforme dos símbolos nativos; pinagem e biblioteca preservadas.
    def escala(form):
        if not isinstance(form, list):
            return
        tag = head(form)
        if tag in ('at', 'start', 'mid', 'end', 'center', 'xy', 'size'):
            for i in (1, 2):
                if i < len(form) and isinstance(form[i], tuple):
                    form[i] = sym(str(float(form[i][1]) * factor))
        elif tag in ('length', 'radius', 'offset'):
            form[1] = sym(str(float(form[1][1]) * factor))
        elif tag == 'width':
            form[1] = sym('0.25')
        for child in form:
            escala(child)
    escala(definition)
    for pin in pins:
        pin['x'] *= factor
        pin['y'] *= factor
    return definition, pins

SCH.resolve = resolve_apresentacao


def display_value(ref, value):
    match = re.fullmatch(r'(\d+)([kMnu])(\d*)', value)
    if match:
        integer, prefix, fraction = match.groups()
        number = integer + (',' + fraction if fraction else '')
        unit = 'Ω' if ref.startswith('R') else 'F'
        return f"{number} {prefix.replace('u', 'µ')}{unit}"
    return value.replace('10k ', '10 kΩ ').replace('3,7V', '3,7 V').replace('5V', '5 V')


def native_sheet(layout, path):
    SCH.S.clear()
    SCH.LIBSYMS.clear()
    for ref, (x, y) in layout.items():
        lb, sn, fp, value, mapa = DEFS[ref]
        SCH.coloca(ref, lb, sn, fp, display_value(ref, value), x, y, mapa)
        # Campos fora do corpo, calculados da geometria real do símbolo.
        inst = next(e for e in reversed(SCH.S) if head(e) == 'symbol')
        definition, pins = SCH.resolve(lb, sn)
        top = max(p['y'] for p in pins)
        bottom = min(p['y'] for p in pins)
        cx, cy = float(find1(inst, 'at')[1][1]), float(find1(inst, 'at')[2][1])
        for prop in (e for e in inst if head(e) == 'property'):
            if prop[1][1] in ('Reference', 'Value'):
                offset = -top - 8 if prop[1][1] == 'Reference' else -bottom + 9
                if ref not in ('U1', 'U2', 'U3', 'U4', 'U5', 'J3'):
                    px, py = cx + 14, cy + (-3 if prop[1][1] == 'Reference' else 4)
                    find1(prop, 'effects').append([sym('justify'), sym('left')])
                else:
                    px, py = cx + 35, cy + offset
                prop[3] = [sym('at'), sym(str(px)), sym(str(py)), sym('0')]
    def estilo(form):
        if not isinstance(form, list):
            return
        if head(form) == 'font':
            size = find1(form, 'size')
            if size:
                size[1:] = [sym('2.514'), sym('2.514')]
            form.append([sym('face'), st('Arial')])
        if head(form) == 'stroke':
            find1(form, 'width')[1] = sym('0.13')
        for child in form:
            estilo(child)
    # Fontes das bibliotecas são parte da cópia de apresentação, nunca da PCB.
    for e in SCH.S:
        if head(e) == 'global_label':
            at = find1(e, 'at')
            effects = find1(e, 'effects')
            find1(effects, 'justify')[1] = sym('right' if int(at[3][1]) in (180, 270) else 'left')
    for element in list(SCH.LIBSYMS.values()) + SCH.S:
        estilo(element)
    def contornos(form):
        if isinstance(form, list):
            if head(form) == 'stroke':
                find1(form, 'width')[1] = sym('0.25')
            for child in form:
                contornos(child)
    for definition in SCH.LIBSYMS.values():
        contornos(definition)
    doc = [sym('kicad_sch'), [sym('version'), sym('20231120')],
           [sym('generator'), st('kaelix_gen')], [sym('uuid'), st(SCH.ROOT_UUID)],
           [sym('paper'), st('A3')],
           [sym('lib_symbols')] + list(SCH.LIBSYMS.values())] + SCH.S + [
           [sym('sheet_instances'), [sym('path'), st('/'), [sym('page'), st('1')]]]]
    path.write_text(dump(doc) + '\n')


def element(parent, tag, **attributes):
    return ET.SubElement(parent, f'{{{NS}}}{tag}', {k: str(v) for k, v in attributes.items()})


def text(parent, x, y, value, size=2.514):
    e = element(parent, 'text', x=x, y=y, fill=COR,
                **{'font-family': FONTE, 'font-size': size, 'stroke': 'none'})
    e.text = value


def line(parent, x1, y1, x2, y2, width=0.13):
    element(parent, 'line', x1=x1, y1=y1, x2=x2, y2=y2,
            stroke=COR, **{'stroke-width': width})


def compose(svg, title, number):
    root = ET.parse(svg).getroot()
    root.set('width', '420mm')
    root.set('height', '297mm')
    root.set('viewBox', '0 0 420 297')
    for parent in root.iter():
        for child in list(parent):
            if child.get('class') == 'stroked-text':
                parent.remove(child)
    for e in root.iter():
        if e.tag == f'{{{NS}}}title':
            e.text = title
        if e.tag == f'{{{NS}}}text':
            e.set('opacity', '1')
            e.set('stroke-opacity', '0')
            e.set('fill', COR)
            e.set('stroke', 'none')
            e.set('font-family', FONTE)
        for attr in ('style', 'stroke', 'fill'):
            if e.get(attr):
                e.set(attr, e.get(attr).replace('#000000', COR).replace('stroke-width:0.1524', 'stroke-width:0.1300'))
    frame = element(root, 'g')
    element(frame, 'rect', x=20, y=10, width=390, height=277, fill='none',
            stroke=COR, **{'stroke-width': '0.25'})
    for x1,y1,x2,y2 in [(15,148.5,25,148.5), (405,148.5,415,148.5),
                          (210,5,210,15), (210,282,210,292)]:
        line(frame,x1,y1,x2,y2,0.25)
    text(frame, 27, 24, title, 3.5)
    # Carimbo <= 180 mm, sem identificação, data, autor, aprovação ou situação.
    element(frame,'rect',x=230,y=259,width=180,height=28,fill='white',stroke=COR,
            **{'stroke-width':'0.25'})
    text(frame,234,267,'KAELIX — SISTEMA DE MONITORAMENTO',4.9)
    line(frame,230,271,410,271)
    text(frame,234,277,'ESQUEMA ELÉTRICO · SEM ESCALA',3.5)
    text(frame,234,284,f'A3 · folha {number} de {len(FOLHAS)}')
    text(frame,27,247,'1. Rótulos iguais representam a mesma rede em todas as folhas.')
    text(frame,27,253,'2. Valores de projeto [D]; cruzamento sem ponto não é conexão; × indica pino sem conexão.')
    text(frame,27,263,'3. Símbolos e pinagem provenientes da biblioteca eletrônica do KiCad.')
    if number == 1:
        text(frame,27,273,'4. RT2 monitora a placa; conferir a pinagem física do HT7333 antes de fabricar.')
    elif number == 3:
        text(frame,27,273,'4. PERIPH_EN alto liga +3V3_SW; nível baixo ou flutuante desliga os sensores.')
    else:
        text(frame,27,273,'4. O rádio permanece em +3V3 e requer comando de repouso por software.')
    line(frame,27,281,42,281,0.13)
    text(frame,45,282,'Ligação elétrica · linha contínua fina')
    return '\n'.join(line.rstrip() for line in ET.tostring(root,encoding='unicode').splitlines()) + '\n'


def run(*args):
    subprocess.run([CLI, *map(str,args)],check=True)


def check_netlists(paths, work):
    expected = {(r, p, n) for r, (_,_,_,_,m) in DEFS.items() for p,n in m.items()}
    observed = set()
    for path in paths:
        net = work / f'{path.stem}.xml'
        run('sch','export','netlist','--format','kicadxml','--output',net,path)
        tree = ET.parse(net)
        for network in tree.findall('./nets/net'):
            name = network.get('name')
            net_name = SCH.NC if name.startswith('unconnected-') else name.lstrip('/')
            for node in network.findall('node'):
                observed.add((node.get('ref'),node.get('pin'),net_name))
    if observed != expected:
        raise RuntimeError(f'Netlist divergente: faltam {expected-observed}; sobram {observed-expected}')
    print(f'Netlist: {len(DEFS)} componentes e {sum(n != SCH.NC for _,_,n in expected)} conexões e pinos NC conferidos pelo KiCad')


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    EDITAVEL.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='kaelix-prancha-') as temporary:
        work = pathlib.Path(temporary)
        paths = []
        writer = PdfWriter()
        for number,(title,layout) in enumerate(FOLHAS,1):
            name = 'fig8_circuito' if number == 1 else f'fig8_circuito_folha{number}'
            source = EDITAVEL / f'folha{number}.kicad_sch'
            native_sheet(layout,source)
            paths.append(source)
            run('sch','export','svg','--black-and-white','--exclude-drawing-sheet',
                '--default-font','Arial','--output',work,source)
            svg = compose(work / f'{source.stem}.svg',title,number)
            (OUT / f'{name}.svg').write_text(svg)
            pdf = work / f'{name}.pdf'
            cairosvg.svg2pdf(bytestring=svg.encode(),write_to=str(pdf))
            writer.append(PdfReader(pdf))
            subprocess.run(['pdftoppm','-singlefile','-r','300','-png',str(pdf),
                            str(OUT / name)],check=True)
        check_netlists(paths,work)
        writer.write(OUT / 'fig8_circuito.pdf')
    print('Prancha A3: PDF com três folhas, SVG/PNG por folha; PNG a 300 dpi')


if __name__ == '__main__':
    main()
