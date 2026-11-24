#!/usr/bin/env python3
"""Regenerate the ready-stacked three-panel selection and patch."""
import json
import math
from pathlib import Path
root = Path(__file__).resolve().parents[1]
VERSION = json.loads((root/'plugin.json').read_text())['version']
# Order follows CE2600::Params; values are each control's default.
defaults = []
for n in range(3):
    defaults += [-1 if n == 1 else 0, 0, .5, 0, 0, .65 if n == 0 else 0, 0]
defaults += [math.log2(2400),.15,1.5,1,0,0,0,0,1,0,.7,0,1,1,0,.3,.35,1,1,.25,.5,.65,.45,.15,.5,0,.18,.65]
defaults += [0]*6 + [1]*3 + [0,0,1,0,0,1,.5,1,1] + [1]*3 + [1,1]
# Preamp range x100, keyboard envelope gate, power on, right reverb level.
defaults += [1, 0, 1, .18]
defaults += [0,0,0,1,.4,0,0,1,0,0,0,1,0,.7,1]

# A Rack selection inserts the three panels together into the current patch.
modules = []
for row, name in enumerate(['Top','Middle','Bottom']):
    params = [{'id':i,'value':v} for i,v in enumerate(defaults)]
    modules.append({'id':row+1,'plugin':'CE2600','model':'CE2600Orange'+name,'version':VERSION,'params':params,'data':{'instrumentLink':1},'pos':[0,row]})
for extension in ['vcv','vcvs']:
    patch = {'version':'2.6.6','modules':modules,'cables':[]}
    if extension == 'vcv':
        patch['zoom'] = .5  # complete ready-stacked patch, with no audio interface selected
    (root/f'res/CE2600 Orange - Three panels.{extension}').write_text(json.dumps(patch,indent=2)+'\n')
